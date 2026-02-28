#include "server/Server.hpp"
#include <iostream>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    server::ServerConfig config;

    const char* host    = std::getenv("SERVER_HOST");
    const char* port    = std::getenv("SERVER_PORT");
    const char* threads = std::getenv("SERVER_THREADS");

    if (host)    config.host    = host;
    if (port)    config.port    = std::stoi(port);
    if (threads) config.threads = std::stoi(threads);

    // Allow CLI overrides: ./cv-backend [port]
    if (argc > 1) config.port = std::stoi(argv[1]);

    std::cout << "[Main] Starting CV Backend...\n";

    try {
        server::Server svr(config);
        svr.run();
    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
