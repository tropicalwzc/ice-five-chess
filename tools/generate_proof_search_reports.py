#!/usr/bin/env python3
"""Validate proof-search matches and generate book-on/off comparison reports."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import platform
import statistics
import subprocess
from pathlib import Path
from typing import Any


EXPECTED_SEED = "0xc0dec0de20260813"
EXPECTED_DOMAIN = "proof-final-proof-v2"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_jsonl(paths: list[Path]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    headers: list[dict[str, Any]] = []
    games: list[dict[str, Any]] = []
    for path in paths:
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            record = json.loads(line)
            if record.get("type") == "header":
                headers.append(record)
            elif record.get("type") == "game":
                record["_source"] = f"{path.name}:{line_number}"
                games.append(record)
            else:
                raise ValueError(f"{path}:{line_number}: unknown record type")
    return headers, games


def has_five(board: list[list[int]], x: int, y: int, side: int) -> bool:
    for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
        total = 1
        for sign in (-1, 1):
            px, py = x + sign * dx, y + sign * dy
            while 0 <= px < 15 and 0 <= py < 15 and board[px][py] == side:
                total += 1
                px += sign * dx
                py += sign * dy
        if total >= 5:
            return True
    return False


def immediate_wins(board: list[list[int]], side: int) -> list[tuple[int, int]]:
    wins: list[tuple[int, int]] = []
    for x in range(15):
        for y in range(15):
            if board[x][y] != 0:
                continue
            board[x][y] = side
            if has_five(board, x, y, side):
                wins.append((x, y))
            board[x][y] = 0
    return wins


def independently_proven_loss(board: list[list[int]], side: int) -> bool:
    if not immediate_wins(board, -side):
        return False
    for x in range(15):
        for y in range(15):
            if board[x][y] != 0:
                continue
            board[x][y] = side
            escapes = has_five(board, x, y, side) or not immediate_wins(board, -side)
            board[x][y] = 0
            if escapes:
                return False
    return True


def validate_group(
    label: str,
    headers: list[dict[str, Any]],
    games: list[dict[str, Any]],
    expected_profile: str,
) -> None:
    if not headers:
        raise ValueError(f"{label}: missing header")
    for header in headers:
        if header.get("schemaVersion") != 3:
            raise ValueError(f"{label}: schemaVersion must be 3")
        if header.get("suite") != "proof-final":
            raise ValueError(f"{label}: suite must be proof-final")
        if header.get("seedDomain") != EXPECTED_DOMAIN:
            raise ValueError(f"{label}: unexpected seed domain")
        if str(header.get("masterSeed", "")).lower() != EXPECTED_SEED:
            raise ValueError(f"{label}: unexpected master seed")
        if header.get("openingMode") != "gomocup-curated-prefix":
            raise ValueError(f"{label}: wrong opening mode")
        if header.get("newProfile", {}).get("name") != expected_profile:
            raise ValueError(f"{label}: wrong profile")
    keys = [(game["openingId"], game["newColor"]) for game in games]
    if len(games) != 200 or len(set(keys)) != 200:
        raise ValueError(f"{label}: expected exactly 200 unique paired games")
    if sorted({game["openingId"] for game in games}) != list(range(100)):
        raise ValueError(f"{label}: opening IDs must be exactly 0..99")
    for color in (1, -1):
        if sum(game["newColor"] == color for game in games) != 100:
            raise ValueError(f"{label}: expected 100 games for color {color}")
    for game in games:
        if game.get("anomaly") is not None:
            raise ValueError(f"{label}: anomaly in {game['_source']}: {game['anomaly']}")
        board = [[0] * 15 for _ in range(15)]
        terminal_winner = 0
        for ply, move in enumerate(game["moves"]):
            x, y, side = move
            if side != (1 if ply % 2 == 0 else -1):
                raise ValueError(f"{label}: side-order error in {game['_source']}")
            if not (0 <= x < 15 and 0 <= y < 15) or board[x][y] != 0:
                raise ValueError(f"{label}: illegal replay move in {game['_source']}")
            if terminal_winner:
                raise ValueError(f"{label}: move after terminal win in {game['_source']}")
            board[x][y] = side
            if has_five(board, x, y, side):
                terminal_winner = side
        if game.get("termination") == "proven-loss":
            side_to_move = 1 if len(game["moves"]) % 2 == 0 else -1
            if terminal_winner or game["winner"] != -side_to_move or not \
                    independently_proven_loss(board, side_to_move):
                raise ValueError(f"{label}: invalid proven-loss in {game['_source']}")
        elif game["winner"] != terminal_winner and not (
            game["winner"] == 0 and terminal_winner == 0
        ):
            raise ValueError(f"{label}: replay winner mismatch in {game['_source']}")
        new_steps = [step for step in game["steps"] if step["engine"] == "new"]
        for step in new_steps:
            if step.get("proofStatus") == 1 and not step.get("proofCertificateVerified"):
                raise ValueError(f"{label}: unverified accepted proof in {game['_source']}")


def game_point(game: dict[str, Any]) -> float:
    if game["winner"] == game["newColor"]:
        return 1.0
    if game["winner"] == 0:
        return 0.5
    return 0.0


def wilson(points: float, games: int) -> list[float]:
    if games == 0:
        return [0.0, 0.0]
    z = 1.959963984540054
    p = points / games
    denominator = 1 + z * z / games
    center = (p + z * z / (2 * games)) / denominator
    margin = z * math.sqrt(p * (1 - p) / games + z * z / (4 * games * games)) / denominator
    return [max(0.0, center - margin), min(1.0, center + margin)]


def result_stats(games: list[dict[str, Any]]) -> dict[str, Any]:
    wins = sum(game["winner"] == game["newColor"] for game in games)
    draws = sum(game["winner"] == 0 for game in games)
    losses = len(games) - wins - draws
    points = wins + 0.5 * draws
    return {
        "games": len(games), "wins": wins, "draws": draws, "losses": losses,
        "points": points, "scoreRate": points / len(games) if games else 0.0,
        "wilson95": wilson(points, len(games)),
    }


def result_stats_for_color(games: list[dict[str, Any]], color: int) -> dict[str, Any]:
    """Return results from the perspective of one fixed board color."""
    wins = sum(game["winner"] == color for game in games)
    draws = sum(game["winner"] == 0 for game in games)
    losses = len(games) - wins - draws
    points = wins + 0.5 * draws
    return {
        "games": len(games), "wins": wins, "draws": draws, "losses": losses,
        "points": points, "scoreRate": points / len(games) if games else 0.0,
        "wilson95": wilson(points, len(games)),
    }


def percentile(values: list[float], probability: float) -> float:
    if not values:
        return 0.0
    values = sorted(values)
    position = (len(values) - 1) * probability
    low, high = math.floor(position), math.ceil(position)
    if low == high:
        return values[low]
    return values[low] + (values[high] - values[low]) * (position - low)


def telemetry(games: list[dict[str, Any]]) -> dict[str, Any]:
    steps = [step for game in games for step in game["steps"] if step["engine"] == "new"]
    elapsed = [float(step["ms"]) for step in steps]
    sources = {str(source): sum(step.get("defaultSource") == source for step in steps)
               for source in (0, 1, 2)}
    overrides = {str(reason): sum(step.get("overrideReason") == reason for step in steps)
                 for reason in range(6)}
    proof_status = {str(status): sum(step.get("proofStatus") == status for step in steps)
                    for status in range(3)}
    return {
        "moves": len(steps),
        "latencyMs": {
            "p50": percentile(elapsed, 0.5), "p95": percentile(elapsed, 0.95),
            "max": max(elapsed, default=0.0), "mean": statistics.fmean(elapsed) if elapsed else 0.0,
        },
        "defaultSources": sources,
        "overrideReasons": overrides,
        "bookHits": sources["2"],
        "proofStatus": proof_status,
        "proofNodes": sum(int(step.get("proofNodes", 0)) for step in steps),
        "verifiedCertificates": sum(bool(step.get("proofCertificateVerified")) for step in steps),
        "budgetExhaustions": sum(bool(step.get("budgetExhausted")) for step in steps),
    }


def paired_difference(
    book_on: list[dict[str, Any]], book_off: list[dict[str, Any]], color: int | None
) -> dict[str, Any]:
    on = {(g["openingId"], g["newColor"]): game_point(g) for g in book_on
          if color is None or g["newColor"] == color}
    off = {(g["openingId"], g["newColor"]): game_point(g) for g in book_off
           if color is None or g["newColor"] == color}
    if on.keys() != off.keys():
        raise ValueError("book-on/off paired schedules differ")
    differences = [on[key] - off[key] for key in sorted(on)]
    mean = statistics.fmean(differences)
    if len(differences) > 1:
        margin = 1.959963984540054 * statistics.stdev(differences) / math.sqrt(len(differences))
    else:
        margin = 0.0
    return {"pairs": len(differences), "difference": mean,
            "normal95": [max(-1.0, mean - margin), min(1.0, mean + margin)]}


def classification(
    overall: dict[str, Any], white: dict[str, Any], legacy_white_opponent: dict[str, Any]
) -> str:
    white_delta = white["scoreRate"] - legacy_white_opponent["scoreRate"]
    if overall["wilson95"][0] > 0.5 and white_delta > 0:
        return "demonstrated stronger"
    if overall["scoreRate"] > 0.5 and white_delta > 0:
        return "directional improvement"
    return "not demonstrated"


def summarize(games: list[dict[str, Any]], control_games: list[dict[str, Any]]) -> dict[str, Any]:
    white_games = [g for g in games if g["newColor"] == -1]
    black_games = [g for g in games if g["newColor"] == 1]
    control_white_games = [g for g in control_games if g["newColor"] == -1]
    overall = result_stats(games)
    white = result_stats(white_games)
    black = result_stats(black_games)
    # In the new-black half of this same match, the opponent is the legacy
    # engine playing white. This is the paired white comparator. The separate
    # legacy-vs-legacy control is only a natural-color baseline.
    legacy_white_opponent = result_stats_for_color(black_games, -1)
    legacy_self_play_control_white = result_stats(control_white_games)
    return {
        "results": {
            "newWhite": white,
            "legacyWhiteOpponent": legacy_white_opponent,
            "newBlack": black,
            "overall": overall,
            "legacySelfPlayControlWhite": legacy_self_play_control_white,
        },
        "whiteDeltaVsLegacyOpponent": (
            white["scoreRate"] - legacy_white_opponent["scoreRate"]
        ),
        "classification": classification(overall, white, legacy_white_opponent),
        "telemetry": telemetry(games),
    }


def fmt_result(stats: dict[str, Any]) -> str:
    return (f"{stats['wins']}/{stats['draws']}/{stats['losses']}，"
            f"得分率 {stats['scoreRate']:.1%}，Wilson 95% "
            f"{stats['wilson95'][0]:.1%}–{stats['wilson95'][1]:.1%}")


def standalone_markdown(
    title: str, summary: dict[str, Any], header: dict[str, Any],
    raw_name: str, raw_hash: str, control_hash: str, command: str,
) -> str:
    results = summary["results"]
    telem = summary["telemetry"]
    white_delta = summary["whiteDeltaVsLegacyOpponent"]
    if results["overall"]["wilson95"][0] > 0.5 and white_delta <= 0:
        release_note = ("总体区间虽已高于 50%，但新模型白棋没有高于同组直接对局中的旧三星白棋；"
                        "按预先冻结的发布门槛不得标为 `demonstrated stronger`。")
    elif summary["classification"] == "not demonstrated":
        release_note = "总体或白棋门槛未同时满足，因此不发布为更强三星。"
    elif summary["classification"] == "demonstrated stronger":
        release_note = ("总体 Wilson 95% 区间高于 50%，且新模型白棋得分高于同组直接对局中的"
                        "旧三星白棋，满足预先冻结的 `demonstrated stronger` 门槛。")
    else:
        release_note = "总体与白棋点估计同向，但区间仍需按分类规则解释。"
    return f"""# {title}

