#define _USE_MATH_DEFINES
#include "processing/EdgeDetector.hpp"
#include "utils/ImageUtils.hpp"
#include <stdexcept>
#include <cmath>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

namespace processing {

cv::Mat EdgeDetector::toGray(const cv::Mat& input) {
    if (input.channels() == 1) return input.clone();
    if (input.channels() == 3) {
        cv::Mat gray(input.size(), CV_8U);
        for (int y = 0; y < input.rows; ++y) {
            for (int x = 0; x < input.cols; ++x) {
                cv::Vec3b v = input.at<cv::Vec3b>(y, x);
                gray.at<uchar>(y, x) = static_cast<uchar>(
                    0.299 * v[2] + 0.587 * v[1] + 0.114 * v[0]
                );
            }
        }
        return gray;
    }
    throw std::invalid_argument("toGray: only 1- or 3-channel images are supported");
}


EdgeResult EdgeDetector::detect(const cv::Mat& input, const EdgeParams& params) {
    switch (params.type) {
        case EdgeType::SOBEL:
            return detectSobel(toGray(input), params);
        case EdgeType::ROBERTS:
            return detectRoberts(toGray(input));
        case EdgeType::PREWITT:
            return detectPrewitt(toGray(input));
        case EdgeType::CANNY:
            return detectCanny(toGray(input), params.cannyLow, params.cannyHigh);
        default:
            throw std::invalid_argument("Unknown edge detection type.");
    }
}

cv::Mat EdgeDetector::magnitude(const cv::Mat& gx, const cv::Mat& gy) {
    if (gx.size() != gy.size() || gx.type() != gy.type()) {
        throw std::invalid_argument("magnitude: gx and gy must have same size and type");
    }
    cv::Mat mag(gx.size(), gx.type());
    for (int y = 0; y < gx.rows; ++y) {
        for (int x = 0; x < gx.cols; ++x) {
            float vx = gx.at<float>(y, x);
            float vy = gy.at<float>(y, x);
            mag.at<float>(y, x) = std::sqrt(vx * vx + vy * vy);
        }
    }
    return mag;
}

cv::Mat EdgeDetector::normalize8U(const cv::Mat& src) {
    if (src.empty()) return cv::Mat();
    if (src.type() == CV_8U) return src.clone();
    double minVal, maxVal;
    cv::minMaxLoc(src, &minVal, &maxVal);
    if (maxVal == minVal) {
        return cv::Mat::zeros(src.size(), CV_8U);
    }
    double scale = 255.0 / (maxVal - minVal);
    cv::Mat dst;
    src.convertTo(dst, CV_8U, scale, -minVal * scale);
    return dst;
}



EdgeResult EdgeDetector::detectSobel(const cv::Mat& gray, const EdgeParams& params) {
    cv::Mat kxMat = (cv::Mat_<float>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
    cv::Mat kyMat = (cv::Mat_<float>(3, 3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);

    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);

    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

EdgeResult EdgeDetector::detectRoberts(const cv::Mat& gray) {
    cv::Mat kxMat = (cv::Mat_<float>(2, 2) << 1, 0, 0, -1);
    cv::Mat kyMat = (cv::Mat_<float>(2, 2) << 0, 1, -1, 0);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);
    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);
    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

EdgeResult EdgeDetector::detectPrewitt(const cv::Mat& gray) {
const int kx[3][3] = {
        {-1, 0, 1},
        {-1, 0, 1},
        {-1, 0, 1}
    };
    const int ky[3][3] = {
        {-1, -1, -1},
        { 0,  0,  0},
        { 1,  1,  1}
    };
    cv::Mat kxMat = (cv::Mat_<float>(3, 3) << -1, 0, 1, -1, 0, 1, -1, 0, 1);
    cv::Mat kyMat = (cv::Mat_<float>(3, 3) << -1, -1, -1, 0, 0, 0, 1, 1, 1);

    cv::Mat gx = utils::convolutionFast(gray, kxMat, CV_32F);
    cv::Mat gy = utils::convolutionFast(gray, kyMat, CV_32F);

    // compute absolute images and magnitude, normalize to 8-bit for output
    cv::Mat absGx = cv::abs(gx);
    cv::Mat absGy = cv::abs(gy);
    cv::Mat mag = magnitude(gx, gy);
    cv::Mat outGx = normalize8U(absGx);
    cv::Mat outGy = normalize8U(absGy);
    cv::Mat outMag = normalize8U(mag);
    EdgeResult result;
    result.edgeX = outGx;
    result.edgeY = outGy;
    result.combined = outMag;
    return result;
}

// ---------------------------------------------------------------------------
// Canny edge detector — implemented from scratch (no cv::Canny call).
//
// Pipeline:
//   1. Gaussian blur         — suppress noise with a 5×5 kernel
//   2. Sobel gradients       — compute Gx, Gy, magnitude, and angle
//   3. Non-maximum suppression (NMS) — thin edges to 1-pixel width
//   4. Double threshold      — label pixels STRONG / WEAK / NONE
//   5. Hysteresis tracking   — keep WEAK pixels connected to STRONG ones
// ---------------------------------------------------------------------------

EdgeResult EdgeDetector::detectCanny(const cv::Mat& gray, double low, double high) {

    // ------------------------------------------------------------------
    // 1. Gaussian blur (5×5, σ ≈ 1.4)
    //    Weights computed from a separable Gaussian; pre-divided so the
    //    kernel sums to 1 and we can keep float convolution uniform.
    // ------------------------------------------------------------------
    cv::Mat gaussKernel = (cv::Mat_<float>(5, 5) <<
         2,  4,  5,  4,  2,
         4,  9, 12,  9,  4,
         5, 12, 15, 12,  5,
         4,  9, 12,  9,  4,
         2,  4,  5,  4,  2) * (1.0f / 159.0f);

    cv::Mat blurred = utils::convolutionFast(gray, gaussKernel, CV_32F);

    // ------------------------------------------------------------------
    // 2. Sobel gradients
    // ------------------------------------------------------------------
    cv::Mat sobelX = (cv::Mat_<float>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
    cv::Mat sobelY = (cv::Mat_<float>(3, 3) << -1, -2, -1, 0, 0, 0, 1, 2, 1);

    // convolutionFast expects uchar input — convert blurred back to 8U first
    cv::Mat blurred8U;
    blurred.convertTo(blurred8U, CV_8U);

    cv::Mat gx = utils::convolutionFast(blurred8U, sobelX, CV_32F);
    cv::Mat gy = utils::convolutionFast(blurred8U, sobelY, CV_32F);

    const int rows = gray.rows;
    const int cols = gray.cols;

    // Gradient magnitude and angle
    cv::Mat mag(rows, cols, CV_32F);
    cv::Mat angle(rows, cols, CV_32F);  // degrees [0, 180)

    for (int y = 0; y < rows; ++y) {
        const float* gxRow  = gx.ptr<float>(y);
        const float* gyRow  = gy.ptr<float>(y);
        float*       magRow = mag.ptr<float>(y);
        float*       angRow = angle.ptr<float>(y);

        for (int x = 0; x < cols; ++x) {
            float vx = gxRow[x];
            float vy = gyRow[x];
            magRow[x] = std::sqrt(vx * vx + vy * vy);

            // atan2 returns (-π, π]; map to [0°, 180°)
            float deg = std::atan2(vy, vx) * (180.0f / static_cast<float>(M_PI));
            if (deg < 0.0f) deg += 180.0f;
            angRow[x] = deg;
        }
    }

    // ------------------------------------------------------------------
    // 3. Non-maximum suppression
    //    Round each gradient angle to the nearest 45° and compare the
    //    current pixel against its two neighbours along that direction.
    //    Keep the pixel only if it is a local maximum.
    // ------------------------------------------------------------------
    cv::Mat nms = cv::Mat::zeros(rows, cols, CV_32F);

    for (int y = 1; y < rows - 1; ++y) {
        const float* magRow = mag.ptr<float>(y);
        const float* angRow = angle.ptr<float>(y);
        float*       nmsRow = nms.ptr<float>(y);

        for (int x = 1; x < cols - 1; ++x) {
            float deg = angRow[x];
            float cur = magRow[x];

            float n1, n2;   // the two neighbours along the gradient direction

            if (deg < 22.5f || deg >= 157.5f) {
                // 0° — horizontal edge — compare left / right
                n1 = mag.at<float>(y, x - 1);
                n2 = mag.at<float>(y, x + 1);
            } else if (deg < 67.5f) {
                // 45° — diagonal
                n1 = mag.at<float>(y - 1, x + 1);
                n2 = mag.at<float>(y + 1, x - 1);
            } else if (deg < 112.5f) {
                // 90° — vertical edge — compare above / below
                n1 = mag.at<float>(y - 1, x);
                n2 = mag.at<float>(y + 1, x);
            } else {
                // 135° — diagonal
                n1 = mag.at<float>(y - 1, x - 1);
                n2 = mag.at<float>(y + 1, x + 1);
            }

            nmsRow[x] = (cur >= n1 && cur >= n2) ? cur : 0.0f;
        }
    }

    // ------------------------------------------------------------------
    // 4. Double threshold
    //    Pixels above `high`  → STRONG (255)
    //    Pixels in [low,high] → WEAK   (128)
    //    Pixels below `low`  → suppressed (0)
    // ------------------------------------------------------------------
    const uchar STRONG = 255;
    const uchar WEAK   = 128;

    cv::Mat thresh = cv::Mat::zeros(rows, cols, CV_8U);

    for (int y = 0; y < rows; ++y) {
        const float* nmsRow = nms.ptr<float>(y);
        uchar*       tRow   = thresh.ptr<uchar>(y);
        for (int x = 0; x < cols; ++x) {
            float v = nmsRow[x];
            if      (v >= static_cast<float>(high)) tRow[x] = STRONG;
            else if (v >= static_cast<float>(low))  tRow[x] = WEAK;
            // else stays 0
        }
    }

    // ------------------------------------------------------------------
    // 5. Hysteresis edge tracking
    //    Walk from every STRONG pixel and promote any 8-connected WEAK
    //    neighbour to STRONG.  Repeat until no more promotions occur
    //    (iterative flood-fill; avoids deep recursion on large images).
    // ------------------------------------------------------------------
    cv::Mat edges = thresh.clone();
    bool changed = true;

    while (changed) {
        changed = false;
        for (int y = 1; y < rows - 1; ++y) {
            uchar* eRow = edges.ptr<uchar>(y);
            for (int x = 1; x < cols - 1; ++x) {
                if (eRow[x] != WEAK) continue;

                // Check all 8 neighbours for a STRONG pixel
                bool nearStrong = false;
                for (int dy = -1; dy <= 1 && !nearStrong; ++dy)
                    for (int dx = -1; dx <= 1 && !nearStrong; ++dx)
                        if (!(dy == 0 && dx == 0))
                            nearStrong = (edges.at<uchar>(y + dy, x + dx) == STRONG);

                if (nearStrong) {
                    eRow[x] = STRONG;
                    changed = true;
                }
            }
        }
    }

    // Suppress any remaining WEAK pixels that were not connected to a strong edge
    for (int y = 0; y < rows; ++y) {
        uchar* eRow = edges.ptr<uchar>(y);
        for (int x = 0; x < cols; ++x)
            if (eRow[x] == WEAK) eRow[x] = 0;
    }

    EdgeResult result;
    result.edgeX    = cv::Mat();  // not applicable for Canny
    result.edgeY    = cv::Mat();  // not applicable for Canny
    result.combined = edges;
    return result;
}

} // namespace processing
