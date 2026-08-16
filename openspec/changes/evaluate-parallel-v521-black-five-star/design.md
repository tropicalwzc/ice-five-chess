## Context

The completed color-specialized evaluation routes white to the 5.4.1 proof-engine candidate and black to production v5.1. The earlier loss-aware suite exercised v5.2.1 and reported a stronger absolute black score, but it used different openings, deterministic-best play, and decisions above five seconds. Existing A/B evidence also showed v5.1 and v5.2.1 can be outcome-equivalent on some schedules, so a fresh same-schedule control is required before attributing any gain to multithreading.

The current parallel root implementation is gated by `proofEngineCandidate`, a broad flag that also activates proof-session reuse, larger query limits, quiet-root behavior, and other 5.4.1 policies. Simply setting that flag on v5.2.1 would not be an isolated parallelization. Frozen four-star is single-threaded and must remain unchanged.

## Goals / Non-Goals

**Goals:**

- Preserve the latest tested white component, `5.4.1-transactional-deadline-root-parallel-5s`.
- Derive the black component from `5.2.1-certificate-dependency-widening` and add only isolated parallel-root execution, eight-worker resource configuration, telemetry, and a hard decision deadline.
- Guarantee legal best-completed fallback below the 5,000 ms player-visible ceiling.
- Compare one-worker and eight-worker black v5.2.1 on identical fresh schedules, seeds, randomness contract, opponent, and deadlines.
- Measure the final eight-worker hybrid against frozen four-star in Freestyle and forbidden rules using 100 games per rule with colors exchanged.

**Non-Goals:**

- Change frozen four-star, production v5.1, player UI mappings, or lower levels.
- Tune from formal W/D/L outcomes, reuse prior formal openings, or add opponent-specific runtime responses.
- Claim that extra threads improve strength from speed telemetry alone.
- Require stochastic games to reproduce an identical move sequence across independent seeds or wall-clock runs.

## Decisions

### 1. Add an explicit parallel-root capability independent of 5.4.1 policy

Extend the profile with a dedicated parallel-proof flag. The existing 5.4.1 profile and the new v5.2.1-parallel profile enable it; frozen v5.2.1 control, v5.1, four-star, and lower levels do not. Parallel dispatch SHALL partition enumerated proof roots into independent jobs, cap simultaneously active workers at eight, give every worker a private board/session/diagnostics state, and merge only completed certified results.

The new black profile inherits v5.2.1 search class, depths, candidate ordering, corpus policy, loss-aware widening, quiet-root limit, 320 ms query budget, and 1,100 ms emergency policy. It adds eight workers, an aggregate parallel node allowance equal to eight serial root budgets, and a 4,500 ms whole-decision deadline. The one-worker A/B control uses the same profile and deadline but one worker and one serial node allowance.

Alternative rejected: set `proofEngineCandidate=true` on v5.2.1. That would activate unrelated 5.4.1 behavior and make a threading attribution invalid.

### 2. Route by actual stone color and keep the candidate benchmark-only

The new entry point routes `side == -1` directly to frozen 5.4.1 and `side == 1` to parallel v5.2.1. It records component and version on every step. The player-facing five-star continues to call production v5.1; frozen four-star remains its original single-thread profile.

Alternative rejected: replace the prior hybrid entry point in place. That would make old raw evidence impossible to reproduce and blur candidate identity.

### 3. Use shared deadlines and completed-result fallback

Both components receive a 4,500 ms internal decision deadline, leaving finalization reserve under 5,000 ms. Worker batches share the parent absolute deadline; unfinished roots remain unknown. On timeout, the router returns the strongest legal result whose proof/tactical class completed. Nested worker creation is disabled inside worker jobs.

Alternative rejected: retain the historical v5.2.1 unbounded outer decision, whose old report contained a 9,292 ms step and is incompatible with the current product rule.

### 4. Separate thread attribution from final strength

After strict, sanitizer, race, component, and latency tests pass, freeze source/profile/tool hashes and generate fresh rule-separated schedules. For each rule, run:

1. 100 games of the one-worker-control hybrid versus frozen four-star.
2. 100 games of the eight-worker candidate hybrid versus the same four-star on the exact same 50 color-exchanged openings and master seed.

Only the candidate cells are the requested new-five-star strength result. The control cells estimate threading benefit; all opponents remain frozen four-star. One game process runs at a time. Candidate white and black may use at most eight internal workers, while four-star stays single-threaded.

### 5. Predeclare rule-specific interpretation

For each rule, candidate strength requires white score at least 50%, non-negative black same-color advantage over four-star, and overall score at least 50%. Thread benefit requires the eight-worker candidate's black score and overall score to be no lower than the one-worker control, with at least one strict point-estimate improvement across the two measures. Wilson and opening-paired intervals are mandatory. A positive point estimate whose interval overlaps zero is classified as inconclusive, not demonstrated improvement. Rules cannot be pooled to hide a failure.

These gates describe evidence, not UI promotion authority.

## Risks / Trade-offs

- [Parallel merge changes semantics] → Merge only completed results using the same proof-class/certificate precedence and compare one-worker/eight-worker fixtures on deterministic positions.
- [Data races or board corruption] → Give jobs private mutable state, run TSAN plus board-hash restoration fixtures, and forbid nested workers.
- [Eight workers increase CPU but not strength] → Report process CPU, jobs completed, wall latency, RSS, and same-schedule W/D/L separately.
- [Deadline hides parallel work] → Share one absolute deadline, log completed jobs and budget exhaustion, and return only completed legal results.
- [Fresh sample noise] → Use paired color-exchanged openings, 100 games per cell, Wilson/paired intervals, and an inconclusive classification where appropriate.
- [Opening bias is mistaken for black strength] → Compare candidate and control on identical schedules and report both models' same-color black results against four-star.

## Migration Plan

1. Add the isolated profile flag, parallel v5.2.1 profiles, hybrid router, telemetry, and benchmark selectors without changing production routes.
2. Pass strict, golden, component, one/eight-worker equivalence-class, deadline, ASan/UBSan, TSAN, and iOS build checks.
3. Freeze inputs; generate and audit fresh separated schedules.
4. Run control and candidate cells sequentially, replay every game, and publish a checksummed report bundle to the workspace and Downloads.
5. Keep production v5.1 regardless of outcome until a separate explicit UI promotion change.

## Open Questions

- A completed eight-worker search may discover a stronger proof class than the one-worker control and therefore intentionally choose a different move. The report will treat this as the mechanism under test, not as an equivalence failure, provided the recorded move is legal and its completed metadata validates.
