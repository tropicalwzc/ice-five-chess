# Promotion verification

## Outcome

The shared playable five-star method now selects:

```objective-c
production_ai_profile=fc_profile_five_star_early_micro_vcf_candidate();
```

Both iPhone and iPad controllers continue to call that shared method. Four-star remains bound to `fc_profile_proof_guided(false)`.

## Profile verification

The compiled profile snapshot reported:

- Promoted target: `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`, sentinel enabled, adaptive policy, depth 5/7, 16,000 nodes, 80 ms, two early alternatives, 5,000 ms hard limit.
- Rollback/control: `five-star-incremental-dfpn-candidate@5.4.1-transactional-deadline-root-parallel-5s`, early sentinel disabled.
- Frozen four-star control: `four-star-frozen-control@4.0.0-frozen-vcf-vct-control`, early sentinel disabled; the playable four-star method separately remains the proof-guided no-book factory.

## Verification commands

| Check | Result |
| --- | --- |
| `python3 -m py_compile tools/verify_five_star_ui_binding.py` | pass |
| `python3 tools/verify_five_star_ui_binding.py` | pass: playable 5.8.1, rollback 5.4.1, four-star unchanged, iPhone+iPad routed |
| strict C compilation of `five_chess_profile_snapshot.c` | pass |
| `tools/run_five_chess_ai_tests.sh` | pass: `FiveChessAI tests passed` |
| `tools/build_five_chess_benchmark.sh` | pass |
| `xcodebuild ... -scheme 'ice five chess' ... build` | pass, exit 0 |

The application build emitted existing asset-name, deployment-target, precision, and logical-parentheses warnings. No warning or error identified the promoted assignment.

## Scope preservation

The controller and engine hashes are identical to the pre-promotion baseline:

```text
1f100cb0797164379074a30b19964e8e8a53c24db79af3222bb2b87dd67f317f  ice five chess/ViewController.m
62c05e4c4cf20d8d17dd7efc74262340e4e7040ebc0cb3a7139af0e9fccd320d  ice five chess/HDViewController.m
5a952f94004f893e699b03f818c81ef54d01f1da64bc4d4c4523663de04c7b29  ice five chess/FiveChessAI.c
c37286ff5daef343dff0ab102d4bdd75c859322461297f6c3bbed5259b23d3b4  ice five chess/FiveChessAI.h
```

The final shared implementation hash is:

```text
98a4c7b830935c1be2a9a240b08069b56897e4c0feacdb925a8c4cbcd1ea9e91  ice five chess/doublethree.m
4ea74d3f3a353e40687fc9e4d2270c95c302d6d533b3a6d44f3a1cc126fcd39c  tools/verify_five_star_ui_binding.py
```

The only promotion-owned runtime hunk in `doublethree.m` is the five-star comment/factory replacement. The separate pre-existing snapshot buffer increase to 8,192 bytes remains intact.

## Rollback

To restore the prior playable selection without changing engine code or user data, replace only the shared five-star assignment with:

```objective-c
production_ai_profile=fc_profile_five_star_proof_engine_candidate();
```

Exact 5.4.1 remains implemented and benchmark-selectable, so this rollback does not require rebuilding prior engine sources.

## Selection evidence

The promotion decision follows the independently replayed 200-game standard test in [`../../add-five-star-early-micro-vcf-sentinel/evidence/standard-tests/report.md`](../../add-five-star-early-micro-vcf-sentinel/evidence/standard-tests/report.md).
