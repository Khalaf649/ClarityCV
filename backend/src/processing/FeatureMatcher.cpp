#include "processing/FeatureMatcher.hpp"
#include "processing/SIFTProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>

namespace processing {

// ============================================================================
// Public entry point
// ============================================================================

FeatureMatchingResult FeatureMatcher::match(const cv::Mat& img1,
                                             const cv::Mat& img2,
                                             const FeatureMatchingParams& params)
{
    using Clock = std::chrono::high_resolution_clock;

    FeatureMatchingResult result;

    if (img1.empty() || img2.empty()) {
        return result;
    }

    // ── Step 1: Run SIFT on both images independently ──────────────────────
    std::vector<cv::KeyPoint> kp1, kp2;
    cv::Mat desc1, desc2;

    runSIFT(img1, params, kp1, desc1, result.siftTimeImg1Ms);
    runSIFT(img2, params, kp2, desc2, result.siftTimeImg2Ms);

    result.keypointsImg1 = static_cast<int>(kp1.size());
    result.keypointsImg2 = static_cast<int>(kp2.size());

    // ── Step 2: Match descriptors ──────────────────────────────────────────
    std::vector<cv::DMatch> goodMatches;

    if (desc1.empty() || desc2.empty()) {
        // No descriptors — return side-by-side with no lines
        cv::hconcat(img1, img2, result.image);
        result.matchesCount    = 0;
        result.computationTimeMs = result.siftTimeImg1Ms + result.siftTimeImg2Ms;
        return result;
    }

    auto matchStart = Clock::now();

    if (params.method == MatchingMethod::NCC) {
        goodMatches = matchNCC(desc1, desc2, params);
    } else {
        goodMatches = matchSSD(desc1, desc2, params);
    }

    auto matchEnd = Clock::now();
    long long matchTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        matchEnd - matchStart).count();

    result.computationTimeMs = matchTimeMs;
    result.matchesCount      = static_cast<int>(goodMatches.size());

    // ── Step 3: Draw matches ───────────────────────────────────────────────
    result.image = drawMatchesImage(img1, kp1, img2, kp2, goodMatches);

    return result;
}

// ============================================================================
// SIFT helper: extract keypoints & descriptors from a single image
// ============================================================================

