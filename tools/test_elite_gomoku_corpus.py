#!/usr/bin/env python3

import hashlib
import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "elite_importer", ROOT / "tools/import_elite_gomoku_corpus.py")
assert SPEC and SPEC.loader
importer = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = importer
SPEC.loader.exec_module(importer)


def psq(rows: list[str], first: str = "RAPFI25", second: str = "JAX25") -> bytes:
    return ("Piskvorky 15x15, 11:11, 0\n" + "\n".join(rows) +
            f"\n{first}.zip\n{second}.zip\n-1\n0,Freestyle15_1\n").encode()


class EliteCorpusTests(unittest.TestCase):
    division = importer.Division("Freestyle15_1", importer.RULE_FREESTYLE, 15)

    def test_psq_is_one_based_and_assignment_prefix_is_not_advice(self):
        game = importer.parse_psq(2025, self.division, "fixture.psq", psq([
            "8,8,0", "9,8,0", "8,9,123", "9,9,1",
        ]))
        self.assertEqual(game.moves[0][:2], (7, 7))
        self.assertEqual(game.assignment_plies, 2)
        self.assertEqual(game.players, ("RAPFI25", "JAX25"))

    def test_illegal_duplicate_coordinate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "occupied_coordinate"):
            importer.parse_psq(2025, self.division, "fixture.psq", psq([
                "8,8,0", "8,8,1",
            ]))

    def test_assignment_only_record_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "assignment_only"):
            importer.parse_psq(2025, self.division, "fixture.psq", psq([
                "8,8,0", "9,8,0",
            ]))

    def test_translation_and_all_eight_symmetries_canonicalize(self):
        board = {}
        for x, y, side in ((6, 6, 1), (7, 6, -1), (8, 7, 1), (6, 8, -1)):
            board[(x, y)] = side
        expected = importer.canonical_position(
            board, 1, importer.RULE_FREESTYLE, 15)[0]
        for transform in range(8):
            moved = {}
            for (x, y), side in board.items():
                tx, ty = importer.transform(15, transform, x, y)
                moved[(tx, ty)] = side
            self.assertEqual(importer.canonical_position(
                moved, 1, importer.RULE_FREESTYLE, 15)[0], expected)

    def test_rule_partitions_have_distinct_exact_keys(self):
        board = {(6, 6): 1, (7, 6): -1, (8, 7): 1, (6, 8): -1}
        freestyle = importer.canonical_position(
            board, 1, importer.RULE_FREESTYLE, 15)[0]
        forbidden = importer.canonical_position(
            board, 1, importer.RULE_FORBIDDEN, 15)[0]
        self.assertNotEqual(freestyle, forbidden)

    def test_20x20_opening_must_embed_with_target_margin(self):
        compact = {(8, 8): 1, (9, 8): -1, (9, 9): 1, (10, 8): -1}
        self.assertTrue(importer.opening_is_embeddable(compact, 8, 9, 20)[0])
        wide = {(2, 2): 1, (15, 15): -1}
        accepted, reason = importer.opening_is_embeddable(wide, 9, 9, 20)
        self.assertFalse(accepted)
        self.assertEqual(reason, "not_embeddable_on_15x15")

    def test_runtime_asset_matches_manifest_checksum_and_has_no_scripts(self):
        manifest = json.loads((ROOT / "tools/elite_corpus_manifest.json").read_text())
        asset = (ROOT / "ice five chess/FiveChessEliteCorpus.inc").read_bytes()
        self.assertEqual(hashlib.sha256(asset).hexdigest(), manifest["assetSha256"])
        self.assertGreater(manifest["runtimeExactPositions"], 0)
        self.assertGreater(manifest["runtimeLocalPatterns"], 0)
        self.assertNotIn(b"Piskvorky", asset)
        self.assertNotIn(b".psq", asset)

    def test_asset_renderer_is_deterministic(self):
        positions = [{"keyA": 2, "keyB": 3, "candidateStart": 0,
                      "candidateCount": 1, "stoneCount": 4, "rule": 0}]
        candidates = [{"x": 1, "y": 2, "games": 3, "events": 2,
                       "sources": 1, "trustTier": 2, "sourceBoardMask": 1,
                       "wins": 2, "draws": 0, "losses": 1}]
        local_stones = [{"dx": -1, "dy": 0, "side": 1},
                        {"dx": 0, "dy": 1, "side": -1}]
        local_patterns = [{"stoneStart": 0, "games": 3, "wins": 2,
                           "draws": 0, "losses": 1, "stoneCount": 2,
                           "events": 2, "sources": 1, "rule": 0, "side": 1,
                           "trustTier": 2, "sourceBoardMask": 1,
                           "boundaryClass": 0}]
        with tempfile.TemporaryDirectory() as directory:
            left, right = Path(directory) / "a.inc", Path(directory) / "b.inc"
            importer.render_asset(left, positions, candidates, local_patterns, local_stones)
            importer.render_asset(right, positions, candidates, local_patterns, local_stones)
            self.assertEqual(left.read_bytes(), right.read_bytes())


if __name__ == "__main__":
    unittest.main()
