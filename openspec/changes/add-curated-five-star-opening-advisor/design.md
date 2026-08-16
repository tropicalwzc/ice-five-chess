## Context

The current four-star profile is the opening-book-disabled proof-guided engine. In the held-out legacy match it scored 119.5/200 (59.8%), while the otherwise identical first book implementation scored 88.5/200 (44.2%). The original asset contains 24 official Gomocup tournament start sequences of 4–25 plies. Runtime lookup matches every symmetry and uniformly samples a legal next coordinate, then makes that book move the default unless the existing bounded proof search can prove an immediate tactical failure.

That asset was valid for assigning balanced tournament starts, but it was not evidence that each stored ply is advantageous advice for an independently moving side. The implementation also lacks player strength filters, repeated-position support, move-frequency confidence, side-specific quality, or a conservative comparison against the four-star choice. Five-star must therefore be a new position-statistics advisor, not a renamed version of the failed script-following book.

## Goals / Non-Goals

**Goals:**

- Establish an evidence-backed root cause for the book-on regression.
- Source only auditable, rule-compatible, high-level records and reject weak or ambiguous material.
- Let five-star consult the corpus while four-star remains byte-for-byte book-blind.
- Accept book advice only when it passes tactical obligations and is not worse than the four-star fallback under comparable completed search.
- Demonstrate generalization against both four-star and frozen legacy three-star with color-separated held-out matches.

**Non-Goals:**

- Forcing either player to replay a stored game.
- Treating a tournament opening assignment as an expert move recommendation.
- Learning opaque weights from final held-out match outcomes.
- Supporting Renju, Standard Gomoku, Caro, anonymous internet games, or unclear redistribution rights as Freestyle-15 evidence.
- Guaranteeing that five-star must ship if the held-out evidence is inconclusive or negative.

## Decisions

### 1. Separate assignment openings from move-advice records

Official Gomocup opening packages remain benchmark-schedule material. They are not admitted as five-star advice solely because they appeared in a major event. Five-star move evidence must come from actual completed high-level games with both player identities or engine/event class, compatible rules, source URL, event/year, and reproducible record identifier.

Alternative rejected: keep the 24 lines and tune random weights. That would optimize a category error and risk fitting the previous 200-game result.

### 2. Use strict source and record admission

Source priority is official federation/tournament archives, then organizer-published complete result packages. A record is admitted only when Freestyle-15 compatibility is explicit, move order is complete and legal, provenance is immutable, and competitive level meets the frozen allowlist. Human records require recognized title/rating or participation in an allowlisted major championship; engine records require an allowlisted top division/final. Records with missing identity, mixed rules, handicaps, exhibitions, impossible coordinates, early corruption, or unclear redistribution status are excluded.

Raw third-party records with unclear bundling rights are research-only. The shipped asset contains only aggregated factual position counts and attributed source metadata unless redistribution permission is documented.

### 3. Aggregate positions, not scripts

Each admitted game contributes positions only through a bounded opening horizon. Positions are canonicalized across eight symmetries and keyed by board, side to move, rule, and corpus version. Each legal continuation stores independent-game count, independent-event count, source-family count, side, and bounded result metadata. Exact duplicates and mirrored copies count once per original game.

A runtime hit requires frozen independent support. Cross-event continuations form the highest tier; a single official top-division event may qualify only with a substantially higher independent-game repeat threshold and candidate consensus. A reply never becomes mandatory. Exact-position evidence has priority, but a separately aggregated compact local opening shape may remain usable after unrelated opponent deviations when its rule, side, boundary class, local support, and tactical gates all match.

### 4. Make four-star the baseline decision, not merely a fallback after failure

Five-star first computes the exact four-star decision with the same seed domain and budgets. It then obtains supported corpus candidates. Immediate wins and complete necessary defenses always dominate. Every corpus alternative must be legal, immediately safe, and evaluated under the same proof class and budget as the four-star move.

The corpus move is accepted only if it has a strictly better completed proof class/distance, or if it is proof-equivalent and satisfies frozen support plus a conservative search-score margin. Unknown/incomparable evidence, budget exhaustion, certificate failure, or a unique four-star tactical move retains four-star. User randomness is allowed only among candidates tied after all proof and support gates.

### 5. Diagnose without selecting on final outcomes

The old book-on/off raw games are used to identify failure mechanisms and create regression fixtures, not to assign production weights to individual openings. Corpus thresholds and fusion variants are selected on a separate diagnostic seed/opening domain. A new final domain is generated only after source set, thresholds, code, and profile are frozen.

### 6. Require two independent strength comparisons

