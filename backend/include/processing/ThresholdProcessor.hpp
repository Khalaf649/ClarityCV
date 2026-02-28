#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

enum class ThresholdType {
    GLOBAL_BINARY,
    GLOBAL_OTSU,
    LOCAL_MEAN,
    LOCAL_GAUSSIAN
};

struct ThresholdParams {
    ThresholdType type       = ThresholdType::GLOBAL_OTSU;
    double        threshold  = 127.0;   // Used for GLOBAL_BINARY
    int           blockSize  = 11;      // Used for LOCAL methods (must be odd)
    double        C          = 2.0;     // Constant subtracted from mean (local)
};

struct ThresholdResult {
    cv::Mat binary;
    double  appliedThreshold = 0.0;  // Otsu's computed value if applicable
};

class ThresholdProcessor {
public:
    static ThresholdResult apply(const cv::Mat& input, const ThresholdParams& params);

private:
    static cv::Mat toGray(const cv::Mat& input);
};

} // namespace processing
