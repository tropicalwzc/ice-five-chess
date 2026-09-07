## Context

The promotion change selects `fc_profile_five_star_early_micro_vcf_candidate()` at frozen version `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`. Its factory currently inherits from 5.8.0, 5.4.1 and 5.1. Other factories enable color hybrids, recovery, persistent scheduling, recursive branch-first and post-promotion 5.8.2 defense experiments. The new user request supersedes the historical exact-5.4.1 retention requirement. Existing SwiftUI cleanup edits must remain intact.

## Goals / Non-Goals

**Goals:** retain one five-star identity with identical effective parameters; remove obsolete selectable variants and their exclusive implementation/tests/tools; keep current regression coverage and historical evidence.

**Non-Goals:** retune strength, deadlines or randomness; change 1–4 star algorithms, UI, saves or corpus; rewrite shared proof algorithms solely because they originated in an older version.

## Decisions

- Flatten the current factory against the unchanged lower-level base. Freeze every profile field before removal and compare the resulting snapshot. Retain the existing 5.8.1 factory name/identity to avoid unnecessary caller drift.
- Remove experimental factories and dedicated analysis routes rather than making old names aliases: silently redirecting old benchmark names would misattribute results.
- Remove isolated experimental code only when it is not reachable from supported profile settings. Keep shared legality, ledger, proof, corpus and regression primitives even if their introduction predates 5.8.1.
- Retire obsolete executable tools and test cases while preserving historical `openspec` records. Maintain a small current-version verification/benchmark path; document that old reproductions require the corresponding Git revision.

## Risks / Trade-offs

- [Inherited parameter drift] → compare full pre/post profile snapshots and deterministic tactical results.
- [Shared helper removed] → strict C compilation, retained regression suite, lower-level golden checks, application builds/tests.
- [Old commands silently select a new algorithm] → remove old selectors and reject unsupported versions explicitly.
- [Historical evidence loses context] → leave prior spec/evidence untouched and document supersession here.

## Migration Plan

Capture baselines, flatten production selection, remove obsolete surfaces and exclusive code, update checks, run validation. Restore retired variants from Git if historical reproduction is required; no user-data migration is needed.

## Open Questions

None: the user explicitly requested production-only five-star cleanup.
