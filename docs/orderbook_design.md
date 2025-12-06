# Order Book Design

## Data Structure

The order book maintains separate bid and ask sides with price-time priority.

## Structure Hierarchy

```
OrderBook
│
├── Symbol: "BTCUSD"
│
├── Bids (std::map, descending)
│   ├── Price: $50,000
│   │   └── PriceLevel
│   │       ├── head → OrderNode → OrderNode → OrderNode → NULL
│   │       ├── tail ↑
│   │       └── total_qty: 300
│   │
│   ├── Price: $49,999
│   │   └── PriceLevel (...)
│   │
│   └── Price: $49,998
│       └── PriceLevel (...)
│
├── Asks (std::map, ascending)
│   ├── Price: $50,001
│   │   └── PriceLevel (...)
│   │
│   ├── Price: $50,002
│   │   └── PriceLevel (...)
│   │
│   └── Price: $50,003
│       └── PriceLevel (...)
│
└── Orders (unordered_map)
    ├── OrderId: 123 → OrderNode*
    ├── OrderId: 124 → OrderNode*
    └── OrderId: 125 → OrderNode*
```

## Price Level Implementation

Each price level maintains a FIFO queue of orders:

```cpp
class PriceLevel {
    Price price_;
    OrderNode* head_;  // First order (oldest)
    OrderNode* tail_;  // Last order (newest)
    Quantity total_qty_;
};
```

### Operations

**Add Order**: O(1)
```
Add to tail of linked list
Update total_qty
```

**Remove Order**: O(1)
```
Unlink from doubly-linked list
Update total_qty
If empty, remove price level
```

**Match**: O(1) per order
```
Start from head (oldest)
Match in FIFO order
Remove filled orders
```

## Order Node Layout

```
OrderNode (64 bytes, cache-line aligned)
┌─────────────────────────────────────┐
│ Order (48 bytes)                    │
│  ├── id: 8 bytes                    │
│  ├── symbol: 17 bytes               │
│  ├── side: 1 byte                   │
│  ├── type: 1 byte                   │
│  ├── tif: 1 byte                    │
│  ├── price: 8 bytes                 │
│  ├── quantity: 8 bytes              │
│  ├── filled: 8 bytes                │
│  ├── timestamp: 8 bytes             │
│  └── status: 1 byte                 │
├─────────────────────────────────────┤
│ next: 8 bytes                       │
│ prev: 8 bytes                       │
└─────────────────────────────────────┘
```

## Memory Management

### PMR Allocator

Uses `std::pmr::monotonic_buffer_resource`:

```cpp
std::pmr::monotonic_buffer_resource pool_{1024 * 1024};
std::pmr::polymorphic_allocator<OrderNode> allocator_{&pool_};
```

**Benefits**:
- Fast allocation (bump pointer)
- No per-object deallocation overhead
- Bulk release on snapshot
- Cache-friendly contiguous memory

**Lifecycle**:
1. Allocate nodes from pool
2. Use until snapshot
3. Release entire pool
4. Rebuild from snapshot

## Complexity Analysis

| Operation | Time | Space |
|-----------|------|-------|
| Add Order | O(log P) | O(1) |
| Cancel Order | O(log P) | O(1) |
| Modify Order | O(log P) | O(1) |
| Match Order | O(M + log P) | O(M) |
| Best Bid/Ask | O(1) | O(1) |

Where:
- P = number of price levels
- M = number of matches

## Price Representation

Fixed-point arithmetic for exact decimal representation:

```cpp
using Price = int64_t;  // price * 10000

// Example: $50,000.1234
Price price = 500001234;

// Convert to double for display
double display = static_cast<double>(price) / 10000.0;
```

**Why Fixed-Point?**
- No floating-point rounding errors
- Exact comparisons
- Deterministic behavior
- Fast integer arithmetic

## Bid/Ask Ordering

### Bids (Buy Orders)
- Sorted descending by price
- Higher prices have priority
- `std::map<Price, PriceLevel, std::greater<Price>>`

```
Best Bid (top)
$50,000 ← Highest price
$49,999
$49,998
$49,997
...
```

### Asks (Sell Orders)
- Sorted ascending by price
- Lower prices have priority
- `std::map<Price, PriceLevel, std::less<Price>>`

```
Best Ask (top)
$50,001 ← Lowest price
$50,002
$50,003
$50,004
...
```

## Matching Logic

### Price Crossing

**Buy Order Crosses** when:
```
buy_price >= best_ask_price
```

**Sell Order Crosses** when:
```
sell_price <= best_bid_price
```

### Execution Price

Always use the **passive order's price** (maker price):
- Rewards liquidity providers
- Standard exchange behavior
- Price improvement for aggressor

## Example Scenarios

### Scenario 1: Full Match

```
Book State:
  Bids: 100 @ $50,000
  Asks: 50 @ $50,001

New Order: SELL 50 @ $50,000

Result:
  Trade: 50 @ $50,000
  Remaining Bids: 50 @ $50,000
```

### Scenario 2: Partial Match

```
Book State:
  Bids: 50 @ $50,000
  Asks: empty

New Order: SELL 100 @ $50,000

Result:
  Trade: 50 @ $50,000
  Remaining Asks: 50 @ $50,000 (added to book)
```

### Scenario 3: Multi-Level Match

```
Book State:
  Bids: 
    50 @ $50,000
    30 @ $49,999
    20 @ $49,998

New Order: SELL 100 @ $49,998 (market sweep)

Result:
  Trade 1: 50 @ $50,000
  Trade 2: 30 @ $49,999
  Trade 3: 20 @ $49,998
  Total: 100 filled
```

### Scenario 4: No Match

```
Book State:
  Bids: 100 @ $50,000
  Asks: 50 @ $50,002

New Order: BUY 50 @ $50,001

Result:
  No trades (no crossing)
  New Bids: 
    50 @ $50,001 (new)
    100 @ $50,000
```

## Performance Optimizations

### 1. Avoid Map Lookups
- Cache best bid/ask pointers
- Update on level changes only

### 2. Minimize Allocations
- Pre-allocate node pool
- Reuse nodes where possible

### 3. Branch Prediction
- Most common path: no match
- Optimize for add-to-book case

### 4. Cache Locality
- Sequential node traversal
- Contiguous memory layout
- Prefetch next node

## Thread Safety

**Single-Threaded Design**:
- No locks required
- No atomic operations
- Deterministic execution
- Maximum performance

**Multi-Symbol Scaling**:
- One order book per symbol
- Each on separate thread
- No shared state
- Linear scalability
