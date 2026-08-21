## ADDED Requirements

### Requirement: Targeted sentinel policy comparison
The evaluation SHALL compare fixed depth-5, fixed depth-7, and adaptive depth-5/7 sentinel policies on frozen targeted fixtures before selecting a runtime candidate.

#### Scenario: Required fixture coverage
- **WHEN** the targeted comparison is run
- **THEN** it SHALL include independently replayable VCF distances 3, 5, 7, and 9 where available, the three reconstructed late-correction positions from the direct 5.8.0-versus-5.4.1 test, both rule modes, and relevant symmetries

#### Scenario: Per-policy measurements
- **WHEN** a policy completes the targeted suite
- **THEN** the report SHALL include loss recall by certificate distance, false rejection count, certificate replay results, p50/p95/max time, nodes, cache reuse, downstream budget preserved, final move changes, later full-guard catches, and deadline behavior

### Requirement: Correctness-first policy selection
The evaluation SHALL select the lowest-cost policy only after it passes certificate, classification, integrity, deterministic, and in-scope recall gates.

#### Scenario: Eligible policy
- **WHEN** a policy has zero false rejections, every reported winning certificate replays, all in-scope fixtures at or below its effective depth are caught, the board is restored, and repeated deterministic runs agree
- **THEN** it SHALL be eligible for cost comparison

#### Scenario: Cheapest eligible policy
- **WHEN** multiple policies satisfy every correctness gate
- **THEN** the policy with the lowest sentinel p95 elapsed time SHALL be selected, using lower node consumption as the tie-breaker

#### Scenario: Adaptive recall regression
- **WHEN** the adaptive policy misses an in-scope verified loss caught by fixed depth 7
- **THEN** the adaptive policy SHALL NOT be selected solely because it is cheaper

#### Scenario: No useful bounded policy
- **WHEN** no bounded policy passes the declared gates or none catches a useful share of the reconstructed late-correction positions
- **THEN** the evaluation SHALL retain the existing final guard, publish the negative result, and SHALL NOT silently increase the sentinel beyond the proposed ranges

### Requirement: Frozen fresh paired test against exact 5.4.1
After selecting and freezing a sentinel policy, the evaluation SHALL run a deterministic directional paired test against the exact UI-bound `5.4.1-transactional-deadline-root-parallel-5s` profile.

#### Scenario: Fresh opening schedule
- **WHEN** the quick match schedule is generated
- **THEN** it SHALL contain 6–8 fresh natural freestyle opening identities excluded from openings 60–65, targeted fixtures, corpus-derived diagnostics, and prior result-selected positions

#### Scenario: Colors exchanged
- **WHEN** the quick test runs
- **THEN** each opening SHALL be played twice with candidate and 5.4.1 exchanging black and white for 12–16 total games under deterministic-best play

#### Scenario: Frozen provenance
- **WHEN** match evidence is published
- **THEN** it SHALL identify and checksum the candidate profile, selected sentinel parameters, 5.4.1 control, proof engine, rule helpers, corpus, runner, opening schedule, seed, and raw JSONL

### Requirement: Directional report leads with defensive evidence
The quick-test report SHALL lead with candidate-black results and distinguish match outcome from verified defensive behavior and runtime health.

#### Scenario: Required result sections
- **WHEN** the paired test completes
- **THEN** the report SHALL present black, white, and overall W/D/L; paired opening outcomes; early verified-loss catches; losses caught only by the final guard; final move changes; sentinel and total p50/p95/max latency; anomalies; and decisions above 5,000 ms

#### Scenario: Small-sample interpretation
- **WHEN** conclusions are written from 12–16 games
- **THEN** they SHALL be labeled directional and SHALL NOT claim statistical superiority or theoretical color advantage

#### Scenario: Deadline violation
- **WHEN** any decision exceeds 5,000 ms or the runner reports an anomaly
- **THEN** the report SHALL identify the affected game and move and the candidate SHALL fail the runtime-health gate

### Requirement: UI promotion remains out of scope
The evaluation SHALL leave the playable five-star UI binding on its current profile regardless of the quick-test outcome.

#### Scenario: Positive quick result
- **WHEN** the sentinel candidate improves W/D/L or defensive evidence in the quick test
- **THEN** the system SHALL publish the result without changing the UI binding

#### Scenario: Negative or neutral quick result
- **WHEN** the sentinel candidate is neutral or worse in the quick test
- **THEN** the system SHALL preserve 5.4.1 as the playable control and retain the isolated candidate only as research evidence
