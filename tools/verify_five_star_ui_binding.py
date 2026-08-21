#!/usr/bin/env python3
"""Verify the playable five-star factory without banning retained controls."""

from __future__ import annotations

import argparse
from pathlib import Path


TARGET_FACTORY = "fc_profile_five_star_early_micro_vcf_candidate()"
ROLLBACK_FACTORY = "fc_profile_five_star_proof_engine_candidate()"


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
    phone = (root / "ice five chess/ViewController.m").read_text()
    tablet = (root / "ice five chess/HDViewController.m").read_text()
    engine = (root / "ice five chess/FiveChessAI.c").read_text()

    five_star = braced_block(
        doublethree, "-(void) five_star_analysisboard:(int) mode")
    four_star = braced_block(
        doublethree, "-(void) four_star_analysisboard:(int) mode")
    target_profile = braced_block(
        engine, "FCAIProfile fc_profile_five_star_early_micro_vcf_candidate(void)")
    rollback_profile = braced_block(
        engine, "FCAIProfile fc_profile_five_star_proof_engine_candidate(void)")

    errors: list[str] = []
    require(TARGET_FACTORY in five_star,
            "playable five-star method does not select the 5.8.1 factory",
            errors)
    require(ROLLBACK_FACTORY not in five_star,
            "playable five-star method still selects exact 5.4.1", errors)
    require("fc_profile_proof_guided(false)" in four_star,
            "four-star method no longer selects proof-guided no-book", errors)

    controller_call = "[ice_fiver five_star_analysisboard:-player];"
    require(controller_call in phone,
            "iPhone controller no longer calls the shared five-star method",
            errors)
    require(controller_call in tablet,
            "iPad controller no longer calls the shared five-star method",
            errors)

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

    require("5.4.1-transactional-deadline-root-parallel-5s" in
            rollback_profile, "exact 5.4.1 rollback version drifted", errors)
    require("profile.earlyVCFSentinelEnabled = true;" not in rollback_profile,
            "exact 5.4.1 unexpectedly enables the early sentinel", errors)

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1

    print(
        "five-star UI binding: pass; "
        "playable=5.8.1 rollback=5.4.1 four-star=proof-guided-no-book "
        "controllers=iphone+ipad"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
