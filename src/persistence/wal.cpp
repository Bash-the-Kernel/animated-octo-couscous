#include "wal.hpp"
#include "../common/logging.hpp"
#include <filesystem>

namespace hft {

WAL::WAL(const std::string& path) : path_(path) {
    buffer_.reserve(4096);
}

WAL::~WAL() {
    close();
}

bool WAL::open() {
    std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
    
    bool exists = std::filesystem::exists(path_);
    
    file_.open(path_, std::ios::in | std::ios::out | std::ios::binary | std::ios::app);
    if (!file_.is_open()) {
        LOG_ERROR("Failed to open WAL: ", path_);
        return false;
    }
    
    if (exists) {
        // Read existing header
        file_.seekg(0);
        file_.read(reinterpret_cast<char*>(&header_), sizeof(header_));
        
        if (header_.magic != 0x57414C46) {
            LOG_ERROR("Invalid WAL magic");
            return false;
        }
    } else {
        // Write new header
        file_.seekp(0);
        file_.write(reinterpret_cast<const char*>(&header_), sizeof(header_));
        file_.flush();
    }
    
    LOG_INFO("WAL opened: ", path_, ", entries: ", header_.entry_count);
    return true;
}

void WAL::close() {
    if (file_.is_open()) {
        sync();
        file_.close();
    }
}

bool WAL::append(EventType type, SequenceNum seq, const void* payload, size_t size) {
    if (!file_.is_open()) return false;
    
    WALEntryHeader entry_header{
        .type = type,
        .sequence = seq,
        .timestamp = 0,  // Set by caller if needed
        .payload_size = static_cast<uint32_t>(size)
    };
    
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<const char*>(&entry_header), sizeof(entry_header));
    file_.write(static_cast<const char*>(payload), size);
    
    ++header_.entry_count;
    
    return file_.good();
}

bool WAL::sync() {
    if (!file_.is_open()) return false;
    
    // Update header
    file_.seekp(0);
    file_.write(reinterpret_cast<const char*>(&header_), sizeof(header_));
    file_.flush();
    
    return file_.good();
}

std::vector<WAL::Entry> WAL::read_all() {
    std::vector<Entry> entries;
    if (!file_.is_open()) return entries;
    
    file_.seekg(sizeof(WALHeader));
    
    while (file_.good() && !file_.eof()) {
        WALEntryHeader header;
        file_.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (file_.gcount() != sizeof(header)) break;
        
        Entry entry;
        entry.header = header;
        entry.payload.resize(header.payload_size);
        file_.read(reinterpret_cast<char*>(entry.payload.data()), header.payload_size);
        
        if (file_.gcount() != header.payload_size) break;
        
        entries.push_back(std::move(entry));
    }
    
    return entries;
}

} // namespace hft
