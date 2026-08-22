#!/bin/sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_path=${1:-"${TMPDIR:-/tmp}/five-chess-benchmark/libfive_chess_research.dylib"}
mkdir -p "$(dirname -- "$output_path")"

clang -fobjc-arc -dynamiclib -O2 -w -framework Foundation \
  "$repo_root/ice five chess/FiveChessAI.c" \
  "$repo_root/ice five chess/doublethree.m" \
  -lm -o "$output_path"
printf '%s\n' "$output_path"
