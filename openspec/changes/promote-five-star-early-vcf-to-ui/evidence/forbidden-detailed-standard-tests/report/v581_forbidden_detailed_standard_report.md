# 五星 5.8.1 对三星、四星和 exact 5.4.1（黑方禁手开启）详细标准测试报告

## 测试结论

本报告比较 UI 已推广的五星模型 `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` 与冻结三星、四星和 exact 5.4.1。规则为黑方禁手开启；每个对手使用同一批 50 个自然开局并交换颜色，共 100 局；5.8.1 执黑 50 局、执白 50 局。三组共 300 局。

| 对手 | 5.8.1 执黑 W/D/L | 5.8.1 执白 W/D/L | 总体 W/D/L | 得分率 | Wilson 95% | 配对 bootstrap 95% |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 冻结三星 | 30/0/20 | 30/0/20 | 60/0/40 | 60.0% | 50.2%–69.1% | 52.0%–68.0% |
| 四星 | 29/0/21 | 26/0/24 | 55/0/45 | 55.0% | 45.2%–64.4% | 47.0%–63.0% |
| exact 5.4.1 | 29/0/21 | 25/0/25 | 54/0/46 | 54.0% | 44.3%–63.4% | 50.0%–59.0% |

三组汇总为 **169/0/131**，得分率 **56.3%**。该合计只描述这三个不同对手的测试总量，不能用来代替逐对手结论。

逐对手点估计为：对冻结三星 60.0%、对四星 55.0%、对exact 5.4.1 54.0%。显著性应同时参考 Wilson 和按 50 个开局成对 bootstrap 的区间；颜色交换结果仍需按执黑、执白和配对开局分别解释。

## 统一测试口径

- Suite：`five-star-natural-final`；seed domain：`five-star-natural-final-five-star-natural-forbidden-v4`。
- Master seed：`0x9ee4d91480ac5889`；开局 ID 0–49。
- 规则：黑方禁手开启；每局最多 120 手。
- 选择：deterministic-best；策略：hybrid-deep-verified。
- 每个开局交换颜色；每个对手 100 局，黑白各 50。
- 5.8.1 sentinel：adaptive depth 5/7、16,000 nodes、80 ms、最多 2 个 early alternatives。
- 决策内部预算 4,500 ms，玩家可见硬门槛 5,000 ms。
- 三组均为本轮新跑、单进程串行运行，并由同一当前 replay 工具重新验证。

## 逐对手详细结果

### 对冻结三星

- 执黑：30/0/20，得分率 60.0%，Wilson 95% 46.2%–72.4%。
- 执白：30/0/20，得分率 60.0%，Wilson 95% 46.2%–72.4%。
- 总体：60/0/40，得分率 60.0%，Wilson 95% 50.2%–69.1%。
- 同色比较：5.8.1 黑棋 60.0% vs 对手黑棋 40.0%，变化 +20.0 个百分点；5.8.1 白棋 60.0% vs 对手白棋 40.0%，变化 +20.0 个百分点。
- 50 个配对开局：5.8.1 双杀 15，对手双杀 5，各胜一盘 30，含和棋配对 0；配对得分率 60.0%，bootstrap 95% 52.0%–68.0%。
- 终局类型：{"five-in-a-row": 99, "legacy-illegal-move-loss": 1}；棋局手数 p50/p95/max = 32/78/89。

决策和运行时：

- 5.8.1 决策 1556 次（执黑局 813、执白局 743）；对手决策 1546 次。
- 5.8.1 wall p50/p95/max = 856.495/4507.717/5021.533 ms；CPU = 940.886/6042.311/15623.233 ms；nodes = 14/87/713。
- 对手 wall p50/p95/max = 0.499/62.598/712.668 ms；CPU = 0.490/62.270/712.018 ms；nodes = 0/0/0。
- 峰值 RSS：5.8.1 34.5 MiB；对手 34.5 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、5021.533 ms。
- 最大对手决策：opening 12、ply 10、(6,10)、712.668 ms。
- >5,000 ms 决策 1；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1497，skip 59；effective depth {"5": 354, "7": 1143}；adaptive escalation 1143。
- Sentinel 状态 {"no-forced-win-in-scope": 1126, "proven-win": 97, "unknown": 274}；审计 alternatives 1677，其中 verified-loss 267。
- 提前避免 verified loss 18 次；early move change 18 次；final-guard-only catch 111 次。
- Sentinel latency p50/p95/max = 7.495/83.484/111.960 ms；nodes = 5/43/58；cache reuse decisions/hits = 92/1281。
- Final guard eligible 1323；审计 candidates 2328；verified-loss alternatives 990；避免 verified loss 30。
- Final guard VCF latency p50/p95/max = 4.249/679.722/2000.722 ms；VCT eligible 1064，VCT latency = 65.416/1285.603/4116.139 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、3836 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 97、final VCF 94、final VCT 0，合计 191 个证书。
- Runtime 已独立验证的一般分析证书 268；replay anomaly 0。

