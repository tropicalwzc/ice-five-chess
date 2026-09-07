#include "../ice five chess/FiveChessAI.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer) || \
    __has_feature(undefined_behavior_sanitizer)
#define FC_TEST_SANITIZER_SLOW 1
#endif
#endif

static FCAIProfile test_profile(void)
{
    FCAIProfile profile = fc_profile_production();
    profile.maxDepth = 2;
    profile.quiescenceDepth = 1;
    profile.nodeBudget = 5000;
    profile.timeBudgetMs = 0;
    profile.transpositionCapacity = 4096;
    return profile;
}

static void test_immediate_win_and_board_integrity(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y <= 6; y++) board[7][y] = 1;
    board[6][5] = -1;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    FCAIProfile profile = test_profile();
    FCAnalysisResult result;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 123, FC_RANDOM_EVALUATION, &result));
    assert(result.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN);
    assert(result.x == 7 && (result.y == 2 || result.y == 7));
    assert(memcmp(before, board, sizeof(board)) == 0);
}

static void test_must_defend(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[5][4] = 1;
    for (int y = 5; y <= 8; y++) board[5][y] = -1;
    FCAIProfile profile = test_profile();
    FCAnalysisResult result;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 999, FC_RANDOM_EVALUATION, &result));
    assert(result.tacticalClass == FC_TACTICAL_MUST_DEFEND);
    assert(result.x == 5 && result.y == 9);
}

static void test_edge_rotation_equivalence(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    int rotated[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 0; y < 4; y++) board[0][y] = -1;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            rotated[y][FC_BOARD_SIZE - 1 - x] = board[x][y];
        }
    }
    FCAIProfile profile = test_profile();
    FCAnalysisResult a;
    FCAnalysisResult b;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, -1, false,
                      &profile, 1, FC_RANDOM_EVALUATION, &a));
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])rotated, -1, false,
                      &profile, 1, FC_RANDOM_EVALUATION, &b));
    assert(a.x == 0 && a.y == 4);
    assert(b.x == 4 && b.y == 14);
}

static void test_edge_mirror_and_empty_board(void)
{
    int empty[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    FCAIProfile profile = test_profile();
    FCAnalysisResult center;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])empty, 1, false,
                      &profile, 17, FC_RANDOM_EVALUATION, &center));
    assert(center.x == 7 && center.y == 7);

    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    int mirrored[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int x = 0; x < 4; x++) board[x][0] = 1;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            mirrored[FC_BOARD_SIZE - 1 - x][y] = board[x][y];
        }
    }
    FCAnalysisResult a;
    FCAnalysisResult b;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 18, FC_RANDOM_EVALUATION, &a));
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])mirrored, 1, false,
                      &profile, 18, FC_RANDOM_EVALUATION, &b));
    assert(a.x == 4 && a.y == 0);
    assert(b.x == 10 && b.y == 0);
}

static void test_proven_loss_and_tactical_width(void)
{
    int loss[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 5; y <= 8; y++) loss[7][y] = -1;
    FCAIProfile profile = test_profile();
    FCAnalysisResult result;
    assert(!fc_analyze((const int (*)[FC_BOARD_SIZE])loss, 1, false,
                       &profile, 19, FC_RANDOM_EVALUATION, &result));
    assert(result.provenLoss);
    assert(result.tacticalClass == FC_TACTICAL_PROVEN_LOSS);

    int wins[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 5; y <= 8; y++) wins[7][y] = 1;
    profile.candidateLimit = 1;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])wins, 1, false,
                      &profile, 20, FC_RANDOM_EVALUATION, &result));
    assert(result.candidateCount == 2);
    assert(result.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN);
}

static void test_forbidden_overline(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y <= 7; y++) board[7][y] = 1;
    assert(!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                             7, 8, 1, true));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            7, 8, 1, false));
}

static void test_fixed_seed_and_random_isolation(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[7][7] = 1;
    board[6][7] = -1;
    board[8][7] = 1;
    board[7][6] = -1;
    FCAIProfile profile = test_profile();
    profile.maxDepth = 1;
    profile.nearBestWindow = 1000000;
    profile.randomTemperature = 1000000.0;
    profile.maxRandomCandidates = 6;
    FCAnalysisResult first;
    FCAnalysisResult second;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 0x12345678, FC_RANDOM_EVALUATION, &first));
    srand(98765);
    for (int i = 0; i < 100; i++) (void)rand();
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 0x12345678, FC_RANDOM_EVALUATION, &second));
    assert(first.x == second.x && first.y == second.y);
    assert(first.score == second.score);
    assert(first.stats.nodes == second.stats.nodes);

    FCAnalysisResult interleaved;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, -1, false,
                      &profile, 0xdeadbeef, FC_RANDOM_USER_GAME, &interleaved));
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 0x12345678, FC_RANDOM_EVALUATION, &second));
    assert(first.x == second.x && first.y == second.y);
    assert(first.stats.nodes == second.stats.nodes);

    bool selected[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{false}};
    int unique = 0;
    for (uint64_t seed = 1; seed <= 40; seed++) {
        FCAnalysisResult varied;
        assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                          &profile, seed, FC_RANDOM_USER_GAME, &varied));
        if (!selected[varied.x][varied.y]) {
            selected[varied.x][varied.y] = true;
            unique++;
        }
    }
    assert(unique >= 2);
}

static void test_unique_tactical_move_ignores_seed(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[4][3] = -1;
    for (int y = 4; y <= 7; y++) board[4][y] = 1;
    FCAIProfile profile = test_profile();
    for (uint64_t seed = 1; seed <= 20; seed++) {
        FCAnalysisResult result;
        assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                          &profile, seed, FC_RANDOM_USER_GAME, &result));
        assert(result.x == 4 && result.y == 8);
    }
}

static void test_budget_returns_legal_completed_result(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[7][7] = 1;
    board[7][8] = -1;
    board[8][7] = 1;
    board[6][7] = -1;
    FCAIProfile profile = test_profile();
    profile.maxDepth = 8;
    profile.nodeBudget = 20;
    FCAnalysisResult result;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])board, 1, false,
                      &profile, 77, FC_RANDOM_EVALUATION, &result));
    assert(result.stats.budgetExhausted);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            result.x, result.y, 1, false));
}

static int inverse_transform(int transform)
{
    static const int inverse[8] = {0, 3, 2, 1, 4, 5, 6, 7};
    return inverse[transform & 7];
}

static void test_make_unmake_transforms_and_keys(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    assert(fc_make_move(board, 7, 7, 1, false));
    assert(!fc_make_move(board, 7, 7, -1, false));
    fc_unmake_move(board, 7, 7);
    assert(memcmp(before, board, sizeof(board)) == 0);

    for (int transform = 0; transform < 8; transform++) {
        for (int x = 0; x < FC_BOARD_SIZE; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                int tx = -1, ty = -1, rx = -1, ry = -1;
                fc_transform_point(transform, x, y, &tx, &ty);
                fc_transform_point(inverse_transform(transform),
                                   tx, ty, &rx, &ry);
                assert(rx == x && ry == y);
            }
        }
    }

    board[2][3] = 1;
    board[8][9] = -1;
    uint64_t base = fc_board_key(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, 3);
    assert(base == fc_board_key((const int (*)[FC_BOARD_SIZE])board,
                                1, false, FC_PROOF_SEARCH_VCF, 3));
    assert(base != fc_board_key((const int (*)[FC_BOARD_SIZE])board,
                                -1, false, FC_PROOF_SEARCH_VCF, 3));
    assert(base != fc_board_key((const int (*)[FC_BOARD_SIZE])board,
                                1, true, FC_PROOF_SEARCH_VCF, 3));
    assert(base != fc_board_key((const int (*)[FC_BOARD_SIZE])board,
                                1, false, FC_PROOF_SEARCH_VCT, 3));
    assert(base != fc_board_key((const int (*)[FC_BOARD_SIZE])board,
                                1, false, FC_PROOF_SEARCH_VCF, 4));
}

static void assert_incremental_matches_reference(
    const FCIncrementalPosition *position,
    int side,
    int searchClass,
    uint64_t version)
{
    assert(position->complete);
    assert(fc_incremental_board_key(position, side, searchClass, version) ==
           fc_board_key((const int (*)[FC_BOARD_SIZE])position->board,
                        side, position->forbiddenBlack,
                        searchClass, version));
    int counted = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++)
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            counted += position->board[x][y] != 0;
            int index = x * FC_BOARD_SIZE + y;
            for (int sideIndex = 0; sideIndex < 2; sideIndex++) {
                int sideValue = sideIndex == 0 ? 1 : -1;
                bool legal = fc_is_legal_move(
                    (const int (*)[FC_BOARD_SIZE])position->board,
                    x, y, sideValue, position->forbiddenBlack);
                assert(fc_incremental_is_legal_move(
                    position, x, y, sideValue) == legal);
                bool cachedLegal =
                    (position->legalMoves[sideIndex][index >> 6] &
                     (UINT64_C(1) << (index & 63))) != 0;
                if (position->legalMaskComplete[sideIndex])
                    assert(cachedLegal == legal);
                bool cached = (position->immediateWins[sideIndex][index >> 6] &
                    (UINT64_C(1) << (index & 63))) != 0;
                int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
                memcpy(after, position->board, sizeof(after));
                bool expected = fc_make_move(
                    after, x, y, sideValue, position->forbiddenBlack) &&
                    fc_has_five((const int (*)[FC_BOARD_SIZE])after,
                                x, y, sideValue);
                if (position->immediateWinMaskComplete[sideIndex])
                    assert(cached == expected);
            }
        }
    assert(counted == position->stoneCount);
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            int index = x * FC_BOARD_SIZE + y;
            bool occupied = (position->occupied[index >> 6] &
                (UINT64_C(1) << (index & 63))) != 0;
            assert(occupied == (position->board[x][y] != 0));
            bool expectedFrontier = false;
            if (!occupied) {
                for (int sx = 0; sx < FC_BOARD_SIZE && !expectedFrontier; sx++)
                    for (int sy = 0; sy < FC_BOARD_SIZE; sy++)
                        if (position->board[sx][sy] != 0 &&
                            abs(sx - x) <= 4 && abs(sy - y) <= 4) {
                            expectedFrontier = true;
                            break;
                        }
            }
            bool frontier = (position->frontier[index >> 6] &
                (UINT64_C(1) << (index & 63))) != 0;
            assert(frontier == expectedFrontier);
        }
    }
    for (int direction = 0; direction < 4; direction++) {
        for (int lineId = 0;
             lineId < FC_BOARD_SIZE * 2 - 1; lineId++) {
            uint32_t expectedCode = 0;
            int shift = 0;
            for (int x = 0; x < FC_BOARD_SIZE; x++) {
                for (int y = 0; y < FC_BOARD_SIZE; y++) {
                    int expectedLineId = direction == 0 ? y
                        : direction == 1 ? x
                        : direction == 2
                        ? x - y + FC_BOARD_SIZE - 1 : x + y;
                    if (expectedLineId != lineId) continue;
                    unsigned int value = position->board[x][y] == 1 ? 1U
                        : position->board[x][y] == -1 ? 2U : 0U;
                    expectedCode |= value << shift;
                    shift += 2;
                }
            }
            assert(position->lineCode[direction][lineId] == expectedCode);
        }
    }
}

