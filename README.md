# HFT Exchange - High-Performance Market Data Feed Handler + Limit Order Book

A production-grade, high-frequency trading exchange implementation in modern C++20 featuring:
- Ultra-low latency matching engine (sub-microsecond)
- Lock-free SPSC queues for inter-thread communication
- Write-ahead logging with crash recovery
- Memory-mapped snapshots
- Real-time WebSocket market data publisher
- Comprehensive testing (unit, fuzz, property-based)
- Prometheus metrics and admin API

## Architecture

```
┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│ Feed Source │────▶│ Feed Handler │────▶│ SPSC Ring Buffer│
└─────────────┘     └──────────────┘     └────────┬────────┘
                                                   │
                                                   ▼
┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
│ WebSocket   │◀────│  Publisher   │◀────│ Matching Engine │
│  Clients    │     └──────────────┘     └────────┬────────┘
└─────────────┘                                    │
                                                   ▼
                    ┌──────────────┐     ┌─────────────────┐
                    │  Snapshot    │◀────│  WAL Writer     │
                    └──────────────┘     └─────────────────┘
```

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

## Run

```bash
# Start exchange
./hft_exchange --config config.json

# Run benchmarks
./orderbook_bench
./engine_bench

# Run tests
ctest --output-on-failure
```

## Performance

- Matching latency: p50 < 500ns, p99 < 2µs
- Throughput: > 1M orders/sec (single-threaded)
- Memory: < 100MB for 10K active orders

## Configuration

See `config.json` for runtime parameters:
- Feed endpoints
- WAL path
- Snapshot interval
- HTTP API port

## Monitoring

- Metrics: `http://localhost:8080/metrics`
- Health: `http://localhost:8080/healthz`
- UI: `http://localhost:3000`

## Documentation

- [Architecture](docs/architecture.md)
- [Matching Engine](docs/matching_engine.md)
- [Order Book Design](docs/orderbook_design.md)
- [Persistence](docs/persistence.md)

## License

MIT