### 对四星

- 执黑：29/0/21，得分率 58.0%，Wilson 95% 44.2%–70.6%。
- 执白：26/0/24，得分率 52.0%，Wilson 95% 38.5%–65.2%。
- 总体：55/0/45，得分率 55.0%，Wilson 95% 45.2%–64.4%。
- 同色比较：5.8.1 黑棋 58.0% vs 对手黑棋 48.0%，变化 +10.0 个百分点；5.8.1 白棋 52.0% vs 对手白棋 42.0%，变化 +10.0 个百分点。
- 50 个配对开局：5.8.1 双杀 12，对手双杀 7，各胜一盘 31，含和棋配对 0；配对得分率 55.0%，bootstrap 95% 47.0%–63.0%。
- 终局类型：{"five-in-a-row": 45, "proven-loss": 55}；棋局手数 p50/p95/max = 35/84/93。

决策和运行时：

- 5.8.1 决策 1637 次（执黑局 839、执白局 798）；对手决策 1632 次。
- 5.8.1 wall p50/p95/max = 908.853/4506.240/5019.576 ms；CPU = 1077.720/5905.855/15619.109 ms；nodes = 13/89/679。
- 对手 wall p50/p95/max = 177.618/361.729/1184.234 ms；CPU = 176.519/360.020/1180.762 ms；nodes = 14/168/287。
- 峰值 RSS：5.8.1 34.4 MiB；对手 34.4 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、5019.576 ms。
- 最大对手决策：opening 27、ply 34、(13,6)、1184.234 ms。
- >5,000 ms 决策 1；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1637，skip 0；effective depth {"5": 416, "7": 1221}；adaptive escalation 1221。
- Sentinel 状态 {"no-forced-win-in-scope": 1206, "proven-win": 124, "unknown": 307}；审计 alternatives 1864，其中 verified-loss 340。
- 提前避免 verified loss 19 次；early move change 19 次；final-guard-only catch 145 次。
- Sentinel latency p50/p95/max = 7.796/83.403/110.059 ms；nodes = 5/44/58；cache reuse decisions/hits = 118/1482。
- Final guard eligible 1484；审计 candidates 2750；verified-loss alternatives 1269；避免 verified loss 35。
- Final guard VCF latency p50/p95/max = 4.191/697.955/2000.701 ms；VCT eligible 1180，VCT latency = 68.288/1267.619/4109.341 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、4003 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 124、final VCF 122、final VCT 0，合计 246 个证书。
- Runtime 已独立验证的一般分析证书 275；replay anomaly 0。

### 对exact 5.4.1

- 执黑：29/0/21，得分率 58.0%，Wilson 95% 44.2%–70.6%。
- 执白：25/0/25，得分率 50.0%，Wilson 95% 36.6%–63.4%。
- 总体：54/0/46，得分率 54.0%，Wilson 95% 44.3%–63.4%。
- 同色比较：5.8.1 黑棋 58.0% vs 对手黑棋 50.0%，变化 +8.0 个百分点；5.8.1 白棋 50.0% vs 对手白棋 42.0%，变化 +8.0 个百分点。
- 50 个配对开局：5.8.1 双杀 5，对手双杀 1，各胜一盘 44，含和棋配对 0；配对得分率 54.0%，bootstrap 95% 50.0%–59.0%。
- 终局类型：{"five-in-a-row": 100}；棋局手数 p50/p95/max = 38/79/104。

决策和运行时：

