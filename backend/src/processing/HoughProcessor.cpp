#include "processing/HoughProcessor.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace processing {

// ============================================================================
// HOUGH LINES
// ============================================================================

namespace {

struct SinCosTable {
    std::vector<double> cosTheta;
    std::vector<double> sinTheta;
    int numTheta;
    double thetaStep;

    SinCosTable(int count, double step)
        : numTheta(count), thetaStep(step) {
        cosTheta.resize(numTheta);
        sinTheta.resize(numTheta);
        for (int t = 0; t < numTheta; ++t) {
            double angle = t * thetaStep;
            cosTheta[t] = std::cos(angle);
            sinTheta[t] = std::sin(angle);
        }
    }
};

std::vector<HoughLine> houghLinesAdvanced(
    const cv::Mat& edges,
    double rhoRes,
    double thetaRes,
    int threshold,
    int nmsWindowSize,
    int borderMargin)
{
    const int width     = edges.cols;
    const int height    = edges.rows;
    const int numTheta  = std::max(1, cvRound(CV_PI / thetaRes));
    const int maxRho    = cvRound(std::hypot(width, height));
    const int numRho    = (maxRho * 2) + 1;
    const int rhoOffset = maxRho;

    cv::Mat acc = cv::Mat::zeros(numRho, numTheta, CV_32SC1);
    SinCosTable table(numTheta, thetaRes);

    for (int y = borderMargin; y < height - borderMargin; ++y) {
        const uchar* row = edges.ptr<uchar>(y);
        for (int x = borderMargin; x < width - borderMargin; ++x) {
            if (row[x] == 0) continue;
            for (int t = 0; t < numTheta; ++t) {
                int rhoIdx = cvRound(
                    (x * table.cosTheta[t] + y * table.sinTheta[t]) / rhoRes)
                    + rhoOffset;
                if (rhoIdx >= 0 && rhoIdx < numRho)
                    acc.at<int>(rhoIdx, t) += 1;
            }
        }
    }

    int nmsRadius = nmsWindowSize / 2;
    cv::Mat suppressed = acc.clone();

    for (int r = 0; r < numRho; ++r) {
        for (int t = 0; t < numTheta; ++t) {
            int val = acc.at<int>(r, t);
            if (val < threshold) { suppressed.at<int>(r, t) = 0; continue; }
            bool isLocalMax = true;
            for (int dr = -nmsRadius; dr <= nmsRadius && isLocalMax; ++dr) {
                for (int dt = -nmsRadius; dt <= nmsRadius && isLocalMax; ++dt) {
                    int rr = r + dr;
                    int tt = (t + dt + numTheta) % numTheta;
                    if (rr < 0 || rr >= numRho) continue;
                    if (acc.at<int>(rr, tt) > val) isLocalMax = false;
                }
            }
            if (!isLocalMax) suppressed.at<int>(r, t) = 0;
        }
    }

    std::vector<HoughLine> lines;
    for (int r = 0; r < numRho; ++r) {
        for (int t = 0; t < numTheta; ++t) {
            int vote = suppressed.at<int>(r, t);
            if (vote >= threshold)
                lines.push_back(HoughLine((r - rhoOffset) * rhoRes, t * thetaRes, vote));
        }
    }

    std::sort(lines.begin(), lines.end(),
              [](const HoughLine& a, const HoughLine& b) { return a.votes > b.votes; });
    return lines;
}

std::vector<HoughLine> removeDuplicateLines(
    const std::vector<HoughLine>& lines,
    double angleToleranceDeg,
    double distTolerance)
{
    if (lines.empty()) return lines;
    std::vector<bool> marked(lines.size(), false);
    std::vector<HoughLine> unique;
    unique.reserve(lines.size());
    double angleTolRad = angleToleranceDeg * CV_PI / 180.0;

    for (size_t i = 0; i < lines.size(); ++i) {
        if (marked[i]) continue;
        const HoughLine& ref = lines[i];
        unique.push_back(ref);
        marked[i] = true;
        for (size_t j = i + 1; j < lines.size(); ++j) {
            if (marked[j]) continue;
            const HoughLine& cand = lines[j];
            double angleDiff = std::abs(ref.theta - cand.theta);
            if (angleDiff > CV_PI / 2.0) angleDiff = CV_PI - angleDiff;
            if (angleDiff > angleTolRad) continue;
            if (std::abs(ref.rho - cand.rho) <= distTolerance)
                marked[j] = true;
        }
    }
    return unique;
}

std::vector<HoughLine> linesToSegments(
    const cv::Mat& edges,
    const std::vector<HoughLine>& lines,
    double minLineLength)
{
    const int width  = edges.cols;
    const int height = edges.rows;
    std::vector<HoughLine> segments;

    for (auto line : lines) {
        double cosT = std::cos(line.theta);
        double sinT = std::sin(line.theta);
        std::vector<std::pair<double, cv::Point>> points;
        points.reserve(1000);

        for (int y = 0; y < height; ++y) {
            const uchar* row = edges.ptr<uchar>(y);
            for (int x = 0; x < width; ++x) {
                if (row[x] == 0) continue;
                if (std::abs(x * cosT + y * sinT - line.rho) <= 2.0)
                    points.emplace_back(x * (-sinT) + y * cosT, cv::Point(x, y));
            }
        }

        if (points.empty()) continue;
        std::sort(points.begin(), points.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        double chainStart = points[0].first;
        double chainEnd   = points[0].first;
        cv::Point p1Start = points[0].second;

        auto emitSegment = [&](double sp, double ep, const cv::Point& s, const cv::Point& e) {
            if (ep - sp + 1e-6 < minLineLength) return;
            HoughLine seg = line;
            seg.p1 = s; seg.p2 = e;
            segments.push_back(seg);
        };

        for (size_t i = 1; i < points.size(); ++i) {
            double cur = points[i].first;
            if (cur - chainEnd <= 10.0) {
                chainEnd = cur;
            } else {
                emitSegment(chainStart, chainEnd, p1Start, points[i - 1].second);
                chainStart = chainEnd = cur;
                p1Start = points[i].second;
            }
        }
        emitSegment(chainStart, chainEnd, p1Start, points.back().second);
    }
    return segments;
}

} // anonymous namespace

// ============================================================================
// HOUGH CIRCLES
// ============================================================================

namespace {

static const float GK[3][3] = {
    {0.07f, 0.12f, 0.07f},
    {0.12f, 0.24f, 0.12f},
    {0.07f, 0.12f, 0.07f}
};

// Single on-axis vote — no angular spread to keep accumulator peaks sharp
static const double ANGLE_OFFSETS[] = { 0.0 };
static const int    NUM_OFFSETS     = 1;

// ────────────────────────────────────────────────────────────────────────────
// Auto-compute Canny thresholds from image median pixel value.
// Fixed thresholds like 50/150 fail on dark, bright, or low-contrast images.
// This adapts to the actual image content so edges are always consistent.
// ────────────────────────────────────────────────────────────────────────────
static void autoCannyThresholds(const cv::Mat& gray,
                                 double& lower, double& upper,
                                 double sigma = 0.33)
{
    std::vector<uchar> pixels(gray.begin<uchar>(), gray.end<uchar>());
    std::nth_element(pixels.begin(),
                     pixels.begin() + pixels.size() / 2,
                     pixels.end());
    double median = static_cast<double>(pixels[pixels.size() / 2]);
    lower = std::max(0.0,   (1.0 - sigma) * median);
    upper = std::min(255.0, (1.0 + sigma) * median);
}

// ────────────────────────────────────────────────────────────────────────────
// Weighted Hough Circle Voting
// ────────────────────────────────────────────────────────────────────────────
std::vector<Circle> houghCirclesAdvanced(
    const cv::Mat& edges,
    const cv::Mat& mag,
    const cv::Mat& ang,
    int minRadius,
    int maxRadius,
    double param2,
    int minAbsVotes,
    int borderMargin)
{
    const int width  = edges.cols;
    const int height = edges.rows;

    if (minRadius < 1) minRadius = 1;
    if (maxRadius <= minRadius) maxRadius = minRadius + 100;
    maxRadius = std::min(maxRadius, cvRound(std::hypot(width, height)));

    cv::Mat acc(height, width, CV_32FC1, cv::Scalar(0.0f));

    for (int y = borderMargin; y < height - borderMargin; ++y) {
        const uchar* eRow = edges.ptr<uchar>(y);
        const float* mRow = mag.ptr<float>(y);
        const float* aRow = ang.ptr<float>(y);

        for (int x = borderMargin; x < width - borderMargin; ++x) {
            if (eRow[x] == 0) continue;

            float weight    = mRow[x];
            float edgeAngle = aRow[x];

            for (int r = minRadius; r <= maxRadius; ++r) {
                for (int oi = 0; oi < NUM_OFFSETS; ++oi) {
                    double voteAngle = edgeAngle + ANGLE_OFFSETS[oi];
                    double cosA = std::cos(voteAngle);
                    double sinA = std::sin(voteAngle);

                    for (int sign : {-1, 1}) {
                        int cx = cvRound(x + sign * r * cosA);
                        int cy = cvRound(y + sign * r * sinA);

                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                int nx = cx + dx;
                                int ny = cy + dy;
                                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                                    acc.at<float>(ny, nx) += weight * GK[dy + 1][dx + 1];
                            }
                        }
                    }
                }
            }
        }
    }

