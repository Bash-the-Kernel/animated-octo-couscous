#pragma once

#include "../common/types.hpp"
#include "../engine/event.hpp"
#include <string_view>
#include <optional>
#include <string>

namespace hft {

class Parser {
public:
    struct ParseResult {
        bool success{false};
        EngineEvent event;
        std::string error;
    };
    
    static ParseResult parse_json(std::string_view json);
    static ParseResult parse_binary(const uint8_t* data, size_t size);
    
private:
    static std::optional<Side> parse_side(std::string_view str);
    static std::optional<OrderType> parse_order_type(std::string_view str);
    static std::optional<TimeInForce> parse_tif(std::string_view str);
};

} // namespace hft
