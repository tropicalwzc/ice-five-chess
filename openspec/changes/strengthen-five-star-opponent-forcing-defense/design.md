## Context

The playable five-star entry currently selects `5.4.1-transactional-deadline-root-parallel-5s`. Its composition layer first obtains the frozen four-star move, spends candidate proof work primarily on an eligible own VCF/VCT, and refines defense only when the inherited result already carries a verified opponent-win classification. A mandatory immediate defense bypasses candidate VCF/VCT work, a position with no corpus match returns before the later opponent-after-baseline query, and `fc_move_is_safe` excludes only immediate next-move wins. Consequently, a four-star `unknown` can become the five-star final move even when five-star could prove a shallow opponent VCF after that move.

The existing rule-aware proof engine, certificates, loss-aware escape generator, decision ledger, four-star control, and benchmark runner are reusable. The change must preserve honest three-valued proof results, both rule modes, deterministic evaluation behavior, the internal 4.5-second reservation, and the 5-second player-visible hard ceiling. Direct-play failures and derived positions are diagnostics, not opening advice or fresh match evidence.

## Goals / Non-Goals

**Goals:**

- Ensure that every five-star move without an own verified-win certificate reaches one common opponent-forcing acceptance boundary before it can be committed.
- Catch shallow opponent continuous-four wins first, including continuations after a mandatory one-ply block, without requiring four-star or corpus code to discover the danger.
- Reuse verified opponent certificates to find and rank defensive alternatives under one bounded decision contract.
- Preserve lower difficulty behavior and measure the isolated candidate against the exact UI-bound 5.4.1 control and frozen four-star.

**Non-Goals:**

- Prove full-board Gomoku outcomes or describe `no-forced-win-in-scope` as globally safe.
- Encode user-game coordinates, losing opening IDs, or four-star responses into runtime policy.
- Change one-star through four-star search, the frozen corpus, forbidden-move semantics, or the five-second player-visible ceiling.
- Treat the 48-game smoke comparison as a statistically conclusive promotion or theoretical first-player-advantage claim.
- Increase fixed search depth as the primary fix; an uncalled defensive stage gains nothing from a deeper limit.

## Decisions

### 1. Introduce an isolated five-star candidate derived from UI-bound 5.4.1

The new profile will inherit the current playable 5.4.1 proof engine and add explicit opponent-guard enablement and resource fields. The current profile, frozen four-star, research v5.7 profiles, and lower levels remain selectable and byte-for-byte comparable. The UI binding changes only after the candidate passes the declared correctness and smoke gates.

Alternative rejected: patch the frozen four-star defensive budget. That would change the comparison opponent and every four-star user while failing to guarantee that five-star spends its own resources on the result.

### 2. Put one defensive acceptance boundary before every non-winning commit

The five-star composition layer will route its provisional baseline, mandatory-defense selection, quiet result, and corpus replacement through one guard before final acceptance. A move can skip the guard only when it immediately wins legally or carries an independently verified own forced-win certificate whose verifier accounts for all relevant counterplay. The guard runs before the existing tactical/no-corpus early returns, not only inside corpus comparison.

The guard first places the proposed move on a private or reversible position, then queries the opponent from the resulting board. Illegal placement, board restoration failure, incomplete enumeration, deadline, overflow, and certificate failure remain `unknown`; none may be called safe.

Alternative rejected: add another audit only to the no-corpus return. That would leave mandatory defenses and corpus replacements with different safety semantics and invite future early-return gaps.

### 3. Use strict VCF first and bounded VCT only when relevant

Every guarded move receives a strict opponent VCF query at the candidate profile's frozen defensive scope. A verified VCF rejects the move immediately. A completed VCF disproof is recorded only as VCF-scoped; when the resulting position contains a qualifying opponent VCT root/dependency signal and reserved resources remain, the guard continues with bounded VCT. A verified VCT also rejects the move, while `unknown` stays unknown.

The VCF-first order targets the observed continuous-four reversals, usually completes more cheaply than broad VCT, and yields a small certificate from which interruption points can be derived. Structural VCT eligibility prevents unconditional broad proof work on quiet boards.

Alternative rejected: run only broad VCT. It can spend the reserved slice on three-based branching before completing a shallow continuous-four obligation.

### 4. Audit all immediate-defense alternatives rather than the chosen block alone

When an immediate opponent win exists, candidate generation retains every legal move that removes the current win. The guard audits the provisional block; if it is verified losing, or if another generated block can obtain a better completed proof class within the reserved slice, the defense portfolio evaluates the remaining blocks in deterministic tactical/coordinate order. Blocking the current five is necessary but is no longer treated as sufficient evidence against the next white forcing sequence.

An immediate own win remains first. A mandatory-defense label remains visible in telemetry even when a different block is selected after continuation proof.

Alternative rejected: replace the current block heuristic with a larger static four-pattern score. Static scores cannot establish that all legal white replies were handled.

### 5. Reuse certificates for progressive defensive widening and completed-class ranking

After a proposed move permits a verified opponent VCF/VCT, existing gain/cost/rest/certificate-line dependencies seed escape candidates before tactical, nearby ordinary, and remaining legal moves. Alternatives use the same active rule, search class, completed scope, absolute deadline, and aggregate ledger. A completed opponent disproof ranks above an immediately safe `unknown`, which ranks above a verified loss; verified losses are ordered by longest survival. An own verified win ranks first.

