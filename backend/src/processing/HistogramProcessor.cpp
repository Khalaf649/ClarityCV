// ---------------------------------------------------------------------------
// Tasks 4, 5, 6, 8 - Histogram, Equalize, Normalize, RGB channel histograms
// ---------------------------------------------------------------------------
#include "processing/HistogramProcessor.hpp"
#include <stdexcept>

namespace processing {

HistogramResult HistogramProcessor::compute(const cv::Mat& input) {
    throw std::runtime_error("HistogramProcessor::compute() - not yet implemented.");
}

cv::Mat HistogramProcessor::equalize(const cv::Mat& input) {
    throw std::runtime_error("HistogramProcessor::equalize() - not yet implemented.");
}

cv::Mat HistogramProcessor::normalize(const cv::Mat& input) {
    throw std::runtime_error("HistogramProcessor::normalize() - not yet implemented.");
}

ChannelHistogram HistogramProcessor::computeChannel(const cv::Mat& channel,
                                                     const std::string& label) {
    throw std::runtime_error("HistogramProcessor::computeChannel() - not yet implemented.");
}

cv::Mat HistogramProcessor::renderHistogram(const std::vector<ChannelHistogram>& channels,
                                             int width, int height) {
    throw std::runtime_error("HistogramProcessor::renderHistogram() - not yet implemented.");
}

} // namespace processing
