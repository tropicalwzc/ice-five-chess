# Revision-5 reporter compatibility incident

Date: 2026-08-14

The revision-5 engine completed four 100-game dry-run cells with zero runner anomalies and every candidate decision below 5,000ms. Before results were accepted, the frozen report tool was found to retain three predecessor assumptions that conflict with this change's specification:

1. it asserted `openingMode=seeded-random` rather than the official held-out prefix mode `gomocup-held-out-prefix`;
2. it applied an extra same-color black gate to legacy-three-star cells while failing to require strictly positive white direction in four-star cells;
3. its Markdown led with black results rather than the predeclared primary white metric.

The four raw cells are preserved under `reports/five_chess/five_star_proof_engine_20260814/formal/revision5/` as reporter/replay dry-run evidence only. They are not promotion evidence and their seeds/schedules must enter the prior/excluded domains before revision-6 schedule generation.

The corrected reporter passed all four dry-run cells. A new independent replay tool uses a separately compiled C rule library to replay every move under exact production legality and terminal semantics, checks schedule/color/profile provenance, and compares a second deterministic regeneration of every game and completed proof metadata. Self-replay validation covered 400 games and 1,129 accepted certificates before revision-6 freeze.

Revision-5 raw SHA-256:

- no-forbidden vs four-star: `c11c7215b457fa2ca47f27c168caf3fa9196c2c152f39854ffc1408484e4c0c7`
- no-forbidden vs legacy three-star: `62abc35dd85d18b1a9f5b062cc62fcbff7a51fd424b85c7d848624f437d01b5a`
- forbidden vs four-star: `cbf65d9cf583ff44ada27043f54611052cd2385d064fd6fb241d15789605bcf2`
- forbidden vs legacy three-star: `74cee983b4cb2a3bcd2dec0cb9428822d2fe38230d54fe8a27ebaaefaa110658`

No engine, candidate profile, corpus, four-star, or legacy-three-star source changed. The invalidation is solely the required fail-closed response to frozen report/replay logic correction.
