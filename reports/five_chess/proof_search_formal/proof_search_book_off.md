# 五子棋证明搜索新模型（开局库关闭）vs 旧三星

## 结论（白棋优先）

- 新模型执白：52/0/48，得分率 52.0%，Wilson 95% 42.3%–61.5%
- 同组旧三星执白（对应新模型执黑的 100 局）：32/1/67，得分率 32.5%，Wilson 95% 24.1%–42.2%
- 新模型白棋相对同组旧三星白棋：+19.5%
- 强度分类：`demonstrated stronger`

总体 Wilson 95% 区间高于 50%，且新模型白棋得分高于同组直接对局中的旧三星白棋，满足预先冻结的 `demonstrated stronger` 门槛。

新模型执黑为 67/1/32，得分率 67.5%，Wilson 95% 57.8%–75.9%；总体为 119/1/80，得分率 59.8%，Wilson 95% 52.8%–66.3%。报告保留五子棋自然先手优势，不做颜色再加权。

独立旧三星自对练 control 执白为 52/0/48，得分率 52.0%，Wilson 95% 42.3%–61.5%。该值只用于复现赛程与观察自然颜色基线，不参与上述白棋增量或强度分类。

## 赛程与版本

- 赛程：100 个固定 opening ID，每个交换颜色；新模型执黑 100 局、执白 100 局
- profile：`three-star-threat-proof-no-book@3.0.0-vcf-dfpn-no-book`
- 旧模型：`legacy-three-star@5224020`
- 开局模式：`gomocup-curated-prefix`；开局库：`gomocup-2024-2025-freestyle15-v2`
- 固定种子：`0xc0dec0de20260813`；域：`proof-final-proof-v2`
- 正式开局清单：`final-opening-schedule.json`，100/100 canonical ID 唯一，SHA-256 `232a0dbb03d56d924331c1d1b6ff404928d0cb8a1aa8a561efcfd7af686e27ff`
- 禁手：关闭（Freestyle-15）

## 搜索、开局与性能

- 新模型落子：2705；开局库命中：0
- 默认来源（none/legacy/book）：{'0': 0, '1': 2705, '2': 0}
- 覆盖原因（0..5）：{'0': 2139, '1': 119, '2': 172, '3': 134, '4': 141, '5': 0}
- 证明状态（unknown/proven/no-win-in-scope）：{'0': 1286, '1': 612, '2': 807}
- 已验证证书：612；证明节点：59947；预算耗尽：995
- 单步延迟：p50 190.79 ms，p95 460.98 ms，最大 1346.65 ms

所有 200 局均已逐手重放验证：无重复落点、越界、颜色顺序错误、终局后继续落子或结果不一致；所有被接受的 `proven-win` 均带运行时已验证证书标记。

## 复现命令

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
"$BENCHMARK_PATH" --profile proof-no-book --suite proof-final --random-mode best --opening-start 0 --opening-count 100 --max-moves 120 --forbidden-black 0 --output book-off.jsonl
```

## 数据与校验

- 原始数据：`proof_search_book_off_raw.json`
- 原始数据 SHA-256：`eeeecd9100cc681904ae06d9deee77a0eb09f5db1d5845a35317a4a17e979c32`
- legacy control 原始输入组合 SHA-256：`6c12decc206e6cf93145b7e719c5747b4464635d58406c13c8041737bf532e78`
- 运行环境：macOS-26.5.2-arm64-arm-64bit-Mach-O / Python 3.13.5
