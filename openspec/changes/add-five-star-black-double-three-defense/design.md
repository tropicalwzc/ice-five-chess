## Context

The current playable five-star path is the 5.8.1 early-micro-VCF profile. It already has a VCF-first opponent guard and a broader VCT search, but the VCT structural signal is consulted only after the opponent VCF scope completes with a disproof. A VCF `unknown` therefore prevents the VCT stage from running. The existing C threat representation also uses continuation-point counts for `FC_THREAT_FOUR_THREE`; that is useful for proof ordering but is not an exact count of open-three directions.

The legacy Objective-C advisor contains the required application-level meaning in `doublethreetest`: for each of four directions it looks for a four-cell window containing three stones of the attacking side, with both exterior cells empty. `harsh_doublethree_hide_attack` checks for a recursive forcing attack before entering its double-three defense recursion. The new candidate should preserve that meaning in the rule-aware C engine, while leaving 5.8.1, frozen four-star, and lower difficulty profiles available as controls.

## Goals / Non-Goals

**Goals:**

- Detect a legal white reply that creates open threes in at least two independent directions using an explicit direction-based predicate.
- Give black a bounded preemption stage that runs even when the opponent VCF query is unresolved, unless black has an immediate win or an independently verified own VCF.
- Prioritize candidates that eliminate all currently detected white double-three gains, then use the existing VCF/VCT proof-class ranking within the safe candidate set.
- Preserve fail-closed `unknown` semantics, board restoration, rule-aware legality, deterministic candidate order, and the existing five-second decision ceiling.
- Add reproducible 5.8.2 versus frozen four-star and 5.8.2 versus 5.8.1 small-match evidence with the same paired openings and color exchange.

**Non-Goals:**

- Re-define the complete Renju rule book or replace the existing legality oracle.
- Treat a local double-three scan as a global proof that the position is won or safe.
- Change the 5.8.1 profile, frozen four-star behavior, one-to-three-star behavior, or corpus data.
- Increase the general VCF/VCT depth as the primary solution.
- Promote 5.8.2 to the playable iPhone/iPad five-star binding based only on the small matches.

## Decisions

### 1. Add a dedicated direction-based double-three detector

Implement a pure-C helper that evaluates a temporary move and counts affected directions, not continuation points. For each of the four board directions, inspect the same bounded five-cell geometry as the legacy advisor: the four interior cells contain exactly three stones of the attacker and one empty cell, and the two exterior cells are inside the board and empty. Count each direction at most once. A legal white gain is a double-three gain when the post-gain direction count is at least two and the move is not already an immediate five.

The detector will enumerate white gains from the current black-to-move board and retain a bounded coordinate/mask representation. Every legality check uses the active `fc_is_legal_move` rule mode. A scan deadline, storage overflow, or incomplete frontier enumeration produces an incomplete result rather than a safe result.

Alternative rejected: infer double-three from `FC_THREAT_FOUR_THREE` or `restCount`. Those fields count continuation points and may represent one open three with two continuations; they do not encode the number of independent open-three directions required by this change.

### 2. Run the black preemption stage before the opponent-guard skip paths

The 5.8.2 profile adds a black-only structural stage after the existing own-attack work has established the provisional move and before the current opponent guard can skip because of an own verified result. The stage is bypassed for a legal immediate win or an independently verified own VCF certificate. An own VCT result without a VCF certificate does not automatically bypass the structural check unless its independently replayed certificate proves termination before any relevant white reply.

For each provisional or alternative black move, simulate the move and re-check the retained white gains. A gain is removed when black occupies the gain itself or otherwise makes the white gain illegal or removes one of its required open-three directions. A candidate with no residual double-three gains is structurally safe for this narrow obligation. This local result does not replace the existing opponent VCF/VCT audit; it determines which candidates must be examined first.

Alternative rejected: run the detector only after VCF disproof. That reproduces the observed failure whenever VCF is `unknown` at the guard budget.

### 3. Inject targeted preemption candidates without widening every normal search

When the scan finds one or more white double-three gains, add legal black gain-blocking points to a deterministic tactical pool ahead of ordinary heuristic candidates. Also retain existing generated candidates and certificate/dependency candidates. The pool is deduplicated by coordinate and filtered through black legality, including forbidden-black rules.

The candidate comparator first prefers immediate own wins and verified own VCFs. If no such result exists and the scan is complete, candidates with zero residual double-three gains outrank candidates with one or more residual gains. Existing opponent proof class, comparable scope, distance, and current heuristic ordering break ties within the same structural class. If no candidate eliminates every gain, the least-residual candidate is selected only with an unresolved structural status; it is never labeled a proven defense.

