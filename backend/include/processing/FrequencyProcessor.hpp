#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

enum class FrequencyFilterType {
    LOW_PASS,
    HIGH_PASS
};

struct FrequencyParams {
    FrequencyFilterType filterType = FrequencyFilterType::LOW_PASS;
    double              cutoff     = 30.0;
};

struct FrequencyResult {
    cv::Mat filtered;       // Spatial domain result
    cv::Mat magnitudeLog;   // Log-magnitude spectrum visualization
};

class FrequencyProcessor {
public:
    static FrequencyResult apply(const cv::Mat& input, const FrequencyParams& params);

private:
    static cv::Mat buildMask(int rows, int cols, double cutoff, FrequencyFilterType type);
    static cv::Mat computeMagnitudeSpectrum(const cv::Mat& complexImg);
};

} // namespace processing
