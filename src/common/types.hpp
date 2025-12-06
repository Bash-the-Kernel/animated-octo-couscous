#pragma once

#include <cstdint>
#include <string_view>
#include <array>

namespace hft {

using OrderId = uint64_t;
using Price = int64_t;  // Fixed-point: price * 10000
using Quantity = uint64_t;
using Timestamp = uint64_t;  // nanoseconds since epoch
using SequenceNum = uint64_t;

constexpr Price PRICE_SCALE = 10000;
constexpr OrderId INVALID_ORDER_ID = 0;

enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

enum class TimeInForce : uint8_t {
    GTC = 0,  // Good-Till-Cancel
    IOC = 1,  // Immediate-Or-Cancel
    FOK = 2   // Fill-Or-Kill
};

enum class OrderStatus : uint8_t {
    PENDING = 0,
    ACCEPTED = 1,
    PARTIALLY_FILLED = 2,
    FILLED = 3,
    CANCELLED = 4,
    REJECTED = 5
};

enum class EventType : uint8_t {
    NEW_ORDER = 0,
    CANCEL_ORDER = 1,
    MODIFY_ORDER = 2,
    TRADE = 3,
    ORDER_ACK = 4,
    ORDER_REJECT = 5
};

struct Symbol {
    std::array<char, 16> data{};
    uint8_t len{0};
    
    constexpr Symbol() = default;
    constexpr Symbol(std::string_view sv) : len(sv.size()) {
        for (size_t i = 0; i < sv.size() && i < 16; ++i) {
            data[i] = sv[i];
        }
    }
    
    constexpr std::string_view view() const noexcept {
        return {data.data(), len};
    }
    
    constexpr bool operator==(const Symbol& other) const noexcept {
        return view() == other.view();
    }
};

struct Order {
    OrderId id{INVALID_ORDER_ID};
    Symbol symbol;
    Side side{Side::BUY};
    OrderType type{OrderType::LIMIT};
    TimeInForce tif{TimeInForce::GTC};
    Price price{0};
    Quantity quantity{0};
    Quantity filled{0};
    Timestamp timestamp{0};
    OrderStatus status{OrderStatus::PENDING};
    
    [[nodiscard]] constexpr Quantity remaining() const noexcept {
        return quantity - filled;
    }
    
    [[nodiscard]] constexpr bool is_complete() const noexcept {
        return status == OrderStatus::FILLED || 
               status == OrderStatus::CANCELLED ||
               status == OrderStatus::REJECTED;
    }
};

struct Trade {
    OrderId buyer_id{INVALID_ORDER_ID};
    OrderId seller_id{INVALID_ORDER_ID};
    Symbol symbol;
    Price price{0};
    Quantity quantity{0};
    Timestamp timestamp{0};
    bool buyer_is_aggressor{false};
};

struct OrderAck {
    OrderId order_id{INVALID_ORDER_ID};
    OrderStatus status{OrderStatus::ACCEPTED};
    Timestamp timestamp{0};
};

struct OrderReject {
    OrderId order_id{INVALID_ORDER_ID};
    std::array<char, 64> reason{};
    Timestamp timestamp{0};
};

} // namespace hft

namespace std {
    template<>
    struct hash<hft::Symbol> {
        size_t operator()(const hft::Symbol& s) const noexcept {
            size_t h = 0;
            for (size_t i = 0; i < s.len; ++i) {
                h = h * 31 + static_cast<size_t>(s.data[i]);
            }
            return h;
        }
    };
}
