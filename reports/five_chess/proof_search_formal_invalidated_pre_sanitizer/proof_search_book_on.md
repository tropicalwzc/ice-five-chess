# 五子棋证明搜索新模型（开局库开启）vs 旧三星

## 结论（白棋优先）

- 新模型执白：42/0/58，得分率 42.0%，Wilson 95% 32.8%–51.8%
- 冻结旧三星 control 执白：34/0/66，得分率 34.0%，Wilson 95% 25.5%–43.7%
- 白棋得分变化：+8.0%
- 强度分类：`not demonstrated`

新模型执黑为 58/0/42，得分率 58.0%，Wilson 95% 48.2%–67.2%；总体为 100/0/100，得分率 50.0%，Wilson 95% 43.1%–56.9%。报告保留五子棋自然先手优势，不做颜色再加权。

## 赛程与版本

- 赛程：100 个固定 opening ID，每个交换颜色；新模型执黑 100 局、执白 100 局
- profile：`three-star-threat-proof-book@3.0.0-vcf-dfpn-book`
- 旧模型：`legacy-three-star@5224020`
- 开局模式：`gomocup-curated-prefix`；开局库：`gomocup-2025-freestyle15-v1`
- 固定种子：`0xc0dec0de20260813`；域：`proof-final-proof-v1`
- 禁手：关闭（Freestyle-15）

## 搜索、开局与性能

- 新模型落子：2815；开局库命中：104
- 默认来源（none/legacy/book）：{'0': 0, '1': 2711, '2': 104}
- 覆盖原因（0..5）：{'0': 2267, '1': 100, '2': 179, '3': 115, '4': 154, '5': 0}
- 证明状态（unknown/proven/no-win-in-scope）：{'0': 1199, '1': 582, '2': 1034}
- 已验证证书：582；证明节点：50233；预算耗尽：920
- 单步延迟：p50 186.22 ms，p95 447.43 ms，最大 1214.46 ms

所有 200 局均已逐手重放验证：无重复落点、越界、颜色顺序错误、终局后继续落子或结果不一致；所有被接受的 `proven-win` 均带运行时已验证证书标记。

## 复现命令

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
"$BENCHMARK_PATH" --profile proof-book --suite proof-final --random-mode best --opening-start 0 --opening-count 100 --max-moves 120 --forbidden-black 0 --output book-on.jsonl
```

## 数据与校验

- 原始数据：`proof_search_book_on_raw.json`
- 原始数据 SHA-256：`24615eaec16d192c6dd1aa0c1f452c1bc8d32ca993416c5c69ee911aa28696e1`
- legacy control 原始输入组合 SHA-256：`ba87d62154bc84d7619f6d618581e3bf5531d0c2581a459d79af8f73c6c311d3`
- 运行环境：macOS-26.5.2-arm64-arm-64bit-Mach-O / Python 3.13.5