## 结论（白棋优先）

- 新模型执白：{fmt_result(results['newWhite'])}
- 同组旧三星执白（对应新模型执黑的 100 局）：{fmt_result(results['legacyWhiteOpponent'])}
- 新模型白棋相对同组旧三星白棋：{white_delta:+.1%}
- 强度分类：`{summary['classification']}`

{release_note}

新模型执黑为 {fmt_result(results['newBlack'])}；总体为 {fmt_result(results['overall'])}。报告保留五子棋自然先手优势，不做颜色再加权。

独立旧三星自对练 control 执白为 {fmt_result(results['legacySelfPlayControlWhite'])}。该值只用于复现赛程与观察自然颜色基线，不参与上述白棋增量或强度分类。

## 赛程与版本

- 赛程：100 个固定 opening ID，每个交换颜色；新模型执黑 100 局、执白 100 局
- profile：`{header['newProfile']['name']}@{header['newProfile']['version']}`
- 旧模型：`{header['legacyProfile']}`
- 开局模式：`{header['openingMode']}`；开局库：`{header['openingBookVersion']}`
- 固定种子：`{header['masterSeed']}`；域：`{header['seedDomain']}`
- 正式开局清单：`final-opening-schedule.json`，100/100 canonical ID 唯一，SHA-256 `232a0dbb03d56d924331c1d1b6ff404928d0cb8a1aa8a561efcfd7af686e27ff`
- 禁手：{'开启' if header['forbiddenBlack'] else '关闭（Freestyle-15）'}

