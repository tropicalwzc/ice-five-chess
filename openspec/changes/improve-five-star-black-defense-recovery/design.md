## Context

The completed 5.8.2 double-three change uses a soft structural-defense weight of 40/100. Its remaining candidate-black losses are not dominated by isolated double-three weighting. In five of six paired-cell losses, the final position already contains two immediate white winning points; the preceding black move allowed a quiet white move to create that two-step fork. In four of those predecessor positions, the opponent VCF query was unknown and the current guard did not escalate to VCT.

One loss is a direct recovery-ordering regression. In the four-star opening-9 game, the original handoff point `(9,10)` removes the current immediate white win, while the double-three structural replacement `(12,7)` leaves it. The current guard has a bounded alternative budget and receives a generated portfolio that can omit or delay the pre-structural handoff candidate.

The affected path is the isolated five-star profile: early VCF and own-proof stages feed double-three preemption, opponent guard recovery, and then optional corpus selection. Existing profiles and the playable binding are controls and must remain unchanged.

## Goals / Non-Goals

**Goals:**

- Preserve the original baseline, advisory/default, and four-star handoff candidates when structural preemption changes black's provisional move.
- Prioritize legal moves that remove current opponent immediate wins before consuming the bounded guard alternative budget.
- Detect the narrow two-step pattern where a white reply creates at least two immediate winning points, including when the opponent VCF is unknown.
- Use a bounded VCT follow-up for relevant black defenses while retaining honest `unknown` semantics for incomplete searches.
- Keep proof, loss, recovery, and structural telemetry bound to the final selected coordinate.
- Add deterministic regression fixtures and repeat the existing paired small-match protocol.

**Non-Goals:**

- Do not change the 5.8.1, frozen four-star, lower difficulty, or playable five-star profiles.
- Do not promote the research candidate to the UI binding in this change.
- Do not replace the existing VCF/VCT proof engine with an exhaustive game solver.
- Do not reinterpret the legacy double-three geometry or treat a local fork probe as a global proof of safety.
- Do not add opening-specific runtime moves or external data dependencies.

## Decisions

### 1. Snapshot and merge the pre-structural recovery portfolio

Capture the baseline coordinate and its relevant proof/guard context before black double-three preemption. When preemption changes the coordinate, pass the baseline, legal advisory/default point, four-star handoff candidates, and structural candidates into one deduplicated guard portfolio. Reserve explicit priority for the original baseline/default entries instead of relying only on a larger generic alternative limit.

This is preferred over simply increasing `opponentGuardMaxAlternatives`: it fixes candidate omission deterministically and limits latency growth. The structural layer still selects the narrow double-three result; the guard remains authoritative for opponent safety.

### 2. Run an immediate-win blocker pre-pass

For each legal recovery candidate, simulate the black move and count current legal white moves that complete five. Candidates with zero such replies rank above candidates that leave one or more immediate replies, unless black has an immediate win or a verified own VCF. If every candidate leaves an immediate reply, preserve the verified-loss/unknown result and choose through the existing survival ordering.

This local pre-pass is cheaper and more actionable than spending all guard slots on broad proofs. It is a scoped tactical classification, not a claim that the candidate is globally safe.

### 3. Add a bounded two-step fork probe for VCF-unknown black positions

When the black-side opponent audit is VCF-unknown, enumerate a bounded deterministic set of legal white replies after each prioritized recovery candidate. For each reply, count legal white immediate winning points in the resulting position. Two or more points classify the candidate as a local opponent fork risk; zero or one point is a scoped no-fork observation only when the relevant enumeration completes.

The probe is used to rank recovery candidates and to expose telemetry. It does not turn a no-fork result into a VCF/VCT disproof, and an interrupted candidate remains unknown.

### 4. Permit a narrow VCT escalation after VCF unknown

For black positions selected for recovery, reserve a small profile-configured VCT budget even when VCF is unknown. A verified opponent VCT remains a losing class, a replayed scoped disproof outranks an unknown candidate, and any timeout or incomplete certificate remains unknown. The escalation is restricted to the isolated 5.8.2 profile and must honor the existing decision ledger and hard deadline.

This is preferred over treating VCF unknown as safe or over running full VCT for every move. The guard can combine the immediate-block and fork classifications before spending the deeper budget.

### 5. Publish result metadata transactionally

Whenever guard, corpus, or structural recovery changes the selected coordinate, update proof status, proof coordinate, opponent-after-selected fields, loss reason, recovery source, and telemetry as one selected-candidate snapshot. Candidate-level diagnostics may retain the provisional audit, but public fields must describe the final move.

### 6. Freeze evidence before and after the change

Add fixed positions extracted from the remaining black losses: the opening-9 immediate-block replacement and representative quiet-fork predecessors. Verify move legality, board restoration, deterministic repeated evaluation, local fork classification, and proof status. Then run the same 12-opening, two-color, deterministic-best, no-forbidden, 120-move paired cells against frozen four-star and exact 5.8.1.

## Risks / Trade-offs

- [Risk] Additional immediate/fork probes increase black decision latency. → Restrict them to recovery-relevant black positions, use deterministic bounded portfolios, reserve ledger tokens, and report p50/p95/max latency.
- [Risk] A bounded no-fork scan can be mistaken for a global safety proof. → Expose a separate scoped status and keep VCF/VCT certificates authoritative.
- [Risk] Preserving more baseline candidates can retain a tactically inferior move. → Rank current immediate blockers and verified/scoped guard results before heuristic survival ties, and regression-test the opening-9 position.
- [Risk] VCT escalation may exhaust the decision budget and change ordinary play. → Isolate new profile fields/version, enforce the hard deadline, and fail closed to `unknown` without changing controls.
- [Risk] Recovery replacement can leave stale telemetry or proof fields. → Use a single final-candidate publish helper and assert coordinate/status consistency in the C regression suite and replay validator.

## Migration Plan

1. Add the new profile fields and recovery telemetry with defaults that preserve the existing 5.8.2 soft-40 behavior when the new recovery path is disabled.
2. Implement the baseline portfolio, immediate-block pre-pass, fork probe, and VCF-unknown VCT escalation behind the new isolated profile version.
3. Run unit, restoration, sanitizer, certificate-replay, and fixed-loss regression tests.
4. Run and publish both paired small-match cells. Keep the candidate out of the playable binding unless all evaluation gates pass.
5. Roll back by selecting the prior `5.8.2-black-double-three-soft-40-16g-32c-80ms` profile or disabling the new recovery fields; controls remain available for comparison.

## Open Questions

- What default fork-probe reply and immediate-win budgets provide useful recovery without exceeding the existing five-second ceiling?
- Should a scoped no-fork result be recorded as a new telemetry enum or represented as a fork-specific subfield while the public proof status remains unknown?
- If all prioritized candidates are verified losses, should survival distance or immediate-threat count be the first tie-breaker for the new profile?
