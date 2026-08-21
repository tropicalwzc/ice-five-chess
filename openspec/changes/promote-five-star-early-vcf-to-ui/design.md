## Context

Both `ViewController.m` and `HDViewController.m` route the five-star difficulty to `-[doublethree five_star_analysisboard:]`. That shared method currently assigns exact 5.4.1 to `production_ai_profile` before entering the common optimized analysis path. The isolated 5.8.1 factory is already implemented, tested, benchmark-selectable, and profile-frozen; its standard Freestyle follow-up was 56/44 against four-star and 52/48 against exact 5.4.1, with zero candidate-side decisions above 5,000 ms and full replay/certificate verification.

The promotion changes product selection, not engine behavior. The working tree already contains unrelated edits, including a larger profile-snapshot buffer later in `doublethree.m`, which must be preserved.

## Goals / Non-Goals

**Goals:**

- Make every playable five-star move on iPhone and iPad use the frozen 5.8.1 early micro-VCF profile.
- Keep exact 5.4.1 available and byte-comparable as the explicit rollback/control factory.
- Prove the five-star source binding and unchanged four-star/lower-level routing with focused regression checks.
- Verify the existing AI suite and application compilation after promotion.

**Non-Goals:**

- Change 5.8.1 parameters, search behavior, model version, rules, or decision deadlines.
- Remove, rename, or mutate exact 5.4.1.
- Change difficulty ordering, controllers, storyboards, persisted difficulty values, or saved games.
- Claim statistical superiority from the standard follow-up or rerun strength tuning during promotion.

## Decisions

### 1. Replace only the shared five-star factory assignment

`five_star_analysisboard:` will assign `fc_profile_five_star_early_micro_vcf_candidate()` and continue through the existing optimized analysis path. Because both phone and tablet controllers already call this method, changing controller call sites would duplicate policy and create drift.

Alternative considered: make `fc_profile_five_star_proof_engine_candidate()` return 5.8.1. Rejected because it would destroy the exact 5.4.1 control and invalidate rollback and benchmark attribution.

### 2. Preserve explicit 5.4.1 and every lower-level binding

The 5.4.1 factory and version remain unchanged and benchmark-selectable. `four_star_analysisboard:` remains bound to the frozen proof-guided no-book profile; one- through three-star routing and persisted values are untouched.

Alternative considered: rename 5.8.1 to a generic production factory. Rejected because the exact profile identity is already frozen and a one-line UI binding is easier to audit and reverse.

### 3. Add a source-scoped binding regression

A focused script will parse the `five_star_analysisboard:` method body and require the 5.8.1 factory while rejecting 5.4.1 within that method. It will separately require the four-star factory and both controller call paths. The test is source-scoped so the intentionally retained 5.4.1 factory elsewhere does not create a false failure.

Alternative considered: rely only on a whole-project build. Rejected because compilation cannot detect selection of the wrong valid factory.

### 4. Keep rollback as a one-line binding change

Rollback replaces the five-star assignment with `fc_profile_five_star_proof_engine_candidate()`; it does not revert engine code, evidence, or user data. This preserves a fast and reviewable operational fallback.

## Risks / Trade-offs

- [The promoted candidate has near-deadline tail latency] → Preserve the existing 5,000 ms hard limit, run the full AI suite/application build, retain exact 5.4.1 for immediate rollback, and keep the measured limitation in evidence.
- [A source edit accidentally changes four-star or persistence behavior] → Limit the runtime diff to the five-star method and assert controller/four-star routing in the binding regression.
- [The profile identity drifts after promotion] → Reuse the frozen factory directly and keep its existing profile-semantic tests.
- [Existing unrelated `doublethree.m` edits are overwritten] → Apply a narrow patch at the five-star assignment and review the file diff before completion.

## Migration Plan

1. Freeze the pre-promotion five-star and four-star method bodies and exact profile versions in evidence.
2. Add the source-scoped binding regression and confirm it fails against the old five-star assignment.
3. Replace the shared five-star factory assignment with the frozen 5.8.1 factory.
4. Run the binding regression, complete C AI suite, Objective-C benchmark build, and application build where available.
5. Record final source hashes and validation results. Roll back by restoring the single five-star assignment to the exact 5.4.1 factory.

## Open Questions

None. The user explicitly authorized promotion after reviewing the standard-test result.
