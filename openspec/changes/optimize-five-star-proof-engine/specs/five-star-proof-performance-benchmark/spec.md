## ADDED Requirements

### Requirement: Frozen baselines remain independent controls
The benchmark system SHALL preserve production five-star v5.1 as the performance and rollback baseline, frozen four-star as the primary playable strength opponent, and frozen legacy three-star as the generalization opponent. It SHALL record their profile identities, source/control provenance, golden decisions, corpus version, rule mode, and checksums before candidate parameter selection.

#### Scenario: Control manifest is created
- **WHEN** benchmark preparation begins
- **THEN** the system SHALL create a checksummed manifest identifying production v5.1, frozen four-star, legacy three-star, corpus, diagnostic inputs, benchmark tool version, and rule semantics

#### Scenario: Frozen opponent drifts
- **WHEN** four-star or legacy three-star differs from its frozen profile, golden decision set, persistence/UI mapping, or control provenance
- **THEN** the affected benchmark SHALL stop and SHALL NOT be reported as a comparison against that frozen model

#### Scenario: Four-star is used after candidate implementation
- **WHEN** formal games launch against four-star
- **THEN** the runner SHALL invoke the preserved four-star control rather than the new five-star proof session and SHALL report four-star as the existing playable model

### Requirement: Diagnostics are separated from formal strength evidence
Prior losses, prior formal schedules, known proof fixtures, symmetries, and non-final natural positions SHALL be used only for correctness, profiling, ablation, and structural parameter selection. Final strength schedules SHALL be generated from fresh seed domains after the candidate code and parameters are frozen and SHALL exclude diagnostic and prior formal identities.

#### Scenario: Historical loss is reused
- **WHEN** a prior five-star loss or one of its eight symmetries is evaluated
- **THEN** it SHALL contribute only to proof/relevance/correctness diagnostics and SHALL NOT contribute a game or coordinate to final strength evidence or production runtime rules

#### Scenario: Formal seed separation is audited
- **WHEN** final schedules are generated
- **THEN** tooling SHALL prove that their master seeds, opening identities, position hashes, and seed domain were unavailable during parameter selection and disjoint from prior loss, corpus-shaped, non-final, and previous formal inputs

### Requirement: Correctness and activity gates precede performance and strength claims
The candidate SHALL pass full-scan equivalence, randomized make/unmake, both-rule legality, DFPN proof/disproof, transposition-scope, relevance coverage, dependency verification, quiet integration, eight-symmetry, certificate replay, board-integrity, deterministic regeneration, ASan/UBSan, lower-level golden, and iOS Simulator build gates before it is eligible for a performance or formal strength claim.

#### Scenario: Any correctness gate fails
- **WHEN** a mismatch, invalid certificate, uncovered defender move, board mutation, sanitizer finding, nondeterministic result, lower-level drift, or build failure occurs
- **THEN** the candidate SHALL be classified invalid, production SHALL remain v5.1, and no affected strength cell SHALL be accepted

#### Scenario: New search mode is inactive
- **WHEN** held-out eligible diagnostics complete with zero thresholded most-proving expansions, zero compatible session reuse, zero verified relevance omissions, zero dependency combinations, or zero quiet roots examined for a mode declared enabled
- **THEN** the system SHALL report that mode as not demonstrated and SHALL NOT attribute a strength or speed change to it

#### Scenario: Scoped disproof diagnostics exist
- **WHEN** independently constructed completed-scope disproof fixtures are run
- **THEN** the candidate SHALL complete and replay at least one scoped disproof in each supported rule mode before natural formal games begin

### Requirement: Fixed-position speed is measured and every candidate decision is hard-bounded
On a checksummed fixed-position suite and declared reference hardware, the candidate SHALL compare against production v5.1 using identical rule inputs and deterministic modes. Incremental hash/line/threat/legality batch throughput SHALL be at least twice the full-scan reference. Candidate and v5.1 p50, p95, mean, and maximum SHALL be reported separately for no-forbidden and forbidden modes, while every candidate decision SHALL return a legal best-completed move within a 5,000 ms player-visible hard ceiling. One global candidate deadline SHALL cover all nested stages.

