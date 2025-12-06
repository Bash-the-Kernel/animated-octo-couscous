#include <gtest/gtest.h>
#include "engine/order_book.hpp"

using namespace hft;

TEST(OrderBookTest, AddBuyOrder) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order order;
    order.id = 1;
    order.symbol = Symbol("BTCUSD");
    order.side = Side::BUY;
    order.type = OrderType::LIMIT;
    order.price = 50000 * PRICE_SCALE;
    order.quantity = 100;
    
    book.add_order(order);
    
    EXPECT_EQ(book.best_bid(), 50000 * PRICE_SCALE);
    EXPECT_EQ(book.best_ask(), 0);
}

TEST(OrderBookTest, MatchLimitOrders) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order buy;
    buy.id = 1;
    buy.symbol = Symbol("BTCUSD");
    buy.side = Side::BUY;
    buy.type = OrderType::LIMIT;
    buy.price = 50000 * PRICE_SCALE;
    buy.quantity = 100;
    buy.tif = TimeInForce::GTC;
    
    book.add_order(buy);
    
    Order sell;
    sell.id = 2;
    sell.symbol = Symbol("BTCUSD");
    sell.side = Side::SELL;
    sell.type = OrderType::LIMIT;
    sell.price = 50000 * PRICE_SCALE;
    sell.quantity = 50;
    sell.tif = TimeInForce::GTC;
    
    auto trades = book.match_order(sell);
    
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_EQ(trades[0].price, 50000 * PRICE_SCALE);
}

TEST(OrderBookTest, CancelOrder) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order order;
    order.id = 1;
    order.symbol = Symbol("BTCUSD");
    order.side = Side::BUY;
    order.type = OrderType::LIMIT;
    order.price = 50000 * PRICE_SCALE;
    order.quantity = 100;
    
    book.add_order(order);
    EXPECT_TRUE(book.cancel_order(1));
    EXPECT_EQ(book.best_bid(), 0);
}

TEST(OrderBookTest, IOCOrder) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order buy;
    buy.id = 1;
    buy.symbol = Symbol("BTCUSD");
    buy.side = Side::BUY;
    buy.type = OrderType::LIMIT;
    buy.price = 50000 * PRICE_SCALE;
    buy.quantity = 100;
    buy.tif = TimeInForce::IOC;
    
    auto trades = book.match_order(buy);
    
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy.status, OrderStatus::CANCELLED);
}

TEST(OrderBookTest, FOKOrderFails) {
    OrderBook book(Symbol("BTCUSD"));
    
    Order sell;
    sell.id = 1;
    sell.symbol = Symbol("BTCUSD");
    sell.side = Side::SELL;
    sell.type = OrderType::LIMIT;
    sell.price = 50000 * PRICE_SCALE;
    sell.quantity = 50;
    sell.tif = TimeInForce::GTC;
    
    book.add_order(sell);
    
    Order buy;
    buy.id = 2;
    buy.symbol = Symbol("BTCUSD");
    buy.side = Side::BUY;
    buy.type = OrderType::LIMIT;
    buy.price = 50000 * PRICE_SCALE;
    buy.quantity = 100;
    buy.tif = TimeInForce::FOK;
    
    auto trades = book.match_order(buy);
    
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(buy.status, OrderStatus::REJECTED);
}
