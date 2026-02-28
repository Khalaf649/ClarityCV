#include "utils/ImageUtils.hpp"
#include "utils/Base64.hpp"
#include <stdexcept>

namespace utils {

cv::Mat decodeImageFromBase64(const std::string& base64Str) {
    std::string data = base64Str;
    // Strip data URI prefix if present (e.g. "data:image/png;base64,")
    auto commaPos = data.find(',');
    if (commaPos != std::string::npos) {
        data = data.substr(commaPos + 1);
    }

    std::vector<uchar> bytes = base64Decode(data);
    if (bytes.empty()) {
        throw std::invalid_argument("Failed to decode base64 image data.");
    }

    cv::Mat img = cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::invalid_argument("Failed to decode image from bytes.");
    }
    return img;
}

std::string encodeImageToBase64(const cv::Mat& image, const std::string& ext) {
    std::vector<uchar> buf;
    cv::imencode(ext, image, buf);
    return base64Encode(buf);
}

cv::Mat toGrayscale(const cv::Mat& image) {
    if (image.channels() == 1) return image.clone();
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

cv::Mat toRGB(const cv::Mat& image) {
    if (image.channels() == 3) return image.clone();
    if (image.channels() == 1) {
        cv::Mat rgb;
        cv::cvtColor(image, rgb, cv::COLOR_GRAY2BGR);
        return rgb;
    }
    if (image.channels() == 4) {
        cv::Mat rgb;
        cv::cvtColor(image, rgb, cv::COLOR_BGRA2BGR);
        return rgb;
    }
    return image.clone();
}

bool validateImage(const cv::Mat& image) {
    return !image.empty() && image.data != nullptr;
}

} // namespace utils
