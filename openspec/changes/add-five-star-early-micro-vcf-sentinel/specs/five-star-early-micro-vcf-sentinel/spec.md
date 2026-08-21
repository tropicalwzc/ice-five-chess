## ADDED Requirements

### Requirement: Isolated early-sentinel profile
The system SHALL provide a selectable five-star candidate derived from the completed opponent-guard candidate, with independently configurable micro-VCF enablement, depth, node budget, time budget, adaptive escalation, and early alternative cap, while preserving the frozen parent candidate and UI-bound 5.4.1 control.

#### Scenario: Select sentinel candidate
- **WHEN** the sentinel candidate profile is selected for analysis or benchmarking
- **THEN** it SHALL inherit the completed final opponent guard and SHALL enable only the explicitly configured early-sentinel behavior

#### Scenario: Existing profiles remain frozen
- **WHEN** the parent opponent-guard candidate or UI-bound 5.4.1 profile is selected
- **THEN** its profile identity and sentinel behavior SHALL remain unchanged

### Requirement: Sentinel runs before expensive downstream search
For every legal non-immediate-winning provisional move selected after the four-star/fork handoff, the system SHALL run the enabled opponent micro-VCF sentinel before own VCF/VCT, quiet search, loss-aware refinement, corpus replacement, and the final opponent guard.

#### Scenario: Ordinary provisional move
- **WHEN** the handoff/fork layer selects a legal move that does not immediately win
- **THEN** the sentinel SHALL audit the opponent position after that move before downstream own-proof work begins

#### Scenario: Immediate legal win
- **WHEN** the provisional move legally completes five for the side to move
- **THEN** the system SHALL skip the sentinel and retain the immediate win

#### Scenario: Mandatory one-ply defense
- **WHEN** the provisional move blocks an immediate opponent win but is not an own immediate win
- **THEN** the sentinel SHALL remain eligible to detect a continuing opponent VCF

### Requirement: Certificate-safe rejection semantics
The sentinel SHALL reject a provisional move only when the opponent VCF is `PROVEN_WIN`, the certificate is complete, and independent replay verifies it on the exact after-move board under the active rule.

#### Scenario: Verified opponent VCF
- **WHEN** the sentinel produces an opponent `PROVEN_WIN` certificate and independent replay succeeds
- **THEN** the system SHALL classify the provisional move as an early verified loss and SHALL allow deterministic replacement

#### Scenario: Failed certificate verification
- **WHEN** a search reports `PROVEN_WIN` but its certificate is absent, incomplete, or fails independent replay
- **THEN** the sentinel SHALL classify the result as `unknown` and SHALL NOT reject the move on that evidence

#### Scenario: Scoped disproof
- **WHEN** the sentinel completes `NO_FORCED_WIN_IN_SCOPE`
- **THEN** the system SHALL record the exact VCF scope and SHALL NOT describe the move as globally safe or skip the final guard

#### Scenario: Incomplete or invalid audit
- **WHEN** the sentinel reaches a deadline, overflows enumeration, encounters an illegal placement, fails board restoration, or otherwise does not complete valid proof evidence
- **THEN** it SHALL return `unknown`, preserve a legal transactional state, and SHALL NOT claim safety

### Requirement: Bounded fixed and adaptive policies
The system SHALL support fixed depth-5, fixed depth-7, and threat-adaptive depth-5/7 sentinel policies within the aggregate decision ledger and existing absolute deadline.

#### Scenario: Default adaptive query
- **WHEN** the adaptive policy audits an after-move position without a qualifying opponent four, open-four, or VCF dependency signal
- **THEN** it SHALL use the configured depth-5 scope and budget

#### Scenario: Adaptive escalation
- **WHEN** the adaptive policy detects a qualifying opponent forcing signal
- **THEN** it SHALL be eligible to use the configured depth-7 scope without exceeding the sentinel or aggregate decision budget

#### Scenario: Signal is not proof
- **WHEN** an adaptive forcing signal exists but no replayable opponent win certificate completes
- **THEN** the system SHALL NOT reject the move based on the signal alone

#### Scenario: Hard deadline
- **WHEN** sentinel and downstream work execute in one decision
- **THEN** their combined work SHALL remain within the existing 4.5-second internal deadline and 5-second player-visible hard ceiling

### Requirement: Deterministic certificate-directed replacement
When the provisional move is an early verified loss, the system SHALL prioritize replacement candidates derived from the verified certificate before existing deterministic tactical and generated candidates, without labeling unresolved replacements as safe.

#### Scenario: Certificate interruption candidate exists
- **WHEN** a verified loss certificate contains legal gain, cost, rest, or dependency interruption points
- **THEN** the system SHALL place those candidates ahead of ordinary generated alternatives in deterministic order

#### Scenario: Replacement remains unknown
- **WHEN** a legal alternative is not itself a verified loss within the sentinel budget but lacks a completed disproof
- **THEN** it MAY become the downstream provisional move but SHALL remain classified as `unknown`

#### Scenario: All early alternatives are verified losses
- **WHEN** every audited early alternative has a replayable opponent win certificate
- **THEN** the system SHALL retain a legal deterministic provisional result, record the verified-loss evidence and survival ordering, and defer final authority to the full guard

### Requirement: Compatible proof-work reuse
The system SHALL reuse compatible sentinel proof work through the active proof session using keys that distinguish board, side, rule, search class, scope, and proof-engine version.

#### Scenario: Reuse verified shallow win
- **WHEN** the final guard queries the same after-move position and a compatible verified sentinel win certificate is cached
- **THEN** it SHALL be permitted to reuse the certificate after independent replay and SHALL report the reuse

#### Scenario: Extend shallow disproof
- **WHEN** a shallow sentinel disproof is available but the final guard requires a deeper VCF or VCT scope
- **THEN** the system MAY use the prior work as a search seed but SHALL NOT treat it as satisfying the broader query

#### Scenario: Incompatible cache entry
- **WHEN** any board, side, rule, class, scope, or engine-version key differs
- **THEN** the system SHALL NOT reuse the cached result as completed evidence

### Requirement: Final guard remains authoritative
The system SHALL run the existing final opponent VCF/VCT guard for every final non-own-verified-winning move and corpus replacement regardless of sentinel disproof or unknown status.

#### Scenario: Sentinel misses deeper loss
- **WHEN** the sentinel returns disproof or `unknown` and the final guard later verifies an opponent VCF or VCT
- **THEN** the final guard SHALL reject or rank the move according to its existing completed-class rules

#### Scenario: Corpus changes the move
- **WHEN** corpus processing proposes a different final coordinate after the sentinel stage
- **THEN** that coordinate SHALL pass the existing full opponent guard before acceptance

### Requirement: Sentinel telemetry and invariants
The system SHALL expose enough telemetry to replay sentinel decisions and distinguish early cost, evidence, replacement, reuse, downstream savings, and final-guard outcomes without mutating the caller board.

#### Scenario: Sentinel was eligible
- **WHEN** a sentinel audit runs
- **THEN** telemetry SHALL include policy, effective depth, nodes, elapsed time, status, distance, certificate verification, provisional and replacement coordinates, replacement source, alternatives audited, cache reuse, and ledger consumption

#### Scenario: Compare early and final stages
- **WHEN** a decision reaches the final guard
- **THEN** telemetry SHALL identify whether the loss was caught early, caught only by the final guard, or unresolved at both stages, and SHALL report downstream budget spent or preserved

#### Scenario: Board integrity
- **WHEN** sentinel analysis completes, aborts, or rolls back
- **THEN** the caller board and rule state SHALL exactly match their pre-sentinel values
