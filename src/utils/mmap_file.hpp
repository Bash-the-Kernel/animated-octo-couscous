#pragma once

#include <string>
#include <cstddef>
#include <span>

namespace hft {

class MMapFile {
public:
    MMapFile() = default;
    ~MMapFile();
    
    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;
    MMapFile(MMapFile&&) noexcept;
    MMapFile& operator=(MMapFile&&) noexcept;
    
    bool open(const std::string& path, size_t size, bool read_only = false);
    void close();
    
    [[nodiscard]] std::span<std::byte> data() noexcept;
    [[nodiscard]] std::span<const std::byte> data() const noexcept;
    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] bool is_open() const noexcept { return addr_ != nullptr; }
    
    bool sync();
    
private:
    void* addr_{nullptr};
    size_t size_{0};
    int fd_{-1};
};

} // namespace hft
