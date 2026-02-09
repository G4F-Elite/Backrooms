#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build/linux"

mkdir -p "$BUILD_DIR"
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  -I"$ROOT_DIR/shared" \
  "$ROOT_DIR/master/src/main.cpp" \
  -o "$BUILD_DIR/backrooms_master"

echo "Built: $BUILD_DIR/backrooms_master"
