# 五子棋证明搜索新模型（开局库关闭）vs 旧三星

## 结论（白棋优先）

- 新模型执白：38/1/61，得分率 38.5%，Wilson 95% 29.6%–48.3%
- 冻结旧三星 control 执白：34/0/66，得分率 34.0%，Wilson 95% 25.5%–43.7%
- 白棋得分变化：+4.5%
- 强度分类：`directional improvement`

新模型执黑为 70/0/30，得分率 70.0%，Wilson 95% 60.4%–78.1%；总体为 108/1/91，得分率 54.2%，Wilson 95% 47.3%–61.0%。报告保留五子棋自然先手优势，不做颜色再加权。

## 赛程与版本

- 赛程：100 个固定 opening ID，每个交换颜色；新模型执黑 100 局、执白 100 局
- profile：`three-star-threat-proof-no-book@3.0.0-vcf-dfpn-no-book`
- 旧模型：`legacy-three-star@5224020`
- 开局模式：`gomocup-curated-prefix`；开局库：`gomocup-2025-freestyle15-v1`
- 固定种子：`0xc0dec0de20260813`；域：`proof-final-proof-v1`
- 禁手：关闭（Freestyle-15）

## 搜索、开局与性能

- 新模型落子：2744；开局库命中：0
- 默认来源（none/legacy/book）：{'0': 0, '1': 2744, '2': 0}
- 覆盖原因（0..5）：{'0': 2216, '1': 108, '2': 163, '3': 104, '4': 153, '5': 0}
- 证明状态（unknown/proven/no-win-in-scope）：{'0': 1190, '1': 568, '2': 986}
- 已验证证书：568；证明节点：56065；预算耗尽：919
- 单步延迟：p50 190.78 ms，p95 444.30 ms，最大 1398.86 ms

所有 200 局均已逐手重放验证：无重复落点、越界、颜色顺序错误、终局后继续落子或结果不一致；所有被接受的 `proven-win` 均带运行时已验证证书标记。

## 复现命令

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
"$BENCHMARK_PATH" --profile proof-no-book --suite proof-final --random-mode best --opening-start 0 --opening-count 100 --max-moves 120 --forbidden-black 0 --output book-off.jsonl
```

## 数据与校验

- 原始数据：`proof_search_book_off_raw.json`
- 原始数据 SHA-256：`6fa5ab6532df9fac86e79b6712defd18e2564e009115fb58b818c5d25f55eb9a`
- legacy control 原始输入组合 SHA-256：`ba87d62154bc84d7619f6d618581e3bf5531d0c2581a459d79af8f73c6c311d3`
- 运行环境：macOS-26.5.2-arm64-arm-64bit-Mach-O / Python 3.13.5
