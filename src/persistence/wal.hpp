#pragma once

#include "../common/types.hpp"
#include "../engine/event.hpp"
#include <fstream>
#include <vector>
#include <cstring>

namespace hft {

#pragma pack(push, 1)
struct WALHeader {
    uint32_t magic{0x57414C46};  // "WALF"
    uint32_t version{1};
    uint64_t entry_count{0};
};

struct WALEntryHeader {
    EventType type;
    SequenceNum sequence;
    Timestamp timestamp;
    uint32_t payload_size;
};
#pragma pack(pop)

class WAL {
public:
    explicit WAL(const std::string& path);
    ~WAL();
    
    bool open();
    void close();
    
    bool append(EventType type, SequenceNum seq, const void* payload, size_t size);
    bool sync();
    
    struct Entry {
        WALEntryHeader header;
        std::vector<uint8_t> payload;
    };
    
    std::vector<Entry> read_all();
    
    [[nodiscard]] uint64_t entry_count() const noexcept { return header_.entry_count; }
    
private:
    std::string path_;
    std::fstream file_;
    WALHeader header_;
    std::vector<uint8_t> buffer_;
};

} // namespace hft
