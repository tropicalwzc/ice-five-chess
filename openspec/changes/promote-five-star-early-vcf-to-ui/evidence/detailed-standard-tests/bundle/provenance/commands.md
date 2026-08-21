# 复现命令与环境

除特别说明外，命令均从仓库根目录执行。三组使用相同 `five-star-natural-final` schedule，单进程串行运行。三星为本轮新跑；四星和 exact 5.4.1 原始日志复用此前完全相同口径的标准测试证据，未重复消耗约 200 局计算时间。三组均用当前源码和同一重放工具重新验证。

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

## 三组标准测试

共同口径：Freestyle、deterministic-best、hybrid-deep-verified、seed `0x9ee4d91480ac5889`、opening 0–49、交换颜色、每局最多 120 手。

```sh
"$BENCHMARK_PATH" \
  --output v581-vs-three-star-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 0 --opponent legacy --paired-phase 0

"$BENCHMARK_PATH" \
  --output v581-vs-four-star-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 0 --opponent four-star --paired-phase 0

"$BENCHMARK_PATH" \
  --output v581-vs-v541-100g.jsonl \
  --profile five-star-early-vcf --suite five-star-natural-final \
  --random-mode best --strategy hybrid --seed 0x9ee4d91480ac5889 \
  --opening-start 0 --opening-count 50 --max-moves 120 \
  --forbidden-black 0 --opponent five-star-control --paired-phase 0
```

## 独立重放库

```sh
clang -dynamiclib -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  -I'ice five chess' \
  'ice five chess/FiveChessAI.c' \
  tools/opponent_guard_replay_probe.c -lm \
  -o /private/tmp/libfive_chess_v581_detailed_replay.dylib
```

交付包内的同一库位于 `provenance/libfive_chess_v581_detailed_replay.dylib`。

## 三组独立重放

以下命令从交付包根目录执行：

```sh
python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_detailed_replay.dylib \
  --jsonl raw/v581-vs-three-star-100g.jsonl \
  --output replay/v581-vs-three-star-100g-replay.json

python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_detailed_replay.dylib \
  --jsonl raw/v581-vs-four-star-100g.jsonl \
  --output replay/v581-vs-four-star-100g-replay.json

python3 provenance/replay_early_vcf_quick_match.py \
  --library provenance/libfive_chess_v581_detailed_replay.dylib \
  --jsonl raw/v581-vs-v541-100g.jsonl \
  --output replay/v581-vs-v541-100g-replay.json
```

重放检查逐手合法性、行棋方、配对开局前缀、终局胜方、调用方棋盘恢复和棋盘完整性，并从临时或最终 root 冷启动重新证明可重建的 early/final guard 证书。

## 报告生成

以下命令从交付包根目录执行：

```sh
python3 provenance/generate_v581_detailed_report.py \
  --three raw/v581-vs-three-star-100g.jsonl \
  --four raw/v581-vs-four-star-100g.jsonl \
  --v541 raw/v581-vs-v541-100g.jsonl \
  --three-replay replay/v581-vs-three-star-100g-replay.json \
  --four-replay replay/v581-vs-four-star-100g-replay.json \
  --v541-replay replay/v581-vs-v541-100g-replay.json \
  --output-json report/summary.json \
  --output-md report/v581_detailed_standard_report.md \
  --paired-csv report/paired_openings.csv \
  --profiles-json provenance/profiles.json
```

## 完整性校验

```sh
shasum -a 256 -c checksums.sha256
```

`provenance/profiles.json` 由三份原始 JSONL 的 header 直接生成，保存完整候选参数和各对手冻结身份。`report/summary.json` 同时保存每份 raw/replay 的 SHA-256。
