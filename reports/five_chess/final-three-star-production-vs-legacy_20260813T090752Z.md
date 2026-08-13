# 五子棋三星 AI 新旧版本强度测试报告

## 结论

新三星与旧三星完成 200 局配对对抗。新三星总体战绩为 83 胜、0 和、117 负，得分率 41.5%，Wilson 95% 区间为 34.9%–48.4%。

## 比赛设置

- 新算法：`three-star-production@2.1.0-targeted-10-10-12-narrow-softmax`
- 旧算法：`legacy-three-star@5224020`，源代码基线 `5224020`
- 棋盘：15×15；黑棋为 `1`，白棋为 `-1`
- 禁手：关闭
- 主种子：`0xf1ce5eed20260813`
- 赛程域：`final` / `final-v1`
- 决策模式：`user-softmax`
- 开局：100 个最终保留开局；每个开局交换新旧算法颜色
- 最大手数：120
- 新三星执黑：100 局；新三星执白：100 局
- 异常局：0

## 强度结果

| 分组 | 局数 | 新三星胜 | 和 | 新三星负 | 得分率 | Wilson 95% |
|---|---:|---:|---:|---:|---:|---:|
| 总体 | 200 | 83 | 0 | 117 | 41.5% | 34.9%–48.4% |
| 新三星执黑 | 100 | 70 | 0 | 30 | 70.0% | 60.4%–78.1% |
| 新三星执白 | 100 | 13 | 0 | 87 | 13.0% | 7.8%–21.0% |

得分按胜 1 分、和 0.5 分、负 0 分计算。区间用于表达 200 局样本下的不确定性，不将细小差异解释为确定提升。

## 搜索与性能

| 引擎 | 落子数 | 耗时 p50(ms) | 耗时 p95(ms) | 最大耗时(ms) | 节点 p50 | 节点 p95 | 平均完成深度 | 预算耗尽 | TT 命中 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 新三星 | 2009 | 202.530 | 670.985 | 851.245 | 1392 | 5860 | 2.75 | 56 | 68575 |
| 旧三星 | 2026 | 1.998 | 132.604 | 417.885 | 0 | 0 | 0.00 | 0 | 0 |

旧三星没有节点/深度埋点，因此对应节点和深度显示为 0；耗时仍使用相同的单步墙钟计量。

## 随机性与公平性

- 用户模式仅在通过战术安全检查的近优候选间进行 Softmax 抽样；立即获胜和唯一必要防守保持确定性。
- 本次评测为每个开局、手数、颜色和引擎派生独立固定种子，重复运行可复现。
- 调参使用 `training-v1` 种子域；正式验收仅接受固定的 `final-v1` 主种子和 opening 0..99，生成开局与逐步决策种子均不重叠。
- 两个颜色使用同一批开局并交换算法，降低先手和开局差异造成的偏差。

## 代表棋局

- 最短新三星胜局：B(8,8) W(9,7) B(10,8) W(7,11) B(7,8) W(7,5) B(9,8) W(6,8) B(11,8)
- 最短新三星负局：B(8,8) W(9,7) B(10,8) W(7,11) B(7,8) W(7,5) B(9,8)

坐标为 1-based，`B` 表示黑棋，`W` 表示白棋；完整棋谱位于原始数据。

## 运行环境与版本

- 系统：`macOS-26.5.2-arm64-arm-64bit-Mach-O`
- Python：`3.13.5`
- 代码提交：`8f0436edfbb1228f2b3d7c0709a113e3ac434ac3`
- 工作区状态：包含本次未提交实现变更
- 新三星参数：`{"name":"three-star-production","version":"2.1.0-targeted-10-10-12-narrow-softmax","maxDepth":3,"quiescenceDepth":3,"fourDepth":10,"doubleThreeDepth":10,"forcingDepth":12,"candidateLimit":12,"nodeBudget":14000,"timeBudgetMs":850,"transpositionCapacity":65536,"attackWeight":100,"defenseWeight":112,"centerWeight":4,"nearBestWindow":60,"randomTemperature":45.0,"maxRandomCandidates":2}`

## 复现命令

```sh
BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)
# 每个 opening 内自动交换 new/legacy 颜色
"$BENCHMARK_PATH" --profile production --suite final --random-mode user --seed 0xf1ce5eed20260813 --opening-start 0 --opening-count 100 --max-moves 120 --forbidden-black 0 --output final.jsonl
```

## 原始数据与校验

- 原始逐局数据：`final-three-star-production-vs-legacy_20260813T090752Z_raw.json`
- 汇总数据：`final-three-star-production-vs-legacy_20260813T090752Z_summary.json`
- 原始数据 SHA-256：`e195acc58b19ae9fc691e46878e5d3fe3aa644059f33232d1054b2a9055dccab`
- 输入分片：`final_20260813_production_vs_legacy.jsonl`

## 异常说明

未发现非法落子、状态污染、崩溃或未记录的超时。