    // NMS: window = minRadius for proportional suppression
    const int NMS_RADIUS    = minRadius;
    float minCenterVotes    = static_cast<float>(minAbsVotes);

    struct Candidate { int y, x; float votes; };
    std::vector<Candidate> candidates;

    for (int y = borderMargin; y < height - borderMargin; ++y) {
        for (int x = borderMargin; x < width - borderMargin; ++x) {
            float v = acc.at<float>(y, x);
            if (v < minCenterVotes) continue;

            bool isLocalMax = true;
            for (int dy = -NMS_RADIUS; dy <= NMS_RADIUS && isLocalMax; ++dy) {
                for (int dx = -NMS_RADIUS; dx <= NMS_RADIUS && isLocalMax; ++dx) {
                    int ny = y + dy, nx = x + dx;
                    if (ny < 0 || ny >= height || nx < 0 || nx >= width) continue;
                    if (acc.at<float>(ny, nx) > v) isLocalMax = false;
                }
            }
            if (isLocalMax) candidates.push_back({y, x, v});
        }
    }

    const size_t MAX_CANDIDATES = 500;
    if (candidates.size() > MAX_CANDIDATES) {
        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b) { return a.votes > b.votes; });
        candidates.resize(MAX_CANDIDATES);
    }

    std::vector<Circle> circles;
    std::vector<int> hist(maxRadius + 1, 0);

    for (const auto& cand : candidates) {
        std::fill(hist.begin(), hist.end(), 0);

        // Local window only — avoids histogram pollution from distant edges
        const int searchR = maxRadius;
        const int x0 = std::max(0,         cand.x - searchR);
        const int x1 = std::min(width  - 1, cand.x + searchR);
        const int y0 = std::max(0,         cand.y - searchR);
        const int y1 = std::min(height - 1, cand.y + searchR);

        for (int ey = y0; ey <= y1; ++ey) {
            const uchar* row = edges.ptr<uchar>(ey);
            for (int ex = x0; ex <= x1; ++ex) {
                if (!row[ex]) continue;
                int d = cvRound(std::hypot(ex - cand.x, ey - cand.y));
                if (d >= minRadius && d <= maxRadius)
                    hist[d]++;
            }
        }

        int bestR = minRadius, bestV = 0;
        for (int r = minRadius; r <= maxRadius; ++r)
            if (hist[r] > bestV) { bestV = hist[r]; bestR = r; }

        double expectedVotes = 2.0 * CV_PI * bestR;
        double required      = std::max(static_cast<double>(minAbsVotes),
                                        param2 * expectedVotes);
        if (bestV < required) continue;

        // Geometric verification: 8-sector angular coverage check.
        // Requires edge pixels in at least 4 of 8 sectors around the circle.
        // Rejects arcs, engravings, and reflections that only cover one side.
        const int    NUM_SECTORS = 8;
        const double tolerance   = bestR * 0.2;
        std::vector<bool> sectorHit(NUM_SECTORS, false);

        for (int ey = y0; ey <= y1; ++ey) {
            const uchar* row = edges.ptr<uchar>(ey);
            for (int ex = x0; ex <= x1; ++ex) {
                if (!row[ex]) continue;
                double ddx  = ex - cand.x;
                double ddy  = ey - cand.y;
                double dist = std::hypot(ddx, ddy);
                if (std::abs(dist - bestR) > tolerance) continue;
                double angle  = std::atan2(ddy, ddx);
                if (angle < 0) angle += 2.0 * CV_PI;
                int sector = std::min(
                    static_cast<int>(angle / (2.0 * CV_PI / NUM_SECTORS)),
                    NUM_SECTORS - 1);
                sectorHit[sector] = true;
            }
        }

        int hitSectors = static_cast<int>(
            std::count(sectorHit.begin(), sectorHit.end(), true));
        if (hitSectors < 4) continue;

        circles.push_back({
            static_cast<float>(cand.x),
            static_cast<float>(cand.y),
            static_cast<float>(bestR),
            static_cast<int>(cand.votes)
        });
    }

    std::sort(circles.begin(), circles.end(),
              [](const Circle& a, const Circle& b) { return a.votes > b.votes; });
    return circles;
}

