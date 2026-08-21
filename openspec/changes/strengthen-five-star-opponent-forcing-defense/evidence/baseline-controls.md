# Baseline control freeze

Freeze date: 2026-08-20 (Asia/Shanghai)

Repository commit: `3dd2050a6d320ae823f4b557606f6334921d23f6`

This records the pre-change working-tree content. The only pre-existing tracked
changes were Xcode user/scheme state and are outside this manifest. The
OpenSpec change directory was untracked. The SHA-256 values below were captured
before opponent-guard implementation began.

## Runtime and tool checksums

| Role | Path | SHA-256 |
|---|---|---|
| AI implementation, including rule helpers and proof verifier | `ice five chess/FiveChessAI.c` | `e5f1ec5a474f82bae97209cda1cbe16863827bd09494988e98531f03719bb744` |
| Public profile/proof/result contract | `ice five chess/FiveChessAI.h` | `e5e0debb7e73ef1cc7f53abe6b8cb430d4fd54eece1d543c1d1f55fd4c1cda80` |
| Playable bindings and legacy one/two-star implementations | `ice five chess/doublethree.m` | `979d2327a83dd0b04276ab953ab8c38f1f0390967ca66504d23dde2f8be7aa8d` |
| Frozen elite corpus | `ice five chess/FiveChessEliteCorpus.inc` | `dbe17be3159a37ffa769e841d15ce7aa68dab7e976789d1a2ad61c8e9dad4e45` |
| C correctness suite | `tools/five_chess_ai_tests.c` | `120ba73951b30827ad5da9471fb94cf8ef8d972ac76ebb4342d1d6f0e09b4a47` |
| Benchmark runner/JSONL writer | `tools/five_chess_benchmark.m` | `2eca768652f6d3148d5048db0d7220c7ac133a56ac3357211329662a3019e45f` |
| Formal replay | `tools/replay_five_chess_formal.py` | `5944ad4494e0db87394ad9737b0b95eb097f2cfd2766a45596ca2f0388dab35f` |
| Report generator | `tools/generate_five_chess_report.py` | `6a7825e530d3919daaf323a7fe02ba1e7fc5ecd76a554cb107aa3632064a4ae4` |
| C suite runner | `tools/run_five_chess_ai_tests.sh` | `ca2a51fd74975ff592c78b688bc752373f10e5337df7542d48864cb80298aa43` |
| Benchmark builder | `tools/build_five_chess_benchmark.sh` | `b8cfde2fdd801178f791141213ea6f91184263a09188924a4175fbfbf908c668` |
| Snapshot helper | `tools/five_chess_profile_snapshot.c` | `1c5d316681912367be0c087111e27cbcb234193613eb8d6364aebd113e21337d` |

Rule legality is rooted at `fc_is_legal_move` and the incremental forbidden
oracle in `FiveChessAI.c`; independent win/disproof replay is rooted at
`fc_verify_proof` and `fc_verify_scoped_disproof` in the same checksummed file.

## Playable binding freeze

- One/two-star legacy controls remain in `egg_analysisboard:`,
  `harsh_analysisboard:`, and `easy_analysisboard:` and do not use an
  `FCAIProfile`.
- Three-star control is `fc_profile_production()`.
- Four-star binding calls `fc_profile_proof_guided(false)`, equivalent to the
  named `fc_profile_frozen_four_star_control()` snapshot below.
- Five-star binding calls `fc_profile_five_star_proof_engine_candidate()` and
  is the UI-bound 5.4.1 control.

## Frozen profile snapshots

Generated with:

```sh
clang -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
  'ice five chess/FiveChessAI.c' tools/five_chess_profile_snapshot.c \
  -lm -o /private/tmp/five_chess_profile_snapshot
/private/tmp/five_chess_profile_snapshot
```