void FeatureMatcher::runSIFT(const cv::Mat& img,
                              const FeatureMatchingParams& params,
                              std::vector<cv::KeyPoint>& keypoints,
                              cv::Mat& descriptors,
                              long long& timeMs)
{
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();

    // Build SIFT params (reuse the existing SIFTProcessor pipeline)
    SIFTParams sp;
    sp.contrastThreshold = params.contrastThreshold;
    sp.nfeatures         = params.nfeatures;
    sp.drawKeypoints     = false; // We only need keypoints + descriptors

    SIFTResult sr = SIFTProcessor::apply(img, sp);

    keypoints   = sr.keypoints;
    descriptors = sr.descriptors;

    auto t1 = Clock::now();
    timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

// ============================================================================
// SSD Matching
//   For each descriptor in desc1 find the two nearest neighbours in desc2
//   (by Euclidean / L2 distance = sqrt(SSD)) then apply Lowe's ratio test.
// ============================================================================

std::vector<cv::DMatch> FeatureMatcher::matchSSD(const cv::Mat& desc1,
                                                   const cv::Mat& desc2,
                                                   const FeatureMatchingParams& params)
{
    // desc1, desc2 are CV_32F matrices of shape (N x 128)
    cv::Mat d1, d2;
    desc1.convertTo(d1, CV_32F);
    desc2.convertTo(d2, CV_32F);

    std::vector<cv::DMatch> goodMatches;
    int n1 = d1.rows;
    int n2 = d2.rows;

    for (int i = 0; i < n1; ++i) {
        float bestDist  = std::numeric_limits<float>::max();
        float secondDist= std::numeric_limits<float>::max();
        int   bestIdx   = -1;

        const float* row1 = d1.ptr<float>(i);

        for (int j = 0; j < n2; ++j) {
            const float* row2 = d2.ptr<float>(j);

            // Compute SSD (squared Euclidean distance) between the two 128-D vectors
            float ssd = 0.0f;
            for (int k = 0; k < d1.cols; ++k) {
                float diff = row1[k] - row2[k];
                ssd += diff * diff;
            }

            if (ssd < bestDist) {
                secondDist = bestDist;
                bestDist   = ssd;
                bestIdx    = j;
            } else if (ssd < secondDist) {
                secondDist = ssd;
            }
        }

        // Lowe's ratio test: accept match only if best is significantly closer than second-best
        // Compare sqrt(SSD) ratios: equivalent to comparing SSD ratios
        if (bestIdx >= 0 && bestDist < params.ratioThreshold * params.ratioThreshold * secondDist) {
            cv::DMatch m;
            m.queryIdx = i;
            m.trainIdx = bestIdx;
            m.distance = std::sqrt(bestDist); // store as L2 distance
            goodMatches.push_back(m);
        }
    }

    // Sort by distance and cap at maxMatches
    std::sort(goodMatches.begin(), goodMatches.end(),
              [](const cv::DMatch& a, const cv::DMatch& b){ return a.distance < b.distance; });

    if (static_cast<int>(goodMatches.size()) > params.maxMatches) {
        goodMatches.resize(params.maxMatches);
    }

    return goodMatches;
}

// ============================================================================
// NCC Matching
//   Treat each 128-D SIFT descriptor as a signal vector and compute the
//   normalised cross-correlation (NCC) score ∈ [-1, 1].  A higher score
//   means a better match.  We select the best & second-best match per row
//   and apply a ratio test on the *distance* (1 - NCC).
// ============================================================================

std::vector<cv::DMatch> FeatureMatcher::matchNCC(const cv::Mat& desc1,
                                                   const cv::Mat& desc2,
                                                   const FeatureMatchingParams& params)
{
    cv::Mat d1, d2;
    desc1.convertTo(d1, CV_32F);
    desc2.convertTo(d2, CV_32F);

    // Pre-compute L2 norms for every row in d2
    std::vector<float> norms2(d2.rows, 0.0f);
    for (int j = 0; j < d2.rows; ++j) {
        const float* r = d2.ptr<float>(j);
        float sq = 0.0f;
        for (int k = 0; k < d2.cols; ++k) sq += r[k] * r[k];
        norms2[j] = std::sqrt(sq);
    }

    std::vector<cv::DMatch> goodMatches;
    int n1 = d1.rows;
    int n2 = d2.rows;

    for (int i = 0; i < n1; ++i) {
        const float* row1 = d1.ptr<float>(i);

        // Compute norm of row 1
        float norm1 = 0.0f;
        for (int k = 0; k < d1.cols; ++k) norm1 += row1[k] * row1[k];
        norm1 = std::sqrt(norm1);

        float bestNCC   = -2.0f; // best NCC score (higher is better)
        float secondNCC = -2.0f;
        int   bestIdx   = -1;

        for (int j = 0; j < n2; ++j) {
            float denom = norm1 * norms2[j];
            if (denom < 1e-10f) continue;

            const float* row2 = d2.ptr<float>(j);

            // Dot product
            float dot = 0.0f;
            for (int k = 0; k < d1.cols; ++k) dot += row1[k] * row2[k];

            float ncc = dot / denom; // normalised cross-correlation ∈ [-1, 1]

            if (ncc > bestNCC) {
                secondNCC = bestNCC;
                bestNCC   = ncc;
                bestIdx   = j;
            } else if (ncc > secondNCC) {
                secondNCC = ncc;
            }
        }

        // Convert NCC to a distance metric: dist = 1 - NCC  (lower is better)
        // Apply Lowe's ratio test in distance space
        float bestDist   = 1.0f - bestNCC;
        float secondDist = 1.0f - secondNCC;

        if (bestIdx >= 0 && secondDist > 1e-10f &&
            bestDist < params.ratioThreshold * secondDist)
        {
            cv::DMatch m;
            m.queryIdx = i;
            m.trainIdx = bestIdx;
            m.distance = bestDist;
            goodMatches.push_back(m);
        }
    }

    // Sort by distance (best NCC first)
    std::sort(goodMatches.begin(), goodMatches.end(),
              [](const cv::DMatch& a, const cv::DMatch& b){ return a.distance < b.distance; });

    if (static_cast<int>(goodMatches.size()) > params.maxMatches) {
        goodMatches.resize(params.maxMatches);
    }

    return goodMatches;
}

// ============================================================================
// Draw matches side by side
// ============================================================================

cv::Mat FeatureMatcher::drawMatchesImage(const cv::Mat& img1,
                                          const std::vector<cv::KeyPoint>& kp1,
                                          const cv::Mat& img2,
                                          const std::vector<cv::KeyPoint>& kp2,
                                          const std::vector<cv::DMatch>& matches)
{
    // Ensure both images are BGR (3-channel) for drawing
    cv::Mat bgr1, bgr2;
    if (img1.channels() == 1) cv::cvtColor(img1, bgr1, cv::COLOR_GRAY2BGR);
    else                       bgr1 = img1.clone();
    if (img2.channels() == 1) cv::cvtColor(img2, bgr2, cv::COLOR_GRAY2BGR);
    else                       bgr2 = img2.clone();

    cv::Mat output;
    cv::drawMatches(
        bgr1, kp1,
        bgr2, kp2,
        matches,
        output,
        cv::Scalar(0, 255, 0),    // match line colour: green
        cv::Scalar(0, 0, 255),    // single-point colour: red (unmatched)
        std::vector<char>(),
        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS
    );

    return output;
}

} // namespace processing
