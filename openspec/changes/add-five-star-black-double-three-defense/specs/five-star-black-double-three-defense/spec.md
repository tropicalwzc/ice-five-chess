## ADDED Requirements

### Requirement: The engine SHALL expose an isolated 5.8.2 candidate profile

The engine SHALL expose a named 5.8.2 profile derived from the exact 5.8.1 early-micro-VCF profile. Enabling the candidate profile SHALL NOT alter the 5.8.1 profile, the frozen four-star control, or any lower difficulty profile.

#### Scenario: Candidate profile preserves the 5.8.1 baseline

- **WHEN** a benchmark or diagnostic requests the 5.8.2 profile
- **THEN** the serialized profile identity SHALL identify 5.8.2 and SHALL include the inherited 5.8.1 configuration needed for replay
- **AND** a request for the 5.8.1 profile SHALL continue to resolve to the unchanged 5.8.1 configuration

#### Scenario: Other difficulty profiles remain controls

- **WHEN** a game requests frozen four-star, three-star, two-star, or one-star analysis
- **THEN** the engine SHALL use the existing profile for that difficulty
- **AND** the 5.8.2 double-three defense stage SHALL NOT be implicitly enabled

### Requirement: The engine SHALL detect white double-three gains by independent directions

For a black-to-move position, the 5.8.2 candidate SHALL enumerate legal white reply moves and SHALL count the affected open-three directions rather than continuation points. A direction SHALL match the existing advisor meaning when a bounded four-cell window contains exactly three white stones and one empty cell, with both exterior cells inside the board and empty. Each of the four board directions SHALL count at most once. A legal white reply SHALL be recorded as a double-three gain when at least two independent directions match and the reply is not already an immediate five.

#### Scenario: Crossing white double-three gain is detected

- **WHEN** a legal white reply creates open threes in two or more independent directions under the direction-based predicate
- **THEN** the scan SHALL record that reply as a white double-three gain
- **AND** the recorded gain SHALL retain enough coordinate or direction-mask information for candidate auditing and replay

#### Scenario: A single open three is not a double-three gain

- **WHEN** a legal white reply creates an open three in exactly one direction
- **THEN** the scan SHALL NOT record that reply as a double-three gain
- **AND** the position SHALL remain available as a negative-control fixture

#### Scenario: Edge, blocked, and rule-dependent lines are handled consistently

- **WHEN** a candidate reply touches the board edge, has a blocked exterior, or is forbidden under the active rule mode
- **THEN** the detector SHALL apply the same in-board and empty-exterior conditions as the legacy predicate
- **AND** the detector SHALL use the shared rule-aware legality helper before recording the gain

### Requirement: Black decisions SHALL preempt detected gains unless a verified own forcing result has priority

When black is to move and the complete scan finds one or more white double-three gains, the 5.8.2 candidate SHALL run the structural preemption stage before accepting the final move. A legal immediate black win or an independently verified own VCF certificate SHALL take priority. An unresolved opponent VCF SHALL NOT by itself suppress the structural scan. An own VCT result without a replayed terminal-before-reply certificate SHALL NOT automatically bypass the structural stage.

#### Scenario: No own verified VCF is available

- **WHEN** black has no legal immediate win and no independently verified own VCF
- **AND** the white scan is complete with at least one double-three gain
- **THEN** structural preemption SHALL be considered before the final black move is accepted
- **AND** the existing opponent VCF/VCT audit SHALL remain available after candidate ordering

#### Scenario: An own immediate win or verified VCF takes priority

- **WHEN** black has a legal immediate win or an independently replayed own VCF certificate
- **THEN** the candidate SHALL be allowed to bypass double-three preemption
- **AND** the decision ledger SHALL record the bypass reason and the relevant proof identity

#### Scenario: Opponent VCF status is unresolved

- **WHEN** the opponent VCF query returns `unknown` because its bounded proof did not complete
- **AND** the white double-three scan completes
- **THEN** the candidate SHALL still use the structural scan to order black defensive candidates
- **AND** the unresolved VCF SHALL remain `unknown` rather than being converted into a proof of safety

### Requirement: Structural candidate ordering SHALL prefer complete double-three elimination and fail closed

When structural preemption is required, the candidate SHALL add deterministic legal blockers from the recorded gains to a bounded tactical pool while retaining ordinary, dependency, and certificate candidates. After simulating each candidate, the engine SHALL re-check all recorded gains. Candidates with zero residual gains SHALL outrank candidates with residual gains; proof class, comparable scope, distance, and existing heuristic ordering SHALL break ties within the same residual class. If no candidate eliminates all gains, the engine SHALL select only according to the least-residual unresolved result and SHALL NOT label the result as a proven defense.

#### Scenario: A targeted blocker eliminates every recorded gain

- **WHEN** a legal black candidate occupies a gain point or otherwise makes every recorded white gain illegal or removes a required open-three direction
- **THEN** the candidate SHALL be classified as structurally safe for this narrow obligation
- **AND** it SHALL outrank candidates with one or more residual gains when no higher-priority own win or verified VCF applies

#### Scenario: No single black move eliminates all gains

- **WHEN** every legal candidate leaves at least one recorded white double-three gain
- **THEN** the engine SHALL retain the candidate with the least residual gain count according to deterministic tie-breaking
- **AND** the structural result SHALL be marked unresolved and SHALL NOT claim that the double-three threat is fully defended

#### Scenario: Scan budget, storage, or legality work is incomplete

- **WHEN** the scan reaches its time limit, gain capacity, candidate capacity, or an incomplete legality frontier
- **THEN** the structural result SHALL be marked `unknown` or incomplete
- **AND** incomplete evidence SHALL NOT be treated as zero residual gains or as a safe defense

### Requirement: Temporary analysis SHALL restore the board and emit black-defense telemetry

Every temporary white or black simulation performed by the detector and structural stage SHALL restore the board exactly before the next candidate or the caller resumes. A restoration mismatch SHALL invalidate the structural override. The 5.8.2 decision ledger SHALL distinguish scan completion, white gain count, provisional and selected residual counts, candidates examined and eliminated, own-VCF bypasses, structural overrides, restoration failures, and deadline anomalies.

#### Scenario: Repeated fixed-position evaluation is deterministic and lossless

- **WHEN** the same fixed position, profile, and seed is evaluated repeatedly
- **THEN** the selected move, structural status, and telemetry ordering SHALL be deterministic
- **AND** the board hash and move count after every evaluation SHALL match the pre-evaluation state

#### Scenario: Restoration mismatch occurs

- **WHEN** any temporary simulation fails to restore the original board hash or move count
- **THEN** the engine SHALL discard the structural override
- **AND** the ledger SHALL record a restoration failure and unresolved structural status
