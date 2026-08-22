## ADDED Requirements

### Requirement: The recovery change SHALL include fixed regression fixtures from remaining black losses

The C regression suite SHALL include the opening-9 immediate-block position and representative quiet-white-fork predecessor positions extracted from the soft-40 paired evidence. Fixtures SHALL assert legal moves, deterministic repeatability, selected-candidate recovery classification, and board restoration.

#### Scenario: Opening-9 immediate blocker is retained

- **WHEN** the opening-9 loss position is evaluated with the recovery candidate
- **THEN** a legal move that removes the current immediate white winning point SHALL not be replaced by a structural move that leaves that point open
- **AND** the selected-candidate telemetry SHALL identify the final guard coordinate

#### Scenario: Quiet white fork is classified

- **WHEN** a representative predecessor position permits a white reply that creates two immediate winning points
- **THEN** the recovery probe SHALL report fork risk or a verified VCT loss for the affected black candidate
- **AND** a completed no-fork recovery candidate SHALL rank above an unresolved fork-risk candidate when available

#### Scenario: Fixture simulations restore the board

- **WHEN** every regression fixture is evaluated through structural and recovery stages
- **THEN** the original board hash and stone count SHALL be unchanged after each evaluation
- **AND** repeated deterministic evaluation SHALL produce identical results

### Requirement: The benchmark SHALL preserve the paired 5.8.2 comparison protocol

The recovery candidate SHALL be evaluated against frozen four-star and exact 5.8.1 in two 24-game cells using the same 12 seeded openings, both candidate colors, deterministic-best mode, no-forbidden mode, and 120-move cap as the completed soft-40 follow-up.

#### Scenario: Four-star cell is prepared

- **WHEN** the four-star cell is run
- **THEN** the candidate side SHALL use the new isolated recovery profile
- **AND** the opponent SHALL use the frozen four-star profile with identical opening, color, seed, rule, and move-cap settings

#### Scenario: Exact 5.8.1 cell is prepared

- **WHEN** the direct five-star control cell is run
- **THEN** the opponent identity SHALL serialize the exact 5.8.1 profile
- **AND** the schedule SHALL not alias the opponent to another historical five-star version

#### Scenario: Profile isolation is checked

- **WHEN** a control profile is loaded during the benchmark
- **THEN** its serialized settings SHALL exclude the new recovery fields or retain their disabled defaults
- **AND** the candidate profile version SHALL be distinct and replayable

### Requirement: Benchmark reports SHALL separate recovery behavior from strength results

Each cell summary SHALL report candidate-black, candidate-white, and overall W/D/L results separately from immediate-block outcomes, fork classifications, VCF/VCT statuses, recovery sources, structural overrides, latency, deadline anomalies, incomplete probes, and restoration failures.

#### Scenario: Recovery telemetry is summarized by candidate color

- **WHEN** a cell completes
- **THEN** the report SHALL separate black recovery probes and losses from games where the candidate played white
- **AND** it SHALL list the opening and ply for every verified loss, fork-risk selection, or retained-baseline recovery

#### Scenario: Latency and uncertainty are visible

- **WHEN** any probe or VCT follow-up is bounded, unknown, or near the decision deadline
- **THEN** the raw record and summary SHALL report its budget, completion status, and latency
- **AND** the summary SHALL not present unknown recovery results as proven defenses

### Requirement: Every recovery match SHALL pass replay and integrity gates

Before strength conclusions are published, the benchmark SHALL replay every move, validate active-rule legality and terminal results, verify all claimed VCF/VCT certificates, and verify board restoration and profile identities. Invalid games SHALL be named and excluded from clean totals.

#### Scenario: Clean paired evidence is published

- **WHEN** all games pass move replay, terminal, certificate, profile, and board-integrity checks
- **THEN** the benchmark SHALL publish raw JSONL and a Markdown summary for both cells
- **AND** the result SHALL be labeled directional research evidence rather than a promotion decision

#### Scenario: An integrity gate fails

- **WHEN** a game has an illegal move, terminal mismatch, invalid certificate, restoration mismatch, or missing profile identity
- **THEN** that game SHALL be marked invalid and excluded from clean W/D/L conclusions
- **AND** the affected cell SHALL report the failure before comparing strength
