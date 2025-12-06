#include <gtest/gtest.h>
#include "feed/parser.hpp"

using namespace hft;

TEST(ParserTest, ParseNewOrder) {
    std::string json = R"({
        "type": "new_order",
        "order_id": 123,
        "symbol": "BTCUSD",
        "side": "buy",
        "order_type": "limit",
        "tif": "gtc",
        "price": 50000.0,
        "quantity": 100
    })";
    
    auto result = Parser::parse_json(json);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<NewOrderEvent>(result.event));
    
    auto& event = std::get<NewOrderEvent>(result.event);
    EXPECT_EQ(event.order.id, 123);
    EXPECT_EQ(event.order.symbol, Symbol("BTCUSD"));
    EXPECT_EQ(event.order.side, Side::BUY);
    EXPECT_EQ(event.order.type, OrderType::LIMIT);
    EXPECT_EQ(event.order.price, 50000 * PRICE_SCALE);
    EXPECT_EQ(event.order.quantity, 100);
}

TEST(ParserTest, ParseCancel) {
    std::string json = R"({
        "type": "cancel",
        "order_id": 123,
        "symbol": "BTCUSD"
    })";
    
    auto result = Parser::parse_json(json);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<CancelOrderEvent>(result.event));
    
    auto& event = std::get<CancelOrderEvent>(result.event);
    EXPECT_EQ(event.order_id, 123);
    EXPECT_EQ(event.symbol, Symbol("BTCUSD"));
}

TEST(ParserTest, ParseModify) {
    std::string json = R"({
        "type": "modify",
        "order_id": 123,
        "symbol": "BTCUSD",
        "price": 51000.0,
        "quantity": 200
    })";
    
    auto result = Parser::parse_json(json);
    
    ASSERT_TRUE(result.success);
    ASSERT_TRUE(std::holds_alternative<ModifyOrderEvent>(result.event));
    
    auto& event = std::get<ModifyOrderEvent>(result.event);
    EXPECT_EQ(event.order_id, 123);
    EXPECT_EQ(event.new_price, 51000 * PRICE_SCALE);
    EXPECT_EQ(event.new_quantity, 200);
}

TEST(ParserTest, InvalidJSON) {
    std::string json = "invalid json";
    
    auto result = Parser::parse_json(json);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(ParserTest, MalformedMessage) {
    std::string json = R"({
        "type": "new_order",
        "side": "invalid_side"
    })";
    
    auto result = Parser::parse_json(json);
    
    EXPECT_FALSE(result.success);
}
