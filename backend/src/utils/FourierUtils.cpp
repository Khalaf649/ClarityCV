#include "utils/FourierUtils.hpp"
#include <stdexcept>

namespace utils {

cv::Mat FourierUtils::forwardDFT(const cv::Mat& gray8u) {
    if (gray8u.empty()) throw std::invalid_argument("forwardDFT: input is empty");
    if (gray8u.channels() != 1) throw std::invalid_argument("forwardDFT: input must be 1-channel grayscale");

    cv::Mat floatImg;
    gray8u.convertTo(floatImg, CV_32F);

    // Optional padding for speed
    int m = cv::getOptimalDFTSize(floatImg.rows);
    int n = cv::getOptimalDFTSize(floatImg.cols);

    cv::Mat padded;
    cv::copyMakeBorder(floatImg, padded,
                       0, m - floatImg.rows,
                       0, n - floatImg.cols,
                       cv::BORDER_CONSTANT, cv::Scalar::all(0));

    cv::Mat planes[] = {
        padded,
        cv::Mat::zeros(padded.size(), CV_32F)
    };
    cv::Mat complexImg;
    cv::merge(planes, 2, complexImg);      // CV_32FC2
    cv::dft(complexImg, complexImg);       // forward DFT
    return complexImg;
}

cv::Mat FourierUtils::inverseDFTReal(const cv::Mat& complexImg) {
    if (complexImg.empty()) throw std::invalid_argument("inverseDFTReal: complexImg empty");
    if (complexImg.type() != CV_32FC2) throw std::invalid_argument("inverseDFTReal: expected CV_32FC2");

    cv::Mat inv;
    cv::dft(complexImg, inv, cv::DFT_INVERSE | cv::DFT_REAL_OUTPUT | cv::DFT_SCALE); // CV_32F
    return inv;
}

void FourierUtils::fftShift(cv::Mat& img) {
    if (img.empty()) return;

    // crop if odd
    img = img(cv::Rect(0, 0, img.cols & -2, img.rows & -2));

    int cx = img.cols / 2;
    int cy = img.rows / 2;

    cv::Mat q0(img, cv::Rect(0, 0, cx, cy));
    cv::Mat q1(img, cv::Rect(cx, 0, cx, cy));
    cv::Mat q2(img, cv::Rect(0, cy, cx, cy));
    cv::Mat q3(img, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
    q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);
}

cv::Mat FourierUtils::magnitudeSpectrumLog8U(const cv::Mat& complexImg) {
    if (complexImg.empty()) throw std::invalid_argument("magnitudeSpectrumLog8U: complexImg empty");
    if (complexImg.type() != CV_32FC2) throw std::invalid_argument("magnitudeSpectrumLog8U: expected CV_32FC2");

    cv::Mat planes[2];
    cv::split(complexImg, planes);

    cv::Mat mag;
    cv::magnitude(planes[0], planes[1], mag); // CV_32F

    mag += 1.0f;
    cv::log(mag, mag);

    // shift to center for display
    fftShift(mag);

    cv::normalize(mag, mag, 0, 255, cv::NORM_MINMAX);
    cv::Mat mag8u;
    mag.convertTo(mag8u, CV_8U);
    return mag8u;
}

cv::Mat FourierUtils::applyMask(const cv::Mat& complexImg, const cv::Mat& mask) {
    if (complexImg.empty() || mask.empty())
        throw std::invalid_argument("applyMask: empty input");

    if (complexImg.type() != CV_32FC2)
        throw std::invalid_argument("applyMask: complexImg must be CV_32FC2");

    if (mask.type() != CV_32F)
        throw std::invalid_argument("applyMask: mask must be CV_32F");

    if (complexImg.size() != mask.size())
        throw std::invalid_argument("applyMask: mask size must match complexImg size");

    cv::Mat planes[2];
    cv::split(complexImg, planes);

    planes[0] = planes[0].mul(mask);
    planes[1] = planes[1].mul(mask);

    cv::Mat out;
    cv::merge(planes, 2, out);
    return out;
}

} // namespace utils