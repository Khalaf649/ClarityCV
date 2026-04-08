#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

namespace processing {

// SIFT parameters
struct SIFTParams {
    // Detection parameters
    double contrastThreshold = 0.01;   // Threshold for low-contrast points
    int    nfeatures         = 1000;   // Maximum keypoints to extract
    double sigma             = 1.6;    // Initial Gaussian sigma
    int    octaves           = 4;      // Number of scale-space octaves
    int    scales            = 5;      // Scales per octave
    double edgeThreshold     = 10.0;   // Hessian ratio threshold (filter edges)
    double peakThreshold     = 0.005;  // DoG value threshold
    
    // Visualization parameters (BGR format)
    int    circleColor_B     = 203;    // Blue channel of circle color (default: pink = 203,192,255)
    int    circleColor_G     = 192;    // Green channel of circle color
    int    circleColor_R     = 255;    // Red channel of circle color
    int    lineColor_B       = 255;    // Blue channel of orientation line color (default: magenta)
    int    lineColor_G       = 0;      // Green channel of orientation line color
    int    lineColor_R       = 255;    // Red channel of orientation line color
    int    circleThickness   = 2;      // Thickness of keypoint circle
    int    lineThickness     = 2;      // Thickness of orientation line
    bool   drawKeypoints     = true;   // Whether to draw keypoints on output
};

// SIFT result output
struct SIFTResult {
    cv::Mat image;                      // Output image with keypoints drawn
    std::vector<cv::KeyPoint> keypoints;// Detected keypoints
    cv::Mat descriptors;                // 128-D descriptors
    long long computationTimeMs = 0;    // Execution time
    int featureCount = 0;               // Number of keypoints found
};

// SIFT Implementation
class SIFTProcessor {
public:
    // Main entry point: applies all 6 stages of SIFT
    static SIFTResult apply(const cv::Mat& input, const SIFTParams& params);

private:
    // Stage 1: Build Gaussian pyramid (scale-space)
    static std::vector<std::vector<cv::Mat>> stage_buildScaleSpace(
        const cv::Mat& gray, 
        const SIFTParams& params
    );

    // Stage 2: Compute Difference of Gaussian (DoG)
    static std::vector<std::vector<cv::Mat>> stage_buildDoGPyramid(
        const std::vector<std::vector<cv::Mat>>& gaussianPyramid
    );

    // Stage 3: Detect keypoints at local extrema
    static std::vector<cv::KeyPoint> stage_detectKeypointExtrema(
        const std::vector<std::vector<cv::Mat>>& dogPyramid,
        const SIFTParams& params
    );

    // Stage 4: Localize keypoints and remove edge responses
    static std::vector<cv::KeyPoint> stage_localizeAndFilterKeypoints(
        const std::vector<cv::KeyPoint>& rawKeypoints,
        const std::vector<std::vector<cv::Mat>>& dogPyramid,
        const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
        const SIFTParams& params
    );

    // Stage 5: Assign dominant orientations to keypoints
    static void stage_assignOrientations(
        std::vector<cv::KeyPoint>& keypoints,
        const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
        const SIFTParams& params
    );

    // Stage 6: Generate 128-D SIFT descriptors
    static cv::Mat stage_generateDescriptors(
        const std::vector<cv::KeyPoint>& keypoints,
        const std::vector<std::vector<cv::Mat>>& gaussianPyramid,
        const SIFTParams& params
    );
};

} // namespace processing
