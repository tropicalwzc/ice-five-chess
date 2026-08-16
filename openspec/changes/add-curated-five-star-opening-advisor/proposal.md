## Why

The first opening-book experiment reduced the proof-guided engine from 59.8% to 44.2% against frozen legacy because it treated official tournament start-position sequences as uniformly interchangeable move advice. A five-star level needs a much stricter, independently auditable opening advisor that can improve the current no-book four-star model without forcing either player to follow a script or overfitting to one opponent.

## What Changes

- Audit the failed book-on run by opening identity, color, first book decision, divergence from four-star, proof coverage, and outcome; publish a root-cause Markdown report.
- Research only reputable, traceable professional sources, prioritizing official federation and major tournament game records with compatible Freestyle-15 rules and explicit redistribution status.
- Reject exhibition, anonymous, low-level, rule-incompatible, synthetic, duplicated, suspicious, or insufficiently supported records; keep source/event/player/rating or title metadata and checksums.
- Build a position-to-candidate advisor from repeated high-level exact positions and compact local opening shapes rather than replaying complete tournament opening scripts. The opponent remains unrestricted; unsupported deviations fall back to four-star, while independently supported local shapes may remain usable after unrelated off-pattern moves.
- Partition move evidence by rules. Freestyle evidence SHALL never advise a forbidden-move game, and compatible 15x15 Renju evidence SHALL never advise a no-forbidden game. Compact early 20x20 Freestyle shapes may contribute only when they are provably edge-independent and losslessly embeddable on 15x15.
- Require every book candidate to pass legality, immediate obligations, proof-search safety, and conservative comparison with the original four-star move. A book candidate that cannot establish an acceptable advantage is rejected.
- Add a player-facing five-star difficulty using the strict advisor over the frozen four-star engine, while keeping four stars completely book-blind and preserving one-to-four-star save compatibility.
- Freeze untouched natural final schedules after selection. Under both forbidden and no-forbidden rules, run 100 games versus four-star and 100 versus frozen legacy three-star, split evenly as five-star black and white; no schedule may be constructed to match corpus entries.
- Generate standalone Markdown, machine-readable raw/summary data, a cross-opponent comparison, full provenance, replay validation, and checksums under `reports/five_chess` and `/Users/wangzicheng/Downloads/logs_five_chess`.

## Capabilities

### New Capabilities

- `elite-gomoku-game-corpus`: Strict acquisition, provenance, rule filtering, quality gates, symmetry normalization, deduplication, and aggregation of high-level Freestyle-15 tournament records.
- `conservative-five-star-opening-advisor`: Position-matching opening candidates that remain subordinate to four-star tactical and proof safety gates and fail closed to the four-star move.
- `five-star-difficulty-ui`: Five player difficulty levels with five stars mapped to the curated advisor and four stars remaining book-blind.
- `five-star-strength-benchmark`: Held-out 100-black/100-white matches against both four-star and frozen legacy three-star, with legality replay, white-first statistics, uncertainty, provenance, and Markdown reports.

### Modified Capabilities

None. The predecessor capabilities are change-local and not archived as project baseline specifications.

## Impact

- Affects `FiveChessAI.c/.h`, `doublethree.m/.h`, both iPhone/iPad controllers and storyboards, the benchmark runner, importer/audit/report tooling, and report artifacts.
- Adds immutable local position statistics and provenance assets; the shipped app remains fully offline.
- Does not train a model, use live network services during play, expose the corpus to four-star, or force either side to follow stored games.
