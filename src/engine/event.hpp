#pragma once

#include "../common/types.hpp"
#include <variant>

namespace hft {

struct NewOrderEvent {
    Order order;
};

struct CancelOrderEvent {
    OrderId order_id;
    Symbol symbol;
};

struct ModifyOrderEvent {
    OrderId order_id;
    Symbol symbol;
    Price new_price;
    Quantity new_quantity;
};

using EngineEvent = std::variant<NewOrderEvent, CancelOrderEvent, ModifyOrderEvent>;

} // namespace hft