// ────────────────────────────────────────────────────────────────────────────
// Remove duplicate circles
// ────────────────────────────────────────────────────────────────────────────
std::vector<Circle> removeDuplicateCircles(
    const std::vector<Circle>& circles,
    double radiusTolerance,
    double centerDistTolerance,
    double minDistPx)
{
    if (circles.empty()) return circles;

    auto sorted = circles;
    std::sort(sorted.begin(), sorted.end(),
              [](const Circle& a, const Circle& b) { return a.votes > b.votes; });

    std::vector<bool> marked(sorted.size(), false);
    std::vector<Circle> unique;
    unique.reserve(sorted.size());

    for (size_t i = 0; i < sorted.size(); ++i) {
        if (marked[i]) continue;
        const Circle& ref = sorted[i];
        unique.push_back(ref);
        marked[i] = true;

        for (size_t j = i + 1; j < sorted.size(); ++j) {
            if (marked[j]) continue;
            const Circle& cand = sorted[j];

            double centerDist = std::hypot(ref.cx - cand.cx, ref.cy - cand.cy);
            double avgR       = (ref.radius + cand.radius) / 2.0;

            bool tooCloseAbsolute = centerDist < minDistPx;
            bool sameCoin         = centerDist < ref.radius * 0.5;
            bool centersTooClose  = centerDist < avgR * centerDistTolerance;
            bool radiiTooSimilar  = std::abs(ref.radius - cand.radius) <= avgR * radiusTolerance;

            if (tooCloseAbsolute || sameCoin || (centersTooClose && radiiTooSimilar))
                marked[j] = true;
        }
    }
    return unique;
}

} // anonymous namespace

