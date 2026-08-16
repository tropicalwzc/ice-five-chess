# Non-final structural ablations

Date: 2026-08-14  
Candidate: `5.4.1-transactional-deadline-root-parallel-5s`  
Inputs: synthetic correctness/activity fixtures and the checksummed non-final diagnostic set only. No formal seed or formal game result was used.

Commands:

```sh
tools/run_five_chess_ai_tests.sh
tools/run_five_chess_activity_diagnostics.sh
```

| Dimension | Rejected/control variant | Observation | Frozen candidate selection |
| --- | --- | --- | --- |
| Proof TT | 1 entry | Collision-safe immediate proofs still replay, but the capacity cannot preserve useful cross-query progress. The normal session fixture records 2 compatible hits. | 65,536 entries |
| Graph arena | 1 node / 1 edge | Terminates honestly as unknown and records arena exhaustion; it cannot complete the crossing proof. | 32,768 nodes / 131,072 edges |
| Frontier initialization | uniform `(1,1)` | Removes both branch and depth information. Current estimates are `(1,1)` for branch 1 at root, `(64,1)` for branch 64 at root, and `(1,3)` at eight plies spent. | branch cap 4,096; disproof depth divisor 4 |
| Relevance fallback | unconditional all-legal | The forbidden fallback fixture enumerates all 223 legal defender moves with zero omissions. It is sound but discards the measured scoped reduction. | exact gain/cost/rest/counter/legality zones; all-legal fallback whenever completeness is unavailable |
| Scoped relevance | local-only/radius omission | Rejected by eight-symmetry remote immediate-win and remote forcing-counter fixtures. The selected checker records 3,180 verified omissions in the session fixture and independently replays masks. | conservative commutativity plus counter and forbidden-line dependencies |
| Dependency limit | disabled/VCF-only | Records zero combinations and zero proposed chains on the dependency fixture. | VCT dependency DAG up to the frozen 64-threat capacity; fixture records 1 combination and 1 chain |
| Quiet-root limit | 0 | Eligible natural fixture examines zero roots. | 3 roots maximum; held-out fixture examines 1 and cannot override on heuristic promise |
| Global node allocation | 1 node | Returns unknown with positive proof/disproof numbers and preserves a legal completed move. | 36,000 nodes per private session; ordered parallel root jobs share a pre-partitioned 288,000-node aggregate cap |
| Worker count | 1 and 4 workers | All 1/4/8-worker ablations choose identical moves. Eight workers complete 102/105 no-forbidden jobs versus 91/94 for four workers before the same wall; all forbidden jobs complete at both counts. | 8 isolated root workers on the declared M4 Pro; one formal game process at a time |
| Memory | one private session | Exact session arena is 10,747,904 bytes. | 85,983,232-byte eight-worker arena cap; measured paired-process peak RSS 46,120,960 bytes |
| Wall ceiling | 1 ms diagnostic cutoff and non-transactional partial override | The cutoff returns a legal move; revision 6 showed that retaining an override from an unfinished stage could still make the move schedule-dependent. | 1,200 ms ordinary proof-stage allowance and 4,200 ms emergency allowance inside one absolute 4,500 ms decision deadline; quiet/loss-aware/quiet-defense stages commit atomically or roll back to the last completed result, leaving 500 ms below the 5,000 ms player-visible ceiling |

The selected configuration preserves the frozen four-star control and production v5.1 entry. Renewed sanitizer, TSAN, simulator, primitive-throughput, activity, dual-rule fixed-position hard-ceiling, and broader safety-wall determinism gates pass on this source. Revision-7 freeze is the next step; any later frozen engine/profile/tool change invalidates the resulting formal schedules and games.
