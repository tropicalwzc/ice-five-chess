# Source provenance

The benchmark executable was rebuilt with `tools/build_five_chess_benchmark.sh`. The replay library was built from the same `FiveChessAI.c` with clang `-dynamiclib`. The exact profile snapshot check confirmed that the new selector is 5.8.2, the direct control is 5.8.1, and the existing playable 5.4.1 binding remains separate.

## Post-change SHA-256

```text
fa72e0500ce0d4b22ab8b72e46f830416f330611c716d9c9c6846fba26969be2  ice five chess/FiveChessAI.c
45d9b41ceddc98e8fab7f2409e9e172315788e90003a5f38ab267c9e8edeb668  ice five chess/FiveChessAI.h
0bd005ec6f2085782b32449ffe889291a0a5c7066c019bd874125efe1ace4c3b  tools/five_chess_ai_tests.c
baaaee40c17131ea8af729dda7a8542040a87908b2e4b9079f2987bbdcbdb1ba  tools/five_chess_benchmark.m
48112921f5a0e32dbb0ce65d468073e6f321229142ac84508b1f827ad20b0851  tools/five_chess_profile_snapshot.c
9050253d774a6bba7b67fc2f7aa23a773dcf71d2507c8f769a8dedf44a2e8601  tools/replay_double_three_small_match.py
```

The existing UI files were not touched by this change. Their hashes remain equal to the pre-implementation capture:

```text
ae0c6a058cabccadc7a9ba094280531384c98834c5a6563f275547d2ecc303be  ice five chess/Base.lproj/HDMain.storyboard
e9974bebc8497845a0ae2ec22bc05086bfea804f2fa79f022af3cd7b24a630ed  ice five chess/HDViewController.m
```

Raw record hashes:

```text
90cd6ddb163a77a8f2f77c1ab0d5e96c6a9e6af2b7da134ec8d034a09411f7ca  five-star-5.8.2-vs-four-star.jsonl
c98cf7ff5460a001edd877f9b6d82a8ddce7237aa8483ccc1c54278ac9e17d76  five-star-5.8.2-vs-5.8.1.jsonl
```

## Soft-40 follow-up SHA-256

```text
eb462a2b749a588dd9fd3a23fb6b125b0f4f8778066c6f9ad1bf0e6142294227  ice five chess/FiveChessAI.c
1d86b6da60071356abd651e951e47d4a2e4eb88defeb86740721780d8f04c263  ice five chess/FiveChessAI.h
07fbe144510db5eba1509467b19794edfa8b078855691a6b6faa34e66f323059  tools/five_chess_ai_tests.c
fae6b4d6e7a4db088f07ed6bf641dbda5fb88ad714fe53f8c4a3f8129253c9ee  tools/replay_double_three_small_match.py
cba7c4ec11ec0ba9d27e30e03843e5a39cbd28b739b5bb341aa0aff125acf644  five-star-5.8.2-soft40-vs-four-star.jsonl
54cbe07e4c3e14e030ca94933d1b1a173f37916c7c2f2c9f72303c93419b4f1a  five-star-5.8.2-soft40-vs-5.8.1.jsonl
26c4cb9c84b14af23c6f35b22f283dab79736f384336412c991ac0505a912cf3  replay-soft40-summary.json
b2e84fad815ba814bf5f548821fb0d72704351541a69bc2d3b7a1a636b5c58dc  libFiveChessAI_soft40.dylib
```