static void test_incremental_position_reference_equivalence(void)
{
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
        board[7][7] = 1;
        board[6][7] = -1;
        FCIncrementalPosition position;
        assert(fc_incremental_position_init(&position,
            (const int (*)[FC_BOARD_SIZE])board, forbidden != 0));
        FCIncrementalPosition duplicate;
        assert(fc_incremental_position_init(&duplicate,
            (const int (*)[FC_BOARD_SIZE])board, forbidden != 0));
        assert(position.stoneHash == duplicate.stoneHash);
        assert(position.stoneLock == duplicate.stoneLock);
        assert(memcmp(position.occupied, duplicate.occupied,
                      sizeof(position.occupied)) == 0);
        assert(memcmp(position.frontier, duplicate.frontier,
                      sizeof(position.frontier)) == 0);
        assert(position.legalMaskComplete[0]);
        assert(position.immediateWinMaskComplete[0]);
        assert(position.forbiddenLegalityCacheValid);
        assert(fc_incremental_validate_legality_cache(&position));
        assert(position.legalMaskComplete[1]);
        assert(position.immediateWinMaskComplete[1]);
        assert_incremental_matches_reference(
            &position, 1, FC_PROOF_SEARCH_VCT, 12);
        int initial[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(initial, position.board, sizeof(initial));
        uint16_t initialLineRevision[4][FC_BOARD_SIZE * 2 - 1];
        memcpy(initialLineRevision, position.lineRevision,
               sizeof(initialLineRevision));
        uint32_t initialLineCode[4][FC_BOARD_SIZE * 2 - 1];
        memcpy(initialLineCode, position.lineCode,
               sizeof(initialLineCode));
        const int moves[][3] = {
            {8, 7, 1}, {7, 6, -1}, {8, 8, 1}, {6, 6, -1},
            {9, 9, 1}, {5, 5, -1}
        };
        for (size_t i = 0; i < sizeof(moves) / sizeof(moves[0]); i++) {
            assert(fc_incremental_position_make(
                &position, moves[i][0], moves[i][1], moves[i][2]));
            assert_incremental_matches_reference(
                &position, -moves[i][2], FC_PROOF_SEARCH_VCT, 12);
        }
        for (size_t i = 0; i < sizeof(moves) / sizeof(moves[0]); i++) {
            assert(fc_incremental_position_unmake(&position));
            assert_incremental_matches_reference(
                &position, 1, FC_PROOF_SEARCH_VCF, 9);
        }
        assert(memcmp(initial, position.board, sizeof(initial)) == 0);
        assert(memcmp(initialLineRevision, position.lineRevision,
                      sizeof(initialLineRevision)) == 0);
        assert(memcmp(initialLineCode, position.lineCode,
                      sizeof(initialLineCode)) == 0);
        assert(position.deltaCount == 0);
    }

    int invalid[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    invalid[3][4] = 2;
    FCIncrementalPosition rejected;
    assert(!fc_incremental_position_init(
        &rejected, (const int (*)[FC_BOARD_SIZE])invalid, false));
    assert(!rejected.complete);
}

static void test_frozen_four_star_and_candidate_identity(void)
{
    FCAIProfile frozen = fc_profile_frozen_four_star_control();
    FCAIProfile historical = fc_profile_proof_guided(false);
    assert(strcmp(frozen.name, "four-star-frozen-control") == 0);
    assert(!frozen.proofEngineCandidate);
    assert(frozen.proofMaxDepth == historical.proofMaxDepth);
    assert(frozen.proofNodeBudget == historical.proofNodeBudget);
    assert(frozen.proofTimeBudgetMs == historical.proofTimeBudgetMs);
    assert(frozen.proofTranspositionCapacity ==
           historical.proofTranspositionCapacity);
    FCAIProfile candidate = fc_profile_five_star_early_micro_vcf_candidate();
    assert(candidate.proofEngineCandidate);
    assert(candidate.lossAwareEnabled && candidate.quietThreatEnabled);
    assert(candidate.decisionTimeBudgetMs == 4500);
    assert(candidate.earlyVCFSentinelEnabled);
    assert(candidate.opponentGuardEnabled);
    assert(strcmp(candidate.version,
                  "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a") == 0);

    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    immediate[7][4] = immediate[7][5] =
        immediate[7][6] = immediate[7][7] = 1;
    FCAnalysisResult frozenDecision;
    FCAnalysisResult candidateDecision;
    assert(fc_analyze_four_star_with_hint(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false,
        UINT64_C(0x46524f5a454e4241), FC_RANDOM_EVALUATION,
        7, 8, &frozenDecision));
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false, &candidate,
        UINT64_C(0x46524f5a454e4241), FC_RANDOM_EVALUATION,
        7, 8, &candidateDecision));
    assert(candidateDecision.fourStarX == frozenDecision.x);
    assert(candidateDecision.fourStarY == frozenDecision.y);
}

static void test_opponent_guard_profile_and_reserved_ledger(void)
{
    FCAIProfile control = fc_profile_proof_guided(false);
    FCAIProfile guard = fc_profile_five_star_early_micro_vcf_candidate();
    assert(!control.opponentGuardEnabled);
    assert(control.opponentGuardReservedNodes == 0);
    assert(guard.opponentGuardEnabled);
    assert(guard.opponentGuardVCFMaxDepth == 9);
    assert(guard.opponentGuardVCTMaxDepth == 10);
    assert(guard.opponentGuardReservedNodes == 96000);
    assert(guard.proofWorkerCount == 4);
    assert(guard.proofParallelNodeBudget == 192000);
    assert(guard.decisionNodeBudget == 288000);

    FCDecisionLedger ledger;
    assert(fc_decision_ledger_begin(&ledger, &guard));
    assert(fc_decision_ledger_reserve_guard(
        &ledger, guard.opponentGuardReservedNodes));
    for (uint64_t i = 0; i < guard.proofParallelNodeBudget; i++)
        assert(fc_decision_ledger_consume_node(&ledger));
    assert(!fc_decision_ledger_consume_node(&ledger));
    assert(!atomic_load_explicit(&ledger.exhausted, memory_order_acquire));
    assert(fc_decision_ledger_enter_guard(&ledger));
    for (uint64_t i = 0; i < guard.opponentGuardReservedNodes; i++)
        assert(fc_decision_ledger_consume_node(&ledger));
    assert(!fc_decision_ledger_consume_node(&ledger));
    assert(atomic_load_explicit(&ledger.exhausted, memory_order_acquire));
    assert(atomic_load_explicit(&ledger.nodesConsumed,
                                memory_order_relaxed) ==
           guard.decisionNodeBudget);
    fc_decision_ledger_release_guard(&ledger);

    FCDecisionLedger released;
    assert(fc_decision_ledger_begin(&released, &guard));
    assert(fc_decision_ledger_reserve_guard(
        &released, guard.opponentGuardReservedNodes));
    fc_decision_ledger_release_guard(&released);
    assert(!atomic_load_explicit(&released.guardReservationActive,
                                 memory_order_acquire));
    for (uint64_t i = 0; i < guard.decisionNodeBudget; i++)
        assert(fc_decision_ledger_consume_node(&released));
    assert(!fc_decision_ledger_consume_node(&released));
}

static void test_opponent_guard_vcf_first_audit_and_restoration(void)
{
    int source[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    source[6][7] = source[7][7] = source[8][7] = -1;
    source[7][5] = source[8][5] = 1;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, source, sizeof(before));
    FCAIProfile guard = fc_profile_five_star_early_micro_vcf_candidate();
    guard.parallelProofEnabled = false;
    guard.proofWorkerCount = 1;
    FCOpponentGuardAudit losing;
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])source, 1, false, &guard,
        0, 0, &losing));
    assert(losing.vcf.status == FC_PROOF_PROVEN_WIN);
    assert(losing.vcf.distance == 3);
    assert(losing.vcf.certificateVerified);
    assert(losing.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS);
    assert(losing.completedSearchClass == FC_PROOF_SEARCH_VCF);
    assert(losing.boardRestored);
    assert(memcmp(before, source, sizeof(before)) == 0);

    FCOpponentGuardAudit block;
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])source, 1, false, &guard,
        5, 7, &block));
    assert(block.vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(block.completedClass != FC_GUARD_CLASS_VERIFIED_LOSS);
    assert(block.boardRestored);
    assert(memcmp(before, source, sizeof(before)) == 0);

    int quiet[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    FCOpponentGuardAudit quietAudit;
    fc_proof_diagnostics_reset();
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false, &guard,
        7, 7, &quietAudit));
    FCProofDiagnostics quietDiagnostics = fc_proof_diagnostics_get();
    assert(quietAudit.vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(!quietAudit.vctEligible);
    assert(quietDiagnostics.opponentGuardVCFQueries == 1);
    assert(quietDiagnostics.opponentGuardVCTQueries == 0);
    assert(quietDiagnostics.opponentGuardVCTStructuralSkips == 1);

    guard.opponentGuardVCFNodeBudget = 1;
    FCOpponentGuardAudit exhausted;
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])source, 1, false, &guard,
        0, 0, &exhausted));
    assert(exhausted.vcf.status == FC_PROOF_UNKNOWN);
    assert(exhausted.completedClass ==
           FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN);
    assert(exhausted.boardRestored);
}

