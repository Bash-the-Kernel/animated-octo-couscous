# Quick Start Guide

Get the HFT Exchange running in 5 minutes.

## Prerequisites

- C++20 compiler (GCC 10+, Clang 12+, or MSVC 2022+)
- CMake 3.20+
- Node.js 16+ (for UI)

## 1. Build the Exchange

```bash
# Clone and enter directory
cd animated-octo-couscous

# Build C++ project
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
cd ..
```

## 2. Run Tests (Optional)

```bash
cd build
ctest --output-on-failure
cd ..
```

## 3. Start the Exchange

```bash
# Create data directory
mkdir -p data

# Run exchange
./build/hft_exchange config.json
```

You should see:
```
[INFO] HFT Exchange starting...
[INFO] WAL opened: data/wal.bin, entries: 0
[INFO] Matching engine started
[INFO] Feed handler started: ws://localhost:9001
[INFO] HTTP server starting on port: 8080
[INFO] HFT Exchange running. Press Ctrl+C to stop.
```

## 4. Verify It's Running

Open another terminal:

```bash
# Check health
curl http://localhost:8080/healthz
# Output: OK

# Check metrics
curl http://localhost:8080/metrics
# Output: Prometheus metrics

# Check order count
curl http://localhost:8080/orders/count
# Output: {"processed":0}
```

## 5. Start the UI (Optional)

```bash
cd ui
npm install
npm run dev
```

Open browser to `http://localhost:3000`

## 6. Send Test Orders

Create a test script `test_orders.sh`:

```bash
#!/bin/bash

# Send buy order
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{
    "type": "new_order",
    "symbol": "BTCUSD",
    "side": "buy",
    "order_type": "limit",
    "price": 50000.0,
    "quantity": 100,
    "tif": "gtc"
  }'

# Send sell order (will match)
curl -X POST http://localhost:8080/orders \
  -H "Content-Type: application/json" \
  -d '{
    "type": "new_order",
    "symbol": "BTCUSD",
    "side": "sell",
    "order_type": "limit",
    "price": 50000.0,
    "quantity": 50,
    "tif": "gtc"
  }'
```

## 7. Run Benchmarks

```bash
cd build

# Order book benchmarks
./orderbook_bench

# Engine benchmarks
./engine_bench
```

Expected results:
```
BM_AddOrder         500 ns
BM_MatchOrder      2000 ns
BM_CancelOrder      800 ns
```

## Docker Quick Start

```bash
# Build image
docker build -f docker/Dockerfile -t hft-exchange .

# Run container
docker run -p 8080:8080 -p 9002:9002 hft-exchange

# Or use docker-compose
cd docker
docker-compose up
```

## Kubernetes Quick Start

```bash
# Apply manifests
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# Check status
kubectl get pods
kubectl logs -f deployment/hft-exchange

# Port forward
kubectl port-forward service/hft-exchange 8080:8080
```

## Troubleshooting

### Build fails
- Ensure C++20 compiler installed
- Check CMake version: `cmake --version`
- Install dependencies: `sudo apt-get install build-essential cmake`

### Port already in use
Edit `config.json`:
```json
{
  "http_port": 8081,
  "ws_port": 9003
}
```

### Permission denied on data directory
```bash
mkdir -p data
chmod 755 data
```

## Next Steps

- Read [Architecture](docs/architecture.md) for system design
- Read [Matching Engine](docs/matching_engine.md) for algorithm details
- Explore [Tests](tests/) for usage examples
- Check [BUILD.md](BUILD.md) for advanced build options

## Performance Tips

For maximum performance:

```bash
# Use Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Pin to CPU core
taskset -c 0 ./hft_exchange config.json

# Use huge pages
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages
```

## Support

- Documentation: `docs/`
- Examples: `tests/`
- Issues: GitHub Issues

Happy trading! 🚀
