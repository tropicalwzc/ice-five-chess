# 五星 5.8.1 对三星、四星和 exact 5.4.1 详细标准测试报告

## 测试结论

本报告比较 UI 已推广的五星模型 `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` 与冻结三星、四星和 exact 5.4.1。每个对手使用同一批 50 个自然开局并交换颜色，共 100 局；5.8.1 执黑 50 局、执白 50 局。三组共 300 局。

| 对手 | 5.8.1 执黑 W/D/L | 5.8.1 执白 W/D/L | 总体 W/D/L | 得分率 | Wilson 95% | 配对 bootstrap 95% |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 冻结三星 | 38/0/12 | 25/0/25 | 63/0/37 | 63.0% | 53.2%–71.8% | 56.0%–70.0% |
| 四星 | 35/0/15 | 21/0/29 | 56/0/44 | 56.0% | 46.2%–65.3% | 49.0%–63.0% |
| exact 5.4.1 | 33/0/17 | 19/0/31 | 52/0/48 | 52.0% | 42.3%–61.5% | 44.0%–60.0% |

三组汇总为 **171/0/129**，得分率 **57.0%**。该合计只描述这三个不同对手的测试总量，不能用来代替逐对手结论。

点估计显示 5.8.1 对三星优势最明显，对四星为正向，对 exact 5.4.1 为轻微正向。四星和 5.4.1 的总体区间仍跨越 50%，因此不能仅凭本样本宣称统计显著优势。三个单元均显示黑棋得分高于白棋，配对结果也大量由交换颜色后各胜一盘构成，应继续按颜色和配对开局解释。

## 统一测试口径

- Suite：`five-star-natural-final`；seed domain：`five-star-natural-final-five-star-natural-free-v4`。
- Master seed：`0x9ee4d91480ac5889`；开局 ID 0–49。
- 规则：Freestyle，无禁手；每局最多 120 手。
- 选择：deterministic-best；策略：hybrid-deep-verified。
- 每个开局交换颜色；每个对手 100 局，黑白各 50。
- 5.8.1 sentinel：adaptive depth 5/7、16,000 nodes、80 ms、最多 2 个 early alternatives。
- 决策内部预算 4,500 ms，玩家可见硬门槛 5,000 ms。
- 三组均单进程串行运行；四星和 5.4.1 使用此前同口径标准原始日志，三星为本轮补跑；三组均由同一当前 replay 工具重新验证。

## 逐对手详细结果

### 对 冻结三星

- 执黑：38/0/12，得分率 76.0%，Wilson 95% 62.6%–85.7%。
- 执白：25/0/25，得分率 50.0%，Wilson 95% 36.6%–63.4%。
- 总体：63/0/37，得分率 63.0%，Wilson 95% 53.2%–71.8%。
- 同色比较：5.8.1 黑棋 76.0% vs 对手黑棋 50.0%，变化 +26.0 个百分点；5.8.1 白棋 50.0% vs 对手白棋 24.0%，变化 +26.0 个百分点。
- 50 个配对开局：5.8.1 双杀 15，对手双杀 2，各胜一盘 33，含和棋配对 0；配对得分率 63.0%，bootstrap 95% 56.0%–70.0%。
- 终局类型：{"five-in-a-row": 100}；棋局手数 p50/p95/max = 31/66/82。

决策和运行时：

- 5.8.1 决策 1450 次（执黑局 717、执白局 733）；对手决策 1437 次。
- 5.8.1 wall p50/p95/max = 603.469/4502.912/4858.980 ms；CPU = 723.199/5399.318/14294.545 ms；nodes = 15/180/804。
- 对手 wall p50/p95/max = 0.375/53.043/567.104 ms；CPU = 0.375/52.883/566.559 ms；nodes = 0/0/0。
- 峰值 RSS：5.8.1 37.0 MiB；对手 37.0 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、4858.980 ms。
- 最大对手决策：opening 12、ply 10、(6,10)、567.104 ms。
- >5,000 ms 决策 0；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1387，skip 63；effective depth {"5": 324, "7": 1063}；adaptive escalation 1063。
- Sentinel 状态 {"no-forced-win-in-scope": 1076, "proven-win": 88, "unknown": 223}；审计 alternatives 1560，其中 verified-loss 258。
- 提前避免 verified loss 5 次；early move change 5 次；final-guard-only catch 115 次。
- Sentinel latency p50/p95/max = 4.117/81.678/82.926 ms；nodes = 5/68/106；cache reuse decisions/hits = 85/3052。
- Final guard eligible 1193；审计 candidates 2147；verified-loss alternatives 970；避免 verified loss 14。
- Final guard VCF latency p50/p95/max = 2.492/451.660/1500.247 ms；VCT eligible 976，VCT latency = 34.571/890.692/3503.157 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、3621 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 88、final VCF 97、final VCT 0，合计 185 个证书。
- Runtime 已独立验证的一般分析证书 291；replay anomaly 0。