static void test_early_micro_vcf_profile_and_semantics(void)
{
    FCAIProfile parent = fc_profile_proof_guided(false);
    FCAIProfile sentinel =
        fc_profile_five_star_early_micro_vcf_candidate();
    FCAIProfile control = fc_profile_production();
    assert(!parent.earlyVCFSentinelEnabled);
    assert(!control.earlyVCFSentinelEnabled);
    assert(sentinel.earlyVCFSentinelEnabled);
    assert(sentinel.earlyVCFSentinelPolicy ==
           FC_EARLY_VCF_POLICY_ADAPTIVE);
    assert(sentinel.earlyVCFBaseDepth == 5);
    assert(sentinel.earlyVCFMaxDepth == 7);
    assert(sentinel.earlyVCFNodeBudget == 16000);
    assert(sentinel.earlyVCFTimeBudgetMs == 80);
    assert(sentinel.earlyVCFMaxAlternatives == 2);
    char snapshot[8192];
    assert(fc_profile_snapshot(&sentinel, snapshot, sizeof(snapshot)) > 0);
    assert(strstr(snapshot, "\"earlyVCFSentinelEnabled\":true") != NULL);
    assert(strstr(snapshot, "\"earlyVCFBaseDepth\":5") != NULL);
    assert(strstr(snapshot, "\"earlyVCFMaxDepth\":7") != NULL);

    int forcing[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    forcing[6][7] = forcing[7][7] = forcing[8][7] = -1;
    forcing[7][5] = forcing[8][5] = 1;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, forcing, sizeof(before));
    FCOpponentGuardAudit loss;
    int effectiveDepth = 0;
    bool escalated = false;
    fc_proof_diagnostics_reset();
    assert(fc_audit_opponent_micro_vcf_after_move(
        (const int (*)[FC_BOARD_SIZE])forcing, 1, false,
        &sentinel, 0, 0, &loss, &effectiveDepth, &escalated));
    assert(effectiveDepth == 7);
    assert(escalated);
    assert(loss.vcf.status == FC_PROOF_PROVEN_WIN);
    assert(loss.vcf.certificateVerified);
    assert(loss.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS);
    assert(memcmp(before, forcing, sizeof(before)) == 0);

    int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(after, forcing, sizeof(after));
    assert(fc_make_move(after, 0, 0, 1, false));
    FCProofResult malformed = loss.vcf;
    malformed.certificateId ^= UINT64_C(1);
    assert(!fc_verify_proof(
        (const int (*)[FC_BOARD_SIZE])after, -1, false, &malformed));

    FCAIProfile limited = sentinel;
    limited.earlyVCFNodeBudget = 1;
    FCOpponentGuardAudit unknown;
    assert(fc_audit_opponent_micro_vcf_after_move(
        (const int (*)[FC_BOARD_SIZE])forcing, 1, false,
        &limited, 0, 0, &unknown, &effectiveDepth, &escalated));
    assert(unknown.vcf.status == FC_PROOF_UNKNOWN);
    assert(!unknown.vcf.certificateVerified);
    assert(unknown.boardRestored);

    int ownWin[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y < 7; y++) ownWin[7][y] = 1;
    FCOpponentGuardAudit immediate;
    assert(fc_audit_opponent_micro_vcf_after_move(
        (const int (*)[FC_BOARD_SIZE])ownWin, 1, false,
        &sentinel, 7, 7, &immediate, &effectiveDepth, &escalated));
    assert(immediate.ownImmediateWin);
    assert(immediate.completedClass == FC_GUARD_CLASS_OWN_VERIFIED_WIN);
    assert(immediate.boardRestored);

    int quiet[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    FCOpponentGuardAudit quietAudit;
    assert(fc_audit_opponent_micro_vcf_after_move(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false,
        &sentinel, 7, 7, &quietAudit, &effectiveDepth, &escalated));
    assert(effectiveDepth == 5);
    assert(!escalated);
    assert(quietAudit.vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(quietAudit.completedClass == FC_GUARD_CLASS_SCOPED_DISPROOF);
    assert(quietAudit.boardRestored);
}

static void test_opponent_guard_acceptance_paths_and_isolation(void)
{
    FCAIProfile guard = fc_profile_five_star_early_micro_vcf_candidate();
    guard.parallelProofEnabled = false;
    guard.proofWorkerCount = 1;

    /* A legal immediate win is the first independently complete skip path. */
    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y <= 6; y++) immediate[7][y] = 1;
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false, &guard,
        UINT64_C(0x4755415244494d4d), FC_RANDOM_EVALUATION,
        7, 7, &result));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(result.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN);
    assert(result.opponentGuardSkipReason == FC_GUARD_SKIP_IMMEDIATE_WIN);
    assert(!result.opponentGuardEligible);
    assert(diagnostics.opponentGuardSkippedImmediateWins == 1);
    assert(diagnostics.opponentGuardVCFQueries == 0);
    assert(diagnostics.opponentGuardVCTQueries == 0);

    /* A non-immediate VCF accepted by the independent verifier is the
     * second complete skip path and must not be confused with an opponent
     * proof stored in the legacy proofStatus field. */
    int forced[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    forced[6][7] = forced[7][7] = forced[8][7] = -1;
    forced[7][5] = forced[8][5] = 1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])forced, -1, false, &guard,
        UINT64_C(0x47554152444f574e), FC_RANDOM_EVALUATION,
        5, 7, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(result.tacticalClass == FC_TACTICAL_FORCED_ATTACK);
    assert(result.proofStatus == FC_PROOF_PROVEN_WIN);
    assert(result.proofCertificateVerified);
    assert(result.opponentGuardSkipReason ==
           FC_GUARD_SKIP_VERIFIED_OWN_WIN);
    assert(!result.opponentGuardEligible);
    assert(diagnostics.opponentGuardSkippedVerifiedOwnWins == 1);
    assert(diagnostics.opponentGuardVCFQueries == 0);
    assert(diagnostics.opponentGuardVCTQueries == 0);

    /* The no-corpus/frozen-four-star-unknown handoff must still cross the
     * candidate guard, while frozen lower profiles remain guard-free. */
    int quiet[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    quiet[7][7] = 1;
    quiet[6][7] = -1;
    quiet[8][8] = 1;
    quiet[6][8] = -1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false, &guard,
        UINT64_C(0x47554152444e4f43), FC_RANDOM_EVALUATION,
        7, 8, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(result.corpusReason == FC_CORPUS_NO_POSITION);
    assert(result.handoffReason == FC_HANDOFF_FOUR_STAR_UNKNOWN ||
           result.handoffReason == FC_HANDOFF_GENERATED_FALLBACK);
    assert(result.opponentGuardEligible);
    assert(result.opponentGuardAuditedCount >= 1);
    assert(diagnostics.opponentGuardVCFQueries >= 1);

    FCAIProfile production = fc_profile_production();
    FCAIProfile four = fc_profile_frozen_four_star_control();
    assert(!production.opponentGuardEnabled);
    assert(!four.opponentGuardEnabled);
    fc_proof_diagnostics_reset();
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])quiet, 1, false,
                      &production, UINT64_C(0x47554152444c4f57),
                      FC_RANDOM_EVALUATION, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(!result.opponentGuardEligible);
    assert(diagnostics.opponentGuardEligibleDecisions == 0);
    assert(diagnostics.opponentGuardVCFQueries == 0);
    assert(diagnostics.opponentGuardVCTQueries == 0);

    /* Fixed inputs must reproduce both the completed class and coordinate. */
    FCOpponentGuardAudit first;
    FCOpponentGuardAudit second;
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false, &guard,
        7, 6, &first));
    assert(fc_audit_opponent_after_move(
        (const int (*)[FC_BOARD_SIZE])quiet, 1, false, &guard,
        7, 6, &second));
    assert(first.completedClass == second.completedClass);
    assert(first.completedSearchClass == second.completedSearchClass);
    assert(first.vcf.status == second.vcf.status);
    assert(first.vcf.certificateId == second.vcf.certificateId);
    assert(first.boardRestored && second.boardRestored);
}

static void test_five_star_hard_deadline_returns_completed_legal_move(void)
{
    const int stones[][3] = {
        {5,10,1}, {6,9,-1}, {6,8,1}, {7,8,-1}, {7,5,1},
        {8,4,-1}, {7,7,1}, {8,6,-1}, {7,9,1}, {8,7,-1},
        {8,5,1}, {8,8,-1}, {8,9,1}, {8,10,-1}, {9,6,1},
        {9,7,-1}, {9,8,1}, {10,7,-1}, {11,7,1}
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.decisionTimeBudgetMs = 1;
    FCAnalysisResult result;
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, -1, true, &profile,
        UINT64_C(0x4841524444454144), FC_RANDOM_EVALUATION,
        10, 6, &result));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            result.x, result.y, -1, true));
    assert(result.stats.budgetExhausted);
    assert(result.stats.elapsedMilliseconds < 1000.0);
}

static void test_candidate_single_proof_session_and_counters(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;
    const int remote[][3] = {
        {0,0,1}, {0,2,-1}, {2,0,1}, {2,2,-1}, {12,12,1},
        {12,14,-1}, {14,12,1}, {14,14,-1}, {0,14,1}, {14,0,-1}
    };
    for (size_t i = 0; i < sizeof(remote) / sizeof(remote[0]); i++)
        board[remote[i][0]][remote[i][1]] = remote[i][2];
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofWorkerCount = 1;
    profile.proofParallelNodeBudget = 0;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        0x5353455353494f4eULL, FC_RANDOM_EVALUATION, 0, 0, &result));
    assert(result.x == 7 && result.y == 7);
    assert(result.proofCertificateVerified);
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.allocations == 1);
    assert(diagnostics.proofSessionQueries >= 1);
    assert(diagnostics.incrementalInitializations >= 1);
    assert(diagnostics.incrementalMakes > 0);
    assert(diagnostics.incrementalMakes == diagnostics.incrementalUnmakes);
    assert(diagnostics.mostProvingExpansions > 0);
    assert(diagnostics.proofGraphNodes > 0);
    assert(diagnostics.proofGraphEdges > 0);
    assert(diagnostics.proofGraphArenaExhaustions == 0);
    assert(diagnostics.relevanceAllLegalReplies ==
           diagnostics.relevanceReplyCandidates +
           diagnostics.relevanceVerifiedOmissions);
    assert(diagnostics.relevanceVerifiedOmissions > 0);
    assert(diagnostics.relevanceUnresolved == 0);

    FCProofResult first;
    FCProofResult second;
    fc_proof_diagnostics_reset();
    assert(fc_test_candidate_proof_session_reuse(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &first, &second));
    diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.allocations == 1);
    assert(diagnostics.incrementalInitializations == 1);
    assert(diagnostics.proofSessionQueries == 2);
    assert(diagnostics.proofSessionHits > 0);
}

static void test_quiet_portfolio_is_active_but_does_not_heuristically_override(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[4][7] = board[10][7] = 1;
    board[7][4] = board[7][10] = 1;
    board[5][5] = board[9][9] = -1;
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    profile.proofMaxDepth = 2;
    profile.proofNodeBudget = 200;
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        0x5155494554524f4fULL, FC_RANDOM_EVALUATION, 7, 7, &result));
    assert(result.quietRootsExamined > 0);
    assert(!result.quietThreatSelected);
    assert(result.overrideReason != FC_OVERRIDE_QUIET_PROVEN_ATTACK);
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.quietEligibleDecisions == 1);
    assert(diagnostics.quietRootsExamined ==
           (uint64_t)result.quietRootsExamined);

    FCAIProfile noQuiet = profile;
    noQuiet.proofQuietRootLimit = 0;
    FCAnalysisResult noQuietResult;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &noQuiet,
        0x4e4f515549455400ULL, FC_RANDOM_EVALUATION,
        7, 7, &noQuietResult));
    diagnostics = fc_proof_diagnostics_get();
    assert(noQuietResult.quietRootsExamined == 0);
    assert(diagnostics.quietEligibleDecisions == 0);
    assert(diagnostics.quietRootsExamined == 0);

    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    immediate[7][4] = immediate[7][5] =
        immediate[7][6] = immediate[7][7] = 1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false, &profile,
        17, FC_RANDOM_EVALUATION, 0, 0, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(result.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN);
    assert(result.quietRootsExamined == 0);
    assert(diagnostics.quietEligibleDecisions == 0);
}

