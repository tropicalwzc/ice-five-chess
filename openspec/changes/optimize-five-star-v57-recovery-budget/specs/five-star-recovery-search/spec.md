## ADDED Requirements

### Requirement: Decision certainty is explicit
The five-star analyzer SHALL expose a decision certainty state that distinguishes
`NO_LEGAL_MOVE`, `VERIFIED_WIN`, `VERIFIED_LOSS`, and `UNKNOWN_OR_DEADLINE`.
The legacy `provenLoss` field MUST be true only for the first or third state and
MUST remain false when a bounded candidate list merely contains no safe move.

#### Scenario: Bounded candidates contain no safe move
- **WHEN** candidate generation is truncated and every generated candidate is
  marked unsafe, but a full legal move exists outside that list
- **THEN** the analyzer returns `UNKNOWN_OR_DEADLINE`, preserves `provenLoss=false`,
  and continues to legal fallback selection

#### Scenario: Complete proof establishes loss
- **WHEN** every legal move in the complete candidate universe has an
  independently replayed loss certificate
- **THEN** the analyzer returns `VERIFIED_LOSS`, sets `provenLoss=true`, and
  records the certificate/reason in telemetry

### Requirement: Every non-terminal position receives a legal fallback
The five-star analyzer SHALL return a move that passes the rule-aware legality
oracle for every non-terminal board with at least one legal move.  It SHALL select
the best completed candidate first and SHALL fall back to a deterministic
full-board legal scan when staged candidates are empty or incomplete.

#### Scenario: Search reaches the deadline before a proof completes
- **WHEN** the shared decision deadline expires while all proof results are
  unknown
- **THEN** the analyzer returns the highest-ranked completed legal move, marks
  the result unknown/budget-exhausted, and does not end the game

#### Scenario: No legal move exists
- **WHEN** the board is terminal or every empty point is illegal for the side to
  move
- **THEN** the analyzer returns `NO_LEGAL_MOVE` with coordinates `(-1,-1)` and
  the caller may end the game

### Requirement: Four-star handoff is fail-open
Five-star recovery MUST continue from a legal baseline when the frozen four-star
hint returns false, unknown, or an incomplete result.  A four-star failure MUST
NOT bypass own-win, escape, corpus-validation, or fallback stages.

#### Scenario: Four-star hint analysis fails
- **WHEN** `fc_analyze_four_star_with_hint` cannot produce a complete result
- **THEN** five-star constructs a legal baseline, records the handoff reason, and
  executes its remaining stages within the same decision budget

#### Scenario: Four-star result is already an immediate win
- **WHEN** the handoff baseline contains a verified immediate win
- **THEN** five-star preserves that move and does not replace it with a corpus or
  quiet candidate

### Requirement: Escape candidates are searched in ordered layers
When a baseline is backed by a verified opponent certificate, the analyzer SHALL
deduplicate and process candidates in this order: certificate dependencies,
tactical defenses, nearby relevance-zone moves, and full-board legal fallback.
It MUST perform cheap legality/immediate-loss checks before dispatching proof and
MUST not classify an unsearched layer as losing.

#### Scenario: A scoped disproof is found in a tactical layer
- **WHEN** a tactical escape candidate produces a certificate that replays on the
  original position
- **THEN** that move is selected immediately, later layers are skipped, and the
  result records `FC_OVERRIDE_PROVEN_DEFENSE`

#### Scenario: All searched escapes remain unknown
- **WHEN** no candidate produces a replayable scoped disproof before the budget
  expires
- **THEN** the analyzer retains the best completed survival/unknown candidate,
  reports the unknown reason, and does not claim a verified loss

### Requirement: DFPN no-progress is not a loss
The five-star proof session MUST postpone a no-progress edge and revisit eligible
siblings before returning.  A deadline, node budget, or no-progress condition
SHALL yield `UNKNOWN_OR_DEADLINE` unless a verified certificate has already been
completed.

#### Scenario: Selected edge makes no progress
- **WHEN** a DFPN edge produces unchanged proof/disproof thresholds in its slice
- **THEN** the edge is queued for a later epoch, another deterministic sibling is
  selected, and the session remains searchable

#### Scenario: All postponed work reaches the deadline
- **WHEN** no postponed edge can complete before the aggregate deadline
- **THEN** the proof returns unknown with counters intact and never upgrades the
  no-progress condition to a loss
