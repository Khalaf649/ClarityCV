#include "processing/SIFTProcessor.hpp"

#include <chrono>

namespace processing {

SIFTResult SIFTProcessor::apply(const cv::Mat& input, const SIFTParams& params) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    SIFTResult result;

    // Placeholder behaviour: return the input image (converted to BGR if needed)
    if (input.empty()) {
        result.image = cv::Mat();
        result.featureCount = 0;
    } else {
        if (input.channels() == 1) cv::cvtColor(input, result.image, cv::COLOR_GRAY2BGR);
        else result.image = input.clone();
        result.featureCount = 0; // TODO: detect actual keypoints
    }

    auto end = Clock::now();
    result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
}

} // namespace processing
