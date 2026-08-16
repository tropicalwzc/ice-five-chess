## 1. Profile and API

- [x] 1.1 Add the isolated 5.4.1 scheduler profile factory with persistent-pool, token-block, ledger, deadline, and memory settings while leaving the baseline factory unchanged.
- [x] 1.2 Add the scheduler hybrid component identity and public profile/analyzer declarations.
- [x] 1.3 Add the direct analyzer route with validated 1/4/8 worker overrides and fixed aggregate budget semantics.

## 2. Benchmark and correctness coverage

- [x] 2.1 Add `five-star-v541-thread-scheduler` profile selection and CLI help to the benchmark.
- [x] 2.2 Extend focused AI tests for profile isolation, snapshot identity, worker caps, pool reuse, token returns, certificate validity, and deterministic board restoration.
- [x] 2.3 Run the benchmark build, AI test suite, and fixed-position baseline/candidate worker matrix; record wall/CPU and scheduler telemetry.

## 3. Formal replay and report

- [x] 3.1 Run the candidate against the frozen four-star control in free and forbidden-black 100-game cells.
- [x] 3.2 Run the candidate against the frozen legacy three-star control in free and forbidden-black 100-game cells.
- [x] 3.3 Validate all 400 games for legality, anomalies, deadlines, certificates, deterministic identities, and budget/memory accounting.
- [x] 3.4 Generate a concise speed-and-strength report, compare the candidate to baseline 5.4.1, and classify strength conservatively.
