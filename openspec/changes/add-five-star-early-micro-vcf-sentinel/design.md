## Context

The completed `strengthen-five-star-opponent-forcing-defense` change produced the isolated `5.8.0-vcf-first-opponent-guard-4w` candidate while leaving the playable UI on `5.4.1-transactional-deadline-root-parallel-5s`. Its final opponent guard reserves 96,000 nodes and 1,400 ms, audits with depth-9 VCF followed by structurally eligible depth-10 VCT, and can widen to eight alternatives.

A direct 12-game candidate-versus-5.4.1 follow-up was 6/0/6 overall and entirely opening/color driven. More importantly, the candidate corrected three provisional moves only at the final guard; all three games still ended as candidate losses. Across that run, selected VCF audits cost 0.280/81.824 ms p50/p95 and 1,733.856 ms total, while eligible VCT audits cost 15.797/717.073 ms p50/p95 and 16,423.553 ms total. This makes strict VCF the appropriate low-cost early detector and broad VCT unsuitable as an unconditional pre-search stage.

The current composition pipeline obtains a frozen four-star result, rebuilds and optionally reorders the handoff candidate pool through the fork layer, then performs own-proof, quiet, loss-aware, and final guard work. The sentinel belongs after the fork layer has committed a legal provisional coordinate but before `ownCandidateEligible` and its downstream work. Existing proof verification, escape generation, proof sessions, transactional ledger, deadline, and telemetry patterns are reusable.

## Goals / Non-Goals

**Goals:**

- Prove and avoid cheap shallow opponent VCF losses before expensive own-search stages consume time on the losing candidate.
- Preserve honest three-valued proof semantics: only a replayable opponent win certificate can reject a move.
- Select the cheapest useful depth/budget policy from fixed depth 5, fixed depth 7, and an adaptive depth-5/7 variant.
- Reuse early proof work and certificate dependencies without weakening the final full opponent guard.
- Measure whether earlier intervention preserves budget or changes decisions in targeted fixtures and fresh paired play against exact UI-bound 5.4.1.

**Non-Goals:**

- Replace the final depth-9 VCF/depth-10 VCT guard, prove global safety, or treat a shallow disproof as safe.
- Run broad VCT before own search or encode losing opening IDs and coordinates into runtime policy.
- Change one-star through four-star, the frozen corpus, rules, the 4.5-second internal deadline, or the 5-second hard ceiling.
- Promote the candidate to the playable UI based on this directional test.
- Claim statistically significant strength from 12–16 games.

## Decisions

### 1. Derive a new isolated candidate from the completed opponent-guard candidate

The new profile inherits `5.8.0-vcf-first-opponent-guard-4w` and adds explicit sentinel enablement and budget fields. The frozen 5.8.0 candidate and exact 5.4.1 control remain selectable. This isolates the value of earlier detection and makes rollback a profile selection rather than a source rollback.

Alternative considered: modify 5.8.0 in place. Rejected because it would destroy the baseline needed to attribute targeted and runtime differences.

### 2. Place the sentinel after legal handoff/fork selection and before own proof

The sentinel receives the legal provisional coordinate after the four-star/fork layer has built the full candidate pool. Immediate legal wins bypass it. For every other eligible provisional move, it creates the after-move board and queries an opponent strict VCF before own VCF/VCT, quiet search, loss-aware refinement, corpus replacement, and the final opponent guard.

This placement is early enough to preserve downstream budget but late enough to operate on the actual provisional move and existing deterministic candidate pool. Mandatory-defense candidates are eligible because a one-ply block can still permit a continuous-four continuation.

Alternative considered: place the sentinel before fork recovery. Rejected because it would audit a handoff coordinate that the fork layer may already replace and would lack the final deterministic replacement pool.

### 3. Reject only independently replayable opponent `PROVEN_WIN`

The sentinel uses the existing proof verifier on the exact after-move board. A candidate is marked early verified-loss only when the VCF status is `PROVEN_WIN`, its certificate is complete, and independent replay succeeds under the active rule. Certificate failure, enumeration overflow, deadline, illegal placement, board-restoration failure, and incomplete work become `unknown`.

`NO_FORCED_WIN_IN_SCOPE` is only a depth/budget-scoped disproof and `unknown` is unresolved; neither permits a “safe” label. Both allow normal downstream processing, and the final guard remains authoritative for deeper VCF and VCT.

Alternative considered: reject using static pattern score or an unverified search status. Rejected because neither proves that every forced reply is covered and either could discard a valid move.

### 4. Evaluate three bounded policies and freeze by correctness first, cost second

Implementation exposes diagnostic overrides for:

- A: depth 5, 4,000–8,000 nodes, 20–40 ms.
- B: depth 7, 8,000–16,000 nodes, 40–80 ms.
- C: depth 5 by default, escalating to depth 7 only when the after-move position has an opponent four/open-four or VCF dependency signal.

All variants replay known VCF distances 3/5/7/9 and the three reconstructed late-correction positions. The selected policy must have zero false rejections, replay every reported certificate, catch every available in-scope fixture at or below its effective depth, preserve board state, and remain deterministic. Among policies satisfying those gates, select the lowest p95 sentinel time; if p95 is effectively tied, select the lower node consumption. Distance-9 misses are explicitly reported and remain the responsibility of the final guard unless a shallower certificate is found.

