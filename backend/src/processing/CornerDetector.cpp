#include "processing/CornerDetector.hpp"
#include "utils/ImageUtils.hpp"
#include "processing/EdgeDetector.hpp"
#include "processing/FilterProcessor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>


namespace processing {

struct Candidate {
    int x;
    int y;
    float r;
};

CornerResult CornerDetector::detect(const cv::Mat& input, const CornerParams& params) {
    cv::Mat gray = utils::toGrayscale(input);

    if (params.mode == CornerDetectionMode::SHI_TOMASI) {
        return detectShiTomasi(gray, params);
    } else {
        return detectHarris(gray, params);
    }
}
void CornerDetector::finalizeCornerResponse(const cv::Mat& gray,
                            const cv::Mat& response,
                            const processing::CornerParams& params,
                            processing::CornerResult& result) {
    float threshold = params.threshold;
    int nmsRadius = std::max(1, params.windowSize / 2);
    int minDistance = std::max(5, params.windowSize + 1);
    int maxCorners = 400;

    std::vector<Candidate> candidates;
    candidates.reserve(gray.rows * gray.cols / 20);

    for (int y = nmsRadius; y < gray.rows - nmsRadius; ++y) {
        for (int x = nmsRadius; x < gray.cols - nmsRadius; ++x) {
            float R = response.at<float>(y, x);
            if (R <= threshold) {
                continue;
            }

            bool isLocalMax = true;
            for (int j = -nmsRadius; j <= nmsRadius && isLocalMax; ++j) {
                for (int i = -nmsRadius; i <= nmsRadius; ++i) {
                    if (i == 0 && j == 0) {
                        continue;
                    }
                    if (response.at<float>(y + j, x + i) >= R) {
                        isLocalMax = false;
                        break;
                    }
                }
            }

            if (isLocalMax) {
                candidates.push_back({x, y, R});
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.r > b.r;
              });

    std::vector<cv::Point> acceptedCorners;
    acceptedCorners.reserve(std::min<int>(maxCorners, static_cast<int>(candidates.size())));

    for (const auto& c : candidates) {
        bool tooClose = false;

        for (const auto& pt : acceptedCorners) {
            int dx = pt.x - c.x;
            int dy = pt.y - c.y;
            if (dx * dx + dy * dy < minDistance * minDistance) {
                tooClose = true;
                break;
            }
        }

        if (tooClose) {
            continue;
        }

        acceptedCorners.emplace_back(c.x, c.y);
        result.featureCount++;

        if (result.featureCount >= maxCorners) {
            break;
        }
    }

    cv::Mat cornerImage;
    cv::cvtColor(gray, cornerImage, cv::COLOR_GRAY2BGR);

    for (const auto& pt : acceptedCorners) {
        cv::circle(cornerImage, pt, 4, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
        cv::circle(cornerImage, pt, 4, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }

    result.image = cornerImage;
}
matrix_elements CornerDetector::compute_matrix_elements(const cv::Mat& gray, const CornerParams& params) {
    matrix_elements elements;
    int windowSize = params.windowSize;
    double sigma = params.sigma;
    const int sobelKsize = 3;

    cv::Mat grayFloat;
    gray.convertTo(grayFloat, CV_32F, 1.0 / 255.0);

    cv::Mat blurred;
    cv::GaussianBlur(grayFloat, blurred, cv::Size(windowSize, windowSize), sigma);

    cv::Mat Gx, Gy;
    cv::Sobel(blurred, Gx, CV_32F, 1, 0, sobelKsize);
    cv::Sobel(blurred, Gy, CV_32F, 0, 1, sobelKsize);

    cv::Mat Gxx = Gx.mul(Gx);
    cv::Mat Gyy = Gy.mul(Gy);
    cv::Mat Gxy = Gx.mul(Gy);

    cv::Mat Sx, Sy, Sxy;
    cv::GaussianBlur(Gxx, Sx, cv::Size(windowSize, windowSize), sigma);
    cv::GaussianBlur(Gyy, Sy, cv::Size(windowSize, windowSize), sigma);
    cv::GaussianBlur(Gxy, Sxy, cv::Size(windowSize, windowSize), sigma);

    elements.Sx = Sx;
    elements.Sy = Sy;
    elements.Sxy = Sxy;

    return elements;
}

CornerResult CornerDetector::detectHarris(const cv::Mat& gray, const CornerParams& params) {
    using Clock = std::chrono::high_resolution_clock;
    auto start = Clock::now();

    CornerResult result;

    matrix_elements elements = compute_matrix_elements(gray, params);
    cv::Mat Sx = elements.Sx;
    cv::Mat Sy = elements.Sy;
    cv::Mat Sxy = elements.Sxy;
    cv::Mat response = cv::Mat::zeros(gray.size(), CV_32F);
    float maxResponse = 0.0f;

    for (int y = 0; y < gray.rows; ++y) {
        for (int x = 0; x < gray.cols; ++x) {
            float sx = Sx.at<float>(y, x);
            float sy = Sy.at<float>(y, x);
            float sxy = Sxy.at<float>(y, x);

            float det = sx * sy - sxy * sxy;
            float trace = sx + sy;
            float R = det - static_cast<float>(params.k) * trace * trace;

            response.at<float>(y, x) = R;
            if (R > maxResponse) {
                maxResponse = R;
            }
        }
    }

    finalizeCornerResponse(gray, response, params, result);

    auto end = Clock::now();
    result.computationTimeMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

CornerResult CornerDetector::detectShiTomasi(const cv::Mat& gray, const CornerParams& params) {
    auto start = std::chrono::high_resolution_clock::now();
    CornerResult result;

    matrix_elements elements = compute_matrix_elements(gray, params);
    cv::Mat Sx = elements.Sx;
    cv::Mat Sy = elements.Sy;
    cv::Mat Sxy = elements.Sxy;
    cv::Mat response = cv::Mat::zeros(gray.size(), CV_32F);
    float maxResponse = 0.0f;

    for (int y = 0; y < gray.rows; ++y) {
        for (int x = 0; x < gray.cols; ++x) {
            float sx = Sx.at<float>(y, x);
            float sy = Sy.at<float>(y, x);
            float sxy = Sxy.at<float>(y, x);

            float lambda1 = (sx + sy + std::sqrt((sx - sy) * (sx - sy) + 4.0f * sxy * sxy)) / 2.0f;
            float lambda2 = (sx + sy - std::sqrt((sx - sy) * (sx - sy) + 4.0f * sxy * sxy)) / 2.0f;
            float R = std::min(lambda1, lambda2);

            response.at<float>(y, x) = R;
            if (R > maxResponse) {
                maxResponse = R;
            }
        }
    }

    finalizeCornerResponse(gray, response, params, result);

    auto end = std::chrono::high_resolution_clock::now();
    result.computationTimeMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

} // namespace processing