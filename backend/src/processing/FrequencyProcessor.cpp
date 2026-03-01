#include "processing/FrequencyProcessor.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>

namespace processing {

cv::Mat FrequencyProcessor::buildMask(int rows, int cols, double cutoff,
                                       FrequencyFilterType type) {
    throw std::runtime_error("FrequencyProcessor::buildMask() - not yet implemented.");
}

cv::Mat FrequencyProcessor::computeMagnitudeSpectrum(const cv::Mat& complexImg) {
    throw std::runtime_error("FrequencyProcessor::computeMagnitudeSpectrum() - not yet implemented.");
}

FrequencyResult FrequencyProcessor::apply(const cv::Mat& input, const FrequencyParams& params) {
    throw std::runtime_error("FrequencyProcessor::apply() - not yet implemented.");
}

} // namespace processing
