# Architecture

## System Overview

The HFT Exchange is a high-performance, deterministic matching engine designed for ultra-low latency order processing.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         HFT Exchange                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐         ┌─────────────────┐                  │
│  │ Feed Handler │────────▶│ SPSC Ring Buffer│                  │
│  │  (Thread 1)  │         │   (Lock-Free)   │                  │
│  └──────────────┘         └────────┬────────┘                  │
│         │                           │                            │
│         │ Parse JSON/Binary         ▼                            │
│         │                  ┌─────────────────┐                  │
│         │                  │ Matching Engine │                  │
│         │                  │   (Thread 2)    │                  │
│         │                  │  Single-Threaded│                  │
│         │                  └────────┬────────┘                  │
│         │                           │                            │
│         │                           │ Trades/Acks                │
│         │                           ▼                            │
│         │                  ┌─────────────────┐                  │
│         │                  │   Publisher     │                  │
│         │                  │  (WebSocket)    │                  │
│         │                  └─────────────────┘                  │
│         │                           │                            │
│         │                           ▼                            │
│         │                  ┌─────────────────┐                  │
│         └─────────────────▶│  WAL Writer     │                  │
│                            │   (Thread 3)    │                  │
│                            └────────┬────────┘                  │
│                                     │                            │
│                                     ▼                            │
│                            ┌─────────────────┐                  │
│                            │   Snapshot      │                  │
│                            │  (Periodic)     │                  │
│                            └─────────────────┘                  │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

## Threading Model

### Thread 1: Feed Ingress
- Receives market data from WebSocket/TCP
- Parses JSON/binary messages
- Validates and normalizes events
- Pushes to SPSC ring buffer
- Non-blocking, drops on overflow

### Thread 2: Matching Engine (Critical Path)
- Single-threaded for determinism
- Pops events from ring buffer
- Maintains order books per symbol
- Executes matching algorithm
- Emits trades and acknowledgments
- Zero dynamic allocation in hot path

### Thread 3: Persistence
- Consumes events from separate queue
- Appends to write-ahead log
- Batches fsync for performance
- Creates periodic snapshots

### Thread 4: HTTP API
- Serves metrics and health endpoints
- Non-blocking I/O
- Does not interfere with critical path

## SPSC Queue Design

Lock-free single-producer single-consumer ring buffer:

```
Producer (Feed)          Consumer (Engine)
     │                         │
     ▼                         ▼
┌────────────────────────────────┐
│  [E1][E2][E3][  ][  ][  ][  ] │
│    ^write         ^read        │
└────────────────────────────────┘
```

- Power-of-2 capacity for fast modulo
- Cache-line aligned atomics
- Memory ordering: acquire/release
- No locks, no syscalls

## Data Flow

1. **Ingress**: Feed → Parser → Validation → Ring Buffer
2. **Matching**: Ring Buffer → Engine → Order Book → Trades
3. **Egress**: Trades → Publisher → WebSocket Clients
4. **Persistence**: Events → WAL → Disk (async)

## Memory Layout

### Order Book Structure

```
OrderBook
├── bids_: map<Price, PriceLevel> (descending)
├── asks_: map<Price, PriceLevel> (ascending)
└── orders_: unordered_map<OrderId, OrderNode*>

PriceLevel
├── price_: Price
├── head_: OrderNode* (FIFO queue)
├── tail_: OrderNode*
└── total_qty_: Quantity

OrderNode (64 bytes, cache-friendly)
├── order: Order (48 bytes)
├── next: OrderNode*
└── prev: OrderNode*
```

### Memory Allocation

- Order nodes: PMR monotonic buffer (1MB pool)
- No allocation in matching loop
- Periodic pool reset on snapshot

## Persistence Model

### Write-Ahead Log (WAL)

Binary format:
```
[Header: 16 bytes]
  - magic: 0x57414C46
  - version: 1
  - entry_count: uint64

[Entry: variable]
  - type: EventType (1 byte)
  - sequence: uint64
  - timestamp: uint64
  - payload_size: uint32
  - payload: bytes
```

### Snapshot Format

```
[Header: 32 bytes]
  - magic: 0x534E4150
  - version: 1
  - timestamp: uint64
  - checksum: uint64 (FNV-1a)
  - data_size: uint64

[Data: variable]
  - Serialized order books
  - Active orders
  - Price levels
```

### Recovery Sequence

1. Load latest snapshot (if exists)
2. Replay WAL from snapshot offset
3. Rebuild order book state
4. Resume processing

## Performance Characteristics

- **Latency**: p50 < 500ns, p99 < 2µs
- **Throughput**: > 1M orders/sec (single-threaded)
- **Memory**: < 100MB for 10K active orders
- **Determinism**: Bit-identical replay from WAL

## Scalability

- Horizontal: Multiple symbols on separate engines
- Vertical: NUMA-aware thread pinning
- Sharding: Symbol-based partitioning
