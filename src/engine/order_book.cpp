#include "order_book.hpp"
#include "../utils/time.hpp"
#include <algorithm>

namespace hft {

// PriceLevel implementation
void PriceLevel::add_order(OrderNode* node) {
    if (!head_) {
        head_ = tail_ = node;
        node->next = node->prev = nullptr;
    } else {
        tail_->next = node;
        node->prev = tail_;
        node->next = nullptr;
        tail_ = node;
    }
    total_qty_ += node->order.remaining();
}

void PriceLevel::remove_order(OrderNode* node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        head_ = node->next;
    }
    
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        tail_ = node->prev;
    }
    
    total_qty_ -= node->order.remaining();
}

// OrderBook implementation
OrderBook::OrderBook(Symbol symbol) : symbol_(symbol) {}

OrderBook::~OrderBook() {
    clear();
}

void OrderBook::add_order(const Order& order) {
    OrderNode* node = allocate_node(order);
    orders_[order.id] = node;
    
    if (order.side == Side::BUY) {
        auto [it, inserted] = bids_.try_emplace(order.price, order.price);
        it->second.add_order(node);
    } else {
        auto [it, inserted] = asks_.try_emplace(order.price, order.price);
        it->second.add_order(node);
    }
}

bool OrderBook::cancel_order(OrderId order_id) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return false;
    
    OrderNode* node = it->second;
    Order& order = node->order;
    
    if (order.side == Side::BUY) {
        auto level_it = bids_.find(order.price);
        if (level_it != bids_.end()) {
            level_it->second.remove_order(node);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        }
    } else {
        auto level_it = asks_.find(order.price);
        if (level_it != asks_.end()) {
            level_it->second.remove_order(node);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
    }
    
    orders_.erase(it);
    deallocate_node(node);
    return true;
}


bool OrderBook::modify_order(OrderId order_id, Price new_price, Quantity new_quantity) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) return false;
    
    OrderNode* node = it->second;
    Order old_order = node->order;
    
    // Cancel and re-add (loses time priority)
    cancel_order(order_id);
    
    Order new_order = old_order;
    new_order.price = new_price;
    new_order.quantity = new_quantity;
    new_order.filled = 0;
    
    add_order(new_order);
    return true;
}

std::vector<Trade> OrderBook::match_order(Order& order) {
    if (order.type == OrderType::MARKET) {
        return match_market_order(order);
    }
    return match_limit_order(order);
}

std::vector<Trade> OrderBook::match_market_order(Order& order) {
    if (order.side == Side::BUY) {
        return execute_matches(order, asks_);
    } else {
        return execute_matches(order, bids_);
    }
}

