## ADDED Requirements

### Requirement: The isolated 5.8.2 candidate SHALL preserve pre-structural recovery candidates

When black double-three preemption changes the provisional move, the candidate SHALL retain the pre-structural baseline, legal advisory/default point, and legal four-star or handoff candidates in the opponent-guard recovery portfolio. The portfolio SHALL deduplicate coordinates and SHALL remain isolated from 5.8.1 and other profiles.

#### Scenario: Structural preemption replaces an immediate blocker

- **WHEN** the pre-structural baseline is legal and differs from the double-three-selected move
- **THEN** the baseline SHALL be available to the final opponent guard before the bounded alternative budget is exhausted
- **AND** the final guard SHALL publish the audit for the selected coordinate rather than the structural provisional coordinate

#### Scenario: The advisory/default move is legal and distinct

- **WHEN** a legal advisory/default move differs from the structural provisional move
- **THEN** the guard SHALL explicitly consider that move as a recovery candidate
- **AND** candidate omission caused only by generated-list truncation SHALL NOT remove it from consideration

#### Scenario: Controls remain unchanged

- **WHEN** 5.8.1, frozen four-star, or a lower difficulty profile is requested
- **THEN** the new recovery portfolio and fork probe SHALL NOT be enabled implicitly
- **AND** the serialized profile identity SHALL remain the existing control identity

### Requirement: Black recovery SHALL prioritize current opponent immediate-win blockers

For each legal recovery candidate, the isolated 5.8.2 guard SHALL count legal opponent moves that complete five after the black candidate. A candidate with zero current opponent immediate wins SHALL outrank a candidate with one or more such wins unless black has a legal immediate win or an independently verified own VCF certificate.

#### Scenario: A structural move leaves an immediate white win

- **WHEN** a structural candidate leaves at least one legal white move that completes five
- **AND** a retained baseline or recovery candidate leaves zero such moves
- **THEN** the guard SHALL select or prefer the zero-immediate-win candidate
- **AND** the result SHALL NOT claim that the structurally safe candidate is an acceptable defense

#### Scenario: Every candidate leaves an immediate win

- **WHEN** every audited legal black candidate leaves at least one immediate white winning move
- **THEN** the result SHALL retain a verified-loss or unknown classification as applicable
- **AND** the engine SHALL NOT convert the position into a proven safe defense

#### Scenario: Black has a higher-priority own win

- **WHEN** black has a legal immediate win or a replay-verified own VCF certificate
- **THEN** the own win SHALL retain priority over the immediate-block ranking
- **AND** the bypass reason and certificate identity SHALL remain observable

### Requirement: VCF-unknown black defenses SHALL receive bounded two-step fork classification

When the opponent VCF audit for a black recovery candidate is unknown, the candidate SHALL be eligible for a bounded deterministic probe that enumerates legal white replies and counts the resulting legal white immediate winning points. Two or more resulting winning points SHALL classify the candidate as an opponent fork risk; incomplete enumeration SHALL remain unknown.

#### Scenario: A quiet white reply creates two immediate wins

- **WHEN** a legal white reply after a black candidate creates at least two legal white moves that complete five
- **THEN** the candidate SHALL be classified as a two-step opponent-fork risk
- **AND** the guard SHALL prefer a completed no-fork candidate over that risk when proof classes are otherwise unresolved

#### Scenario: A completed reply scan finds no two-step fork

- **WHEN** the bounded legal reply scan completes and every white reply creates at most one immediate winning point
- **THEN** the candidate SHALL receive a scoped no-fork classification
- **AND** that classification SHALL NOT be serialized as a global VCF/VCT disproof

#### Scenario: The fork probe reaches its bound or deadline

- **WHEN** the reply frontier, node budget, or decision deadline prevents complete classification
- **THEN** the fork result SHALL be unknown
- **AND** the engine SHALL preserve the existing proof uncertainty instead of treating missing replies as safe

### Requirement: Relevant VCF-unknown black defenses SHALL support bounded VCT escalation

For a black recovery decision whose opponent VCF is unknown, the isolated 5.8.2 profile SHALL be able to reserve a bounded VCT follow-up under the decision ledger. A verified opponent VCT SHALL remain a verified loss, a replayed scoped disproof SHALL outrank an unresolved candidate, and incomplete work SHALL remain unknown.

#### Scenario: VCT verifies an opponent forcing win

- **WHEN** the bounded VCT follow-up returns a replay-verified opponent certificate
- **THEN** the candidate SHALL be classified as a verified loss
- **AND** it SHALL not outrank a completed scoped disproof or a candidate with a stronger guard class

#### Scenario: VCT completes with a scoped disproof

- **WHEN** the VCT follow-up completes and the isolated disproof replay succeeds
- **THEN** the candidate SHALL be eligible for the proven-defense recovery class
- **AND** the public result SHALL identify the search class and replayed proof status

#### Scenario: VCT is not completed

- **WHEN** the VCT budget or deadline expires before a certificate or scoped disproof is available
- **THEN** the result SHALL remain unknown
- **AND** the engine SHALL not report a global safety claim from the partial search

### Requirement: Final recovery metadata SHALL describe the selected coordinate transactionally

Whenever structural, guard, or corpus recovery changes the selected black coordinate, the public result SHALL update proof status, proof coordinate, opponent-after-selected fields, loss reason, recovery source, and fork/double-three telemetry consistently for that final coordinate. Temporary simulations SHALL restore the board before returning.

#### Scenario: Guard selects a retained baseline instead of the structural move

- **WHEN** the final guard replaces the structural provisional coordinate
- **THEN** all public proof and loss fields SHALL describe the retained coordinate
- **AND** provisional audit data SHALL remain distinguishable from selected-candidate data

#### Scenario: A temporary fork or recovery simulation returns

- **WHEN** the immediate-block or fork probe finishes, succeeds, or aborts
- **THEN** the board hash and stone count SHALL match the pre-probe state
- **AND** a restoration mismatch SHALL invalidate the recovery override and be recorded

#### Scenario: The same position is evaluated repeatedly

- **WHEN** the same board, profile, and deterministic seed are evaluated repeatedly
- **THEN** the selected coordinate, recovery class, and telemetry ordering SHALL be identical
- **AND** no proof or fork status from a different candidate SHALL be published
