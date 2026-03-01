#pragma once
#include <opencv2/opencv.hpp>

namespace processing {

enum class FilterType {
    AVERAGE,
    GAUSSIAN,
    MEDIAN
};

struct FilterParams {
    FilterType type       = FilterType::GAUSSIAN;
    int        kernelSize = 3;  // Must be odd: 3, 5, 7, ...
    double     sigmaX     = 0.0; // Gaussian sigma (0 = auto-compute from kernel)
};

class FilterProcessor {
public:
    static cv::Mat applyFilter(const cv::Mat& input, const FilterParams& params);

private:
    static cv::Mat applyAverage(const cv::Mat& input, int kernelSize);
    static cv::Mat applyGaussian(const cv::Mat& input, int kernelSize, double sigma);
    static cv::Mat applyMedian(const cv::Mat& input, int kernelSize);
};

} // namespace processing
