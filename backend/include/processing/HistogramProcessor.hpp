#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

namespace processing {

// ---------------------------------------------------------------------------
// Channel histogram structure
// ---------------------------------------------------------------------------

struct ChannelHistogram
{
    std::string label;

    // histogram bins (256 values)
    std::vector<float> bins;

    // cumulative distribution function (normalized 0..1)
    std::vector<float> cdf;

    // distribution curve stats (computed from bins)
    double mean   = 0.0;
    double stddev = 0.0;
};


// ---------------------------------------------------------------------------
// Histogram result structure
// ---------------------------------------------------------------------------

struct HistogramResult
{
    std::vector<ChannelHistogram> channels;

    // plain histogram plot
    cv::Mat plotImage;

    // histogram bars + Gaussian distribution curve overlay
    cv::Mat plotImageWithCurve;
};


// ---------------------------------------------------------------------------
// Histogram processor
// ---------------------------------------------------------------------------

class HistogramProcessor
{
public:

    // compute histogram, plain plot, and plot-with-curve
    static HistogramResult compute(const cv::Mat& input);

    // equalize histogram
    static cv::Mat equalize(const cv::Mat& input);

    // normalize image
    static cv::Mat normalize(const cv::Mat& input);

private:

    // compute single channel histogram (also fills mean & stddev)
    static ChannelHistogram computeChannel(
        const cv::Mat& channel,
        const std::string& label
    );

    // render plain histogram lines
    static cv::Mat renderHistogram(
        const std::vector<ChannelHistogram>& channels,
        int width,
        int height
    );

    // render histogram bars + Gaussian distribution curve overlay
    static cv::Mat renderHistogramWithCurve(
        const std::vector<ChannelHistogram>& channels,
        int width,
        int height
    );
};

} // namespace processing