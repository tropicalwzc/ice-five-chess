## Why

The current five-star opponent guard can prove and replace a move that permits an opponent forcing win, but it runs only after expensive own-proof and quiet-search work. Direct play against UI-bound 5.4.1 found three such late corrections that still ended in losses, so shallow continuous-four losses should be rejected earlier without paying the cost or semantic risk of an early broad VCT search.

## What Changes

- Add an isolated five-star candidate with a bounded opponent micro-VCF sentinel immediately after the frozen four-star/fork handoff selects its provisional move and before own VCT, quiet search, loss-aware refinement, corpus work, and the existing final opponent guard.
- Reject or replace an early candidate only when the opponent VCF returns an independently replayable verified `PROVEN_WIN`; treat disproof and `unknown` conservatively and keep the final full VCF/VCT guard authoritative.
- Compare fixed depth-5, fixed depth-7, and threat-adaptive depth-5/7 sentinel budgets on known VCF distances and the three previously observed late-correction positions, then freeze the cheapest variant that meets correctness and recall gates.
- Reuse verified certificate dependencies for deterministic replacement ordering and reuse compatible proof-session work so the final guard does not needlessly repeat the sentinel search.
- Add telemetry and reports for sentinel eligibility, budget, result, certificate verification, early rejected move, replacement source, cache reuse, latency, later full-guard catches, and preserved downstream budget.
- Run a directional 12–16-game paired test of the selected candidate against exact UI-bound 5.4.1 on 6–8 fresh natural openings with colors exchanged, reporting black first and enforcing the existing 5-second hard deadline.
- Keep the playable five-star UI binding unchanged unless a later, separately authorized promotion decision is made.

## Capabilities

### New Capabilities

- `five-star-early-micro-vcf-sentinel`: Early, certificate-safe detection and avoidance of provisional five-star moves that allow shallow opponent continuous-four wins.
- `five-star-micro-vcf-evaluation`: Targeted parameter selection and a fresh paired quick test against the exact UI-bound 5.4.1 control.

### Modified Capabilities


## Impact

- Five-star composition, profile configuration, proof-session/cache reuse, decision-ledger accounting, result telemetry, and opponent-guard integration in `ice five chess/FiveChessAI.c` and `FiveChessAI.h`.
- Unit and regression coverage in `tools/five_chess_ai_tests.c`, including certificate replay, both rule modes, board restoration, deterministic behavior, and deadline accounting.
- Benchmark runner/report tooling and new evidence under this change directory.
- One-star through four-star behavior, the frozen corpus, UI-bound 5.4.1 control, forbidden-move rules, and persisted user data are unaffected.
