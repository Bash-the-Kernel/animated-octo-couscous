#include "common/config.hpp"
#include "common/logging.hpp"
#include "engine/matching_engine.hpp"
#include "feed/feed_handler.hpp"
#include "publish/publisher.hpp"
#include "api/http_server.hpp"
#include "api/metrics.hpp"
#include "persistence/wal.hpp"
#include "persistence/snapshot.hpp"
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running.store(false);
}

int main(int argc, char** argv) {
    using namespace hft;
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    LOG_INFO("HFT Exchange starting...");
    
    // Load config
    Config config;
    if (argc > 1) {
        config = Config::load(argv[1]);
    }
    
    // Initialize components
    MatchingEngine engine;
    FeedHandler feed(engine);
    Publisher publisher;
    HttpServer http_server(engine);
    WAL wal(config.wal_path);
    Snapshot snapshot(config.snapshot_path);
    
    // Set up callbacks
    engine.set_trade_callback([&](const Trade& trade) {
        publisher.publish_trade(trade);
        Metrics::instance().increment_trades();
    });
    
    engine.set_ack_callback([](const OrderAck& ack) {
        if (ack.status == OrderStatus::ACCEPTED) {
            Metrics::instance().increment_orders();
        } else if (ack.status == OrderStatus::CANCELLED) {
            Metrics::instance().increment_cancels();
        }
    });
    
    // Open WAL
    if (!wal.open()) {
        LOG_ERROR("Failed to open WAL");
        return 1;
    }
    
    // Load snapshot if exists
    if (snapshot.exists()) {
        LOG_INFO("Loading snapshot...");
        snapshot.load(engine);
    }
    
    // Start components
    engine.start();
    feed.start(config.feed_url);
    publisher.start(config.ws_port);
    
    // Start HTTP server in separate thread
    std::thread http_thread([&] {
        http_server.start(config.http_port);
    });
    
    LOG_INFO("HFT Exchange running. Press Ctrl+C to stop.");
    
    // Main loop
    auto last_snapshot = std::chrono::steady_clock::now();
    
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Periodic snapshot
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_snapshot).count();
        
        if (elapsed >= config.snapshot_interval_sec) {
            LOG_INFO("Creating snapshot...");
            snapshot.save(engine);
            wal.sync();
            last_snapshot = now;
        }
    }
    
    LOG_INFO("Shutting down...");
    
    // Stop components
    feed.stop();
    engine.stop();
    publisher.stop();
    http_server.stop();
    
    if (http_thread.joinable()) {
        http_thread.detach();
    }
    
    // Final snapshot
    snapshot.save(engine);
    wal.close();
    
    LOG_INFO("HFT Exchange stopped");
    return 0;
}
