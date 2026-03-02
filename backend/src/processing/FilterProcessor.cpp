// ---------------------------------------------------------------------------
// Task 2 - Filter noisy image: Average, Gaussian, Median filters
// ---------------------------------------------------------------------------
#include "processing/FilterProcessor.hpp"
#include <stdexcept>

namespace processing {
cv::Mat FilterProcessor::applyFilter(const cv::Mat& input, const FilterParams& params) {
    switch (params.type) {
        case FilterType::AVERAGE:
            return applyAverage(input, params.kernelSize);
        case FilterType::GAUSSIAN:
            return applyGaussian(input, params.kernelSize, params.sigmaX);
        case FilterType::MEDIAN:
            return applyMedian(input, params.kernelSize);
        default:
            throw std::invalid_argument("Unknown filter type.");
    }
}


cv::Mat FilterProcessor::applyAverage(const cv::Mat& input, int kernelSize) {
    if (kernelSize <= 0) {
        throw std::invalid_argument("kernelSize must be positive");
    }
    // ensure odd kernel
    if (kernelSize % 2 == 0) {
        kernelSize += 1;
    }

    if (input.depth() != CV_8U) {
        throw std::invalid_argument("applyAverage: only 8-bit images (CV_8U) are supported");
    }

    const int rows = input.rows;
    const int cols = input.cols;
    const int channels = input.channels();
    const int k = kernelSize / 2;
    const int area = kernelSize * kernelSize; // for zero-padding we always divide by full kernel area

    cv::Mat result(input.size(), input.type());

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (channels == 1) {
                int sum = 0;
                for (int j = -k; j <= k; ++j) {
                    for (int i = -k; i <= k; ++i) {
                        int ny = y + j;
                        int nx = x + i;
                        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols) {
                            sum += input.at<uchar>(ny, nx);
                        } else {
                            // zero padding
                        }
                    }
                }
                result.at<uchar>(y, x) = static_cast<uchar>(sum / area);
            } else if (channels == 3) {
                int sum0 = 0, sum1 = 0, sum2 = 0;
                for (int j = -k; j <= k; ++j) {
                    for (int i = -k; i <= k; ++i) {
                        int ny = y + j;
                        int nx = x + i;
                        if (ny >= 0 && ny < rows && nx >= 0 && nx < cols) {
                            cv::Vec3b v = input.at<cv::Vec3b>(ny, nx);
                            sum0 += v[0];
                            sum1 += v[1];
                            sum2 += v[2];
                        } else {
                            // zero padding
                        }
                    }
                }
                result.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    static_cast<uchar>(sum0 / area),
                    static_cast<uchar>(sum1 / area),
                    static_cast<uchar>(sum2 / area)
                );
            } else {
                throw std::invalid_argument("applyAverage: only 1- or 3-channel images are supported");
            }
        }
    }

    return result;

}

cv::Mat FilterProcessor::applyGaussian(const cv::Mat& input, int kernelSize, double sigma) {
    if (sigma <= 0.0) {
        sigma = 0.3 * ((kernelSize - 1) * 0.5 - 1) + 0.8; // OpenCV default sigma calculation
    }
    cv::Mat result(input.size(), input.type());
    int k = kernelSize / 2;
    for (int y= 0; y < input.rows; ++y){
        for (int x = 0; x < input.cols; ++x){
            if (input.channels() == 1) {
                double sum = 0.0;
                double weightSum = 0.0;
                for (int j = -k; j <= k; ++j) {
                    for (int i = -k; i <= k; ++i) {
                        int ny = y + j;
                        int nx = x + i;
                        if (ny >= 0 && ny < input.rows && nx >= 0 && nx < input.cols) {
                            double weight = std::exp(-(i*i + j*j) / (2 * sigma * sigma));
                            sum += input.at<uchar>(ny, nx) * weight;
                            weightSum += weight;
                        }else {
                            // zero padding
                        }
                    }
                }
                result.at<uchar>(y, x) = static_cast<uchar>(sum / weightSum);
            } else if (input.channels() == 3) {
                double sum0 = 0.0, sum1 = 0.0, sum2 = 0.0;
                double weightSum = 0.0;
                for (int j = -k; j <= k; ++j) {
                    for (int i = -k; i <= k; ++i) {
                        int ny = y + j;
                        int nx = x + i;
                        if (ny >= 0 && ny < input.rows && nx >= 0 && nx < input.cols) {
                            double weight = std::exp(-(i*i + j*j) / (2 * sigma * sigma));
                            cv::Vec3b v = input.at<cv::Vec3b>(ny, nx);
                            sum0 += v[0] * weight;
                            sum1 += v[1] * weight;
                            sum2 += v[2] * weight;
                            weightSum += weight;
                        } else {
                            // zero padding
                        }
                    }
                }
                result.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    static_cast<uchar>(sum0 / weightSum),
                    static_cast<uchar>(sum1 / weightSum),
                    static_cast<uchar>(sum2 / weightSum)
                );
            } else {
                throw std::invalid_argument("applyGaussian: only 1- or 3-channel images are supported");
            }
        }
    }
    return result;

}

cv::Mat FilterProcessor::applyMedian(const cv::Mat& input, int kernelSize) {
cv::Mat result(input.size(), input.type());
int k = kernelSize / 2;
for (int y = 0; y < input.rows; ++y) {
    for (int x = 0; x < input.cols; ++x) {
        if (input.channels() == 1) {
            std::vector<uchar> values;
            for (int j = -k; j <= k; ++j) {
                for (int i = -k; i <= k; ++i) {
                    int ny = y + j;
                    int nx = x + i;
                    if (ny >= 0 && ny < input.rows && nx >= 0 && nx < input.cols) {
                        values.push_back(input.at<uchar>(ny, nx));
                    } else {
                        values.push_back(0); // zero padding
                    }

                }
            
            }
            std::sort(values.begin(), values.end());
            int medianIndex = values.size() / 2;
            result.at<uchar>(y, x) = values[medianIndex];
        } 
        else if (input.channels() == 3) {
            std::vector<uchar> values0, values1, values2;
            for (int j = -k; j <= k; ++j) {
                for (int i = -k; i <= k; ++i) {
                    int ny = y + j;
                    int nx = x + i;
                    if (ny >= 0 && ny < input.rows && nx >= 0 && nx < input.cols) {
                        cv::Vec3b v = input.at<cv::Vec3b>(ny, nx);
                        values0.push_back(v[0]);
                        values1.push_back(v[1]);
                        values2.push_back(v[2]);
                    } else {
                        values0.push_back(0); // zero padding
                        values1.push_back(0);
                        values2.push_back(0);
                    }
                }
            }
            std::sort(values0.begin(), values0.end());
            std::sort(values1.begin(), values1.end());
            std::sort(values2.begin(), values2.end());
            int medianIndex = values0.size() / 2;
            result.at<cv::Vec3b>(y, x) = cv::Vec3b(
                values0[medianIndex],
                values1[medianIndex],
                values2[medianIndex]
            );
        } else {
            throw std::invalid_argument("applyMedian: only 1- or 3-channel images are supported");
        }
    }
}

    return result;

}

} // namespace processing
