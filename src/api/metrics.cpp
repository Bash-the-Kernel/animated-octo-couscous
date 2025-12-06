#include "metrics.hpp"
#include <sstream>
#include <algorithm>
#include <vector>

namespace hft {

void Metrics::record_latency(uint64_t ns) {
    size_t idx = latency_idx_.fetch_add(1) % latency_samples_.size();
    latency_samples_[idx].store(ns, std::memory_order_relaxed);
}

void Metrics::increment_orders() {
    orders_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::increment_trades() {
    trades_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::increment_cancels() {
    cancels_.fetch_add(1, std::memory_order_relaxed);
}

std::string Metrics::prometheus_format() const {
    std::ostringstream oss;
    
    oss << "# HELP hft_orders_total Total orders processed\n";
    oss << "# TYPE hft_orders_total counter\n";
    oss << "hft_orders_total " << orders_.load() << "\n\n";
    
    oss << "# HELP hft_trades_total Total trades executed\n";
    oss << "# TYPE hft_trades_total counter\n";
    oss << "hft_trades_total " << trades_.load() << "\n\n";
    
    oss << "# HELP hft_cancels_total Total cancellations\n";
    oss << "# TYPE hft_cancels_total counter\n";
    oss << "hft_cancels_total " << cancels_.load() << "\n\n";
    
    auto stats = get_stats();
    
    oss << "# HELP hft_latency_p50 P50 latency in nanoseconds\n";
    oss << "# TYPE hft_latency_p50 gauge\n";
    oss << "hft_latency_p50 " << stats.latency_p50 << "\n\n";
    
    oss << "# HELP hft_latency_p95 P95 latency in nanoseconds\n";
    oss << "# TYPE hft_latency_p95 gauge\n";
    oss << "hft_latency_p95 " << stats.latency_p95 << "\n\n";
    
    oss << "# HELP hft_latency_p99 P99 latency in nanoseconds\n";
    oss << "# TYPE hft_latency_p99 gauge\n";
    oss << "hft_latency_p99 " << stats.latency_p99 << "\n";
    
    return oss.str();
}

Metrics::Stats Metrics::get_stats() const {
    Stats stats;
    stats.orders = orders_.load();
    stats.trades = trades_.load();
    stats.cancels = cancels_.load();
    
    // Collect latency samples
    std::vector<uint64_t> samples;
    samples.reserve(latency_samples_.size());
    
    for (const auto& sample : latency_samples_) {
        uint64_t val = sample.load(std::memory_order_relaxed);
        if (val > 0) samples.push_back(val);
    }
    
    if (!samples.empty()) {
        std::sort(samples.begin(), samples.end());
        stats.latency_p50 = samples[samples.size() * 50 / 100];
        stats.latency_p95 = samples[samples.size() * 95 / 100];
        stats.latency_p99 = samples[samples.size() * 99 / 100];
    }
    
    return stats;
}

} // namespace hft
