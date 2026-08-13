#!/usr/bin/env python3
"""Combine benchmark shards and generate reproducible JSON/Markdown reports."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import math
import os
import platform
import statistics
import subprocess
from pathlib import Path
from typing import Any, Iterable


def percentile(values: list[float], probability: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    position = (len(ordered) - 1) * probability
    lower = math.floor(position)
    upper = math.ceil(position)
    if lower == upper:
        return ordered[lower]
    return ordered[lower] + (ordered[upper] - ordered[lower]) * (position - lower)


def wilson_interval(successes: float, total: int, z: float = 1.959963984540054) -> tuple[float, float]:
    if total == 0:
        return (0.0, 0.0)
    proportion = successes / total
    denominator = 1.0 + z * z / total
    center = (proportion + z * z / (2.0 * total)) / denominator
    margin = z * math.sqrt(
        proportion * (1.0 - proportion) / total + z * z / (4.0 * total * total)
    ) / denominator
    return (max(0.0, center - margin), min(1.0, center + margin))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_value(root: Path, *arguments: str) -> str:
    try:
        return subprocess.check_output(
            ["git", *arguments], cwd=root, text=True, stderr=subprocess.DEVNULL
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def load_records(paths: Iterable[Path]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    headers: list[dict[str, Any]] = []
    games: list[dict[str, Any]] = []
    for path in paths:
        with path.open(encoding="utf-8") as handle:
            for line_number, line in enumerate(handle, 1):
                if not line.strip():
                    continue
                record = json.loads(line)
                if record.get("type") == "header":
                    record["sourceFile"] = str(path)
                    headers.append(record)
                elif record.get("type") == "game":
                    record["sourceFile"] = str(path)
                    games.append(record)
                else:
                    raise ValueError(f"{path}:{line_number}: unknown record type")
    games.sort(key=lambda game: (game["openingId"], -game["newColor"]))
    return headers, games


def result_stats(games: list[dict[str, Any]]) -> dict[str, Any]:
    wins = sum(game["winner"] == game["newColor"] for game in games)
    draws = sum(game["winner"] == 0 for game in games)
    losses = len(games) - wins - draws
    score = wins + draws * 0.5
    low, high = wilson_interval(score, len(games))
    return {
        "games": len(games),
        "wins": wins,
        "draws": draws,
        "losses": losses,
        "winRate": wins / len(games) if games else 0.0,
        "drawRate": draws / len(games) if games else 0.0,
        "lossRate": losses / len(games) if games else 0.0,
        "scoreRate": score / len(games) if games else 0.0,
        "wilson95": [low, high],
    }


def performance_stats(games: list[dict[str, Any]], engine: str) -> dict[str, Any]:
    steps = [
        step
        for game in games
        for step in game["steps"]
        if step["engine"] == engine
    ]
    times = [float(step["ms"]) for step in steps]
    nodes = [float(step["nodes"]) for step in steps]
    depths = [float(step["depth"]) for step in steps]
    return {
        "moves": len(steps),
        "timeMs": {
            "p50": percentile(times, 0.50),
            "p95": percentile(times, 0.95),
            "max": max(times, default=0.0),
            "mean": statistics.fmean(times) if times else 0.0,
        },
        "nodes": {
            "p50": percentile(nodes, 0.50),
            "p95": percentile(nodes, 0.95),
            "max": max(nodes, default=0.0),
            "mean": statistics.fmean(nodes) if nodes else 0.0,
        },
        "averageDepth": statistics.fmean(depths) if depths else 0.0,
        "budgetExhaustions": sum(bool(step["budgetExhausted"]) for step in steps),
        "transpositionHits": sum(int(step["hits"]) for step in steps),
    }


def validate_games(headers: list[dict[str, Any]], games: list[dict[str, Any]], require_final: bool) -> None:
    if not headers or not games:
        raise ValueError("benchmark input is empty")
    anomalies = [game for game in games if game.get("anomaly") is not None]
    if anomalies:
        raise ValueError(f"{len(anomalies)} anomalous games found; final report is invalid")
    identities = [(game["openingId"], game["newColor"]) for game in games]
    if len(identities) != len(set(identities)):
        raise ValueError("duplicate opening/color games found")
    profile_versions = {
        (header["newProfile"]["name"], header["newProfile"]["version"])
        for header in headers
    }
    master_seeds = {header["masterSeed"] for header in headers}
    suites = {header.get("suite", "legacy-unspecified") for header in headers}
    rules = {
        (header["forbiddenBlack"], header["maxMoves"], header.get("randomMode"))
        for header in headers
    }
    if len(profile_versions) != 1 or len(master_seeds) != 1 or len(suites) != 1 or len(rules) != 1:
        raise ValueError("input shards do not share profile, seed domain, rules, and budget")
    if require_final:
        if suites != {"final"}:
            raise ValueError(f"final report requires suite=final, got {sorted(suites)}")
        if master_seeds != {"0xf1ce5eed20260814"}:
            raise ValueError(f"unexpected final master seed: {sorted(master_seeds)}")
        if {header.get("seedDomain") for header in headers} != {"final-v2"}:
            raise ValueError("final report requires held-out seed domain final-v2")
        if {header.get("randomMode") for header in headers} != {"user-softmax"}:
            raise ValueError("final report requires seeded user-softmax mode")
        if len(games) != 200:
            raise ValueError(f"final report requires 200 games, got {len(games)}")
        black = [game for game in games if game["newColor"] == 1]
        white = [game for game in games if game["newColor"] == -1]
        if len(black) != 100 or len(white) != 100:
            raise ValueError(f"expected new black/white 100/100, got {len(black)}/{len(white)}")
        for opening_id in range(100):
            colors = {game["newColor"] for game in games if game["openingId"] == opening_id}
            if colors != {1, -1}:
                raise ValueError(f"opening {opening_id} is not a complete color-swapped pair")


def result_row(label: str, stats: dict[str, Any]) -> str:
    low, high = stats["wilson95"]
    return (
        f"| {label} | {stats['games']} | {stats['wins']} | {stats['draws']} | "
        f"{stats['losses']} | {stats['scoreRate']:.1%} | {low:.1%}–{high:.1%} |"
    )


def performance_row(label: str, stats: dict[str, Any]) -> str:
    return (
        f"| {label} | {stats['moves']} | {stats['timeMs']['p50']:.3f} | "
        f"{stats['timeMs']['p95']:.3f} | {stats['timeMs']['max']:.3f} | "
        f"{stats['nodes']['p50']:.0f} | {stats['nodes']['p95']:.0f} | "
        f"{stats['averageDepth']:.2f} | {stats['budgetExhaustions']} | "
        f"{stats['transpositionHits']} |"
    )


def representative_game(games: list[dict[str, Any]], new_wins: bool) -> dict[str, Any] | None:
    filtered = [
        game for game in games
        if (game["winner"] == game["newColor"]) == new_wins and game["winner"] != 0
    ]
    return min(filtered, key=lambda game: game["moveCount"], default=None)


def move_text(game: dict[str, Any] | None, limit: int = 30) -> str:
    if game is None:
        return "无"
    rendered = [
        f"{'B' if side == 1 else 'W'}({x + 1},{y + 1})"
        for x, y, side in game["moves"][:limit]
    ]
    suffix = " …" if len(game["moves"]) > limit else ""
    return " ".join(rendered) + suffix


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", action="append", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--label", default="three-star-strength")
    parser.add_argument("--require-final", action="store_true")
    args = parser.parse_args()

    paths = [path.resolve() for path in args.input]
    headers, games = load_records(paths)
    validate_games(headers, games, args.require_final)

    root = Path(__file__).resolve().parent.parent
    args.output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    prefix = f"{args.label}_{timestamp}"
    raw_path = args.output_dir / f"{prefix}_raw.json"
    summary_path = args.output_dir / f"{prefix}_summary.json"
    report_path = args.output_dir / f"{prefix}.md"

    raw_document = {"headers": headers, "games": games}
    raw_path.write_text(json.dumps(raw_document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    raw_digest = sha256(raw_path)

    overall = result_stats(games)
    new_black = result_stats([game for game in games if game["newColor"] == 1])
    new_white = result_stats([game for game in games if game["newColor"] == -1])
    new_performance = performance_stats(games, "new")
    legacy_performance = performance_stats(games, "legacy")
    anomalies = [game for game in games if game.get("anomaly") is not None]
    new_steps = [
        step for game in games for step in game["steps"] if step["engine"] == "new"
    ]
    hint_recorded = [step for step in new_steps if "choseHint" in step]
    hint_choices = sum(bool(step.get("choseHint")) for step in hint_recorded)
    safety_overrides = len(hint_recorded) - hint_choices

    summary = {
        "generatedAtUTC": timestamp,
        "gameCount": len(games),
        "results": {"overall": overall, "newBlack": new_black, "newWhite": new_white},
        "performance": {"new": new_performance, "legacy": legacy_performance},
        "anomalyCount": len(anomalies),
        "safetyGate": {
            "recordedMoves": len(hint_recorded),
            "legacyHintChoices": hint_choices,
            "overrides": safety_overrides,
        },
        "rawFile": raw_path.name,
        "rawSha256": raw_digest,
    }
    summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    header = headers[0]
    profile = header["newProfile"]
    commit = git_value(root, "rev-parse", "HEAD")
    dirty = git_value(root, "status", "--short")
    command_opening_start = 0 if args.require_final else header["openingStart"]
    command_opening_count = 100 if args.require_final else header["openingCount"]
    command_profile = "production" if args.require_final else profile["name"].split("-")[-1]
    command = (
        "BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)\n"
        "# 每个 opening 内自动交换 new/legacy 颜色\n"
        f'"$BENCHMARK_PATH" --profile {command_profile} '
        f'--suite {header.get("suite", "smoke")} '
        f'--random-mode {"user" if header.get("randomMode") == "user-softmax" else "best"} '
        f"--seed {header['masterSeed']} --opening-start {command_opening_start} "
        f"--opening-count {command_opening_count} "
        f"--max-moves {header['maxMoves']} --forbidden-black "
        f"{1 if header['forbiddenBlack'] else 0} --output final.jsonl"
    )
    shortest_win = representative_game(games, True)
    shortest_loss = representative_game(games, False)
    opening_count = len({game["openingId"] for game in games})
    suite = header.get("suite", "legacy-unspecified")
    report = f"""# 五子棋三星 AI 新旧版本强度测试报告

