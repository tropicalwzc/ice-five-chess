#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUTPUT_DIR="${TMPDIR:-/tmp}/five-star-fork-replay"
mkdir -p "$OUTPUT_DIR"

clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic -pthread \
  "$ROOT_DIR/ice five chess/FiveChessAI.c" \
  "$ROOT_DIR/tools/five_star_fork_fixture_runner.c" \
  -lm -o "$OUTPUT_DIR/five_star_fork_fixture_runner"

python3 "$ROOT_DIR/tools/replay_five_star_fork_recovery.py" \
  --manifest "$ROOT_DIR/tools/five_star_fork_recovery_fixtures.json" \
  --runner "$OUTPUT_DIR/five_star_fork_fixture_runner" \
  --output-dir "$ROOT_DIR/reports/five_chess/five_star_fork_recovery_$(date +%Y%m%d)" \
  "$@"
