// ---------------------------------------------------------------------------
// Task 7 - Local and global thresholding
// ---------------------------------------------------------------------------
#include "processing/ThresholdProcessor.hpp"
#include <stdexcept>

namespace processing {

cv::Mat ThresholdProcessor::toGray(const cv::Mat& input) {
    if (input.channels() == 1) return input.clone();
    cv::Mat gray;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

ThresholdResult ThresholdProcessor::apply(const cv::Mat& input, const ThresholdParams& params) {
    throw std::runtime_error("ThresholdProcessor::apply() - not yet implemented.");
}

} // namespace processing
