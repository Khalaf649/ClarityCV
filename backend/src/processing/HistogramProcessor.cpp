#include "processing/HistogramProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace processing {


// ---------------------------------------------------------------------------
// Compute histogram of image
// ---------------------------------------------------------------------------

HistogramResult HistogramProcessor::compute(const cv::Mat& input)
{
    if (input.empty())
        throw std::runtime_error("HistogramProcessor::compute(): empty image");

    HistogramResult result;

    std::vector<cv::Mat> splitChannels;

    if (input.channels() == 3)
        cv::split(input, splitChannels);
    else
        splitChannels = {input};

    for (int i = 0; i < (int)splitChannels.size(); i++)
    {
        std::string label =
            splitChannels.size() == 1 ? "Gray" :
            i == 0 ? "Blue"  :
            i == 1 ? "Green" : "Red";

        result.channels.push_back(computeChannel(splitChannels[i], label));
    }

    // generate both plots
    result.plotImage          = renderHistogram(result.channels, 512, 400);
    result.plotImageWithCurve = renderHistogramWithCurve(result.channels, 512, 400);

    return result;
}


// ---------------------------------------------------------------------------
// Compute single channel histogram — fills bins, cdf, mean, stddev
// ---------------------------------------------------------------------------

ChannelHistogram HistogramProcessor::computeChannel(
    const cv::Mat& channel,
    const std::string& label)
{
    ChannelHistogram result;
    result.label = label;
    result.bins.resize(256);
    result.cdf.resize(256);

    int   histSize  = 256;
    float range[]   = {0, 256};
    const float* hRange = {range};

    cv::Mat hist;
    cv::calcHist(&channel, 1, 0, cv::Mat(), hist, 1, &histSize, &hRange);

    // copy bins and build CDF
    float sum = 0;
    for (int i = 0; i < 256; i++)
    {
        float v        = hist.at<float>(i);
        result.bins[i] = v;
        sum           += v;
        result.cdf[i]  = sum;
    }

    // normalize CDF to 0..1
    if (sum > 0)
        for (int i = 0; i < 256; i++)
            result.cdf[i] /= sum;

    // compute mean from histogram bins
    double mean = 0.0;
    for (int i = 0; i < 256; i++)
        mean += i * result.bins[i];
    if (sum > 0)
        mean /= sum;

    // compute variance / stddev from histogram bins
    double variance = 0.0;
    for (int i = 0; i < 256; i++)
        variance += result.bins[i] * (i - mean) * (i - mean);
    if (sum > 0)
        variance /= sum;

    result.mean   = mean;
    result.stddev = std::sqrt(variance);

    return result;
}


// ---------------------------------------------------------------------------
// Equalize histogram
// ---------------------------------------------------------------------------

cv::Mat HistogramProcessor::equalize(const cv::Mat& input)
{
    if (input.empty())
        throw std::runtime_error("HistogramProcessor::equalize(): empty image");

    cv::Mat result;

    if (input.channels() == 1)
    {
        cv::equalizeHist(input, result);
    }
    else
    {
        cv::Mat ycrcb;
        cv::cvtColor(input, ycrcb, cv::COLOR_BGR2YCrCb);

        std::vector<cv::Mat> channels;
        cv::split(ycrcb, channels);
        cv::equalizeHist(channels[0], channels[0]);
        cv::merge(channels, ycrcb);

        cv::cvtColor(ycrcb, result, cv::COLOR_YCrCb2BGR);
    }

    return result;
}


// ---------------------------------------------------------------------------
// Normalize image
// ---------------------------------------------------------------------------

cv::Mat HistogramProcessor::normalize(const cv::Mat& input)
{
    if (input.empty())
        throw std::runtime_error("HistogramProcessor::normalize(): empty image");

    cv::Mat result;
    cv::normalize(input, result, 0, 255, cv::NORM_MINMAX);
    result.convertTo(result, input.type());
    return result;
}


// ---------------------------------------------------------------------------
// Render plain histogram (line chart, original style)
// ---------------------------------------------------------------------------