## 结论

新三星与旧三星完成 {len(games)} 局配对对抗。新三星总体战绩为 {overall['wins']} 胜、{overall['draws']} 和、{overall['losses']} 负，得分率 {overall['scoreRate']:.1%}，Wilson 95% 区间为 {overall['wilson95'][0]:.1%}–{overall['wilson95'][1]:.1%}。{'结果未显示相对旧三星的统计显著棋力提升；本轮验证的是安全随机与搜索架构不退化。' if overall['wilson95'][0] <= 0.5 <= overall['wilson95'][1] else '结果显示新旧算法存在可测的总体差异。'}

## 比赛设置

- 新算法：`{profile['name']}@{profile['version']}`
- 旧算法：`{header['legacyProfile']}`，源代码基线 `5224020`
- 棋盘：15×15；黑棋为 `1`，白棋为 `-1`
- 禁手：{'启用' if header['forbiddenBlack'] else '关闭'}
- 主种子：`{header['masterSeed']}`
- 赛程域：`{suite}` / `{header.get('seedDomain', '未记录')}`
- 决策模式：`{header.get('randomMode', '未记录')}`
- 开局：{opening_count} 个{'最终保留' if suite == 'final' else '测试/训练'}开局；每个开局交换新旧算法颜色
- 最大手数：{header['maxMoves']}
- 新三星执黑：{new_black['games']} 局；新三星执白：{new_white['games']} 局
- 异常局：{len(anomalies)}

