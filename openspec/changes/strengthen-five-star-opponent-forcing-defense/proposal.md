## Why

The playable five-star 5.4.1 path can accept a black move after only the frozen four-star defensive check: if that smaller check returns `unknown`, if the move is classified as `MUST_DEFEND`, or if no corpus position matches, five-star often returns without spending its own budget on the opponent's continuation. This allows shallow verified white VCF sequences to survive candidate selection and can erase black's expected initiative in direct play.

## What Changes

- Add a five-star-only opponent-forcing guard that audits every proposed move lacking an own verified-win certificate, independent of the four-star result, tactical class, and corpus availability.
- Run strict opponent VCF before broader bounded VCT, and apply the guard to ordinary moves, every immediate-defense alternative, and every corpus replacement.
- When the proposed move permits a verified opponent win, reuse its certificate dependencies to search interruption and counter-threat candidates, then rank completed scoped defenses above unknown alternatives and verified losses.
- Reserve deterministic decision resources for the defensive audit so own-attack, quiet, corpus, or earlier inconclusive work cannot starve shallow opponent-forcing detection; retain honest `unknown` semantics and the five-second hard ceiling.
- Add defensive-audit telemetry, certificate replay, symmetry/rule regressions, and a small same-schedule A/B against frozen four-star that leads with five-star-black outcomes and avoidable opponent-VCF incidents.
- Keep one-through-four-star behavior, the frozen four-star opponent, and existing corpus data unchanged. The new profile remains isolated until correctness and the small comparison pass.

## Capabilities

### New Capabilities

- `five-star-opponent-forcing-defense`: Five-star candidate safety auditing, mandatory-defense continuation checking, certificate-directed escape, defensive resource reservation, proof-class selection, and telemetry.
- `five-star-four-star-smoke-benchmark`: Targeted forcing regressions plus a fresh 48-game current-versus-candidate comparison against frozen four-star with black-first reporting and latency/correctness gates.

### Modified Capabilities

None. Related predecessor requirements are change-local and have not been archived into `openspec/specs`.

## Impact

- Affects `ice five chess/FiveChessAI.c`, `ice five chess/FiveChessAI.h`, the playable five-star profile binding after promotion, C tests, benchmark/replay/report tools, and checksummed OpenSpec evidence.
- Reuses the existing rule-aware VCF/VCT solver, independent certificate verifier, decision ledger, candidate generator, and frozen four-star control; adds no training model, network dependency, or opponent-specific runtime table.
- Adds bounded five-star decision work, so budget partitioning and transactional rollback must be verified under freestyle and forbidden-black rules without changing the five-second player-visible limit.
