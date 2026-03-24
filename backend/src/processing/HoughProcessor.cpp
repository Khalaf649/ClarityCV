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

} // anonymous namespace (lines)

// ============================================================================
// HOUGH CIRCLES
// ============================================================================

namespace {

static const float GK[3][3] = {
    {0.07f, 0.12f, 0.07f},
    {0.12f, 0.24f, 0.12f},
    {0.07f, 0.12f, 0.07f}
};

static const double ANGLE_OFFSETS[] = { 0.0 };
static const int    NUM_OFFSETS     = 1;

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

        const int searchR = maxRadius;
        const int x0 = std::max(0,          cand.x - searchR);
        const int x1 = std::min(width  - 1,  cand.x + searchR);
        const int y0 = std::max(0,          cand.y - searchR);
        const int y1 = std::min(height - 1,  cand.y + searchR);

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

} // anonymous namespace (circles)

// ============================================================================
// HOUGH ELLIPSE — hierarchical Hough transform (no OpenCV fitting)
// ============================================================================

namespace {

// ── Internal ellipse descriptor ──────────────────────────────────────────────
struct EllipseData {
    double a      = -1;   // half major axis (pixels)
    double b      = -1;   // half minor axis (pixels)
    int    x0     = -1;   // centre x
    int    y0     = -1;   // centre y
    int    x1     = -1;   // major-axis endpoint 1 x
    int    y1     = -1;   // major-axis endpoint 1 y
    int    x2     = -1;   // major-axis endpoint 2 x
    int    y2     = -1;   // major-axis endpoint 2 y
    double orient = 0.0;  // radians (atan2 convention)

    bool valid() const { return x0 >= 0 && y0 >= 0 && a > 0 && b > 0; }
};

// ── Geometry helpers (ed-prefixed to avoid collisions) ───────────────────────
inline double edDistSq(double x1, double y1, double x2, double y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}

inline double edMajorAxisSq(int x1, int y1, int x2, int y2) {
    return edDistSq(x1, y1, x2, y2) / 4.0;
}

// Orientation of the major axis in radians.
// Using atan2 gives a stable result even when x1 == x2.
inline double edOrientation(int x1, int y1, int x2, int y2) {
    return std::atan2((double)(y2 - y1), (double)(x2 - x1));
}

// ── Pyramid types ─────────────────────────────────────────────────────────────
using PyramidLayer   = std::vector<std::vector<short>>;
using EllipsePyramid = std::vector<PyramidLayer>;

// ── Build integer-summed image pyramid ───────────────────────────────────────
// Level 0 is resized to a square whose side is the largest power of 2 that
// fits both dimensions.  Each subsequent level halves the side and sums 2x2
// blocks, so a pixel value at level k represents how many edge pixels live
// in the corresponding 2^k × 2^k block of level 0.
EllipsePyramid buildEllipsePyramid(const cv::Mat& edges, int minSize)
{
    int log2h  = (int)std::floor(std::log2(edges.rows));
    int log2w  = (int)std::floor(std::log2(edges.cols));
    int maxSz  = (int)std::pow(2, std::min(log2h, log2w));

    cv::Mat sq;
    cv::resize(edges, sq, {maxSz, maxSz});

    int steps = (int)std::floor(std::log2(maxSz))
              - (int)std::floor(std::log2(std::max(minSize, 1))) + 1;
    if (steps < 1) steps = 1;

    EllipsePyramid pyr(steps);

    // Level 0 – binarised
    pyr[0].assign(maxSz, std::vector<short>(maxSz, 0));
    for (int r = 0; r < maxSz; ++r)
        for (int c = 0; c < maxSz; ++c)
            if (sq.at<uchar>(r, c) > 0) pyr[0][r][c] = 1;

    // Levels 1..N – 2×2 sums
    int sz = maxSz;
    for (int i = 1; i < steps; ++i) {
        sz /= 2;
        pyr[i].assign(sz, std::vector<short>(sz, 0));
        for (int r = 0; r < sz; ++r)
            for (int c = 0; c < sz; ++c)
                pyr[i][r][c] = pyr[i-1][r*2  ][c*2  ]
                             + pyr[i-1][r*2+1][c*2  ]
                             + pyr[i-1][r*2  ][c*2+1]
                             + pyr[i-1][r*2+1][c*2+1];
    }
    return pyr;
}

// ── One voting pass for a fixed pair of major-axis endpoints ─────────────────
// For every other edge pixel p3, we compute the implied minor half-axis b
// (using the ellipse focal-point formula) and cast a vote into acc[b].
// The most-voted b is the best minor-axis candidate for this endpoint pair.
void ellipseVote(std::vector<std::pair<EllipseData, int>>& out,
                 const PyramidLayer& data,
                 int minDist, int minVote,
                 int x1, int y1, int x2, int y2)
{
    const int H = (int)data.size();
    const int W = (int)data[0].size();

    int x0 = cvRound((x1 + x2) / 2.0);
    int y0 = cvRound((y1 + y2) / 2.0);

    double aSq = edMajorAxisSq(x1, y1, x2, y2);
    double a   = std::sqrt(aSq);
    if (a < minDist) return;  // degenerate / too small

    double orient = edOrientation(x1, y1, x2, y2);

    int maxLen = cvRound(std::hypot((double)H, (double)W)) + 1;
    std::vector<int> acc(maxLen, 0);

    for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x)
    {
        if (data[y][x] == 0) continue;
        if ((x == x1 && y == y1) || (x == x2 && y == y2)) continue;

        double dSq = edDistSq(x, y, x0, y0);
        double d   = std::sqrt(dSq);
        if (d < minDist || d > a) continue;  // outside useful range

        // Focal-point formula: b² = (a²·d²·sin²τ) / (a² − d²·cos²τ)
        double fSq    = std::min(edDistSq(x, y, x1, y1),
                                 edDistSq(x, y, x2, y2));
        double cosTau = (aSq + dSq - fSq) / (2.0 * a * d);
        double cos2   = std::clamp(cosTau * cosTau, 0.0, 1.0);
        double sin2   = 1.0 - cos2;
        double denom  = aSq - dSq * cos2;
        if (std::abs(denom) < 1e-9) continue;
        double bSq = (aSq * dSq * sin2) / denom;
        if (bSq <= 0.0) continue;
        int b = cvRound(std::sqrt(bSq));
        if (b > minDist && b < maxLen)
            acc[b] += data[y][x];
    }

