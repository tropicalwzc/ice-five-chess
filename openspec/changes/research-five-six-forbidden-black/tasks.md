## 1. Freeze and attribution (research artifacts)

- [x] Record the current candidate as the `5.6` alias without rewriting 5.6.0/5.6.1.
- [x] Preserve the current overlap-grouped report and raw/replay checksums.
- [x] Quantify opening-schedule winner balance and pair-level schedule effects.
- [x] Quantify actual worker launches, root jobs, overlap groups, escape unknowns,
      and forbidden legality-copy cost.
- [x] Record the RIF/opening-protocol limitation and authoritative references.
- [ ] Run same-schedule 5.6.1 versus 5.6.2 and isolated
      `proofCandidateStagesEnabled` toggle A/B.

## 2. Correct parallel search architecture

- [x] Dispatch independent root jobs dynamically instead of serializing a
      connected overlap component on one worker.
- [x] Add a diagnostic gate requiring more than one worker on multi-root
      positions and report actual concurrency, not only configured cap.
- [ ] Add virtual PN/DN, postponed sibling generation, and/or dovetailing only
      behind deterministic certificate and budget tests.
- [x] Parallelize loss-aware escape verification with a shared absolute
      deadline, reserved budget, private sessions, and completed-result merge.

## 3. Forbidden legality and rules

- [x] Implement a reversible local legality cache and cross-check every cache
      result against `fc_is_legal_move` under both terminal and non-terminal
      positions.
- [x] Add explicit double-three fixtures; document any intentional deviation
      from exact RIF legality until the oracle is complete.
- [ ] Generate/audit no-swap, RIF exchange, Swap2, and Taraguchi-style schedules
      with role/choice metadata and independent 100-game cells.

## 4. Re-evaluate and publish

- [ ] Re-run fixed-position speed/activity tests with 1/4/8 workers and a
      separate escape-stage utilization table.
- [ ] Run each opening protocol against frozen 4-star, with 3-star retained as
      a secondary robustness control; preserve same-color and first-player
      statistics.
- [x] Replay every game, calculate Wilson and opening-paired intervals, and
      classify inconclusive results without pooling rules or colors.
- [ ] Only after a complete pass, prepare a separate explicit promotion change.
