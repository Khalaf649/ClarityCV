#include "utils/ImageUtils.hpp"
#include "utils/Base64.hpp"
#include <stdexcept>

namespace utils {

cv::Mat decodeImageFromBase64(const std::string& base64Str) {
    std::string data = base64Str;
    auto commaPos = data.find(',');
    if (commaPos != std::string::npos)
        data = data.substr(commaPos + 1);

    std::vector<uchar> bytes = base64Decode(data);
    if (bytes.empty())
        throw std::invalid_argument("Failed to decode base64 image data.");

    cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
    if (img.empty())
        throw std::invalid_argument("Failed to decode image from bytes.");
    return img;
}

std::string encodeImageToBase64(const cv::Mat& image, const std::string& ext) {
    if(image.empty()) return "";
    std::vector<uchar> buf;
    cv::imencode(ext, image, buf);
    return base64Encode(buf);
}

cv::Mat toGrayscale(const cv::Mat& image) {
    if (image.empty())
        throw std::invalid_argument("toGrayscale: image is empty");

    // Already grayscale
    if (image.channels() == 1)
        return image.clone();

    // 3-channel (BGR)
    if (image.channels() == 3) {
        cv::Mat gray(image.size(), CV_8U);

        for (int y = 0; y < image.rows; ++y) {
            for (int x = 0; x < image.cols; ++x) {
                cv::Vec3b v = image.at<cv::Vec3b>(y, x);
                gray.at<uchar>(y, x) = static_cast<uchar>(
                    0.299 * v[2] + 0.587 * v[1] + 0.114 * v[0]
                );
            }
        }
        return gray;
    }

    // 4-channel (BGRA)
    if (image.channels() == 4) {
        cv::Mat gray(image.size(), CV_8U);

        for (int y = 0; y < image.rows; ++y) {
            for (int x = 0; x < image.cols; ++x) {
                cv::Vec4b v = image.at<cv::Vec4b>(y, x);
                gray.at<uchar>(y, x) = static_cast<uchar>(
                    0.299 * v[2] + 0.587 * v[1] + 0.114 * v[0]
                );
                // ignore alpha channel v[3]
            }
        }
        return gray;
    }

    throw std::invalid_argument("toGrayscale: only 1-, 3-, or 4-channel images are supported");
}

cv::Mat toRGB(const cv::Mat& image) {
    if (image.channels() == 3) return image.clone();
    cv::Mat rgb;
    if      (image.channels() == 1) cv::cvtColor(image, rgb, cv::COLOR_GRAY2BGR);
    else if (image.channels() == 4) cv::cvtColor(image, rgb, cv::COLOR_BGRA2BGR);
    else return image.clone();
    return rgb;
}

cv::Mat convolutionFast(const cv::Mat& image, const cv::Mat& kernelD, int outDepth) {
    // Work in float throughout
    cv::Mat kernel;
    kernelD.convertTo(kernel, CV_32F);

    const int kCX = kernel.cols / 2;
    const int kCY = kernel.rows / 2;

    cv::Mat padded;
    cv::copyMakeBorder(image, padded, kCY, kCY, kCX, kCX, cv::BORDER_REFLECT_101);

    cv::Mat result(image.size(), outDepth);

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < image.rows; ++y) {
        uchar* dst8 = (outDepth == CV_8U) ? result.ptr<uchar>(y) : nullptr;
        float* dst32 = (outDepth == CV_32F) ? result.ptr<float>(y) : nullptr;
        for (int x = 0; x < image.cols; ++x) {
            float sum = 0.0f;
            for (int j = 0; j < kernel.rows; ++j) {
                const uchar* imgRow = padded.ptr<uchar>(y + j);
                const float* kerRow = kernel.ptr<float>(j);
                for (int i = 0; i < kernel.cols; ++i)
                    sum += imgRow[x + i] * kerRow[i];  
            }
            if (outDepth == CV_8U) {
                dst8[x] = cv::saturate_cast<uchar>(sum);
            } else if (outDepth == CV_32F) {
                dst32[x] = sum;
            }
        }
    }
    return result;
}

} // namespace utils

