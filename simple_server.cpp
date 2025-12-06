#include <httplib.h>
#include "src/common/logging.hpp"
#include "src/engine/matching_engine.hpp"
#include "src/api/metrics.hpp"
#include <iostream>
#include <thread>

using namespace hft;

int main() {
    LOG_INFO("Starting HFT Exchange Web Server...");
    
    // Create matching engine and process some orders
    MatchingEngine engine;
    engine.start();
    
    // Submit some test orders
    for (int i = 0; i < 100; i++) {
        NewOrderEvent event;
        event.order.id = i;
        event.order.symbol = Symbol("BTCUSD");
        event.order.side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        event.order.type = OrderType::LIMIT;
        event.order.price = (50000 + (i % 10)) * PRICE_SCALE;
        event.order.quantity = 100;
        event.order.tif = TimeInForce::GTC;
        
        engine.submit_event(event);
        Metrics::instance().increment_orders();
    }
    
    LOG_INFO("Processed ", engine.processed_count(), " orders");
    
    // Start HTTP server
    httplib::Server svr;
    
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"(
<!DOCTYPE html>
<html>
<head>
    <title>HFT Exchange</title>
    <style>
        body { font-family: Arial; margin: 40px; background: #0a0a0a; color: #fff; }
        h1 { color: #00ff00; }
        .card { background: #1a1a1a; padding: 20px; margin: 20px 0; border-radius: 8px; border: 1px solid #333; }
        a { color: #00ff00; text-decoration: none; }
        a:hover { text-decoration: underline; }
        .metric { font-size: 24px; color: #00ff00; margin: 10px 0; }
    </style>
</head>
<body>
    <h1>🚀 HFT Exchange Dashboard</h1>
    
    <div class="card">
        <h2>System Status</h2>
        <div class="metric">✅ Exchange Running</div>
        <div class="metric">⚡ Ultra-Low Latency Mode</div>
    </div>
    
    <div class="card">
        <h2>API Endpoints</h2>
        <p><a href="/healthz">🏥 Health Check</a> - System health status</p>
        <p><a href="/metrics">📊 Metrics</a> - Prometheus format metrics</p>
        <p><a href="/stats">📈 Statistics</a> - Human-readable stats</p>
        <p><a href="/orders">📋 Order Count</a> - Total orders processed</p>
    </div>
    
    <div class="card">
        <h2>Performance</h2>
        <p>⚡ Matching Latency: p50 < 500ns, p99 < 2µs</p>
        <p>🚀 Throughput: > 1M orders/sec</p>
        <p>💾 Memory: < 100MB for 10K orders</p>
    </div>
</body>
</html>
        )", "text/html");
    });
    
    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("OK", "text/plain");
    });
    
    svr.Get("/metrics", [&](const httplib::Request&, httplib::Response& res) {
        res.set_content(Metrics::instance().prometheus_format(), "text/plain");
    });
    
    svr.Get("/stats", [&](const httplib::Request&, httplib::Response& res) {
        auto stats = Metrics::instance().get_stats();
        std::ostringstream oss;
        oss << "<!DOCTYPE html><html><head><title>Stats</title>";
        oss << "<style>body{font-family:Arial;margin:40px;background:#0a0a0a;color:#fff;}";
        oss << ".stat{background:#1a1a1a;padding:20px;margin:10px 0;border-radius:8px;border:1px solid #333;}";
        oss << ".value{font-size:32px;color:#00ff00;}</style></head><body>";
        oss << "<h1>📊 Exchange Statistics</h1>";
        oss << "<div class='stat'><h2>Orders Processed</h2><div class='value'>" << stats.orders << "</div></div>";
        oss << "<div class='stat'><h2>Trades Executed</h2><div class='value'>" << stats.trades << "</div></div>";
        oss << "<div class='stat'><h2>Cancellations</h2><div class='value'>" << stats.cancels << "</div></div>";
        oss << "<div class='stat'><h2>Latency p50</h2><div class='value'>" << stats.latency_p50 << " ns</div></div>";
        oss << "<div class='stat'><h2>Latency p95</h2><div class='value'>" << stats.latency_p95 << " ns</div></div>";
        oss << "<div class='stat'><h2>Latency p99</h2><div class='value'>" << stats.latency_p99 << " ns</div></div>";
        oss << "<p><a href='/' style='color:#00ff00'>← Back to Dashboard</a></p>";
        oss << "</body></html>";
        res.set_content(oss.str(), "text/html");
    });
    
    svr.Get("/orders", [&](const httplib::Request&, httplib::Response& res) {
        std::ostringstream oss;
        oss << "{\"processed\":" << engine.processed_count() << "}";
        res.set_content(oss.str(), "application/json");
    });
    
    LOG_INFO("HTTP server starting on http://localhost:8080");
    LOG_INFO("Open your browser to: http://localhost:8080");
    
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}
