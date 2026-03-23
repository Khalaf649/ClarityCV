#include "processing/ActiveContour.hpp"
#include <stdexcept>
#include <limits>
#include <cmath>
#include "utils/ImageUtils.hpp"
#include "processing/EdgeDetector.hpp"
#include "processing/FilterProcessor.hpp"
#include "processing/HistogramProcessor.hpp"

namespace processing {

namespace {

void appendLinePoints(
    std::vector<cv::Point>& points,
    const cv::Point& start,
    const cv::Point& end,
    bool includeStart
) {
    int x1 = start.x;
    int y1 = start.y;
    const int x2 = end.x;
    const int y2 = end.y;

    const int dx = std::abs(x2 - x1);
    const int dy = std::abs(y2 - y1);
    const int sx = (x1 < x2) ? 1 : -1;
    const int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    bool firstPoint = true;

    while (true) {
        if ((includeStart || !firstPoint)
            && (points.empty() || points.back() != cv::Point(x1, y1))) {
            points.emplace_back(x1, y1);
        }

        if (x1 == x2 && y1 == y2) {
            break;
        }

        const int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }

        firstPoint = false;
    }
}

std::vector<cv::Point> densifyInitialContour(const std::vector<cv::Point>& initialPoints) {
    if (initialPoints.size() < 3) {
        return initialPoints;
    }

    std::vector<cv::Point> densePoints;
    for (size_t i = 0; i < initialPoints.size(); ++i) {
        appendLinePoints(
            densePoints,
            initialPoints[i],
            initialPoints[(i + 1) % initialPoints.size()],
            i == 0
        );
    }

    if (densePoints.size() > 1 && densePoints.front() == densePoints.back()) {
        densePoints.pop_back();
    }

    return densePoints;
}

} // namespace

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
    // 1- Lighter but still effective blur
    processing::FilterParams blurParams;
    blurParams.type = processing::FilterType::GAUSSIAN;
    blurParams.kernelSize = 21;
    blurParams.sigmaX = 5.0;
    cv::Mat blurred = processing::FilterProcessor::applyFilter(gray, blurParams);

    // 2- Sobel gradient magnitude
    cv::Mat gradX, gradY, gradMag;
    cv::Sobel(blurred, gradX, CV_64F, 1, 0, 3);
    cv::Sobel(blurred, gradY, CV_64F, 0, 1, 3);
    cv::magnitude(gradX, gradY, gradMag);

    cv::normalize(gradMag, gradMag, 0.0, 1.0, cv::NORM_MINMAX, CV_64F);
    cv::Mat edgeNormalized = 1.0 - gradMag;

    // 3- Initialize snake
    int cols = gray.cols;
    int rows = gray.rows;
    std::vector<cv::Point2d> snake;
    if (!initialPoints.empty()) {
        std::vector<cv::Point> sampledInitialPoints = densifyInitialContour(initialPoints);
        snake.reserve(sampledInitialPoints.size());
        for (const auto& point : sampledInitialPoints) {
            snake.emplace_back(point.x, point.y);
        }
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
    int W = 7; 

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

    // Pre-allocate cache vectors for the greedy loop to avoid re-allocation
    const int windowDim = 2 * W + 1;
    const int cacheSize = windowDim * windowDim;
    std::vector<double> cacheCont(cacheSize);
    std::vector<double> cacheCurv(cacheSize);
    std::vector<double> cacheEimg(cacheSize);
    std::vector<cv::Point2d> cachePos(cacheSize);

    // 4- Greedy iteration (Optimized Single-Pass Caching)
    for (int iter = 0; iter < params.iterations; ++iter) {
        bool moved = false;

        double avgDist = 0.0;
        for (int i = 0; i < N; ++i) {
            int prev = (i - 1 + N) % N;
            avgDist += cv::norm(snake[i] - snake[prev]);
        }
        avgDist /= N;

        for (int i = 0; i < N; ++i) {
            int prev = (i - 1 + N) % N;
            int next = (i + 1) % N;

            double minCont = std::numeric_limits<double>::max(), maxCont = std::numeric_limits<double>::lowest();
            double minCurv = std::numeric_limits<double>::max(), maxCurv = std::numeric_limits<double>::lowest();
            double minEimg = std::numeric_limits<double>::max(), maxEimg = std::numeric_limits<double>::lowest();

            // First pass: Calculate, Cache, and Track Min/Max
            int cacheIdx = 0;
            for (int dy = -W; dy <= W; ++dy) {
                for (int dx = -W; dx <= W; ++dx) {
                    cv::Point2d candidate = clamp({ snake[i].x + dx, snake[i].y + dy });

                    double d = cv::norm(candidate - snake[prev]);
                    double cont = (avgDist > 1e-6) ? std::pow(d - avgDist, 2) / (avgDist * avgDist) : 0.0;

                    cv::Point2d curv2d = snake[prev] - 2.0 * candidate + snake[next];
                    double curv = (curv2d.x * curv2d.x + curv2d.y * curv2d.y) / (avgDist * avgDist + 1e-6);

                    double eimg = getEimage(static_cast<int>(candidate.x), static_cast<int>(candidate.y));

                    cacheCont[cacheIdx] = cont;
                    cacheCurv[cacheIdx] = curv;
                    cacheEimg[cacheIdx] = eimg;
                    cachePos[cacheIdx] = candidate;
                    cacheIdx++;

                    minCont = std::min(minCont, cont); maxCont = std::max(maxCont, cont);
                    minCurv = std::min(minCurv, curv); maxCurv = std::max(maxCurv, curv);
                    minEimg = std::min(minEimg, eimg); maxEimg = std::max(maxEimg, eimg);
                }
            }

            // Second pass: Read from Cache, Normalize, and Pick Best
            cv::Point2d bestPos = snake[i];
            double bestE = std::numeric_limits<double>::max();

            double rangeCont = (maxCont - minCont > 1e-12) ? (maxCont - minCont) : 1.0;
            double rangeCurv = (maxCurv - minCurv > 1e-12) ? (maxCurv - minCurv) : 1.0;
            double rangeEimg = (maxEimg - minEimg > 1e-12) ? (maxEimg - minEimg) : 1.0;

            for (int k = 0; k < cacheSize; ++k) {
                double contN = (cacheCont[k] - minCont) / rangeCont;
                double curvN = (cacheCurv[k] - minCurv) / rangeCurv;
                double eimgN = (cacheEimg[k] - minEimg) / rangeEimg;

                double totalE = params.alpha * contN
                              + params.beta  * curvN
                              + params.gamma * eimgN;

                if (totalE < bestE) {
                    bestE = totalE;
                    bestPos = cachePos[k];
                }
            }

            if (bestPos != snake[i]) {
                snake[i] = bestPos;
                moved = true;
            }
        }
        if (!moved) break;
    }

    // 5- Pack results and enforce Convex Polygon
    std::vector<cv::Point> rawPoints;
    rawPoints.reserve(N);
    for (const auto& p : snake) {
        rawPoints.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));
    }

    std::vector<cv::Point> convexPoints;
    cv::convexHull(rawPoints, convexPoints, true); // 'true' forces clockwise orientation

    ContourResult result;
    result.points = convexPoints;
    int convexN = static_cast<int>(result.points.size());

    // Calculate perimeter (on the convex shape)
    double perimeter = 0.0;
    for (int i = 0; i < convexN; ++i) {
        int next = (i + 1) % convexN;
        perimeter += cv::norm(result.points[i] - result.points[next]);
    }
    result.perimeter = perimeter;

    // Calculate area using the shoelace formula (on the convex shape)
    double area = 0.0;
    for (int i = 0; i < convexN; ++i) {
        int next = (i + 1) % convexN;
        area += result.points[i].x * result.points[next].y;
        area -= result.points[next].x * result.points[i].y;
    }
    result.area = std::abs(area) / 2.0;

    // Calculate chain code using Bresenham's (on the convex shape)
    result.chainCode = computeChainCode8(result.points);

    // Visualisation
    cv::Mat viz;
    cv::cvtColor(gray, viz, cv::COLOR_GRAY2BGR);
    
    // Draw the raw snake in faint blue to see what the active contour actually found
    // for (int i = 0; i < N; ++i) {
    //     cv::line(viz, rawPoints[i], rawPoints[(i + 1) % N], cv::Scalar(255, 0, 0), 1);
    // }

    // Draw the final convex polygon on top in green
    for (int i = 0; i < convexN; ++i) {
        cv::circle(viz, result.points[i], 3, cv::Scalar(0, 0, 255), -1);
        cv::line(viz, result.points[i], result.points[(i + 1) % convexN], cv::Scalar(0, 255, 0), 2);
    }
    result.contourImage = viz;
    
    return result;
}

