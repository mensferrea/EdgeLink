#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "=== EdgeLink System Demo ==="
./build/edgelink_gateway 9001 9002 &
GATEWAY_PID=$!

sleep 1

./build/edgelink_simulator 100 127.0.0.1 &
SIM_PID=$!

sleep 7

echo "Stopping simulator..."
kill -SIGINT $SIM_PID || true
wait $SIM_PID 2>/dev/null || true

sleep 1

echo "Stopping gateway..."
kill -SIGINT $GATEWAY_PID || true
wait $GATEWAY_PID 2>/dev/null || true

echo "=== Demo completed successfully ==="
