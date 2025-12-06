#pragma once

#include "order.hpp"
#include <unordered_map>
#include <map>
#include <vector>
#include <memory_resource>

namespace hft {

class PriceLevel {
public:
    explicit PriceLevel(Price price) : price_(price) {}
    
    void add_order(OrderNode* node);
    void remove_order(OrderNode* node);
    
    [[nodiscard]] OrderNode* head() const noexcept { return head_; }
    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Quantity total_quantity() const noexcept { return total_qty_; }
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
    
private:
    Price price_;
    OrderNode* head_{nullptr};
    OrderNode* tail_{nullptr};
    Quantity total_qty_{0};
};

class OrderBook {
public:
    explicit OrderBook(Symbol symbol);
    ~OrderBook();
    
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    
    void add_order(const Order& order);
    bool cancel_order(OrderId order_id);
    bool modify_order(OrderId order_id, Price new_price, Quantity new_quantity);
    
    std::vector<Trade> match_order(Order& order);
    
    [[nodiscard]] const Order* find_order(OrderId order_id) const;
    [[nodiscard]] Price best_bid() const;
    [[nodiscard]] Price best_ask() const;
    [[nodiscard]] Symbol symbol() const noexcept { return symbol_; }
    
    void clear();
    
private:
    Symbol symbol_;
    
    // Price levels: price -> level
    std::map<Price, PriceLevel, std::greater<Price>> bids_;  // Descending
    std::map<Price, PriceLevel, std::less<Price>> asks_;     // Ascending
    
    // Order lookup
    std::unordered_map<OrderId, OrderNode*> orders_;
    
    // Memory pool for order nodes
    std::pmr::monotonic_buffer_resource pool_{1024 * 1024};
    std::pmr::polymorphic_allocator<OrderNode> allocator_{&pool_};
    
    OrderNode* allocate_node(const Order& order);
    void deallocate_node(OrderNode* node);
    
    std::vector<Trade> match_market_order(Order& order);
    std::vector<Trade> match_limit_order(Order& order);
    
    template<typename Map>
    std::vector<Trade> execute_matches(Order& aggressor, Map& passive_side);
};

} // namespace hft