- 5.8.1 决策 1708 次（执黑局 840、执白局 868）；对手决策 1704 次。
- 5.8.1 wall p50/p95/max = 736.718/4503.653/5044.334 ms；CPU = 890.133/5976.910/15606.018 ms；nodes = 13/80/655。
- 对手 wall p50/p95/max = 222.301/4500.409/5233.651 ms；CPU = 221.146/8844.329/34373.043 ms；nodes = 13/111/649。
- 峰值 RSS：5.8.1 50.7 MiB；对手 50.7 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、5044.334 ms。
- 最大对手决策：opening 12、ply 10、(6,10)、5233.651 ms。
- >5,000 ms 决策 2；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1654，skip 54；effective depth {"5": 443, "7": 1211}；adaptive escalation 1211。
- Sentinel 状态 {"no-forced-win-in-scope": 1246, "proven-win": 138, "unknown": 270}；审计 alternatives 1904，其中 verified-loss 373。
- 提前避免 verified loss 20 次；early move change 20 次；final-guard-only catch 156 次。
- Sentinel latency p50/p95/max = 8.183/83.996/110.025 ms；nodes = 5/38/60；cache reuse decisions/hits = 129/1365。
- Final guard eligible 1486；审计 candidates 2822；verified-loss alternatives 1344；避免 verified loss 36。
- Final guard VCF latency p50/p95/max = 4.484/500.917/1506.435 ms；VCT eligible 1215，VCT latency = 62.695/1154.454/4098.812 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、4146 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 138、final VCF 127、final VCT 3，合计 268 个证书。
- Runtime 已独立验证的一般分析证书 532；replay anomaly 0。

## 运行时门槛汇总

| 对手 | 5.8.1 最大决策 | 对手最大决策 | >5秒来源 | Runner anomaly | Replay |
| --- | ---: | ---: | --- | ---: | --- |
| 冻结三星 | 5021.533 ms | 712.668 ms | new opening 12 ply 7 5021.533ms | 0 | pass |
| 四星 | 5019.576 ms | 1184.234 ms | new opening 12 ply 7 5019.576ms | 0 | pass |
| exact 5.4.1 | 5044.334 ms | 5233.651 ms | new opening 12 ply 7 5044.334ms; five-star-control opening 12 ply 10 5233.651ms | 0 | pass |

5.8.1 在三组共 300 局中有 3 次决策超过 5,000 ms。对手侧共有 1 次决策超过 5,000 ms，具体位置见上表。
候选侧的 3 次超时均为同一固定局面：opening 12、5.8.1 执白、ply 7、走 (12,5)，属于跨对手重复出现的尾延迟。

## 综合解释

- 对冻结三星：总体 60.0%，两种 95% 区间均高于 50%；配对双杀 15 比 5。
- 对四星：总体 55.0%，两种 95% 区间未同时排除 50%；配对双杀 12 比 7。
- 对exact 5.4.1：总体 54.0%，两种 95% 区间未同时排除 50%；配对双杀 5 比 1。
- Early sentinel 三组共提前避免 verified loss 57 次；final-guard-only catch 412 次。两层机制的具体负载和收益应结合逐对手遥测解释。
- 黑白表现和换色配对结果见逐对手部分；不同规则下不可直接套用自由规则结论。
- 三星单元有 1 局 `legacy-illegal-move-loss`：旧三星执黑尝试禁手，非法着未落盘并按规则判负；独立重放已验证棋盘和胜方。

## 限制

- 本报告只覆盖黑方禁手开启规则；不代表 Freestyle 表现。
- 每个对手仅 50 个配对开局；是否显著以逐对手 Wilson 与配对区间为准。
- 三组复用了同一批开局以增强横向可比性，因此三组结果不是相互独立样本。
- JSONL 保存一般分析证书 ID 和验证标志，但未保存所有证书节点；冷重证范围是能够从最终/临时根精确重建的 early/final guard 证书。
- 计时会受机器负载影响；硬门槛判断以原始每步 wall `ms` 为准。

## 证据文件

最终目录按 `raw/`、`replay/`、`report/`、`provenance/` 组织。`summary.json` 保存本报告全部结构化统计，`paired_openings.csv` 保存每个对手每个开局的换色配对结果，`checksums.sha256` 固定交付文件哈希。

