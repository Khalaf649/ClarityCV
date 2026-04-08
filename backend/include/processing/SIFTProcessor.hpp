#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

struct SIFTParams {
    double contrastThreshold = 0.04;
    int    nfeatures         = 500;
};

struct SIFTResult {
    cv::Mat image;
    long long computationTimeMs = 0;
    int featureCount = 0;
};

class SIFTProcessor {
public:
    // Placeholder — implement SIFT detection and visualization later
    static SIFTResult apply(const cv::Mat& input, const SIFTParams& params);
};

} // namespace processing
