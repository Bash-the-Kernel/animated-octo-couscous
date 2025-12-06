#pragma once

#include <atomic>
#include <string>
#include <chrono>
#include <array>

namespace hft {

class Metrics {
public:
    static Metrics& instance() {
        static Metrics m;
        return m;
    }
    
    void record_latency(uint64_t ns);
    void increment_orders();
    void increment_trades();
    void increment_cancels();
    
    [[nodiscard]] std::string prometheus_format() const;
    
    struct Stats {
        uint64_t orders{0};
        uint64_t trades{0};
        uint64_t cancels{0};
        uint64_t latency_p50{0};
        uint64_t latency_p95{0};
        uint64_t latency_p99{0};
    };
    
    [[nodiscard]] Stats get_stats() const;
    
private:
    std::atomic<uint64_t> orders_{0};
    std::atomic<uint64_t> trades_{0};
    std::atomic<uint64_t> cancels_{0};
    
    // Simple latency tracking
    std::array<std::atomic<uint64_t>, 1000> latency_samples_{};
    std::atomic<size_t> latency_idx_{0};
};

} // namespace hft