The selected result records its exact scope. Unknown alternatives may replace a verified loss but are labeled `unknown`, never proven defense. A corpus move must pass this same guard and cannot replace a completed defense with narrower or incomplete evidence.

Alternative rejected: inspect a fixed top-six set. Prior loss diagnostics already demonstrated that certificate-specific blocks can lie outside heuristic width.

### 6. Reserve a non-starvable defensive slice inside the existing decision ledger

At decision start the candidate reserves a frozen node quota and a bounded time opportunity for the opponent guard. Earlier own-attack, quiet, and corpus stages may not consume its node tokens. The reservation is part of the existing aggregate decision node/memory/query ledger and absolute deadline; it does not extend the 4.5-second internal or 5-second hard limits. Unused defensive tokens may be released only after the guard completes or becomes ineligible.

Exact VCF/VCT depth, node split, maximum alternatives, and time reserve will be selected on targeted and non-final natural diagnostics before smoke schedules are generated. Each guard attempt is transactional: if a stage reaches the deadline, the last completed legal result is restored and the unfinished classification is `unknown`.

Alternative rejected: simply run the guard last with whatever budget remains. That reproduces the current failure whenever earlier proof or corpus work consumes the decision window.

### 7. Make defensive coverage and outcomes directly observable

Analysis and benchmark telemetry will record guard eligibility and skip reason, proposed and selected coordinates, VCF/VCT status and distance, nodes/time, certificate verification, candidates audited by stage, completed disproofs, unknowns, verified losses, reserved/consumed resources, transactional rollback, and whether a verified opponent forcing line was avoided. The reporter will identify the first candidate move in a lost game after which the opponent has an independently replayed VCF/VCT certificate.

The independent verifier remains authoritative. Runtime telemetry is evidence about the decision path, not proof by itself.

### 8. Validate with targeted regressions before a fresh 48-game smoke A/B

Diagnostics will include at least one separating position where UI-bound 5.4.1 selects a black move permitting a verified shallow white VCF while a legal alternative completes a comparable disproof. The suite also covers VCF distances 3/5/7/9 where constructible, mandatory-block continuations, no-corpus positions, four-star `unknown`, corpus conflicts, all eight symmetries, and both rules. All accepted proof and disproof artifacts replay independently and restore the board.

After code/profile/budget/runner hashes freeze, generate 12 fresh natural freestyle opening identities excluded from diagnostics, corpus-shaped starts, and prior result-selected schedules. Run both the current 5.4.1 control and the candidate against the same frozen four-star from both colors: 12 openings × 2 colors × 2 five-star profiles = 48 games. Reports lead with five-star black, compare avoidable verified opponent-forcing incidents as well as W/D/L, and label the sample directional only. Forbidden play is covered by regressions; a second 48-game forbidden smoke run is required only if the reported direct-play failure is confirmed under forbidden mode or before a later formal promotion claim.

## Risks / Trade-offs

- [Guard work increases ordinary-move latency] → Use VCF-first completion, structural VCT eligibility, a frozen candidate limit, incremental state, and one absolute deadline.
- [Reserved defense tokens reduce discovery of own long wins] → Skip only for verified own wins, tune the split on non-final diagnostics, and report own-proof activity and move differences in the A/B.
- [VCF disproof is mistaken for global safety] → Persist the exact search class/scope and retain `unknown` for unresolved VCT or broader play.
- [Mandatory-defense widening consumes the full deadline] → Audit the provisional block first, prioritize certificate/tactical alternatives, cap the portfolio, and transactionally retain the best completed legal class.
- [Parallel timing changes decisions] → Use deterministic candidate order, aggregate ledger limits, canonical completed-result merging, repeat fixed regressions, and never rank unfinished worker progress as a completed class.
- [Small match noise is overinterpreted] → Freeze paired inputs, report raw W/D/L and intervals, emphasize opponent-VCF incidence, and prohibit a formal strength claim from 48 games.
- [Forbidden-black legality makes guard cost explode] → Keep rule in every key, cross-check incremental legality with the established oracle, and allow conservative unknown/fallback without pooling rule evidence.

## Migration Plan

1. Freeze and checksum the UI-bound 5.4.1 control, frozen four-star, corpus, rule helpers, relevant tools, and diagnostic sources.
2. Add the isolated profile, telemetry, guard acceptance boundary, and reserved ledger slice without changing the playable binding.
3. Implement VCF-first/VCT-conditional audits, mandatory-defense portfolio checks, certificate-directed widening, proof-class composition, and corpus enforcement.
4. Pass unit, symmetry, both-rule, certificate/disproof replay, deterministic fixed-position, sanitizer, board-integrity, and simulator/hard-deadline gates.
5. Freeze structural parameters on non-final diagnostics, generate the fresh 12-opening schedule, and run/replay/report the 48-game A/B.
6. Bind only the five-star UI entry to the candidate if every correctness gate passes, targeted avoidable VCF regressions are removed, no new independently verified avoidable opponent-forcing incident appears, black direction is non-negative versus the same-schedule control, and white/overall show no obvious smoke regression. Otherwise retain 5.4.1 and publish the failed evidence.
7. Rollback consists of restoring the existing five-star profile binding; no persistent user data or corpus migration is required.

## Open Questions

- The exact reserved node/time split, VCF/VCT depths, and maximum audited alternatives remain diagnostic selections and must be frozen before smoke schedule generation.
- If direct-play move records become available, they will be added as diagnostic fixtures only; implementation does not depend on receiving them.
- A statistically meaningful larger dual-rule evaluation remains a separate follow-up if the smoke run is promising.
