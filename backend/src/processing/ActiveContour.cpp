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
    
    // 1- Apply Gaussian blur to reduce noise
    processing::FilterParams blurParams;
    blurParams.type       = processing::FilterType::GAUSSIAN;
    blurParams.kernelSize = 9;   // larger kernel = smoother gradient field, fewer local traps
    cv::Mat blurred = processing::FilterProcessor::applyFilter(gray, blurParams);

    // 2- Use Sobel operator to compute the gradient magnitude
    EdgeResult sobel = processing::EdgeDetector::detect(blurred, EdgeParams{EdgeType::SOBEL});
    cv::Mat edge = sobel.combined;

    // 3- Single correct normalization to [0,1] then invert
    //    so strong edges = low energy (snake is attracted to them)
    cv::Mat edgeNormalized;
    edge.convertTo(edgeNormalized, CV_64F, 1.0 / 255.0);  // ✅ single division, no double normalize
    edgeNormalized = 1.0 - edgeNormalized;                 // invert: edges become energy minima

    // 4- Initialize the snake
    int cols = gray.cols;
    int rows = gray.rows;
    std::vector<cv::Point2d> snake;

    if (!initialPoints.empty()) {
        snake.assign(initialPoints.begin(), initialPoints.end());
    } else {
        int N = params.controlPoints > 3 ? params.controlPoints : 50; 

        // Logical initialization: A circle centered in the image spanning 80% of the minor dimension
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
    int W = 3;   // ✅ wider search window: 7×7 neighbourhood gives snake more reach per iteration

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

    // 5- Iteratively move each snake point to minimise total energy
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
            double      bestE   = std::numeric_limits<double>::max();

            for (int dy = -W; dy <= W; ++dy) {
                for (int dx = -W; dx <= W; ++dx) {
                    cv::Point2d candidate = clamp({ snake[i].x + dx, snake[i].y + dy });

                    // — Continuity: penalises uneven spacing between points
                    double d    = cv::norm(candidate - snake[prev]);
                    double cont = (avgDist > 1e-6)
                                ? std::pow(d - avgDist, 2) / (avgDist * avgDist)
                                : 0.0;

                    // — Curvature: normalised by avgDist² so it stays comparable
                    //   to continuity regardless of how far apart points are  ✅
                    cv::Point2d curv2d = snake[prev] - 2.0 * candidate + snake[next];
                    double curv = (curv2d.x * curv2d.x + curv2d.y * curv2d.y)
                                / (avgDist * avgDist + 1e-6);

                    // — Image energy: low at strong edges, high in flat regions
                    double eimg = getEimage(
                        static_cast<int>(candidate.x),
                        static_cast<int>(candidate.y)
                    );

                    double totalE = params.alpha * cont
                                  + params.beta  * curv
                                  + params.gamma * eimg;

                    if (totalE < bestE) {
                        bestE   = totalE;
                        bestPos = candidate;
                    }
                }
            }

            if (bestPos != snake[i]) {
                snake[i] = bestPos;
                moved    = true;
            }
        }

        if (!moved) break;  // converged early
    }

    // 6- Pack results
    ContourResult result;
    result.points.reserve(N);
    for (const auto& p : snake)
        result.points.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));

    // Visualisation
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