static void test_randomized_incremental_long_sequences(void)
{
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        int reference[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
        FCIncrementalPosition position;
        assert(fc_incremental_position_init(
            &position, (const int (*)[FC_BOARD_SIZE])reference,
            forbidden != 0));
        FCPoint history[64];
        int historyCount = 0;
        FCRandom random;
        fc_random_seed(&random, UINT64_C(0x494e4352454d454e) +
                                (uint64_t)forbidden);
        for (int ply = 0; ply < 48; ply++) {
            int side = (ply & 1) == 0 ? 1 : -1;
            int start = (int)(fc_random_next(&random) %
                              (FC_BOARD_SIZE * FC_BOARD_SIZE));
            int selected = -1;
            for (int offset = 0;
                 offset < FC_BOARD_SIZE * FC_BOARD_SIZE; offset++) {
                int index = (start + offset) %
                            (FC_BOARD_SIZE * FC_BOARD_SIZE);
                int x = index / FC_BOARD_SIZE;
                int y = index % FC_BOARD_SIZE;
                if (fc_is_legal_move(
                        (const int (*)[FC_BOARD_SIZE])reference,
                        x, y, side, forbidden != 0)) {
                    selected = index;
                    break;
                }
            }
            if (selected < 0) break;
            int x = selected / FC_BOARD_SIZE;
            int y = selected % FC_BOARD_SIZE;
            assert(fc_make_move(reference, x, y, side, forbidden != 0));
            assert(fc_incremental_position_make(&position, x, y, side));
            history[historyCount++] = (FCPoint){x, y};
            assert(memcmp(reference, position.board,
                          sizeof(reference)) == 0);
            assert_incremental_matches_reference(
                &position, -side, FC_PROOF_SEARCH_VCT, 12);
            assert(fc_board_winner(
                (const int (*)[FC_BOARD_SIZE])reference) ==
                fc_board_winner(
                (const int (*)[FC_BOARD_SIZE])position.board));
            if ((ply & 7) == 7) {
                FCThreat expected[FC_MAX_THREATS];
                FCThreat actual[FC_MAX_THREATS];
                bool expectedOverflow = false;
                bool actualOverflow = false;
                int expectedCount = fc_enumerate_threats(
                    (const int (*)[FC_BOARD_SIZE])reference, -side,
                    forbidden != 0, FC_PROOF_SEARCH_VCT,
                    expected, FC_MAX_THREATS, &expectedOverflow);
                int actualCount = fc_incremental_enumerate_threats(
                    &position, -side, FC_PROOF_SEARCH_VCT,
                    actual, FC_MAX_THREATS, &actualOverflow);
                assert(expectedCount == actualCount);
                assert(expectedOverflow == actualOverflow);
                assert(memcmp(expected, actual,
                              (size_t)expectedCount * sizeof(FCThreat)) == 0);
            }
        }
        while (historyCount > 0) {
            FCPoint move = history[--historyCount];
            fc_unmake_move(reference, move.x, move.y);
            assert(fc_incremental_position_unmake(&position));
            assert(memcmp(reference, position.board,
                          sizeof(reference)) == 0);
        }
        int empty[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
        assert(memcmp(empty, position.board, sizeof(empty)) == 0);
        assert(position.stoneCount == 0 && position.deltaCount == 0);
        uint16_t emptyRevisions[4][FC_BOARD_SIZE * 2 - 1] = {{0}};
        assert(memcmp(emptyRevisions, position.lineRevision,
                      sizeof(emptyRevisions)) == 0);
    }
}

static void test_dense_incremental_reply_set_equivalence(void)
{
    const FCPoint emptyPoints[] = {
        {1,1}, {3,4}, {5,9}, {7,7},
        {9,5}, {11,10}, {13,2}, {14,14}
    };
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        for (int x = 0; x < FC_BOARD_SIZE; x++)
            for (int y = 0; y < FC_BOARD_SIZE; y++)
                board[x][y] = ((x * 7 + y * 11) % 5) < 2 ? 1 : -1;
        for (size_t i = 0;
             i < sizeof(emptyPoints) / sizeof(emptyPoints[0]); i++)
            board[emptyPoints[i].x][emptyPoints[i].y] = 0;
        FCIncrementalPosition position;
        assert(fc_incremental_position_init(
            &position, (const int (*)[FC_BOARD_SIZE])board,
            forbidden != 0));
        for (int pass = 0; pass < 2; pass++) {
            FCPoint expected[FC_BOARD_SIZE * FC_BOARD_SIZE];
            FCPoint actual[FC_BOARD_SIZE * FC_BOARD_SIZE];
            int expectedCount = fc_reference_generate_refutations(
                (const int (*)[FC_BOARD_SIZE])board, 1,
                forbidden != 0, FC_PROOF_SEARCH_VCF,
                expected, FC_BOARD_SIZE * FC_BOARD_SIZE);
            int actualCount = fc_incremental_generate_refutations(
                &position, 1, FC_PROOF_SEARCH_VCF,
                actual, FC_BOARD_SIZE * FC_BOARD_SIZE);
            assert(expectedCount == actualCount);
            assert(memcmp(expected, actual,
                          (size_t)expectedCount * sizeof(FCPoint)) == 0);
            if (pass == 0) {
                assert(fc_make_move(board, 7, 7, -1,
                                    forbidden != 0));
                assert(fc_incremental_position_make(
                    &position, 7, 7, -1));
            }
        }
        fc_unmake_move(board, 7, 7);
        assert(fc_incremental_position_unmake(&position));
        assert(memcmp(board, position.board, sizeof(board)) == 0);
    }
}

static void test_opening_book_reproducibility_and_variety(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    int x = -1, y = -1, id = -1, ply = -1;
    assert(strcmp(fc_opening_book_version(),
                  "gomocup-2024-2025-freestyle15-v2") == 0);
    assert(fc_opening_book_lookup(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, 123,
        &x, &y, &id, &ply));
    int firstX = x, firstY = y, firstId = id;
    assert(ply == 0);
    assert(fc_opening_book_lookup(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, 123,
        &x, &y, &id, &ply));
    assert(x == firstX && y == firstY && id == firstId && ply == 0);
    assert(!fc_opening_book_lookup(
        (const int (*)[FC_BOARD_SIZE])board, 1, true, 123,
        &x, &y, &id, &ply));

    bool selected[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{false}};
    int unique = 0;
    for (uint64_t seed = 1; seed <= 64; seed++) {
        assert(fc_opening_book_lookup(
            (const int (*)[FC_BOARD_SIZE])board, 1, false, seed,
            &x, &y, &id, &ply));
        assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                x, y, 1, false));
        if (!selected[x][y]) {
            selected[x][y] = true;
            unique++;
        }
    }
    assert(unique >= 4);
}

static void test_threat_enumeration_and_proof_certificate(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 5; y <= 8; y++) board[7][y] = 1;
    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int count = fc_enumerate_threats(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, threats, FC_MAX_THREATS, &overflow);
    assert(!overflow);
    assert(count >= 2);
    assert(threats[0].severity == FC_THREAT_FIVE);

    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    FCProofResult proof;
    assert(fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 1000, 0, 1024, &proof));
    assert(proof.status == FC_PROOF_PROVEN_WIN);
    assert(proof.proofNumber == 0 && proof.disproofNumber > 0);
    assert(proof.distance == 1);
    assert(proof.certificateVerified);
    assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                           1, false, &proof));
    assert(memcmp(before, board, sizeof(board)) == 0);

    proof.certificate[0].x++;
    assert(!fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                            1, false, &proof));
}

static void test_proof_determinism_and_budget_unknown(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[7][4] = -1;
    board[7][5] = 1;
    board[7][6] = 1;
    board[7][7] = 1;
    FCProofResult first;
    FCProofResult second;
    (void)fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 2048, &first);
    srand(1234);
    for (int i = 0; i < 50; i++) (void)rand();
    (void)fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 2048, &second);
    assert(first.status == second.status);
    assert(first.x == second.x && first.y == second.y);
    assert(first.nodes == second.nodes);
    assert(first.certificateId == second.certificateId);

    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    FCProofResult limited;
    (void)fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 1, 0, 64, &limited);
    assert(limited.status == FC_PROOF_UNKNOWN);
    assert(limited.proofNumber > 0 && limited.disproofNumber > 0);
    assert(limited.budgetExhausted);
    assert(memcmp(before, board, sizeof(board)) == 0);
}

