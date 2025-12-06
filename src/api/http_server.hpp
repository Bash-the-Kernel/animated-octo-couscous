#pragma once

#include "../engine/matching_engine.hpp"
#include <string>
#include <cstdint>
#include <memory>
#include <thread>

namespace httplib {
    class Server;
}

namespace hft {

class HttpServer {
public:
    explicit HttpServer(MatchingEngine& engine);
    ~HttpServer();
    
    void start(uint16_t port);
    void stop();
    
private:
    void run(uint16_t port);
    
    MatchingEngine& engine_;
    std::unique_ptr<httplib::Server> server_;
    std::thread thread_;
    bool running_{false};
};

} // namespace hft
