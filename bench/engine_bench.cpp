#include <benchmark/benchmark.h>
#include "engine/matching_engine.hpp"
#include "utils/time.hpp"

using namespace hft;

static void BM_EngineLatency(benchmark::State& state) {
    MatchingEngine engine;
    
    std::vector<uint64_t> latencies;
    latencies.reserve(state.max_iterations);
    
    engine.set_ack_callback([&](const OrderAck& ack) {
        // Callback for acknowledgment
    });
    
    engine.start();
    
    for (auto _ : state) {
        auto start = now_ns();
        
        NewOrderEvent event;
        event.order.symbol = Symbol("BTCUSD");
        event.order.side = Side::BUY;
        event.order.type = OrderType::LIMIT;
        event.order.price = 50000 * PRICE_SCALE;
        event.order.quantity = 100;
        event.order.tif = TimeInForce::GTC;
        
        engine.submit_event(event);
        
        auto end = now_ns();
        latencies.push_back(end - start);
    }
    
    engine.stop();
    
    // Calculate percentiles
    std::sort(latencies.begin(), latencies.end());
    
    auto p50 = latencies[latencies.size() * 50 / 100];
    auto p95 = latencies[latencies.size() * 95 / 100];
    auto p99 = latencies[latencies.size() * 99 / 100];
    auto p999 = latencies[latencies.size() * 999 / 1000];
    
    state.counters["p50_ns"] = p50;
    state.counters["p95_ns"] = p95;
    state.counters["p99_ns"] = p99;
    state.counters["p999_ns"] = p999;
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EngineLatency)->Iterations(10000);

static void BM_EngineThroughput(benchmark::State& state) {
    MatchingEngine engine;
    engine.start();
    
    OrderId order_id = 1;
    
    for (auto _ : state) {
        NewOrderEvent event;
        event.order.id = order_id++;
        event.order.symbol = Symbol("BTCUSD");
        event.order.side = (order_id % 2) ? Side::BUY : Side::SELL;
        event.order.type = OrderType::LIMIT;
        event.order.price = 50000 * PRICE_SCALE;
        event.order.quantity = 100;
        event.order.tif = TimeInForce::GTC;
        
        while (!engine.submit_event(event)) {
            // Spin until queue has space
        }
    }
    
    engine.stop();
    
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("orders/sec");
}
BENCHMARK(BM_EngineThroughput);

static void BM_RingBufferPush(benchmark::State& state) {
    RingBuffer<EngineEvent, 65536> queue;
    
    NewOrderEvent event;
    event.order.symbol = Symbol("BTCUSD");
    event.order.side = Side::BUY;
    event.order.type = OrderType::LIMIT;
    event.order.price = 50000 * PRICE_SCALE;
    event.order.quantity = 100;
    
    for (auto _ : state) {
        bool success = queue.try_push(event);
        benchmark::DoNotOptimize(success);
        
        if (!success) {
            queue.try_pop(); // Make space
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RingBufferPush);

BENCHMARK_MAIN();
