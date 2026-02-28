#pragma once
#include <opencv2/opencv.hpp>
#include <string>

namespace processing {

enum class EdgeType {
    SOBEL,
    ROBERTS,
    PREWITT,
    CANNY
};

enum class EdgeDirection {
    X,
    Y,
    COMBINED  // Magnitude of X and Y
};

struct EdgeParams {
    EdgeType      type        = EdgeType::SOBEL;
    EdgeDirection direction   = EdgeDirection::COMBINED;
    // Canny-specific
    double        cannyLow    = 50.0;
    double        cannyHigh   = 150.0;
    // Sobel kernel size
    int           sobelKsize  = 3;
};

struct EdgeResult {
    cv::Mat edgeX;       // Horizontal edges (not used for Canny)
    cv::Mat edgeY;       // Vertical edges (not used for Canny)
    cv::Mat combined;    // Final result
};

class EdgeDetector {
public:
    static EdgeResult detect(const cv::Mat& input, const EdgeParams& params);

private:
    static EdgeResult detectSobel(const cv::Mat& gray, const EdgeParams& params);
    static EdgeResult detectRoberts(const cv::Mat& gray);
    static EdgeResult detectPrewitt(const cv::Mat& gray);
    static EdgeResult detectCanny(const cv::Mat& gray, double low, double high);
    static cv::Mat    toGray(const cv::Mat& input);
    static cv::Mat    magnitude(const cv::Mat& gx, const cv::Mat& gy);
    static cv::Mat    normalize8U(const cv::Mat& src);
};

} // namespace processing
