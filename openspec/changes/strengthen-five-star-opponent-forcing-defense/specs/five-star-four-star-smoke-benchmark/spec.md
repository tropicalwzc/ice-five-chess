## ADDED Requirements

### Requirement: Frozen controls and diagnostic separation
Before tuning the opponent guard, the evaluation SHALL checksum the UI-bound five-star 5.4.1 control, candidate source/profile, frozen four-star opponent, corpus, rule helpers, proof verifier, benchmark/replay/report tools, and diagnostic inputs. Direct-play failures, synthetic forcing fixtures, their symmetries, and result-selected positions SHALL be diagnostic only and MUST NOT determine runtime coordinates or appear in the fresh smoke opening schedule.

#### Scenario: A frozen input changes
- **WHEN** candidate code, profile parameters, control, opponent, corpus, rules, verifier, runner, or report logic changes after the smoke freeze
- **THEN** the affected results SHALL be invalidated and regenerated from a new checksummed freeze

#### Scenario: Diagnostic position overlaps smoke schedule
- **WHEN** an opening prefix or scheduled pre-decision position overlaps a forcing diagnostic, its symmetry, or a prior result-selected fixture
- **THEN** the schedule SHALL be rejected and regenerated before any game is counted

### Requirement: Targeted opponent-forcing regression matrix
The correctness suite SHALL contain at least one separating black-to-move position where UI-bound 5.4.1 selects a move permitting an independently verified shallow white VCF and a legal alternative completes a comparable disproof. It SHALL additionally cover mandatory-defense continuations, no-corpus exits, inherited four-star `unknown`, corpus conflicts, all eight board symmetries, freestyle and forbidden-black legality, and shallow VCF distances 3, 5, 7, and 9 wherever legal fixtures exist.

#### Scenario: Separating regression is evaluated
- **WHEN** the current control and candidate analyze the same checksummed separating board and deterministic seed
- **THEN** the report SHALL preserve the control failure as diagnostic evidence and require the candidate not to select the verified-losing move when the completed defensive alternative is available

#### Scenario: Symmetry or rule variant is evaluated
- **WHEN** a forcing fixture is transformed or run under its declared rule mode
- **THEN** the candidate SHALL return the correspondingly transformed proof class and legal move, and every accepted certificate/disproof SHALL replay while restoring the board

#### Scenario: No completed defensive alternative exists
- **WHEN** a regression query exhausts with only unknown alternatives
- **THEN** the test SHALL accept an honestly labeled legal unknown result but SHALL NOT count it as proof that the defect is fixed

### Requirement: Fresh paired 48-game four-star smoke schedule
After structural parameters and tools freeze, the benchmark SHALL generate 12 fresh natural freestyle opening identities and run both UI-bound 5.4.1 and the opponent-guard candidate against the same frozen four-star with colors exchanged. Each five-star profile SHALL play 12 games as black and 12 as white, producing exactly 48 valid games.

#### Scenario: Complete smoke run
- **WHEN** all scheduled games finish and replay passes
- **THEN** the dataset SHALL contain 24 control-versus-four-star games and 24 candidate-versus-four-star games with identical 12 opening identities, paired seeds, and exchanged colors

#### Scenario: Game is missing or anomalous
- **WHEN** any game has an illegal move, rule mismatch, wrong profile/color identity, invalid terminal result, unreplayable accepted certificate, board-integrity failure, or decision above 5,000 ms
- **THEN** the affected smoke dataset SHALL be invalid and SHALL NOT support a playable-binding decision

### Requirement: Black-first forcing and strength reporting
The smoke report SHALL lead with five-star-black W/D/L and score rate for candidate and same-schedule control, followed by white and overall results. It SHALL report the count and first ply of independently verified opponent VCF/VCT incidents after selected five-star moves, distinguish avoidable incidents with a completed alternative from unavoidable/unknown cases, and include proof activity, audit coverage, latency, budget exhaustion, randomness mode, commands, provenance, raw data, and checksums.

#### Scenario: Lost black game contains a forcing boundary
- **WHEN** independent replay finds the first selected black move after which frozen four-star has a verified white VCF/VCT
- **THEN** the report SHALL record the board, selected move, proof class/distance/certificate, available candidate classes, guard telemetry, and whether a completed defensive alternative existed

#### Scenario: WDL direction conflicts with forcing incidence
- **WHEN** candidate black W/D/L improves but avoidable verified opponent-forcing incidents increase, or incidents fall while W/D/L declines
- **THEN** the report SHALL present both results separately and SHALL NOT hide the conflict in an overall score

### Requirement: Smoke classification and limited claim
The candidate SHALL be eligible for the playable five-star binding only if all correctness, legality, certificate/disproof replay, board-integrity, determinism, and five-second gates pass; every targeted avoidable shallow-VCF regression is removed; the candidate introduces no new avoidable verified opponent-forcing incident in smoke replay; candidate-black direction is non-negative versus the same-schedule 5.4.1 control; and white plus overall show no obvious smoke regression. The 48-game result SHALL be labeled directional and MUST NOT be reported as a statistically conclusive strength promotion.

#### Scenario: All smoke gates pass
- **WHEN** the targeted suite and complete 48-game dataset satisfy every declared gate
- **THEN** the report MAY recommend binding the candidate to the playable five-star entry while explicitly requiring a larger dual-rule evaluation for any formal strength claim

#### Scenario: Any gate fails
- **WHEN** a correctness anomaly, avoidable regression, new avoidable opponent-forcing incident, negative black direction, obvious white/overall regression, or hard-deadline violation occurs
- **THEN** UI-bound 5.4.1 SHALL remain selected and the failed candidate evidence SHALL be published without tuning on smoke outcomes

#### Scenario: Forbidden-mode claim is requested
- **WHEN** a later report seeks a forbidden-mode playable or strength conclusion beyond targeted correctness
- **THEN** it SHALL run a separately frozen forbidden schedule rather than extrapolating from the freestyle 48-game smoke run
