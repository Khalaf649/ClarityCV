#pragma once

// Forward declare httplib types to avoid circular includes
namespace httplib {
class Server;
class Request;
class Response;
} // namespace httplib

namespace server {

class Router {
public:
    static void registerRoutes(httplib::Server& svr);

private:
    // --- Endpoint handlers ---
    static void handleLoadImage(const httplib::Request& req, httplib::Response& res);
    static void handleAddNoise(const httplib::Request& req, httplib::Response& res);
    static void handleFilter(const httplib::Request& req, httplib::Response& res);
    static void handleEdgeDetect(const httplib::Request& req, httplib::Response& res);
    static void handleHistogram(const httplib::Request& req, httplib::Response& res);
    static void handleHistogramCurve(const httplib::Request& req, httplib::Response& res); // NEW
    static void handleEqualize(const httplib::Request& req, httplib::Response& res);
    static void handleNormalize(const httplib::Request& req, httplib::Response& res);
    static void handleThreshold(const httplib::Request& req, httplib::Response& res);
    static void handleFrequency(const httplib::Request& req, httplib::Response& res);
    static void handleHybrid(const httplib::Request& req, httplib::Response& res);
    static void handleActiveContour(const httplib::Request& req, httplib::Response& res);
    static void handleHoughTransform(const httplib::Request& req, httplib::Response& res);
    static void handleHealth(const httplib::Request& req, httplib::Response& res);
};

} // namespace server