    // Pick the best minor-axis length
    int bestB = 0, bestVote = 0;
    for (int k = 0; k < maxLen; ++k)
        if (acc[k] > bestVote) { bestVote = acc[k]; bestB = k; }

    if (bestVote > minVote) {
        EllipseData e;
        e.x0 = x0;  e.y0 = y0;
        e.a  = a;   e.b  = bestB;
        e.orient = orient;
        e.x1 = x1; e.y1 = y1;
        e.x2 = x2; e.y2 = y2;
        out.push_back({e, bestVote});
    }
}

// ── Full Hough search on one pyramid level ───────────────────────────────────
// O(N⁴) in the number of edge pixels N — this is why we run on the smallest
// pyramid level first and only refine on finer levels.
std::vector<std::pair<EllipseData, int>>
houghEllipseAtLevel(const PyramidLayer& data, int minVote, int minDist)
{
    const int H = (int)data.size();
    const int W = (int)data[0].size();
    std::vector<std::pair<EllipseData, int>> res;

    for (int y1 = 0; y1 < H; ++y1)
    for (int x1 = 0; x1 < W; ++x1)
    {
        if (data[y1][x1] == 0) continue;
        for (int y2 = y1; y2 < H; ++y2)
        for (int x2 = 0;  x2 < W; ++x2)
        {
            if ((y2 == y1 && x2 <= x1) || data[y2][x2] == 0) continue;
            ellipseVote(res, data, minDist, minVote, x1, y1, x2, y2);
        }
    }
    return res;
}

// ── Refine a coarse result at the next (finer) pyramid level ─────────────────
// The coarse major-axis endpoints are scaled up by 2 and searched within a
// ±2 pixel window at the finer level, giving sub-pixel accuracy without
// re-running the full O(N⁴) search.
EllipseData refineEllipse(const PyramidLayer& data,
                           const EllipseData& prev,
                           int minVote, int minDist)
{
    const int H = (int)data.size();
    const int W = (int)data[0].size();

    auto clH = [&](int v){ return std::clamp(v, 0, H - 1); };
    auto clW = [&](int v){ return std::clamp(v, 0, W - 1); };

    int sy1s = clH(2*prev.y1 - 2), sy1e = clH(2*prev.y1 + 2);
    int sy2s = clH(2*prev.y2 - 2), sy2e = clH(2*prev.y2 + 2);
    int sx1s = clW(2*prev.x1 - 2), sx1e = clW(2*prev.x1 + 2);
    int sx2s = clW(2*prev.x2 - 2), sx2e = clW(2*prev.x2 + 2);

    std::vector<std::pair<EllipseData, int>> res;
    for (int y1 = sy1s; y1 <= sy1e; ++y1)
    for (int x1 = sx1s; x1 <= sx1e; ++x1)
    {
        if (data[y1][x1] == 0) continue;
        for (int y2 = sy2s; y2 <= sy2e; ++y2)
        for (int x2 = sx2s; x2 <= sx2e; ++x2)
        {
            if ((y2 == y1 && x2 <= x1) || data[y2][x2] == 0) continue;
            ellipseVote(res, data, minDist, minVote, x1, y1, x2, y2);
        }
    }

    if (res.empty()) return prev;  // nothing better found — keep previous

    return std::max_element(res.begin(), res.end(),
        [](const auto& a, const auto& b){ return a.second < b.second; })->first;
}

