## ADDED Requirements

### Requirement: Forbidden legality cache is incrementally reversible
The incremental position SHALL update legality masks and affected line facts on
make/unmake for the four crossing directions and their affected windows.  Threat,
refutation, and immediate-win hot paths MUST be able to query the cache without a
full-board copy while preserving the project's established forbidden semantics.

#### Scenario: Make and unmake a forbidden black candidate
- **WHEN** a black candidate changes an overline, double-four, or double-three
  fact and is then undone
- **THEN** the cached legality result and board key return exactly to their prior
  values

#### Scenario: A move is outside the affected lines
- **WHEN** a make/unmake operation cannot affect a cached line or window
- **THEN** unrelated revisions and masks remain valid and are not recomputed

### Requirement: The reference oracle gates cache pruning
The implementation MUST retain `fc_is_legal_move` as the reference oracle and
MUST compare cached answers in seeded random samples and adversarial fixtures.
Any mismatch SHALL increment telemetry and disable cache-based pruning for the
current decision (or fall back to the oracle path).

#### Scenario: Cache and oracle agree
- **WHEN** validation returns identical legality for every sampled move
- **THEN** the cache may prune hot-path legality checks and the manifest records
  zero mismatches

#### Scenario: Cache and oracle disagree
- **WHEN** a sampled or adversarial move produces different legality answers
- **THEN** the current decision uses the oracle, records the exact fixture/move,
  and cannot report a cache-derived verified certificate

### Requirement: One ledger owns the decision resources
Every five-star stage SHALL consume from one decision ledger containing an
absolute monotonic deadline, aggregate nodes, temporary bytes, active sessions,
and corpus/book queries.  Helpers MUST NOT create independent emergency budgets
or extend the deadline beyond the five-second player-visible ceiling.

#### Scenario: A proof stage exhausts its reservation
- **WHEN** a root proof consumes its reserved nodes before completing
- **THEN** the ledger marks that stage unknown, releases only completed-result
  resources, and permits later stages only if aggregate budget remains

#### Scenario: Internal and player deadlines differ
- **WHEN** the internal target is 4,500 ms and the wall clock approaches 5,000 ms
- **THEN** all stages stop at the absolute deadline, the best completed legal move
  is returned, and no emergency extension is granted

### Requirement: Budget handoffs are auditable
The analyzer SHALL record requested, reserved, consumed, and abandoned resources
per stage, plus the exhaustion cause and actual worker count.  A result without a
complete ledger record MUST be treated as an invalid diagnostic run.

#### Scenario: Corpus lookup competes with proof
- **WHEN** a corpus query and a proof stage both request remaining budget
- **THEN** the fixed stage policy grants/denies the request deterministically and
  records the decision without silently borrowing from another stage

#### Scenario: A worker cap exceeds available jobs
- **WHEN** the profile cap is eight but only three root jobs are available
- **THEN** at most three workers launch and telemetry distinguishes configured cap
  from actual launches and maximum concurrency

### Requirement: Profile switches are independent
The research profile SHALL expose opening-book and elite-corpus switches
independently.  A corpus/book candidate MUST pass legality, mandatory-defense,
  proof, and shared-budget checks before it can override a searched move.

#### Scenario: Corpus is enabled without an opening book
- **WHEN** `eliteCorpusEnabled=true` and `openingBookEnabled=false`
- **THEN** corpus telemetry is present, no book move is injected, and search
  semantics/budget remain otherwise identical

#### Scenario: A corpus move conflicts with a mandatory defense
- **WHEN** a corpus candidate is legal but a verified immediate defense exists
- **THEN** the defense is selected and the corpus candidate is recorded as rejected
  with a deterministic reason

