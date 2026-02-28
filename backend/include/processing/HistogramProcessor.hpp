#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

namespace processing {

struct ChannelHistogram {
    std::string          label;   // "gray", "red", "green", "blue"
    std::vector<double>  bins;    // 256 frequency values (normalized 0-1)
    std::vector<double>  cdf;     // Cumulative distribution function
};

struct HistogramResult {
    std::vector<ChannelHistogram> channels;
    cv::Mat                       plotImage;   // Rendered histogram image (PNG)
};

class HistogramProcessor {
public:
    static HistogramResult compute(const cv::Mat& input);
    static cv::Mat          equalize(const cv::Mat& input);
    static cv::Mat          normalize(const cv::Mat& input);

private:
    static ChannelHistogram computeChannel(const cv::Mat& channel, const std::string& label);
    static cv::Mat          renderHistogram(const std::vector<ChannelHistogram>& channels,
                                            int width = 512, int height = 400);
};

} // namespace processing
