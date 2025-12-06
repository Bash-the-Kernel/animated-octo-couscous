#include <gtest/gtest.h>
#include "engine/matching_engine.hpp"
#include <chrono>

using namespace hft;

TEST(MatchingEngineTest, ProcessNewOrder) {
    MatchingEngine engine;
    
    bool ack_received = false;
    engine.set_ack_callback([&](const OrderAck& ack) {
        ack_received = true;
        EXPECT_EQ(ack.status, OrderStatus::ACCEPTED);
    });
    
    engine.start();
    
    NewOrderEvent event;
    event.order.symbol = Symbol("BTCUSD");
    event.order.side = Side::BUY;
    event.order.type = OrderType::LIMIT;
    event.order.price = 50000 * PRICE_SCALE;
    event.order.quantity = 100;
    event.order.tif = TimeInForce::GTC;
    
    EXPECT_TRUE(engine.submit_event(event));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.stop();
    
    EXPECT_TRUE(ack_received);
    EXPECT_GT(engine.processed_count(), 0);
}

TEST(MatchingEngineTest, MatchOrders) {
    MatchingEngine engine;
    
    int trade_count = 0;
    engine.set_trade_callback([&](const Trade& trade) {
        ++trade_count;
        EXPECT_EQ(trade.quantity, 50);
    });
    
    engine.start();
    
    // Submit buy order
    NewOrderEvent buy_event;
    buy_event.order.symbol = Symbol("BTCUSD");
    buy_event.order.side = Side::BUY;
    buy_event.order.type = OrderType::LIMIT;
    buy_event.order.price = 50000 * PRICE_SCALE;
    buy_event.order.quantity = 100;
    buy_event.order.tif = TimeInForce::GTC;
    
    engine.submit_event(buy_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Submit sell order
    NewOrderEvent sell_event;
    sell_event.order.symbol = Symbol("BTCUSD");
    sell_event.order.side = Side::SELL;
    sell_event.order.type = OrderType::LIMIT;
    sell_event.order.price = 50000 * PRICE_SCALE;
    sell_event.order.quantity = 50;
    sell_event.order.tif = TimeInForce::GTC;
    
    engine.submit_event(sell_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.stop();
    
    EXPECT_EQ(trade_count, 1);
}

TEST(MatchingEngineTest, CancelOrder) {
    MatchingEngine engine;
    
    engine.start();
    
    // Submit order
    NewOrderEvent new_event;
    new_event.order.id = 123;
    new_event.order.symbol = Symbol("BTCUSD");
    new_event.order.side = Side::BUY;
    new_event.order.type = OrderType::LIMIT;
    new_event.order.price = 50000 * PRICE_SCALE;
    new_event.order.quantity = 100;
    new_event.order.tif = TimeInForce::GTC;
    
    engine.submit_event(new_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Cancel order
    CancelOrderEvent cancel_event;
    cancel_event.order_id = 123;
    cancel_event.symbol = Symbol("BTCUSD");
    
    engine.submit_event(cancel_event);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    engine.stop();
    
    EXPECT_GT(engine.processed_count(), 1);
}
