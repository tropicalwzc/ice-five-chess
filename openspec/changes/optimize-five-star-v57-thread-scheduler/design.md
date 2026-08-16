## Context

The current v5.7 dispatcher already enumerates roots once and gives each root a
private proof session, but it creates and joins OS threads inside both
`fc_parallel_prove_forced_win` and `fc_parallel_escape_search`.  A decision can
invoke several such batches, so the fixed-position replay measures thread
lifecycle overhead repeatedly.  The DFPN budget check then uses two shared
atomics (the decision ledger and the parallel aggregate counter) on every
search boundary.  These operations are correct but serialize short proof
waves.

The optimization must not turn the worker pool into a shared mutable search
state.  Worker sessions, boards, graphs, tables, result slots, and diagnostics
remain private.  Only the scheduler's task pointer/counters are shared, and a
single batch is active at a time in the current decision call graph.

## Goals / Non-Goals

**Goals:**

- Remove repeated worker creation/destruction from root and escape batches.
- Keep 1-worker behavior as a valid semantic fallback and keep 1/4/8 worker
  benchmark overrides reproducible.
- Reduce atomic token traffic while preserving an exact aggregate cap and
  honest unknown/deadline results.
- Make pool reuse and token-block behavior observable, testable, and reversible
  through the research change.

**Non-Goals:**

- No shared mutable transposition table, lock-free proof graph, or cross-turn
  cache.
- No change to root ordering, overlap semantics, proof certificates, result
  ranking, profile strength parameters, or five-star fork recovery.
- No change to the five-second player-visible deadline or to frozen controls.

## Decisions

### 1. One process-local bounded pool

Create at most eight worker threads lazily on the first parallel batch.  A
condition-variable task gate assigns a batch callback and a worker slot; the
caller waits on a completion condition before stack-owned arrays go out of
scope.  The pool has no public shutdown path because it is process-lifetime
research infrastructure, and it does not own boards or proof memory.

If initialization or dispatch cannot provide a worker, the existing direct
worker path remains available and increments a scheduler fallback counter.

### 2. Shared scheduler, private batch state

Root and escape code use one generic pool dispatch helper.  The callback invokes
the existing root/escape worker with its slot-specific diagnostics object.  The
pool does not reorder jobs: root/escape workers claim canonical job indices,
and the coordinator merges result slots in index order exactly as before.

### 3. Exact block token reservation

Parallel workers keep a small TLS remainder (16 scheduler tokens by default).
When empty, a worker claims the smaller of the block size and the remaining
aggregate/ledger budget with a compare-exchange loop.  A block can never push a
counter above its cap.  The unused remainder is returned at each job boundary
and at worker exit, so a short job cannot starve later siblings.  The existing
per-context query budget and deadline checks run before a block is claimed.

The counters continue to report actual consumed search boundaries; block claims
are separate telemetry.  Single-thread/non-parallel calls retain the existing
per-operation atomic path.

### 4. Concurrency and reentrancy boundary

Only one pool dispatch is allowed at a time.  A nested dispatch is not expected
because pool workers disable parallel proof in their private job profile.  If a
future caller violates this boundary, the helper fails closed to the direct
worker path rather than sharing a batch or deadlocking.

### 5. Verification gates

The existing certificate, board-integrity, deterministic-root, deadline, and
budget tests remain acceptance gates.  New tests check repeated batches reuse
the pool, root and escape batches reach their requested concurrency, block
claims never exceed aggregate/ledger budgets, unused tokens are returned, and
the selected move/certificate is unchanged between repeated 1/4/8 runs.

The fixed replay records wall/CPU time, completed and useful proof work,
workers, pool dispatches/fallbacks, token block claims, unknown/fallback rate,
legality, and peak live memory.  A faster wall time without increased useful
work is reported as scheduler efficiency, not as strength improvement.
