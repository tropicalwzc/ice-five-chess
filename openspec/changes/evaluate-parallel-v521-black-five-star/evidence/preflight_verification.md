# Parallel v5.2.1 preflight verification

Date: 2026-08-15

## Correctness and isolation

- Strict C11 `-Wall -Wextra -Werror -pedantic`: pass.
- Existing one/two/legacy-three-star, frozen four-star, v5.1, v5.2.1, 5.4.1, prior hybrid, rule, proof, certificate, randomness, deadline, and board-integrity suite: pass.
- New profile fixtures prove parallel v5.2.1 does not enable `proofEngineCandidate`; inherited v5.2.1 policy fields match the source profile.
- New routing fixtures pass for both rules: white is 5.4.1, black is the selected one/eight-worker v5.2.1 component, four-star launches no candidate worker.
- Parallel-root fixture validates completed proof precedence, verified certificate, board restoration, worker cap, cumulative job telemetry, private make/unmake balance, and no nested worker creation.
- ASan/UBSan full suite: pass, zero findings.
- TSAN full suite: pass, zero data-race findings.
- iOS Simulator Debug build: pass. Existing asset-name, deployment-target, and legacy Objective-C warnings remain; no new build error was introduced.
- UI inspection: `five_star_analysisboard` still assigns `fc_profile_five_star()` and calls the production v5.1 entry point. New hybrid selectors occur only in benchmark/test code.

## Same-position resource diagnostics

Diagnostics use aggregated held-out positions and are excluded from formal identities.

| Rule/profile | Games | Black decisions | Black decisions launching workers | Cumulative worker launches | Configured black cap | Four-star worker launches | max candidate step | >5s | anomalies |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Freestyle, eight-worker | 40 | 20 | 12 | 206 | 8 | 0 | 4546.917 ms | 0 | 0 |
| Forbidden, eight-worker | 40 | 20 | 12 | 121 | 8 | 0 | 2714.391 ms | 0 | 0 |
| Freestyle, one-worker | 20 | 10 | 0 | 0 | 1 | 0 | 4547.329 ms | 0 | 0 |
| Forbidden, one-worker | 20 | 10 | 0 | 0 | 1 | 0 | 2695.230 ms | 0 | 0 |

`proofWorkersLaunched` is cumulative across batches in one decision. `proofWorkerCap` is the simultaneous concurrency limit and is eight for the candidate. Worker jobs set the parallel flag false and worker count one, preventing nested thread creation.
