#pragma once

#include <atomic>
#include <array>
#include <optional>

namespace hft {

// Lock-free SPSC ring buffer
template<typename T, size_t Capacity>
class RingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    
public:
    bool try_push(const T& item) noexcept {
        const size_t write_pos = write_pos_.load(std::memory_order_relaxed);
        const size_t next_write = (write_pos + 1) & mask_;
        
        if (next_write == read_pos_.load(std::memory_order_acquire)) {
            return false; // Full
        }
        
        buffer_[write_pos] = item;
        write_pos_.store(next_write, std::memory_order_release);
        return true;
    }
    
    std::optional<T> try_pop() noexcept {
        const size_t read_pos = read_pos_.load(std::memory_order_relaxed);
        
        if (read_pos == write_pos_.load(std::memory_order_acquire)) {
            return std::nullopt; // Empty
        }
        
        T item = buffer_[read_pos];
        read_pos_.store((read_pos + 1) & mask_, std::memory_order_release);
        return item;
    }
    
    [[nodiscard]] bool empty() const noexcept {
        return read_pos_.load(std::memory_order_acquire) == 
               write_pos_.load(std::memory_order_acquire);
    }
    
    [[nodiscard]] size_t size() const noexcept {
        const size_t write = write_pos_.load(std::memory_order_acquire);
        const size_t read = read_pos_.load(std::memory_order_acquire);
        return (write - read) & mask_;
    }
    
private:
    static constexpr size_t mask_ = Capacity - 1;
    alignas(64) std::atomic<size_t> write_pos_{0};
    alignas(64) std::atomic<size_t> read_pos_{0};
    std::array<T, Capacity> buffer_;
};

} // namespace hft
