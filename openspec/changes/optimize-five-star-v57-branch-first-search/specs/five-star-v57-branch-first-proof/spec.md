## ADDED Requirements

### Requirement: Branch-first is an opt-in v5.7 profile

The engine SHALL expose a distinct v5.7 branch-first research profile that inherits the existing v5.7 legality, ledger, certificate, and recovery contracts while enabling recursive branch scheduling.  The default v5.7 and persistent root-scheduler profiles MUST remain behaviorally selectable without this capability.

#### Scenario: Default profile remains unchanged

- **WHEN** a caller selects the existing v5.7 hybrid or persistent root-scheduler profile
- **THEN** branch-first scheduling is disabled and the existing root proof route is used

#### Scenario: Branch-first profile advertises its extended horizon

- **WHEN** a caller selects the branch-first profile
- **THEN** its snapshot identifies branch-first scheduling and a proof depth greater than the 12-ply v5.7 baseline, with worker count restricted to 1, 4, or 8

### Requirement: Shallow previews SHALL determine deterministic priority only

For each distinct root gain in a branch-first proof query, the engine SHALL compute a bounded preview score using advanced-four/advanced-three severity, immediate winning costs, continuation/rest points, dependency locality, and shallow refutation width.  The engine SHALL order equal inputs deterministically, and SHALL NOT use a preview alone as proof or as a certificate.

#### Scenario: Advanced-four and advanced-three branches are prioritized

- **WHEN** two legal root gains are available and one has a stronger advanced-four/advanced-three preview
- **THEN** the stronger preview is visited first, and diagnostics record the previewed branch class

#### Scenario: Preview tie is reproducible

- **WHEN** two gains have equal preview scores and equal tactical metadata
- **THEN** the engine orders them by the canonical dependency order and coordinate tie-break, independent of worker completion order

#### Scenario: Preview reaches the deadline

- **WHEN** the decision deadline is reached during preview generation
- **THEN** the engine records an incomplete preview, preserves legal fallback behavior, and does not publish a proof based on the partial score

### Requirement: Only long recursive sibling waves SHALL use the pool

The branch-first engine SHALL keep root obligation traversal ordered by the coordinator and SHALL dispatch a pool wave only below the root when the remaining depth and refutation width meet the configured thresholds.  A pool worker MUST NOT synchronously dispatch another pool wave.

#### Scenario: Short recursive node

- **WHEN** a recursive node is below the long-recursion depth or branch-count threshold
- **THEN** its siblings are searched by the existing serial recursion path

#### Scenario: Long recursive node

- **WHEN** a non-root attacker node has sufficient remaining depth and at least the minimum number of legal refutation branches
- **THEN** the coordinator may dispatch those reply branches to the bounded pool and wait for the wave to complete

#### Scenario: Pool worker reaches a recursive node

- **WHEN** a branch task is already running on a pool worker and its continuation reaches a branch-wave condition
- **THEN** nested dispatch is suppressed and the continuation remains private and serial

### Requirement: Branch workers SHALL preserve proof isolation and deterministic merge

Every branch task SHALL use a private mutable board, proof session, transposition state, and certificate buffer.  The coordinator SHALL merge only completed verified branch certificates in canonical reply order, remapping parent indices below the corresponding defender node.  All defender replies SHALL be proven before the parent attack is classified as a verified win.

#### Scenario: All recursive replies are verified

- **WHEN** every branch task returns a verified forced-win certificate
- **THEN** the coordinator merges all branch certificates, restores the parent board, and may publish the parent proof with a certificate that passes the existing verifier

#### Scenario: One branch is unknown or disproved

- **WHEN** any branch task is unknown, budget-exhausted, unverified, or disproves the continuation
- **THEN** the parent branch is not considered proven and the engine continues or falls back without treating the incomplete wave as a win

#### Scenario: Certificate arena overflows

- **WHEN** merging a verified branch would exceed the certificate capacity
- **THEN** the candidate is marked unknown, the caller checkpoint is restored, and no partial certificate is published

#### Scenario: Concurrent completion order differs

- **WHEN** the same branch query completes with different worker timing
- **THEN** the selected move, proof status, certificate validity, and board contents after return are deterministic

### Requirement: Depth extension SHALL remain within the decision contract

The branch-first profile MAY use a deeper proof horizon, but every branch task SHALL consume the shared atomic node ledger, memory reservation, token budget, and absolute internal/hard deadlines.  Search MUST return before the five-second hard limit and MUST report unknown/deadline when the budget is insufficient.

#### Scenario: Aggregate budget is exhausted

- **WHEN** branch workers collectively consume the aggregate node budget
- **THEN** no further branch token is granted, unfinished branches are unknown, and the result does not exceed the configured ledger totals

#### Scenario: Five-second hard gate is reached

- **WHEN** the hard decision deadline is reached while a deeper branch is running
- **THEN** workers unwind, flush unused token blocks, restore all boards/sessions, and the caller returns a legal fallback or unknown result within the hard gate

#### Scenario: Depth-14 branch has budget

- **WHEN** the branch-first profile searches an eligible long recursive node before the deadline
- **THEN** it may search the configured extended depth and diagnostics expose the extension without changing the proof certificate rules

#### Scenario: Tactical branch receives bounded extension

- **WHEN** an eligible recursive continuation is classified as advanced-four or advanced-three
- **THEN** the continuation may use respectively `base depth +2` or `base depth +1`, never exceeding the profile tactical cap of 16 plies, and diagnostics identify the extension class

### Requirement: Branch-first diagnostics SHALL distinguish useful work

The engine SHALL expose preview counts, branch wave/job/completion counts, worker concurrency, verified useful branch count, serial fallback, depth extension, token/deadline termination, and existing root-pool counters.  Test tools SHALL be able to compare these metrics for worker counts 1, 4, and 8.

#### Scenario: One-worker control

- **WHEN** the branch-first profile runs with one worker
- **THEN** no multi-worker branch advantage is claimed, branch ordering and proof results remain valid, and diagnostics identify the serial control

#### Scenario: Four/eight-worker branch run

- **WHEN** the same fixed positions run with four or eight workers
- **THEN** the output reports branch jobs, maximum concurrency, useful verified branches, CPU/elapsed time, legality, certificate status, and replay determinism

#### Scenario: Strength replay

- **WHEN** the existing 200-game four-star and legacy-three-star replay suites run with branch-first enabled
- **THEN** all moves remain legal and replay certificates pass, while any strength change is classified separately from scheduler efficiency
