// ---------------------------------------------------------------------------
// Task 3 - Edge detection: Sobel, Roberts, Prewitt, Canny
// ---------------------------------------------------------------------------
#include "processing/EdgeDetector.hpp"
#include <stdexcept>

namespace processing {

cv::Mat EdgeDetector::toGray(const cv::Mat& input) {
    if (input.channels() == 1) return input.clone();
    cv::Mat gray;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

cv::Mat EdgeDetector::normalize8U(const cv::Mat& src) {
    cv::Mat result;
    cv::normalize(src, result, 0, 255, cv::NORM_MINMAX, CV_8U);
    return result;
}

cv::Mat EdgeDetector::magnitude(const cv::Mat& gx, const cv::Mat& gy) {
    throw std::runtime_error("EdgeDetector::magnitude() - not yet implemented.");
}

EdgeResult EdgeDetector::detect(const cv::Mat& input, const EdgeParams& params) {
    throw std::runtime_error("EdgeDetector::detect() - not yet implemented.");
}

EdgeResult EdgeDetector::detectSobel(const cv::Mat& gray, const EdgeParams& params) {
    throw std::runtime_error("EdgeDetector::detectSobel() - not yet implemented.");
}

EdgeResult EdgeDetector::detectRoberts(const cv::Mat& gray) {
    throw std::runtime_error("EdgeDetector::detectRoberts() - not yet implemented.");
}

EdgeResult EdgeDetector::detectPrewitt(const cv::Mat& gray) {
    throw std::runtime_error("EdgeDetector::detectPrewitt() - not yet implemented.");
}

EdgeResult EdgeDetector::detectCanny(const cv::Mat& gray, double low, double high) {
    throw std::runtime_error("EdgeDetector::detectCanny() - not yet implemented.");
}

} // namespace processing
