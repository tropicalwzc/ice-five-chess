#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUTPUT_DIR="${TMPDIR:-/tmp}/five-chess-ai-tests"
mkdir -p "$OUTPUT_DIR"

clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  "$ROOT_DIR/ice five chess/FiveChessAI.c" \
  "$ROOT_DIR/tools/five_chess_ai_tests.c" \
  -lm \
  -o "$OUTPUT_DIR/five_chess_ai_tests"

"$OUTPUT_DIR/five_chess_ai_tests"
