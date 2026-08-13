#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUTPUT_DIR="${TMPDIR:-/tmp}/five-chess-benchmark"
mkdir -p "$OUTPUT_DIR"

clang -fobjc-arc -O3 -w \
  -framework Foundation \
  "$ROOT_DIR/ice five chess/FiveChessAI.c" \
  "$ROOT_DIR/ice five chess/doublethree.m" \
  "$ROOT_DIR/tools/five_chess_benchmark.m" \
  -lm \
  -o "$OUTPUT_DIR/five_chess_benchmark"

printf '%s\n' "$OUTPUT_DIR/five_chess_benchmark"
