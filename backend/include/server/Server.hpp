#pragma once
#include <string>

namespace server {

struct ServerConfig {
    std::string host = "0.0.0.0";
    int         port = 8080;
    int         threads = 4;
};

class Server {
public:
    explicit Server(const ServerConfig& config);
    void run();

private:
    ServerConfig config_;
};

} // namespace server
