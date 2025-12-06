#pragma once

#include "../common/types.hpp"

namespace hft {

struct OrderNode {
    Order order;
    OrderNode* next{nullptr};
    OrderNode* prev{nullptr};
};

} // namespace hft
