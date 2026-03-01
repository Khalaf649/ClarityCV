#pragma once
#include <opencv2/opencv.hpp>
#include <string>

namespace processing {

enum class NoiseType {
    UNIFORM,
    GAUSSIAN,
    SALT_AND_PEPPER
};

struct NoiseParams {
    NoiseType type      = NoiseType::GAUSSIAN;
    double    mean      = 0.0;      // Gaussian mean
    double    stddev    = 25.0;     // Gaussian stddev
    double    low       = -50.0;    // Uniform low bound
    double    high      = 50.0;     // Uniform high bound
    double    saltProb  = 0.02;     // Salt & pepper salt probability
    double    pepperProb = 0.02;    // Salt & pepper pepper probability
};

class NoiseProcessor {
public:
    static cv::Mat addNoise(const cv::Mat& input, const NoiseParams& params);

private:
    static cv::Mat addGaussianNoise(const cv::Mat& input, double mean, double stddev);
    static cv::Mat addUniformNoise(const cv::Mat& input, double low, double high);
    static cv::Mat addSaltAndPepperNoise(const cv::Mat& input, double saltProb, double pepperProb);
};

} // namespace processing