// ============================================================================
// PUBLIC PROCESSOR INTERFACE
// ============================================================================

HoughResult HoughProcessor::apply(const cv::Mat& input, const HoughParams& params) {
    switch (params.shapeType) {
        case HoughShapeType::LINE:    return applyLine(input, params);
        case HoughShapeType::CIRCLE:  return applyCircle(input, params);
        case HoughShapeType::ELLIPSE: return applyEllipse(input, params);
        default: throw std::invalid_argument("Unknown HoughShapeType");
    }
}

// ────────────────────────────────────────────────────────────────────────────
// HOUGH LINES
// ────────────────────────────────────────────────────────────────────────────
HoughResult HoughProcessor::applyLine(const cv::Mat& input, const HoughParams& params) {
    HoughResult result;
    if (input.empty()) { result.transformImage = cv::Mat(); return result; }

    cv::Mat gray;
    if (input.channels() == 3) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else gray = input.clone();

    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

    cv::Mat edges;
    cv::Canny(gray, edges, params.cannyThreshold1, params.cannyThreshold2);

    auto lines = houghLinesAdvanced(edges, params.rho, params.theta,
                                    params.threshold, params.nmsWindowSize,
                                    params.borderMargin);
    lines = removeDuplicateLines(lines, params.lineAngleTolerance, params.lineDistTolerance);
    lines = linesToSegments(edges, lines, params.minLineLength);

    cv::Mat output;
    if (input.channels() == 1) cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    else output = input.clone();

    result.lines          = lines;
    result.transformImage = overlayLines(output, lines);
    for (const auto& l : lines)
        result.lineSegments.emplace_back(l.p1.x, l.p1.y, l.p2.x, l.p2.y);
    return result;
}

