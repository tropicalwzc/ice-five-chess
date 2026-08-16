## ADDED Requirements

### Requirement: Candidate profiles are reproducible
Every diagnostic decision and match cell SHALL persist a profile manifest that
includes version identity, rule mode, opening/book/corpus switches, worker cap,
internal/hard deadlines, node/memory/query budgets, seed schedule, and source
checksums.  Frozen three-star and four-star controls MUST be identified by their
existing immutable manifests.

#### Scenario: Replaying a fixed position
- **WHEN** a fixture is rerun with the same manifest and seed
- **THEN** the result status, selected verified move, certificate replay outcome,
  and stage ordering are reproducible

#### Scenario: Profile differs only by corpus switch
- **WHEN** two runs use identical resources and seeds but toggle the corpus
- **THEN** reports attribute any move/result difference to that switch and keep
  the two cells separate

### Requirement: Correctness and performance gates precede strength tests
The evaluation harness MUST run fallback legality, handoff, root-job equivalence,
postponed-sibling determinism, certificate replay, and legality-cache oracle
fixtures before game matches.  It SHALL record one-, four-, and eight-worker
throughput, p50/p95/max latency, duplicate-root work, and cache mismatches.

#### Scenario: A correctness fixture fails
- **WHEN** any fallback, replay, determinism, or oracle fixture fails
- **THEN** the strength run is blocked and the report identifies the failing
  fixture instead of classifying the candidate as stronger or weaker

#### Scenario: Eight workers do not overlap safely
- **WHEN** the eight-worker diagnostic detects duplicate root enumeration or
  nondeterministic verified results
- **THEN** the profile falls back to one worker for diagnosis and the candidate is
  not eligible for the 100-game comparison

### Requirement: Strength reports preserve rule and color attribution
After gates pass, the harness SHALL run separate 100-game forbidden and
no-forbidden cells against the frozen four-star, with legacy three-star as a
non-gating generalization control.  Each cell MUST report candidate black and
white W/D/L, score rate, paired deltas, Wilson intervals, unknown/no-progress
counts, latency, worker utilization, and the exact schedule/provenance.

#### Scenario: Candidate plays white
- **WHEN** a rule-mode cell contains 100 games with the candidate as white
- **THEN** the report gives the white score independently and does not reweight it
  by the natural black first-player advantage

#### Scenario: A match ends after an unknown fallback
- **WHEN** a decision returns a legal unknown fallback during a game
- **THEN** the raw log counts the decision and reason, while the game result is
  scored normally and is not silently counted as a verified loss

### Requirement: Production and controls remain unchanged
The evaluation implementation MUST leave the current production/UI routing and
frozen three-/four-star control profiles untouched.  Promotion of a passing
candidate SHALL require a later explicit change.

#### Scenario: Research profile is selected in a benchmark
- **WHEN** a benchmark requests the new recovery-budget profile
- **THEN** only the benchmark instance uses it and the playable production route
  continues to use its existing profile

#### Scenario: A gate passes
- **WHEN** all correctness, performance, and strength evidence is complete
- **THEN** the result is marked research-ready and no UI difficulty mapping is
  changed automatically

