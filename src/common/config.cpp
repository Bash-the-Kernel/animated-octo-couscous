#include "config.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace hft {

Config Config::load(const std::string& path) {
    Config cfg;
    std::ifstream file(path);
    if (!file.is_open()) return cfg;
    
    nlohmann::json j;
    file >> j;
    
    if (j.contains("feed_url")) cfg.feed_url = j["feed_url"];
    if (j.contains("wal_path")) cfg.wal_path = j["wal_path"];
    if (j.contains("snapshot_path")) cfg.snapshot_path = j["snapshot_path"];
    if (j.contains("snapshot_interval_sec")) cfg.snapshot_interval_sec = j["snapshot_interval_sec"];
    if (j.contains("http_port")) cfg.http_port = j["http_port"];
    if (j.contains("ws_port")) cfg.ws_port = j["ws_port"];
    if (j.contains("enable_metrics")) cfg.enable_metrics = j["enable_metrics"];
    
    return cfg;
}

} // namespace hft
