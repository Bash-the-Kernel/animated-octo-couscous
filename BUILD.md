# Build Instructions

## Prerequisites

### Linux/macOS
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git ninja-build libssl-dev

# macOS
brew install cmake ninja openssl
```

### Windows
- Visual Studio 2022 with C++ tools
- CMake 3.20+
- Git

## Build Steps

### 1. Clone Repository
```bash
git clone <repository-url>
cd animated-octo-couscous
```

### 2. Build C++ Project
```bash
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### 3. Run Tests
```bash
ctest --output-on-failure
```

### 4. Run Benchmarks
```bash
./orderbook_bench
./engine_bench
```

### 5. Build UI
```bash
cd ../ui
npm install
npm run build
```

## Running the Exchange

### Standalone
```bash
# Create data directory
mkdir -p data

# Run exchange
./build/hft_exchange config.json
```

### Docker
```bash
# Build image
docker build -f docker/Dockerfile -t hft-exchange .

# Run container
docker run -p 8080:8080 -p 9002:9002 -v $(pwd)/data:/app/data hft-exchange
```

### Docker Compose
```bash
cd docker
docker-compose up -d
```

### Kubernetes
```bash
# Apply manifests
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# Check status
kubectl get pods
kubectl logs -f <pod-name>
```

## Development

### Run UI in Dev Mode
```bash
cd ui
npm run dev
# Open http://localhost:3000
```

### Run Fuzz Tests (Clang only)
```bash
cd build
./fuzz_parser corpus/ -max_total_time=60
```

### Generate Coverage Report
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ..
cmake --build .
ctest
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Performance Tuning

### CPU Affinity
```bash
taskset -c 0 ./hft_exchange config.json
```

### Huge Pages
```bash
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages
```

### Disable Turbo Boost (for consistent benchmarks)
```bash
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```

## Troubleshooting

### Build Errors
- Ensure C++20 compiler (GCC 10+, Clang 12+, MSVC 2022+)
- Check CMake version >= 3.20
- Verify all dependencies installed

### Runtime Errors
- Check data directory permissions
- Verify ports 8080, 9002 available
- Review logs in console output

### Performance Issues
- Use Release build
- Enable compiler optimizations (-O3, -march=native)
- Pin to dedicated CPU cores
- Use fast storage (NVMe) for WAL
