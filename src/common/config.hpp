#pragma once

#include <string>
#include <cstdint>

namespace hft {

struct Config {
    std::string feed_url{"ws://localhost:9001"};
    std::string wal_path{"data/wal.bin"};
    std::string snapshot_path{"data/snapshot.bin"};
    uint32_t snapshot_interval_sec{60};
    uint16_t http_port{8080};
    uint16_t ws_port{9002};
    bool enable_metrics{true};
    
    static Config load(const std::string& path);
};

} // namespace hft
