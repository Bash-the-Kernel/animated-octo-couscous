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
        
        if (order.side == Side::BUY) {
            for (auto it = sell_orders_.begin(); it != sell_orders_.end() && order.remaining() > 0;) {
                Price price = it->first;
                auto& order_ids = it->second;
                
                if (order.price < price) break;
                
                for (auto oid_it = order_ids.begin(); oid_it != order_ids.end() && order.remaining() > 0;) {
                    Order& passive = orders_[*oid_it];
                    Quantity match_qty = std::min(order.remaining(), passive.remaining());
                    
                    Trade trade{
                        .buyer_id = order.id,
                        .seller_id = passive.id,
                        .symbol = order.symbol,
                        .price = passive.price,
                        .quantity = match_qty,
                        .timestamp = 0,
                        .buyer_is_aggressor = true
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
                    it = sell_orders_.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            for (auto it = buy_orders_.begin(); it != buy_orders_.end() && order.remaining() > 0;) {
                Price price = it->first;
                auto& order_ids = it->second;
                
                if (order.price > price) break;
                
                for (auto oid_it = order_ids.begin(); oid_it != order_ids.end() && order.remaining() > 0;) {
                    Order& passive = orders_[*oid_it];
                    Quantity match_qty = std::min(order.remaining(), passive.remaining());
                    
                    Trade trade{
                        .buyer_id = passive.id,
                        .seller_id = order.id,
                        .symbol = order.symbol,
                        .price = passive.price,
                        .quantity = match_qty,
                        .timestamp = 0,
                        .buyer_is_aggressor = false
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
                    it = buy_orders_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        if (order.remaining() > 0 && order.tif == TimeInForce::GTC) {
            if (order.side == Side::BUY) {
                buy_orders_[order.price].push_back(order.id);
            } else {
                sell_orders_[order.price].push_back(order.id);
            }
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