The adaptive signal is eligibility for a larger VCF query, not evidence of loss. If the adaptive policy loses in-scope recall because its signal is incomplete, fixed depth 7 wins even if it is somewhat slower.

Alternative considered: pick adaptive policy in advance. Rejected because a structural gate can be cheap yet silently miss the exact shallow loss class being targeted.

### 5. Use certificate-directed replacement without claiming unknown safety

When the provisional move is a verified loss, the existing escape builder uses certificate gain/cost/rest/dependency points first, followed by generated mandatory/tactical and ordinary handoff candidates in deterministic order. Each early alternative can receive the same bounded sentinel audit until one is not itself a verified loss or the small alternative cap/budget is exhausted.

An unresolved alternative may replace a verified loss so downstream search can investigate it, but telemetry labels it `unknown`, not proven defense. If every audited alternative is a verified loss, longest verified survival is provisional and the final guard remains responsible for the final classification.

Alternative considered: spend the full final-guard portfolio early. Rejected because the purpose is to stop wasting own-search budget, not duplicate the expensive acceptance boundary.

### 6. Share compatible proof work and use one transactional budget

Sentinel queries run inside the existing decision proof session and aggregate decision ledger. Cache keys include canonical board, side, rule, VCF class, depth/scope, and proof-engine version. A verified shallower proof can be reused directly by the final guard; a scoped disproof can seed a deeper query but cannot satisfy it. Cache reuse is reported separately from fresh nodes and time.

The sentinel receives a small reserved node/time slice inside the existing 4.5-second internal deadline and does not extend the 5-second hard ceiling. On deadline or transactional failure, the last completed legal state is restored and downstream handling remains conservative.

Alternative considered: use an independent timer and cache. Rejected because it could exceed the player-visible limit, double count resources, or pool incompatible scopes.

### 7. Keep the final guard and corpus acceptance boundary unchanged in authority

After early replacement, the normal own-proof and quiet/loss-aware stages run on the new provisional coordinate. The final full guard still checks the final non-winning candidate and every corpus replacement using its existing depth-9 VCF and conditional depth-10 VCT scope. If final-guard evidence conflicts with the sentinel, the verified deeper/final evidence wins and the mismatch is recorded.

Alternative considered: skip the final guard after a sentinel disproof. Rejected because shallow VCF disproof says nothing about deeper VCF or VCT.

### 8. Validate targeted behavior before fresh direct play

Targeted evaluation runs before any head-to-head test and records loss recall by distance, false rejection count, certificate replay, sentinel p50/p95/max, nodes, cache reuse, own-search budget preserved, final move changes, later guard catches, and deadline behavior. The three direct-play late-correction positions are reconstructed from raw moves and provisional coordinates and used only as diagnostics.

After parameters and hashes freeze, generate 6–8 fresh natural freestyle opening identities excluded from openings 60–65 and all diagnostic/result-selected positions. Run deterministic-best play with colors exchanged against the exact UI-bound 5.4.1 profile for 12–16 games. Report candidate black first, then white and overall, plus early catches, later catches, decision latency, anomalies, and decisions over 5,000 ms.

Alternative considered: rerun openings 60–65. Rejected because their prior paired winners were completely color/opening driven and they are no longer fresh evidence.

## Risks / Trade-offs

- [Sentinel overhead applies to positions that are not losing] → Keep it strict VCF-only, bound it to a small slice, compare adaptive and fixed variants, and select by measured p95 after correctness gates.
- [Adaptive signal misses a shallow forcing line] → Measure recall against fixed depth 7 and reject adaptive selection on any in-scope recall loss.
- [Unknown alternative is mistaken for safety] → Persist exact status/scope and reserve “verified defense” terminology for completed evidence only.
- [Early rejection reduces time for a real own win] → Skip immediate wins, keep the sentinel bounded, and measure downstream proof activity and preserved/spent budget.
- [Cache reuses incompatible proof scope or rules] → Include rule, class, depth, board, side, and engine version in keys and independently replay every reused winning certificate.
- [Small paired test gives a noisy W/D/L result] → Label it directional, publish raw games, and prioritize verified-loss incidence and deadline health.
- [A late corpus replacement reintroduces the same risk] → Retain the existing final guard as the authoritative acceptance boundary.

## Migration Plan

1. Freeze hashes for 5.8.0, UI-bound 5.4.1, the proof engine, rules, corpus, diagnostic inputs, and benchmark tool.
2. Add the isolated sentinel profile, configuration, ledger fields, proof reuse, telemetry, and benchmark overrides without changing UI binding.
3. Add unit and targeted fixtures, including reconstructed late-correction boards, both rule modes, symmetries, rollback, certificate replay, and final-guard interaction.
4. Run variants A/B/C, publish the comparison, and freeze the cheapest policy that passes every correctness/recall gate.
5. Freeze 6–8 fresh openings and run/replay the 12–16-game paired test against exact 5.4.1.
6. Publish the directional report and retain the current UI binding. Rollback is disabling/selecting away from the isolated sentinel profile.

## Open Questions

- The exact effective node/time values and early alternative cap remain diagnostic selections within the declared ranges.
- If none of A/B/C catches a useful share of the three reconstructed late-correction positions within the p95 target, the report will retain the final guard and recommend no runtime sentinel rather than silently increasing the early budget.
- A formal promotion decision and larger dual-rule strength evaluation remain separate changes.