cv::Mat HistogramProcessor::renderHistogram(
    const std::vector<ChannelHistogram>& channels,
    int width,
    int height)
{
    if (channels.empty())
        throw std::runtime_error("renderHistogram(): no channels");

    cv::Mat image(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

    int   bins     = 256;
    float maxValue = 0;
    for (const auto& ch : channels)
        for (float v : ch.bins)
            maxValue = std::max(maxValue, v);

    if (maxValue == 0) return image;

    int binWidth = width / bins;

    std::vector<cv::Scalar> colors = {
        cv::Scalar(255, 0,   0),   // Blue
        cv::Scalar(0,   255, 0),   // Green
        cv::Scalar(0,   0,   255)  // Red
    };

    for (int c = 0; c < (int)channels.size(); c++)
    {
        const auto& hist = channels[c];
        for (int i = 1; i < bins; i++)
        {
            int x1 = (i - 1) * binWidth;
            int x2 = i       * binWidth;
            int y1 = height - cvRound(hist.bins[i - 1] * height / maxValue);
            int y2 = height - cvRound(hist.bins[i]     * height / maxValue);

            cv::line(image,
                     cv::Point(x1, y1),
                     cv::Point(x2, y2),
                     colors[c % colors.size()],
                     2);
        }
    }

    return image;
}


// ---------------------------------------------------------------------------
// Render histogram bars + Gaussian distribution curve overlay
// ---------------------------------------------------------------------------

cv::Mat HistogramProcessor::renderHistogramWithCurve(
    const std::vector<ChannelHistogram>& channels,
    int width,
    int height)
{
    if (channels.empty())
        throw std::runtime_error("renderHistogramWithCurve(): no channels");

    // dark background
    cv::Mat image(height, width, CV_8UC3, cv::Scalar(20, 20, 20));

    int   bins     = 256;
    float maxValue = 0;
    for (const auto& ch : channels)
        for (float v : ch.bins)
            maxValue = std::max(maxValue, v);

    if (maxValue == 0) return image;

    int binWidth = width / bins;

    // colors for bar fills
    std::vector<cv::Scalar> barColors = {
        cv::Scalar(130, 60,  60),   // BGR: dark-blue bar for Blue channel
        cv::Scalar(60,  130, 60),   // dark-green bar for Green channel
        cv::Scalar(60,  60,  130)   // dark-red bar for Red channel
    };

    // bright colors for the Gaussian curve
    std::vector<cv::Scalar> curveColors = {
        cv::Scalar(255, 120, 120),  // bright blue
        cv::Scalar(120, 255, 120),  // bright green
        cv::Scalar(120, 120, 255)   // bright red
    };

    // grayscale overrides
    if (channels.size() == 1)
    {
        barColors   = { cv::Scalar(90, 90, 90) };
        curveColors = { cv::Scalar(255, 255, 255) };
    }

    // ---- draw histogram bars ----
    for (int c = 0; c < (int)channels.size(); c++)
    {
        const auto& hist = channels[c];
        for (int i = 0; i < bins; i++)
        {
            int barH = cvRound(hist.bins[i] * height / maxValue);
            if (barH <= 0) continue;

            int x1 = i * binWidth;
            int x2 = x1 + binWidth;

            cv::rectangle(
                image,
                cv::Point(x1, height - barH),
                cv::Point(x2, height),
                barColors[c % barColors.size()],
                cv::FILLED
            );
        }
    }

    // ---- draw Gaussian distribution curve overlay ----
    //
    // gaussian(x) = maxValue * exp( -0.5 * ((x - mu) / sigma)^2 )
    // This peaks at maxValue when x == mu, matching the histogram scale.
    //
    for (int c = 0; c < (int)channels.size(); c++)
    {
        const auto& ch = channels[c];

        if (ch.stddev < 1e-6) continue; // flat / constant image — skip

        double mu    = ch.mean;
        double sigma = ch.stddev;

        std::vector<cv::Point> pts;
        pts.reserve(bins);

        for (int i = 0; i < bins; i++)
        {
            double x   = i + 0.5; // bin centre
            double exp_val  = -0.5 * ((x - mu) / sigma) * ((x - mu) / sigma);
            double gaussVal = maxValue * std::exp(exp_val);

            int px = i * binWidth + binWidth / 2;
            int py = height - cvRound(gaussVal * height / maxValue);
            py     = std::max(0, std::min(height - 1, py));

            pts.emplace_back(px, py);
        }

        // draw as anti-aliased polyline
        for (int i = 1; i < (int)pts.size(); i++)
        {
            cv::line(
                image,
                pts[i - 1],
                pts[i],
                curveColors[c % curveColors.size()],
                2,
                cv::LINE_AA
            );
        }
    }

    // ---- subtle baseline ----
    cv::line(image,
             cv::Point(0,       height - 1),
             cv::Point(width - 1, height - 1),
             cv::Scalar(70, 70, 70), 1);

    return image;
}


} // namespace processing