## ADDED Requirements

### Requirement: Isolated 5.4.1 scheduler profile

The system SHALL expose an opt-in 5.4.1 scheduler profile whose tactical
parameters, proof search class, proof depth, node budgets, candidate ordering,
and certificate acceptance semantics match the existing 5.4.1 baseline, while
its profile identity and scheduler fields are distinct and inspectable.

#### Scenario: Baseline remains unchanged

- **WHEN** a caller requests the existing 5.4.1 profile factory
- **THEN** the returned profile SHALL retain its prior version and scheduler
  flags, and the production route SHALL continue to select it

#### Scenario: Candidate exposes scheduler identity

- **WHEN** a caller requests the 5.4.1 scheduler profile
- **THEN** the profile snapshot SHALL identify the candidate separately and
  SHALL report persistent-pool and token-block scheduling as enabled

### Requirement: Bounded persistent proof scheduling

The candidate SHALL use the existing bounded worker pool for eligible parallel
root and escape proof batches, keep mutable search state private to each
worker, and merge results by canonical job index after all dispatched workers
complete.

#### Scenario: Repeated batches reuse workers

- **WHEN** two or more candidate proof batches are executed in one process
- **THEN** pool dispatch and worker-reuse telemetry SHALL increase without
  requiring a new pthread for every batch, and the published result SHALL
  remain deterministic

#### Scenario: Pool failure falls back safely

- **WHEN** the pool cannot dispatch a requested batch
- **THEN** the candidate SHALL use the existing serial/direct proof fallback,
  record a pool fallback, and SHALL NOT publish an unverified win

### Requirement: Exact block-based budget accounting

The candidate SHALL reserve token blocks atomically within the shared aggregate
node budget (and any enclosing decision ledger), return unused tokens when a
worker or query ends, and stop work at the existing 4,500 ms internal reserve
and five-second hard limit.

#### Scenario: Unused tokens are returned

- **WHEN** a worker finishes before consuming its reserved block
- **THEN** the unused reservation SHALL be returned and consumed nodes SHALL
  not exceed the configured aggregate budget

#### Scenario: Deadline stops all workers

- **WHEN** the internal deadline or hard limit is reached during a batch
- **THEN** workers SHALL stop or return unknown through the existing bounded
  path, the candidate's 4,200 ms internal reserve SHALL leave room for pool
  finalization, and the caller SHALL retain a legal completed move or safe fallback

### Requirement: Worker-count benchmark route

The benchmark SHALL expose the candidate as a direct 5.4.1 scheduler profile
and SHALL accept only research worker counts 1, 4, and 8 for fixed-position and
formal replay cells.

#### Scenario: Fixed worker matrix is auditable

- **WHEN** the benchmark runs the candidate at workers 1, 4, and 8
- **THEN** each record SHALL include the requested/effective worker cap,
  wall/CPU latency, pool counters, token claim/return counters, proof status,
  and certificate verification state

#### Scenario: Invalid worker count is rejected

- **WHEN** the benchmark is given a worker count other than 1, 4, or 8
- **THEN** it SHALL reject the invocation before running a game

### Requirement: Replay safety and strength evidence

The candidate SHALL pass the existing legality, board-restoration, certificate,
determinism, and five-second gates before its four-star or legacy-three-star
score results are considered valid.

#### Scenario: Four-star replay passes safety gates

- **WHEN** the candidate plays the formal free and forbidden-black four-star
  cells
- **THEN** all 200 games SHALL have legal moves, zero anomalies, candidate
  steps no slower than 5,000 ms, and valid scheduler/proof telemetry

#### Scenario: Legacy-three-star replay passes safety gates

- **WHEN** the candidate plays the same formal cells against the frozen legacy
  three-star engine
- **THEN** all 200 games SHALL satisfy the same legality, deadline, and
  certificate requirements before score rates are reported

#### Scenario: Strength result is classified conservatively

- **WHEN** a replay's paired score interval crosses zero or a required color
  gate fails
- **THEN** the report SHALL classify the result as not demonstrated even if the
  point estimate is positive
