#include "../ice five chess/FiveChessAI.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int depths[] = {12, 14, 16};
    const int thresholds[] = {6, 10};

    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;

    for (size_t depthIndex = 0;
         depthIndex < sizeof(depths) / sizeof(depths[0]); depthIndex++) {
        for (size_t thresholdIndex = 0;
             thresholdIndex < sizeof(thresholds) / sizeof(thresholds[0]);
             thresholdIndex++) {
            FCAIProfile profile =
                fc_profile_five_star_v57_branch_first_candidate();
            profile.proofMaxDepth = depths[depthIndex];
            profile.proofNodeBudget = 72000;
            profile.proofParallelNodeBudget = 288000;
            profile.proofWorkerCount = 4;
            profile.proofTimeBudgetMs = 0;
            profile.proofEmergencyTimeBudgetMs = 0;
            profile.branchFirstMinRemainingDepth = thresholds[thresholdIndex];

            int original[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(original, board, sizeof(original));
            FCProofResult result;
            fc_proof_diagnostics_reset();
            bool proven = fc_test_parallel_root_proof(
                (const int (*)[FC_BOARD_SIZE])board, 1, false,
                FC_PROOF_SEARCH_VCT, &profile, &result);
            FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
            bool verified = proven && fc_verify_proof(
                (const int (*)[FC_BOARD_SIZE])board, 1, false, &result);
            bool restored = memcmp(board, original, sizeof(original)) == 0;

            printf("{\"depth\":%d,\"minRemainingDepth\":%d,"
                   "\"status\":%d,\"proven\":%s,\"verified\":%s,"
                   "\"restored\":%s,\"previewBranches\":%llu,"
                   "\"waves\":%llu,\"jobs\":%llu,"
                   "\"jobsCompleted\":%llu,\"usefulJobs\":%llu,"
                   "\"maxConcurrentWorkers\":%llu,"
                   "\"depthExtensions\":%llu,"
                   "\"advancedFourDepthExtensions\":%llu,"
                   "\"advancedThreeDepthExtensions\":%llu,"
                   "\"maxChildDepth\":%llu}\n",
                   profile.proofMaxDepth,
                   profile.branchFirstMinRemainingDepth,
                   result.status,
                   proven ? "true" : "false",
                   verified ? "true" : "false",
                   restored ? "true" : "false",
                   (unsigned long long)diagnostics.branchFirstPreviewBranches,
                   (unsigned long long)diagnostics.branchFirstWaves,
                   (unsigned long long)diagnostics.branchFirstJobs,
                   (unsigned long long)diagnostics.branchFirstJobsCompleted,
                   (unsigned long long)diagnostics.branchFirstUsefulJobs,
                   (unsigned long long)diagnostics.branchFirstMaxConcurrentWorkers,
                   (unsigned long long)diagnostics.branchFirstDepthExtensions,
                   (unsigned long long)diagnostics.branchFirstAdvancedFourDepthExtensions,
                   (unsigned long long)diagnostics.branchFirstAdvancedThreeDepthExtensions,
                   (unsigned long long)diagnostics.branchFirstMaxChildDepth);
            if (!restored) return 1;
        }
    }
    return 0;
}
