#include "mmap_file.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace hft {

MMapFile::~MMapFile() {
    close();
}

MMapFile::MMapFile(MMapFile&& other) noexcept
    : addr_(other.addr_), size_(other.size_), fd_(other.fd_) {
    other.addr_ = nullptr;
    other.size_ = 0;
    other.fd_ = -1;
}

MMapFile& MMapFile::operator=(MMapFile&& other) noexcept {
    if (this != &other) {
        close();
        addr_ = other.addr_;
        size_ = other.size_;
        fd_ = other.fd_;
        other.addr_ = nullptr;
        other.size_ = 0;
        other.fd_ = -1;
    }
    return *this;
}

bool MMapFile::open(const std::string& path, size_t size, bool read_only) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path.c_str(),
        read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
        0, nullptr,
        read_only ? OPEN_EXISTING : OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) return false;
    
    if (!read_only) {
        LARGE_INTEGER li;
        li.QuadPart = size;
        SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN);
        SetEndOfFile(hFile);
    }
    
    HANDLE hMap = CreateFileMappingA(hFile, nullptr,
        read_only ? PAGE_READONLY : PAGE_READWRITE,
        0, 0, nullptr);
    
    if (!hMap) {
        CloseHandle(hFile);
        return false;
    }
    
    addr_ = MapViewOfFile(hMap, read_only ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, size);
    CloseHandle(hMap);
    CloseHandle(hFile);
    
    if (!addr_) return false;
    size_ = size;
    return true;
#else
    int flags = read_only ? O_RDONLY : (O_RDWR | O_CREAT);
    fd_ = ::open(path.c_str(), flags, 0644);
    if (fd_ < 0) return false;
    
    if (!read_only) {
        if (ftruncate(fd_, size) < 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }
    }
    
    int prot = read_only ? PROT_READ : (PROT_READ | PROT_WRITE);
    addr_ = mmap(nullptr, size, prot, MAP_SHARED, fd_, 0);
    
    if (addr_ == MAP_FAILED) {
        ::close(fd_);
        fd_ = -1;
        addr_ = nullptr;
        return false;
    }
    
    size_ = size;
    return true;
#endif
}

void MMapFile::close() {
    if (!addr_) return;
    
#ifdef _WIN32
    UnmapViewOfFile(addr_);
#else
    munmap(addr_, size_);
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    
    addr_ = nullptr;
    size_ = 0;
}

std::span<std::byte> MMapFile::data() noexcept {
    return {static_cast<std::byte*>(addr_), size_};
}

std::span<const std::byte> MMapFile::data() const noexcept {
    return {static_cast<const std::byte*>(addr_), size_};
}

bool MMapFile::sync() {
    if (!addr_) return false;
    
#ifdef _WIN32
    return FlushViewOfFile(addr_, size_) != 0;
#else
    return msync(addr_, size_, MS_SYNC) == 0;
#endif
}

} // namespace hft
