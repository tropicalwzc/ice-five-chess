## ADDED Requirements

### Requirement: The benchmark SHALL run two paired small-match cells

The benchmark SHALL run one cell with 5.8.2 against the frozen four-star control and a second cell with 5.8.2 against the exact 5.8.1 profile. Each cell SHALL use the same 12 frozen natural-freestyle opening identities, both color assignments for every identity, deterministic-best play, identical per-game seeds, no-forbidden mode, and a 120-move cap. Each cell SHALL contain 24 games and the complete benchmark SHALL contain 48 games.

#### Scenario: Candidate versus frozen four-star

- **WHEN** the four-star benchmark cell is selected
- **THEN** the candidate side SHALL resolve to 5.8.2
- **AND** the opponent side SHALL resolve to the frozen four-star control
- **AND** the report SHALL contain 12 paired opening identities with candidate-black and candidate-white games

#### Scenario: Candidate versus exact 5.8.1

- **WHEN** the direct five-star benchmark cell is selected
- **THEN** the candidate side SHALL resolve to 5.8.2
- **AND** the opponent side SHALL resolve to the exact 5.8.1 profile rather than a generic or legacy five-star alias
- **AND** the report SHALL contain the same paired opening identities, color assignments, seeds, rule mode, and move cap as the four-star cell

### Requirement: Benchmark artifacts SHALL identify exact profiles and controls

Every match schedule, game record, and summary SHALL serialize the candidate profile identity, opponent profile identity, source or build hash, rule mode, opening identity, color assignment, seed, move cap, and benchmark version. The selector for the exact 5.8.1 opponent SHALL remain distinct from any selector that resolves to 5.4.1 or another historical five-star control.

#### Scenario: Exact 5.8.1 selector is validated

- **WHEN** a direct 5.8.2-versus-5.8.1 cell is prepared
- **THEN** validation SHALL reject a schedule whose opponent selector or serialized version does not identify 5.8.1
- **AND** the schedule SHALL not be accepted as benchmark evidence

#### Scenario: Paired schedule is replayable

- **WHEN** a game record is loaded for replay
- **THEN** the recorded profile identities, source hash, seed, opening, colors, rule mode, and move cap SHALL be sufficient to reconstruct the same match configuration

### Requirement: Reports SHALL separate strength results from double-three behavior

The benchmark SHALL report candidate-black, candidate-white, and overall win/draw/loss results for each cell and for the paired aggregate. It SHALL separately report detected white double-three gains, residual gains after the selected black move, independently replayed forcing incidents, median/p95/maximum decision latency, deadline anomalies, incomplete scans, and restoration or replay failures.

#### Scenario: Tactical incidence is visible by color

- **WHEN** a completed cell is summarized
- **THEN** the summary SHALL show double-three detections and residual-defense outcomes separately for games where 5.8.2 played black and where it played white
- **AND** the summary SHALL not hide these counts inside the aggregate win/loss score

#### Scenario: Deadline anomalies are reported

- **WHEN** any decision exceeds the existing five-second ceiling, has an incomplete scan, or has a telemetry integrity failure
- **THEN** the raw record and summary SHALL identify the game, ply, profile, and anomaly type
- **AND** the cell SHALL be labeled with its anomaly count before any comparison is interpreted

### Requirement: Every match SHALL pass replay and integrity gates before publication

The benchmark SHALL replay every recorded move, validate active-rule legality and terminal results, verify any claimed VCF/VCT certificate through the existing replay path, and verify board restoration and record checksums. A game or cell with failed replay, mismatched terminal state, invalid certificate, or missing profile identity SHALL be marked invalid and SHALL NOT be used for a strength conclusion. The small match SHALL be presented as directional evidence and SHALL NOT by itself promote 5.8.2 to the playable five-star binding.

#### Scenario: Valid games produce paired evidence

- **WHEN** all games in both cells pass replay and integrity gates
- **THEN** the benchmark SHALL publish raw JSONL records and a Markdown summary for both cells
- **AND** the summary SHALL include black, white, overall, tactical, and latency views

#### Scenario: An integrity gate fails

- **WHEN** any game fails replay, terminal, certificate, checksum, or profile validation
- **THEN** the failed game SHALL be excluded from valid-match totals and named in the evidence
- **AND** the affected cell SHALL not be described as a clean comparison