Alternative rejected: hard-code the center/gain coordinate from a diagnostic board. The required defense may be a shared line blocker, an edge point, or a rule-dependent alternative, and diagnostic coordinates must not enter runtime policy.

### 4. Keep the structural stage bounded by the existing decision ledger

The detector and its candidate checks run under the 5.8.1 absolute decision deadline and the reserved opponent-guard opportunity. They do not grant a second five-second window. Profile fields will cap the number of retained gains, targeted alternatives, and optional scan time; exact limits are selected on non-final fixtures before match seeds are frozen. The stage is transactional: every temporary move is undone, and any restoration mismatch downgrades the structural result and prevents a structural override.

Telemetry will distinguish scan complete/incomplete, white double-three gain count, provisional residual count, selected residual count, candidates examined, candidates eliminated, own-VCF bypasses, structural overrides, and rollbacks. Existing VCF/VCT statuses and certificates remain authoritative for broader forcing claims.

### 5. Isolate 5.8.2 and make both comparison cells paired

Create a named 5.8.2 profile derived from the exact 5.8.1 profile. Add a benchmark profile selector for the candidate and a distinct opponent selector for 5.8.1; the current `five-star-control` naming must not be reused if it resolves to 5.4.1. Use the frozen four-star control for the first cell and the exact 5.8.1 profile for the second cell.

Each cell uses 12 frozen natural freestyle opening identities, two color assignments per identity, deterministic-best play, identical seeds, no-forbidden mode, and a 120-move cap: 24 games per cell, 48 games total. Reports lead with the candidate's black result, then white, overall paired result, double-three incidence, independently replayed forcing incidents, latency, deadline anomalies, and board/replay integrity. The result is directional evidence only.

Alternative rejected: compare 5.8.2 and 5.8.1 on unrelated openings or only as self-play. That cannot separate the defensive change from opening/color effects.

### 6. Add focused correctness fixtures before match generation

The C suite will cover a crossing double-three gain, all eight symmetries, a single-open-three negative control, edge and blocked-line cases, both rule modes, multiple white gains, a verified own VCF bypass, an unresolved VCF with a complete structural scan, scan overflow/deadline behavior, deterministic repeated evaluation, and board restoration. Match schedules and diagnostic fixtures are frozen separately so no selected move or historical loss becomes runtime data.

## Risks / Trade-offs

- [The direction scan adds latency on ordinary black moves] → Run it only for the isolated black candidate, restrict it to the local frontier and bounded gain/alternative counts, and measure p50/p95/max under the existing deadline.
- [A local scan falsely calls a move safe] → Require complete enumeration, use the shared legality helper, retain structural status separately from proof status, and fail closed on overflow, deadline, or restoration mismatch.
- [The preemption layer suppresses a valid black attack] → Exempt immediate wins and independently verified own VCFs, retain existing proof ranking among structurally safe candidates, and record every bypass/override for replay.
- [No single black move can eliminate all double-three gains] → Select the least-residual candidate only as unresolved, continue existing VCF/VCT auditing, and report the position as structurally unresolved rather than claiming safety.
- [The 24-game cells are dominated by color or opening bias] → Exchange colors, reuse paired seeds/openings, report black and white separately, and prohibit a statistical strength claim from the smoke result.
- [The benchmark accidentally compares against 5.4.1 instead of 5.8.1] → Add an explicit 5.8.1 opponent selector, serialize the exact profile version, and validate the header before accepting evidence.

## Migration Plan

1. Freeze the current 5.8.1 profile snapshot, frozen four-star snapshot, source hashes, existing test baseline, and diagnostic fixture separation.
2. Implement the isolated detector, profile fields, structural candidate stage, telemetry, and regression fixtures without changing the playable binding.
3. Run C tests, symmetry/rule tests, sanitizer checks where supported, and fixed-position deterministic repeats.
4. Freeze budgets and benchmark source hashes, then run the 5.8.2-versus-four-star and 5.8.2-versus-5.8.1 cells.
5. Replay every game, verify terminal results and accepted proof certificates, and publish raw JSONL plus Markdown summaries under the change evidence directory.
6. Promote the iPhone/iPad five-star binding only in a later change if correctness, latency, black-directional behavior, and white/overall non-regression gates all pass. Rollback remains the unchanged 5.8.1 binding.

## Open Questions

- The exact scan time slice, gain cap, and targeted alternative cap must be selected after measuring the focused fixtures on the supported device targets.
- Whether a verified own VCT certificate should bypass the structural scan remains intentionally conservative: the default is to require an own VCF or an independently replayed terminal-before-reply certificate.
- The small-match schedule is no-forbidden to match the existing 5.8.1/four-star evidence; a forbidden direct-match cell should be added only if the targeted rule fixtures show a material rule-specific difference.
