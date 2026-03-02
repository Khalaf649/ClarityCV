#pragma once

#include <opencv2/opencv.hpp>

namespace utils {

class Image {
public:
	Image() = default;
	explicit Image(const cv::Mat& orig) : original(orig.clone()) {}

public:
	cv::Mat original; // uploaded/original image
	cv::Mat noisy;    // noisy version (if created)
	cv::Mat filtered; // filtered version (if created)
};

} // namespace utils

