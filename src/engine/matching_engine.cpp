#include "matching_engine.hpp"
#include "../common/logging.hpp"
#include "../utils/time.hpp"
#include <thread>
#include <cstring>

namespace hft {

MatchingEngine::MatchingEngine() = default;

MatchingEngine::~MatchingEngine() {
    stop();
}

void MatchingEngine::start() {
    running_.store(true);
}

void MatchingEngine::stop() {
    running_.store(false);
}

bool MatchingEngine::submit_event(const EngineEvent& event) {
    if (event_queue_.try_push(event)) {
        // Process immediately for testing (no separate thread)
        if (running_.load()) {
            auto ev = event_queue_.try_pop();
            if (ev) {
                process_event(*ev);
                ++processed_;
            }
        }
        return true;
    }
    return false;
}

void MatchingEngine::run() {
    LOG_INFO("Matching engine started");
    
    while (running_.load(std::memory_order_relaxed)) {
        auto event = event_queue_.try_pop();
        if (event) {
            process_event(*event);
            ++processed_;
        } else {
            std::this_thread::yield();
        }
    }
    
    // Drain remaining events
    while (auto event = event_queue_.try_pop()) {
        process_event(*event);
        ++processed_;
    }
    
    LOG_INFO("Matching engine stopped, processed: ", processed_);
}

void MatchingEngine::process_event(const EngineEvent& event) {
    std::visit([this](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, NewOrderEvent>) {
            handle_new_order(e);
        } else if constexpr (std::is_same_v<T, CancelOrderEvent>) {
            handle_cancel(e);
        } else if constexpr (std::is_same_v<T, ModifyOrderEvent>) {
            handle_modify(e);
        }
    }, event);
}

void MatchingEngine::handle_new_order(const NewOrderEvent& event) {
    Order order = event.order;
    
    if (order.id == INVALID_ORDER_ID) {
        order.id = next_order_id_++;
    }
    
    order.timestamp = now_ns();
    order.status = OrderStatus::PENDING;
    
    // Validate
    if (order.quantity == 0 || (order.type == OrderType::LIMIT && order.price == 0)) {
        order.status = OrderStatus::REJECTED;
        if (reject_cb_) {
            OrderReject reject{order.id, {}, order.timestamp};
            const char* msg = "Invalid order parameters";
            std::copy_n(msg, std::strlen(msg), reject.reason.data());
            reject_cb_(reject);
        }
        return;
    }
    
    OrderBook& book = get_or_create_book(order.symbol);
    
    // Match
    auto trades = book.match_order(order);
    
    // Emit trades
    for (const auto& trade : trades) {
        if (trade_cb_) trade_cb_(trade);
    }
    
    // Emit ack
    if (ack_cb_) {
        OrderAck ack{order.id, order.status, order.timestamp};
        ack_cb_(ack);
    }
}

void MatchingEngine::handle_cancel(const CancelOrderEvent& event) {
    auto it = books_.find(event.symbol);
    if (it == books_.end()) {
        if (reject_cb_) {
            OrderReject reject{event.order_id, {}, now_ns()};
            const char* msg = "Order not found";
            std::copy_n(msg, std::strlen(msg), reject.reason.data());
            reject_cb_(reject);
        }
        return;
    }
    
    bool success = it->second->cancel_order(event.order_id);
    
    if (ack_cb_) {
        OrderAck ack{event.order_id, success ? OrderStatus::CANCELLED : OrderStatus::REJECTED, now_ns()};
        ack_cb_(ack);
    }
}

void MatchingEngine::handle_modify(const ModifyOrderEvent& event) {
    auto it = books_.find(event.symbol);
    if (it == books_.end()) {
        if (reject_cb_) {
            OrderReject reject{event.order_id, {}, now_ns()};
            const char* msg = "Order not found";
            std::copy_n(msg, std::strlen(msg), reject.reason.data());
            reject_cb_(reject);
        }
        return;
    }
    
    bool success = it->second->modify_order(event.order_id, event.new_price, event.new_quantity);
    
    if (ack_cb_) {
        OrderAck ack{event.order_id, success ? OrderStatus::ACCEPTED : OrderStatus::REJECTED, now_ns()};
        ack_cb_(ack);
    }
}

OrderBook& MatchingEngine::get_or_create_book(Symbol symbol) {
    auto it = books_.find(symbol);
    if (it != books_.end()) {
        return *it->second;
    }
    
    auto [new_it, _] = books_.emplace(symbol, std::make_unique<OrderBook>(symbol));
    return *new_it->second;
}

const OrderBook* MatchingEngine::get_book(Symbol symbol) const {
    auto it = books_.find(symbol);
    return (it != books_.end()) ? it->second.get() : nullptr;
}

} // namespace hft