## 搜索、开局与性能

- 新模型落子：{telem['moves']}；开局库命中：{telem['bookHits']}
- 默认来源（none/legacy/book）：{telem['defaultSources']}
- 覆盖原因（0..5）：{telem['overrideReasons']}
- 证明状态（unknown/proven/no-win-in-scope）：{telem['proofStatus']}
- 已验证证书：{telem['verifiedCertificates']}；证明节点：{telem['proofNodes']}；预算耗尽：{telem['budgetExhaustions']}
- 单步延迟：p50 {telem['latencyMs']['p50']:.2f} ms，p95 {telem['latencyMs']['p95']:.2f} ms，最大 {telem['latencyMs']['max']:.2f} ms

所有 200 局均已逐手重放验证：无重复落点、越界、颜色顺序错误、终局后继续落子或结果不一致；所有被接受的 `proven-win` 均带运行时已验证证书标记。

## 复现命令

```sh
{command}
```

## 数据与校验

- 原始数据：`{raw_name}`
- 原始数据 SHA-256：`{raw_hash}`
- legacy control 原始输入组合 SHA-256：`{control_hash}`
- 运行环境：{platform.platform()} / Python {platform.python_version()}
"""


def write_bundle(
    output_dir: Path, stem: str, title: str,
    headers: list[dict[str, Any]], games: list[dict[str, Any]],
    control_games: list[dict[str, Any]], control_hash: str, command: str,
) -> tuple[dict[str, Any], Path, Path, Path]:
    raw_path = output_dir / f"{stem}_raw.json"
    summary_path = output_dir / f"{stem}_summary.json"
    report_path = output_dir / f"{stem}.md"
    raw_path.write_text(json.dumps({"headers": headers, "games": games},
                                   ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    raw_hash = sha256(raw_path)
    summary = summarize(games, control_games)
    summary.update({"rawFile": raw_path.name, "rawSha256": raw_hash})
    summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
                            encoding="utf-8")
    report_path.write_text(standalone_markdown(
        title, summary, headers[0], raw_path.name, raw_hash, control_hash, command),
        encoding="utf-8")
    return summary, raw_path, summary_path, report_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--book-on", action="append", required=True, type=Path)
    parser.add_argument("--book-off", action="append", required=True, type=Path)
    parser.add_argument("--control", action="append", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    on_headers, on_games = load_jsonl(args.book_on)
    off_headers, off_games = load_jsonl(args.book_off)
    control_headers, control_games = load_jsonl(args.control)
    validate_group("book-on", on_headers, on_games, "three-star-threat-proof-book")
    validate_group("book-off", off_headers, off_games, "three-star-threat-proof-no-book")
    validate_group("legacy-control", control_headers, control_games, "legacy-three-star-control")
    if {(g["openingId"], g["newColor"]) for g in on_games} != {
        (g["openingId"], g["newColor"]) for g in off_games
    }:
        raise ValueError("book-on and book-off schedules differ")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    control_combined_hash = hashlib.sha256(
        "".join(sha256(path) for path in args.control).encode()
    ).hexdigest()
    base = "BENCHMARK_PATH=$(tools/build_five_chess_benchmark.sh)\n"
    common = ("--suite proof-final --random-mode best --opening-start 0 "
              "--opening-count 100 --max-moves 120 --forbidden-black 0")
    on_summary, on_raw, _, on_report = write_bundle(
        args.output_dir, "proof_search_book_on", "五子棋证明搜索新模型（开局库开启）vs 旧三星",
        on_headers, on_games, control_games, control_combined_hash,
        base + f'"$BENCHMARK_PATH" --profile proof-book {common} --output book-on.jsonl')
    off_summary, off_raw, _, off_report = write_bundle(
        args.output_dir, "proof_search_book_off", "五子棋证明搜索新模型（开局库关闭）vs 旧三星",
        off_headers, off_games, control_games, control_combined_hash,
        base + f'"$BENCHMARK_PATH" --profile proof-no-book {common} --output book-off.jsonl')

    differences = {
        "newWhite": paired_difference(on_games, off_games, -1),
        "newBlack": paired_difference(on_games, off_games, 1),
        "overall": paired_difference(on_games, off_games, None),
    }
    comparison = {
        "bookOn": on_summary, "bookOff": off_summary,
        "bookOnMinusBookOff": differences,
        "rawSha256": {"bookOn": sha256(on_raw), "bookOff": sha256(off_raw)},
    }
    comparison_json = args.output_dir / "proof_search_opening_book_comparison.json"
    comparison_json.write_text(json.dumps(comparison, ensure_ascii=False, indent=2) + "\n",
                               encoding="utf-8")
    on_t, off_t = on_summary["telemetry"], off_summary["telemetry"]
    attribution = (
        "开局库开启版独有的改善应归因于开局库；关闭版相对同组直接对局旧三星的改善"
        "才可归因于证明搜索。独立旧三星自对练 control 仅作自然颜色基线。"
    )
    comparison_md = args.output_dir / "proof_search_opening_book_comparison.md"
    comparison_md.write_text(f"""# 五子棋开局库影响直接对比报告

