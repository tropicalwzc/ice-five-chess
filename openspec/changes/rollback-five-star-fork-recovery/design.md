## Context

`recoverySearchEnabled` currently controls two different concerns: the recovery
contract introduced by 5.7 and the newer fork-first candidate layer.  Turning it
off would therefore roll back more than requested and would remove the legal
fallback, shared decision deadline, and old escape search that the next
multithreading study needs.  The latest fork layer also annotates candidates and
changes selection order, so it must be isolated at every entry point rather than
only at the final chooser.

## Goals / Non-Goals

**Goals:**

- Restore the default v5.7 move-selection path with a narrow, auditable gate.
- Keep old recovery, root/escape parallel proof, incremental legality, and the
  unified ledger active for the baseline.
- Retain a named opt-in profile so the fork experiment remains reproducible.
- Make snapshots and tests expose which behavior is active.

**Non-Goals:**

- No deletion or rewriting of reports, fixtures, or OpenSpec history.
- No thread-pool, shared-TT, or budget-scheduler optimization in this rollback.
- No change to frozen controls, production/UI routing, or rule semantics.

## Decisions

### 1. Add a separate `forkFirstRecoveryEnabled` profile field

Use a boolean independent of `recoverySearchEnabled`.  The default v5.7
candidate sets it to false; a named fork-recovery opt-in profile copies v5.7 and
sets it to true.  This is safer than a version-string check because tests and
future worker profiles can select the behavior explicitly.

### 2. Gate only fork-layer behavior

Use the new field for fork candidate construction, fork-risk ranking, fork
telemetry, and invalid-handoff fork probing.  Continue using
`recoverySearchEnabled` for full-board legal fallback, old loss-aware escape,
ledger activation, and the existing 5.7 recovery contract.

### 3. Preserve accounting and evidence

The reversible memory reservation/release code remains enabled as diagnostics;
it does not select moves and is useful for the upcoming worker study.  Existing
reports and fixture manifests remain immutable.  The default benchmark profile
is v5.7, while fork-specific fixture tests use the explicit opt-in profile.

### 4. Verify the baseline before scheduler work

Run the existing unit target and a short 1/4/8-worker replay after the gate is
implemented.  Compare selected moves, legality, decision status, useful proof
jobs, CPU time, and concurrency against the recorded 5.7 evidence before
proposing a persistent scheduler.

## Risks / Trade-offs

- **[Risk]** A fork-specific test may silently use the baseline profile. →
  **Mitigation:** use the named opt-in profile and assert its gate in tests.
- **[Risk]** A missed gate site could leave fork ranking active in the default
  profile. → **Mitigation:** centralize the predicate and assert zero fork
  probes in a baseline smoke test.
- **[Risk]** Keeping ledger release accounting means this is not a byte-for-byte
  source rewind. → **Mitigation:** document it as an intentional behavior-level
  rollback and retain the pre-change reports as the comparison reference.

## Migration Plan

1. Add the field, default profile values, opt-in profile, and serialization.
2. Replace only fork-layer checks with the new predicate and update fork tests.
3. Build and run unit/fixture tests plus the fixed worker replay.
4. Use the restored 5.7 profile as the control for a separate multithreading
   optimization change.

## Open Questions

- Whether the next scheduler experiment should use a persistent pool or first
  reduce atomic-token contention with block reservations.
- Whether a shared read-only TT is worthwhile after the baseline wall/CPU
  profile is recorded.
