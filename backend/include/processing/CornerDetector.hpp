#include <opencv2/opencv.hpp>
#pragma once

namespace processing {

enum class CornerDetectionMode {
    HARRIS,
    SHI_TOMASI
};

struct CornerParams {
    CornerDetectionMode mode = CornerDetectionMode::HARRIS;
    double sigma = 1.0;
    int windowSize = 3;
    double threshold = 100.0;
    double k = 0.04;
};

struct CornerResult {
    cv::Mat image; // Image with corners drawn or corner map
    long long computationTimeMs = 0;
    int featureCount = 0;
};
struct matrix_elements {
    cv::Mat Sx;
    cv::Mat Sy;
    cv::Mat Sxy;
};


class CornerDetector {
public:
    static CornerResult detect(const cv::Mat& input, const CornerParams& params);

private:
    static matrix_elements compute_matrix_elements(const cv::Mat& gray, const CornerParams& params);
    static CornerResult detectHarris(const cv::Mat& gray, const CornerParams& params);
    static CornerResult detectShiTomasi(const cv::Mat& gray, const CornerParams& params);
    static void finalizeCornerResponse(const cv::Mat& gray,
                            const cv::Mat& response,
                            const processing::CornerParams& params,
                            CornerResult& result);
};

} // namespace processing