// ────────────────────────────────────────────────────────────────────────────
// HOUGH CIRCLES
// ────────────────────────────────────────────────────────────────────────────
HoughResult HoughProcessor::applyCircle(const cv::Mat& input, const HoughParams& params) {
    HoughResult result;
    if (input.empty()) { result.transformImage = cv::Mat(); return result; }

    cv::Mat gray;
    if (input.channels() == 3)        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else if (input.type() == CV_8UC1)  gray = input.clone();
    else                               input.convertTo(gray, CV_8UC1);

    // ── FIX 1: Two-pass blur to suppress coin texture and engravings ─────────
    // A single medianBlur(5) is not strong enough for coin surfaces which have
    // fine texture, lettering, and engraving detail. The median pass kills
    // salt-and-pepper noise while preserving edges; the Gaussian pass then
    // smooths out remaining fine texture without destroying the outer boundary.
    cv::medianBlur(gray, gray, 7);                              // stronger than 5
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);         // second smoothing pass

    // Sobel gradients for magnitude + angle (used in voting)
    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_32F, 0, 1, 3);

    cv::Mat mag(gray.size(), CV_32FC1);
    cv::Mat ang(gray.size(), CV_32FC1);
    float maxMag = 0.0f;

    for (int y = 0; y < gray.rows; ++y) {
        const float* gxRow = gradX.ptr<float>(y);
        const float* gyRow = gradY.ptr<float>(y);
        float*       mRow  = mag.ptr<float>(y);
        float*       aRow  = ang.ptr<float>(y);
        for (int x = 0; x < gray.cols; ++x) {
            float gx = gxRow[x], gy = gyRow[x];
            mRow[x] = std::sqrt(gx * gx + gy * gy);
            aRow[x] = std::atan2(gy, gx);
            maxMag  = std::max(maxMag, mRow[x]);
        }
    }
    if (maxMag > 0.0f) mag /= maxMag;

    // ── FIX 2: Auto-threshold Canny from image median ────────────────────────
    // Fixed thresholds (50/150) break on dark, bright, or low-contrast images.
    // The median-based sigma method adapts to actual image content so the edge
    // map is always consistent regardless of lighting or camera exposure.
    // params.cannyThreshold1/2 are used as fallback only if median gives 0.
    cv::Mat edges;
    {
        double lower, upper;
        autoCannyThresholds(gray, lower, upper, 0.33);
        // Fall back to manual thresholds if auto produces degenerate values
        if (upper < 1.0) {
            lower = params.cannyThreshold1;
            upper = params.cannyThreshold2;
        }
        cv::Canny(gray, edges, lower, upper);
    }

    // ── FIX 3: Morphological closing to connect broken outer coin edges ───────
    // Canny can leave small gaps in the outer coin boundary (especially where
    // the coin edge meets a bright or dark background). Closing with a small
    // elliptical kernel bridges those gaps so the outer ring votes strongly
    // without adding new false edges.
    {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
    }

    // Effective minimum distance between circle centers
    int effectiveMinDist = (params.minDist > 0.0)
        ? static_cast<int>(params.minDist)
        : std::max(params.minRadius * 2, gray.rows / 10);

    auto circles = houghCirclesAdvanced(
        edges, mag, ang,
        params.minRadius, params.maxRadius,
        params.param2,
        params.minAbsVotes,
        params.borderMargin);

    circles = removeDuplicateCircles(circles,
                                     params.circleRadiusTolerance,
                                     params.circleCenterDistTolerance,
                                     static_cast<double>(effectiveMinDist));

    cv::Mat output;
    if (input.channels() == 1) cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    else output = input.clone();

    result.circles        = circles;
    result.transformImage = overlayCircles(output, circles);
    for (const auto& c : circles)
        result.circleVec3f.emplace_back(c.cx, c.cy, c.radius);
    return result;
}

