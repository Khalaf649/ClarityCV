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

} // namespace utils
