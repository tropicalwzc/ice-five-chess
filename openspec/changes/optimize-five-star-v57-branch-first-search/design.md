## Context

The v5.7 candidate currently enumerates root threats, starts one proof job per root gain, and lets the pool race those jobs.  Each job owns a private DFPN session, so correctness is safe, but the fixed 97-position replay shows that many jobs are short, overlap heavily, or terminate on the shared deadline before the extra workers produce additional useful depth.

The new candidate must retain the existing C11/pthread pool, decision ledger, incremental legality cache, certificate verifier, and deterministic replay contract.  It must also be usable from the existing 1/4/8-worker fixture runner without changing the production profile.  The user-visible decision remains bounded by the internal 4.5-second reservation and the 5-second hard limit.

## Goals / Non-Goals

**Goals:**

- Keep root obligation selection deterministic and serial, using a cheap preview that recognizes advanced-four/advanced-three structure before expensive proof work.
- Dispatch only long recursive sibling waves, after at least one attacker/defender turn has been entered and only when the node has enough remaining depth and branch width to amortize a pool wave.
- Give each branch worker private board/session/TT/certificate state and merge only completed, verified branch certificates in canonical reply order.
- Raise the opt-in candidate's proof horizon from 12 to 14 plies and measure the effect under the same ledger/deadline contract.
- Extend only eligible long forcing continuations beyond that base horizon: advanced-four attack predictions add two plies, advanced-three predictions add one, and the tactical cap is 16 plies.
- Preserve exact legality, proof status, certificate verification, board restoration, and deterministic result selection.
- Expose enough telemetry to distinguish preview work, branch work, useful verified branches, serial fallback, and deadline exhaustion.

**Non-Goals:**

- Do not modify the default v5.7 profile, the four-star control, or the completed root-scheduler change.
- Do not share a mutable DFPN graph or transposition table between threads.
- Do not dispatch recursively from a pool worker; nested dispatch is explicitly disabled to avoid pool starvation/deadlock.
- Do not treat a shallow preview score, an incomplete branch, or an unverified certificate as a win.
- Do not promise a strength improvement before the fixed-position and 200-game replays demonstrate it.

## Decisions

### 1. Opt-in profile and depth extension

Add a profile derived from `fc_profile_five_star_v57_hybrid_candidate()` with a distinct version, `branchFirstSearchEnabled = true`, root `parallelProofEnabled = false`, `proofMaxDepth = 14`, and a larger aggregate proof budget.  The profile keeps the existing 4.5-second internal reservation, 5-second hard limit, memory limit, and ledger.  Worker-count overrides continue to accept only 1, 4, and 8.

The root dispatcher will route this profile to the serial `fc_prove_forced_win()` path with a branch-first TLS configuration.  This makes the root-vs-recursive distinction explicit instead of relying on a worker count of one as an accidental behavior.

The profile uses depth 14 as its normal proof horizon.  When the preview classifies
an eligible continuation as advanced-four or advanced-three, the recursive child
budget may extend to 16 or 15 plies respectively (`+2` and `+1`).  The extension
is advisory for scheduling and is still bounded by the shared node ledger,
internal deadline, and hard deadline; it never upgrades an incomplete result to
a proof.

### 2. Shallow preview and canonical ordering

At each eligible root proof query, enumerate the same legal threat records used by the proof engine.  For every distinct gain, compute a bounded preview score from threat severity, immediate winning costs, advanced-four/advanced-three rest points, dependency-zone size, and a shallow refutation-width probe.  Tie-break by the existing dependency order and then by `(x,y)`.

The preview changes scheduling order only.  It never changes the legal candidate set, proof result, or certificate acceptance rule.  A preview that reaches the deadline records an incomplete preview and falls back to the canonical existing order.

### 3. Recursive branch wave

`fc_proof_search_attacker()` will carry a per-query `branchWaveDispatched` flag.  The first node satisfying all of these conditions is eligible:

- it is below the root (`parent != -1`);
- remaining proof depth is at least the configured long-recursion threshold;
- there are at least the configured number of refutation replies;
- the current call is not already executing as a pool task; and
- the decision deadline and aggregate node budget are still available.

