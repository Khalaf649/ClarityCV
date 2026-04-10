#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

namespace processing {

enum class MatchingMethod {
    SSD,
    NCC
};

struct FeatureMatchingParams {
    MatchingMethod method      = MatchingMethod::SSD;
    int    maxMatches          = 50;    // Maximum number of matches to display
    float  ratioThreshold      = 0.75f; // Lowe's ratio test threshold
    double contrastThreshold   = 0.01;  // SIFT contrast threshold
    int    nfeatures           = 500;   // Max SIFT keypoints per image
};

struct FeatureMatchingResult {
    cv::Mat image;                          // Side-by-side image with match lines drawn
    long long computationTimeMs     = 0;    // Total matching time (ms)
    long long siftTimeImg1Ms        = 0;    // SIFT time for image 1
    long long siftTimeImg2Ms        = 0;    // SIFT time for image 2
    int matchesCount                = 0;    // Number of good matches found
    int keypointsImg1               = 0;    // Keypoints in image 1
    int keypointsImg2               = 0;    // Keypoints in image 2
};

class FeatureMatcher {
public:
    static FeatureMatchingResult match(const cv::Mat& img1,
                                       const cv::Mat& img2,
                                       const FeatureMatchingParams& params);

private:
    // Run SIFT on a single image and return keypoints + descriptors
    static void runSIFT(const cv::Mat& img,
                        const FeatureMatchingParams& params,
                        std::vector<cv::KeyPoint>& keypoints,
                        cv::Mat& descriptors,
                        long long& timeMs);

    // SSD: match by minimising sum-of-squared-differences between descriptor vectors
    static std::vector<cv::DMatch> matchSSD(const cv::Mat& desc1,
                                             const cv::Mat& desc2,
                                             const FeatureMatchingParams& params);

    // NCC: match by maximising normalised cross-correlation between descriptor vectors
    static std::vector<cv::DMatch> matchNCC(const cv::Mat& desc1,
                                             const cv::Mat& desc2,
                                             const FeatureMatchingParams& params);

    // Draw the matches side by side and return the visualisation image
    static cv::Mat drawMatchesImage(const cv::Mat& img1,
                                    const std::vector<cv::KeyPoint>& kp1,
                                    const cv::Mat& img2,
                                    const std::vector<cv::KeyPoint>& kp2,
                                    const std::vector<cv::DMatch>& matches);
};

} // namespace processing