The final benchmark contains exactly 200 five-star-versus-four-star games and 200 five-star-versus-legacy-three-star games, each split 100/100 by five-star color on held-out neutral opening identities. Only five-star may access the corpus. Reports lead with five-star white performance, then black and overall score, Wilson intervals, paired differences, book acceptance/rejection reasons, fallback rate, latency, proof coverage, anomalies, versions, and checksums.

Promotion to the player-facing five-star entry requires no correctness anomaly, no material tactical regression, and non-negative white and overall point estimates against both baselines. `Demonstrated stronger` requires the configured primary five-star-versus-four-star interval threshold; otherwise the UI may retain the implementation behind a disabled switch and report the result honestly.

### 7. Preserve difficulty-save compatibility

Persisted values remain 0=three, 1=two, 2=one, 3=four; value 4 is five-star. UI order becomes five, four, three, two, one on both device layouts. Loading any existing value retains its prior meaning.

### 8. Partition rule evidence and admit only edge-independent 20x20 openings

The corpus has separate no-forbidden Freestyle and forbidden/Renju partitions. Runtime lookup selects exactly one partition from `forbiddenBlack`; no candidate support is pooled across partitions. Renju records are replayed with the app's forbidden-move implementation, and incompatible games are rejected rather than approximately translated.

Official top-division 20x20 Freestyle records may contribute only within the frozen early-opening horizon. Every contributing position and continuation must be compact enough to embed on 15x15 under all recorded relative coordinates, be separated from the source edge, and be placed with the same boundary class on the target board. The importer records original board size and exclusion reasons. Later 20x20 positions, edge-dependent shapes, and spans too large for 15x15 never enter runtime evidence.

### 9. Generalize with local shapes without weakening tactics

The importer additionally builds side-specific local signatures around the recorded candidate from actual elite decisions. Signatures include the bounded neighborhood, rule partition, local stone count, boundary class, candidate-relative coordinate, independent games/events, and consensus. Runtime may ignore stones outside that neighborhood, which lets advice survive unrelated deviations, but it must reject any conflicting in-window stone, insufficient local support, side mismatch, boundary mismatch, illegal candidate, immediate obligation, immediate opponent win after the move, or verified opponent forced win. Exact matches outrank local matches and telemetry distinguishes both.

### 10. Select and evaluate only on natural schedules

Coverage and fusion thresholds are selected on seeded natural openings whose generation is independent of corpus contents. Corpus fixtures remain unit tests only and SHALL NOT be used as strength or coverage evidence. Final evaluation uses new untouched natural seeds, separately under forbidden and no-forbidden rules, with 100 paired-color games per rule and opponent (50 five-star black, 50 white).

## Risks / Trade-offs

- [Too few redistributable elite games] → Fail closed to a small corpus; aggregate research-only sources without bundling raw records; do not relax quality thresholds merely to increase coverage.
- [20x20 strategy depends on its larger board] → Use only compact early shapes with explicit source/target edge-independence; never use 20x20 midgame or edge evidence.
- [Local shape collides with strategically different global positions] → Require rule/side/boundary compatibility, high independent support and consensus, prefer exact matches, and retain legality/immediate/proof/search safety gates.
- [Result bias mistakes correlation for move quality] → Require repeated independent position support and proof/search gates; do not use held-out outcomes for weights.
- [Corpus move is strategically good but proof search cannot distinguish it] → Retain four-star. Five-star favors safety over coverage.
- [Same event creates pseudo-replication] → Track game, event, and source-family counts separately and cap contributions.
- [Five-star overfits four-star behavior] → Freeze a second formal match against legacy three-star and require non-regression against both.
- [Latency roughly doubles because both baseline and alternatives are evaluated] → Reuse generated candidates and proof transpositions where sound, cap opening horizon/candidate count, and report p50/p95/max.

## Migration Plan

1. Freeze four-star code/profile/hash and old book-on/off evidence.
2. Publish root-cause analysis and source-admission manifest.
3. Import and aggregate the approved corpus into a versioned offline asset.
4. Implement conservative five-star fusion behind a disabled profile and add tactical/provenance tests.
5. Run diagnostic ablations, freeze thresholds/profile/corpus, and generate a new held-out domain.
6. Run both 200-game formal matches, replay all games, generate reports/checksums, and enable the five-star UI only if the release gate passes.
7. Rollback removes the five-star UI selection while keeping four-star and all lower levels unchanged.

## Open Questions

- Availability and redistribution terms of additional federation game archives must be recorded source by source during implementation.
- If no external corpus meets the strict rights and quality gates, the valid outcome is a disabled five-star advisor with a documented data blocker rather than importing low-quality games.
