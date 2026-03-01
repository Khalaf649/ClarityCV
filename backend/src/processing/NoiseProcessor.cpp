#include "processing/NoiseProcessor.hpp"
#include <stdexcept>

namespace processing {

cv::Mat NoiseProcessor::addNoise(const cv::Mat& input, const NoiseParams& params) {
    switch (params.type) {
        case NoiseType::GAUSSIAN:
            return addGaussianNoise(input, params.mean, params.stddev);
        case NoiseType::UNIFORM:
            return addUniformNoise(input, params.low, params.high);
        case NoiseType::SALT_AND_PEPPER:
            return addSaltAndPepperNoise(input, params.saltProb, params.pepperProb);
        default:
            throw std::invalid_argument("Unknown noise type.");
    }
}

cv::Mat NoiseProcessor::addGaussianNoise(const cv::Mat& input, double mean, double stddev) {
    cv::Mat noise(input.size(), CV_64FC(input.channels()));
    cv::randn(noise, mean, stddev);

    cv::Mat floatInput;
    input.convertTo(floatInput, CV_64FC(input.channels()));
    
    cv::Mat result;
    cv::add(floatInput, noise, result);
    cv::threshold(result, result, 255.0, 255.0, cv::THRESH_TRUNC);
    cv::threshold(result, result, 0.0, 0.0, cv::THRESH_TOZERO);

    result.convertTo(result, input.type());
    return result;
}

cv::Mat NoiseProcessor::addUniformNoise(const cv::Mat& input, double low, double high) {
    cv::Mat noise(input.size(), CV_64FC(input.channels()));
    cv::randu(noise, low, high);

    cv::Mat floatInput;
    input.convertTo(floatInput, CV_64FC(input.channels()));

    cv::Mat result;
    cv::add(floatInput, noise, result);
    cv::threshold(result, result, 255.0, 255.0, cv::THRESH_TRUNC);
    cv::threshold(result, result, 0.0, 0.0, cv::THRESH_TOZERO);

    result.convertTo(result, input.type());
    return result;
}

cv::Mat NoiseProcessor::addSaltAndPepperNoise(const cv::Mat& input, double saltProb, double pepperProb) {
    cv::Mat result = input.clone();
    cv::Mat randMat(input.size(), CV_64F);
    cv::randu(randMat, 0.0, 1.0);

    for (int r = 0; r < result.rows; r++) {
        for (int c = 0; c < result.cols; c++) {
            double val = randMat.at<double>(r, c);
            if (val < saltProb) {
                if (result.channels() == 1) {
                    result.at<uchar>(r, c) = 255;
                } else {
                    result.at<cv::Vec3b>(r, c) = cv::Vec3b(255, 255, 255);
                }
            } else if (val < saltProb + pepperProb) {
                if (result.channels() == 1) {
                    result.at<uchar>(r, c) = 0;
                } else {
                    result.at<cv::Vec3b>(r, c) = cv::Vec3b(0, 0, 0);
                }
            }
        }
    }
    return result;
}

} // namespace processing