### 对 四星

- 执黑：35/0/15，得分率 70.0%，Wilson 95% 56.2%–80.9%。
- 执白：21/0/29，得分率 42.0%，Wilson 95% 29.4%–55.8%。
- 总体：56/0/44，得分率 56.0%，Wilson 95% 46.2%–65.3%。
- 同色比较：5.8.1 黑棋 70.0% vs 对手黑棋 58.0%，变化 +12.0 个百分点；5.8.1 白棋 42.0% vs 对手白棋 30.0%，变化 +12.0 个百分点。
- 50 个配对开局：5.8.1 双杀 10，对手双杀 4，各胜一盘 36，含和棋配对 0；配对得分率 56.0%，bootstrap 95% 49.0%–63.0%。
- 终局类型：{"five-in-a-row": 44, "proven-loss": 56}；棋局手数 p50/p95/max = 33/80/116。

决策和运行时：

- 5.8.1 决策 1643 次（执黑局 811、执白局 832）；对手决策 1637 次。
- 5.8.1 wall p50/p95/max = 630.397/4502.678/4862.800 ms；CPU = 771.803/5417.925/13503.981 ms；nodes = 14/180/829。
- 对手 wall p50/p95/max = 162.871/335.924/1055.249 ms；CPU = 162.697/335.803/1055.120 ms；nodes = 22/451/745。
- 峰值 RSS：5.8.1 35.9 MiB；对手 35.9 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、4862.800 ms。
- 最大对手决策：opening 46、ply 29、(6,7)、1055.249 ms。
- >5,000 ms 决策 0；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1643，skip 0；effective depth {"5": 413, "7": 1230}；adaptive escalation 1230。
- Sentinel 状态 {"no-forced-win-in-scope": 1219, "proven-win": 139, "unknown": 285}；审计 alternatives 1910，其中 verified-loss 401。
- 提前避免 verified loss 7 次；early move change 7 次；final-guard-only catch 176 次。
- Sentinel latency p50/p95/max = 4.643/81.570/90.085 ms；nodes = 5/70/108；cache reuse decisions/hits = 134/3866。
- Final guard eligible 1481；审计 candidates 2923；verified-loss alternatives 1502；避免 verified loss 16。
- Final guard VCF latency p50/p95/max = 2.467/500.194/2000.226 ms；VCT eligible 1183，VCT latency = 30.062/871.813/3503.203 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、4014 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 139、final VCF 156、final VCT 0，合计 295 个证书。
- Runtime 已独立验证的一般分析证书 318；replay anomaly 0。

### 对 exact 5.4.1

- 执黑：33/0/17，得分率 66.0%，Wilson 95% 52.2%–77.6%。
- 执白：19/0/31，得分率 38.0%，Wilson 95% 25.9%–51.8%。
- 总体：52/0/48，得分率 52.0%，Wilson 95% 42.3%–61.5%。
- 同色比较：5.8.1 黑棋 66.0% vs 对手黑棋 62.0%，变化 +4.0 个百分点；5.8.1 白棋 38.0% vs 对手白棋 34.0%，变化 +4.0 个百分点。
- 50 个配对开局：5.8.1 双杀 9，对手双杀 7，各胜一盘 34，含和棋配对 0；配对得分率 52.0%，bootstrap 95% 44.0%–60.0%。
- 终局类型：{"five-in-a-row": 100}；棋局手数 p50/p95/max = 37/74/115。

决策和运行时：

