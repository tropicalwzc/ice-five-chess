# 5.6 禁手黑棋探索记录

本文件只记录探索证据，不代表已经 apply 或改变生产模型。

## 结论先行

当前 5.6 报告中“禁手黑棋 44%”不能直接解释为算法从旧版 58% 退化：

1. 两个报告使用完全不同且无交集的 50 个开局前缀；当前赛程的实际石色胜负
   是黑 43、白 56、和棋 1，旧赛程是黑 58、白 42。
2. 当前重叠分组调度把每个批次压成一组，禁手候选共 982 批、982 worker，
   即平均每批 1 worker；旧 5.6.1 是 1046 批、7108 worker，平均约 6.8
   worker。当前 8 worker 主要是配置上限，不是实际并行度。
3. 真正的禁手黑棋瓶颈在逃生证明与合法性成本：843 个黑棋决策检查了 3669
   个逃生候选，其中 3605 个 unknown，只有 1 个 scoped disproof；同一批决策
   触发约 105.8 亿次合法性调用和 24.6 亿次整盘复制。

## 代码定位

- `FiveChessAI.c:4404` 的根证明可并行，但 `:4552-4554` 将 worker 数限制为
  overlap group 数；`groupCount == 1` 时必然只启动一个 worker。
- `FiveChessAI.c:5626` 的 loss-aware escape 仍调用串行
  `fc_prove_forced_win`，没有使用并行 dispatcher。
- `FiveChessAI.c:782-787`、`:956-960`、`:1032-1044` 明确保留 forbidden
  black 的 full-board legality fallback。
- `FiveChessAI.c:2123` 在黑方禁手依赖中将整条四方向线放入依赖掩码，容易把
  不同 gain 连接成一个保守组。
- `tools/five_chess_benchmark.m:394-411` 只按 ply 奇偶给前缀着色，没有
  swap/RIF 第五手选择信息。

## 论文落地顺序

先做 job-level root/escape 调度和实际并发度门禁，再做 virtual PN/DN、
postponed siblings/dovetailing；最后才考虑共享 TT。不同 gain 的证明结论不
能因为掩码重叠而合并。禁手合法性缓存必须用原始 oracle 做逐点交叉验证。

## 补充权威文献与可落地启示

以下文献用于确定下一版搜索架构，不把论文中的实验结果直接当作本项目的棋力
保证：

- Schijf、Allis、Uiterwijk，*Proof-Number Search and Transpositions*，ICGA
  Journal 17(2), DOI `10.3233/ICG-1994-17203`：转置表只有在规则、轮到谁、
  搜索类别和 profile 语义都进入 key 时才可安全复用；禁手合法性缓存不能与无禁手
  节点混用。
- Nagai，*A New Depth-First Search Algorithm for AND/OR Trees*，Artificial
  Intelligence，DOI `10.1016/S0004-3702(01)00084-4`：DFPN 的阈值/最有希望子节点
  选择比机械提高固定深度更重要，应把根 gain 作为可动态再平衡的 job。
- Yoshizoe、Kishimoto、Müller，*Lambda Depth-First Proof Number Search and
  its Application to Go*，DOI `10.1007/978-3-540-87608-3_3`：lambda 阈值可减少
  在 4.5 秒门禁内反复重做浅层兄弟的开销，适合在完成可复现基准后作为第二阶段。
- Saito、Winands，*Paranoid Proof-Number Search*，DOI
  `10.1109/itw.2010.5593354`；Saito 等，*Randomized Parallel Proof-Number
  Search*，DOI `10.1007/978-3-642-12993-3_8`：共享内存并行需要 virtual PN/DN、
  最有希望节点的竞争控制和可重放的完成结果；仅把线程上限设为 8 不构成并行收益。
- Zhang、Iida、van den Herik，*Probability based Proof Number Search*，DOI
  `10.5220/0007386806610668`：可用于把 unknown 候选按证明潜力排序，但不能用概率
  分数替代禁手合法性证明。
- Saffidine、Cazenave，*Multiple-Outcome Proof Number Search*，DOI
  `10.3233/978-1-61499-098-7-708`：将胜、负、和棋作为独立结果有助于避免把超时
  unknown 错报成平局；当前报告仍应保留 W/D/L 原始分类。
- Wágner、Virág，*Solving Renju*，DOI `10.3233/ICG-2001-24104`：禁手与开局
  交换流程必须一起建模；自然前缀/no-swap 只能用于搜索诊断，不能用来声称正式
  RIF 平衡开局下黑方理论优势消失或恢复。

由这些文献共同得到的工程顺序是：

```text
合法性 oracle 等价缓存
        ↓
独立 gain job + 动态负载均衡 + 逃生候选并行
        ↓
virtual PN/DN、DFPN 阈值与 postponed siblings
        ↓
可选共享 TT（严格 key/完成结果合并）
        ↓
RIF/Swap2/Taraguchi 分规则 A/B 与棋力报告
```

这条顺序解释了为什么当前先加“更深搜索”可能无效：时间先被保守重叠分组、串行
逃生查询和禁手全盘合法性复制消耗，深度上限实际上没有转化成更多完成证明。

## 规则解释

RIF 官方规则资料说明，禁手本身不足以完全平衡黑方，正式 Renju 依赖交换和
开局选择流程。当前 natural-prefix/no-swap 赛程应作为算法诊断，不应作为
正式平衡开局下的黑方理论优势证据。
