#include "processing/FrequencyProcessor.hpp"
#include "utils/FourierUtils.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>
#include <algorithm>

namespace processing {

cv::Mat FrequencyProcessor::buildMask(int rows, int cols, double cutoff, FrequencyFilterType type) {
    if (rows <= 0 || cols <= 0) throw std::invalid_argument("buildMask: invalid size");
    if (cutoff <= 0) cutoff = 1.0;

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
            if (type == FrequencyFilterType::HIGH_PASS)
                val = 1.0f - val;

            mask.at<float>(y, x) = val;
        }
    }
    return mask;
}

cv::Mat FrequencyProcessor::computeMagnitudeSpectrum(const cv::Mat& complexImg) {
    return utils::FourierUtils::magnitudeSpectrumLog8U(complexImg);
}

FrequencyResult FrequencyProcessor::apply(const cv::Mat& input, const FrequencyParams& params) {
    if (input.empty()) throw std::invalid_argument("FrequencyProcessor::apply: input is empty");


    cv::Mat gray = utils::toGrayscale(input);

    cv::Mat complexImg = utils::FourierUtils::forwardDFT(gray);
    cv::Mat magLog = computeMagnitudeSpectrum(complexImg);

    utils::FourierUtils::fftShift(complexImg);

    cv::Mat mask = buildMask(complexImg.rows, complexImg.cols, params.cutoff, params.filterType);

    cv::Mat filteredComplex = utils::FourierUtils::applyMask(complexImg, mask);

    // 6) shift back before inverse
    utils::FourierUtils::fftShift(filteredComplex);

    // 7) inverse DFT
    cv::Mat invFloat = utils::FourierUtils::inverseDFTReal(filteredComplex);

    // 8) crop back to original size (because we padded)
    invFloat = invFloat(cv::Rect(0, 0, gray.cols, gray.rows));

    // 9) clamp + convert to 8-bit for output
    cv::Mat inv8u;
    cv::normalize(invFloat, invFloat, 0, 255, cv::NORM_MINMAX);
    invFloat.convertTo(inv8u, CV_8U);

    FrequencyResult out;
    out.filtered = inv8u;
    out.magnitudeLog = magLog;
    return out;
}

} // namespace processing