std::string ActiveContour::computeChainCode8(const std::vector<cv::Point>& points) {
    if (points.empty()) return "";
    
    std::string code;
    int N = static_cast<int>(points.size());
    
    for (int i = 0; i < N; ++i) {
        cv::Point p1 = points[i];
        cv::Point p2 = points[(i + 1) % N];
        
        // Bresenham's Line Algorithm to interpolate pixel steps
        int x1 = p1.x, y1 = p1.y;
        int x2 = p2.x, y2 = p2.y;
        
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;
        
        while (x1 != x2 || y1 != y2) {
            int current_x = x1;
            int current_y = y1;
            
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x1 += sx; }
            if (e2 < dx)  { err += dx; y1 += sy; }
            
            // Calculate 8-connected direction for the 1-pixel step
            int step_dx = x1 - current_x;
            int step_dy = y1 - current_y;
            
            int direction = 0;
            if (step_dx > 0 && step_dy == 0)      direction = 0; // East
            else if (step_dx > 0 && step_dy < 0)  direction = 1; // NE
            else if (step_dx == 0 && step_dy < 0) direction = 2; // North
            else if (step_dx < 0 && step_dy < 0)  direction = 3; // NW
            else if (step_dx < 0 && step_dy == 0) direction = 4; // West
            else if (step_dx < 0 && step_dy > 0)  direction = 5; // SW
            else if (step_dx == 0 && step_dy > 0) direction = 6; // South
            else if (step_dx > 0 && step_dy > 0)  direction = 7; // SE
            
            code += std::to_string(direction);
        }
    }
    return code;
}

} // namespace processing
