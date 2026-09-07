#include "../ice five chess/FiveChessAI.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void snapshot(FCAIProfile p)
{
    printf("PROFILE %s\n", p.name);
    printf("name=%s\n", p.name);
    printf("version=%s\n", p.version);
    printf("maxDepth=%lld\n", (long long)p.maxDepth);
    printf("quiescenceDepth=%lld\n", (long long)p.quiescenceDepth);
    printf("fourDepth=%lld\n", (long long)p.fourDepth);
    printf("doubleThreeDepth=%lld\n", (long long)p.doubleThreeDepth);
    printf("forcingDepth=%lld\n", (long long)p.forcingDepth);
    printf("candidateLimit=%lld\n", (long long)p.candidateLimit);
    printf("nodeBudget=%lld\n", (long long)p.nodeBudget);
    printf("timeBudgetMs=%lld\n", (long long)p.timeBudgetMs);
    printf("transpositionCapacity=%lld\n", (long long)p.transpositionCapacity);
    printf("attackWeight=%lld\n", (long long)p.attackWeight);
    printf("defenseWeight=%lld\n", (long long)p.defenseWeight);
    printf("centerWeight=%lld\n", (long long)p.centerWeight);
    printf("nearBestWindow=%lld\n", (long long)p.nearBestWindow);
    printf("randomTemperature=%.17g\n", p.randomTemperature);
    printf("maxRandomCandidates=%lld\n", (long long)p.maxRandomCandidates);
    printf("proofEnabled=%lld\n", (long long)p.proofEnabled);
    printf("proofEngineCandidate=%lld\n", (long long)p.proofEngineCandidate);
    printf("proofCandidateStagesEnabled=%lld\n", (long long)p.proofCandidateStagesEnabled);
    printf("parallelProofEnabled=%lld\n", (long long)p.parallelProofEnabled);
    printf("openingBookEnabled=%lld\n", (long long)p.openingBookEnabled);
    printf("proofSearchClass=%lld\n", (long long)p.proofSearchClass);
    printf("proofMaxDepth=%lld\n", (long long)p.proofMaxDepth);
    printf("proofNodeBudget=%lld\n", (long long)p.proofNodeBudget);
    printf("proofParallelNodeBudget=%lld\n", (long long)p.proofParallelNodeBudget);
    printf("proofTimeBudgetMs=%lld\n", (long long)p.proofTimeBudgetMs);
    printf("proofWorkerCount=%lld\n", (long long)p.proofWorkerCount);
    printf("workerCountOverride=%lld\n", (long long)p.workerCountOverride);
    printf("proofTranspositionCapacity=%lld\n", (long long)p.proofTranspositionCapacity);
    printf("proofGraphNodeCapacity=%lld\n", (long long)p.proofGraphNodeCapacity);
    printf("proofGraphEdgeCapacity=%lld\n", (long long)p.proofGraphEdgeCapacity);
    printf("lossAwareEnabled=%lld\n", (long long)p.lossAwareEnabled);
    printf("quietThreatEnabled=%lld\n", (long long)p.quietThreatEnabled);
    printf("proofEscapeCandidateLimit=%lld\n", (long long)p.proofEscapeCandidateLimit);
    printf("proofQuietRootLimit=%lld\n", (long long)p.proofQuietRootLimit);
    printf("proofEmergencyTimeBudgetMs=%lld\n", (long long)p.proofEmergencyTimeBudgetMs);
    printf("decisionTimeBudgetMs=%lld\n", (long long)p.decisionTimeBudgetMs);
    printf("eliteCorpusEnabled=%lld\n", (long long)p.eliteCorpusEnabled);
    printf("decisionHardLimitMs=%lld\n", (long long)p.decisionHardLimitMs);
    printf("decisionLedgerVersion=%lld\n", (long long)p.decisionLedgerVersion);
    printf("decisionNodeBudget=%lld\n", (long long)p.decisionNodeBudget);
    printf("decisionMemoryBudgetBytes=%lld\n", (long long)p.decisionMemoryBudgetBytes);
    printf("decisionCorpusQueryBudget=%lld\n", (long long)p.decisionCorpusQueryBudget);
    printf("opponentGuardEnabled=%lld\n", (long long)p.opponentGuardEnabled);
    printf("opponentGuardVCFMaxDepth=%lld\n", (long long)p.opponentGuardVCFMaxDepth);
    printf("opponentGuardVCFNodeBudget=%lld\n", (long long)p.opponentGuardVCFNodeBudget);
    printf("opponentGuardVCFTimeBudgetMs=%lld\n", (long long)p.opponentGuardVCFTimeBudgetMs);
    printf("opponentGuardVCTMaxDepth=%lld\n", (long long)p.opponentGuardVCTMaxDepth);
    printf("opponentGuardVCTNodeBudget=%lld\n", (long long)p.opponentGuardVCTNodeBudget);
    printf("opponentGuardVCTTimeBudgetMs=%lld\n", (long long)p.opponentGuardVCTTimeBudgetMs);
    printf("opponentGuardReservedNodes=%lld\n", (long long)p.opponentGuardReservedNodes);
    printf("opponentGuardReservedTimeMs=%lld\n", (long long)p.opponentGuardReservedTimeMs);
    printf("opponentGuardStructuralVCTEnabled=%lld\n", (long long)p.opponentGuardStructuralVCTEnabled);
    printf("opponentGuardMaxAlternatives=%lld\n", (long long)p.opponentGuardMaxAlternatives);
    printf("earlyVCFSentinelEnabled=%lld\n", (long long)p.earlyVCFSentinelEnabled);
    printf("earlyVCFSentinelPolicy=%lld\n", (long long)p.earlyVCFSentinelPolicy);
    printf("earlyVCFBaseDepth=%lld\n", (long long)p.earlyVCFBaseDepth);
    printf("earlyVCFMaxDepth=%lld\n", (long long)p.earlyVCFMaxDepth);
    printf("earlyVCFNodeBudget=%lld\n", (long long)p.earlyVCFNodeBudget);
    printf("earlyVCFTimeBudgetMs=%lld\n", (long long)p.earlyVCFTimeBudgetMs);
    printf("earlyVCFMaxAlternatives=%lld\n", (long long)p.earlyVCFMaxAlternatives);
    printf("corpusScoreMargin=%lld\n", (long long)p.corpusScoreMargin);
    printf("corpusMinGames=%lld\n", (long long)p.corpusMinGames);
    printf("corpusMinEvents=%lld\n", (long long)p.corpusMinEvents);
}

int main(void)
{
    snapshot(fc_profile_production());
    snapshot(fc_profile_proof_guided(false));
    snapshot(fc_profile_five_star_early_micro_vcf_candidate());
    for (int rule = 0; rule < 2; rule++) {
        for (int side = -1; side <= 1; side += 2) {
            for (int fixture = 0; fixture < 3; fixture++) {
                int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}}, original[FC_BOARD_SIZE][FC_BOARD_SIZE];
                if (fixture > 0) {
                    for (int y = 5; y < 9; y++) board[7][y] = fixture == 1 ? side : -side;
                    board[7][4] = fixture == 1 ? -side : side;
                }
                memcpy(original, board, sizeof(board));
                FCAIProfile p = fc_profile_five_star_early_micro_vcf_candidate();
                FCAnalysisResult result;
                bool found = fc_analyze_five_star_profile_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board, side, rule != 0,
                    &p, 581, FC_RANDOM_EVALUATION, -1, -1, &result);
                assert(found);
                assert(memcmp(original, board, sizeof(board)) == 0);
                printf("TACTICAL rule=%d side=%d fixture=%d move=%d,%d class=%d\n",
                       rule, side, fixture, result.x, result.y, result.tacticalClass);
            }
        }
    }
    return 0;
}
