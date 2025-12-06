#include "parser.hpp"
#include <nlohmann/json.hpp>

namespace hft {

Parser::ParseResult Parser::parse_json(std::string_view json_str) {
    ParseResult result;
    
    try {
        auto j = nlohmann::json::parse(json_str);
        
        std::string type = j.value("type", "");
        
        if (type == "new_order") {
            NewOrderEvent event;
            event.order.id = j.value("order_id", 0ULL);
            event.order.symbol = Symbol(j.value("symbol", ""));
            
            auto side = parse_side(j.value("side", ""));
            if (!side) {
                result.error = "Invalid side";
                return result;
            }
            event.order.side = *side;
            
            auto order_type = parse_order_type(j.value("order_type", "limit"));
            if (!order_type) {
                result.error = "Invalid order type";
                return result;
            }
            event.order.type = *order_type;
            
            auto tif = parse_tif(j.value("tif", "gtc"));
            if (!tif) {
                result.error = "Invalid time in force";
                return result;
            }
            event.order.tif = *tif;
            
            double price_dbl = j.value("price", 0.0);
            event.order.price = static_cast<Price>(price_dbl * PRICE_SCALE);
            event.order.quantity = j.value("quantity", 0ULL);
            
            result.event = event;
            result.success = true;
            
        } else if (type == "cancel") {
            CancelOrderEvent event;
            event.order_id = j.value("order_id", 0ULL);
            event.symbol = Symbol(j.value("symbol", ""));
            
            result.event = event;
            result.success = true;
            
        } else if (type == "modify") {
            ModifyOrderEvent event;
            event.order_id = j.value("order_id", 0ULL);
            event.symbol = Symbol(j.value("symbol", ""));
            
            double price_dbl = j.value("price", 0.0);
            event.new_price = static_cast<Price>(price_dbl * PRICE_SCALE);
            event.new_quantity = j.value("quantity", 0ULL);
            
            result.event = event;
            result.success = true;
            
        } else {
            result.error = "Unknown message type";
        }
        
    } catch (const std::exception& e) {
        result.error = e.what();
    }
    
    return result;
}

Parser::ParseResult Parser::parse_binary(const uint8_t* data, size_t size) {
    ParseResult result;
    result.error = "Binary parsing not implemented";
    return result;
}

std::optional<Side> Parser::parse_side(std::string_view str) {
    if (str == "buy" || str == "BUY") return Side::BUY;
    if (str == "sell" || str == "SELL") return Side::SELL;
    return std::nullopt;
}

std::optional<OrderType> Parser::parse_order_type(std::string_view str) {
    if (str == "limit" || str == "LIMIT") return OrderType::LIMIT;
    if (str == "market" || str == "MARKET") return OrderType::MARKET;
    return std::nullopt;
}

std::optional<TimeInForce> Parser::parse_tif(std::string_view str) {
    if (str == "gtc" || str == "GTC") return TimeInForce::GTC;
    if (str == "ioc" || str == "IOC") return TimeInForce::IOC;
    if (str == "fok" || str == "FOK") return TimeInForce::FOK;
    return std::nullopt;
}

} // namespace hft
