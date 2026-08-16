# DFPN expanded-graph deadline incident

During the invalidated revision-4 formal run, two no-forbidden benchmark processes stopped making game progress at opening 10 while each consumed roughly one CPU core. Sampling placed both in `fc_dfpn_search` through `fc_prove_forced_win` and the five-star candidate analysis path.

The loop checked deadline and node budgets only when expanding a new graph node. An already-expanded graph could repeatedly select an unchanged most-proving edge without allocating a node, bypassing the node-bound checks. The partial games and stack samples were preserved under `reports/five_chess/five_star_proof_engine_20260814/formal/invalidated_pre_dfpn_loop_deadline/` and `/tmp/five_chess_benchmark_freestyle_2026-08-14_185119_*.sample.txt`.

Two safeguards now apply:

1. Every DFPN loop iteration polls the absolute decision/session deadline.
2. If recursion changes neither the deterministic node count, root proof/disproof values, nor the selected edge, the fixed point terminates immediately as `unknown` instead of spinning until the deadline.

The regression hook constructs an expanded no-progress graph, verifies prompt unknown unwind and exact board restoration, and is exercised by strict, ASan/UBSan, and TSAN suites. The former opening-10 reproduction, which exceeded nine minutes before the fix, completed both games in about 6.8 seconds after the deadline poll; its maximum candidate decision was 1,636.101ms with zero decisions above 5,000ms. All revision-4 schedules and seeds remain invalidated because the source changed after freeze.
