#include "processing/NoiseProcessor.hpp"
#include <stdexcept>
#include <opencv2/opencv.hpp>

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
    cv::Mat floatInput;
    input.convertTo(floatInput, CV_64F);
    cv::Mat noise(floatInput.size(), floatInput.type());
    cv::randn(noise, mean, stddev);

    

    cv::Mat result;
    cv::add(floatInput, noise, result);
    cv::threshold(result, result, 255.0, 255.0, cv::THRESH_TRUNC);
    cv::threshold(result, result, 0.0, 0.0, cv::THRESH_TOZERO);

    result.convertTo(result, input.type());
    return result;
}

cv::Mat NoiseProcessor::addUniformNoise(const cv::Mat& input, double low, double high) {
    cv::Mat floatInput;
    input.convertTo(floatInput, CV_64F);

    cv::Mat noise(floatInput.size(), floatInput.type());
    cv::randu(noise, low, high);

    

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

    cv::Mat saltMask = randMat < saltProb; // CV_8U mask where salt should be applied
    cv::Mat pepperMask = (randMat >= saltProb) & (randMat < saltProb + pepperProb);

    if (result.channels() == 1) {
        result.setTo(255, saltMask);
        result.setTo(0, pepperMask);
    } else {
        result.setTo(cv::Scalar(255, 255, 255), saltMask);
        result.setTo(cv::Scalar(0, 0, 0), pepperMask);
    }

    return result;
}

} 
    