// ── Membership test: does pixel (x,y) lie on ellipse e? ──────────────────────
bool onDetectedEllipse(int x, int y, const EllipseData& e, double eps = 0.15)
{
    // Rotate into the ellipse's local frame then apply the ellipse equation.
    double cx = std::cos(e.orient) * (x - e.x0) + std::sin(e.orient) * (y - e.y0);
    double cy = std::sin(e.orient) * (x - e.x0) - std::cos(e.orient) * (y - e.y0);
    return std::abs((cx*cx)/(e.a*e.a) + (cy*cy)/(e.b*e.b) - 1.0) < eps;
}

// ── Erase ellipse pixels from edge map (enables multi-ellipse search) ─────────
void eraseDetectedEllipse(cv::Mat& edges, const EllipseData& e)
{
    for (int y = 0; y < edges.rows; ++y)
    for (int x = 0; x < edges.cols; ++x)
        if (onDetectedEllipse(x, y, e))
            edges.at<uchar>(y, x) = 0;
}

// ── Convert internal EllipseData → cv::RotatedRect ───────────────────────────
// a, b are half-axes; RotatedRect size fields are full width/height.
// orient (radians, atan2) → OpenCV angle (degrees, clockwise from x-axis).
cv::RotatedRect ellipseDataToRotatedRect(const EllipseData& e)
{
    float angleDeg = (float)(e.orient * 180.0 / CV_PI);
    return cv::RotatedRect(
        cv::Point2f((float)e.x0, (float)e.y0),
        cv::Size2f((float)(2.0 * e.a), (float)(2.0 * e.b)),
        angleDeg);
}

} // anonymous namespace (ellipse)

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

    cv::medianBlur(gray, gray, 7);
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

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

    cv::Mat edges;
    {
        double lower, upper;
        autoCannyThresholds(gray, lower, upper, 0.33);
        if (upper < 1.0) {
            lower = params.cannyThreshold1;
            upper = params.cannyThreshold2;
        }
        cv::Canny(gray, edges, lower, upper);
    }

    {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
    }

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
HoughResult HoughProcessor::applyEllipse(const cv::Mat& input, const HoughParams& params)
{
    HoughResult result;
    if (input.empty()) { result.transformImage = cv::Mat(); return result; }

    // ── Pre-processing ────────────────────────────────────────────────────────
    cv::Mat gray;
    if (input.channels() == 3) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else gray = input.clone();

    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

    cv::Mat edges;
    cv::Canny(gray, edges, params.cannyThreshold1, params.cannyThreshold2);

    // ── Setup ─────────────────────────────────────────────────────────────────
    const int minSize     = params.pyramidMinSize;
    const int minVote     = params.houghMinVote;
    const int minDist     = params.houghMinDist;
    const int maxEllipses = params.maxEllipses;

    cv::Mat workEdges = edges.clone();

    cv::Mat output;
    if (input.channels() == 1) cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    else output = input.clone();

    std::vector<cv::RotatedRect> ellipses;

    // ── Main detection loop (one ellipse per iteration) ───────────────────────
    for (int ei = 0; ei < maxEllipses; ++ei)
    {
        // Build a fresh pyramid from the current (possibly erased) edge map
        EllipsePyramid pyr = buildEllipsePyramid(workEdges, minSize);
        int n = (int)pyr.size();

        // Coarse search at the smallest (top) pyramid level
        auto candidates = houghEllipseAtLevel(pyr[n - 1], minVote, minDist);
        if (candidates.empty()) break;

        // Best candidate at the coarse level
        EllipseData found = std::max_element(
            candidates.begin(), candidates.end(),
            [](const auto& a, const auto& b){ return a.second < b.second; })->first;

        // Refine level-by-level back down to full resolution
        for (int i = n - 2; i >= 0; --i)
            found = refineEllipse(pyr[i], found, minVote, minDist);

        if (!found.valid()) break;

        // Optional aspect-ratio guard (mirrors old fitEllipse filter)
        float major = (float)(2.0 * std::max(found.a, found.b));
        float minor = (float)(2.0 * std::min(found.a, found.b));
        if (minor >= 1.0f && major / minor <= params.maxEllipseAspectRatio)
        {
            cv::RotatedRect rr = ellipseDataToRotatedRect(found);
            ellipses.push_back(rr);

            // Draw onto output
            cv::ellipse(output, rr, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
            cv::circle(output,
                cv::Point(cvRound(rr.center.x), cvRound(rr.center.y)),
                3, cv::Scalar(0, 0, 255), -1, cv::LINE_AA);
        }

        // Erase the found ellipse so the next iteration finds a different one
        eraseDetectedEllipse(workEdges, found);
    }

    result.ellipses       = ellipses;
    result.transformImage = output;
    return result;
}

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
        cv::circle(output, center, 3,                 centerColor, -1,        cv::LINE_AA);
        cv::circle(output, center, cvRound(c.radius), radiusColor, thickness, cv::LINE_AA);
    }
    return output;
}

} // namespace processing