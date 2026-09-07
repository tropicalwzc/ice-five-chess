# ice-five-chess

## 强力五子棋AI

<https://apps.apple.com/cn/app/ice-five-chess/id1474468171>

## 项目结构

- `ChessApp.swift`、`ChessBoard.swift`：iPhone / iPad 共用的 SwiftUI 页面和交互。
- `ChessEngine.h/.m`、`Chess-Bridging-Header.h`：SwiftUI 与棋盘算法的桥接，负责后台计算和棋局存档（含旧存档迁移）。
- `doublethree.h/.m`、`FiveChessAI.h/.c`、`FiveChess*.inc`：五子棋规则、AI 搜索和棋谱数据。
- `Assets.xcassets/AppIcon.appiconset`、`Base.lproj/LaunchScreen.storyboard`：应用图标和系统启动屏。
- `ice five chessTests`、`ice five chessUITests`、`tools`：引擎、界面和算法回归验证。

旧 Objective-C 页面及其专属资源已移除；保留的 Objective-C 代码用于算法、桥接和测试，不包含旧页面。

## 五星算法

当前唯一支持的五星版本为 `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`。旧五星工厂、颜色混合、实验调度器、分支优先搜索、恢复搜索及 5.8.2 试验入口已移除；1–4 星和现用五星依赖的共用算法保留。

历史 OpenSpec 和评测数据保留用于追溯，其中要求保留 5.4.1 回滚入口的条款已被 `prune-five-star-experimental-branches` 变更取代。复现旧版本需要检出对应 Git 历史；旧报告读取脚本不代表当前仍支持那些算法。

当前验证命令：

```sh
python3 tools/verify_five_star_ui_binding.py
python3 tools/verify_five_star_production.py
sh tools/run_five_chess_ai_tests.sh
sh tools/run_legacy_three_star_golden.sh
sh tools/build_five_chess_benchmark.sh
```

最后一个命令输出精简对战工具路径，使用 `--help` 查看参数。支持 `five-star` / `five-star-5.8.1` 和 `four-star`，不再支持历史实验版本或参数变异。该工具按实际应用的随机对局路径运行，输出新版逐步 JSONL；不是旧批量研究工具的兼容替代。固定参数/战术一致性由 `verify_five_star_production.py` 验证。