static void transform_board(const int source[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int transform,
                            int destination[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    memset(destination, 0,
           sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            int tx = 0, ty = 0;
            fc_transform_point(transform, x, y, &tx, &ty);
            destination[tx][ty] = source[x][y];
        }
    }
}

static void test_opponent_guard_distance_matrix_symmetry_and_rules(void)
{
    static const int opening20[][3] = {
        {9,10,1},{6,8,-1},{5,7,1},{8,4,-1},{9,4,1},{8,7,-1},
        {8,6,1},{7,8,-1},{9,6,1},{9,8,-1},{5,8,1},{7,6,-1},
        {10,9,1},{7,7,-1},{7,5,1},{9,7,-1},{10,7,1},{5,4,-1},
        {6,5,1},{7,9,-1},{7,10,1},{8,8,-1},{10,8,1},{6,10,-1},
    };
    static const struct {
        int ply;
        int x;
        int y;
        int distance;
    } cases[] = {
        {16,10,7,9}, {18,6,5,7}, {20,7,10,5}, {22,10,8,3},
    };
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.parallelProofEnabled = false;
    profile.proofWorkerCount = 1;
    profile.opponentGuardVCTNodeBudget = 1;
    profile.opponentGuardVCTTimeBudgetMs = 1;
    int applicable = 0;
    for (size_t caseIndex = 0;
         caseIndex < sizeof(cases) / sizeof(cases[0]); caseIndex++) {
        int source[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
        for (int ply = 0; ply < cases[caseIndex].ply; ply++)
            source[opening20[ply][0]][opening20[ply][1]] =
                opening20[ply][2];
        for (int forbidden = 0; forbidden <= 1; forbidden++) {
            for (int transform = 0; transform < 8; transform++) {
                int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
                transform_board(source, transform, board);
                int x = 0;
                int y = 0;
                fc_transform_point(transform, cases[caseIndex].x,
                                   cases[caseIndex].y, &x, &y);
                if (!fc_is_legal_move(
                        (const int (*)[FC_BOARD_SIZE])board,
                        x, y, 1, forbidden != 0)) continue;
                applicable++;
                int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
                memcpy(before, board, sizeof(before));
                FCOpponentGuardAudit audit;
                assert(fc_audit_opponent_after_move(
                    (const int (*)[FC_BOARD_SIZE])board, 1,
                    forbidden != 0, &profile, x, y, &audit));
                if (!forbidden) {
                    assert(audit.vcf.status == FC_PROOF_PROVEN_WIN);
                    if (transform == 0)
                        assert(audit.vcf.distance ==
                               cases[caseIndex].distance);
                    assert(audit.vcf.certificateVerified);
                } else if (audit.vcf.status == FC_PROOF_PROVEN_WIN) {
                    assert(audit.vcf.certificateVerified);
                } else {
                    assert(audit.vcf.status ==
                               FC_PROOF_NO_FORCED_WIN_IN_SCOPE ||
                           audit.vcf.status == FC_PROOF_UNKNOWN);
                }
                assert(audit.boardRestored);
                assert(memcmp(before, board, sizeof(before)) == 0);

                FCAIProfile micro =
                    fc_profile_five_star_early_micro_vcf_candidate();
                micro.earlyVCFSentinelPolicy =
                    FC_EARLY_VCF_POLICY_FIXED;
                micro.earlyVCFBaseDepth = 7;
                micro.earlyVCFMaxDepth = 7;
                micro.earlyVCFNodeBudget = 16000;
#if defined(FC_TEST_SANITIZER_SLOW)
                /* Sanitizer instrumentation is intentionally much slower;
                 * this matrix checks proof/replay/integrity semantics, while
                 * the ordinary build separately enforces the frozen 80 ms
                 * production budget. */
                micro.earlyVCFTimeBudgetMs = 800;
#else
                micro.earlyVCFTimeBudgetMs = 80;
#endif
                FCOpponentGuardAudit microAudit;
                int effectiveDepth = 0;
                bool escalated = false;
                assert(fc_audit_opponent_micro_vcf_after_move(
                    (const int (*)[FC_BOARD_SIZE])board, 1,
                    forbidden != 0, &micro, x, y, &microAudit,
                    &effectiveDepth, &escalated));
                assert(effectiveDepth == 7);
                assert(!escalated);
                if (!forbidden) {
                    assert(microAudit.vcf.status == FC_PROOF_PROVEN_WIN);
                    assert(microAudit.vcf.certificateVerified);
                } else if (microAudit.vcf.status == FC_PROOF_PROVEN_WIN) {
                    assert(microAudit.vcf.certificateVerified);
                } else {
                    assert(microAudit.vcf.status ==
                               FC_PROOF_NO_FORCED_WIN_IN_SCOPE ||
                           microAudit.vcf.status == FC_PROOF_UNKNOWN);
                }
                assert(microAudit.boardRestored);
                assert(memcmp(before, board, sizeof(before)) == 0);
            }
        }
    }
    assert(applicable >= 32);

    static const int separation[][3] = {
        {10,2,1},{9,2,-1},{9,5,1},{10,4,-1},{12,3,1},{11,5,-1},
        {12,6,1},{8,3,-1},{12,4,1},{12,5,-1},{11,3,1},{9,1,-1},
        {11,7,1},{13,5,-1},{10,3,1},{9,3,-1},{13,3,1},{14,3,-1},
    };
    int source[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(separation) / sizeof(separation[0]); i++)
        source[separation[i][0]][separation[i][1]] = separation[i][2];
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(source, transform, board);
        int losingX = 0, losingY = 0, defenseX = 0, defenseY = 0;
        fc_transform_point(transform, 10, 6, &losingX, &losingY);
        fc_transform_point(transform, 7, 2, &defenseX, &defenseY);
        FCOpponentGuardAudit loss;
        FCOpponentGuardAudit defense;
        assert(fc_audit_opponent_after_move(
            (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
            losingX, losingY, &loss));
        assert(loss.vcf.status == FC_PROOF_PROVEN_WIN);
        if (transform == 0) assert(loss.vcf.distance == 9);
        assert(loss.vcf.certificateVerified);
        assert(fc_audit_opponent_after_move(
            (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
            defenseX, defenseY, &defense));
        assert(defense.vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
        int replay[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(replay, board, sizeof(replay));
        assert(fc_make_move(replay, defenseX, defenseY, 1, false));
        assert(fc_verify_scoped_disproof(
            (const int (*)[FC_BOARD_SIZE])replay,
            -1, false, &defense.vcf));
    }
}

static void test_opponent_guard_completed_class_ordering(void)
{
    FCOpponentGuardAudit loss3 = {0};
    loss3.completedClass = FC_GUARD_CLASS_VERIFIED_LOSS;
    loss3.completedSearchClass = FC_PROOF_SEARCH_VCF;
    loss3.vcf.distance = 3;
    FCOpponentGuardAudit loss7 = loss3;
    loss7.vcf.distance = 7;
    FCOpponentGuardAudit unknown = {0};
    unknown.completedClass = FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN;
    FCOpponentGuardAudit disproof = {0};
    disproof.completedClass = FC_GUARD_CLASS_SCOPED_DISPROOF;
    disproof.completedSearchClass = FC_PROOF_SEARCH_VCF;
    FCOpponentGuardAudit own = {0};
    own.completedClass = FC_GUARD_CLASS_OWN_VERIFIED_WIN;
    assert(fc_test_opponent_guard_audit_better(&loss7, &loss3));
    assert(fc_test_opponent_guard_audit_better(&unknown, &loss7));
    assert(fc_test_opponent_guard_audit_better(&disproof, &unknown));
    assert(fc_test_opponent_guard_audit_better(&own, &disproof));
    assert(!fc_test_opponent_guard_audit_better(&loss3, &loss7));
}

static const FCThreat *find_threat(const FCThreat *threats,
                                   int count,
                                   int x,
                                   int y)
{
    for (int i = 0; i < count; i++) {
        if (threats[i].gain.x == x && threats[i].gain.y == y)
            return &threats[i];
    }
    return NULL;
}

static bool threat_mask_has(
    const uint64_t mask[FC_POSITION_BITSET_WORDS],
    int x,
    int y)
{
    int index = x * FC_BOARD_SIZE + y;
    return (mask[index >> 6] &
            (UINT64_C(1) << (index & 63))) != 0;
}

static int threat_mask_count(
    const uint64_t mask[FC_POSITION_BITSET_WORDS])
{
    int count = 0;
    for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
        uint64_t value = mask[word];
        while (value != 0) {
            value &= value - 1;
            count++;
        }
    }
    return count;
}

static void assert_pattern_under_symmetry(
    const int source[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int gainX,
    int gainY,
    int expectedSeverity,
    int expectedCosts)
{
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(source, transform, board);
        int tx = 0, ty = 0;
        fc_transform_point(transform, gainX, gainY, &tx, &ty);
        FCThreat threats[FC_MAX_THREATS];
        bool overflow = false;
        int count = fc_enumerate_threats(
            (const int (*)[FC_BOARD_SIZE])board, 1, false,
            FC_PROOF_SEARCH_VCT, threats, FC_MAX_THREATS, &overflow);
        assert(!overflow);
        const FCThreat *threat = find_threat(threats, count, tx, ty);
        assert(threat != NULL);
        assert(threat->severity == expectedSeverity);
        assert(threat->costCount == expectedCosts);
        assert(threat_mask_has(threat->dependencyMask, tx, ty));
        assert(threat_mask_has(threat->certificateZoneMask, tx, ty));
        assert(threat_mask_count(threat->fiveWindowMask) >= 5);
        assert(threat_mask_count(threat->legalityDependencyMask) == 0);
        for (int i = 0; i < threat->costCount; i++)
            assert(threat_mask_has(threat->dependencyMask,
                                   threat->costs[i].x,
                                   threat->costs[i].y));
        for (int i = 0; i < threat->restCount; i++)
            assert(threat_mask_has(threat->dependencyMask,
                                   threat->rests[i].x,
                                   threat->rests[i].y));
    }
}

static void test_table_driven_threat_patterns_all_symmetries(void)
{
    int straight[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    straight[6][7] = straight[7][7] = straight[8][7] = 1;
    assert_pattern_under_symmetry(straight, 5, 7,
                                  FC_THREAT_OPEN_FOUR, 2);

    int broken[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    broken[5][7] = broken[6][7] = broken[8][7] = 1;
    assert_pattern_under_symmetry(broken, 7, 7,
                                  FC_THREAT_OPEN_FOUR, 2);

    int blocked[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    blocked[4][7] = -1;
    blocked[5][7] = blocked[6][7] = blocked[7][7] = 1;
    assert_pattern_under_symmetry(blocked, 8, 7,
                                  FC_THREAT_FOUR, 1);

    int edge[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    edge[0][2] = edge[1][2] = edge[2][2] = 1;
    assert_pattern_under_symmetry(edge, 3, 2,
                                  FC_THREAT_FOUR, 1);

    FCThreat tiny[1];
    bool overflow = false;
    int stored = fc_enumerate_threats(
        (const int (*)[FC_BOARD_SIZE])straight, 1, false,
        FC_PROOF_SEARCH_VCF, tiny, 1, &overflow);
    assert(stored == 1);
    assert(overflow);
}

static void test_vcf_and_node_multi_defense_certificate(void)
{
    int source[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    source[6][7] = source[7][7] = source[8][7] = 1;
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(source, transform, board);
        int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(before, board, sizeof(board));
        FCProofResult proof;
        assert(fc_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])board, 1, false,
            FC_PROOF_SEARCH_VCF, 7, 10000, 0, 4096, &proof));
        assert(proof.status == FC_PROOF_PROVEN_WIN);
        assert(proof.distance == 3);
        assert(proof.certificateVerified);
        assert(proof.certificateNodeCount >= 5);
        bool hasReplayableOmissionCoverage = false;
        for (int nodeIndex = 0;
             nodeIndex < proof.certificateNodeCount; nodeIndex++) {
            const FCProofNode *node = &proof.certificate[nodeIndex];
            for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
                if (node->verifiedOmissionMask[word] != 0) {
                    hasReplayableOmissionCoverage = true;
                    break;
                }
            }
        }
        assert(hasReplayableOmissionCoverage);
        assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                               1, false, &proof));
        assert(memcmp(before, board, sizeof(board)) == 0);

        FCProofResult missingBranch = proof;
        missingBranch.certificate[1].parent = -7;
        /* Recompute is intentionally impossible through the public API;
         * changing the DAG must invalidate its certificate identifier. */
        assert(!fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                                1, false, &missingBranch));

        FCProofResult corruptedCoverage = proof;
        corruptedCoverage.certificate[0].verifiedOmissionMask[0] ^=
            UINT64_C(1);
        assert(!fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                                1, false, &corruptedCoverage));
    }

    int refuted[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    refuted[5][7] = refuted[6][7] = refuted[7][7] = 1;
    refuted[4][7] = -1;
    refuted[9][7] = -1;
    FCProofResult noWin;
    assert(!fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 4096, &noWin));
    assert(noWin.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(noWin.proofNumber > 0 && noWin.disproofNumber == 0);
    assert(!fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, true,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 4096, &noWin));
    assert(noWin.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(noWin.proofNumber > 0 && noWin.disproofNumber == 0);

    int counterWin[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    counterWin[5][7] = counterWin[6][7] = counterWin[7][7] = 1;
    counterWin[4][4] = counterWin[5][4] = counterWin[6][4] = -1;
    counterWin[7][4] = -1;
    assert(!fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])counterWin, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 4096, &noWin));
    assert(noWin.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
}

static void test_candidate_dfpn_proof_disproof_thresholds_and_collisions(void)
{
    uint64_t shallowProof = 0, shallowDisproof = 0;
    uint64_t wideProof = 0, wideDisproof = 0;
    uint64_t deepProof = 0, deepDisproof = 0;
    fc_dfpn_frontier_estimate(9, 9, 1,
                              &shallowProof, &shallowDisproof);
    fc_dfpn_frontier_estimate(9, 9, 64,
                              &wideProof, &wideDisproof);
    fc_dfpn_frontier_estimate(9, 1, 1,
                              &deepProof, &deepDisproof);
    assert(shallowProof == 1 && shallowDisproof == 1);
    assert(wideProof == 64 && wideDisproof == 1);
    assert(deepProof == 1 && deepDisproof == 3);

    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofMaxDepth = 9;
    profile.proofNodeBudget = 18000;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;

    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    immediate[7][4] = immediate[7][5] =
        immediate[7][6] = immediate[7][7] = 1;
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        for (int transform = 0; transform < 8; transform++) {
            int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
            transform_board(immediate, transform, board);
            FCProofResult proof;
            profile.proofTranspositionCapacity = 1;
            fc_proof_diagnostics_reset();
            assert(fc_prove_forced_win_candidate_session(
                (const int (*)[FC_BOARD_SIZE])board, 1, forbidden != 0,
                FC_PROOF_SEARCH_VCF, &profile, &proof));
            assert(proof.status == FC_PROOF_PROVEN_WIN);
            assert(proof.proofNumber == 0 &&
                   proof.disproofNumber > 0);
            assert(proof.certificateVerified);
            assert(fc_verify_proof(
                (const int (*)[FC_BOARD_SIZE])board, 1,
                forbidden != 0, &proof));
            FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
            assert(diagnostics.mostProvingExpansions > 0);
            assert(diagnostics.proofGraphArenaExhaustions == 0);
            int independentRootContinuations = 0;
            for (int nodeIndex = 0;
                 nodeIndex < proof.certificateNodeCount; nodeIndex++) {
                if (proof.certificate[nodeIndex].parent == -1)
                    independentRootContinuations++;
            }
            assert(independentRootContinuations >= 2);
            assert(diagnostics.iteratedRelatedZoneIntersections > 0);
            assert(diagnostics.iteratedRelatedZoneContinuations >= 2);
            assert(diagnostics.iteratedRelatedZonePointsRemoved > 0);
        }
    }

    int refuted[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    refuted[5][7] = refuted[6][7] = refuted[7][7] = 1;
    refuted[4][7] = refuted[9][7] = -1;
    profile.proofTranspositionCapacity = 4096;
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        FCProofResult disproof;
        fc_proof_diagnostics_reset();
        assert(!fc_prove_forced_win_candidate_session(
            (const int (*)[FC_BOARD_SIZE])refuted, 1, forbidden != 0,
            FC_PROOF_SEARCH_VCF, &profile, &disproof));
        assert(disproof.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
        assert(disproof.proofNumber > 0 && disproof.disproofNumber == 0);
        assert(fc_verify_scoped_disproof(
            (const int (*)[FC_BOARD_SIZE])refuted, 1,
            forbidden != 0, &disproof));
        FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
        assert(diagnostics.completedScopeDisproofs >= 1);
        assert(diagnostics.relevanceUnresolved == 0);
        assert(diagnostics.relevanceAllLegalReplies ==
               diagnostics.relevanceReplyCandidates +
               diagnostics.relevanceVerifiedOmissions);
        assert(diagnostics.relevanceFallbacks == 0);
        assert(diagnostics.relevanceFallbackReplies == 0);
    }

    int crossing[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    crossing[5][7] = crossing[6][7] = 1;
    crossing[7][5] = crossing[7][6] = 1;
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        FCProofResult vct;
        fc_proof_diagnostics_reset();
        assert(fc_prove_forced_win_candidate_session(
            (const int (*)[FC_BOARD_SIZE])crossing, 1, forbidden != 0,
            FC_PROOF_SEARCH_VCT, &profile, &vct));
        assert(vct.status == FC_PROOF_PROVEN_WIN);
        assert(vct.distance == 5);
        assert(vct.certificateVerified);
        FCProofDiagnostics vctDiagnostics = fc_proof_diagnostics_get();
        assert(vctDiagnostics.relevanceZonePoints > 0);
        assert(vctDiagnostics.relevanceAllLegalReplies ==
               vctDiagnostics.relevanceReplyCandidates +
               vctDiagnostics.relevanceVerifiedOmissions);
        assert(vctDiagnostics.relevanceFallbacks == 0);
        assert(vctDiagnostics.relevanceFallbackReplies == 0);
    }

    profile.proofNodeBudget = 1;
    FCProofResult unknown;
    fc_proof_diagnostics_reset();
    assert(!fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])crossing, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &unknown));
    assert(unknown.status == FC_PROOF_UNKNOWN);
    assert(unknown.proofNumber > 0 && unknown.disproofNumber > 0);
    assert(unknown.budgetExhausted);

    profile.proofNodeBudget = 18000;
    profile.proofGraphNodeCapacity = 1;
    profile.proofGraphEdgeCapacity = 1;
    fc_proof_diagnostics_reset();
    assert(!fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])crossing, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &unknown));
    assert(unknown.status == FC_PROOF_UNKNOWN);
    assert(unknown.budgetExhausted);
    FCProofDiagnostics exhausted = fc_proof_diagnostics_get();
    assert(exhausted.proofGraphArenaExhaustions > 0);

    /* A new generation/session must not inherit the exhausted graph. */
    profile.proofGraphNodeCapacity = 0;
    profile.proofGraphEdgeCapacity = 0;
    profile.proofTranspositionCapacity = 4096;
    FCProofResult fresh;
    fc_proof_diagnostics_reset();
    assert(fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false,
        FC_PROOF_SEARCH_VCF, &profile, &fresh));
    assert(fresh.status == FC_PROOF_PROVEN_WIN);
    assert(fresh.certificateVerified);
    FCProofDiagnostics freshDiagnostics = fc_proof_diagnostics_get();
    assert(freshDiagnostics.proofGraphArenaExhaustions == 0);
}

