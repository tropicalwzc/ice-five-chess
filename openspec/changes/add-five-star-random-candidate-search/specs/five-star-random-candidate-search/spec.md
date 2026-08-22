## ADDED Requirements

### Requirement: Deterministic random opening sampling

The search driver SHALL select exactly 12 distinct training opening IDs per round without replacement, using a recorded fixed-width round seed, and SHALL persist the ordered IDs before executing the paired games.

#### Scenario: Repeating a round reproduces its sample

- **WHEN** the driver is invoked with the same opening-pool version, round seed, sample size, and source/build inputs
- **THEN** it produces the same ordered 12 opening IDs and the same sample manifest

#### Scenario: Training sampling excludes held-out openings

- **WHEN** a normal training round samples openings
- **THEN** every selected ID belongs to the configured generated training pool and no formal held-out opening is selected

### Requirement: Explicit one-factor candidate nodes

The search driver SHALL create each candidate node from a parent node and SHALL record exactly one declared parameter or policy mutation, the canonical profile snapshot, the source/build hash, and a stable node identity.

#### Scenario: A candidate mutation is reproducible

- **WHEN** a candidate node is recreated from its parent snapshot and mutation manifest
- **THEN** the resulting profile snapshot and node identity match the ledger entry

#### Scenario: Unknown mutations are rejected

- **WHEN** a mutation manifest contains an unsupported field, invalid range, or non-canonical value
- **THEN** the driver rejects the node before launching a game and leaves the current champion unchanged

### Requirement: Incumbent champion lineage

The search driver SHALL initialize the shared champion from the exact 5.8.1 profile, SHALL promote only validated nodes, and SHALL preserve parent/child lineage and rejection reasons in an append-only ledger.

#### Scenario: Failed candidate does not replace the champion

- **WHEN** a candidate fails a training or validation gate
- **THEN** the ledger records the candidate as rejected with its evidence paths and the current-pointer continues to identify the prior champion

#### Scenario: Validated candidate becomes the next parent

- **WHEN** a candidate passes every configured rule-mode gate and improves the incumbent objective
- **THEN** the driver writes a finalized promoted node and uses that node as the parent for the next round

### Requirement: Fifty-percent black safety floor

The search driver SHALL reject a candidate whose black score rate is below 0.50 in any required rule mode, where score rate is `(wins + 0.5 * draws) / games`.

#### Scenario: Candidate below the floor is rejected

- **WHEN** a 12-game rule-mode sample produces fewer than six score-equivalent points for the candidate as black
- **THEN** the candidate is rejected without independent promotion and the reason identifies the failed 50% floor

#### Scenario: Exact threshold remains eligible for comparison

- **WHEN** a candidate scores exactly 0.50 in a required rule mode and has no anomalies
- **THEN** it remains eligible for comparison against the incumbent but is not promoted unless the complete objective improves

### Requirement: Research-only isolation

The search workflow SHALL keep candidate nodes, overrides, and promotion pointers outside the playable profile binding and SHALL not modify the exact 5.8.1 or frozen four-star control identities.

#### Scenario: Research promotion does not change runtime binding

- **WHEN** a candidate node is promoted in the search ledger
- **THEN** the playable binding and control profile snapshots remain byte-for-byte unchanged
