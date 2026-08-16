## ADDED Requirements

### Requirement: Root gains are independent proof jobs
The coordinator SHALL enumerate root threats once and create one job per distinct
gain in canonical coordinate/order-key order.  Each worker MUST receive the
pre-enumerated gain and start below that gain; it MUST NOT re-enumerate the full
root threat set through the all-root proof entry point.

#### Scenario: Multiple root gains overlap in a dependency line
- **WHEN** two gains share a line or forbidden-legality dependency
- **THEN** they remain separate proof jobs, may run concurrently, and only share
  explicitly immutable metadata or a versioned read-only transposition entry

#### Scenario: Root enumeration overflows its capacity
- **WHEN** threat enumeration reports overflow
- **THEN** the coordinator records incomplete root coverage, limits claims to
  completed scopes, and returns unknown rather than a global verified loss

### Requirement: Parallel results merge deterministically
Worker results SHALL be stored by canonical job index and merged in that order
after completion.  The selected move, proof counters, and certificate identity
MUST be independent of worker completion order for the same seed, profile, and
budget.

#### Scenario: One-worker and eight-worker runs use the same fixture
- **WHEN** the same root fixture is run with one and eight workers
- **THEN** completed-result status, selected verified move, and replayable
  certificate match, while telemetry reports actual concurrency separately

#### Scenario: A worker exits without a completed result
- **WHEN** a worker hits its slice or fails before publishing a completed proof
- **THEN** its job is marked unknown, no partial counters are merged as proof, and
  other jobs may continue within the shared budget

### Requirement: Verified results stop work safely
The coordinator MUST set an atomic stop condition when a verified own win or a
replayable scoped disproof is published.  Workers SHALL observe the condition at
node boundaries, publish only completed results, and be joined before the caller
uses the selected move.

#### Scenario: A verified own win arrives early
- **WHEN** one root job publishes a certificate that passes replay verification
- **THEN** new jobs are not started, active workers stop at safe boundaries, and
  the verified move is selected without waiting for unrelated unknown jobs

#### Scenario: A proof result cannot be replayed
- **WHEN** a worker reports a disproof whose certificate fails replay
- **THEN** the result is downgraded to unknown, the stop flag is unchanged, and
  another job may provide the first valid disproof

### Requirement: Escape proof workers use private mutable state
Parallel escape candidates MUST use private board/position, proof graph, and
allocator state per worker.  The batch SHALL stop on the first independently
replayable scoped disproof but SHALL retain completed unknown/survival results for
deterministic fallback ranking.

#### Scenario: Two escape candidates mutate the same coordinates
- **WHEN** two candidates are evaluated concurrently
- **THEN** each worker observes an isolated position and neither worker can alter
  the other's certificate or board state

#### Scenario: No scoped disproof completes
- **WHEN** the batch deadline is reached with only unknown or verified-loss
  candidate results
- **THEN** the coordinator ranks completed results deterministically and returns a
  legal fallback without claiming a scoped disproof