std::vector<Trade> OrderBook::match_limit_order(Order& order) {
    std::vector<Trade> trades;
    
    if (order.side == Side::BUY) {
        while (order.remaining() > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            Price price = it->first;
            PriceLevel& level = it->second;
            
            if (order.price < price) break;
            
            OrderNode* passive_node = level.head();
            if (!passive_node) break;
            
            Order& passive_order = passive_node->order;
            Quantity match_qty = std::min(order.remaining(), passive_order.remaining());
            
            Trade trade{
                .buyer_id = order.id,
                .seller_id = passive_order.id,
                .symbol = symbol_,
                .price = passive_order.price,
                .quantity = match_qty,
                .timestamp = now_ns(),
                .buyer_is_aggressor = true
            };
            trades.push_back(trade);
            
            order.filled += match_qty;
            passive_order.filled += match_qty;
            
            if (passive_order.remaining() == 0) {
                level.remove_order(passive_node);
                orders_.erase(passive_order.id);
                deallocate_node(passive_node);
                
                if (level.empty()) {
                    asks_.erase(it);
                }
            }
        }
    } else {
        while (order.remaining() > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            Price price = it->first;
            PriceLevel& level = it->second;
            
            if (order.price > price) break;
            
            OrderNode* passive_node = level.head();
            if (!passive_node) break;
            
            Order& passive_order = passive_node->order;
            Quantity match_qty = std::min(order.remaining(), passive_order.remaining());
            
            Trade trade{
                .buyer_id = passive_order.id,
                .seller_id = order.id,
                .symbol = symbol_,
                .price = passive_order.price,
                .quantity = match_qty,
                .timestamp = now_ns(),
                .buyer_is_aggressor = false
            };
            trades.push_back(trade);
            
            order.filled += match_qty;
            passive_order.filled += match_qty;
            
            if (passive_order.remaining() == 0) {
                level.remove_order(passive_node);
                orders_.erase(passive_order.id);
                deallocate_node(passive_node);
                
                if (level.empty()) {
                    bids_.erase(it);
                }
            }
        }
    }
    
    // Handle time-in-force
    if (order.tif == TimeInForce::IOC || order.tif == TimeInForce::FOK) {
        if (order.tif == TimeInForce::FOK && order.remaining() > 0) {
            // Rollback trades for FOK
            trades.clear();
            order.filled = 0;
            order.status = OrderStatus::REJECTED;
        } else {
            order.status = (order.remaining() == 0) ? OrderStatus::FILLED : OrderStatus::CANCELLED;
        }
    } else {
        // GTC: add remaining to book
        if (order.remaining() > 0) {
            add_order(order);
            order.status = (order.filled > 0) ? OrderStatus::PARTIALLY_FILLED : OrderStatus::ACCEPTED;
        } else {
            order.status = OrderStatus::FILLED;
        }
    }
    
    return trades;
}

template<typename Map>
std::vector<Trade> OrderBook::execute_matches(Order& aggressor, Map& passive_side) {
    std::vector<Trade> trades;
    
    while (aggressor.remaining() > 0 && !passive_side.empty()) {
        auto& [price, level] = *passive_side.begin();
        OrderNode* passive_node = level.head();
        if (!passive_node) break;
        
        Order& passive_order = passive_node->order;
        Quantity match_qty = std::min(aggressor.remaining(), passive_order.remaining());
        
        Trade trade{
            .buyer_id = (aggressor.side == Side::BUY) ? aggressor.id : passive_order.id,
            .seller_id = (aggressor.side == Side::SELL) ? aggressor.id : passive_order.id,
            .symbol = symbol_,
            .price = passive_order.price,
            .quantity = match_qty,
            .timestamp = now_ns(),
            .buyer_is_aggressor = (aggressor.side == Side::BUY)
        };
        trades.push_back(trade);
        
        aggressor.filled += match_qty;
        passive_order.filled += match_qty;
        
        if (passive_order.remaining() == 0) {
            level.remove_order(passive_node);
            orders_.erase(passive_order.id);
            deallocate_node(passive_node);
            
            if (level.empty()) {
                passive_side.erase(passive_side.begin());
            }
        }
    }
    
    aggressor.status = (aggressor.remaining() == 0) ? OrderStatus::FILLED : OrderStatus::PARTIALLY_FILLED;
    return trades;
}

const Order* OrderBook::find_order(OrderId order_id) const {
    auto it = orders_.find(order_id);
    return (it != orders_.end()) ? &it->second->order : nullptr;
}

Price OrderBook::best_bid() const {
    return bids_.empty() ? 0 : bids_.begin()->first;
}

Price OrderBook::best_ask() const {
    return asks_.empty() ? 0 : asks_.begin()->first;
}

void OrderBook::clear() {
    orders_.clear();
    bids_.clear();
    asks_.clear();
    pool_.release();
}

OrderNode* OrderBook::allocate_node(const Order& order) {
    OrderNode* node = allocator_.allocate(1);
    allocator_.construct(node, OrderNode{order, nullptr, nullptr});
    return node;
}

void OrderBook::deallocate_node(OrderNode* node) {
    allocator_.destroy(node);
    allocator_.deallocate(node, 1);
}

} // namespace hft
