// ---------------------------------------------------------------------------
// Task 10 - Hybrid images (low-frequency + high-frequency blend)
// ---------------------------------------------------------------------------
#include "processing/HybridProcessor.hpp"
#include "utils/FourierUtils.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>
#include <vector>

namespace processing {


std::vector<cv::Mat> HybridProcessor::extractLowFrequency(const cv::Mat& input, double cutoff) {
    cv::Mat gray = utils::toGrayscale(input);
cv::Mat complexImg = utils::FourierUtils::forwardDFT(gray); 
    int rows = complexImg.rows;
    int cols = complexImg.cols;
    cv::Mat mask(rows, cols, CV_32F, cv::Scalar(0));
    int cx = cols / 2;
    int cy = rows / 2;
    double r2 = cutoff * cutoff;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double dist2 = dx * dx + dy * dy;

            float val = (dist2 <= r2) ? 1.0f : 0.0f;   // inside circle
            mask.at<float>(y, x) = val;
        }
    }    
   
    utils::FourierUtils::fftShift(complexImg);
    cv::Mat filteredComplex = utils::FourierUtils::applyMask(complexImg, mask);
    utils::FourierUtils::fftShift(filteredComplex);
    cv::Mat invFloat = utils::FourierUtils::inverseDFTReal(filteredComplex);
    invFloat = invFloat(cv::Rect(0, 0, gray.cols, gray.rows));
    cv::Mat inv8u;
    cv::normalize(invFloat, invFloat, 0, 255, cv::NORM_MINMAX);
    invFloat.convertTo(inv8u, CV_8U);
    std::vector<cv::Mat> out;
    out.push_back(inv8u);
    out.push_back(filteredComplex);
    return out;
}

std::vector<cv::Mat> HybridProcessor::extractHighFrequency(const cv::Mat& input, double cutoff) {
     cv::Mat gray = utils::toGrayscale(input);
     cv::Mat complexImg = utils::FourierUtils::forwardDFT(gray);
    int rows = complexImg.rows;
    int cols = complexImg.cols;
    cv::Mat mask(rows, cols, CV_32F, cv::Scalar(0));
    int cx = cols / 2;
    int cy = rows / 2;
    double r2 = cutoff * cutoff;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            double dx = x - cx;
            double dy = y - cy;
            double dist2 = dx * dx + dy * dy;

            float val = (dist2 <= r2) ? 0.0f : 1.0f;   // inside circle
            mask.at<float>(y, x) = val;
        }
    }    
    
    utils::FourierUtils::fftShift(complexImg);
    cv::Mat filteredComplex = utils::FourierUtils::applyMask(complexImg, mask);
    utils::FourierUtils::fftShift(filteredComplex);
    cv::Mat invFloat = utils::FourierUtils::inverseDFTReal(filteredComplex);
    invFloat = invFloat(cv::Rect(0, 0, gray.cols, gray.rows));
    cv::Mat inv8u;
    cv::normalize(invFloat, invFloat, 0, 255, cv::NORM_MINMAX);
    invFloat.convertTo(inv8u, CV_8U);
    std::vector<cv::Mat> out;
    out.push_back(inv8u);
    out.push_back(filteredComplex);
    return out;
}

HybridResult HybridProcessor::create(const cv::Mat& image1, const cv::Mat& image2,
                                      const HybridParams& params) {
    HybridResult result;
    
    int rows = std::min(image1.rows, image2.rows);
    int cols = std::min(image1.cols, image2.cols);
    cv::Mat img1, img2;
    cv::resize(image1, img1, cv::Size(cols, rows));
    cv::resize(image2, img2, cv::Size(cols, rows));
    
    std::vector<cv::Mat> lowFreq = extractLowFrequency(img1, params.lowPassCutoff);
    std::vector<cv::Mat> highFreq = extractHighFrequency(img2, params.highPassCutoff);
    result.lowFreqImage = lowFreq[0];
    result.highFreqImage = highFreq[0];
    cv::Mat mix;
    cv::addWeighted(lowFreq[1], params.blendAlpha, highFreq[1], 1.0 - params.blendAlpha, 0.0, mix);

    cv::Mat invFloat = utils::FourierUtils::inverseDFTReal(mix);
    invFloat = invFloat(cv::Rect(0, 0, cols, rows));
    cv::Mat inv8u;
    cv::normalize(invFloat, invFloat, 0, 255, cv::NORM_MINMAX);
    invFloat.convertTo(inv8u, CV_8U);
    result.hybridImage = inv8u;
    return result;
}

} // namespace processing
