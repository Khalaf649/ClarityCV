// ---------------------------------------------------------------------------
// Task 9 - Frequency domain filters (high-pass and low-pass)
// ---------------------------------------------------------------------------
#include "processing/FrequencyProcessor.hpp"
#include <stdexcept>

namespace processing {

cv::Mat FrequencyProcessor::toGray(const cv::Mat& input) {
    if (input.channels() == 1) return input.clone();
    if (input.channels() == 3) {
        cv::Mat gray(input.size(), CV_8U);
        for (int y = 0; y < input.rows; ++y) {
            for (int x = 0; x < input.cols; ++x) {
                cv::Vec3b v = input.at<cv::Vec3b>(y, x);
                gray.at<uchar>(y, x) = static_cast<uchar>(
                    0.299 * v[2] + 0.587 * v[1] + 0.114 * v[0]
                );
            }
        }
        return gray;
    }
    throw std::invalid_argument("toGray: only 1- or 3-channel images are supported");
}

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
