## ADDED Requirements

### Requirement: Root-cause report for the failed opening book
The benchmark tooling SHALL correlate the previous book-on/off games by opening identity, five-star color, first differing decision, book ID/ply, proof state, and result, and SHALL publish a Markdown explanation separating corpus-content, selection-policy, and search-gate causes.

#### Scenario: A book move changes the outcome
- **WHEN** paired book-on/off games diverge at a book decision
- **THEN** the report SHALL show the decision metadata and downstream outcome without claiming causation beyond the recorded evidence

### Requirement: Dual-baseline held-out formal matches
After corpus, thresholds, and profile are frozen, five-star SHALL play exactly 100 games as black and 100 as white against book-blind four-star, and exactly 100 as black and 100 as white against frozen legacy three-star, using a new held-out seed/opening domain not used for source selection or diagnostics.

#### Scenario: Formal suite completes
- **WHEN** both opponent schedules finish
- **THEN** the bundle SHALL contain 400 valid games with no omitted anomaly, or SHALL mark the affected formal schedule invalid

### Requirement: Natural rule-separated evaluation
Corpus coverage, threshold selection, and formal strength evidence SHALL use seeded schedules generated independently of corpus contents. Corpus-derived or deliberately matchable starts SHALL be limited to unit tests and SHALL NOT count as diagnostic or formal evidence. Forbidden and no-forbidden modes SHALL each run 100 games against four-star and 100 against frozen legacy three-star, with five-star playing 50 as black and 50 as white in each rule/opponent cell.

#### Scenario: A schedule was selected to increase corpus hits
- **WHEN** an opening or seed was chosen because it matches a corpus entry
- **THEN** the affected diagnostic or formal result SHALL be invalid

### Requirement: Anti-overfitting and release classification
Reports SHALL lead with five-star white W/D/L, score and Wilson interval for each opponent, followed by black and overall results, paired uncertainty, corpus coverage, fallback/rejection reasons, proof activity, latency, anomalies, and checksums. Promotion SHALL require non-negative white and overall point estimates against both opponents and no tactical/correctness regression.

#### Scenario: Five-star improves against four-star but regresses against legacy
- **WHEN** the primary comparison improves but either white or overall point estimate is negative against frozen legacy
- **THEN** the result SHALL be classified as not generalization-demonstrated and the five-star player entry SHALL remain disabled

### Requirement: Reproducible reports and external bundle
The system SHALL generate standalone Markdown and machine-readable reports for both 200-game matches plus a comparison/root-cause report, replay every move, verify accepted proof certificates, checksum all artifacts, and copy the complete bundle to `/Users/wangzicheng/Downloads/logs_five_chess` after authorization.

#### Scenario: Bundle is regenerated
- **WHEN** the documented command runs with recorded sources, corpus hash, profiles, schedule, and seeds
- **THEN** game results, aggregates, reports, and checksums SHALL be reproducible
