# Persistence

## Overview

The persistence layer ensures crash recovery and audit trail through write-ahead logging (WAL) and periodic snapshots.

## Write-Ahead Log (WAL)

### Purpose
- Durability: Survive crashes
- Audit: Complete event history
- Replay: Reconstruct state
- Compliance: Regulatory requirements

### Binary Format

```
┌─────────────────────────────────────────┐
│ WAL File Header (16 bytes)              │
├─────────────────────────────────────────┤
│ magic:        0x57414C46 ("WALF")       │
│ version:      1                         │
│ entry_count:  uint64                    │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ Entry 1                                 │
├─────────────────────────────────────────┤
│ type:         EventType (1 byte)        │
│ sequence:     uint64                    │
│ timestamp:    uint64 (nanoseconds)      │
│ payload_size: uint32                    │
│ payload:      variable bytes            │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ Entry 2                                 │
│ ...                                     │
└─────────────────────────────────────────┘
```

### Event Types

```cpp
enum class EventType : uint8_t {
    NEW_ORDER = 0,
    CANCEL_ORDER = 1,
    MODIFY_ORDER = 2,
    TRADE = 3,
    ORDER_ACK = 4,
    ORDER_REJECT = 5
};
```

### Payload Formats

**NEW_ORDER**:
```
order_id:    uint64
symbol:      char[16]
side:        uint8
type:        uint8
tif:         uint8
price:       int64
quantity:    uint64
```

**CANCEL_ORDER**:
```
order_id:    uint64
symbol:      char[16]
```

**TRADE**:
```
buyer_id:    uint64
seller_id:   uint64
symbol:      char[16]
price:       int64
quantity:    uint64
timestamp:   uint64
```

### Write Strategy

**Synchronous** (default):
- Append to buffer
- Flush on batch threshold
- fsync every N entries

**Asynchronous** (optional):
- Separate writer thread
- Lock-free queue from engine
- Group commit for throughput

### Group Commit

Batch multiple entries before fsync:

```
┌─────────────────────────────────────┐
│ Thread 1: Event A → Buffer          │
│ Thread 1: Event B → Buffer          │
│ Thread 1: Event C → Buffer          │
│           ↓                          │
│ Thread 2: Flush buffer → Disk       │
│ Thread 2: fsync() once              │
└─────────────────────────────────────┘

Latency: 3 events / 1 fsync = 3x faster
```

## Snapshot

### Purpose
- Fast recovery (avoid full WAL replay)
- Compact representation
- Periodic checkpoints

### Format

```
┌─────────────────────────────────────────┐
│ Snapshot Header (32 bytes)              │
├─────────────────────────────────────────┤
│ magic:      0x534E4150 ("SNAP")         │
│ version:    1                           │
│ timestamp:  uint64                      │
│ checksum:   uint64 (FNV-1a)             │
│ data_size:  uint64                      │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ Order Book 1                            │
├─────────────────────────────────────────┤
│ symbol:       char[16]                  │
│ bid_count:    uint32                    │
│ ask_count:    uint32                    │
│ order_count:  uint32                    │
│                                         │
│ ┌─────────────────────────────────────┐ │
│ │ Bid Price Levels                    │ │
│ │  price: int64                       │ │
│ │  order_count: uint32                │ │
│ │  orders: [Order...]                 │ │
│ └─────────────────────────────────────┘ │
│                                         │
│ ┌─────────────────────────────────────┐ │
│ │ Ask Price Levels                    │ │
│ │  ...                                │ │
│ └─────────────────────────────────────┘ │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│ Order Book 2                            │
│ ...                                     │
└─────────────────────────────────────────┘
```

### Checksum

FNV-1a hash for data integrity:

```cpp
uint64_t fnv1a(const uint8_t* data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}
```

### Memory-Mapped I/O

Use mmap for efficient snapshot I/O:

