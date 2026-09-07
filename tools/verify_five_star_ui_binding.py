#!/usr/bin/env python3
"""Verify the shared SwiftUI → ChessEngine → five-star algorithm binding."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


TARGET_FACTORY = "fc_profile_five_star_early_micro_vcf_candidate()"


def braced_block(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing signature: {signature}")
    opening = source.find("{", start + len(signature))
    if opening < 0:
        raise AssertionError(f"missing body: {signature}")
    depth = 0
    for index in range(opening, len(source)):
        character = source[index]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated body: {signature}")


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--root", type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repository root",
    )
    args = parser.parse_args()
    root = args.root.resolve()

    doublethree = (root / "ice five chess/doublethree.m").read_text()
    app = (root / "ice five chess/ChessApp.swift").read_text()
    bridge = (root / "ice five chess/ChessEngine.m").read_text()
    engine = (root / "ice five chess/FiveChessAI.c").read_text()

    five_star = braced_block(
        doublethree, "-(void) five_star_analysisboard:(int) mode")
    four_star = braced_block(
        doublethree, "-(void) four_star_analysisboard:(int) mode")
    target_profile = braced_block(
        engine, "FCAIProfile fc_profile_five_star_early_micro_vcf_candidate(void)")
    errors: list[str] = []
    require(TARGET_FACTORY in five_star,
            "playable five-star method does not select the 5.8.1 factory",
            errors)
    require("fc_profile_five_star_proof_engine_candidate" not in engine,
            "retired 5.4.1 factory is still present", errors)
    require("fc_profile_proof_guided(false)" in four_star,
            "four-star method no longer selects proof-guided no-book", errors)

    analyze = braced_block(bridge, "- (void)analyzeWithCompletion:")
    swift_analyze = braced_block(app, "func analyze()")
    difficulty_menu = braced_block(app, "private var difficultyMenu: some View")
    require("case 4: [copy five_star_analysisboard:color]; break;" in analyze,
            "shared engine no longer maps five stars to the production method", errors)
    require("engine.analyze" in swift_analyze,
            "SwiftUI game no longer calls the shared engine", errors)
    require("ForEach([2, 1, 0, 3, 4]" in difficulty_menu and
            "game.difficulty = $0" in difficulty_menu,
            "shared difficulty menu no longer exposes all five levels", errors)
    for signature in ("private var phoneControls: some View", "private var controls: some View"):
        require("difficultyMenu" in braced_block(app, signature),
                f"layout no longer uses the shared difficulty menu: {signature}", errors)
    require("engine.difficulty = difficulty" in app,
            "SwiftUI difficulty is no longer forwarded to the engine", errors)

    require("5.8.1-early-micro-vcf-adaptive-16k-80ms-2a" in target_profile,
            "5.8.1 profile version drifted", errors)
    for field in (
        "profile.earlyVCFSentinelPolicy = FC_EARLY_VCF_POLICY_ADAPTIVE;",
        "profile.earlyVCFBaseDepth = 5;",
        "profile.earlyVCFMaxDepth = 7;",
        "profile.earlyVCFNodeBudget = 16000;",
        "profile.earlyVCFTimeBudgetMs = 80;",
        "profile.earlyVCFMaxAlternatives = 2;",
    ):
        require(field in target_profile,
                f"5.8.1 profile field drifted: {field}", errors)

    factories = re.findall(r"^FCAIProfile (fc_profile_five_star\w*)\(void\)", engine, re.MULTILINE)
    require(factories == ["fc_profile_five_star_early_micro_vcf_candidate"],
            f"unexpected five-star factories: {factories}", errors)

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print(
        "five-star UI binding: pass; "
        "playable=5.8.1-only four-star=proof-guided-no-book "
        "ui=shared-swiftui-iphone+ipad"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
