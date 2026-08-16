# Hybrid preflight verification

Date: 2026-08-15

Candidate: `5.5.0-white-proof-engine-black-v51-stochastic`

## Correctness and isolation

- Strict C11 `-Wall -Wextra -Werror -pedantic` suite: pass.
- One/two/legacy-three-star golden suite: pass.
- Frozen four-star, production-v5.1 rollback, hybrid color routing, board restoration, certificate, multi-seed equivalence, and deadline fixtures: pass.
- Multi-seed random fixture selected both members of an equal completed class and never selected the score-near candidate with a different proof class.
- ASan/UBSan full suite: pass with no finding.
- TSAN full suite, including parallel root-worker fixtures: pass with no data race.
- iOS Simulator Debug build: pass. Existing asset-name and legacy Objective-C warnings remain. Player-facing five-star still routes through `five_star_analysisboard` to `fc_profile_five_star()`/v5.1; the hybrid remains benchmark-only.

## Same-position resource diagnostic

Both rules used openings 0–4, exchanged colors, user randomness, frozen four-star, and play through ply 16.

| Rule | Hybrid decisions | max wall | max process CPU | peak RSS | decisions >5s | random eligibility failures |
|---|---:|---:|---:|---:|---:|---:|
| Freestyle | 44 | 2594.498 ms | 3272.170 ms | 43,941,888 bytes | 0 | 0 |
| Forbidden | 44 | 3316.328 ms | 3308.927 ms | 39,976,960 bytes | 0 | 0 |

Black-v5.1 decisions launched zero proof workers. White proof-engine decisions exercised root-worker batches; the serialized `proofWorkersLaunched` field is a cumulative per-decision launch count across batches, not simultaneous concurrency, whose profile cap remains eight.

These diagnostics are pre-freeze only. Their openings and seed are excluded from final identities.
