#include "publisher.hpp"
#include "../common/logging.hpp"
#include <nlohmann/json.hpp>

namespace hft {

void Publisher::start(uint16_t port) {
    running_ = true;
    LOG_INFO("Publisher started on port: ", port);
    // Simplified: In production, would start WebSocket server
}

void Publisher::stop() {
    running_ = false;
    LOG_INFO("Publisher stopped");
}

void Publisher::publish_trade(const Trade& trade) {
    nlohmann::json j;
    j["type"] = "trade";
    j["buyer_id"] = trade.buyer_id;
    j["seller_id"] = trade.seller_id;
    j["symbol"] = std::string(trade.symbol.view());
    j["price"] = static_cast<double>(trade.price) / PRICE_SCALE;
    j["quantity"] = trade.quantity;
    j["timestamp"] = trade.timestamp;
    
    broadcast(j.dump());
}

void Publisher::publish_book_update(Symbol symbol, Side side, Price price, Quantity qty) {
    nlohmann::json j;
    j["type"] = "book_update";
    j["symbol"] = std::string(symbol.view());
    j["side"] = (side == Side::BUY) ? "buy" : "sell";
    j["price"] = static_cast<double>(price) / PRICE_SCALE;
    j["quantity"] = qty;
    
    broadcast(j.dump());
}

void Publisher::broadcast(const std::string& message) {
    std::lock_guard lock(mutex_);
    // Simplified: In production, would send to all connected WebSocket clients
    LOG_DEBUG("Broadcast: ", message);
}

} // namespace hft
