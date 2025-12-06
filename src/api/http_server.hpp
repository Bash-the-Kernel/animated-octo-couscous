#pragma once

#include "../engine/matching_engine.hpp"
#include <string>
#include <cstdint>

namespace hft {

class HttpServer {
public:
    explicit HttpServer(MatchingEngine& engine);
    
    void start(uint16_t port);
    void stop();
    
private:
    MatchingEngine& engine_;
    bool running_{false};
};

} // namespace hft
