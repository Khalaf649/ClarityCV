#include "server/Server.hpp"
#include "server/Router.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
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

    Router::registerRoutes(svr);

    std::cout << "[Server] Listening on "
              << config_.host << ":" << config_.port
              << " with " << config_.threads << " threads.\n";

    svr.listen(config_.host.c_str(), config_.port);
}

} // namespace server
