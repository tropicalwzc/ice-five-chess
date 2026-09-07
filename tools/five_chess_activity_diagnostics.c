#include "../ice five chess/FiveChessAI.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static void place(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                  const int stones[][3], size_t count)
{
    for (size_t i = 0; i < count; i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
}

static void print_diagnostics(const char *name, FCProofDiagnostics d)
{
    printf("\"%s\":{", name);
    printf("\"mostProvingExpansions\":%llu,",
           (unsigned long long)d.mostProvingExpansions);
    printf("\"proofSessionHits\":%llu,",
           (unsigned long long)d.proofSessionHits);
    printf("\"allocatedBytes\":%llu,",
           (unsigned long long)d.allocatedBytes);
    printf("\"proofGraphNodes\":%llu,",
           (unsigned long long)d.proofGraphNodes);
    printf("\"proofGraphEdges\":%llu,",
           (unsigned long long)d.proofGraphEdges);
    printf("\"proofGraphArenaExhaustions\":%llu,",
           (unsigned long long)d.proofGraphArenaExhaustions);
    printf("\"relevanceVerifiedOmissions\":%llu,",
           (unsigned long long)d.relevanceVerifiedOmissions);
    printf("\"iteratedRelatedZoneIntersections\":%llu,",
           (unsigned long long)d.iteratedRelatedZoneIntersections);
    printf("\"iteratedRelatedZoneContinuations\":%llu,",
           (unsigned long long)d.iteratedRelatedZoneContinuations);
    printf("\"iteratedRelatedZonePointsRemoved\":%llu,",
           (unsigned long long)d.iteratedRelatedZonePointsRemoved);
    printf("\"dependencyCombinations\":%llu,",
           (unsigned long long)d.dependencyCombinations);
    printf("\"dependencyChainsProposed\":%llu,",
           (unsigned long long)d.dependencyChainsProposed);
    printf("\"quietRootsExamined\":%llu,",
           (unsigned long long)d.quietRootsExamined);
    printf("\"completedScopeDisproofs\":%llu,",
           (unsigned long long)d.completedScopeDisproofs);
    printf("\"parallelBatches\":%llu,",
           (unsigned long long)d.parallelBatches);
    printf("\"parallelWorkersLaunched\":%llu,",
           (unsigned long long)d.parallelWorkersLaunched);
    printf("\"parallelRootJobs\":%llu,",
           (unsigned long long)d.parallelRootJobs);
    printf("\"parallelRootJobsCompleted\":%llu,",
           (unsigned long long)d.parallelRootJobsCompleted);
    printf("\"parallelRootWins\":%llu,",
           (unsigned long long)d.parallelRootWins);
    printf("\"parallelAggregateDisproofs\":%llu}",
           (unsigned long long)d.parallelAggregateDisproofs);
}

int main(void)
{
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    profile.proofNodeBudget = 18000;
    profile.proofMaxDepth = 9;

    int sessionBoard[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int sessionStones[][3] = {
        {5,7,1}, {6,7,1}, {7,5,1}, {7,6,1},
        {0,0,1}, {0,2,-1}, {12,12,1}, {12,14,-1}
    };
    place(sessionBoard, sessionStones,
          sizeof(sessionStones) / sizeof(sessionStones[0]));
    FCProofResult first;
    FCProofResult second;
    fc_proof_diagnostics_reset();
    bool reuseStable = fc_test_candidate_proof_session_reuse(
        (const int (*)[FC_BOARD_SIZE])sessionBoard, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &first, &second);
    FCProofDiagnostics sessionDiagnostics = fc_proof_diagnostics_get();

    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    immediate[7][4] = immediate[7][5] =
        immediate[7][6] = immediate[7][7] = 1;
    FCProofResult iteratedProof;
    fc_proof_diagnostics_reset();
    bool iteratedVerified = fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false,
        FC_PROOF_SEARCH_VCF, &profile, &iteratedProof) &&
        iteratedProof.certificateVerified &&
        fc_verify_proof((const int (*)[FC_BOARD_SIZE])immediate,
                        1, false, &iteratedProof);
    FCProofDiagnostics iteratedDiagnostics = fc_proof_diagnostics_get();

    int dependency[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int dependencyStones[][3] = {
        {5,7,1}, {5,8,-1}, {7,6,1}, {7,5,-1}, {8,5,1},
        {7,7,-1}, {8,7,1}, {8,6,-1}, {9,5,1}, {9,8,-1}
    };
    place(dependency, dependencyStones,
          sizeof(dependencyStones) / sizeof(dependencyStones[0]));
    FCProofResult dependencyResult;
    fc_proof_diagnostics_reset();
    (void)fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])dependency, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &dependencyResult);
    FCProofDiagnostics dependencyDiagnostics = fc_proof_diagnostics_get();

    int quiet[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int quietStones[][3] = {
        {4,7,1}, {10,7,1}, {7,4,1}, {7,10,1},
        {5,5,-1}, {9,9,-1}
    };
    place(quiet, quietStones,
          sizeof(quietStones) / sizeof(quietStones[0]));
    FCAIProfile quietProfile = profile;
    quietProfile.proofMaxDepth = 2;
    quietProfile.proofNodeBudget = 200;
    FCAnalysisResult quietResult;
    fc_proof_diagnostics_reset();
    bool quietAnalyzed = fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false, &quietProfile,
        UINT64_C(0x4143544956495459), FC_RANDOM_EVALUATION,
        7, 7, &quietResult);
    FCProofDiagnostics quietDiagnostics = fc_proof_diagnostics_get();

    int refuted[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    refuted[5][7] = refuted[6][7] = refuted[7][7] = 1;
    refuted[4][7] = refuted[9][7] = -1;
    FCProofResult freeDisproof;
    fc_proof_diagnostics_reset();
    (void)fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, false,
        FC_PROOF_SEARCH_VCF, &profile, &freeDisproof);
    FCProofDiagnostics freeDiagnostics = fc_proof_diagnostics_get();
    bool freeReplay = fc_verify_scoped_disproof(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, false, &freeDisproof);

    FCProofResult forbiddenDisproof;
    fc_proof_diagnostics_reset();
    (void)fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, true,
        FC_PROOF_SEARCH_VCF, &profile, &forbiddenDisproof);
    FCProofDiagnostics forbiddenDiagnostics = fc_proof_diagnostics_get();
    bool forbiddenReplay = fc_verify_scoped_disproof(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, true,
        &forbiddenDisproof);
    bool parallelActive = quietDiagnostics.parallelBatches > 0 &&
        quietDiagnostics.parallelWorkersLaunched >= 2 &&
        quietDiagnostics.parallelRootJobs >= 2;

    printf("{");
    printf("\"schemaVersion\":2,");
    printf("\"profileVersion\":\"%s\",", profile.version);
    printf("\"reuseStable\":%s,", reuseStable ? "true" : "false");
    printf("\"iteratedCertificateReplay\":%s,",
           iteratedVerified ? "true" : "false");
    printf("\"quietAnalyzed\":%s,", quietAnalyzed ? "true" : "false");
    printf("\"freeScopedDisproofReplay\":%s,",
           freeReplay ? "true" : "false");
    printf("\"forbiddenScopedDisproofReplay\":%s,",
           forbiddenReplay ? "true" : "false");
    printf("\"parallelActive\":%s,",
           parallelActive ? "true" : "false");
    printf("\"sections\":{");
    print_diagnostics("session", sessionDiagnostics);
    printf(",");
    print_diagnostics("iterated", iteratedDiagnostics);
    printf(",");
    print_diagnostics("dependency", dependencyDiagnostics);
    printf(",");
    print_diagnostics("quiet", quietDiagnostics);
    printf(",");
    print_diagnostics("freeDisproof", freeDiagnostics);
    printf(",");
    print_diagnostics("forbiddenDisproof", forbiddenDiagnostics);
    printf("}}\n");
    return reuseStable && iteratedVerified && quietAnalyzed && parallelActive &&
           freeReplay && forbiddenReplay ? 0 : 1;
}
