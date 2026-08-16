## ADDED Requirements

### Requirement: Hybrid routes white to 5.4.1 and black to parallel v5.2.1
The candidate SHALL route every white decision to frozen `5.4.1-transactional-deadline-root-parallel-5s` and every black decision to a parallel-root derivative of `5.2.1-certificate-dependency-widening`, independent of rule, opponent label, opening identity, or prior result.

#### Scenario: Candidate owns white
- **WHEN** the candidate analyzes white to move
- **THEN** it SHALL invoke the 5.4.1 component and log its exact component identity

#### Scenario: Candidate owns black
- **WHEN** the candidate analyzes black to move
- **THEN** it SHALL invoke the parallel v5.2.1 component and log its exact component identity

### Requirement: Parallel v5.2.1 is isolated from unrelated proof-engine policy
The black component SHALL retain v5.2.1 search classes, depths, ordering, corpus, loss-aware, quiet-root, query-time, and emergency policies while enabling a dedicated parallel-root capability that does not require the broader 5.4.1 proof-engine-candidate flag.

#### Scenario: Parallel profile is constructed
- **WHEN** the eight-worker v5.2.1 profile is requested
- **THEN** it SHALL enable parallel-root execution with eight workers and SHALL NOT enable unrelated 5.4.1-only policy gates

#### Scenario: One-worker control is constructed
- **WHEN** the v5.2.1 threading control is requested
- **THEN** it SHALL use the same v5.2.1 parameters and decision deadline with one worker and serial root search

### Requirement: Parallel root work is bounded and independently owned
The component SHALL use at most eight simultaneously active workers, SHALL give each job private mutable search state, SHALL disable nested worker creation, and SHALL merge only completed proof results and verified certificates.

#### Scenario: More than eight roots are available
- **WHEN** a proof query contains more than eight independent roots
- **THEN** at most eight workers SHALL execute roots from the shared job queue

#### Scenario: A worker finishes or times out
- **WHEN** a root job completes or reaches the shared deadline
- **THEN** its private state SHALL be joined safely and unfinished proof work SHALL remain unknown

### Requirement: Every hybrid move completes within five seconds
The candidate SHALL apply a shared internal deadline with finalization reserve and SHALL return a legal best-completed move in no more than 5,000 ms under both rules.

#### Scenario: Black parallel search exhausts its deadline
- **WHEN** one or more black root jobs remain unfinished at the decision deadline
- **THEN** the component SHALL stop and join workers, mark budget exhaustion, and return the strongest completed legal result

#### Scenario: White search exhausts its deadline
- **WHEN** the frozen white component reaches its deadline
- **THEN** the router SHALL preserve its completed legal fallback and return within 5,000 ms

### Requirement: Existing levels and four-star resources remain unchanged
Production v5.1, player UI mappings, frozen four-star, legacy three-star, and lower levels SHALL remain unchanged; four-star SHALL use its original single-thread search during evaluation.

#### Scenario: Player selects five-star
- **WHEN** no later promotion change has been applied
- **THEN** the player-facing path SHALL continue to use production v5.1

#### Scenario: Benchmark invokes frozen four-star
- **WHEN** four-star acts as the opponent
- **THEN** it SHALL not enable the new parallel-root flag or launch candidate workers

### Requirement: Telemetry distinguishes concurrency from cumulative work
Every candidate decision SHALL report side, component/version, configured worker cap, cumulative workers launched, root jobs scheduled/completed, parallel nodes, wall time, process CPU, peak RSS, budget exhaustion, legality, proof metadata, and random-policy metadata.

#### Scenario: Multiple worker batches execute
- **WHEN** a decision launches more than one parallel batch
- **THEN** telemetry SHALL identify eight as the concurrency cap while preserving cumulative worker and job counts separately

