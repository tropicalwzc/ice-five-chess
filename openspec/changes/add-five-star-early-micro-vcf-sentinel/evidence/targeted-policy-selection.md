# Early micro-VCF targeted policy selection

Date: 2026-08-20 (Asia/Shanghai)

## Frozen fixtures and policies

The isolated probe evaluates the same strict opponent-after-move VCF entry
used by the runtime sentinel. It covers replayable VCF distance 3/5/7/9
positions in freestyle and forbidden-black modes, the three reconstructed
late-correction boards, two non-loss controls, and the full final guard on the
three reconstructed boards. The C correctness suite supplies the corresponding
eight-symmetry matrix, both-rule replay, malformed-certificate, restoration,
rollback, cache-scope, and final-guard interaction coverage.

The three policies use these frozen limits:

| Policy | Depth | Nodes | Time | Result |
|---|---:|---:|---:|---|
| fixed d5 | 5 | 8,000 | 40 ms | Correct but lower in-scope recall |
| fixed d7 | 7 | 16,000 | 80 ms | Eligible |
| adaptive d5/7 | 5, escalating to 7 on forcing signal | 16,000 | 80 ms | Selected |

## Result

Freestyle targeted measurements (nine records per policy):

| Policy | VCF 3/5/7/9 caught | Late corrections caught | False rejects | p50/p95/max | Fresh nodes | Board restored |
|---|---:|---:|---:|---:|---:|---:|
| fixed d5 | 2/4 | 0/3 | 0 | 1.362/40.420/40.420 ms | 240 | 9/9 |
| fixed d7 | 4/4 | 1/3 | 0 | 1.287/80.175/80.175 ms | 267 | 9/9 |
| adaptive d5/7 | 4/4 | 1/3 | 0 | 1.304/80.166/80.166 ms | 265 | 9/9 |

The distance-9 fixture has a shorter replayable distance-7 certificate, so
both depth-7 policies correctly catch it at distance 7. All winning results
have independently replayed certificates. The forbidden-black copies have the
same 2/4, 4/4, and 4/4 recall pattern. No safe control was rejected, every
caller board was restored, and cache reuse was zero because every probe query
starts from a reset proof session.

The isolated probe does not execute own-search, so it consumes no downstream
own-proof budget and has no final-move change by construction. The integration
suite verifies that sentinel disproof/unknown continues through own-search and
the authoritative final guard. On the reconstructed boards, the final guard
finds two VCF losses and one VCT-only loss; adaptive catches one of the two VCF
losses early, while the other two remain final-guard-only:

- `late-correction-60`: final VCF distance 9, 150.680 ms; the 80 ms depth-7
  sentinel remains `unknown`.
- `late-correction-63-early`: VCF disproof followed by VCT distance 9,
  1,969.706 ms; a VCF-only sentinel correctly does not reject it.
- `late-correction-63-late`: final VCF distance 9; the sentinel finds and
  replays a shorter distance-7 VCF in 36.201 ms.

## Deterministic-repeat gate

The two raw 42-record runs agree exactly on fixture order, policy and effective
depth, adaptive signal, validity, three-valued status, completed class,
distance, certificate verification and ID, session hits, restoration, and all
completed-search node counts. Four time-limited `unknown` records differ by
1-5 nodes at the wall-clock boundary. This is expected deadline jitter and
does not change a search classification, move, or completed proof.

The equality gate therefore removes elapsed-time fields and, only for
`status == unknown`, the deadline-bound node count. Under that semantic
normalization the two runs are byte-identical (42/42).

Raw SHA-256:

- `targeted-policy-probe.jsonl`:
  `d6c960506da3d8df80c76cb7ffeb20cf2ca49fd9ca0abf1d712793adc780e7ef`
- `targeted-policy-probe-repeat.jsonl`:
  `1c9b8b025ecd503cc01afe42955aef9def42517b9aef968b2424e0be0e2d1648`
- `tools/five_star_early_vcf_probe.c`:
  `59bfed4982ce29083c7cffb07a1d83244ddb00ec665523dfc52c18c29ac0ba74`

Reproduction comparison:

```sh
diff -u \
  <(jq -c 'del(.ms,.vcfMs,.vctMs) | if .status == 0 then del(.nodes) else . end' \
    targeted-policy-probe.jsonl) \
  <(jq -c 'del(.ms,.vcfMs,.vctMs) | if .status == 0 then del(.nodes) else . end' \
    targeted-policy-probe-repeat.jsonl)
```

## Selection

Freeze `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`: adaptive base depth 5,
maximum depth 7, 16,000 total nodes, 80 ms total time, and at most two early
alternatives. It matches fixed depth 7 on all in-scope recall and correctness
gates and has the lower measured p95. This is a directional research candidate;
the playable UI remains bound to 5.4.1.
