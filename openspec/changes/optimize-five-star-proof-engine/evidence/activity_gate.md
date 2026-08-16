# Five-star proof-engine activity gate

Candidate profile: `5.4.1-transactional-deadline-root-parallel-5s`

Command:

```sh
tools/run_five_chess_activity_diagnostics.sh
```

The fixtures are non-final diagnostics and are excluded from formal strength schedules. Each mode is measured in an isolated diagnostics reset so activity cannot be borrowed from another fixture.

| Required activity | Fixture | Evidence | Result |
| --- | --- | ---: | --- |
| Most-proving DFPN expansion | session | 19 expansions | pass |
| Compatible proof-session reuse | session | 2 hits, stable repeated result | pass |
| Verified relevance omissions | session | 3,180 omissions | pass |
| Iterated related-zone reduction | iterated terminal continuations | 2 continuations, 1 intersection, 26 points removed; certificate replay passed | pass |
| Dependency combination | dependency | 1 compatible combination and 1 proposed chain | pass |
| Eligible quiet roots | quiet | 1 root examined | pass |
| Root-parallel DFPN | quiet | 8 workers, 8 ordered root jobs, 8 completed, 1 aggregate scoped disproof | pass |
| Freestyle scoped disproof | freeDisproof | 1 completed disproof; independent full-scan replay passed | pass |
| Forbidden scoped disproof | forbiddenDisproof | 1 completed disproof; independent full-scan replay passed | pass |

The independent disproof replay uses the non-session full-scan proof path at the same rule, search class, and completed depth. Winning certificates regenerate defender replies and recompute initial, reply-related, iterated related-zone, and per-omission masks while restoring the input board.

The bounded session allocation is 10,747,904 bytes. Eight simultaneous private worker arenas are therefore capped at 85,983,232 bytes (about 82.0 MiB), excluding small thread/result records. The largest single-session activity fixture used 453 graph nodes and 456 edges with zero arena exhaustion, versus the frozen 32,768-node/131,072-edge caps. The parallel fixture allocated nine sessions cumulatively (one caller session plus eight workers) but never ran more than eight worker sessions concurrently.

Machine-readable evidence is in `activity_diagnostics.json`.
