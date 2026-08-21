# 5.8.1 黑方禁手规则详细测试交付包

本目录保存 UI 五星模型 `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` 在黑方禁手开启规则下，对冻结三星、四星和 exact 5.4.1 的标准测试证据。每个对手 100 局，5.8.1 执黑 50 局、执白 50 局；三组共 300 局。

首要阅读文件：`report/v581_forbidden_detailed_standard_report.md`。

- `raw/`：三组逐局、逐步 JSONL 原始日志。
- `replay/`：使用禁手合法性规则完成的三组独立重放与证书重证结果。
- `report/`：中文详细报告、结构化 summary 和 150 个逐开局配对记录。
- `provenance/`：完整 profile headers、复现命令、引擎/runner/replay/report 源码、benchmark binary 与重放动态库。
- `checksums.sha256`：交付包全部文件（不含自身）的 SHA-256。

结果摘要：对三星 60/0/40，对四星 55/0/45，对 exact 5.4.1 为 54/0/46。三组总计 169/0/131。300 局全部独立重放通过，共重新证明 705 个 early/final guard 证书。

运行时限制：5.8.1 在同一个固定局面（opening 12、执白、ply 7、`(12,5)`）跨三个对手重复出现 3 次超过 5,000ms，范围为 5,019.576–5,044.334ms；exact 5.4.1 对照另有 1 次 5,233.651ms。三星单元另有 1 局旧三星执黑尝试禁手而判负，独立重放已验证非法着未落盘及胜方正确。
