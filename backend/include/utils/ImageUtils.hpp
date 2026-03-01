#pragma once
#include <opencv2/opencv.hpp>
#include <string>

namespace utils {

cv::Mat decodeImageFromBase64(const std::string& base64Str);
std::string encodeImageToBase64(const cv::Mat& image, const std::string& ext = ".png");

cv::Mat toGrayscale(const cv::Mat& image);
cv::Mat toRGB(const cv::Mat& image);

} // namespace utils
