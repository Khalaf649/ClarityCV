#include "processing/HybridProcessor.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>

namespace processing {

cv::Mat HybridProcessor::applyDFTFilter(const cv::Mat& gray, double cutoff, bool lowPass) {
    throw std::runtime_error("HybridProcessor::applyDFTFilter() - not yet implemented.");
}

cv::Mat HybridProcessor::extractLowFrequency(const cv::Mat& input, double cutoff) {
    throw std::runtime_error("HybridProcessor::extractLowFrequency() - not yet implemented.");
}

cv::Mat HybridProcessor::extractHighFrequency(const cv::Mat& input, double cutoff) {
    throw std::runtime_error("HybridProcessor::extractHighFrequency() - not yet implemented.");
}

HybridResult HybridProcessor::create(const cv::Mat& image1, const cv::Mat& image2,
                                      const HybridParams& params) {
    throw std::runtime_error("HybridProcessor::create() - not yet implemented.");
}

} // namespace processing
