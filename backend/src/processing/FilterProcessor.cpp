// ---------------------------------------------------------------------------
// Task 2 - Filter noisy image: Average, Gaussian, Median filters
// ---------------------------------------------------------------------------
#include "processing/FilterProcessor.hpp"
#include <stdexcept>

namespace processing {

cv::Mat FilterProcessor::applyFilter(const cv::Mat& input, const FilterParams& params) {
    throw std::runtime_error("FilterProcessor::applyFilter() - not yet implemented.");
}

cv::Mat FilterProcessor::applyAverage(const cv::Mat& input, int kernelSize) {
    throw std::runtime_error("FilterProcessor::applyAverage() - not yet implemented.");
}

cv::Mat FilterProcessor::applyGaussian(const cv::Mat& input, int kernelSize, double sigma) {
    throw std::runtime_error("FilterProcessor::applyGaussian() - not yet implemented.");
}

cv::Mat FilterProcessor::applyMedian(const cv::Mat& input, int kernelSize) {
    throw std::runtime_error("FilterProcessor::applyMedian() - not yet implemented.");
}

} // namespace processing
