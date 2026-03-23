#include "server/Router.hpp"
#include "utils/ImageUtils.hpp"
#include "utils/Image.hpp"
#include "processing/NoiseProcessor.hpp"
#include "processing/FilterProcessor.hpp"
#include "processing/EdgeDetector.hpp"
#include "processing/HistogramProcessor.hpp"
#include "processing/ThresholdProcessor.hpp"
#include "processing/FrequencyProcessor.hpp"
#include "processing/HybridProcessor.hpp"
#include "processing/ActiveContour.hpp"
#include "processing/HoughProcessor.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace server {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

static void sendError(httplib::Response& res, int status, const std::string& message) {
    setCORSHeaders(res);
    json body = {{"success", false}, {"error", message}};
    res.set_content(body.dump(), "application/json");
    res.status = status;
}

static bool parseBody(const httplib::Request& req,
                      httplib::Response& res,
                      json& out) {
    try {
        out = json::parse(req.body);
        return true;
    } catch (const std::exception& e) {
        sendError(res, 400, std::string("Invalid JSON body: ") + e.what());
        return false;
    }
}

static cv::Mat requireImage(const json& body, const std::string& key = "image") {
    if (!body.contains(key) || !body[key].is_string()) {
        throw std::invalid_argument("Missing or invalid '" + key + "' field (expected base64 string).");
    }
    return utils::decodeImageFromBase64(body[key].get<std::string>());
}

static std::string imageToB64(const cv::Mat& img) {
    return utils::encodeImageToBase64(img, ".png");
}

// ---------------------------------------------------------------------------
// Route registration
// ---------------------------------------------------------------------------

void Router::registerRoutes(httplib::Server& svr) {
    // Preflight OPTIONS for CORS
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCORSHeaders(res);
        res.status = 204;
    });

    svr.Get("/api/health",              handleHealth);
    svr.Post("/api/load",               handleLoadImage);
    svr.Post("/api/noise",              handleAddNoise);
    svr.Post("/api/filter",             handleFilter);
    svr.Post("/api/edge",               handleEdgeDetect);
    svr.Post("/api/histogram",          handleHistogram);
    svr.Post("/api/histogram_curve",    handleHistogramCurve);   // NEW
    svr.Post("/api/equalize",           handleEqualize);
    svr.Post("/api/normalize",          handleNormalize);
    svr.Post("/api/threshold",          handleThreshold);
    svr.Post("/api/frequency",          handleFrequency);
    svr.Post("/api/hybrid",             handleHybrid);
    svr.Post("/api/active_contour",     handleActiveContour);
    svr.Post("/api/hough_transform",    handleHoughTransform);
}

// ---------------------------------------------------------------------------
// GET /api/health
// ---------------------------------------------------------------------------

void Router::handleHealth(const httplib::Request&, httplib::Response& res) {
    setCORSHeaders(res);
    json body = {{"success", true}, {"status", "ok"}};
    res.set_content(body.dump(), "application/json");
}

// ---------------------------------------------------------------------------
// POST /api/load
// Body: { "image": "<base64>", "mode": "rgb" | "gray" }
// ---------------------------------------------------------------------------

