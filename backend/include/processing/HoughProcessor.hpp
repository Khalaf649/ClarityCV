#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

namespace processing {

// ── Shape selector ──────────────────────────────────────────────────────────
enum class HoughShapeType { LINE, CIRCLE, ELLIPSE };

// ── Advanced Hough data structures ──────────────────────────────────────────
struct HoughLine {
    double rho;
    double theta;
    int votes;
    cv::Point p1, p2;

    HoughLine(double r = 0.0, double t = 0.0, int v = 0)
        : rho(r), theta(t), votes(v), p1(0, 0), p2(0, 0) {}
};

struct Circle {
    float cx, cy;
    float radius;
    int votes;

    Circle(float x = 0.0f, float y = 0.0f, float r = 0.0f, int v = 0)
        : cx(x), cy(y), radius(r), votes(v) {}
};

// ── Parameter bundle ────────────────────────────────────────────────────────
struct HoughParams {
    HoughShapeType shapeType = HoughShapeType::LINE;

    // ── Shared / edge-detection ─────────────────────────────────────────────
    // Note: for circles these are used as fallback only when auto-Canny
    // produces degenerate thresholds (e.g. near-black image).
    double cannyThreshold1 = 50.0;
    double cannyThreshold2 = 150.0;

    // ── Line-specific ───────────────────────────────────────────────────────
    bool   useProabilisticHough = true;
    double rho                  = 1.0;
    double theta                = CV_PI / 180.0;
    int    threshold            = 80;
    double minLineLength        = 30.0;
    double maxLineGap           = 10.0;
    int    nmsWindowSize        = 5;
    double lineAngleTolerance   = 1.0;
    double lineDistTolerance    = 2.0;

    // ── Circle-specific ─────────────────────────────────────────────────────
    double dp      = 1.5;
    double minDist = 0.0;   // 0 → computed as max(minRadius*2, rows/10)
    double param1  = 100.0;

    // Fraction of circumference [0.0–1.0] required to accept a circle.
    // 0.35 = 35% edge coverage needed — rejects arcs and small engravings.
    double param2  = 0.35;

    // Controls accumulator center vote threshold directly.
    // Higher = only strongly-voted centers survive (fewer false positives).
    // Lower  = weaker centers accepted (more detections, more noise).
    int minAbsVotes = 20;

    int minRadius = 20;  // 20px minimum — kills sub-coin details
    int maxRadius = 250;

    double angleSampleDeg = 1.0;

    double circleRadiusTolerance     = 0.20;
    double circleCenterDistTolerance = 0.15;

    // ── Ellipse-specific ────────────────────────────────────────────────────
    double minEllipseArea        = 100.0;
    float  maxEllipseAspectRatio = 10.0f;

    // Pyramid and Hough controls for ellipse detection
    int pyramidMinSize = 16;   // smallest pyramid level size (px)
    int houghMinVote   = 40;   // minimum votes at coarse level
    int houghMinDist   = 10;   // minimum center distance (px) during voting
    int maxEllipses    = 5;    // maximum ellipses to detect per run
    // ── Border handling ─────────────────────────────────────────────────────
    int borderMargin = 5;
};

// ── Result bundle ────────────────────────────────────────────────────────────
struct HoughResult {

    cv::Mat                       transformImage;
    std::vector<HoughLine>        lines;
    std::vector<Circle>           circles;
    std::vector<cv::RotatedRect>  ellipses;
    std::vector<cv::Vec4i>        lineSegments;
    std::vector<cv::Vec3f>        circleVec3f;

};

// ── Processor ────────────────────────────────────────────────────────────────
class HoughProcessor {
public:
    HoughResult apply(const cv::Mat& input, const HoughParams& params);

    static cv::Mat overlayLines(cv::Mat output, const std::vector<HoughLine>& lines,
                                const cv::Scalar& color = cv::Scalar(0, 255, 0),
                                int thickness = 2);

    static cv::Mat overlayCircles(cv::Mat output, const std::vector<Circle>& circles,
                                  const cv::Scalar& centerColor = cv::Scalar(0, 255, 0),
                                  const cv::Scalar& radiusColor = cv::Scalar(0, 0, 255),
                                  int thickness = 2);

private:
    HoughResult applyLine   (const cv::Mat& input, const HoughParams& params);
    HoughResult applyCircle (const cv::Mat& input, const HoughParams& params);
    HoughResult applyEllipse(const cv::Mat& input, const HoughParams& params);

};

} // namespace processing