## 强度结果

| 分组 | 局数 | 新三星胜 | 和 | 新三星负 | 得分率 | Wilson 95% |
|---|---:|---:|---:|---:|---:|---:|
{result_row('总体', overall)}
{result_row('新三星执黑', new_black)}
{result_row('新三星执白', new_white)}

得分按胜 1 分、和 0.5 分、负 0 分计算。区间用于表达 {len(games)} 局样本下的不确定性，不将细小差异解释为确定提升。

## 搜索与性能

| 引擎 | 落子数 | 耗时 p50(ms) | 耗时 p95(ms) | 最大耗时(ms) | 节点 p50 | 节点 p95 | 平均完成深度 | 预算耗尽 | TT 命中 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
{performance_row('新三星', new_performance)}
{performance_row('旧三星', legacy_performance)}

旧三星没有节点/深度埋点，因此对应节点和深度显示为 0；耗时仍使用相同的单步墙钟计量。

## 随机性与公平性

- 用户模式的旧三星建议使用每局独立 Park–Miller 随机上下文，并经过新引擎合法性与立即失败安全门控；当需要由新搜索选择时，仅在安全近优候选间 Softmax 抽样，立即获胜和唯一必要防守保持确定性。
- 正式赛记录了 {len(hint_recorded)} 次新算法落子：沿用安全旧建议 {hint_choices} 次，安全门控覆盖 {safety_overrides} 次。
- 本次评测为每个开局、手数、颜色和引擎派生独立固定种子，重复运行可复现。
- 调参使用 `training-v1` 种子域；首轮 `final-v1` 已作为失败诊断集封存；正式验收仅接受新的 `final-v2` 主种子和 opening 0..99，生成开局与逐步决策种子均不重叠。
- 两个颜色使用同一批开局并交换算法，降低先手和开局差异造成的偏差。

