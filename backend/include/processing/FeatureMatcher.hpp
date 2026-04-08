#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

enum class MatchingMethod {
    SSD,
    NCC
};

struct FeatureMatchingParams {
    MatchingMethod method = MatchingMethod::SSD;
};

struct FeatureMatchingResult {
    cv::Mat image;
    long long computationTimeMs = 0;
    int matchesCount = 0;
};

class FeatureMatcher {
public:
    // Placeholder — actual matching logic to be implemented later
    static FeatureMatchingResult match(const cv::Mat& img1, const cv::Mat& img2, const FeatureMatchingParams& params);

private:
    static FeatureMatchingResult matchSSD(const cv::Mat& img1, const cv::Mat& img2);
    static FeatureMatchingResult matchNCC(const cv::Mat& img1, const cv::Mat& img2);
};

} // namespace processing
