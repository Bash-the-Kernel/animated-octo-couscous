#!/bin/bash

set -e

echo "Running HFT Exchange Benchmarks"
echo "================================"

cd "$(dirname "$0")/.."

if [ ! -d "build" ]; then
    echo "Build directory not found. Building..."
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . -j$(nproc)
    cd ..
fi

echo ""
echo "Order Book Benchmarks"
echo "---------------------"
./build/orderbook_bench --benchmark_format=console

echo ""
echo "Matching Engine Benchmarks"
echo "--------------------------"
./build/engine_bench --benchmark_format=console

echo ""
echo "Benchmarks complete!"
