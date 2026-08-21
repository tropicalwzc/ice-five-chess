# Early micro-VCF pre-match freeze

Date: 2026-08-20 (Asia/Shanghai)

This freeze was recorded after the targeted and sanitizer gates passed and
before the six-opening match schedule was generated.

Candidate:

- `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`
- adaptive depth 5/7, 16,000 total sentinel nodes, 80 ms total sentinel time,
  at most two early alternatives
- final depth-9 VCF/conditional depth-10 VCT opponent guard unchanged
- decision internal/hard deadlines: 4,500/5,000 ms

Control:

- exact UI-bound
  `five-star-incremental-dfpn-candidate@5.4.1-transactional-deadline-root-parallel-5s`
- playable binding remains `fc_profile_five_star_proof_engine_candidate()`

SHA-256:

| Input | SHA-256 |
|---|---|
| `ice five chess/FiveChessAI.c` | `5a952f94004f893e699b03f818c81ef54d01f1da64bc4d4c4523663de04c7b29` |
| `ice five chess/FiveChessAI.h` | `c37286ff5daef343dff0ab102d4bdd75c859322461297f6c3bbed5259b23d3b4` |
| `ice five chess/doublethree.m` | `6fec25456b3e84f23a6a4b5ab3b6393fe462d18f921a84fefd0dd3ce3b46b9da` |
| `ice five chess/FiveChessEliteCorpus.inc` | `dbe17be3159a37ffa769e841d15ce7aa68dab7e976789d1a2ad61c8e9dad4e45` |
| `tools/five_chess_ai_tests.c` | `8a250ebd83e21c43167bc036aa4cedb4c9dc31ac156d53c6263969b960338830` |
| `tools/five_chess_benchmark.m` | `f2fdb3df517e64010c8f0cfb642f54614acc889db85a908669f24c5d7062908a` |
| `tools/build_five_chess_benchmark.sh` | `b8cfde2fdd801178f791141213ea6f91184263a09188924a4175fbfbf908c668` |
| `tools/five_star_early_vcf_probe.c` | `59bfed4982ce29083c7cffb07a1d83244ddb00ec665523dfc52c18c29ac0ba74` |
| `tools/generate_early_vcf_quick_schedule.py` | `1a49b16e81873769c0c5ed3dd8cf1e8576f979ca8c91f4bdd3e5e83d9405f4d3` |
| `tools/five_chess_profile_snapshot.c` | `478f2690050f36b96862046144255d44508da71f108f79ce102cb872e8ca1e33` |
| `evidence/pre-match-profiles.json` | `d079d07636bb8c80f13042c8b6ab6f7aca9cadee0976063b6c68e9d779545933` |

Frozen match settings: suite `opponent-guard-smoke`, deterministic-best,
hybrid strategy, freestyle, master seed `0x4755415244534d4b`, opening IDs
66-71, colors exchanged, maximum 120 moves, 12 expected games.