static bool point_list_contains(const FCPoint *points,
                                int count,
                                int x,
                                int y)
{
    for (int i = 0; i < count; i++) {
        if (points[i].x == x && points[i].y == y) return true;
    }
    return false;
}

static void test_relevance_and_dependency_adversarial_cases(void)
{
    int remoteCounters[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    remoteCounters[6][7] = remoteCounters[7][7] =
        remoteCounters[8][7] = 1;
    remoteCounters[2][12] = remoteCounters[3][12] =
        remoteCounters[4][12] = remoteCounters[5][12] = -1;
    remoteCounters[3][2] = remoteCounters[4][2] =
        remoteCounters[5][2] = -1;
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(remoteCounters, transform, board);
        int counterWinX = 0, counterWinY = 0;
        int counterForceX = 0, counterForceY = 0;
        fc_transform_point(transform, 6, 12,
                           &counterWinX, &counterWinY);
        fc_transform_point(transform, 6, 2,
                           &counterForceX, &counterForceY);
        FCPoint replies[FC_BOARD_SIZE * FC_BOARD_SIZE];
        int count = fc_reference_generate_refutations(
            (const int (*)[FC_BOARD_SIZE])board, 1, false,
            FC_PROOF_SEARCH_VCT, replies,
            FC_BOARD_SIZE * FC_BOARD_SIZE);
        assert(point_list_contains(replies, count,
                                   counterWinX, counterWinY));
        assert(point_list_contains(replies, count,
                                   counterForceX, counterForceY));
    }

    FCThreat left = {0};
    FCThreat right = {0};
    left.gain = (FCPoint){7, 7};
    left.rests[0] = (FCPoint){8, 8};
    left.restCount = 1;
    right.gain = (FCPoint){8, 8};
    assert(fc_threats_dependency_compatible(&left, &right));
    right.costs[0] = left.gain;
    right.costCount = 1;
    assert(!fc_threats_dependency_compatible(&left, &right));
    right.costs[0] = left.rests[0];
    assert(!fc_threats_dependency_compatible(&left, &right));
    right.costCount = 0;
    right.gain = left.gain;
    assert(!fc_threats_dependency_compatible(&left, &right));

    int fallbackBoard[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    fallbackBoard[7][7] = 1;
    fallbackBoard[7][8] = -1;
    FCIncrementalPosition position;
    assert(fc_incremental_position_init(
        &position, (const int (*)[FC_BOARD_SIZE])fallbackBoard, true));
    int legalDefenderReplies = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (fc_is_legal_move(
                    (const int (*)[FC_BOARD_SIZE])fallbackBoard,
                    x, y, -1, true)) legalDefenderReplies++;
        }
    }
    FCPoint fallbackReplies[FC_BOARD_SIZE * FC_BOARD_SIZE];
    fc_proof_diagnostics_reset();
    int fallbackCount = fc_incremental_generate_refutations(
        &position, 1, FC_PROOF_SEARCH_VCT, fallbackReplies,
        FC_BOARD_SIZE * FC_BOARD_SIZE);
    FCProofDiagnostics fallbackDiagnostics = fc_proof_diagnostics_get();
    assert(fallbackCount == legalDefenderReplies);
    assert(fallbackDiagnostics.relevanceFallbacks == 1);
    assert(fallbackDiagnostics.relevanceFallbackReplies ==
           (uint64_t)legalDefenderReplies);
    assert(fallbackDiagnostics.relevanceReplyCandidates ==
           (uint64_t)legalDefenderReplies);
    assert(fallbackDiagnostics.relevanceVerifiedOmissions == 0);
}

static void test_dependency_dag_proposes_verified_search_order(void)
{
    const int stones[][3] = {
        {5,7,1}, {5,8,-1}, {7,6,1}, {7,5,-1}, {8,5,1},
        {7,7,-1}, {8,7,1}, {8,6,-1}, {9,5,1}, {9,8,-1}
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofMaxDepth = 8;
    profile.proofNodeBudget = 400;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    FCProofResult result;
    fc_proof_diagnostics_reset();
    (void)fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &result);
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.dependencyCombinations > 0);
    assert(diagnostics.dependencyChainsProposed > 0);
    assert(diagnostics.dependencyMaximumDepth >= 2);
    assert(result.status == FC_PROOF_PROVEN_WIN ||
           result.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE ||
           result.status == FC_PROOF_UNKNOWN);

    fc_proof_diagnostics_reset();
    (void)fc_prove_forced_win_candidate_session(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCF, &profile, &result);
    diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.dependencyCombinations == 0);
    assert(diagnostics.dependencyChainsProposed == 0);
}

static void test_candidate_stage_order_and_obligation_skips(void)
{
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofWorkerCount = 1;
    profile.proofParallelNodeBudget = 0;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    profile.decisionTimeBudgetMs = 0;
    const int naturalStones[][3] = {
        {5,7,1}, {5,8,-1}, {7,6,1}, {7,5,-1}, {8,5,1},
        {7,7,-1}, {8,7,1}, {8,6,-1}, {9,5,1}, {9,8,-1}
    };
    int natural[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0;
         i < sizeof(naturalStones) / sizeof(naturalStones[0]); i++)
        natural[naturalStones[i][0]][naturalStones[i][1]] =
            naturalStones[i][2];
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])natural, 1, false, &profile,
        UINT64_C(0x53544147454f5244), FC_RANDOM_EVALUATION,
        9, 4, &result));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.stageVCFQueries == 1);
    assert(diagnostics.stageVCTQueries == 1);
    assert(diagnostics.stageQuietQueries == 1);
    assert(diagnostics.quietRootsExamined > 0);

    int immediate[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    immediate[7][4] = immediate[7][5] =
        immediate[7][6] = immediate[7][7] = 1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])immediate, 1, false, &profile,
        1, FC_RANDOM_EVALUATION, 7, 8, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.stageImmediateDecisions == 1);
    assert(diagnostics.stageVCFQueries == 0 &&
           diagnostics.stageVCTQueries == 0 &&
           diagnostics.stageQuietQueries == 0);

    int mustDefend[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    mustDefend[7][4] = 1;
    mustDefend[7][5] = mustDefend[7][6] =
        mustDefend[7][7] = mustDefend[7][8] = -1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])mustDefend, 1, false, &profile,
        2, FC_RANDOM_EVALUATION, 7, 9, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.stageMandatoryDefenseDecisions == 1);
    assert(diagnostics.stageVCFQueries == 0 &&
           diagnostics.stageVCTQueries == 0 &&
           diagnostics.stageQuietQueries == 0);

    FCAIProfile guardProfile =
        fc_profile_five_star_early_micro_vcf_candidate();
    guardProfile.parallelProofEnabled = false;
    guardProfile.proofWorkerCount = 1;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])mustDefend, 1, false,
        &guardProfile, 2, FC_RANDOM_EVALUATION, 7, 9, &result));
    diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.stageMandatoryDefenseDecisions == 1);
    assert(diagnostics.opponentGuardVCFQueries >= 1);
    assert(result.opponentGuardEligible);
    assert(result.opponentGuardAuditedCount >= 1);
    assert(result.tacticalClass == FC_TACTICAL_MUST_DEFEND);
}

static void test_crossing_duplicate_and_forbidden_symmetries(void)
{
    int crossing[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    crossing[5][7] = crossing[6][7] = 1;
    crossing[7][5] = crossing[7][6] = 1;
    /* The center is an explicit crossing/double-three fixture.  The current
     * reference oracle intentionally preserves its historical continuation
     * definition (so it accepts this exact double-three shape); the cache
     * must nevertheless return exactly the same result in both rule modes. */
    bool crossingFreeLegal = fc_is_legal_move(
        (const int (*)[FC_BOARD_SIZE])crossing, 7, 7, 1, false);
    bool crossingForbiddenLegal = fc_is_legal_move(
        (const int (*)[FC_BOARD_SIZE])crossing, 7, 7, 1, true);
    assert(crossingFreeLegal);
    FCIncrementalPosition crossingFree;
    FCIncrementalPosition crossingForbidden;
    assert(fc_incremental_position_init(
        &crossingFree, (const int (*)[FC_BOARD_SIZE])crossing, false));
    assert(fc_incremental_position_init(
        &crossingForbidden, (const int (*)[FC_BOARD_SIZE])crossing, true));
    assert(fc_incremental_validate_legality_cache(&crossingFree));
    assert(fc_incremental_validate_legality_cache(&crossingForbidden));
    assert(fc_incremental_is_legal_move(&crossingFree, 7, 7, 1) ==
           crossingFreeLegal);
    assert(fc_incremental_is_legal_move(&crossingForbidden, 7, 7, 1) ==
           crossingForbiddenLegal);
    int crossingFour[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    crossingFour[4][7] = crossingFour[5][7] = crossingFour[6][7] = 1;
    crossingFour[7][4] = crossingFour[7][5] = crossingFour[7][6] = 1;
    assert(!fc_is_legal_move(
        (const int (*)[FC_BOARD_SIZE])crossingFour, 7, 7, 1, true));
    FCIncrementalPosition crossingFourForbidden;
    assert(fc_incremental_position_init(
        &crossingFourForbidden,
        (const int (*)[FC_BOARD_SIZE])crossingFour, true));
    assert(fc_incremental_validate_legality_cache(&crossingFourForbidden));
    assert(!fc_incremental_is_legal_move(
        &crossingFourForbidden, 7, 7, 1));
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(crossing, transform, board);
        int gx = 0, gy = 0;
        fc_transform_point(transform, 7, 7, &gx, &gy);
        FCThreat threats[FC_MAX_THREATS];
        bool overflow = false;
        int count = fc_enumerate_threats(
            (const int (*)[FC_BOARD_SIZE])board, 1, false,
            FC_PROOF_SEARCH_VCT, threats, FC_MAX_THREATS, &overflow);
        const FCThreat *threat = find_threat(threats, count, gx, gy);
        assert(threat != NULL);
        assert(threat->severity == FC_THREAT_FOUR_THREE);
        for (int i = 0; i < threat->costCount; i++) {
            for (int j = i + 1; j < threat->costCount; j++) {
                assert(threat->costs[i].x != threat->costs[j].x ||
                       threat->costs[i].y != threat->costs[j].y);
            }
        }
    }

    int overline[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int x = 4; x <= 8; x++) overline[x][7] = 1;
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(overline, transform, board);
        int gx = 0, gy = 0;
        fc_transform_point(transform, 9, 7, &gx, &gy);
        FCThreat threats[FC_MAX_THREATS];
        bool overflow = false;
        int count = fc_enumerate_threats(
            (const int (*)[FC_BOARD_SIZE])board, 1, true,
            FC_PROOF_SEARCH_VCT, threats, FC_MAX_THREATS, &overflow);
        assert(find_threat(threats, count, gx, gy) == NULL);
    }

    int forbiddenMetadata[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    forbiddenMetadata[6][7] = forbiddenMetadata[7][7] =
        forbiddenMetadata[8][7] = 1;
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(forbiddenMetadata, transform, board);
        int gainX = 0, gainY = 0, dependencyX = 0, dependencyY = 0;
        fc_transform_point(transform, 5, 7, &gainX, &gainY);
        fc_transform_point(transform, 14, 7,
                           &dependencyX, &dependencyY);
        FCThreat threats[FC_MAX_THREATS];
        bool overflow = false;
        int count = fc_enumerate_threats(
            (const int (*)[FC_BOARD_SIZE])board, 1, true,
            FC_PROOF_SEARCH_VCT, threats, FC_MAX_THREATS, &overflow);
        assert(!overflow);
        const FCThreat *metadata = find_threat(
            threats, count, gainX, gainY);
        assert(metadata != NULL);
        assert(threat_mask_count(metadata->legalityDependencyMask) > 0);
        assert(threat_mask_has(metadata->legalityDependencyMask,
                               dependencyX, dependencyY));
        assert(threat_mask_has(metadata->dependencyMask,
                               dependencyX, dependencyY));
    }
}

