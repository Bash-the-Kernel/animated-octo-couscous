#!/bin/bash

set -e

echo "Starting Local Market Data Feed Simulator"
echo "=========================================="

# Simple WebSocket server that sends test orders
# In production, this would connect to a real exchange

PORT=${1:-9001}

echo "Listening on port $PORT"
echo "Sending test market data..."

# This is a placeholder - in production would use a proper WebSocket server
# For now, the feed handler has built-in simulation

while true; do
    echo "$(date): Simulating market data..."
    sleep 5
done
