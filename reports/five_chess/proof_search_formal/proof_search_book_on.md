# 五子棋证明搜索新模型（开局库开启）vs 旧三星

## 结论（白棋优先）

- 新模型执白：38/0/62，得分率 38.0%，Wilson 95% 29.1%–47.8%
- 同组旧三星执白（对应新模型执黑的 100 局）：49/1/50，得分率 49.5%，Wilson 95% 39.9%–59.1%
- 新模型白棋相对同组旧三星白棋：-11.5%
- 强度分类：`not demonstrated`

总体或白棋门槛未同时满足，因此不发布为更强三星。

新模型执黑为 50/1/49，得分率 50.5%，Wilson 95% 40.9%–60.1%；总体为 88/1/111，得分率 44.2%，Wilson 95% 37.5%–51.2%。报告保留五子棋自然先手优势，不做颜色再加权。

独立旧三星自对练 control 执白为 52/0/48，得分率 52.0%，Wilson 95% 42.3%–61.5%。该值只用于复现赛程与观察自然颜色基线，不参与上述白棋增量或强度分类。

## 赛程与版本

- 赛程：100 个固定 opening ID，每个交换颜色；新模型执黑 100 局、执白 100 局
- profile：`three-star-threat-proof-book@3.0.0-vcf-dfpn-book`
- 旧模型：`legacy-three-star@5224020`
- 开局模式：`gomocup-curated-prefix`；开局库：`gomocup-2024-2025-freestyle15-v2`
- 固定种子：`0xc0dec0de20260813`；域：`proof-final-proof-v2`
- 正式开局清单：`final-opening-schedule.json`，100/100 canonical ID 唯一，SHA-256 `232a0dbb03d56d924331c1d1b6ff404928d0cb8a1aa8a561efcfd7af686e27ff`
- 禁手：关闭（Freestyle-15）

## 搜索、开局与性能

- 新模型落子：2674；开局库命中：106
- 默认来源（none/legacy/book）：{'0': 0, '1': 2568, '2': 106}
- 覆盖原因（0..5）：{'0': 2080, '1': 88, '2': 225, '3': 105, '4': 176, '5': 0}
- 证明状态（unknown/proven/no-win-in-scope）：{'0': 1212, '1': 583, '2': 879}
- 已验证证书：583；证明节点：49949；预算耗尽：899
- 单步延迟：p50 191.76 ms，p95 458.99 ms，最大 1192.30 ms

所有 200 局均已逐手重放验证：无重复落点、越界、颜色顺序错误、终局后继续落子或结果不一致；所有被接受的 `proven-win` 均带运行时已验证证书标记。

## 复现命令

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
"$BENCHMARK_PATH" --profile proof-book --suite proof-final --random-mode best --opening-start 0 --opening-count 100 --max-moves 120 --forbidden-black 0 --output book-on.jsonl
```

## 数据与校验

- 原始数据：`proof_search_book_on_raw.json`
- 原始数据 SHA-256：`9b683195d7298f2bf7ad127a8cc48efd4817e759125b9cf5f56dfe798cb0b809`
- legacy control 原始输入组合 SHA-256：`6c12decc206e6cf93145b7e719c5747b4464635d58406c13c8041737bf532e78`
- 运行环境：macOS-26.5.2-arm64-arm-64bit-Mach-O / Python 3.13.5
