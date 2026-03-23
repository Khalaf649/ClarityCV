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
    // 1- Apply Gaussian blur to reduce noise (unchanged - already excellent)
    processing::FilterParams blurParams;
    blurParams.type = processing::FilterType::GAUSSIAN;
    blurParams.kernelSize = 15;
    blurParams.sigmaX = 3;
    cv::Mat blurred = processing::FilterProcessor::applyFilter(gray, blurParams);

    // 2- Compute continuous gradient magnitude with Sobel (REPLACED Canny)
    //    This is the key improvement: Canny produced thin binary edges → snake could jump over gaps
    //    or get trapped on weak/internal edges (eyes/mouth). Sobel gives a smooth, wider attraction
    //    field that pulls the snake reliably from farther away and hugs concavities better.
    cv::Mat gradX, gradY, gradMag;
    cv::Sobel(blurred, gradX, CV_64F, 1, 0, 3);
    cv::Sobel(blurred, gradY, CV_64F, 0, 1, 3);
    cv::magnitude(gradX, gradY, gradMag);

    // Normalize gradient magnitude to [0,1] then invert → strong edges = energy minima (0)
    cv::normalize(gradMag, gradMag, 0.0, 1.0, cv::NORM_MINMAX, CV_64F);
    cv::Mat edgeNormalized = 1.0 - gradMag;   // renamed for clarity, same variable used below

    // 3- Initialize the snake (unchanged)
    int cols = gray.cols;
    int rows = gray.rows;
    std::vector<cv::Point2d> snake;
    if (!initialPoints.empty()) {
        snake.assign(initialPoints.begin(), initialPoints.end());
    } else {
        int N = params.controlPoints > 3 ? params.controlPoints : 50;
        cv::Point2d center(cols / 2.0, rows / 2.0);
        double radius = std::min(cols, rows) * 0.4;
        for (int i = 0; i < N; ++i) {
            double theta = 2.0 * CV_PI * i / N;
            snake.push_back(cv::Point2d(
                center.x + radius * std::cos(theta),
                center.y + radius * std::sin(theta)
            ));
        }
    }
    int N = static_cast<int>(snake.size());
    int W = 11;

    auto clamp = [&](cv::Point2d p) {
        p.x = std::max(0.0, std::min(p.x, static_cast<double>(cols - 1)));
        p.y = std::max(0.0, std::min(p.y, static_cast<double>(rows - 1)));
        return p;
    };

    auto getEimage = [&](int x, int y) -> double {
        x = std::clamp(x, 0, cols - 1);
        y = std::clamp(y, 0, rows - 1);
        return edgeNormalized.at<double>(y, x);
    };

    // 4- Iteratively move each snake point (core logic unchanged, only energy field improved)
    for (int iter = 0; iter < params.iterations; ++iter) {
        bool moved = false;

        // Compute average spacing for continuity normalisation
        double avgDist = 0.0;
        for (int i = 0; i < N; ++i) {
            int prev = (i - 1 + N) % N;
            avgDist += cv::norm(snake[i] - snake[prev]);
        }
        avgDist /= N;

        for (int i = 0; i < N; ++i) {
            int prev = (i - 1 + N) % N;
            int next = (i + 1) % N;
            cv::Point2d bestPos = snake[i];
            double bestE = std::numeric_limits<double>::max();

            for (int dy = -W; dy <= W; ++dy) {
                for (int dx = -W; dx <= W; ++dx) {
                    cv::Point2d candidate = clamp({ snake[i].x + dx, snake[i].y + dy });

                    // — Continuity: penalises uneven spacing
                    double d = cv::norm(candidate - snake[prev]);
                    double cont = (avgDist > 1e-6)
                        ? std::pow(d - avgDist, 2) / (avgDist * avgDist)
                        : 0.0;

                    // — Curvature: normalised for scale-invariance
                    cv::Point2d curv2d = snake[prev] - 2.0 * candidate + snake[next];
                    double curv = (curv2d.x * curv2d.x + curv2d.y * curv2d.y)
                        / (avgDist * avgDist + 1e-6);

                    // — Image energy: now uses smooth gradient field (much stronger pull)
                    double eimg = getEimage(
                        static_cast<int>(candidate.x),
                        static_cast<int>(candidate.y)
                    );

                    double totalE = params.alpha * cont
                                  + params.beta * curv
                                  + params.gamma * eimg;

                    if (totalE < bestE) {
                        bestE = totalE;
                        bestPos = candidate;
                    }
                }
            }

            if (bestPos != snake[i]) {
                snake[i] = bestPos;
                moved = true;
            }
        }
        if (!moved) break; // converged early
    }

    // 5- Pack results (unchanged)
    ContourResult result;
    result.points.reserve(N);
    for (const auto& p : snake)
        result.points.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));

    // Visualisation (unchanged)
    cv::Mat viz;
    cv::cvtColor(gray, viz, cv::COLOR_GRAY2BGR);
    for (int i = 0; i < N; ++i) {
        cv::circle(viz, result.points[i], 3, cv::Scalar(0, 0, 255), -1);
        cv::line(viz, result.points[i], result.points[(i + 1) % N],
                 cv::Scalar(0, 255, 0), 1);
    }
    result.contourImage = viz;
    return result;
}

} // namespace processing