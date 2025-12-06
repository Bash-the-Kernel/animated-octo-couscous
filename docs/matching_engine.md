# Matching Engine

## Overview

The matching engine is the core component responsible for order processing and trade execution. It operates deterministically in a single thread to ensure reproducible behavior.

## Order Lifecycle

```
NEW_ORDER
    │
    ▼
┌─────────┐
│ PENDING │
└────┬────┘
     │
     ▼
  Validate
     │
     ├─── Invalid ──▶ REJECTED
     │
     ▼ Valid
  Match Against Book
     │
     ├─── No Match ──▶ ACCEPTED (added to book)
     │
     ├─── Partial ──▶ PARTIALLY_FILLED
     │
     └─── Full ────▶ FILLED

CANCEL_ORDER
    │
    ▼
  Find Order
    │
    ├─── Not Found ──▶ REJECTED
    │
    └─── Found ──────▶ CANCELLED (removed from book)

MODIFY_ORDER
    │
    ▼
  Cancel + Re-Add (loses time priority)
```

## Matching Algorithm

### Price-Time Priority

Orders are matched based on:
1. **Price**: Best price gets priority
2. **Time**: Earlier orders at same price get priority

### Pseudocode

```
function match_limit_order(aggressor_order):
    passive_side = (aggressor.side == BUY) ? asks : bids
    trades = []
    
    while aggressor.remaining > 0 and not passive_side.empty():
        best_level = passive_side.first()
        
        if not price_crosses(aggressor.price, best_level.price, aggressor.side):
            break
        
        passive_order = best_level.head()
        match_qty = min(aggressor.remaining, passive_order.remaining)
        
        trade = create_trade(aggressor, passive_order, match_qty)
        trades.append(trade)
        
        aggressor.filled += match_qty
        passive_order.filled += match_qty
        
        if passive_order.remaining == 0:
            remove_order(passive_order)
            if best_level.empty():
                passive_side.remove(best_level)
    
    if aggressor.remaining > 0 and aggressor.tif == GTC:
        add_to_book(aggressor)
    
    return trades
```

## Time-In-Force Behaviors

### GTC (Good-Till-Cancel)
- Remains in book until filled or explicitly cancelled
- Default behavior
- Partial fills allowed

```
Order: BUY 100 @ $50,000 GTC
Book: SELL 50 @ $50,000

Result:
  - Trade: 50 @ $50,000
  - Remaining: BUY 50 @ $50,000 (stays in book)
```

### IOC (Immediate-Or-Cancel)
- Match immediately against available liquidity
- Cancel unfilled portion
- No resting in book

```
Order: BUY 100 @ $50,000 IOC
Book: SELL 50 @ $50,000

Result:
  - Trade: 50 @ $50,000
  - Remaining: 50 cancelled
```

### FOK (Fill-Or-Kill)
- All-or-nothing execution
- If full quantity not available, reject entire order
- No partial fills

```
Order: BUY 100 @ $50,000 FOK
Book: SELL 50 @ $50,000

Result:
  - No trades
  - Order rejected
```

## Order Types

### LIMIT
- Specified price
- Will not execute at worse price
- May rest in book

### MARKET
- No price specified
- Executes at best available price
- Sweeps multiple levels if needed
- Unfilled portion cancelled

## Edge Cases

### Self-Trade Prevention
Not implemented in basic version. Production systems would:
- Check if buyer_id == seller_id
- Cancel or reject one side

### Minimum Quantity
Not implemented. Could add:
- Minimum fill quantity
- Display vs. hidden quantity

### Iceberg Orders
Not implemented. Would require:
- Visible quantity
- Hidden reserve quantity
- Replenishment logic

### Stop Orders
Not implemented. Would require:
- Trigger price monitoring
- Conversion to market/limit on trigger

## Matching Performance

### Hot Path Optimizations

1. **No Dynamic Allocation**
   - Pre-allocated memory pool
   - Fixed-size structures
   - Stack-based temporaries

2. **Branch Reduction**
   - Branchless min/max where possible
   - Predictable control flow
   - Avoid virtual dispatch

3. **Cache Efficiency**
   - Contiguous order nodes
   - 64-byte alignment
   - Sequential access patterns

4. **Lock-Free Input**
   - SPSC ring buffer
   - No mutex in critical path
   - Memory barriers only

### Latency Breakdown

```
Total: ~500ns (p50)
├── Queue pop: ~50ns
├── Event dispatch: ~20ns
├── Order validation: ~30ns
├── Book lookup: ~50ns
├── Matching loop: ~200ns
├── Trade emission: ~100ns
└── Callback: ~50ns
```

## Determinism Guarantees

### Requirements
- Same input sequence → same output
- Reproducible from WAL replay
- No non-deterministic sources:
  - No random numbers
  - No system time in logic (only for logging)
  - No thread races
  - No uninitialized memory

### Verification
- Hash order book state periodically
- Compare replay hash with original
- Detect any divergence immediately

## Correctness Invariants

1. **Conservation**: Total quantity in = total quantity out
2. **Price-Time**: Earlier orders at same price match first
3. **No Negative**: Quantities and prices always >= 0
4. **Atomicity**: Order either fully processed or not at all
5. **Consistency**: Book state always valid after each operation
