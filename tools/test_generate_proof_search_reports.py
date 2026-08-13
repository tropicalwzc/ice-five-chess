#!/usr/bin/env python3
"""Regression tests for proof-search report result perspectives."""

from __future__ import annotations

import unittest

import generate_proof_search_reports as reports


def games(new_color: int, new_wins: int, draws: int, new_losses: int) -> list[dict]:
    return (
        [{"newColor": new_color, "winner": new_color, "steps": []}] * new_wins
        + [{"newColor": new_color, "winner": 0, "steps": []}] * draws
        + [{"newColor": new_color, "winner": -new_color, "steps": []}] * new_losses
    )


class SummaryPerspectiveTests(unittest.TestCase):
    def setUp(self) -> None:
        self.control = games(-1, 52, 0, 48)

    def test_book_off_uses_direct_match_legacy_white(self) -> None:
        match = games(-1, 52, 0, 48) + games(1, 67, 1, 32)

        summary = reports.summarize(match, self.control)

        self.assertEqual(
            (32, 1, 67),
            tuple(summary["results"]["legacyWhiteOpponent"][key]
                  for key in ("wins", "draws", "losses")),
        )
        self.assertAlmostEqual(0.325, summary["results"]["legacyWhiteOpponent"]["scoreRate"])
        self.assertAlmostEqual(0.195, summary["whiteDeltaVsLegacyOpponent"])
        self.assertEqual("demonstrated stronger", summary["classification"])
        self.assertAlmostEqual(
            0.52, summary["results"]["legacySelfPlayControlWhite"]["scoreRate"]
        )

    def test_book_on_white_delta_does_not_use_self_play_control(self) -> None:
        match = games(-1, 38, 0, 62) + games(1, 50, 1, 49)

        summary = reports.summarize(match, self.control)

        self.assertAlmostEqual(0.495, summary["results"]["legacyWhiteOpponent"]["scoreRate"])
        self.assertAlmostEqual(-0.115, summary["whiteDeltaVsLegacyOpponent"])
        self.assertEqual("not demonstrated", summary["classification"])


if __name__ == "__main__":
    unittest.main()
