#include "../ice five chess/FiveChessAI.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *id;
    const int (*moves)[3];
    int moveCount;
    int side;
    int x;
    int y;
    int expectedDistance;
} EarlyVCFFixture;

static FILE *probeOutput;

static const int opening20[][3] = {
    {9,10,1},{6,8,-1},{5,7,1},{8,4,-1},{9,4,1},{8,7,-1},
    {8,6,1},{7,8,-1},{9,6,1},{9,8,-1},{5,8,1},{7,6,-1},
    {10,9,1},{7,7,-1},{7,5,1},{9,7,-1},{10,7,1},{5,4,-1},
    {6,5,1},{7,9,-1},{7,10,1},{8,8,-1},{10,8,1},{6,10,-1},
};

static const int late60[][3] = {
    {7,7,1},{7,9,-1},{5,10,1},{10,7,-1},{7,8,1},{6,10,-1},
    {8,7,1},{6,9,-1},{6,7,1},{9,7,-1},{8,8,1},{8,9,-1},
    {5,9,1},{5,8,-1},{9,8,1},{10,8,-1},{10,9,1},{7,6,-1},
    {8,6,1},{6,8,-1},{6,6,1},{6,11,-1},{6,12,1},
};

static const int late63Early[][3] = {
    {7,7,1},{6,7,-1},{5,9,1},{9,6,-1},{5,4,1},
    {5,7,-1},{8,6,1},{6,8,-1},{7,9,1},{6,9,-1},
};

static const int late63Late[][3] = {
    {7,7,1},{6,7,-1},{5,9,1},{9,6,-1},{5,4,1},{5,7,-1},
    {8,6,1},{6,8,-1},{7,9,1},{6,9,-1},{6,6,1},{7,8,-1},
    {8,7,1},{8,8,-1},{9,8,1},{5,8,-1},{4,8,1},{7,10,-1},
    {4,7,1},{9,10,-1},{7,6,1},{6,5,-1},{5,6,1},{4,6,-1},
    {6,10,1},{3,7,-1},{5,5,1},{11,8,-1},{10,9,1},{11,10,-1},
    {4,4,1},{3,3,-1},{5,3,1},{5,2,-1},{7,11,1},{8,12,-1},
    {8,9,1},{12,9,-1},{11,9,1},{9,9,-1},
};

static const int safeBlock[][3] = {
    {6,7,-1},{7,7,-1},{8,7,-1},{7,5,1},{8,5,1},
};

static void fill_board(const EarlyVCFFixture *fixture,
                       int board[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    for (int i = 0; i < fixture->moveCount; i++) {
        int x = fixture->moves[i][0];
        int y = fixture->moves[i][1];
        int side = fixture->moves[i][2];
        if (!fc_make_move(board, x, y, side, false)) {
            fprintf(stderr, "invalid fixture %s at %d\n", fixture->id, i);
        }
    }
}

static FCAIProfile policy_profile(int policy)
{
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.parallelProofEnabled = false;
    profile.proofWorkerCount = 1;
    if (policy == 0) {
        profile.earlyVCFSentinelPolicy = FC_EARLY_VCF_POLICY_FIXED;
        profile.earlyVCFBaseDepth = 5;
        profile.earlyVCFMaxDepth = 5;
        profile.earlyVCFNodeBudget = 8000;
        profile.earlyVCFTimeBudgetMs = 40;
    } else if (policy == 1) {
        profile.earlyVCFSentinelPolicy = FC_EARLY_VCF_POLICY_FIXED;
        profile.earlyVCFBaseDepth = 7;
        profile.earlyVCFMaxDepth = 7;
        profile.earlyVCFNodeBudget = 16000;
        profile.earlyVCFTimeBudgetMs = 80;
    } else {
        profile.earlyVCFSentinelPolicy = FC_EARLY_VCF_POLICY_ADAPTIVE;
        profile.earlyVCFBaseDepth = 5;
        profile.earlyVCFMaxDepth = 7;
        profile.earlyVCFNodeBudget = 16000;
        profile.earlyVCFTimeBudgetMs = 80;
    }
    return profile;
}

static void run_fixture(const EarlyVCFFixture *fixture, int policy,
                        bool forbiddenBlack)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
    fill_board(fixture, board);
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));
    FCAIProfile profile = policy_profile(policy);
    FCOpponentGuardAudit audit;
    int effectiveDepth = 0;
    bool escalated = false;
    fc_proof_diagnostics_reset();
    bool ok = fc_audit_opponent_micro_vcf_after_move(
        (const int (*)[FC_BOARD_SIZE])board, fixture->side,
        forbiddenBlack, &profile, fixture->x, fixture->y,
        &audit, &effectiveDepth, &escalated);
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    fprintf(probeOutput,
           "{\"fixture\":\"%s\",\"policy\":%d,"
           "\"forbiddenBlack\":%s,\"effectiveDepth\":%d,"
           "\"adaptiveEscalated\":%s,\"ok\":%s,"
           "\"status\":%d,\"class\":%d,\"distance\":%d,"
           "\"expectedDistance\":%d,\"nodes\":%llu,\"ms\":%.6f,"
           "\"certificateVerified\":%s,\"certificateId\":"
           "\"0x%016llx\",\"sessionHits\":%llu,"
           "\"boardRestored\":%s}\n",
           fixture->id, policy, forbiddenBlack ? "true" : "false",
           effectiveDepth, escalated ? "true" : "false",
           ok ? "true" : "false", audit.vcf.status,
           audit.completedClass, audit.vcf.distance,
           fixture->expectedDistance,
           (unsigned long long)audit.vcf.nodes,
           audit.vcf.elapsedMilliseconds,
           audit.vcf.certificateVerified ? "true" : "false",
           (unsigned long long)audit.vcf.certificateId,
           (unsigned long long)diagnostics.earlyVCFCacheHits,
           memcmp(before, board, sizeof(before)) == 0
               ? "true" : "false");
}

