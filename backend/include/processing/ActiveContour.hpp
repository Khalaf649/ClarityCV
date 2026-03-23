#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

namespace processing {

enum class ContourType {
    GREEDY
};

struct ContourParams {
    ContourType type       = ContourType::GREEDY;
    double      alpha      = 1.0;  // Continuity
    double      beta       = 1.0;  // Curvature
    double      gamma      = 1.0;  // Image energy
    int         iterations = 100;
    int         controlPoints = 100;
};

struct ContourResult {
    cv::Mat contourImage;             // Optional visualization
    std::vector<cv::Point> points;    // Resulting contour points
    double perimeter = 0.0;           // Contour perimeter
    double area = 0.0;                // Contour area
};

class ActiveContour {
public:
    static ContourResult run_active_contour(const cv::Mat& input, const std::vector<cv::Point>& initialPoints, const ContourParams& params);

private:
    static ContourResult processGreedy(const cv::Mat& gray, const std::vector<cv::Point>& initialPoints, const ContourParams& params);
    static cv::Mat toGray(const cv::Mat& input);
};

} // namespace processing