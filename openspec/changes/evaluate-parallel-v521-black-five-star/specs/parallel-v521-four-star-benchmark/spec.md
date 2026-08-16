## ADDED Requirements

### Requirement: Inputs are frozen before formal schedules
The evaluation SHALL checksum candidate/control profiles, source, binaries, frozen four-star, corpus, random contract, replay/report tools, resource limits, and gates before generating formal seeds or schedules.

#### Scenario: A frozen input changes
- **WHEN** candidate, control, opponent, schedule, runner, rule helper, random eligibility, resource limit, or evaluation logic changes after freeze
- **THEN** every affected formal cell SHALL be invalidated and regenerated from fresh seeds

### Requirement: One-worker and eight-worker candidates use identical schedules
For each rule, the benchmark SHALL run the one-worker-control hybrid and eight-worker candidate hybrid against frozen four-star on the same 50 natural openings, master seed, color exchange, user-random contract, and 5,000 ms ceiling.

#### Scenario: A rule A/B runs
- **WHEN** Freestyle or forbidden evaluation begins
- **THEN** each profile SHALL play 100 games against four-star with 50 hybrid-white and 50 hybrid-black games

#### Scenario: Resources are assigned
- **WHEN** a formal game runs
- **THEN** only one game process SHALL run at a time, eight-worker candidate components MAY use eight internal workers, the one-worker black control SHALL remain serial, and frozen four-star SHALL remain single-threaded

### Requirement: Fresh Freestyle and forbidden evidence is independently replayed
The suite SHALL use fresh rule-separated openings excluded from prior formal, diagnostic, corpus-shaped, and failure identities, and SHALL replay every recorded move using production C rule helpers.

#### Scenario: A recorded cell completes
- **WHEN** 100 games have been written
- **THEN** replay SHALL validate schedule/color identity, seeds, component routing, every move, forbidden legality, terminal result, certificate metadata, random eligibility, board integrity, and the 5,000 ms limit

#### Scenario: Independently seeded choices differ
- **WHEN** stochastic runs select different legal moves within their completed eligible classes
- **THEN** the difference SHALL NOT be treated as a replay failure

### Requirement: Reports compare same-color strength and threading benefit
Each rule report SHALL lead with candidate white, report candidate and four-star black scores in the same color, report overall W/D/L, and compare eight-worker candidate black/overall scores with the one-worker control on the same openings.

#### Scenario: Black strength is calculated
- **WHEN** black results are summarized
- **THEN** candidate-black SHALL be compared with four-star-black from exchanged games and SHALL NOT be compared with candidate-white

#### Scenario: Thread benefit is calculated
- **WHEN** candidate and control cells are complete
- **THEN** the report SHALL provide paired point differences and uncertainty for eight-worker minus one-worker black and overall scores

### Requirement: Rule-specific gates determine the conclusion
For each rule, candidate white SHALL score at least 50%, candidate same-color black advantage over four-star SHALL be non-negative, and candidate overall SHALL score at least 50%. Eight-worker black and overall scores SHALL be no lower than the one-worker control, with at least one strict point-estimate improvement across those two measures. Correctness and latency SHALL pass with zero anomalies.

#### Scenario: Point estimate improves with overlapping uncertainty
- **WHEN** the eight-worker point estimate is positive but its paired interval overlaps zero
- **THEN** the report SHALL classify threading benefit as inconclusive rather than demonstrated

#### Scenario: One rule fails
- **WHEN** either rule fails a required strength, threading, correctness, or latency gate
- **THEN** pooled results SHALL NOT hide the failure and production SHALL remain v5.1

### Requirement: Complete evidence is published and checksummed
The system SHALL publish Markdown, JSON, raw games, replay evidence, schedules, freeze/provenance manifests, commands, latency/CPU/worker/RSS telemetry, limitations, release decision, and checksums in the workspace and `/Users/wangzicheng/Downloads/logs_five_chess`.

#### Scenario: Downloads synchronization completes
- **WHEN** the workspace bundle passes its checksum audit
- **THEN** the Downloads copy SHALL be byte-identical according to the same manifest