static void run_full_guard_fixture(const EarlyVCFFixture *fixture)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
    fill_board(fixture, board);
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));
    FCAIProfile profile = fc_profile_five_star_opponent_guard_candidate();
    FCOpponentGuardAudit audit;
    bool ok = fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])board, fixture->side, false,
        &profile, fixture->x, fixture->y, &audit);
    fprintf(probeOutput,
           "{\"fixture\":\"%s\",\"policy\":\"full-guard\","
           "\"ok\":%s,\"class\":%d,\"vcfStatus\":%d,"
           "\"vcfDistance\":%d,\"vcfNodes\":%llu,\"vcfMs\":%.6f,"
           "\"vcfCertificateVerified\":%s,\"vctEligible\":%s,"
           "\"vctStatus\":%d,\"vctDistance\":%d,"
           "\"vctNodes\":%llu,\"vctMs\":%.6f,"
           "\"vctCertificateVerified\":%s,\"boardRestored\":%s}\n",
           fixture->id, ok ? "true" : "false", audit.completedClass,
           audit.vcf.status, audit.vcf.distance,
           (unsigned long long)audit.vcf.nodes,
           audit.vcf.elapsedMilliseconds,
           audit.vcf.certificateVerified ? "true" : "false",
           audit.vctEligible ? "true" : "false", audit.vct.status,
           audit.vct.distance, (unsigned long long)audit.vct.nodes,
           audit.vct.elapsedMilliseconds,
           audit.vct.certificateVerified ? "true" : "false",
           memcmp(before, board, sizeof(before)) == 0
               ? "true" : "false");
}

int main(int argc, char **argv)
{
    probeOutput = stdout;
    if (argc > 1) {
        probeOutput = fopen(argv[1], "w");
        if (probeOutput == NULL) return 2;
    }
    const EarlyVCFFixture fixtures[] = {
        {"vcf-distance-9", opening20, 16, 1, 10, 7, 9},
        {"vcf-distance-7", opening20, 18, 1, 6, 5, 7},
        {"vcf-distance-5", opening20, 20, 1, 7, 10, 5},
        {"vcf-distance-3", opening20, 22, 1, 10, 8, 3},
        {"late-correction-60", late60,
         (int)(sizeof(late60) / sizeof(late60[0])), -1, 9, 9, 0},
        {"late-correction-63-early", late63Early,
         (int)(sizeof(late63Early) / sizeof(late63Early[0])),
         1, 6, 10, 0},
        {"late-correction-63-late", late63Late,
         (int)(sizeof(late63Late) / sizeof(late63Late[0])),
         1, 13, 10, 0},
        {"safe-block", safeBlock,
         (int)(sizeof(safeBlock) / sizeof(safeBlock[0])),
         1, 5, 7, -1},
        {"quiet-center", NULL, 0, 1, 7, 7, -1},
    };
    for (int policy = 0; policy < 3; policy++) {
        for (size_t i = 0; i < sizeof(fixtures) / sizeof(fixtures[0]); i++)
            run_fixture(&fixtures[i], policy, false);
    }
    for (int policy = 0; policy < 3; policy++) {
        for (size_t i = 0; i < 4; i++)
            run_fixture(&fixtures[i], policy, true);
    }
    for (size_t i = 4; i < 7; i++)
        run_full_guard_fixture(&fixtures[i]);
    if (probeOutput != stdout) fclose(probeOutput);
    return 0;
}
