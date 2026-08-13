## ADDED Requirements

### Requirement: Tactical proof regression suite
The validation suite SHALL include curated VCF, VCT, multiple-defense, false-threat, counter-win, edge, forbidden-move, and no-solution positions with expected proof class and, where applicable, winning distance. Each applicable position SHALL be tested under all eight board symmetries, and accepted winning results SHALL pass the independent certificate verifier.

#### Scenario: Solver finds the expected VCT
- **WHEN** a labeled VCT position is evaluated within its declared node budget
- **THEN** the returned proof class and winning distance SHALL match the expected result and its certificate SHALL verify under every symmetry

#### Scenario: False positive is detected
- **WHEN** the solver claims a win in a labeled refutation position
- **THEN** the suite SHALL fail even if the selected first move matches a historical move

### Requirement: Dual held-out paired legacy matches
After profiles and opening assets are frozen, the benchmark SHALL run two formal matches against the frozen legacy three-star implementation. With opening-book lookup enabled it SHALL play 100 games as black and 100 games as white; with lookup disabled it SHALL repeat 100 games as black and 100 games as white. Both configurations SHALL use the same 100 held-out opening identities, exchanged engine colors, paired deterministic seeds, identical non-book search/rule/budget policies, and no silent removal of anomalies.

#### Scenario: Both formal schedules complete
- **WHEN** all 100 held-out openings have been played with exchanged engine colors in both configurations
- **THEN** the result bundle SHALL contain exactly 200 book-on games and 200 book-off games, each split into 100 new-black and 100 new-white games, or mark the affected complete schedule invalid if any required game is missing or anomalous

### Requirement: White performance is a primary metric
The report SHALL present new-engine white wins, draws, losses, score rate, Wilson 95% interval, and change relative to the frozen legacy engine playing white in the directly corresponding new-engine-as-black games before the overall metric. The paired white comparison SHALL use the same held-out opening identities. The separate legacy-versus-legacy control SHALL be reported only as a supplemental schedule-reproduction and natural-color baseline and SHALL NOT drive the white delta or strength classification. The report SHALL preserve the natural black advantage of the selected rule and SHALL NOT rebalance results by artificial color weighting.

#### Scenario: Black wins most games
- **WHEN** the completed suite shows a strong natural first-player advantage
- **THEN** the report SHALL retain the raw color results and judge white improvement explicitly rather than dismissing the suite solely because the aggregate is close to 50%

#### Scenario: Independent control differs from the direct opponent result
- **WHEN** the designated white score in the legacy-versus-legacy control differs from the legacy engine's white score in the new-model match
- **THEN** the report SHALL calculate white improvement from the direct-match legacy-white result and show the independent control only as a supplemental baseline

### Requirement: Strength claim classification
The report SHALL classify the result as `demonstrated stronger`, `directional improvement`, or `not demonstrated`. `Demonstrated stronger` SHALL require an overall 95% interval excluding 50%, no tactical regression, and a positive white-score change against the frozen legacy engine's direct-match white result; `directional improvement` SHALL require positive overall and white point estimates while at least one interval remains inconclusive.

#### Scenario: Overall score rises but white regresses
- **WHEN** the new engine scores above 50% overall but its white score does not exceed the frozen legacy engine's direct-match white score
- **THEN** the report SHALL NOT classify the result as `demonstrated stronger`

### Requirement: Standalone and comparative opening-book reports
The system SHALL generate a standalone Markdown and machine-readable report for the 200-game book-on match, a standalone Markdown and machine-readable report for the 200-game book-off match, and a third Markdown comparison report. The comparison SHALL present book-on minus book-off overall and by color, book coverage, move-source attribution, latency, proof activity, and uncertainty so that opening knowledge is not presented as proof-search gain.

#### Scenario: Improvement comes only from book moves
- **WHEN** the formal book-on match improves but the formal book-off match does not improve over frozen legacy
- **THEN** the comparison report SHALL attribute the observed gain to opening coverage and SHALL NOT claim that proof search alone became stronger

### Requirement: Performance, reproducibility, and report artifacts
The benchmark SHALL record per-move elapsed time, proof nodes, proof/disproof counts, transposition hits, completed stage/depth, budget exhaustion, certificate failures, illegal moves, default source, override reason, and book hit. It SHALL generate Markdown plus machine-readable raw data and checksums in the repository, then copy the complete report bundle to `/Users/wangzicheng/Downloads/logs_five_chess` after authorization.

#### Scenario: Report is regenerated
- **WHEN** the documented command is rerun using the recorded commit, profiles, library checksum, schedule, and seeds
- **THEN** game outcomes, proof decisions, and report aggregates SHALL be reproducible
