#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include <map>
#include <random>

using namespace hft;

// Naive reference implementation for correctness comparison
class ReferenceEngine {
public:
    void add_order(const Order& order) {
        orders_[order.id] = order;
    }
    
    std::vector<Trade> match_order(Order& order) {
        std::vector<Trade> trades;
        
        auto& passive_orders = (order.side == Side::BUY) ? sell_orders_ : buy_orders_;
        
        for (auto it = passive_orders.begin(); it != passive_orders.end() && order.remaining() > 0;) {
            auto& [price, order_ids] = *it;
            
            bool crosses = (order.side == Side::BUY) ? (order.price >= price) : (order.price <= price);
            if (!crosses) break;
            
            for (auto oid_it = order_ids.begin(); oid_it != order_ids.end() && order.remaining() > 0;) {
                Order& passive = orders_[*oid_it];
                Quantity match_qty = std::min(order.remaining(), passive.remaining());
                
                Trade trade{
                    .buyer_id = (order.side == Side::BUY) ? order.id : passive.id,
                    .seller_id = (order.side == Side::SELL) ? order.id : passive.id,
                    .symbol = order.symbol,
                    .price = passive.price,
                    .quantity = match_qty,
                    .timestamp = 0,
                    .buyer_is_aggressor = (order.side == Side::BUY)
                };
                trades.push_back(trade);
                
                order.filled += match_qty;
                passive.filled += match_qty;
                
                if (passive.remaining() == 0) {
                    oid_it = order_ids.erase(oid_it);
                } else {
                    ++oid_it;
                }
            }
            
            if (order_ids.empty()) {
                it = passive_orders.erase(it);
            } else {
                ++it;
            }
        }
        
        if (order.remaining() > 0 && order.tif == TimeInForce::GTC) {
            auto& active_orders = (order.side == Side::BUY) ? buy_orders_ : sell_orders_;
            active_orders[order.price].push_back(order.id);
            orders_[order.id] = order;
        }
        
        return trades;
    }
    
private:
    std::map<OrderId, Order> orders_;
    std::map<Price, std::vector<OrderId>, std::greater<Price>> buy_orders_;
    std::map<Price, std::vector<OrderId>, std::less<Price>> sell_orders_;
};

TEST(PropertyTest, RandomOrdersMatchReference) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<Price> price_dist(49000 * PRICE_SCALE, 51000 * PRICE_SCALE);
    std::uniform_int_distribution<Quantity> qty_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    
    ReferenceEngine ref_engine;
    OrderBook fast_book(Symbol("BTCUSD"));
    
    for (int i = 0; i < 100; ++i) {
        Order order;
        order.id = i + 1;
        order.symbol = Symbol("BTCUSD");
        order.side = side_dist(rng) ? Side::BUY : Side::SELL;
        order.type = OrderType::LIMIT;
        order.price = price_dist(rng);
        order.quantity = qty_dist(rng);
        order.tif = TimeInForce::GTC;
        
        Order ref_order = order;
        Order fast_order = order;
        
        auto ref_trades = ref_engine.match_order(ref_order);
        auto fast_trades = fast_book.match_order(fast_order);
        
        EXPECT_EQ(ref_trades.size(), fast_trades.size()) << "Trade count mismatch at order " << i;
        
        for (size_t j = 0; j < std::min(ref_trades.size(), fast_trades.size()); ++j) {
            EXPECT_EQ(ref_trades[j].quantity, fast_trades[j].quantity) << "Trade quantity mismatch";
            EXPECT_EQ(ref_trades[j].price, fast_trades[j].price) << "Trade price mismatch";
        }
    }
}

TEST(PropertyTest, OrderInvariants) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order order;
    order.id = 1;
    order.symbol = Symbol("BTCUSD");
    order.side = Side::BUY;
    order.type = OrderType::LIMIT;
    order.price = 50000 * PRICE_SCALE;
    order.quantity = 100;
    order.tif = TimeInForce::GTC;
    
    auto trades = book.match_order(order);
    
    // Invariant: filled + remaining = quantity
    EXPECT_EQ(order.filled + order.remaining(), order.quantity);
    
    // Invariant: filled <= quantity
    EXPECT_LE(order.filled, order.quantity);
}