At that node, the coordinator keeps the attacker gain and current board local, creates one immutable task record per defender reply, and dispatches the reply branches through the existing persistent pool.  Each task copies the board, plays exactly its reply, and searches the continuation with the extended proof depth using a private session.  The task profile has both root and branch-first dispatch disabled, so a worker can never recursively wait for the same pool.

The coordinator waits for the wave, then processes results in reply-array order.  A branch counts as useful only when it completed with `FC_PROOF_PROVEN_WIN` and a verified certificate.  All replies must be proven before the parent attacker gain is proven.  A failed or unknown branch contributes only bounded proof/disproof telemetry and causes the caller to continue with the existing serial root logic; it is never converted into a proof.

### 4. Certificate merge

Each branch result has a certificate rooted at the post-reply board.  On a verified result, the coordinator appends its nodes to the parent context, remapping local parent indices so the branch certificate is attached below the already-created defender node.  Masks and terminal flags are copied unchanged.  If the append would overflow `FC_MAX_PROOF_NODES`, the whole candidate remains unknown and the partial certificate is discarded at the caller checkpoint.

The final parent certificate is still produced and verified by the existing `fc_verify_proof()` implementation.  Deterministic reply order, canonical preview ties, and canonical winning-root selection ensure that worker completion order cannot change the published move or certificate id.

### 5. Budget and diagnostics

Branch tasks share the existing atomic parallel-node counter and decision ledger.  Unused token blocks are flushed by each worker before it returns.  A branch may use the extended per-job ceiling, but the aggregate ledger and absolute deadline remain authoritative.  Branch session memory is reserved through the existing ledger and released on session end.

Add counters for preview branches/advanced-four/advanced-three, branch waves/jobs/completions/useful verified jobs, maximum concurrent branch workers, branch dispatch/fallback, branch depth extension, and branch deadline stops.  Report these beside the existing root/pool counters so a speedup cannot be confused with extra root races.

### Alternatives considered

- **Keep root-parallel scheduling and increase worker count:** rejected because the prior replay already showed more root activity without more useful proof work.
- **Share one DFPN graph across workers:** rejected because graph mutation, incremental board state, and certificate construction would require broad synchronization and would weaken deterministic replay.
- **Dispatch every recursive node:** rejected because short nodes cannot amortize pool coordination and nested pool waits can deadlock.
- **Use preview scores as final move scores:** rejected because shallow tactical evidence is not a proof and can mis-rank a move whose deeper defense is decisive.

## Risks / Trade-offs

- [Risk] Preview probes add work before deep search. → Keep them bounded, count them separately, and fall back to the existing order when the deadline is near.
- [Risk] Independent branch certificates can exceed the fixed certificate arena when merged. → Treat overflow as unknown, preserve the caller checkpoint, and retain the existing verified fallback behavior.
- [Risk] Eight workers can consume the shared ledger before the highest-ranked branch finishes. → Use canonical priority, one recursive wave per query, aggregate token blocks, and compare useful verified branch work rather than raw jobs.
- [Risk] A deeper 14-ply horizon can increase latency or memory pressure. → Keep the candidate opt-in, retain the 4.5/5.0-second gates, enforce the existing memory ledger, and scan 12/14-depth controls in the benchmark.
- [Risk] A pool worker accidentally re-enters the branch scheduler. → Set both dispatch flags off in worker profiles and require the TLS pool-task guard before dispatch.

## Migration Plan

1. Add the opt-in profile and branch diagnostics without changing existing profiles.
2. Add the recursive branch wave and certificate merge, then run unit tests and the fixed fixtures at 1/4/8 workers.
3. Compare the v5.7 baseline, scheduler, and branch-first profile at depth 12/14 where practical.
4. Run the same 200-game four-star and legacy-three-star replays, publish a research report, and leave the new profile as research-only unless the evidence clears the existing strength gates.
5. Roll back by selecting `fc_profile_five_star_v57_hybrid_candidate()`; no data migration or persistent state change is required.

## Open Questions

- Whether 14 plies is the best depth under the 5-second window; the benchmark will decide whether a 16-ply follow-up is worthwhile.
- Whether recursive branch waves produce enough useful verified work on real loss positions to justify changing the production route; this remains an empirical decision.