static void test_vct_proof_and_integration_gates(void)
{
    int crossing[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    crossing[5][7] = crossing[6][7] = 1;
    crossing[7][5] = crossing[7][6] = 1;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, crossing, sizeof(crossing));
    FCProofResult vct;
    assert(fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])crossing, 1, false,
        FC_PROOF_SEARCH_VCT, 9, 18000, 0, 32768, &vct));
    assert(vct.searchClass == FC_PROOF_SEARCH_VCT);
    assert(vct.status == FC_PROOF_PROVEN_WIN);
    assert(vct.distance == 5);
    assert(vct.certificateVerified);
    assert(memcmp(before, crossing, sizeof(crossing)) == 0);

    FCAIProfile profile = fc_profile_proof_guided(false);
    profile.proofTimeBudgetMs = 0;
    FCAnalysisResult attack;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])crossing, 1, false,
        &profile, 44, FC_RANDOM_EVALUATION, 0, 0, &attack));
    assert(attack.x == 7 && attack.y == 7);
    assert(attack.overrideReason == FC_OVERRIDE_PROVEN_ATTACK);
    assert(attack.proofCertificateVerified);

    int defense[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    defense[6][7] = defense[7][7] = defense[8][7] = -1;
    FCAnalysisResult guarded;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])defense, 1, false,
        &profile, 45, FC_RANDOM_EVALUATION, 0, 0, &guarded));
    assert(guarded.overrideReason == FC_OVERRIDE_PROVEN_DEFENSE);
    assert((guarded.x == 5 && guarded.y == 7) ||
           (guarded.x == 9 && guarded.y == 7));

    profile.proofNodeBudget = 1;
    FCAnalysisResult unknown;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])crossing, 1, false,
        &profile, 46, FC_RANDOM_EVALUATION, 0, 0, &unknown));
    assert(unknown.x == 0 && unknown.y == 0);
    assert(unknown.proofStatus == FC_PROOF_UNKNOWN);
    assert(unknown.stats.budgetExhausted);

    int empty[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    profile = fc_profile_proof_guided(true);
    profile.proofTimeBudgetMs = 0;
    FCAnalysisResult book;
    assert(fc_analyze((const int (*)[FC_BOARD_SIZE])empty, 1, false,
                      &profile, 9876, FC_RANDOM_USER_GAME, &book));
    assert(book.defaultSource == FC_DEFAULT_OPENING_BOOK);
    assert(book.x == book.defaultX && book.y == book.defaultY);
    assert(book.bookId >= 0 && book.bookPly == 0);

    int rejectedBook[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    rejectedBook[14][12] = 1;
    rejectedBook[12][14] = -1;
    rejectedBook[14][11] = 1;
    rejectedBook[11][14] = -1;
    rejectedBook[14][10] = 1;
    FCAnalysisResult rejected;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])rejectedBook, -1, false,
        &profile, UINT64_C(0x558c38a4d28c3f98),
        FC_RANDOM_EVALUATION, 14, 13, &rejected));
    assert(rejected.defaultSource == FC_DEFAULT_OPENING_BOOK);
    assert(rejected.overrideReason == FC_OVERRIDE_PROVEN_DEFENSE);
    assert(rejected.x == 14 && rejected.y == 9);
}

static void test_elite_corpus_lookup_and_fail_closed_five_star(void)
{
    int source[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int stones[][3] = {
        {4,8,1}, {5,7,-1}, {6,6,-1}, {6,8,-1}, {7,5,-1},
        {7,6,1}, {7,8,1}, {8,7,-1}, {8,8,1}, {9,6,1}
    };
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        source[stones[i][0]][stones[i][1]] = stones[i][2];
    for (int transform = 0; transform < 8; transform++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        transform_board(source, transform, board);
        FCEliteCorpusMove moves[FC_MAX_CORPUS_CANDIDATES];
        int position = -1;
        int matchType = FC_CORPUS_MATCH_NONE;
        int count = fc_elite_corpus_lookup_detailed(
            (const int (*)[FC_BOARD_SIZE])board, 1, false,
            moves, FC_MAX_CORPUS_CANDIDATES, &position, &matchType);
        assert(count == 1 && position >= 0);
        assert(matchType == FC_CORPUS_MATCH_EXACT);
        int expectedX = 0, expectedY = 0;
        fc_transform_point(transform, 6, 7, &expectedX, &expectedY);
        assert(moves[0].x == expectedX && moves[0].y == expectedY);
        assert(moves[0].games >= 2 && moves[0].events >= 2);
        assert(moves[0].trustTier == 2);
    }

    /* The change-local corpus-conflict fixture must reach the universal
     * guard before corpus advice can replace the provisional defense. */
    FCAIProfile guard = fc_profile_five_star_early_micro_vcf_candidate();
    guard.parallelProofEnabled = false;
    guard.proofWorkerCount = 1;
    FCAnalysisResult guardedCorpus;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])source, 1, false, &guard,
        UINT64_C(0x4755415244434f52), FC_RANDOM_EVALUATION,
        6, 7, &guardedCorpus));
    FCProofDiagnostics guardDiagnostics = fc_proof_diagnostics_get();
    assert(guardedCorpus.corpusLookup);
    assert(guardedCorpus.opponentGuardEligible ||
           guardedCorpus.opponentGuardSkipReason ==
               FC_GUARD_SKIP_VERIFIED_OWN_WIN);
    if (guardedCorpus.opponentGuardEligible) {
        assert(guardedCorpus.opponentGuardAuditedCount >= 1);
        assert(guardDiagnostics.opponentGuardVCFQueries >= 1);
        if (guardedCorpus.corpusAccepted &&
            (guardedCorpus.opponentGuardProvisionalX !=
                 guardedCorpus.x ||
             guardedCorpus.opponentGuardProvisionalY !=
                 guardedCorpus.y))
            assert((guardedCorpus.opponentGuardAuditedStages &
                    FC_GUARD_STAGE_CORPUS) != 0);
    }

    int empty[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    FCAnalysisResult four;
    FCAnalysisResult five;
    assert(fc_analyze_four_star_with_hint(
        (const int (*)[FC_BOARD_SIZE])empty, 1, false, 1234,
        FC_RANDOM_EVALUATION, 7, 7, &four));
    assert(fc_analyze_five_star_with_hint(
        (const int (*)[FC_BOARD_SIZE])empty, 1, false, 1234,
        FC_RANDOM_EVALUATION, 7, 7, &five));
    assert(five.x == four.x && five.y == four.y);
    assert(!five.corpusLookup && five.corpusReason == FC_CORPUS_NO_POSITION);
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    assert(profile.eliteCorpusEnabled && !profile.openingBookEnabled);
    assert(strcmp(fc_elite_corpus_version(),
                  "gomocup-elite-openings-2020-2026-rule-partitioned-v2") == 0);
    FCEliteCorpusMove partitioned[FC_MAX_CORPUS_CANDIDATES];
    int partitionPosition = -1;
    int partitionMatch = FC_CORPUS_MATCH_NONE;
    (void)fc_elite_corpus_lookup_detailed(
        (const int (*)[FC_BOARD_SIZE])source, 1, true, partitioned,
        FC_MAX_CORPUS_CANDIDATES, &partitionPosition, &partitionMatch);
    assert(partitionMatch != FC_CORPUS_MATCH_EXACT || partitionPosition < 0);
}

static void test_elite_local_lookup_survives_natural_deviation_and_partitions_rules(void)
{
    const int opening[][3] = {
        {7,7,1}, {5,10,-1}, {7,10,1}, {10,6,-1}, {8,6,1}, {7,8,-1},
        {6,8,1}, {5,9,-1}, {6,9,1}, {5,8,-1}, {5,7,1}, {6,7,-1}
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(opening) / sizeof(opening[0]); i++)
        assert(fc_make_move(board, opening[i][0], opening[i][1],
                            opening[i][2], false));
    FCEliteCorpusMove moves[FC_MAX_CORPUS_CANDIDATES];
    int position = -1;
    int matchType = FC_CORPUS_MATCH_NONE;
    int count = fc_elite_corpus_lookup_detailed(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, moves,
        FC_MAX_CORPUS_CANDIDATES, &position, &matchType);
    assert(count > 0 && matchType == FC_CORPUS_MATCH_LOCAL && position < -1);
    assert(moves[0].requiredStones >= 4 && moves[0].trustTier >= 1);
    for (int i = 0; i < count; i++)
        assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                moves[i].x, moves[i].y, 1, false));

    position = -1;
    matchType = FC_CORPUS_MATCH_NONE;
    count = fc_elite_corpus_lookup_detailed(
        (const int (*)[FC_BOARD_SIZE])board, 1, true, moves,
        FC_MAX_CORPUS_CANDIDATES, &position, &matchType);
    for (int i = 0; i < count; i++)
        assert(moves[i].sourceBoardMask == 1);
}

static const FCCorpusCandidateTelemetry *selected_corpus_telemetry(
    const FCAnalysisResult *result)
{
    for (int i = 0; i < result->corpusCandidateCount; i++) {
        const FCCorpusCandidateTelemetry *candidate =
            &result->corpusCandidates[i];
        if (candidate->x == result->x && candidate->y == result->y)
            return candidate;
    }
    return NULL;
}

