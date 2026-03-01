#include "processing/ThresholdProcessor.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>

namespace processing {

ThresholdResult ThresholdProcessor::apply(const cv::Mat& input, const ThresholdParams& params) {
    throw std::runtime_error("ThresholdProcessor::apply() - not yet implemented.");
}

} // namespace processing
