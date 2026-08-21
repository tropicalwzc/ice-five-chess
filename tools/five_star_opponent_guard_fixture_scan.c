#include "../ice five chess/FiveChessAI.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_audit(const char *label,
                        const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                        int x,
                        int y,
                        const FCAIProfile *profile)
{
    FCOpponentGuardAudit audit;
    bool ok = fc_audit_opponent_after_move(
        board, 1, false, profile, x, y, &audit);
    printf("%s=%d,%d legal=%d class=%d vcf=%d distance=%d "
           "vcfCertificate=%d vctEligible=%d vct=%d restored=%d\n",
           label, x, y, ok, audit.completedClass, audit.vcf.status,
           audit.vcf.distance, audit.vcf.certificateVerified,
           audit.vctEligible, audit.vct.status, audit.boardRestored);
}

int main(int argc, char **argv)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[6][7] = board[7][7] = board[8][7] = -1;
    board[7][5] = board[8][5] = 1;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));

    FCAIProfile control = fc_profile_five_star_proof_engine_candidate();
    control.parallelProofEnabled = false;
    control.proofWorkerCount = 1;
    FCAIProfile guard = fc_profile_five_star_opponent_guard_candidate();
    guard.parallelProofEnabled = false;
    guard.proofWorkerCount = 1;
    FCAnalysisResult controlResult;
    FCAnalysisResult guardResult;
    bool controlFound = fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &control,
        UINT64_C(0x0f0e0d0c0b0a0908), FC_RANDOM_EVALUATION,
        0, 0, &controlResult);
    bool guardFound = fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &guard,
        UINT64_C(0x0f0e0d0c0b0a0908), FC_RANDOM_EVALUATION,
        0, 0, &guardResult);
    printf("control found=%d move=%d,%d tactical=%d proof=%d\n",
           controlFound, controlResult.x, controlResult.y,
           controlResult.tacticalClass, controlResult.proofStatus);
    printf("guard found=%d provisional=%d,%d move=%d,%d class=%d "
           "avoided=%d\n",
           guardFound, guardResult.opponentGuardProvisionalX,
           guardResult.opponentGuardProvisionalY,
           guardResult.x, guardResult.y,
           guardResult.opponentGuardSelectedClass,
           guardResult.opponentGuardAvoidedVerifiedLoss);
    print_audit("control-audit",
                (const int (*)[FC_BOARD_SIZE])board,
                controlResult.x, controlResult.y, &guard);
    print_audit("left-block-audit",
                (const int (*)[FC_BOARD_SIZE])board, 5, 7, &guard);
    print_audit("right-block-audit",
                (const int (*)[FC_BOARD_SIZE])board, 9, 7, &guard);
    printf("boardRestored=%d\n", memcmp(before, board, sizeof(before)) == 0);

    static const int defaultRecordedLoss[][2] = {
        {7,7},{6,8},{7,10},{8,9},{9,6},{7,9},{6,9},{8,10},{5,7},{8,7},
        {8,11},{5,8},{8,8},{4,8},{3,8},{7,6},{9,9},{9,11},{10,12},{10,10},
        {9,8},{9,10},{11,12},{8,12},{11,9},{6,7},{9,12},{10,13},{6,6},{5,5},
        {9,7},{9,5},{12,12},{13,12},{8,5},{7,8}
    };
    int parsedLoss[FC_MAX_POSITION_DELTAS][2];
    const int (*recordedLoss)[2] = defaultRecordedLoss;
    size_t recordedCount =
        sizeof(defaultRecordedLoss) / sizeof(defaultRecordedLoss[0]);
    const char *fixtureLabel = "opening-0";
    if (argc >= 2) {
        size_t inputLength = strlen(argv[1]);
        char *input = malloc(inputLength + 1);
        if (input == NULL) return 2;
        memcpy(input, argv[1], inputLength + 1);
        recordedCount = 0;
        for (char *token = strtok(input, ";"); token != NULL;
             token = strtok(NULL, ";")) {
            int x = -1;
            int y = -1;
            if (recordedCount >= FC_MAX_POSITION_DELTAS ||
                sscanf(token, "%d,%d", &x, &y) != 2) {
                free(input);
                return 2;
            }
            parsedLoss[recordedCount][0] = x;
            parsedLoss[recordedCount][1] = y;
            recordedCount++;
        }
        free(input);
        recordedLoss = (const int (*)[2])parsedLoss;
        if (argc >= 3) fixtureLabel = argv[2];
    }
    memset(board, 0, sizeof(board));
    guard.opponentGuardVCTNodeBudget = 1;
    guard.opponentGuardVCTTimeBudgetMs = 1;
    for (size_t ply = 0;
         ply < recordedCount; ply++) {
        int side = (ply & 1U) == 0 ? 1 : -1;
        int x = recordedLoss[ply][0];
        int y = recordedLoss[ply][1];
        if (side == 1) {
            FCOpponentGuardAudit selectedAudit;
            if (fc_audit_opponent_after_move(
                    (const int (*)[FC_BOARD_SIZE])board, 1, false,
                    &guard, x, y, &selectedAudit) &&
                selectedAudit.vcf.status == FC_PROOF_PROVEN_WIN &&
                selectedAudit.vcf.certificateVerified) {
                printf("recorded-separation-candidate ply=%zu move=%d,%d "
                       "vcfDistance=%d vcfNodes=%llu vcfMs=%.3f "
                       "certificate=0x%016llx\n",
                       ply, x, y, selectedAudit.vcf.distance,
                       (unsigned long long)selectedAudit.vcf.nodes,
                       selectedAudit.vcf.elapsedMilliseconds,
                       (unsigned long long)selectedAudit.vcf.certificateId);
                for (int ax = 0; ax < FC_BOARD_SIZE; ax++) {
                    for (int ay = 0; ay < FC_BOARD_SIZE; ay++) {
                        if (!fc_is_legal_move(
                                (const int (*)[FC_BOARD_SIZE])board,
                                ax, ay, 1, false)) continue;
                        FCOpponentGuardAudit alternative;
                        if (fc_audit_opponent_after_move(
                                (const int (*)[FC_BOARD_SIZE])board,
                                1, false, &guard, ax, ay, &alternative) &&
                            alternative.vcf.status ==
                                FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
                            FCAIProfile exactGuard =
                                fc_profile_five_star_opponent_guard_candidate();
                            FCAnalysisResult guardedDecision;
                            bool guardedFound =
                                fc_analyze_five_star_profile_with_hint(
                                    (const int (*)[FC_BOARD_SIZE])board,
                                    1, false, &exactGuard,
                                    UINT64_C(0x5e9a13a7d1a60001),
                                    FC_RANDOM_EVALUATION, x, y,
                                    &guardedDecision);
                            printf("recorded-separation fixture=%s "
                                   "ply=%zu selected=%d,%d alternative=%d,%d "
                                   "distance=%d disproofDepth=%d "
                                   "disproofNodes=%llu disproofMs=%.3f "
                                   "guardFound=%d guardProvisional=%d,%d "
                                   "guardSelected=%d,%d guardClass=%d "
                                   "guardAvoided=%d\n",
                                   fixtureLabel, ply, x, y, ax, ay,
                                   selectedAudit.vcf.distance,
                                   alternative.vcf.completedDepth,
                                   (unsigned long long)alternative.vcf.nodes,
                                   alternative.vcf.elapsedMilliseconds,
                                   guardedFound,
                                   guardedDecision.opponentGuardProvisionalX,
                                   guardedDecision.opponentGuardProvisionalY,
                                   guardedDecision.x, guardedDecision.y,
                                   guardedDecision.opponentGuardSelectedClass,
                                   guardedDecision.opponentGuardAvoidedVerifiedLoss);
                            return 0;
                        }
                    }
                }
            }
        }
        if (!fc_make_move(board, x, y, side, false)) {
            fprintf(stderr, "recorded move illegal at ply %zu\n", ply);
            return 2;
        }
    }
    return 0;
}
