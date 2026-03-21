#include "processing/EdgeDetector.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>
#include <cmath>

namespace processing {

cv::Mat EdgeDetector::toGray(const cv::Mat& input) {
    if (input.channels() == 1) return input.clone();
    if (input.channels() == 3) {
        cv::Mat gray(input.size(), CV_8U);
        for (int y = 0; y < input.rows; ++y) {
            for (int x = 0; x < input.cols; ++x) {
                cv::Vec3b v = input.at<cv::Vec3b>(y, x);
                gray.at<uchar>(y, x) = static_cast<uchar>(
                    0.299 * v[2] + 0.587 * v[1] + 0.114 * v[0]
                );
            }
        }
        return gray;
    }
    throw std::invalid_argument("toGray: only 1- or 3-channel images are supported");
}


EdgeResult EdgeDetector::detect(const cv::Mat& input, const EdgeParams& params) {
    switch (params.type) {
        case EdgeType::SOBEL:
            return detectSobel(toGray(input), params);
        case EdgeType::ROBERTS:
            return detectRoberts(toGray(input));
        case EdgeType::PREWITT:
            return detectPrewitt(toGray(input));
        case EdgeType::CANNY:
            return detectCanny(toGray(input), params.cannyLow, params.cannyHigh);
        default:
            throw std::invalid_argument("Unknown edge detection type.");
    }
}

cv::Mat EdgeDetector::magnitude(const cv::Mat& gx, const cv::Mat& gy) {
    if (gx.size() != gy.size() || gx.type() != gy.type()) {
        throw std::invalid_argument("magnitude: gx and gy must have same size and type");
    }
    cv::Mat mag(gx.size(), gx.type());
    for (int y = 0; y < gx.rows; ++y) {
        for (int x = 0; x < gx.cols; ++x) {
            float vx = gx.at<float>(y, x);
            float vy = gy.at<float>(y, x);
            mag.at<float>(y, x) = std::sqrt(vx * vx + vy * vy);
        }
    }
    return mag;
}

cv::Mat EdgeDetector::normalize8U(const cv::Mat& src) {
    if (src.empty()) return cv::Mat();
    if (src.type() == CV_8U) return src.clone();
    double minVal, maxVal;
    cv::minMaxLoc(src, &minVal, &maxVal);
    if (maxVal == minVal) {
        return cv::Mat::zeros(src.size(), CV_8U);
    }
    double scale = 255.0 / (maxVal - minVal);
    cv::Mat dst;
    src.convertTo(dst, CV_8U, scale, -minVal * scale);
    return dst;
}



EdgeResult EdgeDetector::detectSobel(const cv::Mat& gray, const EdgeParams& params) {
    cv::Mat kxMat = (cv::Mat_<float>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
    cv::Mat kyMat = (cv::Mat_<float>(3, 3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);

    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);

    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

EdgeResult EdgeDetector::detectRoberts(const cv::Mat& gray) {
    cv::Mat kxMat = (cv::Mat_<float>(2, 2) << 1, 0, 0, -1);
    cv::Mat kyMat = (cv::Mat_<float>(2, 2) << 0, 1, -1, 0);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);
    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);
    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

EdgeResult EdgeDetector::detectPrewitt(const cv::Mat& gray) {
const int kx[3][3] = {
        {-1, 0, 1},
        {-1, 0, 1},
        {-1, 0, 1}
    };
    const int ky[3][3] = {
        {-1, -1, -1},
        { 0,  0,  0},
        { 1,  1,  1}
    };
    cv::Mat kxMat = (cv::Mat_<float>(3, 3) << -1, 0, 1, -1, 0, 1, -1, 0, 1);
    cv::Mat kyMat = (cv::Mat_<float>(3, 3) << -1, -1, -1, 0, 0, 0, 1, 1, 1);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);
    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);
    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

EdgeResult EdgeDetector::detectCanny(const cv::Mat& gray, double low, double high) {
    cv::Mat edges;
    cv::Canny(gray, edges, low, high);
    EdgeResult result;
    result.edgeX = cv::Mat(); // not applicable for Canny
    result.edgeY = cv::Mat(); // not applicable for Canny
    result.combined = edges;
    return result;
}

} // namespace processing
