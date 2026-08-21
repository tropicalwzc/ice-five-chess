# 5.8.1 详细标准测试交付包

本目录保存 UI 五星模型 `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` 对冻结三星、四星和 exact 5.4.1 的标准测试证据。每个对手 100 局，5.8.1 执黑 50 局、执白 50 局；三组共 300 局。

首要阅读文件：`report/v581_detailed_standard_report.md`。

- `raw/`：三组逐局、逐步 JSONL 原始日志。
- `replay/`：三组独立重放与证书重证结果。
- `report/`：中文详细报告、结构化 summary 和 150 个逐开局配对记录。
- `provenance/`：完整 profile headers、复现命令、引擎/runner/replay/report 源码及重放动态库。
- `checksums.sha256`：交付包全部文件（不含自身）的 SHA-256。

结果摘要：对三星 63/0/37，对四星 56/0/44，对 exact 5.4.1 为 52/0/48。5.8.1 在 300 局中没有决策超过 5,000 ms；唯一硬门槛超时来自 exact 5.4.1 对照侧。三组 300 局全部重放，共重新证明 805 个 early/final guard 证书。
