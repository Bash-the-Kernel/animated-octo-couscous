#include "snapshot.hpp"
#include "../common/logging.hpp"
#include <filesystem>
#include <fstream>

namespace hft {

Snapshot::Snapshot(const std::string& path) : path_(path) {}

bool Snapshot::save(const MatchingEngine& engine) {
    // Simplified: serialize basic state
    // In production, would serialize full order book state
    
    std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
    
    std::ofstream file(path_, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create snapshot: ", path_);
        return false;
    }
    
    SnapshotHeader header;
    header.timestamp = system_now_ns();
    header.data_size = 0;  // Placeholder
    header.checksum = 0;
    
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // TODO: Serialize order books
    // For now, just write header
    
    file.close();
    
    LOG_INFO("Snapshot saved: ", path_);
    return true;
}

bool Snapshot::load(MatchingEngine& engine) {
    if (!exists()) return false;
    
    std::ifstream file(path_, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open snapshot: ", path_);
        return false;
    }
    
    SnapshotHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (header.magic != 0x534E4150) {
        LOG_ERROR("Invalid snapshot magic");
        return false;
    }
    
    // TODO: Deserialize order books
    
    LOG_INFO("Snapshot loaded: ", path_);
    return true;
}

bool Snapshot::exists() const {
    return std::filesystem::exists(path_);
}

uint64_t Snapshot::compute_checksum(const std::byte* data, size_t size) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

} // namespace hft
