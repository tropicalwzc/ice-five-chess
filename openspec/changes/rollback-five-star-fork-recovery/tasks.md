## 1. Profile and gate

- [x] 1.1 Add `forkFirstRecoveryEnabled` to the profile contract and profile snapshot serialization.
- [x] 1.2 Make default v5.7 disable the fork-first layer and add a named opt-in fork-recovery profile.
- [x] 1.3 Route all fork-layer entry points and fork-specific chooser behavior through the new gate without changing legacy recovery.

## 2. Regression coverage

- [x] 2.1 Update fork-specific unit tests and the fixture runner to use the explicit opt-in profile.
- [x] 2.2 Add assertions that default v5.7 retains parallel/legality/ledger settings and emits no fork probes on a baseline smoke case.
- [x] 2.3 Verify profile snapshots and historical fixture metadata remain distinguishable and unchanged.

## 3. Verification and handoff

- [x] 3.1 Build and run the existing C/unit test target with warnings enabled.
- [x] 3.2 Run a short fixed 1/4/8-worker baseline replay and validate legality, determinism, budget, and useful-work telemetry.
- [x] 3.3 Record the restored baseline and the next multithreading experiment hypotheses without modifying historical reports.
