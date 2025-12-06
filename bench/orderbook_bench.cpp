#include <benchmark/benchmark.h>
#include "engine/order_book.hpp"

using namespace hft;

static void BM_AddOrder(benchmark::State& state) {
    OrderBook book(Symbol("BTCUSD"));
    OrderId id = 1;
    
    for (auto _ : state) {
        Order order;
        order.id = id++;
        order.symbol = Symbol("BTCUSD");
        order.side = Side::BUY;
        order.type = OrderType::LIMIT;
        order.price = 50000 * PRICE_SCALE;
        order.quantity = 100;
        
        book.add_order(order);
        benchmark::DoNotOptimize(book);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder);

static void BM_MatchOrder(benchmark::State& state) {
    OrderBook book(Symbol("BTCUSD"));
    
    // Pre-populate with buy orders
    for (int i = 0; i < 100; ++i) {
        Order order;
        order.id = i;
        order.symbol = Symbol("BTCUSD");
        order.side = Side::BUY;
        order.type = OrderType::LIMIT;
        order.price = (50000 - i) * PRICE_SCALE;
        order.quantity = 100;
        
        book.add_order(order);
    }
    
    OrderId sell_id = 1000;
    
    for (auto _ : state) {
        Order sell;
        sell.id = sell_id++;
        sell.symbol = Symbol("BTCUSD");
        sell.side = Side::SELL;
        sell.type = OrderType::LIMIT;
        sell.price = 49900 * PRICE_SCALE;
        sell.quantity = 50;
        sell.tif = TimeInForce::GTC;
        
        auto trades = book.match_order(sell);
        benchmark::DoNotOptimize(trades);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchOrder);

static void BM_CancelOrder(benchmark::State& state) {
    OrderBook book(Symbol("BTCUSD"));
    
    for (auto _ : state) {
        state.PauseTiming();
        
        Order order;
        order.id = 1;
        order.symbol = Symbol("BTCUSD");
        order.side = Side::BUY;
        order.type = OrderType::LIMIT;
        order.price = 50000 * PRICE_SCALE;
        order.quantity = 100;
        
        book.add_order(order);
        
        state.ResumeTiming();
        
        bool result = book.cancel_order(1);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder);

static void BM_MatchMarketOrder(benchmark::State& state) {
    OrderBook book(Symbol("BTCUSD"));
    
    // Pre-populate with sell orders
    for (int i = 0; i < 100; ++i) {
        Order order;
        order.id = i;
        order.symbol = Symbol("BTCUSD");
        order.side = Side::SELL;
        order.type = OrderType::LIMIT;
        order.price = (50000 + i) * PRICE_SCALE;
        order.quantity = 100;
        
        book.add_order(order);
    }
    
    OrderId buy_id = 1000;
    
    for (auto _ : state) {
        Order buy;
        buy.id = buy_id++;
        buy.symbol = Symbol("BTCUSD");
        buy.side = Side::BUY;
        buy.type = OrderType::MARKET;
        buy.quantity = 50;
        buy.tif = TimeInForce::IOC;
        
        auto trades = book.match_order(buy);
        benchmark::DoNotOptimize(trades);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchMarketOrder);

BENCHMARK_MAIN();
