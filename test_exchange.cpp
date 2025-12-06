#include "src/common/logging.hpp"
#include "src/engine/matching_engine.hpp"
#include "src/engine/event.hpp"
#include <iostream>

using namespace hft;

int main() {
    LOG_INFO("Testing HFT Exchange...");
    
    MatchingEngine engine;
    
    int trade_count = 0;
    engine.set_trade_callback([&](const Trade& trade) {
        LOG_INFO("TRADE: ", trade.quantity, " @ ", trade.price / PRICE_SCALE);
        trade_count++;
    });
    
    engine.set_ack_callback([](const OrderAck& ack) {
        LOG_INFO("ACK: Order ", ack.order_id, " status=", static_cast<int>(ack.status));
    });
    
    engine.start();
    
    // Submit buy order
    NewOrderEvent buy;
    buy.order.id = 1;
    buy.order.symbol = Symbol("BTCUSD");
    buy.order.side = Side::BUY;
    buy.order.type = OrderType::LIMIT;
    buy.order.price = 50000 * PRICE_SCALE;
    buy.order.quantity = 100;
    buy.order.tif = TimeInForce::GTC;
    
    LOG_INFO("Submitting BUY order...");
    engine.submit_event(buy);
    
    // Submit sell order (will match)
    NewOrderEvent sell;
    sell.order.id = 2;
    sell.order.symbol = Symbol("BTCUSD");
    sell.order.side = Side::SELL;
    sell.order.type = OrderType::LIMIT;
    sell.order.price = 50000 * PRICE_SCALE;
    sell.order.quantity = 50;
    sell.order.tif = TimeInForce::GTC;
    
    LOG_INFO("Submitting SELL order...");
    engine.submit_event(sell);
    
    LOG_INFO("Processed: ", engine.processed_count(), " events");
    LOG_INFO("Trades: ", trade_count);
    
    engine.stop();
    
    LOG_INFO("Test complete!");
    return 0;
}
