#pragma once
#include <opencv2/opencv.hpp>

namespace utils {

class FourierUtils {
public:
    static cv::Mat forwardDFT(const cv::Mat& gray8u);

    // Compute inverse DFT from complex image (CV_32FC2) -> grayscale float (CV_32F).
    static cv::Mat inverseDFTReal(const cv::Mat& complexImg);

    // Shift quadrants (center <-> corners). Works for 1-channel or 2-channel matrices.
    static void fftShift(cv::Mat& img);

    // Compute log-magnitude spectrum image for visualization (returns CV_8U).
    static cv::Mat magnitudeSpectrumLog8U(const cv::Mat& complexImg);

    // Multiply complex spectrum by a 1-channel float mask (CV_32F).
    // complexImg must be CV_32FC2, mask must be CV_32F same size.
    static cv::Mat applyMask(const cv::Mat& complexImg, const cv::Mat& mask);
};

} // namespace utils