// ────────────────────────────────────────────────────────────────────────────
// HOUGH ELLIPSE
// ────────────────────────────────────────────────────────────────────────────
HoughResult HoughProcessor::applyEllipse(const cv::Mat& input, const HoughParams& params) {
    HoughResult result;
    if (input.empty()) { result.transformImage = cv::Mat(); return result; }

    cv::Mat gray;
    if (input.channels() == 3) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else gray = input.clone();

    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

    cv::Mat edges;
    cv::Canny(gray, edges, params.cannyThreshold1, params.cannyThreshold2);
//dont use open cv start from here 
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);

    cv::Mat output;
    if (input.channels() == 1) cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    else output = input.clone();

    std::vector<cv::RotatedRect> ellipses;
    for (const auto& contour : contours) {
        if (contour.size() < 5) continue;
        double area = cv::contourArea(contour);
        if (area < params.minEllipseArea) continue;
        cv::RotatedRect ell = cv::fitEllipse(contour);
        float major = std::max(ell.size.width, ell.size.height);
        float minor = std::min(ell.size.width, ell.size.height);
        if (minor < 1.0f || major / minor > params.maxEllipseAspectRatio) continue;
        ellipses.push_back(ell);
        cv::ellipse(output, ell, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
        cv::circle(output,
                   cv::Point(cvRound(ell.center.x), cvRound(ell.center.y)),
                   3, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
    }

    result.ellipses       = ellipses;
    result.transformImage = output;
    return result;
}
/////////////////to here
// ============================================================================
// OVERLAY HELPERS
// ============================================================================

cv::Mat HoughProcessor::overlayLines(
    cv::Mat output,
    const std::vector<HoughLine>& lines,
    const cv::Scalar& color,
    int thickness)
{
    for (const auto& line : lines)
        cv::line(output, line.p1, line.p2, color, thickness, cv::LINE_AA);
    return output;
}

cv::Mat HoughProcessor::overlayCircles(
    cv::Mat output,
    const std::vector<Circle>& circles,
    const cv::Scalar& centerColor,
    const cv::Scalar& radiusColor,
    int thickness)
{
    for (const auto& c : circles) {
        cv::Point center(cvRound(c.cx), cvRound(c.cy));
        cv::circle(output, center, 3,                centerColor, -1,        cv::LINE_AA);
        cv::circle(output, center, cvRound(c.radius), radiusColor, thickness, cv::LINE_AA);
    }
    return output;
}

} // namespace processing