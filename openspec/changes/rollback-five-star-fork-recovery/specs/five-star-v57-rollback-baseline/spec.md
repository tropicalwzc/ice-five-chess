## ADDED Requirements

### Requirement: The default 5.7 profile disables only fork-first recovery

The default `fc_profile_five_star_v57_hybrid_candidate` SHALL disable the
experimental fork-first candidate layer while retaining legacy recovery,
root/escape parallel proof, incremental legality, and the unified decision
ledger settings.

#### Scenario: Default profile is a pre-fork-first baseline

- **WHEN** the default v5.7 profile is created
- **THEN** `forkFirstRecoveryEnabled` is false
- **AND** `recoverySearchEnabled`, parallel proof, incremental legality, and the
  4,500/5,000 ms decision contract remain enabled as before

#### Scenario: Legacy recovery remains available

- **WHEN** the default v5.7 search reaches an invalid handoff or needs a legal
  fallback
- **THEN** the existing recovery/fallback path remains eligible
- **AND** disabling the fork-first layer does not change frozen four-star or
  production profile behavior

### Requirement: Fork recovery is explicit and reproducible

The fork-first behavior SHALL be available only through a profile that
explicitly enables `forkFirstRecoveryEnabled`, and profile snapshots SHALL
serialize the flag.

#### Scenario: Fork fixture opts in

- **WHEN** a fork-recovery fixture uses the named opt-in profile
- **THEN** fork probing, fork-risk ordering, and fork telemetry are enabled
- **AND** the snapshot distinguishes it from default v5.7

#### Scenario: Baseline smoke test does not probe forks

- **WHEN** the default v5.7 profile evaluates a non-terminal fixture without an
  immediate win or mandatory defense
- **THEN** the fork-first candidate counters remain zero
- **AND** candidate selection uses the pre-fork-first ordering

### Requirement: Existing worker and budget behavior is preserved

The rollback SHALL NOT alter the existing v5.7 worker-count override, private
worker state, incremental forbidden-legality cache, reversible memory ledger, or
shared internal/hard deadline contract.

#### Scenario: Worker matrix remains valid

- **WHEN** the same fixed position is run with 1, 4, and 8 requested workers
- **THEN** each request is accepted and selected output remains legal and
  deterministic under the restored profile
- **AND** useful proof work, CPU time, and actual concurrency remain reportable
