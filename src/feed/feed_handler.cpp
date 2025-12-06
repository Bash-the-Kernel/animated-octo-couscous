#include "feed_handler.hpp"
#include "../common/logging.hpp"
#include <chrono>

namespace hft {

FeedHandler::FeedHandler(MatchingEngine& engine) : engine_(engine) {}

FeedHandler::~FeedHandler() {
    stop();
}

void FeedHandler::start(const std::string& url) {
    if (running_.exchange(true)) return;
    worker_ = std::thread([this, url] { run(url); });
}

void FeedHandler::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void FeedHandler::run(const std::string& url) {
    LOG_INFO("Feed handler started: ", url);
    
    // Simplified: In production, would connect to WebSocket/TCP
    // For now, simulate with test messages
    
    while (running_.load(std::memory_order_relaxed)) {
        // Simulate receiving messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Example message
        std::string msg = R"({
            "type": "new_order",
            "order_id": 0,
            "symbol": "BTCUSD",
            "side": "buy",
            "order_type": "limit",
            "tif": "gtc",
            "price": 50000.0,
            "quantity": 100
        })";
        
        handle_message(msg);
    }
    
    LOG_INFO("Feed handler stopped, received: ", msg_count_, ", dropped: ", dropped_);
}

void FeedHandler::handle_message(std::string_view msg) {
    ++msg_count_;
    
    auto result = Parser::parse_json(msg);
    
    if (!result.success) {
        LOG_WARN("Parse error: ", result.error);
        ++dropped_;
        return;
    }
    
    if (!engine_.submit_event(result.event)) {
        ++dropped_;
    }
}

} // namespace hft