#### Scenario: Primitive performance is measured
- **WHEN** the incremental and reference primitives process the same boards and operations
- **THEN** the report SHALL verify identical outputs and SHALL report operations per second, cells/lines scanned, board copies, allocations, cache fallbacks, and a throughput ratio of at least 2.0 for the declared aggregate batch

#### Scenario: Decision latency is measured
- **WHEN** candidate and production v5.1 analyze the same fixed diagnostic positions
- **THEN** the report SHALL provide per-rule p50, p95, maximum, mean, completed classifications per second, deterministic nodes, TT/session reuse, worker count, parallel jobs, CPU time, peak resident memory, fallbacks, and budget exhaustion, and every candidate maximum SHALL be no greater than 5,000 ms

#### Scenario: Nested stage reaches the decision ceiling
- **WHEN** own proof, baseline counter-proof, escape, corpus, dependency, or quiet work consumes the remaining global wall or node allowance
- **THEN** the session SHALL stop unfinished work as unknown, SHALL return the best legal move from the highest completed evaluation/proof class available at expiry, and SHALL NOT start a fresh per-candidate allowance that can extend the declared ceiling

#### Scenario: Speed gate fails
- **WHEN** semantic outputs differ, aggregate primitive throughput is below 2.0, any candidate decision exceeds 5,000 ms, or timeout fails to return a legal best-completed move
- **THEN** formal 400-game testing SHALL not begin and production SHALL remain v5.1

### Requirement: Candidate parameters freeze before final schedules
All proof-table capacities, node budgets, wall ceiling, worker count, root partitioning and merge rules, per-worker memory cap, frontier initialization, relevance fallbacks, dependency limits, quiet-root limits, corpus gates, randomness equivalence rules, and promotion criteria SHALL be selected on diagnostics and checksummed before final seed generation. Any later code or parameter change SHALL invalidate all final cells.

#### Scenario: Freeze succeeds
- **WHEN** correctness, activity, and speed gates pass
- **THEN** the system SHALL write a freeze manifest containing code/profile/corpus/tool hashes and structural parameters before generating untouched final schedules

#### Scenario: Post-freeze change occurs
- **WHEN** any frozen source, parameter, corpus, runner, replay logic, or schedule rule changes after final seed generation
- **THEN** all affected final games SHALL be discarded and the complete separation/freeze process SHALL restart with new seeds

### Requirement: Four fresh color-exchanged 100-game cells are executed
The formal suite SHALL run exactly four independent 100-game cells: no-forbidden new five-star versus frozen four-star, no-forbidden new five-star versus legacy three-star, forbidden new five-star versus frozen four-star, and forbidden new five-star versus legacy three-star. Each cell SHALL contain 50 fresh natural opening identities with colors exchanged, yielding 50 games with new five-star black and 50 with new five-star white.

#### Scenario: No-forbidden cells run
- **WHEN** the no-forbidden formal schedule launches
- **THEN** the runner SHALL complete 100 games against frozen four-star and 100 games against legacy three-star with 50/50 new-five-star color balance in each cell

#### Scenario: Forbidden cells run
- **WHEN** the forbidden formal schedule launches
- **THEN** the runner SHALL enforce the frozen forbidden legality semantics and complete 100 games against frozen four-star and 100 games against legacy three-star with 50/50 new-five-star color balance in each cell

#### Scenario: Formal outcome becomes available
- **WHEN** any subset of final game outcomes is known
- **THEN** those outcomes SHALL NOT alter candidate parameters, opening selection, remaining schedules, retry policy, or promotion criteria

#### Scenario: Formal games use parallel candidate search
- **WHEN** a formal cell runs on the declared 10-performance-core plus 4-efficiency-core M4 Pro host
- **THEN** the runner SHALL use the frozen candidate search-worker count, SHALL run only one game process at a time unless a documented resource ablation proves non-interference, and SHALL give both colors and opponents the same host availability, priority, wall-clock policy, and thermal preparation

