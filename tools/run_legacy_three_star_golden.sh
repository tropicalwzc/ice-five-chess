#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUTPUT_DIR="${TMPDIR:-/tmp}/five-chess-ai-tests"
mkdir -p "$OUTPUT_DIR"

clang -fobjc-arc -O2 -w \
  -framework Foundation \
  "$ROOT_DIR/ice five chess/FiveChessAI.c" \
  "$ROOT_DIR/ice five chess/doublethree.m" \
  "$ROOT_DIR/tools/legacy_three_star_golden.m" \
  -lm \
  -o "$OUTPUT_DIR/legacy_three_star_golden"

"$OUTPUT_DIR/legacy_three_star_golden" "$@"