- 5.8.1 决策 1694 次（执黑局 844、执白局 850）；对手决策 1692 次。
- 5.8.1 wall p50/p95/max = 390.509/4501.341/4867.479 ms；CPU = 484.399/5369.608/13503.500 ms；nodes = 13/172/261。
- 对手 wall p50/p95/max = 178.291/4500.247/5074.030 ms；CPU = 179.325/7766.534/35429.839 ms；nodes = 15/262/1343。
- 峰值 RSS：5.8.1 59.7 MiB；对手 59.7 MiB。
- 最大 5.8.1 决策：opening 12、ply 7、(12,5)、4867.479 ms。
- 最大对手决策：opening 12、ply 10、(5,9)、5074.030 ms。
- >5,000 ms 决策 1；runner anomaly 0。

Early micro-VCF 与最终 guard：

- Sentinel eligible 1642，skip 52；effective depth {"5": 501, "7": 1141}；adaptive escalation 1141。
- Sentinel 状态 {"no-forced-win-in-scope": 1265, "proven-win": 157, "unknown": 220}；审计 alternatives 1947，其中 verified-loss 453。
- 提前避免 verified loss 13 次；early move change 13 次；final-guard-only catch 186 次。
- Sentinel latency p50/p95/max = 3.643/81.645/90.400 ms；nodes = 5/65/103；cache reuse decisions/hits = 152/2804。
- Final guard eligible 1486；审计 candidates 3076；verified-loss alternatives 1646；避免 verified loss 23。
- Final guard VCF latency p50/p95/max = 0.746/340.753/2000.273 ms；VCT eligible 1179，VCT latency = 24.655/939.939/3503.063 ms。
- Sentinel rollback 0，final guard rollback 0，evidence mismatch 0。

独立重放：

- 100 局、4120 手全部重放；合法性、胜负、棋盘完整性通过。
- 冷启动重新证明 early VCF 157、final VCF 166、final VCT 2，合计 325 个证书。
- Runtime 已独立验证的一般分析证书 599；replay anomaly 0。

## 运行时门槛汇总

| 对手 | 5.8.1 最大决策 | 对手最大决策 | >5秒来源 | Runner anomaly | Replay |
| --- | ---: | ---: | --- | ---: | --- |
| 冻结三星 | 4858.980 ms | 567.104 ms | 无 | 0 | pass |
| 四星 | 4862.800 ms | 1055.249 ms | 无 | 0 | pass |
| exact 5.4.1 | 4867.479 ms | 5074.030 ms | five-star-control opening 12 ply 10 5074.030ms | 0 | pass |

5.8.1 在三组共 300 局中没有任何决策超过 5,000 ms。exact 5.4.1 单元仍包含此前已复现的对照侧 5,074.030 ms 超时，因此该单元的“双方均通过硬门槛”结论为失败，但不属于 5.8.1 超时。

## 综合解释

- 对冻结三星：样本内优势方向明确，适合作为低级别强度回归证据。
- 对四星：总体 56%，但 Wilson 与配对区间仍覆盖 50%，属于正向但未证明显著。
- 对 exact 5.4.1：总体 52%，双杀开局 9 比 7，属于轻微正向且未证明显著。
- 5.8.1 的核心防守机制在三个对手上都实际触发并提前替换 verified-loss 走法；最终 guard 仍捕获大量超出浅层 sentinel 范围的风险，说明两层防守都不可省略。
- 白棋得分在三组中均弱于黑棋。后续若继续改进，应优先分析白棋局和final-guard-only catches，而不是只根据总体胜率继续扩大浅层 sentinel。

## 限制

- 本报告只有 Freestyle；不代表有禁手规则表现。
- 每个对手仅 50 个配对开局；四星和 5.4.1 的区间不足以支持显著性结论。
- 三组复用了同一批开局以增强横向可比性，因此三组结果不是相互独立样本。
- JSONL 保存一般分析证书 ID 和验证标志，但未保存所有证书节点；冷重证范围是能够从最终/临时根精确重建的 early/final guard 证书。
- 计时会受机器负载影响；硬门槛判断以原始每步 wall `ms` 为准。

## 证据文件

最终目录按 `raw/`、`replay/`、`report/`、`provenance/` 组织。`summary.json` 保存本报告全部结构化统计，`paired_openings.csv` 保存每个对手每个开局的换色配对结果，`checksums.sha256` 固定交付文件哈希。