### Requirement: Color comparisons are computed from the correct player perspective
Each cell SHALL report new-five-star black, new-five-star white, opponent black, opponent white, and overall W/D/L and score rates without color weighting. Same-color deltas SHALL compare the new model's games in one color with the opponent's games in that same color from the exchanged openings; first-player advantage SHALL remain visible.

#### Scenario: White delta is computed
- **WHEN** a 100-game color-exchanged cell is summarized
- **THEN** `new white score rate` SHALL use only the 50 games where new five-star is white, `opponent white score rate` SHALL use only the paired 50 games where the opponent is white, and the reported white delta SHALL equal the former minus the latter

#### Scenario: Black delta is computed
- **WHEN** a 100-game color-exchanged cell is summarized
- **THEN** `new black score rate` SHALL use only the 50 games where new five-star is black, `opponent black score rate` SHALL use only the paired 50 games where the opponent is black, and the reported black delta SHALL equal the former minus the latter

#### Scenario: Natural black advantage appears
- **WHEN** black wins more often in either rule mode
- **THEN** the report SHALL retain the observed black and white rates and SHALL NOT reweight colors to manufacture a neutralized overall score

### Requirement: Promotion emphasizes white strength and forbids hidden regressions
Promotion SHALL be evaluated separately for each rule and opponent cell using predeclared point estimates and uncertainty. Against frozen four-star, new five-star SHALL have a positive same-color white score-rate delta in both rule cells and non-negative black and overall directions in both cells. Against legacy three-star, white and overall directions SHALL be non-negative in both rule cells. No pooled color or rule result SHALL override a failing required cell.

#### Scenario: White improves but black regresses against four-star
- **WHEN** new five-star has a positive white delta but a negative black or overall direction in either four-star rule cell
- **THEN** the candidate SHALL fail promotion and production SHALL remain v5.1

#### Scenario: Overall improves but white regresses
- **WHEN** a cell has a positive overall direction but a negative same-color white direction against its required opponent
- **THEN** the candidate SHALL fail that promotion gate and the overall result SHALL NOT hide the white regression

#### Scenario: Required point estimates pass but uncertainty overlaps zero
- **WHEN** all directional point-estimate gates pass but a paired confidence interval includes zero
- **THEN** the report SHALL classify the result as directionally passing but statistically inconclusive rather than claiming demonstrated superiority

### Requirement: Every formal game and accepted proof is independently replayed
The system SHALL replay all 400 formal games from their frozen schedules and SHALL validate opening identity, color assignment, move legality, rule enforcement, terminal result, board integrity, decision provenance, and every accepted proof/certificate before a cell is valid.

#### Scenario: Replay anomaly occurs
- **WHEN** a scheduled opening, move, winner, rule decision, proof certificate, relevance coverage, or final board differs during replay
- **THEN** the affected cell SHALL be invalid, the anomaly SHALL be preserved, and promotion SHALL be blocked until the cause is corrected and the required fresh suite is rerun

#### Scenario: Replay succeeds
- **WHEN** all moves and accepted proof artifacts in a cell replay exactly
- **THEN** the report SHALL record complete replay success with the replay-tool version and checksums

### Requirement: Complete reports are published to both required locations
The system SHALL generate Markdown, JSON summaries, raw per-game records, schedules, freeze/provenance manifests, replay evidence, commands, checksums, and a release decision under a versioned workspace report directory and synchronize the complete bundle to `/Users/wangzicheng/Downloads/logs_five_chess`.

#### Scenario: Report bundle is generated
- **WHEN** diagnostic/performance and formal runs finish
- **THEN** the Markdown SHALL lead with per-rule new-five-star white results, then black and overall, and SHALL include same-color deltas, Wilson and paired uncertainty, proof/relevance/quiet activity, latency, fallbacks, budget exhaustion, commands, profile identities, seeds, and limitations

#### Scenario: Downloads bundle is synchronized
- **WHEN** the workspace report bundle passes checksum verification
- **THEN** the complete bundle SHALL be copied to `/Users/wangzicheng/Downloads/logs_five_chess` and a checksum comparison SHALL prove the workspace and Downloads copies are identical
