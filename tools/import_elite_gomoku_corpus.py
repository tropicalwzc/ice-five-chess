#!/usr/bin/env python3
"""Build rule-partitioned elite opening evidence for the 15x15 app.

Official raw PSQ archives remain research inputs.  The shipped C include holds
only aggregated exact-position and compact local-shape facts.  Early 20x20
Freestyle decisions are admitted only when the complete opening shape and move
are edge-independent and losslessly embeddable on a 15x15 board.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import re
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

TARGET_BOARD = 15
SOURCE_MARGIN = 2
TARGET_MARGIN = 2
MAX_STONES = 28
POST_ASSIGNMENT_HORIZON = 16
LOCAL_RADIUS = 5
LOCAL_REQUIRED_STONES = 6
LOCAL_MIN_STONES = 4
EXACT_SINGLE_EVENT_GAMES = 6
LOCAL_MULTI_EVENT_GAMES = 3
LOCAL_SINGLE_EVENT_GAMES = 10
MOVE_RE = re.compile(r"^(\d+),(\d+),(-?\d+)$")
HEADER_RE = re.compile(r"^Piskvorky (15|20)x\1\b")

RULE_FREESTYLE = 0
RULE_FORBIDDEN = 1

FROZEN_ARCHIVE_SHA256 = {
    2020: "5b32328f688831b0e889683a862277478e4ae5fad82955991068d4e6fc703e49",
    2021: "75647a9024c9dce50129cc4d9bb9464b342dd64d9359d3f86220ba92abd92b7d",
    2022: "1367336fa707245ffe1897bb90b6847093c8b75d80f00c5678855533bc0535c5",
    2023: "ebb01a6f27fbdf3c189f203500b2c7d34e417fd446980bf48d6aceeca3eba411",
    2024: "f6b45f816fd9fe3f94ea751a48b1c3a7db0a52ea3f806b4def1f12464a43eab6",
    2025: "91fc8cbefee5918994e028dd3f17143c5a46b948ed09b21c10ac0c1f1d4a8f09",
    2026: "d14c1acc0daae22a2f7589ca262abb6f1622c27f1f88ec811814cef4c948bf25",
}


@dataclass(frozen=True)
class Division:
    name: str
    rule: int
    board_size: int


@dataclass(frozen=True)
class Game:
    year: int
    division: str
    rule: int
    board_size: int
    member: str
    players: tuple[str, str]
    moves: tuple[tuple[int, int, int], ...]
    assignment_plies: int
    winner: int
    canonical_id: str


def divisions_for_year(year: int) -> tuple[Division, ...]:
    freestyle = "Freestyle15_1" if year >= 2024 else "Freestyle1"
    freestyle_size = 15 if year >= 2024 else 20
    return (
        Division(freestyle, RULE_FREESTYLE, freestyle_size),
        Division("Renju", RULE_FORBIDDEN, 15),
    )


def transform(board_size: int, t: int, x: int, y: int) -> tuple[int, int]:
    n = board_size - 1
    return (
        (x, y), (y, n - x), (n - x, n - y), (n - y, x),
        (n - x, y), (x, n - y), (y, x), (n - y, n - x),
    )[t & 7]


def transform_offset(t: int, dx: int, dy: int) -> tuple[int, int]:
    return (
        (dx, dy), (dy, -dx), (-dx, -dy), (-dy, dx),
        (-dx, dy), (dx, -dy), (dy, dx), (-dy, -dx),
    )[t & 7]


def has_five(board: dict[tuple[int, int], int], x: int, y: int, side: int) -> bool:
    for dx, dy in ((1, 0), (0, 1), (1, 1), (1, -1)):
        count = 1
        for sign in (-1, 1):
            px, py = x + sign * dx, y + sign * dy
            while board.get((px, py)) == side:
                count += 1
                px, py = px + sign * dx, py + sign * dy
        if count >= 5:
            return True
    return False


def line_length(board: dict[tuple[int, int], int], x: int, y: int,
                dx: int, dy: int, side: int) -> int:
    count = 1
    for sign in (-1, 1):
        px, py = x + sign * dx, y + sign * dy
        while board.get((px, py)) == side:
            count += 1
            px, py = px + sign * dx, py + sign * dy
    return count


def project_forbidden_legal(board: dict[tuple[int, int], int], x: int, y: int,
                            side: int, board_size: int) -> bool:
    if (x, y) in board or not (0 <= x < board_size and 0 <= y < board_size):
        return False
    if side != 1:
        return True
    board[(x, y)] = side
    directions = ((1, 0), (0, 1), (1, 1), (1, -1))
    if any(line_length(board, x, y, dx, dy, side) > 5 for dx, dy in directions):
        del board[(x, y)]
        return False
    if has_five(board, x, y, side):
        del board[(x, y)]
        return True
    winning_directions = 0
    for dx, dy in directions:
        continuation = False
        for offset in range(-4, 5):
            px, py = x + offset * dx, y + offset * dy
            if not (0 <= px < board_size and 0 <= py < board_size) or (px, py) in board:
                continue
            board[(px, py)] = side
            if line_length(board, px, py, dx, dy, side) >= 5:
                continuation = True
            del board[(px, py)]
        if continuation:
            winning_directions += 1
    del board[(x, y)]
    return winning_directions < 2


def canonical_game(game: Game | None, board_size: int, rule: int,
                   moves: Iterable[tuple[int, int, int]]) -> str:
    del game
    variants = []
    for t in range(8):
        payload = bytearray((board_size, rule))
        for x, y, side in moves:
            tx, ty = transform(board_size, t, x, y)
            payload.extend((tx, ty, 1 if side == 1 else 2))
        variants.append(bytes(payload))
    return hashlib.sha256(min(variants)).hexdigest()


def canonical_position(board: dict[tuple[int, int], int], side: int,
                       rule: int, source_size: int) -> tuple[bytes, int, int, int]:
    variants: list[tuple[bytes, int, int, int]] = []
    for t in range(8):
        occupied = [transform(source_size, t, x, y) for x, y in board]
        min_x = min(x for x, _ in occupied)
        min_y = min(y for _, y in occupied)
        cells = bytearray(TARGET_BOARD * TARGET_BOARD + 2)
        for (x, y), value in board.items():
            tx, ty = transform(source_size, t, x, y)
            cells[(tx - min_x) * TARGET_BOARD + ty - min_y] = 1 if value == 1 else 2
        cells[-2] = 1 if side == 1 else 2
        cells[-1] = rule
        variants.append((bytes(cells), t, min_x, min_y))
    return min(variants)


def canonical_local_pattern(board: dict[tuple[int, int], int], x: int, y: int,
                            side: int, rule: int) -> tuple[tuple[int, int, int], ...] | None:
    nearby = []
    for (bx, by), value in board.items():
        dx, dy = bx - x, by - y
        distance = max(abs(dx), abs(dy))
        if distance <= LOCAL_RADIUS:
            nearby.append((distance, abs(dx) + abs(dy), dx, dy, value))
    nearby.sort()
    selected = nearby[:LOCAL_REQUIRED_STONES]
    if len(selected) < LOCAL_MIN_STONES:
        return None
    variants = []
    for t in range(8):
        stones = []
        for _, _, dx, dy, value in selected:
            tx, ty = transform_offset(t, dx, dy)
            stones.append((tx, ty, value))
        stones.sort()
        variants.append(tuple(stones))
    # Side and rule are part of the aggregation key rather than the stone list.
    del side, rule
    return min(variants)


def boundary_class(board_size: int, x: int, y: int) -> int:
    edge = min(x, y, board_size - 1 - x, board_size - 1 - y)
    return 0 if edge >= LOCAL_RADIUS else 1


def opening_is_embeddable(board: dict[tuple[int, int], int], x: int, y: int,
                          source_size: int) -> tuple[bool, str]:
    points = list(board) + [(x, y)]
    if not board:
        return False, "empty_position"
    if len(board) > MAX_STONES:
        return False, "beyond_opening_horizon"
    if min(min(px, py, source_size - 1 - px, source_size - 1 - py)
           for px, py in points) < SOURCE_MARGIN:
        return False, "source_edge_sensitive"
    span_x = max(px for px, _ in points) - min(px for px, _ in points)
    span_y = max(py for _, py in points) - min(py for _, py in points)
    maximum_span = TARGET_BOARD - 1 - 2 * TARGET_MARGIN
    if span_x > maximum_span or span_y > maximum_span:
        return False, "not_embeddable_on_15x15"
    return True, "accepted"


def fnv(payload: bytes, seed: int) -> int:
    value = seed
    for byte in payload:
        value ^= byte
        value = (value * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def parse_psq(year: int, division: Division, member: str, payload: bytes) -> Game:
    text = payload.decode("utf-8-sig", errors="strict")
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    match = HEADER_RE.match(lines[0]) if lines else None
    if not match or int(match[1]) != division.board_size:
        raise ValueError("header_or_board_size")
    raw_moves: list[tuple[int, int, int]] = []
    cursor = 1
    while cursor < len(lines):
        move_match = MOVE_RE.match(lines[cursor])
        if not move_match:
            break
        x, y, elapsed = map(int, move_match.groups())
        if not (1 <= x <= division.board_size and 1 <= y <= division.board_size and elapsed >= 0):
            raise ValueError("coordinate_or_time")
        raw_moves.append((x - 1, y - 1, elapsed))
        cursor += 1
    player_lines = [line for line in lines[cursor:] if line.lower().endswith(".zip")]
    if len(player_lines) < 2:
        raise ValueError("missing_identity")
    players = tuple(line[:-4].upper() for line in player_lines[:2])
    if len(raw_moves) < 2:
        raise ValueError("incomplete_game")
    assignment = 0
    while assignment < len(raw_moves) and raw_moves[assignment][2] == 0:
        assignment += 1
    if assignment == len(raw_moves):
        raise ValueError("assignment_only")

    board: dict[tuple[int, int], int] = {}
    winner = 0
    moves: list[tuple[int, int, int]] = []
    for ply, (x, y, _) in enumerate(raw_moves):
        side = 1 if ply % 2 == 0 else -1
        if (x, y) in board:
            raise ValueError("occupied_coordinate")
        if winner:
            raise ValueError("move_after_terminal")
        if division.rule == RULE_FORBIDDEN and not project_forbidden_legal(
                board, x, y, side, division.board_size):
            raise ValueError("project_forbidden_incompatible")
        board[(x, y)] = side
        moves.append((x, y, side))
        if has_five(board, x, y, side):
            winner = side
    identity = canonical_game(None, division.board_size, division.rule, moves)
    return Game(year, division.name, division.rule, division.board_size, member,
                players, tuple(moves), assignment, winner, identity)


def parse_ratings(payload: bytes) -> list[dict[str, object]]:
    records = []
    pattern = re.compile(r"^\s*(\d+)\s+\d+#(\S+)\s+(\d+)\s+")
    for line in payload.decode("utf-8-sig").splitlines():
        match = pattern.match(line)
        if match:
            records.append({"rank": int(match[1]), "engine": match[2].upper(),
                            "elo": int(match[3])})
    if len(records) < 5:
        raise ValueError("ratings_have_fewer_than_five_engines")
    return records


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def trust_tier(games: int, events: int, local: bool) -> int:
    if events >= 2 and games >= (LOCAL_MULTI_EVENT_GAMES if local else 2):
        return 2
    threshold = LOCAL_SINGLE_EVENT_GAMES if local else EXACT_SINGLE_EVENT_GAMES
    if events == 1 and games >= threshold:
        return 1
    return 0


def render_asset(path: Path, positions: list[dict[str, object]],
                 candidates: list[dict[str, object]], local_patterns: list[dict[str, object]],
                 local_stones: list[dict[str, int]]) -> None:
    lines = [
        "/* Deterministically generated by tools/import_elite_gomoku_corpus.py.",
        " * Aggregated facts only; no complete third-party game records. */",
        "typedef struct {",
        "    uint64_t keyA; uint64_t keyB; unsigned short candidateStart;",
        "    unsigned char candidateCount; unsigned char stoneCount; unsigned char rule;",
        "} FCEliteCorpusPosition;",
        "typedef struct {",
        "    signed char x; signed char y; unsigned short games;",
        "    unsigned char events; unsigned char sources; unsigned char trustTier; unsigned char sourceBoardMask;",
        "    unsigned short wins; unsigned short draws; unsigned short losses;",
        "} FCEliteCorpusCandidate;",
        "typedef struct { signed char dx; signed char dy; signed char side; } FCEliteLocalStone;",
        "typedef struct {",
        "    unsigned short stoneStart; unsigned short games; unsigned short wins; unsigned short draws; unsigned short losses;",
        "    unsigned char stoneCount; unsigned char events; unsigned char sources; unsigned char rule;",
        "    signed char side; unsigned char trustTier; unsigned char sourceBoardMask; unsigned char boundaryClass;",
        "} FCEliteLocalPattern;", "",
        "static const FCEliteCorpusPosition fcEliteCorpusPositions[] = {",
    ]
    for item in positions:
        lines.append(f"    {{UINT64_C(0x{item['keyA']:016x}), UINT64_C(0x{item['keyB']:016x}), "
                     f"{item['candidateStart']}, {item['candidateCount']}, {item['stoneCount']}, {item['rule']}}},")
    lines += ["};", "", "static const FCEliteCorpusCandidate fcEliteCorpusCandidates[] = {"]
    for item in candidates:
        lines.append(f"    {{{item['x']}, {item['y']}, {item['games']}, {item['events']}, {item['sources']}, "
                     f"{item['trustTier']}, {item['sourceBoardMask']}, {item['wins']}, {item['draws']}, {item['losses']}}},")
    lines += ["};", "", "static const FCEliteLocalStone fcEliteLocalStones[] = {"]
    for item in local_stones:
        lines.append(f"    {{{item['dx']}, {item['dy']}, {item['side']}}},")
    lines += ["};", "", "static const FCEliteLocalPattern fcEliteLocalPatterns[] = {"]
    for item in local_patterns:
        lines.append(f"    {{{item['stoneStart']}, {item['games']}, {item['wins']}, {item['draws']}, {item['losses']}, "
                     f"{item['stoneCount']}, {item['events']}, {item['sources']}, {item['rule']}, {item['side']}, "
                     f"{item['trustTier']}, {item['sourceBoardMask']}, {item['boundaryClass']}}},")
    lines += [
        "};", "",
        "static const int fcEliteCorpusPositionCount = (int)(sizeof(fcEliteCorpusPositions) / sizeof(fcEliteCorpusPositions[0]));",
        "static const int fcEliteCorpusCandidateCount = (int)(sizeof(fcEliteCorpusCandidates) / sizeof(fcEliteCorpusCandidates[0]));",
        "static const int fcEliteLocalPatternCount = (int)(sizeof(fcEliteLocalPatterns) / sizeof(fcEliteLocalPatterns[0]));", "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8")


def render_diagnostic_positions(path: Path, fixtures: list[dict[str, object]]) -> None:
    lines = [
        "/* Aggregated-position diagnostics; not a runtime game corpus. */",
        "typedef struct { unsigned char length; unsigned char rule; signed char coordinates[56]; } FCEliteDiagnosticPosition;",
        "static const FCEliteDiagnosticPosition fcEliteDiagnosticPositions[] = {",
    ]
    for fixture in fixtures:
        black = [(x, y) for x, y, side in fixture["stones"] if side == 1]
        white = [(x, y) for x, y, side in fixture["stones"] if side == -1]
        ordered = []
        for index in range(max(len(black), len(white))):
            if index < len(black): ordered.append(black[index])
            if index < len(white): ordered.append(white[index])
        values = ", ".join(str(v) for point in ordered for v in point)
        lines.append(f"    {{{len(ordered)}, {fixture['rule']}, {{{values}}}}},")
    lines += ["};", "static const int fcEliteDiagnosticPositionCount = "
              "(int)(sizeof(fcEliteDiagnosticPositions) / sizeof(fcEliteDiagnosticPositions[0]));", ""]
    path.write_text("\n".join(lines), encoding="utf-8")


def evidence_bucket() -> dict[str, object]:
    return {"games": set(), "events": set(), "sources": set(), "boards": set(),
            "wins": 0, "draws": 0, "losses": 0}


def add_evidence(bucket: dict[str, object], game: Game, side: int) -> None:
    bucket["games"].add(game.canonical_id)
    bucket["events"].add(f"Gomocup-{game.year}-{game.division}")
    bucket["sources"].add("gomocup.org")
    bucket["boards"].add(game.board_size)
    if game.winner == side:
        bucket["wins"] += 1
    elif game.winner == 0:
        bucket["draws"] += 1
    else:
        bucket["losses"] += 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", action="append", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--asset", type=Path, required=True)
    parser.add_argument("--diagnostic-asset", type=Path, required=True)
    args = parser.parse_args()
    if len(args.package) != len(FROZEN_ARCHIVE_SHA256):
        raise ValueError("exactly the frozen 2020-2026 packages are required")

    packages = []
    admitted: list[Game] = []
    exclusions: collections.Counter[str] = collections.Counter()
    seen: dict[tuple[int, str], Game] = {}
    duplicate_refs: list[dict[str, str]] = []
    allowlists: dict[str, list[str]] = {}
    seen_years = set()
    for package in sorted(args.package):
        year_match = re.search(r"(2020|2021|2022|2023|2024|2025|2026)", package.name)
        if not year_match:
            raise ValueError(f"cannot determine year from {package}")
        year = int(year_match[1])
        if year in seen_years:
            raise ValueError(f"duplicate package year {year}")
        seen_years.add(year)
        package_payload = package.read_bytes()
        archive_hash = sha256_bytes(package_payload)
        if archive_hash != FROZEN_ARCHIVE_SHA256[year]:
            raise ValueError(f"{year} archive hash differs from frozen official package")
        division_records = []
        with zipfile.ZipFile(package) as archive:
            names = set(archive.namelist())
            for division in divisions_for_year(year):
                rating_name = f"{division.name}/ratings.txt"
                if rating_name not in names:
                    exclusions[f"missing_{division.name}"] += 1
                    continue
                ratings_payload = archive.read(rating_name)
                ratings = parse_ratings(ratings_payload)
                top_five = {str(item["engine"]) for item in ratings if int(item["rank"]) <= 5}
                allowlists[f"{year}:{division.name}"] = sorted(top_five)
                members = sorted(n for n in names if n.startswith(f"{division.name}/") and n.endswith(".psq"))
                division_games = 0
                for member in members:
                    try:
                        game = parse_psq(year, division, member, archive.read(member))
                    except (UnicodeError, ValueError) as error:
                        exclusions[f"{division.name}:{error}"] += 1
                        continue
                    duplicate_key = (division.rule, game.canonical_id)
                    if duplicate_key in seen:
                        exclusions["cross_archive_or_mirror_duplicate"] += 1
                        retained = seen[duplicate_key]
                        duplicate_refs.append({"duplicate": f"{year}:{member}",
                                               "retained": f"{retained.year}:{retained.member}"})
                        continue
                    seen[duplicate_key] = game
                    admitted.append(game)
                    division_games += 1
                division_records.append({
                    "division": division.name,
                    "rule": "Freestyle" if division.rule == RULE_FREESTYLE else "Renju/forbidden",
                    "boardSize": division.board_size,
                    "ratingsMemberSha256": sha256_bytes(ratings_payload),
                    "topFive": sorted(top_five),
                    "admittedGames": division_games,
                })
        packages.append({
            "year": year,
            "officialUrl": f"https://gomocup.org/static/tournaments/{year}/results/gomocup{year}results.zip",
            "archiveSha256": archive_hash,
            "archiveBytes": len(package_payload),
            "eventLevel": "official annual engine championship, top division",
            "divisions": division_records,
        })

    exact: dict[tuple[int, int, int, int], dict[tuple[int, int], dict[str, object]]] = {}
    exact_payloads: dict[tuple[int, int, int, int], bytes] = {}
    local: dict[tuple[int, int, int, tuple[tuple[int, int, int], ...]], dict[str, object]] = {}
    counters: collections.Counter[str] = collections.Counter()
    for game in admitted:
        top_five = set(allowlists[f"{game.year}:{game.division}"])
        board: dict[tuple[int, int], int] = {}
        for ply, (x, y, side) in enumerate(game.moves):
            decision_player = game.players[0 if side == 1 else 1]
            in_decision_horizon = (ply >= game.assignment_plies and
                                   ply < game.assignment_plies + POST_ASSIGNMENT_HORIZON and
                                   ply <= MAX_STONES)
            if in_decision_horizon and decision_player in top_five:
                embeddable, reason = opening_is_embeddable(board, x, y, game.board_size)
                if not embeddable:
                    counters[reason] += 1
                else:
                    counters["contributingEliteDecisions"] += 1
                    counters["contributingFreestyleDecisions" if game.rule == RULE_FREESTYLE
                             else "contributingForbiddenDecisions"] += 1
                    canonical, selected, shift_x, shift_y = canonical_position(
                        board, side, game.rule, game.board_size)
                    key = (fnv(canonical, 0xCBF29CE484222325),
                           fnv(canonical, 0x84222325CBF29CE4), ply, game.rule)
                    exact_payloads[key] = canonical
                    tx, ty = transform(game.board_size, selected, x, y)
                    move = (tx - shift_x, ty - shift_y)
                    bucket = exact.setdefault(key, {}).setdefault(move, evidence_bucket())
                    add_evidence(bucket, game, side)

                    pattern = canonical_local_pattern(board, x, y, side, game.rule)
                    if pattern is not None:
                        boundary = boundary_class(game.board_size, x, y)
                        local_bucket = local.setdefault((game.rule, side, boundary, pattern), evidence_bucket())
                        add_evidence(local_bucket, game, side)
                    else:
                        counters["insufficientLocalStones"] += 1
            elif ply >= game.assignment_plies and decision_player not in top_five:
                counters["nonTopFiveDecisionsExcluded"] += 1
            board[(x, y)] = side

    runtime_positions: list[dict[str, object]] = []
    runtime_candidates: list[dict[str, object]] = []
    fixtures: list[dict[str, object]] = []
    for key in sorted(exact):
        accepted = []
        for move, evidence in exact[key].items():
            games, events = len(evidence["games"]), len(evidence["events"])
            tier = trust_tier(games, events, False)
            if tier == 0:
                counters["exactLowSupportExcluded"] += 1
                continue
            accepted.append({
                "x": move[0], "y": move[1], "games": games, "events": events,
                "sources": len(evidence["sources"]), "trustTier": tier,
                "sourceBoardMask": sum(1 if size == 15 else 2 for size in evidence["boards"]),
                "wins": evidence["wins"], "draws": evidence["draws"], "losses": evidence["losses"],
            })
        if not accepted:
            continue
        accepted.sort(key=lambda item: (-item["trustTier"], -item["games"],
                                        -(item["wins"] * 2 + item["draws"]), item["x"], item["y"]))
        start = len(runtime_candidates)
        runtime_candidates.extend(accepted[:8])
        runtime_positions.append({"keyA": key[0], "keyB": key[1], "stoneCount": key[2],
                                  "rule": key[3], "candidateStart": start,
                                  "candidateCount": min(8, len(accepted))})
        if len(fixtures) < 96:
            payload = exact_payloads[key]
            occupied = [(index // TARGET_BOARD, index % TARGET_BOARD, 1 if value == 1 else -1)
                        for index, value in enumerate(payload[:TARGET_BOARD * TARGET_BOARD]) if value]
            max_x = max(x for x, _, _ in occupied)
            max_y = max(y for _, y, _ in occupied)
            offset_x = max(TARGET_MARGIN, (TARGET_BOARD - 1 - max_x) // 2)
            offset_y = max(TARGET_MARGIN, (TARGET_BOARD - 1 - max_y) // 2)
            if max_x + offset_x < TARGET_BOARD and max_y + offset_y < TARGET_BOARD:
                fixtures.append({"rule": key[3], "stones": [[x + offset_x, y + offset_y, s]
                                                              for x, y, s in occupied]})

    runtime_local: list[dict[str, object]] = []
    runtime_local_stones: list[dict[str, int]] = []
    for (rule, side, boundary, pattern), evidence in sorted(local.items()):
        games, events = len(evidence["games"]), len(evidence["events"])
        tier = trust_tier(games, events, True)
        if tier == 0:
            counters["localLowSupportExcluded"] += 1
            continue
        start = len(runtime_local_stones)
        runtime_local_stones.extend({"dx": dx, "dy": dy, "side": value}
                                    for dx, dy, value in pattern)
        runtime_local.append({
            "stoneStart": start, "stoneCount": len(pattern), "games": games,
            "events": events, "sources": len(evidence["sources"]), "rule": rule,
            "side": side, "trustTier": tier,
            "boundaryClass": boundary,
            "sourceBoardMask": sum(1 if size == 15 else 2 for size in evidence["boards"]),
            "wins": evidence["wins"], "draws": evidence["draws"], "losses": evidence["losses"],
        })
    runtime_local.sort(key=lambda item: (-item["trustTier"], -item["events"], -item["games"],
                                         item["rule"], -item["side"], item["stoneStart"]))

    for path in (args.manifest, args.audit, args.asset, args.diagnostic_asset):
        path.parent.mkdir(parents=True, exist_ok=True)
    render_asset(args.asset, runtime_positions, runtime_candidates,
                 runtime_local, runtime_local_stones)
    render_diagnostic_positions(args.diagnostic_asset, fixtures)
    audit = {
        "schemaVersion": 2, "retrievedAt": "2026-08-14",
        "sourcePriority": "official Gomocup organizer archives only",
        "sources": packages,
        "redistributionTreatment": "Raw official archives remain research-only in /private/tmp; the shipped asset contains attributed aggregate facts only.",
        "allowlistPolicy": "Exact top five by each official division's published Elo/rank; only their post-assignment decisions contribute.",
        "allowlists": allowlists,
        "rulePartitionPolicy": "Freestyle and 15x15 Renju/forbidden evidence are never pooled.",
        "twentyByTwentyPolicy": "Only early Freestyle1 shapes that include the candidate, stay at least two intersections from the source edge, and embed on 15x15 with a two-intersection target margin contribute.",
        "explicitExclusions": [
            {"class": "zero-time tournament assignment plies", "reason": "assigned start, not an engine decision"},
            {"class": "lower divisions and Fastgame", "reason": "outside frozen top-division standard-time scope"},
            {"class": "Standard/Caro records", "reason": "rule incompatible"},
            {"class": "anonymous internet/exhibition games", "reason": "identity and level not auditable"},
            {"class": "20x20 midgame/edge shapes", "reason": "not safely transferable to 15x15 opening play"},
        ],
        "recordExclusions": dict(sorted(exclusions.items())),
        "duplicates": duplicate_refs,
    }
    args.audit.write_text(json.dumps(audit, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    generation = "python3 tools/import_elite_gomoku_corpus.py " + " ".join(
        f"--package /private/tmp/gomocup{year}results.zip" for year in sorted(FROZEN_ARCHIVE_SHA256)) + \
        " --manifest tools/elite_corpus_manifest.json --audit tools/elite_source_audit.json" \
        " --asset 'ice five chess/FiveChessEliteCorpus.inc' --diagnostic-asset tools/FiveChessEliteDiagnosticPositions.inc"
    manifest = {
        "schemaVersion": 2,
        "corpusVersion": "gomocup-elite-openings-2020-2026-rule-partitioned-v2",
        "generatedAt": "2026-08-14",
        "postAssignmentHorizon": POST_ASSIGNMENT_HORIZON,
        "maximumRuntimeStones": MAX_STONES,
        "localRadius": LOCAL_RADIUS,
        "localRequiredStones": LOCAL_REQUIRED_STONES,
        "localMinimumStones": LOCAL_MIN_STONES,
        "trustTiers": {"2": "cross-event", "1": "high-repeat single-event"},
        "thresholds": {"exactSingleEventGames": EXACT_SINGLE_EVENT_GAMES,
                       "localMultiEventGames": LOCAL_MULTI_EVENT_GAMES,
                       "localSingleEventGames": LOCAL_SINGLE_EVENT_GAMES},
        "admittedGames": len(admitted),
        "admittedGamesByRule": {
            "freestyle": sum(game.rule == RULE_FREESTYLE for game in admitted),
            "forbidden": sum(game.rule == RULE_FORBIDDEN for game in admitted),
        },
        "runtimeExactPositions": len(runtime_positions),
        "runtimeExactCandidates": len(runtime_candidates),
        "runtimeLocalPatterns": len(runtime_local),
        "runtimeLocalStones": len(runtime_local_stones),
        "runtimeByRule": {
            "freestyle": {
                "exactPositions": sum(item["rule"] == RULE_FREESTYLE for item in runtime_positions),
                "exactCandidates": sum(position["candidateCount"] for position in runtime_positions
                                       if position["rule"] == RULE_FREESTYLE),
                "localPatterns": sum(item["rule"] == RULE_FREESTYLE for item in runtime_local),
            },
            "forbidden": {
                "exactPositions": sum(item["rule"] == RULE_FORBIDDEN for item in runtime_positions),
                "exactCandidates": sum(position["candidateCount"] for position in runtime_positions
                                       if position["rule"] == RULE_FORBIDDEN),
                "localPatterns": sum(item["rule"] == RULE_FORBIDDEN for item in runtime_local),
            },
        },
        "runtimeTestFixtures": fixtures,
        "recordExclusions": dict(sorted(exclusions.items())),
        "counters": dict(sorted(counters.items())),
        "sourceAuditSha256": sha256_bytes(args.audit.read_bytes()),
        "assetSha256": sha256_bytes(args.asset.read_bytes()),
        "diagnosticAssetSha256": sha256_bytes(args.diagnostic_asset.read_bytes()),
        "generationCommand": generation,
    }
    args.manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: manifest[key] for key in ("admittedGames", "runtimeExactPositions",
                                                     "runtimeExactCandidates", "runtimeLocalPatterns")},
                     ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
