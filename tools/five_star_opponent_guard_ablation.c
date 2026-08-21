#include "../ice five chess/FiveChessAI.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_int(const char *text)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        fprintf(stderr, "invalid integer: %s\n", text);
        exit(2);
    }
    return (int)value;
}

static uint64_t parse_u64(const char *text)
{
    char *end = NULL;
    unsigned long long value = strtoull(text, &end, 0);
    if (end == text || *end != '\0') {
        fprintf(stderr, "invalid uint64: %s\n", text);
        exit(2);
    }
    return (uint64_t)value;
}

int main(int argc, char **argv)
{
    if (argc != 12) {
        fprintf(stderr,
                "usage: %s LABEL VCF_DEPTH VCT_DEPTH VCF_NODES VCT_NODES "
                "VCF_MS VCT_MS RESERVED_NODES RESERVED_MS STRUCTURAL "
                "ALTERNATIVES\n",
                argv[0]);
        return 2;
    }
    FCAIProfile profile = fc_profile_five_star_opponent_guard_candidate();
    profile.parallelProofEnabled = false;
    profile.proofWorkerCount = 1;
    profile.workerCountOverride = 1;
    profile.opponentGuardVCFMaxDepth = parse_int(argv[2]);
    profile.opponentGuardVCTMaxDepth = parse_int(argv[3]);
    profile.opponentGuardVCFNodeBudget = parse_u64(argv[4]);
    profile.opponentGuardVCTNodeBudget = parse_u64(argv[5]);
    profile.opponentGuardVCFTimeBudgetMs = (uint32_t)parse_int(argv[6]);
    profile.opponentGuardVCTTimeBudgetMs = (uint32_t)parse_int(argv[7]);
    profile.opponentGuardReservedNodes = parse_u64(argv[8]);
    profile.opponentGuardReservedTimeMs = (uint32_t)parse_int(argv[9]);
    profile.opponentGuardStructuralVCTEnabled = parse_int(argv[10]) != 0;
    profile.opponentGuardMaxAlternatives = parse_int(argv[11]);

    static const int stones[][3] = {
        {10,2,1},{9,2,-1},{9,5,1},{10,4,-1},{12,3,1},{11,5,-1},
        {12,6,1},{8,3,-1},{12,4,1},{12,5,-1},{11,3,1},{9,1,-1},
        {11,7,1},{13,5,-1},{10,3,1},{9,3,-1},{13,3,1},{14,3,-1},
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));

    FCOpponentGuardAudit loss;
    FCOpponentGuardAudit defense;
    bool lossOK = fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        10, 6, &loss);
    bool defenseOK = fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        7, 2, &defense);

    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    bool found = fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        UINT64_C(0x41424c4154454752), FC_RANDOM_EVALUATION,
        10, 6, &result);
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    bool restored = memcmp(before, board, sizeof(before)) == 0;

    printf("{\"label\":\"%s\",\"vcfDepth\":%d,\"vctDepth\":%d,"
           "\"vcfNodes\":%" PRIu64 ",\"vctNodes\":%" PRIu64 ","
           "\"vcfMs\":%u,\"vctMs\":%u,\"reservedNodes\":%" PRIu64 ","
           "\"reservedMs\":%u,\"structuralVCT\":%s,"
           "\"alternatives\":%d,\"lossAuditOK\":%s,"
           "\"lossStatus\":%d,\"lossDistance\":%d,"
           "\"lossCertificateVerified\":%s,\"defenseAuditOK\":%s,"
           "\"defenseStatus\":%d,\"defenseDepth\":%d,"
           "\"found\":%s,\"provisional\":[%d,%d],\"selected\":[%d,%d],"
           "\"selectedClass\":%d,\"avoidedLoss\":%s,"
           "\"auditedCount\":%d,\"completedDisproofs\":%d,"
           "\"unknowns\":%d,\"verifiedLosses\":%d,"
           "\"guardNodes\":%" PRIu64 ",\"elapsedMs\":%.3f,"
           "\"peakAllocatedBytes\":%" PRIu64 ",\"budgetExhausted\":%s,"
           "\"boardRestored\":%s}\n",
           argv[1], profile.opponentGuardVCFMaxDepth,
           profile.opponentGuardVCTMaxDepth,
           profile.opponentGuardVCFNodeBudget,
           profile.opponentGuardVCTNodeBudget,
           profile.opponentGuardVCFTimeBudgetMs,
           profile.opponentGuardVCTTimeBudgetMs,
           profile.opponentGuardReservedNodes,
           profile.opponentGuardReservedTimeMs,
           profile.opponentGuardStructuralVCTEnabled ? "true" : "false",
           profile.opponentGuardMaxAlternatives,
           lossOK ? "true" : "false", loss.vcf.status, loss.vcf.distance,
           loss.vcf.certificateVerified ? "true" : "false",
           defenseOK ? "true" : "false", defense.vcf.status,
           defense.vcf.completedDepth, found ? "true" : "false",
           result.opponentGuardProvisionalX,
           result.opponentGuardProvisionalY, result.x, result.y,
           result.opponentGuardSelectedClass,
           result.opponentGuardAvoidedVerifiedLoss ? "true" : "false",
           result.opponentGuardAuditedCount,
           result.opponentGuardCompletedDisproofs,
           result.opponentGuardUnknowns,
           result.opponentGuardVerifiedLosses,
           result.opponentGuardConsumedNodes,
           result.stats.elapsedMilliseconds,
           diagnostics.allocatedBytes,
           result.stats.budgetExhausted ? "true" : "false",
           restored ? "true" : "false");
    return restored && found ? 0 : 1;
}
