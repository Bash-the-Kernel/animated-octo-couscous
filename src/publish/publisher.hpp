#pragma once

#include "../common/types.hpp"
#include <vector>
#include <string>
#include <mutex>

namespace hft {

class Publisher {
public:
    Publisher() = default;
    
    void start(uint16_t port);
    void stop();
    
    void publish_trade(const Trade& trade);
    void publish_book_update(Symbol symbol, Side side, Price price, Quantity qty);
    
private:
    struct Client {
        int fd{-1};
        bool active{false};
    };
    
    std::vector<Client> clients_;
    std::mutex mutex_;
    bool running_{false};
    
    void broadcast(const std::string& message);
};

} // namespace hft
