#include "processing/ActiveContour.hpp"
#include <stdexcept>
#include "utils/ImageUtils.hpp"
#include "processing/EdgeDetector.hpp"
#include "processing/FilterProcessor.hpp"
#include "processing/HistogramProcessor.hpp"
namespace processing {

ContourResult ActiveContour::run_active_contour(const cv::Mat& input, const std::vector<cv::Point>& initialPoints, const ContourParams& params) {
    cv::Mat gray = utils::toGrayscale(input);
    switch (params.type) {
        case ContourType::GREEDY:
            return processGreedy(gray, initialPoints, params);
        default:
            throw std::invalid_argument("Unknown active contour type.");
    }
}

ContourResult ActiveContour::processGreedy(const cv::Mat& gray, const std::vector<cv::Point>& initialPoints, const ContourParams& params) {
    // 1. Pre-processing for a smooth energy field
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(7, 7), 0);

    // Create a Distance Transform map: 0 at edges, increasing as we move away
    cv::Mat edges, distTransform;
    cv::Canny(blurred, edges, 50, 150);
    cv::bitwise_not(edges, edges); // Invert so edges are 0
    cv::distanceTransform(edges, distTransform, cv::DIST_L2, 3);
    
    // Normalize distance transform to [0, 1] for stable energy calculation
    cv::normalize(distTransform, distTransform, 0, 1, cv::NORM_MINMAX);

    // 2. Initialize Snake
    int cols = gray.cols;
    int rows = gray.rows;
    std::vector<cv::Point2d> snake;
    if (!initialPoints.empty()) {
        for (auto p : initialPoints) snake.push_back(cv::Point2d(p.x, p.y));
    } else {
        // Default circle initialization
        int N = params.controlPoints > 3 ? params.controlPoints : 40;
        cv::Point2d center(cols / 2.0, rows / 2.0);
        double radius = std::min(cols, rows) * 0.4;
        for (int i = 0; i < N; ++i) {
            double theta = 2.0 * CV_PI * i / N;
            snake.push_back({center.x + radius * std::cos(theta), center.y + radius * std::sin(theta)});
        }
    }

    int N = static_cast<int>(snake.size());
    int W = 3; // 7x7 search window

    // 3. Iterative Optimization
    for (int iter = 0; iter < params.iterations; ++iter) {
        int pointsMoved = 0;

        // Calculate average distance for continuity term
        double avgDist = 0;
        for (int i = 0; i < N; ++i) {
            avgDist += cv::norm(snake[i] - snake[(i + 1) % N]);
        }
        avgDist /= N;

        for (int i = 0; i < N; ++i) {
            cv::Point2d prev = snake[(i - 1 + N) % N];
            cv::Point2d next = snake[(i + 1) % N];
            
            double minEnergy = std::numeric_limits<double>::max();
            cv::Point2d bestPos = snake[i];

            // Buffers to store neighborhood energies for normalization
            std::vector<double> e_cont, e_curv, e_img;
            std::vector<cv::Point2d> candidates;

            // First pass: Calculate raw energies in the neighborhood
            for (int dy = -W; dy <= W; ++dy) {
                for (int dx = -W; dx <= W; ++dx) {
                    cv::Point2d v(std::clamp(snake[i].x + dx, 0.0, (double)cols - 1),
                                  std::clamp(snake[i].y + dy, 0.0, (double)rows - 1));
                    
                    candidates.push_back(v);
                    
                    // Continuity: minimize deviation from average distance
                    e_cont.push_back(std::pow(cv::norm(v - prev) - avgDist, 2));
                    
                    // Curvature: minimize the second derivative (sharp angles)
                    cv::Point2d curvature = prev - 2.0 * v + next;
                    e_curv.push_back(curvature.x * curvature.x + curvature.y * curvature.y);
                    
                    // Image: value from the distance transform
                    e_img.push_back(distTransform.at<float>(static_cast<int>(v.y), static_cast<int>(v.x)));
                }
            }

            // Helper to normalize a vector to [0, 1]
            auto normalize = [](std::vector<double>& vec) {
                auto [min, max] = std::minmax_element(vec.begin(), vec.end());
                double range = *max - *min;
                for (double& val : vec) {
                    val = (range > 1e-6) ? (val - *min) / range : 0.0;
                }
            };

            normalize(e_cont);
            normalize(e_curv);
            normalize(e_img);

            // Second pass: Find candidate with minimum weighted energy
            for (size_t j = 0; j < candidates.size(); ++j) {
                double totalE = params.alpha * e_cont[j] + 
                                params.beta  * e_curv[j] + 
                                params.gamma * e_img[j];
                
                if (totalE < minEnergy) {
                    minEnergy = totalE;
                    bestPos = candidates[j];
                }
            }

            if (bestPos != snake[i]) {
                snake[i] = bestPos;
                pointsMoved++;
            }
        }

        // Convergence check
        if (pointsMoved < 2) break; 
    }

    // 4. Pack results
    ContourResult result;
    for (const auto& p : snake) result.points.push_back(cv::Point(p.x, p.y));

    cv::cvtColor(gray, result.contourImage, cv::COLOR_GRAY2BGR);
    for (int i = 0; i < N; ++i) {
        cv::line(result.contourImage, result.points[i], result.points[(i + 1) % N], cv::Scalar(0, 255, 0), 2);
        cv::circle(result.contourImage, result.points[i], 3, cv::Scalar(0, 0, 255), -1);
    }

    return result;
}

} // namespace processing