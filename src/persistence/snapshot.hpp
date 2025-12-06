#pragma once

#include "../engine/matching_engine.hpp"
#include "../utils/mmap_file.hpp"
#include <string>

namespace hft {

#pragma pack(push, 1)
struct SnapshotHeader {
    uint32_t magic{0x534E4150};  // "SNAP"
    uint32_t version{1};
    uint64_t timestamp;
    uint64_t checksum;
    uint64_t data_size;
};
#pragma pack(pop)

class Snapshot {
public:
    explicit Snapshot(const std::string& path);
    
    bool save(const MatchingEngine& engine);
    bool load(MatchingEngine& engine);
    
    [[nodiscard]] bool exists() const;
    
private:
    std::string path_;
    
    uint64_t compute_checksum(const std::byte* data, size_t size);
};

} // namespace hft