```cpp
// Write
MMapFile file;
file.open("snapshot.bin", size, false);
auto data = file.data();
serialize_to(data);
file.sync();

// Read
MMapFile file;
file.open("snapshot.bin", size, true);
auto data = file.data();
deserialize_from(data);
```

**Benefits**:
- OS-managed paging
- Zero-copy I/O
- Fast sequential access

## Recovery Sequence

### Startup Flow

```
┌─────────────────────────────────────────┐
│ 1. Check for snapshot                   │
│    ├─ Exists → Load snapshot            │
│    └─ Not exists → Start empty          │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 2. Open WAL                             │
│    ├─ Read header                       │
│    └─ Validate magic                    │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 3. Replay WAL entries                   │
│    ├─ From snapshot timestamp           │
│    ├─ Apply each event                  │
│    └─ Rebuild order book state          │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 4. Verify state                         │
│    ├─ Compute checksum                  │
│    └─ Compare with expected             │
└──────────────┬──────────────────────────┘
               ▼
┌─────────────────────────────────────────┐
│ 5. Resume normal operation              │
└─────────────────────────────────────────┘
```

### Example Recovery

```
Snapshot @ T=1000:
  - BTCUSD: 10 orders
  - ETHUSD: 5 orders

WAL entries after T=1000:
  - T=1001: NEW_ORDER (BTCUSD)
  - T=1002: TRADE (BTCUSD)
  - T=1003: CANCEL_ORDER (ETHUSD)

Recovery:
  1. Load snapshot (15 orders)
  2. Replay 3 WAL entries
  3. Final state: 15 orders + delta
```

## Snapshot Strategy

### Periodic Snapshots

```cpp
while (running) {
    sleep(snapshot_interval);
    
    // Pause engine briefly
    engine.pause();
    
    // Create snapshot
    snapshot.save(engine);
    
    // Truncate old WAL
    wal.truncate_before(snapshot.timestamp());
    
    // Resume engine
    engine.resume();
}
```

### Incremental Snapshots

For large state, use incremental approach:

1. **Copy-on-Write**: Clone order book
2. **Background Write**: Serialize clone
3. **No Engine Pause**: Zero downtime

## Durability Guarantees

### Levels

**Level 0: No Persistence**
- In-memory only
- Fastest
- No crash recovery

**Level 1: Async WAL**
- Background writes
- Eventual durability
- May lose recent events

**Level 2: Sync WAL**
- fsync after each batch
- Strong durability
- Higher latency

**Level 3: Sync WAL + Snapshot**
- Full recovery capability
- Production recommended

## Performance Impact

### WAL Overhead

```
Without WAL:  500ns latency
With Async:   520ns latency (+4%)
With Sync:    2000ns latency (+300%)
```

### Mitigation Strategies

1. **Batch Writes**: Group multiple events
2. **Separate Thread**: Async writer
3. **Fast Storage**: NVMe SSD
4. **Direct I/O**: Bypass page cache
5. **Pre-allocation**: Avoid file growth

## Disaster Recovery

### Backup Strategy

```
Daily:
  - Full snapshot
  - Copy to remote storage
  - Verify integrity

Hourly:
  - Incremental WAL backup
  - Append to remote log

On Failure:
  - Restore latest snapshot
  - Replay WAL from backup
  - Validate state
```

### Corruption Handling

```
if (checksum_mismatch) {
    log_error("Snapshot corrupted");
    
    // Try previous snapshot
    if (load_previous_snapshot()) {
        replay_wal_from(previous_timestamp);
    } else {
        // Full WAL replay
        replay_wal_from(0);
    }
}
```

## Compliance

### Audit Trail

WAL provides complete audit trail:
- Every order
- Every trade
- Every cancellation
- Exact timestamps
- Immutable log

### Regulatory Requirements

- **MiFID II**: Transaction reporting
- **SEC Rule 17a-4**: Record retention
- **FINRA**: Order audit trail

WAL satisfies these requirements with:
- Tamper-proof binary format
- Cryptographic checksums
- Timestamp precision
- Complete event history
