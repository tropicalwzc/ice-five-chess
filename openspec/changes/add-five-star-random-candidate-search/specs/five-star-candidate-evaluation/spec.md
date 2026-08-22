## ADDED Requirements

### Requirement: Paired frozen-four-star evaluation

The evaluator SHALL run the candidate and incumbent against the same frozen four-star profile, ordered opening IDs, rule mode, random mode, move limit, and engine build for each comparison round.

#### Scenario: Candidate and incumbent share a sample

- **WHEN** a training or validation comparison starts
- **THEN** both profiles receive the exact same 12 opening IDs and the header records one common sample manifest

#### Scenario: Frozen opponent identity is verified

- **WHEN** a comparison JSONL file is written
- **THEN** its header contains the expected frozen four-star profile name, version, and snapshot hash, and a mismatch invalidates the comparison

### Requirement: Independent validation before promotion

The evaluator SHALL run a surviving candidate on a fresh 12-opening sample that is distinct from its paired training sample before reporting the candidate as promotable.

#### Scenario: Training winner fails fresh validation

- **WHEN** a candidate beats the incumbent on the paired training sample but fails the fresh validation floor or objective
- **THEN** the candidate remains rejected and cannot update the current champion pointer

#### Scenario: Validation passes in both rule modes

- **WHEN** a candidate reaches at least 0.50 black score rate and improves the shared objective in both no-forbidden and forbidden-black validation cells
- **THEN** the evaluator marks the node eligible for promotion and records both cell results

### Requirement: Rule-mode separated scoring

The evaluator SHALL report no-forbidden and forbidden-black black W/D/L, score rate, latency, and anomaly counts separately, and SHALL NOT combine them into a single unqualified win rate.

#### Scenario: Rule modes disagree

- **WHEN** a candidate improves no-forbidden score but falls below 0.50 with forbidden black
- **THEN** the shared candidate is rejected and the report identifies the forbidden-black failure

#### Scenario: Rule-specific branch is retained

- **WHEN** a user explicitly enables rule-specific research branching
- **THEN** the result is stored under a separate branch identity and is not presented as the shared champion

### Requirement: Replay and anomaly integrity gate

The evaluator SHALL invalidate a cell for promotion when a game is anomalous, exceeds the decision hard limit, fails board restoration, fails certificate replay, or has an invalid/missing sample manifest.

#### Scenario: An invalid game is present

- **WHEN** any game in a required cell has an anomaly or replay/certificate failure
- **THEN** the cell is marked invalid, excluded from promotion scoring, and the node remains rejected

#### Scenario: Clean cell is replayed

- **WHEN** every game in a cell has valid moves, restored board state, and replay-verified telemetry
- **THEN** the evaluator includes the cell in W/D/L and latency summaries and records the replay digest

### Requirement: Search provenance report

The evaluator SHALL emit raw JSONL, a machine-readable round summary, and a human-readable report containing node ancestry, sample seeds and IDs, profile/opponent snapshots, W/D/L, score rates, latency percentiles, anomalies, replay status, and promotion reason.

#### Scenario: Round can be audited offline

- **WHEN** a reviewer has the ledger entry, raw JSONL, and recorded source/build hashes
- **THEN** the reviewer can reproduce the sampled openings and determine why the node was promoted or rejected without external services

#### Scenario: Existing evidence is preserved

- **WHEN** a new search round writes results
- **THEN** it uses a new dated output directory under `Downloads/logs_five_chess` and does not overwrite prior standard or candidate evidence
