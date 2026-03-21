#pragma once
#include <opencv2/opencv.hpp>
#include <string>

namespace processing {

enum class HoughShapeType {
    LINE,
    CIRCLE,
    ELLIPSE
};

struct HoughParams {
    HoughShapeType shapeType = HoughShapeType::LINE;
    int votesThreshold       = 100;
};

struct HoughResult {
    cv::Mat transformImage;
    // std::vector<cv::Vec4i> lines;
    // std::vector<cv::Vec3f> circles;
    // std::vector<cv::Vec4f> ellipses;
};

class HoughProcessor {
public:
    static HoughResult apply(const cv::Mat& input, const HoughParams& params);
// private:
//     static HoughResult applyLine(const cv::Mat& input, const HoughParams& params);
//     static HoughResult applyCircle(const cv::Mat& input, const HoughParams& params);
//     static HoughResult applyEllipse(const cv::Mat& input, const HoughParams& params);
};

} // namespace processing