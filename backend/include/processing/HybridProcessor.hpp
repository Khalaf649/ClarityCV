#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

struct HybridParams {
    double lowPassCutoff  = 20.0;
    double highPassCutoff = 20.0;
    double blendAlpha     = 0.5;
};

struct HybridResult {
    cv::Mat lowFreqImage;
    cv::Mat highFreqImage;
    cv::Mat hybridImage;
};

class HybridProcessor {
public:
    static HybridResult create(const cv::Mat& image1,
                               const cv::Mat& image2,
                               const HybridParams& params);

private:
    static cv::Mat applyDFTFilter(const cv::Mat& gray, double cutoff, bool lowPass);
    static cv::Mat extractLowFrequency(const cv::Mat& input, double cutoff);
    static cv::Mat extractHighFrequency(const cv::Mat& input, double cutoff);
};

} // namespace processing
