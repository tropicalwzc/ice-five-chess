## ADDED Requirements

### Requirement: Proof-session memory reservations SHALL be reversible

The five-star decision ledger SHALL treat proof arena reservations as live
concurrent usage.  A successful reservation SHALL increase live bytes and
update a monotonic peak; releasing a session SHALL subtract exactly the bytes
owned by that session.  A sequence of completed sessions SHALL not exhaust the
memory budget merely because their arenas were previously allocated.

#### Scenario: Sequential proof sessions reuse the decision memory budget

- **WHEN** a decision starts and completes multiple proof sessions one after another
- **THEN** each completed session releases its arena reservation
- **AND** the next session can reserve the freed bytes while live usage stays within the configured budget

#### Scenario: Concurrent workers cannot oversubscribe the ledger

- **WHEN** multiple proof workers reserve arenas concurrently
- **THEN** reservation succeeds only while the aggregate live reservation is within the decision memory budget
- **AND** the peak counter is at least the observed live usage and never exceeds the configured budget

### Requirement: Session teardown SHALL release ownership on every lifecycle path

Each proof session that holds a reservation MUST retain its reservation size and
ledger owner.  Normal completion, proof exhaustion, cancellation, worker
teardown, and allocation failure after reservation SHALL release that ownership
exactly once.  A session that did not acquire a reservation SHALL not release
bytes belonging to another session.

#### Scenario: Exhausted session is torn down

- **WHEN** a proof session reaches a node, time, or ledger limit and is ended
- **THEN** its reserved bytes are returned to the owning ledger
- **AND** the ledger does not retain a phantom reservation that blocks later recovery work

#### Scenario: Arena allocation fails after reservation

- **WHEN** the ledger reservation succeeds but the arena allocation returns null
- **THEN** the begin path releases the reservation before reporting the session as unavailable
- **AND** the live counter does not become negative or remain charged for the failed allocation

#### Scenario: Repeated teardown is harmless

- **WHEN** cleanup is attempted after a session has already released its reservation
- **THEN** the second cleanup performs no additional release
- **AND** live and released counters remain internally consistent

### Requirement: Ledger telemetry SHALL distinguish live, peak, released, and exhausted usage

The five-star result and diagnostic stream SHALL expose live reserved bytes at
decision end, peak reserved bytes, released bytes, cumulative committed/used
bytes when available, and the number and stage of reservation failures.  The
telemetry SHALL identify an unknown result caused by budget exhaustion without
claiming that the board was a verified loss.

#### Scenario: A healthy multi-session decision is reported

- **WHEN** all proof arenas are released before the decision ends
- **THEN** the result reports zero live reservation, a nonzero peak when an arena was used, and released bytes equal to the held reservations
- **AND** no memory-ledger exhaustion is recorded

#### Scenario: A memory budget prevents a new session

- **WHEN** a requested arena would exceed the live memory budget
- **THEN** the reservation is rejected atomically and the exhaustion stage is recorded
- **AND** the decision may return unknown with a legal move, but not verified loss solely for that rejection
