#include "../ice five chess/FiveChessAI.h"

#include <string.h>

int fc_replay_opponent_guard_probe(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int x,
    int y,
    int *completedClass,
    int *completedSearchClass,
    int *vcfStatus,
    int *vcfDistance,
    bool *vcfCertificateVerified,
    int *vctStatus,
    int *vctDistance,
    bool *vctCertificateVerified,
    bool *boardRestored)
{
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));
    FCAIProfile profile = fc_profile_five_star_opponent_guard_candidate();
    FCOpponentGuardAudit audit;
    bool audited = fc_audit_opponent_after_move(
        board, side, forbiddenBlack, &profile, x, y, &audit);
    bool restored = memcmp(before, board, sizeof(before)) == 0;
    if (completedClass != NULL) *completedClass = audit.completedClass;
    if (completedSearchClass != NULL)
        *completedSearchClass = audit.completedSearchClass;
    if (vcfStatus != NULL) *vcfStatus = audit.vcf.status;
    if (vcfDistance != NULL) *vcfDistance = audit.vcf.distance;
    if (vcfCertificateVerified != NULL)
        *vcfCertificateVerified = audit.vcf.certificateVerified;
    if (vctStatus != NULL) *vctStatus = audit.vct.status;
    if (vctDistance != NULL) *vctDistance = audit.vct.distance;
    if (vctCertificateVerified != NULL)
        *vctCertificateVerified = audit.vct.certificateVerified;
    if (boardRestored != NULL) *boardRestored = restored;
    return audited && restored;
}