void Router::handleLoadImage(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));
        std::string mode = body.value("mode", "rgb");

        cv::Mat result;
        if (mode == "gray") {
            result = utils::toGrayscale(img.original);
        } else {
            result = utils::toRGB(img.original);
        }

        setCORSHeaders(res);
        json response = {
            {"success",  true},
            {"image",    imageToB64(result)},
            {"width",    result.cols},
            {"height",   result.rows},
            {"channels", result.channels()}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/noise
// Body: { "image": "<base64>", "type": "gaussian"|"uniform"|"salt_pepper",
//         "mean": 0, "stddev": 25, "low": -50, "high": 50,
//         "salt_prob": 0.02, "pepper_prob": 0.02 }
// ---------------------------------------------------------------------------

void Router::handleAddNoise(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::NoiseParams params;
        std::string typeStr = body.value("type", "gaussian");
        if      (typeStr == "uniform")     params.type = processing::NoiseType::UNIFORM;
        else if (typeStr == "salt_pepper") params.type = processing::NoiseType::SALT_AND_PEPPER;
        else                               params.type = processing::NoiseType::GAUSSIAN;

        params.mean       = body.value("mean",        0.0);
        params.stddev     = body.value("stddev",      25.0);
        params.low        = body.value("low",        -50.0);
        params.high       = body.value("high",        50.0);
        params.saltProb   = body.value("salt_prob",   0.02);
        params.pepperProb = body.value("pepper_prob", 0.02);

        cv::Mat result = processing::NoiseProcessor::addNoise(img.original, params);
        img.noisy = result;

        setCORSHeaders(res);
        json response = {{"success", true}, {"image", imageToB64(result)}};
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/filter
// Body: { "image": "<base64>", "type": "average"|"gaussian"|"median",
//         "kernel_size": 3, "sigma": 0 }
// ---------------------------------------------------------------------------

void Router::handleFilter(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::FilterParams params;
        std::string typeStr = body.value("type", "gaussian");
        if      (typeStr == "average") params.type = processing::FilterType::AVERAGE;
        else if (typeStr == "median")  params.type = processing::FilterType::MEDIAN;
        else                           params.type = processing::FilterType::GAUSSIAN;

        int ks = body.value("kernel_size", 3);
        if (ks % 2 == 0) ks += 1;   // OpenCV requires odd kernel size
        params.kernelSize = ks;
        params.sigmaX     = body.value("sigma", 0.0);

        cv::Mat result = processing::FilterProcessor::applyFilter(
            img.noisy.empty() ? img.original : img.noisy, params);

        setCORSHeaders(res);
        json response = {{"success", true}, {"image", imageToB64(result)}};
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/edge
// Body: { "image": "<base64>", "type": "sobel"|"roberts"|"prewitt"|"canny",
//         "direction": "x"|"y"|"combined",
//         "canny_low": 50, "canny_high": 150, "sobel_ksize": 3 }
// Returns: { "image_x", "image_y", "image_combined" }
// ---------------------------------------------------------------------------

void Router::handleEdgeDetect(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::EdgeParams params;
        std::string typeStr = body.value("type", "sobel");
        if      (typeStr == "roberts") params.type = processing::EdgeType::ROBERTS;
        else if (typeStr == "prewitt") params.type = processing::EdgeType::PREWITT;
        else if (typeStr == "canny")   params.type = processing::EdgeType::CANNY;
        else                           params.type = processing::EdgeType::SOBEL;

        params.cannyLow   = body.value("canny_low",   50.0);
        params.cannyHigh  = body.value("canny_high",  150.0);
        params.sobelKsize = body.value("sobel_ksize", 3);

        processing::EdgeResult result = processing::EdgeDetector::detect(img.original, params);

        setCORSHeaders(res);
        json response = {
            {"success",        true},
            {"image_x",        imageToB64(result.edgeX)},
            {"image_y",        imageToB64(result.edgeY)},
            {"image_combined", imageToB64(result.combined)}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/histogram
// Body: { "image": "<base64>" }
// Returns: { "channels": [...], "plot": "<base64>" }
// ---------------------------------------------------------------------------

void Router::handleHistogram(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));
        processing::HistogramResult result = processing::HistogramProcessor::compute(img.original);

        json channelsJson = json::array();
        for (const auto& ch : result.channels) {
            channelsJson.push_back({
                {"label", ch.label},
                {"bins",  ch.bins},
                {"cdf",   ch.cdf}
            });
        }

        setCORSHeaders(res);
        json response = {
            {"success",  true},
            {"channels", channelsJson},
            {"plot",     imageToB64(result.plotImage)}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/histogram_curve
// Body: { "image": "<base64>" }
// Returns: {
//   "channels": [ { "label", "bins", "cdf", "mean", "stddev" }, ... ],
//   "plot":       "<base64>",   <- plain histogram (same as /api/histogram)
//   "plot_curve": "<base64>"    <- histogram bars + Gaussian distribution curve
// }
// ---------------------------------------------------------------------------

void Router::handleHistogramCurve(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));
        processing::HistogramResult result = processing::HistogramProcessor::compute(img.original);

        json channelsJson = json::array();
        for (const auto& ch : result.channels) {
            channelsJson.push_back({
                {"label",  ch.label},
                {"bins",   ch.bins},
                {"cdf",    ch.cdf},
                {"mean",   ch.mean},
                {"stddev", ch.stddev}
            });
        }

        setCORSHeaders(res);
        json response = {
            {"success",    true},
            {"channels",   channelsJson},
            {"plot",       imageToB64(result.plotImage)},           // plain histogram
            {"plot_curve", imageToB64(result.plotImageWithCurve)}   // histogram + distribution curve
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/equalize
// Body: { "image": "<base64>" }
// ---------------------------------------------------------------------------

void Router::handleEqualize(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img = utils::Image(requireImage(body));
        cv::Mat result   = processing::HistogramProcessor::equalize(img.original);

        setCORSHeaders(res);
        json response = {{"success", true}, {"image", imageToB64(result)}};
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/normalize
// Body: { "image": "<base64>" }
// ---------------------------------------------------------------------------

void Router::handleNormalize(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img = utils::Image(requireImage(body));
        cv::Mat result   = processing::HistogramProcessor::normalize(img.original);

        setCORSHeaders(res);
        json response = {{"success", true}, {"image", imageToB64(result)}};
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/threshold
// Body: { "image": "<base64>", "type": "global_binary"|"global_otsu"|"local_mean"|"local_gaussian",
//         "threshold": 127, "block_size": 11, "c": 2 }
// ---------------------------------------------------------------------------

void Router::handleThreshold(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::ThresholdParams params;
        std::string typeStr = body.value("type", "global_otsu");
        if      (typeStr == "global_binary")  params.type = processing::ThresholdType::GLOBAL_BINARY;
        else if (typeStr == "local_mean")     params.type = processing::ThresholdType::LOCAL_MEAN;
        else if (typeStr == "local_gaussian") params.type = processing::ThresholdType::LOCAL_GAUSSIAN;
        else                                  params.type = processing::ThresholdType::GLOBAL_OTSU;

        params.threshold = body.value("threshold", 127.0);
        int bs = body.value("block_size", 11);
        if (bs % 2 == 0) bs += 1;   // OpenCV adaptive threshold requires odd block size
        params.blockSize = bs;
        params.C         = body.value("c", 2.0);

        processing::ThresholdResult result = processing::ThresholdProcessor::apply(img.original, params);

        setCORSHeaders(res);
        json response = {
            {"success",           true},
            {"image",             imageToB64(result.binary)},
            {"applied_threshold", result.appliedThreshold}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/frequency
// Body: { "image": "<base64>", "filter_type": "low_pass"|"high_pass", "cutoff": 30 }
// ---------------------------------------------------------------------------

void Router::handleFrequency(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::FrequencyParams params;
        std::string typeStr = body.value("filter_type", "low_pass");
        params.filterType = (typeStr == "high_pass")
            ? processing::FrequencyFilterType::HIGH_PASS
            : processing::FrequencyFilterType::LOW_PASS;
        params.cutoff = body.value("cutoff", 30.0);

        processing::FrequencyResult result = processing::FrequencyProcessor::apply(img.original, params);

        setCORSHeaders(res);
        json response = {
            {"success",  true},
            {"image",    imageToB64(result.filtered)},
            {"spectrum", imageToB64(result.magnitudeLog)}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/hybrid
// Body: { "image1": "<base64>", "image2": "<base64>",
//         "low_cutoff": 20, "high_cutoff": 20, "alpha": 0.5 }
// ---------------------------------------------------------------------------

void Router::handleHybrid(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img1(requireImage(body, "image1"));
        utils::Image img2(requireImage(body, "image2"));

        processing::HybridParams params;
        params.lowPassCutoff  = body.value("low_cutoff",  20.0);
        params.highPassCutoff = body.value("high_cutoff", 20.0);
        params.blendAlpha     = body.value("alpha",        0.5);

        processing::HybridResult result = processing::HybridProcessor::create(
            img1.original, img2.original, params);

        setCORSHeaders(res);
        json response = {
            {"success",        true},
            {"low_freq_image", imageToB64(result.lowFreqImage)},
            {"high_freq_image",imageToB64(result.highFreqImage)},
            {"hybrid_image",   imageToB64(result.hybridImage)}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/active_contour
// Body: { "image": "<base64>", "alpha": 1.0, "beta": 1.0, "gamma": 1.0, 
//         "iterations": 100, "control_points": 10, "initial_points": [{"x":10, "y":20}, ...] }
// ---------------------------------------------------------------------------

void Router::handleActiveContour(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::ContourParams params;
        params.alpha = body.value("alpha", 1.0);
        params.beta  = body.value("beta", 1.0);
        params.gamma = body.value("gamma", 1.0);
        params.iterations = body.value("iterations", 100);
        params.controlPoints = body.value("controlPoints", 50);

        std::vector<cv::Point> initialPoints;
        if (body.contains("initial_points") && body["initial_points"].is_array()) {
            for (const auto& pt : body["initial_points"]) {
                if (pt.contains("x") && pt.contains("y")) {
                    initialPoints.push_back(cv::Point(pt["x"].get<int>(), pt["y"].get<int>()));
                }
            }
        }

        processing::ContourResult result = processing::ActiveContour::run_active_contour(img.original, initialPoints, params);

        json pointsJson = json::array();
        for (const auto& pt : result.points) {
            pointsJson.push_back({{"x", pt.x}, {"y", pt.y}});
        }

        setCORSHeaders(res);
        json response = {
            {"success", true},
            {"image",   imageToB64(result.contourImage)},
            {"points",  pointsJson}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

// ---------------------------------------------------------------------------
// POST /api/hough_transform
// Body: { "image": "<base64>", "shape_type": "line"|"circle"|"ellipse", "votes_threshold": 100 }
// ---------------------------------------------------------------------------

void Router::handleHoughTransform(const httplib::Request& req, httplib::Response& res) {
    json body;
    if (!parseBody(req, res, body)) return;
    try {
        utils::Image img(requireImage(body));

        processing::HoughParams params;
        std::string shapeStr = body.value("shape_type", "line");
        if      (shapeStr == "circle")  params.shapeType = processing::HoughShapeType::CIRCLE;
        else if (shapeStr == "ellipse") params.shapeType = processing::HoughShapeType::ELLIPSE;
        else                            params.shapeType = processing::HoughShapeType::LINE;

        params.threshold = body.value("votes_threshold", 100);

        processing::HoughProcessor processor;
        processing::HoughResult result = processor.apply(img.original, params);

        setCORSHeaders(res);
        json response = {
            {"success", true},
            {"image",   imageToB64(result.transformImage)}
        };
        res.set_content(response.dump(), "application/json");
    } catch (const std::exception& e) {
        sendError(res, 422, e.what());
    }
}

} // namespace server