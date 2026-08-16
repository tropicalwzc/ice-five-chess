## ADDED Requirements

### Requirement: Controls and stochastic policy freeze before final schedules
The benchmark SHALL checksum the hybrid router, both frozen component profiles, corpus, randomness contract, four-star and legacy controls, replay/report tools, resource limits, and evaluation gates before generating final master seeds or schedules.

#### Scenario: Freeze succeeds
- **WHEN** component-equivalence, randomness-class, correctness, golden, sanitizer, build, and latency gates pass
- **THEN** the system SHALL write a freeze manifest before generating fresh rule-separated schedules

#### Scenario: Frozen input changes
- **WHEN** any router, component, opponent, corpus, random eligibility rule, runner, replay logic, resource limit, or schedule changes after freeze
- **THEN** all affected final games SHALL be invalidated and fresh seeds SHALL be required

### Requirement: Four fresh seeded-stochastic cells are executed
The suite SHALL run no-forbidden and forbidden hybrid-versus-four-star plus hybrid-versus-legacy cells, each containing 50 fresh natural openings with colors exchanged for 100 games, 50 hybrid-white and 50 hybrid-black.

#### Scenario: A final cell runs
- **WHEN** a frozen cell launches
- **THEN** it SHALL use production-style user randomness, log the master and derived decision seeds, run one game process at a time, and use eight workers only inside white proof-engine searches

#### Scenario: Partial outcomes become visible
- **WHEN** any final game result is known
- **THEN** remaining schedules, retry policy, component routing, randomness rules, and gates SHALL remain unchanged

### Requirement: Stochastic replay validates correctness rather than identical realization
The replay system SHALL independently validate every recorded opening, color, seed/provenance record, move, rule decision, terminal result, accepted certificate, randomness equivalence signature, and final board. It SHALL NOT require a separately seeded stochastic run to select the same move sequence.

#### Scenario: Different equivalent choices occur
- **WHEN** two independently seeded evaluations choose different moves that both satisfy the logged completed equivalence signature
- **THEN** the difference SHALL be reported as intended stochastic variation and SHALL NOT invalidate either game

#### Scenario: Random choice crosses an equivalence boundary
- **WHEN** a selected random move differs from the best candidate in a required completed equivalence field
- **THEN** the affected game and cell SHALL be invalid and the hybrid SHALL be ineligible for promotion

#### Scenario: Game replay finds a rule or certificate anomaly
- **WHEN** a move is illegal, a forbidden-black decision is wrong, a terminal result differs, a certificate fails, provenance is missing, or the board is not restored
- **THEN** the affected cell SHALL be invalid and the anomaly SHALL be preserved in the report

### Requirement: Reports compare the correct color perspectives
Each cell SHALL report hybrid white, opponent white, hybrid black, opponent black, and overall W/D/L and score rates. White and black deltas SHALL compare the two models in the same color from exchanged openings, and natural black advantage SHALL not be reweighted.

#### Scenario: White result is reported
- **WHEN** a 100-game cell is summarized
- **THEN** hybrid white SHALL use only the 50 hybrid-white games and opponent white SHALL use only the paired 50 opponent-white games

#### Scenario: Black result is reported
- **WHEN** a 100-game cell is summarized
- **THEN** hybrid black SHALL use only the 50 hybrid-black games and opponent black SHALL use only the paired 50 opponent-black games

### Requirement: Evaluation gates test the color-specialization hypothesis
Against frozen four-star in each rule, hybrid white score rate SHALL be at least 50%, hybrid same-color black direction SHALL be non-negative, and hybrid overall score rate SHALL be at least 50%. Against legacy three-star in each rule, hybrid white and overall directions SHALL be non-negative. Wilson and paired uncertainty SHALL be reported, and no pooled rule or color result SHALL hide a failing cell.

#### Scenario: White reaches fifty percent against four-star
- **WHEN** hybrid white scores at least 50% but its uncertainty interval overlaps a weaker result
- **THEN** the point-estimate gate SHALL pass but the report SHALL classify strength as statistically inconclusive rather than demonstrated

#### Scenario: Hybrid overall improves but a required cell fails
- **WHEN** a pooled or overall result is positive but one rule-specific white, black, overall, correctness, or latency gate fails
- **THEN** the candidate SHALL fail the evaluation and production SHALL remain v5.1

### Requirement: Complete reports are checksummed and synchronized
The system SHALL publish Markdown, JSON summaries, raw games, schedules, freeze/provenance manifests, replay evidence, commands, latency/activity/randomness telemetry, limitations, release decision, and checksums under the workspace and `/Users/wangzicheng/Downloads/logs_five_chess`.

#### Scenario: Bundle is complete
- **WHEN** all four cells and replay audits finish
- **THEN** the report SHALL lead with hybrid-white results and SHALL include black/overall strength, stochastic variation, Wilson/paired uncertainty, routing evidence, latency, worker use, corpus activity, and every failure

#### Scenario: Downloads synchronization completes
- **WHEN** the workspace bundle passes its checksum audit
- **THEN** the Downloads copy SHALL be byte-identical according to the same manifest
