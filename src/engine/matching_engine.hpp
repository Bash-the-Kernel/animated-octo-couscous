#pragma once

#include "order_book.hpp"
#include "event.hpp"
#include "../utils/ring_buffer.hpp"
#include <unordered_map>
#include <functional>
#include <atomic>

namespace hft {

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using AckCallback = std::function<void(const OrderAck&)>;
    using RejectCallback = std::function<void(const OrderReject&)>;
    
    MatchingEngine();
    ~MatchingEngine();
    
    void set_trade_callback(TradeCallback cb) { trade_cb_ = std::move(cb); }
    void set_ack_callback(AckCallback cb) { ack_cb_ = std::move(cb); }
    void set_reject_callback(RejectCallback cb) { reject_cb_ = std::move(cb); }
    
    void start();
    void stop();
    
    bool submit_event(const EngineEvent& event);
    
    [[nodiscard]] const OrderBook* get_book(Symbol symbol) const;
    [[nodiscard]] uint64_t processed_count() const noexcept { return processed_; }
    
private:
    void run();
    void process_event(const EngineEvent& event);
    void handle_new_order(const NewOrderEvent& event);
    void handle_cancel(const CancelOrderEvent& event);
    void handle_modify(const ModifyOrderEvent& event);
    
    OrderBook& get_or_create_book(Symbol symbol);
    
    std::unordered_map<Symbol, std::unique_ptr<OrderBook>> books_;
    RingBuffer<EngineEvent, 65536> event_queue_;
    
    TradeCallback trade_cb_;
    AckCallback ack_cb_;
    RejectCallback reject_cb_;
    
    std::atomic<bool> running_{false};
    std::thread worker_;
    uint64_t processed_{0};
    OrderId next_order_id_{1};
};

} // namespace hft
