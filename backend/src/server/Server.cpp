#include "server/Server.hpp"
#include "server/Router.hpp"
#include <httplib.h>
#include <iostream>

namespace server {

Server::Server(const ServerConfig& config)
    : config_(config) {}
void Server::run() {
    httplib::Server svr;
    svr.new_task_queue = [this] {
        return new httplib::ThreadPool(config_.threads);
    };

    // Route تجريبي للتأكد
    svr.Get("/ping", [](const httplib::Request&, httplib::Response& res){
        res.set_content(R"({"status":"ok","message":"pong"})", "application/json");
    });

    Router::registerRoutes(svr);

    std::cout << "[Server] ✅ Backend server started successfully!\n"
              << "Listening on " << config_.host << ":" << config_.port
              << " with " << config_.threads << " threads.\n";

    svr.listen(config_.host.c_str(), config_.port);
}


} // namespace server