static void test_five_star_randomness_requires_proof_equivalence(void)
{
    const int stones[][3] = {
        {4,9,1}, {6,7,-1}, {7,8,1},
        {7,9,-1}, {9,9,1}, {8,5,-1}
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.decisionTimeBudgetMs = 1200;
    FCAnalysisResult evaluationA;
    FCAnalysisResult evaluationB;
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        11, FC_RANDOM_EVALUATION, 8, 8, &evaluationA));
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        12, FC_RANDOM_EVALUATION, 8, 8, &evaluationB));
    assert(evaluationA.x == evaluationB.x && evaluationA.y == evaluationB.y);
    assert(evaluationA.proofStatus == evaluationB.proofStatus);
    assert(evaluationA.proofSearchClass == evaluationB.proofSearchClass);
    assert(evaluationA.proofDistance == evaluationB.proofDistance);

    FCAnalysisResult userA;
    FCAnalysisResult userB;
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        101, FC_RANDOM_USER_GAME, 8, 8, &userA));
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        202, FC_RANDOM_USER_GAME, 8, 8, &userB));
    assert(userA.randomEquivalenceSignature != 0);
    assert(userB.randomEquivalenceSignature != 0);
    if (userA.randomSelectionUsed)
        assert(userA.randomEligibilityVerified);
    if (userB.randomSelectionUsed)
        assert(userB.randomEligibilityVerified);
    const FCCorpusCandidateTelemetry *left =
        selected_corpus_telemetry(&userA);
    const FCCorpusCandidateTelemetry *right =
        selected_corpus_telemetry(&userB);
    if (left != NULL && right != NULL &&
        (userA.x != userB.x || userA.y != userB.y)) {
        assert(userA.randomEquivalenceSignature ==
               userB.randomEquivalenceSignature);
        assert(left->proofStatus == right->proofStatus);
        assert(left->proofSearchClass == right->proofSearchClass);
        assert(left->proofDistance == right->proofDistance);
        assert(left->completedDepth == right->completedDepth);
        assert(left->tacticalClass == right->tacticalClass);
        assert(left->games == right->games);
        assert(left->events == right->events);
        assert(left->sources == right->sources);
        assert(left->trustTier == right->trustTier);
        assert(left->reason == right->reason);
    }
}

static void test_multi_seed_randomness_stays_in_completed_equivalence_class(void)
{
    FCCorpusCandidateTelemetry candidates[3] = {{0}};
    candidates[0] = (FCCorpusCandidateTelemetry){
        .x = 6, .y = 7, .games = 24, .events = 2, .sources = 2,
        .trustTier = 2, .matchType = FC_CORPUS_MATCH_EXACT,
        .requiredStones = 8, .sourceBoardMask = 1,
        .completedDepth = 10,
        .proofStatus = FC_PROOF_NO_FORCED_WIN_IN_SCOPE,
        .proofSearchClass = FC_PROOF_SEARCH_VCT, .proofDistance = 0,
        .tacticalClass = FC_TACTICAL_NORMAL, .accepted = true,
        .reason = FC_CORPUS_ACCEPTED_SUPERIOR
    };
    candidates[1] = candidates[0];
    candidates[1].x = 8;
    candidates[2] = candidates[0];
    candidates[2].x = 7;
    candidates[2].proofStatus = FC_PROOF_UNKNOWN;
    const int scores[3] = {120, 114, 119};
    assert(fc_test_corpus_random_equivalent(
        &candidates[0], scores[0], &candidates[1], scores[1]));
    assert(!fc_test_corpus_random_equivalent(
        &candidates[0], scores[0], &candidates[2], scores[2]));

    bool sawFirst = false;
    bool sawSecond = false;
    for (uint64_t seed = 1; seed <= 64; seed++) {
        int selected = fc_test_select_corpus_random_equivalent(
            candidates, scores, 3, seed);
        assert(selected == 0 || selected == 1);
        sawFirst = sawFirst || selected == 0;
        sawSecond = sawSecond || selected == 1;
    }
    assert(sawFirst && sawSecond);

#define ASSERT_RANDOM_FIELD_REJECTED(field, value) do {                    \
    FCCorpusCandidateTelemetry changed = candidates[1];                    \
    changed.field = (value);                                               \
    assert(!fc_test_corpus_random_equivalent(                              \
        &candidates[0], scores[0], &changed, scores[1]));                  \
} while (0)
    ASSERT_RANDOM_FIELD_REJECTED(games, 23);
    ASSERT_RANDOM_FIELD_REJECTED(events, 1);
    ASSERT_RANDOM_FIELD_REJECTED(sources, 1);
    ASSERT_RANDOM_FIELD_REJECTED(trustTier, 1);
    ASSERT_RANDOM_FIELD_REJECTED(matchType, FC_CORPUS_MATCH_LOCAL);
    ASSERT_RANDOM_FIELD_REJECTED(requiredStones, 7);
    ASSERT_RANDOM_FIELD_REJECTED(sourceBoardMask, 3);
    ASSERT_RANDOM_FIELD_REJECTED(completedDepth, 9);
    ASSERT_RANDOM_FIELD_REJECTED(proofStatus, FC_PROOF_UNKNOWN);
    ASSERT_RANDOM_FIELD_REJECTED(proofSearchClass, FC_PROOF_SEARCH_VCF);
    ASSERT_RANDOM_FIELD_REJECTED(proofDistance, 2);
    ASSERT_RANDOM_FIELD_REJECTED(tacticalClass, FC_TACTICAL_MUST_DEFEND);
    ASSERT_RANDOM_FIELD_REJECTED(accepted, false);
    ASSERT_RANDOM_FIELD_REJECTED(reason,
                                 FC_CORPUS_ACCEPTED_TRUSTED_NEAR_EQUIVALENT);
#undef ASSERT_RANDOM_FIELD_REJECTED
    assert(!fc_test_corpus_random_equivalent(
        &candidates[0], scores[0], &candidates[1], 109));
}

static void test_dfpn_stalled_expanded_graph_respects_deadline(void)
{
    double elapsed = 0.0;
    assert(fc_test_dfpn_stalled_graph_respects_deadline(&elapsed));
    assert(elapsed >= 0.0 && elapsed < 250.0);
}

static void test_parallel_root_dfpn_isolated_and_deterministic(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;
    int original[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(original, board, sizeof(original));
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.proofMaxDepth = 9;
    profile.proofNodeBudget = 18000;
    profile.proofParallelNodeBudget = 72000;
    profile.proofWorkerCount = 4;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;

    FCProofResult first;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &first));
    assert(first.status == FC_PROOF_PROVEN_WIN);
    assert(first.certificateVerified);
    assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                           1, false, &first));
    assert(memcmp(board, original, sizeof(board)) == 0);
    FCProofDiagnostics firstDiagnostics = fc_proof_diagnostics_get();
    assert(firstDiagnostics.parallelBatches == 1);
    assert(firstDiagnostics.parallelWorkersLaunched >= 1);
    assert(firstDiagnostics.parallelRootJobs >= 2);
    assert(firstDiagnostics.parallelRootWins == 1);
    assert(firstDiagnostics.incrementalMakes ==
           firstDiagnostics.incrementalUnmakes);

    FCProofResult second;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &second));
    assert(second.status == first.status);
    assert(second.x == first.x && second.y == first.y);
    assert(second.distance == first.distance);
    assert(second.certificateId == first.certificateId);
    assert(second.nodes == first.nodes);
    assert(memcmp(board, original, sizeof(board)) == 0);

    int refuted[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(refuted, board, sizeof(refuted));
    refuted[10][2] = refuted[10][3] = -1;
    refuted[10][4] = refuted[10][5] = -1;
    FCProofResult disproof;
    fc_proof_diagnostics_reset();
    assert(!fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &disproof));
    assert(disproof.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
    assert(disproof.disproofNumber == 0);
    assert(fc_verify_scoped_disproof(
        (const int (*)[FC_BOARD_SIZE])refuted, 1, false, &disproof));
    FCProofDiagnostics disproofDiagnostics = fc_proof_diagnostics_get();
    assert(disproofDiagnostics.parallelRootJobs >= 2);
    assert(disproofDiagnostics.parallelAggregateDisproofs == 1);

    profile.proofParallelNodeBudget = 2;
    FCProofResult partial;
    fc_proof_diagnostics_reset();
    assert(!fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &partial));
    assert(partial.status == FC_PROOF_UNKNOWN);
    assert(partial.disproofNumber != 0);
    assert(partial.budgetExhausted);
    assert(memcmp(board, original, sizeof(board)) == 0);
}

static void test_decision_ledger_saturation_contract(void)
{
    FCAIProfile profile = fc_profile_five_star_early_micro_vcf_candidate();
    profile.decisionNodeBudget = 3;
    profile.decisionMemoryBudgetBytes = 16;
    profile.decisionCorpusQueryBudget = 2;
    FCDecisionLedger ledger;
    assert(fc_decision_ledger_begin(&ledger, &profile));
    assert(fc_decision_ledger_reserve_nodes(&ledger, 2));
    assert(!fc_decision_ledger_reserve_nodes(&ledger, 2));
    assert(fc_decision_ledger_consume_node(&ledger));
    assert(fc_decision_ledger_consume_node(&ledger));
    assert(fc_decision_ledger_consume_node(&ledger));
    assert(!fc_decision_ledger_consume_node(&ledger));
    assert(fc_decision_ledger_reserve_memory(&ledger, 8));
    assert(!fc_decision_ledger_consume_memory(&ledger, 32));
    assert(fc_decision_ledger_consume_query(&ledger));
    assert(fc_decision_ledger_consume_query(&ledger));
    assert(!fc_decision_ledger_consume_query(&ledger));
}

int main(void)
{
    test_immediate_win_and_board_integrity();
    test_must_defend();
    test_edge_rotation_equivalence();
    test_edge_mirror_and_empty_board();
    test_proven_loss_and_tactical_width();
    test_forbidden_overline();
    test_fixed_seed_and_random_isolation();
    test_unique_tactical_move_ignores_seed();
    test_budget_returns_legal_completed_result();
    test_make_unmake_transforms_and_keys();
    test_incremental_position_reference_equivalence();
    test_frozen_four_star_and_candidate_identity();
    test_opponent_guard_profile_and_reserved_ledger();
    test_opponent_guard_vcf_first_audit_and_restoration();
    test_early_micro_vcf_profile_and_semantics();
    test_opponent_guard_acceptance_paths_and_isolation();
    test_five_star_hard_deadline_returns_completed_legal_move();
    test_candidate_single_proof_session_and_counters();
    test_quiet_portfolio_is_active_but_does_not_heuristically_override();
    test_randomized_incremental_long_sequences();
    test_dense_incremental_reply_set_equivalence();
    test_opening_book_reproducibility_and_variety();
    test_threat_enumeration_and_proof_certificate();
    test_proof_determinism_and_budget_unknown();
    test_opponent_guard_distance_matrix_symmetry_and_rules();
    test_opponent_guard_completed_class_ordering();
    test_table_driven_threat_patterns_all_symmetries();
    test_vcf_and_node_multi_defense_certificate();
    test_candidate_dfpn_proof_disproof_thresholds_and_collisions();
    test_relevance_and_dependency_adversarial_cases();
    test_dependency_dag_proposes_verified_search_order();
    test_candidate_stage_order_and_obligation_skips();
    test_crossing_duplicate_and_forbidden_symmetries();
    test_vct_proof_and_integration_gates();
    test_elite_corpus_lookup_and_fail_closed_five_star();
    test_elite_local_lookup_survives_natural_deviation_and_partitions_rules();
    test_five_star_randomness_requires_proof_equivalence();
    test_multi_seed_randomness_stays_in_completed_equivalence_class();
    test_dfpn_stalled_expanded_graph_respects_deadline();
    test_parallel_root_dfpn_isolated_and_deterministic();
    test_decision_ledger_saturation_contract();
    puts("FiveChessAI tests passed");
    return 0;
}