```json
{
  "three-star-control": {"name":"three-star-production","version":"2.2.0-legacy-hint-safe-gate","maxDepth":3,"quiescenceDepth":3,"fourDepth":10,"doubleThreeDepth":10,"forcingDepth":12,"candidateLimit":12,"nodeBudget":14000,"timeBudgetMs":850,"transpositionCapacity":65536,"attackWeight":100,"defenseWeight":112,"centerWeight":4,"nearBestWindow":60,"randomTemperature":45.000,"maxRandomCandidates":2,"proofEnabled":false,"proofEngineCandidate":false,"proofCandidateStagesEnabled":false,"parallelProofEnabled":false,"openingBookEnabled":false,"proofSearchClass":0,"proofMaxDepth":0,"proofNodeBudget":0,"proofParallelNodeBudget":0,"proofTimeBudgetMs":0,"proofWorkerCount":0,"workerCountOverride":0,"proofTranspositionCapacity":0,"proofGraphNodeCapacity":0,"proofGraphEdgeCapacity":0,"proofSessionMemoryBytes":7602176,"proofParallelPeakMemoryBytes":7602176,"lossAwareEnabled":false,"quietThreatEnabled":false,"proofEscapeCandidateLimit":0,"proofQuietRootLimit":0,"proofEmergencyTimeBudgetMs":0,"decisionTimeBudgetMs":0,"decisionHardLimitMs":0,"decisionLedgerVersion":0,"decisionNodeBudget":0,"decisionMemoryBudgetBytes":0,"decisionCorpusQueryBudget":0,"incrementalLegalityEnabled":false,"validateLegalityCache":false,"recoverySearchEnabled":false,"forkFirstRecoveryEnabled":false,"persistentWorkerPoolEnabled":false,"parallelTokenBlockEnabled":false,"parallelTokenBlockSize":0,"branchFirstSearchEnabled":false,"branchFirstMinRemainingDepth":0,"branchFirstMinBranchCount":0,"branchFirstMaxBranches":0,"branchFirstPreviewDepth":0,"branchFirstAdvancedFourDepthBonus":0,"branchFirstAdvancedThreeDepthBonus":0,"branchFirstTacticalDepthCap":0,"eliteCorpusEnabled":false,"corpusVersion":"none","corpusScoreMargin":0,"corpusMinGames":0,"corpusMinEvents":0},
  "frozen-four-star-control": {"name":"four-star-frozen-control","version":"4.0.0-frozen-vcf-vct-control","maxDepth":3,"quiescenceDepth":3,"fourDepth":10,"doubleThreeDepth":10,"forcingDepth":12,"candidateLimit":12,"nodeBudget":14000,"timeBudgetMs":850,"transpositionCapacity":65536,"attackWeight":100,"defenseWeight":112,"centerWeight":4,"nearBestWindow":60,"randomTemperature":45.000,"maxRandomCandidates":2,"proofEnabled":true,"proofEngineCandidate":false,"proofCandidateStagesEnabled":false,"parallelProofEnabled":false,"openingBookEnabled":false,"proofSearchClass":2,"proofMaxDepth":10,"proofNodeBudget":18000,"proofParallelNodeBudget":0,"proofTimeBudgetMs":160,"proofWorkerCount":0,"workerCountOverride":0,"proofTranspositionCapacity":32768,"proofGraphNodeCapacity":0,"proofGraphEdgeCapacity":0,"proofSessionMemoryBytes":9175040,"proofParallelPeakMemoryBytes":9175040,"lossAwareEnabled":false,"quietThreatEnabled":false,"proofEscapeCandidateLimit":0,"proofQuietRootLimit":0,"proofEmergencyTimeBudgetMs":0,"decisionTimeBudgetMs":0,"decisionHardLimitMs":0,"decisionLedgerVersion":0,"decisionNodeBudget":0,"decisionMemoryBudgetBytes":0,"decisionCorpusQueryBudget":0,"incrementalLegalityEnabled":false,"validateLegalityCache":false,"recoverySearchEnabled":false,"forkFirstRecoveryEnabled":false,"persistentWorkerPoolEnabled":false,"parallelTokenBlockEnabled":false,"parallelTokenBlockSize":0,"branchFirstSearchEnabled":false,"branchFirstMinRemainingDepth":0,"branchFirstMinBranchCount":0,"branchFirstMaxBranches":0,"branchFirstPreviewDepth":0,"branchFirstAdvancedFourDepthBonus":0,"branchFirstAdvancedThreeDepthBonus":0,"branchFirstTacticalDepthCap":0,"eliteCorpusEnabled":false,"corpusVersion":"none","corpusScoreMargin":0,"corpusMinGames":0,"corpusMinEvents":0},
  "ui-bound-five-star-5.4.1-control": {"name":"five-star-incremental-dfpn-candidate","version":"5.4.1-transactional-deadline-root-parallel-5s","maxDepth":3,"quiescenceDepth":3,"fourDepth":10,"doubleThreeDepth":10,"forcingDepth":12,"candidateLimit":12,"nodeBudget":14000,"timeBudgetMs":850,"transpositionCapacity":65536,"attackWeight":100,"defenseWeight":112,"centerWeight":4,"nearBestWindow":60,"randomTemperature":45.000,"maxRandomCandidates":2,"proofEnabled":true,"proofEngineCandidate":true,"proofCandidateStagesEnabled":true,"parallelProofEnabled":true,"openingBookEnabled":false,"proofSearchClass":2,"proofMaxDepth":12,"proofNodeBudget":36000,"proofParallelNodeBudget":288000,"proofTimeBudgetMs":1200,"proofWorkerCount":8,"workerCountOverride":0,"proofTranspositionCapacity":65536,"proofGraphNodeCapacity":0,"proofGraphEdgeCapacity":0,"proofSessionMemoryBytes":10747904,"proofParallelPeakMemoryBytes":85983232,"lossAwareEnabled":true,"quietThreatEnabled":true,"proofEscapeCandidateLimit":8,"proofQuietRootLimit":3,"proofEmergencyTimeBudgetMs":4200,"decisionTimeBudgetMs":4500,"decisionHardLimitMs":0,"decisionLedgerVersion":0,"decisionNodeBudget":0,"decisionMemoryBudgetBytes":0,"decisionCorpusQueryBudget":0,"incrementalLegalityEnabled":false,"validateLegalityCache":false,"recoverySearchEnabled":false,"forkFirstRecoveryEnabled":false,"persistentWorkerPoolEnabled":false,"parallelTokenBlockEnabled":false,"parallelTokenBlockSize":0,"branchFirstSearchEnabled":false,"branchFirstMinRemainingDepth":0,"branchFirstMinBranchCount":0,"branchFirstMaxBranches":0,"branchFirstPreviewDepth":0,"branchFirstAdvancedFourDepthBonus":0,"branchFirstAdvancedThreeDepthBonus":0,"branchFirstTacticalDepthCap":0,"eliteCorpusEnabled":true,"corpusVersion":"gomocup-elite-openings-2020-2026-rule-partitioned-v2","corpusScoreMargin":-80,"corpusMinGames":2,"corpusMinEvents":1}
}
```
