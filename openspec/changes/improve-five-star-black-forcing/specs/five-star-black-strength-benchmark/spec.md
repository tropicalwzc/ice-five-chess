## ADDED Requirements

### Requirement: Prior-loss diagnostic isolation
The system SHALL extract the prior five-star-black losses and all eight symmetries as checksummed diagnostics, but SHALL NOT encode their opening IDs, opponent continuations or outcome-selected coordinates into production or count them as new formal strength evidence.

#### Scenario: Historical loss becomes a regression fixture
- **WHEN** a prior loss is imported for diagnosis
- **THEN** the fixture SHALL assert proof, legality, ranking and determinism properties without selecting a production move solely from the historical result

### Requirement: Fresh rule-separated four-cell evaluation
After implementation and parameter freeze, five-star SHALL play exactly 100 games in each of four cells: no-forbidden versus frozen four-star, no-forbidden versus frozen legacy three-star, forbidden versus frozen four-star, and forbidden versus frozen legacy three-star. Each cell SHALL use 50 natural opening identities with five-star black in 50 games and white in 50 games.

#### Scenario: Formal suite completes
- **WHEN** the four formal cells finish
- **THEN** the bundle SHALL contain exactly 400 valid games with correct rule/opponent/color identities or mark an affected cell invalid

### Requirement: Untouched natural schedules
Formal seeds and openings SHALL be generated independently of the corpus and prior loss set after code, profile and diagnostic parameters are frozen. No deliberately matchable, prior-failure-derived or corpus-derived opening SHALL count as formal evidence.

#### Scenario: Opening was selected from a prior loss
- **WHEN** a formal opening or seed was chosen because it reproduces a historical failure or corpus match
- **THEN** the affected formal cell SHALL be invalid

### Requirement: Black-first color-correct reporting
Reports SHALL lead with five-star-black W/D/L, score and paired change from the corresponding baseline behavior, followed by five-star white and overall results. White and overall point estimates SHALL be reported without color reweighting and SHALL be non-negative in every cell for promotion.

#### Scenario: Black improves but white regresses
- **WHEN** five-star-black improves but five-star white or overall point estimate declines in any rule/opponent cell
- **THEN** the result SHALL be classified as not promotion-demonstrated

### Requirement: Reproducible replay and reports
Every game SHALL be replayed for legality, rule enforcement, terminal identity and schedule identity; every accepted proof certificate SHALL be verified. The bundle SHALL include Markdown, JSON summaries, raw JSONL, diagnostic ablations, latency/proof activity, hashes, commands and checksums in the workspace and authorized Downloads directory.

#### Scenario: Replay or proof anomaly occurs
- **WHEN** a five-star/four-star illegal move, schedule mismatch or accepted certificate failure is detected
- **THEN** the affected formal cell SHALL be invalid and the anomaly SHALL NOT be omitted from the report
