#include "processing/FeatureMatcher.hpp"

#include <chrono>

namespace processing {

FeatureMatchingResult FeatureMatcher::match(const cv::Mat& img1, const cv::Mat& img2, const FeatureMatchingParams& params) {
    if (params.method == MatchingMethod::NCC) {
        return matchNCC(img1, img2);
    } else {
        return matchSSD(img1, img2);
    }
}

FeatureMatchingResult FeatureMatcher::matchSSD(const cv::Mat& img1, const cv::Mat& img2) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    FeatureMatchingResult result;

    // Placeholder behaviour: return a side-by-side image and zero matches
    if (img1.empty() || img2.empty()) {
        result.image = cv::Mat();
        result.matchesCount = 0;
    } else {
        cv::hconcat(img1, img2, result.image);
        result.matchesCount = 0; // TODO: implement actual SSD matching
    }

    auto end = Clock::now();
    result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;

}

FeatureMatchingResult FeatureMatcher::matchNCC(const cv::Mat& img1, const cv::Mat& img2) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    FeatureMatchingResult result;

    // Placeholder behaviour: return a side-by-side image and zero matches
    if (img1.empty() || img2.empty()) {
        result.image = cv::Mat();
        result.matchesCount = 0;
    } else {
        cv::hconcat(img1, img2, result.image);
        result.matchesCount = 0; // TODO: implement actual NCC matching
    }

    auto end = Clock::now();
    result.computationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;

}



} // namespace processing