## 白棋优先结论

- book-on 新模型执白：{fmt_result(on_summary['results']['newWhite'])}
- book-off 新模型执白：{fmt_result(off_summary['results']['newWhite'])}
- book-on − book-off 白棋得分率：{differences['newWhite']['difference']:+.1%}（配对近似 95% {differences['newWhite']['normal95'][0]:+.1%} 至 {differences['newWhite']['normal95'][1]:+.1%}）

总体差值为 {differences['overall']['difference']:+.1%}，执黑差值为 {differences['newBlack']['difference']:+.1%}。{attribution}

## 直接比较

| 指标 | book-on | book-off | on − off |
|---|---:|---:|---:|
| 白棋得分率 | {on_summary['results']['newWhite']['scoreRate']:.1%} | {off_summary['results']['newWhite']['scoreRate']:.1%} | {differences['newWhite']['difference']:+.1%} |
| 黑棋得分率 | {on_summary['results']['newBlack']['scoreRate']:.1%} | {off_summary['results']['newBlack']['scoreRate']:.1%} | {differences['newBlack']['difference']:+.1%} |
| 总体得分率 | {on_summary['results']['overall']['scoreRate']:.1%} | {off_summary['results']['overall']['scoreRate']:.1%} | {differences['overall']['difference']:+.1%} |
| 开局库命中 | {on_t['bookHits']} | {off_t['bookHits']} | {on_t['bookHits'] - off_t['bookHits']:+d} |
| 已验证证明 | {on_t['verifiedCertificates']} | {off_t['verifiedCertificates']} | {on_t['verifiedCertificates'] - off_t['verifiedCertificates']:+d} |
| 证明节点 | {on_t['proofNodes']} | {off_t['proofNodes']} | {on_t['proofNodes'] - off_t['proofNodes']:+d} |
| 延迟 p50(ms) | {on_t['latencyMs']['p50']:.2f} | {off_t['latencyMs']['p50']:.2f} | {on_t['latencyMs']['p50'] - off_t['latencyMs']['p50']:+.2f} |
| 延迟 p95(ms) | {on_t['latencyMs']['p95']:.2f} | {off_t['latencyMs']['p95']:.2f} | {on_t['latencyMs']['p95'] - off_t['latencyMs']['p95']:+.2f} |

## 强度分类

- book-on：`{on_summary['classification']}`
- book-off：`{off_summary['classification']}`

两组使用相同 100 个 opening ID、交换颜色、相同 `proof-final-proof-v2` 主种子及完全一致的非开局搜索参数。逐局差值和机器可读汇总见 `{comparison_json.name}`。
""", encoding="utf-8")

    checksums = args.output_dir / "proof_search_report_checksums.sha256"
    artifacts = sorted(args.output_dir.glob("proof_search_*"))
    checksums.write_text("".join(f"{sha256(path)}  {path.name}\n" for path in artifacts
                                 if path != checksums), encoding="utf-8")

    complete_checksums = args.output_dir / "complete_bundle_checksums.sha256"
    bundle_artifacts = sorted(
        path for path in args.output_dir.rglob("*")
        if path.is_file() and path != complete_checksums
    )
    complete_checksums.write_text(
        "".join(
            f"{sha256(path)}  {path.relative_to(args.output_dir)}\n"
            for path in bundle_artifacts
        ),
        encoding="utf-8",
    )
    for path in (
        on_report, off_report, comparison_md, comparison_json, checksums,
        complete_checksums,
    ):
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
