# 复现命令与环境

除特别说明外，命令均从仓库根目录执行。三组全部为本轮新跑，使用相同 `five-star-natural-final` schedule、黑方禁手规则与单进程串行执行。

## 环境

```text
macOS 26.5.2 (25F84), arm64
Apple clang 21.0.0 (clang-2100.1.1.101)
Python 3.13.5
Timezone: Asia/Shanghai
```

## Benchmark 构建

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
```

本次实际 binary 同时保存在 `provenance/five_chess_benchmark`。

## 三组禁手规则标准测试

共同口径：黑方禁手开启、deterministic-best、hybrid-deep-verified、seed `0x9ee4d91480ac5889`、opening 0–49、交换颜色、每局最多 120 手。

```sh
"$BENCHMARK_PATH" \
  --output raw/v581-vs-three-star-forbidden-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 1 --opponent legacy --paired-phase 0

"$BENCHMARK_PATH" \
  --output raw/v581-vs-four-star-forbidden-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 1 --opponent four-star --paired-phase 0

"$BENCHMARK_PATH" \
  --output raw/v581-vs-v541-forbidden-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 1 --opponent five-star-control --paired-phase 0
```

## 独立重放库

```sh
clang -dynamiclib -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  -I'ice five chess' \
  'ice five chess/FiveChessAI.c' \
  tools/opponent_guard_replay_probe.c -lm \
  -o /private/tmp/libfive_chess_v581_forbidden_detailed_replay.dylib
```

交付包内的同一库位于 `provenance/libfive_chess_v581_forbidden_detailed_replay.dylib`。重建时，`FiveChessAI.c/.h`、`FiveChessOpeningBook.inc` 和 `FiveChessEliteCorpus.inc` 均已保存在 `provenance/`。

## 三组禁手规则独立重放

以下命令从交付包根目录执行：

```sh
python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_forbidden_detailed_replay.dylib \
  --jsonl raw/v581-vs-three-star-forbidden-100g.jsonl \
  --output replay/v581-vs-three-star-forbidden-100g-replay.json

python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_forbidden_detailed_replay.dylib \
  --jsonl raw/v581-vs-four-star-forbidden-100g.jsonl \
  --output replay/v581-vs-four-star-forbidden-100g-replay.json

python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_forbidden_detailed_replay.dylib \
  --jsonl raw/v581-vs-v541-forbidden-100g.jsonl \
  --output replay/v581-vs-v541-forbidden-100g-replay.json
```

重放检查逐手禁手合法性、行棋方、配对开局前缀、终局胜方、调用方棋盘恢复和棋盘完整性，并从临时或最终 root 冷启动重新证明可重建的 early/final guard 证书。`legacy-illegal-move-loss` 会额外验证非法黑棋未被落盘、棋盘保持非终局且对手被正确判胜。

## 报告生成

以下命令从交付包根目录执行：

```sh
python3 provenance/generate_v581_detailed_report.py \
  --three raw/v581-vs-three-star-forbidden-100g.jsonl \
  --four raw/v581-vs-four-star-forbidden-100g.jsonl \
  --v541 raw/v581-vs-v541-forbidden-100g.jsonl \
  --three-replay replay/v581-vs-three-star-forbidden-100g-replay.json \
  --four-replay replay/v581-vs-four-star-forbidden-100g-replay.json \
  --v541-replay replay/v581-vs-v541-forbidden-100g-replay.json \
  --output-json report/summary.json \
  --output-md report/v581_forbidden_detailed_standard_report.md \
  --paired-csv report/paired_openings.csv \
  --profiles-json provenance/profiles.json
```

## 完整性校验

```sh
shasum -a 256 -c checksums.sha256
```

`provenance/profiles.json` 由三份原始 JSONL 的 header 直接生成，保存完整候选参数和各对手冻结身份；`report/summary.json` 同时保存每份 raw/replay 的 SHA-256。
