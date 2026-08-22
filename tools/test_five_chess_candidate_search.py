#!/usr/bin/env python3
"""Fast, engine-free checks for the random candidate search policy."""

from __future__ import annotations

import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from five_chess_candidate_search import (
    FORMAL_POOL_SIZE,
    SAMPLE_SIZE,
    TRAINING_POOL_SIZE,
    child_mutation,
    floor_passed,
    node_id,
    objective,
    paired_improves,
    parse_mutation_spec,
    sample_ids,
)


class CandidateSearchPolicyTests(unittest.TestCase):
    def test_sampling_is_deterministic_unique_and_fixed_size(self) -> None:
        first = sample_ids(0x1234, TRAINING_POOL_SIZE, SAMPLE_SIZE)
        second = sample_ids(0x1234, TRAINING_POOL_SIZE, SAMPLE_SIZE)
        self.assertEqual(first, second)
        self.assertEqual(len(first), SAMPLE_SIZE)
        self.assertEqual(len(set(first)), SAMPLE_SIZE)
        self.assertTrue(all(0 <= value < TRAINING_POOL_SIZE for value in first))

    def test_validation_sample_is_disjoint(self) -> None:
        training = set(sample_ids(0x1111, TRAINING_POOL_SIZE, SAMPLE_SIZE))
        validation = sample_ids(0x2222, TRAINING_POOL_SIZE, SAMPLE_SIZE,
                               training)
        self.assertTrue(training.isdisjoint(validation))

    def test_formal_pool_is_a_separate_sample_domain(self) -> None:
        heldout = sample_ids(0x3333, FORMAL_POOL_SIZE, SAMPLE_SIZE)
        self.assertEqual(len(set(heldout)), SAMPLE_SIZE)
        self.assertTrue(all(0 <= value < FORMAL_POOL_SIZE for value in heldout))

    def test_mutation_validation_and_canonical_child(self) -> None:
        self.assertEqual(
            child_mutation("none", "black-double-three-weight=20"),
            "black-double-three-weight=20")
        self.assertEqual(
            child_mutation("black-double-three-weight=20",
                           "immediate-block=1"),
            "black-double-three-weight=20;immediate-block=1")
        with self.assertRaises(ValueError):
            parse_mutation_spec("unknown=1")
        with self.assertRaises(ValueError):
            parse_mutation_spec("black-double-three-weight=101")

    def test_floor_and_lexicographic_promotion_policy(self) -> None:
        self.assertTrue(floor_passed({"free": 0.5, "forbidden": 0.5}))
        self.assertFalse(floor_passed({"free": 0.5, "forbidden": 0.49}))
        self.assertEqual(objective({"free": 0.6, "forbidden": 0.5}),
                         (0.5, 0.55, 1.1))
        self.assertTrue(paired_improves(
            {"free": 0.6, "forbidden": 0.6},
            {"free": 0.5, "forbidden": 0.6}))
        self.assertFalse(paired_improves(
            {"free": 0.6, "forbidden": 0.5},
            {"free": 0.5, "forbidden": 0.6}))

    def test_node_id_is_stable_and_lineage_sensitive(self) -> None:
        first = node_id("root", "immediate-block=1", "source")
        self.assertEqual(first,
                         node_id("root", "immediate-block=1", "source"))
        self.assertNotEqual(first,
                            node_id("other", "immediate-block=1", "source"))


if __name__ == "__main__":
    unittest.main()
