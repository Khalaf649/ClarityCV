#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

enum class EdgeType {
    SOBEL,
    ROBERTS,
    PREWITT,
    CANNY
};

struct EdgeParams {
    EdgeType type       = EdgeType::SOBEL;
    double   cannyLow   = 50.0;
    double   cannyHigh  = 150.0;
    int      sobelKsize = 3;
};

struct EdgeResult {
    cv::Mat edgeX;      // Horizontal gradient
    cv::Mat edgeY;      // Vertical gradient
    cv::Mat combined;   // Magnitude of X and Y (or Canny result)
};

class EdgeDetector {
public:
    static EdgeResult detect(const cv::Mat& input, const EdgeParams& params);

private:
    static EdgeResult detectSobel(const cv::Mat& gray, const EdgeParams& params);
    static EdgeResult detectRoberts(const cv::Mat& gray);
    static EdgeResult detectPrewitt(const cv::Mat& gray);
    static EdgeResult detectCanny(const cv::Mat& gray, double low, double high);
    static cv::Mat    magnitude(const cv::Mat& gx, const cv::Mat& gy);
    static cv::Mat    normalize8U(const cv::Mat& src);
};

} // namespace processing
