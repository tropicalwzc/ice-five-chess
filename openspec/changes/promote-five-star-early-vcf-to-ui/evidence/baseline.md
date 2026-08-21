# Pre-promotion baseline

Recorded on 2026-08-21 before changing the playable five-star assignment.

## Source hashes

```text
6fec25456b3e84f23a6a4b5ab3b6393fe462d18f921a84fefd0dd3ce3b46b9da  ice five chess/doublethree.m
1f100cb0797164379074a30b19964e8e8a53c24db79af3222bb2b87dd67f317f  ice five chess/ViewController.m
62c05e4c4cf20d8d17dd7efc74262340e4e7040ebc0cb3a7139af0e9fccd320d  ice five chess/HDViewController.m
5a952f94004f893e699b03f818c81ef54d01f1da64bc4d4c4523663de04c7b29  ice five chess/FiveChessAI.c
c37286ff5daef343dff0ab102d4bdd75c859322461297f6c3bbed5259b23d3b4  ice five chess/FiveChessAI.h
```

The pre-existing working-tree diff in `doublethree.m` changes only the profile snapshot buffer from 1,024 to 8,192 bytes near `production_ai_profile_snapshot`; this promotion must preserve it.

## Pre-promotion shared bindings

```objective-c
-(void) four_star_analysisboard:(int) mode
{
    production_ai_profile=fc_profile_proof_guided(false);
    [self optimized_analysisboard:mode];
}
-(void) five_star_analysisboard:(int) mode
{
    production_ai_profile=fc_profile_five_star_proof_engine_candidate();
    [self optimized_analysisboard:mode];
}
```

Both `ViewController.m` and `HDViewController.m` call `five_star_analysisboard:` for the playable five-star selection.

## Frozen identities

- Promotion target: `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`, adaptive depth 5/7, 16,000 nodes, 80 ms, two early alternatives.
- Rollback/control: `five-star-incremental-dfpn-candidate@5.4.1-transactional-deadline-root-parallel-5s`.
- Four-star remains `fc_profile_proof_guided(false)` in the UI method.

The target and rollback factories coexist in `FiveChessAI.c`; promotion SHALL change neither factory.

## Standard-test evidence

- Report: [`../../add-five-star-early-micro-vcf-sentinel/evidence/standard-tests/report.md`](../../add-five-star-early-micro-vcf-sentinel/evidence/standard-tests/report.md)
- Candidate versus four-star raw SHA-256: `2930810065a35b63059d271cd7e9c0e6ff9e03479e8f02f269afa6bef5c0fbd9`.
- Candidate versus exact 5.4.1 raw SHA-256: `e27720fcac9c738acce3084413109f230090fa554b3f10168e978a000aa52f76`.
- Report SHA-256: `ca34349a344c9359a71c0f3d37224f4ee712777ac1d38227138d963078906ec1`.

The main 200-game result was 56/44 versus four-star and 52/48 versus exact 5.4.1, with zero candidate-side decisions above 5,000 ms and full independent replay.