## 代表棋局

- 最短新三星胜局：{move_text(shortest_win)}
- 最短新三星负局：{move_text(shortest_loss)}

坐标为 1-based，`B` 表示黑棋，`W` 表示白棋；完整棋谱位于原始数据。

## 运行环境与版本

- 系统：`{platform.platform()}`
- Python：`{platform.python_version()}`
- 代码提交：`{commit}`
- 工作区状态：{'clean' if not dirty else '包含本次未提交实现变更'}
- 新三星参数：`{json.dumps(profile, ensure_ascii=False, separators=(',', ':'))}`

## 复现命令

```sh
{command}
```

## 原始数据与校验

- 原始逐局数据：`{raw_path.name}`
- 汇总数据：`{summary_path.name}`
- 原始数据 SHA-256：`{raw_digest}`
- 输入分片：{', '.join(f'`{path.name}`' for path in paths)}

## 异常说明

{'未发现非法落子、状态污染、崩溃或未记录的超时。' if not anomalies else f'发现 {len(anomalies)} 局异常，本报告不可作为正式验收结果。'}

## 参数实验结论

训练域比较了 8/8/10、10/10/12、12/12/14、不同 Top-K/权重/温度，以及 4 层窄候选和三路随机建议集成。更深或更宽组均未超过旧三星，部分组 p95 达到 850–1500ms，因此未上线。首轮纯新搜索 `final-v1` 得分率为 41.5%，已作废并封存；本报告的 `final-v2` 使用保守安全门控配置。结论是随机源隔离、安全性、可复现性和可观测性已改善，但当前测试没有证明三星棋力高于旧版本。
"""
    report_path.write_text(report, encoding="utf-8")
    print(report_path)
    print(raw_path)
    print(summary_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
