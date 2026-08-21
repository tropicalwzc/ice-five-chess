# Opponent-guard parameter selection

Date: 2026-08-20 (Asia/Shanghai)

All cells used diagnostic fixtures or natural opening IDs 40–41. They are
separate from the final smoke IDs 60–71. No smoke outcome was available while
selecting parameters.

## Diagnostic fixture ablation

`tools/five_star_opponent_guard_ablation.c` replayed the checksummed
opening-13/ply-18 separating board, the recorded losing move, and the known
completed defensive alternative. Every cell restored the board exactly and
stayed below 1,456 ms for the full diagnostic decision.

| Cell | VCF/VCT depth | Reserved nodes/ms | Structural VCT | Alternatives | Losing audit | Defensive audit | Result |
|---|---:|---:|---:|---:|---|---|---|
| reserved-48k | 9/10 | 48k/800 | on | 8 | proven, d9 | disproof, d9 | reject: less natural completion and worse tail |
| reserved-64k | 9/10 | 64k/1000 | on | 8 | proven, d9 | disproof, d9 | reject: no diagnostic gain over 96k |
| depth-7-8 | 7/8 | 96k/1400 | on | 8 | bounded d7 | disproof, d7 | reject: does not preserve declared d9 scope |
| VCT-depth-8 | 9/8 | 96k/1400 | on | 8 | proven, d9 | disproof, d9 | reject: fewer natural VCT completions |
| structural-off | 9/10 | 96k/1400 | off | 8 | proven, d9 | disproof, d9 | reject: extra broad VCT work without coverage gain |
| alternatives-4 | 9/10 | 96k/1400 | on | 4 | proven, d9 | disproof, d9 | reject: narrower natural audit coverage |
| selected | 9/10 | 96k/1400 | on | 8 | proven, d9 | disproof, d9 | retain |

## Non-final natural-position ablation

Each JSONL contains four games (opening IDs 40–41, colors exchanged), 20
candidate decisions, deterministic-best play, and no anomaly. Latencies are
per candidate decision.

| Cell | VCF completed | VCT completed | Audited | Disproof/unknown/loss | p50/p95/max ms | Peak RSS |
|---|---:|---:|---:|---:|---:|---:|
| worker-1 | 20 | 11 | 29 | 16/4/9 | 146.9/1184.6/1807.3 | 15.8 MB |
| worker-4 | 20 | 12 | 29 | 17/3/9 | 105.3/1787.3/3585.1 | 27.9 MB |
| selected worker-8 control | 20 | 12 | 29 | 17/3/9 | 108.4/2563.1/3541.0 | 42.1 MB |
| alternatives-0 | 20 | 12 | 21 | 17/3/1 | 107.5/2074.7/3606.2 | 42.5 MB |
| alternatives-4 | 20 | 12 | 25 | 17/3/5 | 106.6/2088.1/3604.1 | 42.5 MB |
| reserved-48k | 20 | 11 | 21 | 17/4/0 | 106.4/3605.5/3700.3 | 42.9 MB |
| depth-7-8 | 20 | 11 | 21 | 17/4/0 | 93.0/1997.7/2720.8 | 42.4 MB |
| structural-off | 20 | 18 | 29 | 17/3/9 | 105.6/2540.0/3650.2 | 42.4 MB |
| VCT-depth-8 | 20 | 11 | 21 | 17/4/0 | 90.4/1987.8/2872.0 | 42.2 MB |

Budget exhaustion occurs in four late tactical decisions in every cell and
returns a completed legal fallback below 5,000 ms; it is therefore reported
but is not a differentiator.

## Frozen selection

The selected candidate is the smallest cell that retains all 17 natural
scoped disproofs and every distance-3/5/7/9 targeted regression:

- VCF depth 9, 48,000 nodes, 500 ms opportunity
- VCT depth 10, 48,000 nodes, 700 ms opportunity
- 96,000 reserved nodes and 1,400 ms guard opportunity
- structural VCT eligibility enabled
- 8 audited alternatives
- 4 proof workers
- 192,000 pre-guard proof nodes, 288,000 aggregate decision nodes
- 4,500 ms internal deadline and 5,000 ms hard deadline

Four workers match the eight-worker completion/coverage cell with materially
lower peak memory and lower p95. One worker misses a completed disproof.

