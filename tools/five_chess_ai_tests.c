#include "../ice five chess/FiveChessAI.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    FCAIProfile candidate = fc_profile_five_star_proof_engine_candidate();
    FCAIProfile production = fc_profile_five_star();
    assert(candidate.proofEngineCandidate);
    assert(candidate.lossAwareEnabled && candidate.quietThreatEnabled);
    assert(candidate.decisionTimeBudgetMs == 4500);
    assert(!production.proofEngineCandidate);
    assert(production.decisionTimeBudgetMs == 0);
    assert(strcmp(production.version,
                  "5.1.0-elite-rule-partitioned-local-v2") == 0);

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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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

    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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
    FCAIProfile profile = fc_profile_five_star();
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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

static void assert_hybrid_component_matches_direct(int side,
                                                   bool forbiddenBlack,
                                                   FCRandomMode randomMode)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y <= 6; y++) board[7][y] = side;
    board[6][5] = -side;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));

    FCAIProfile directProfile = side == -1
        ? fc_profile_five_star_proof_engine_candidate()
        : fc_profile_five_star();
    FCAnalysisResult direct;
    FCAnalysisResult hybrid;
    uint64_t seed = forbiddenBlack ? UINT64_C(0xf0b1dd3e)
                                   : UINT64_C(0x51decafe);
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
        &directProfile, seed, randomMode, 8, 8, &direct));
    assert(fc_analyze_five_star_color_hybrid_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
        seed, randomMode, 8, 8, &hybrid));

    assert(hybrid.x == direct.x && hybrid.y == direct.y);
    assert(hybrid.score == direct.score);
    assert(hybrid.tacticalClass == direct.tacticalClass);
    assert(hybrid.provenLoss == direct.provenLoss);
    assert(hybrid.proofStatus == direct.proofStatus);
    assert(hybrid.proofSearchClass == direct.proofSearchClass);
    assert(hybrid.proofDistance == direct.proofDistance);
    assert(hybrid.proofCertificateId == direct.proofCertificateId);
    assert(hybrid.proofCertificateVerified ==
           direct.proofCertificateVerified);
    assert(hybrid.randomMode == direct.randomMode);
    assert(hybrid.randomCandidateCount == direct.randomCandidateCount);
    assert(hybrid.randomSelectionUsed == direct.randomSelectionUsed);
    assert(hybrid.randomSelectedRank == direct.randomSelectedRank);
    assert(hybrid.randomEquivalenceSignature ==
           direct.randomEquivalenceSignature);
    assert(hybrid.randomEligibilityVerified ==
           direct.randomEligibilityVerified);
    assert(hybrid.seed == seed);
    assert(hybrid.randomEquivalenceSignature != 0);
    assert(hybrid.hybridComponent ==
           (side == -1 ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
                       : FC_HYBRID_COMPONENT_BLACK_V51));
    assert(strcmp(fc_hybrid_component_version(hybrid.hybridComponent),
                  directProfile.version) == 0);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            hybrid.x, hybrid.y, side, forbiddenBlack));
    assert(memcmp(before, board, sizeof(board)) == 0);
}

static void test_color_specialized_hybrid_component_equivalence(void)
{
    const FCRandomMode modes[] = {
        FC_RANDOM_EVALUATION,
        FC_RANDOM_USER_GAME
    };
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        for (size_t mode = 0; mode < sizeof(modes) / sizeof(modes[0]); mode++) {
            assert_hybrid_component_matches_direct(
                1, forbidden != 0, modes[mode]);
            assert_hybrid_component_matches_direct(
                -1, forbidden != 0, modes[mode]);
        }
    }
}

static void assert_v521_parallel_policy_isolated(void)
{
    FCAIProfile source = fc_profile_five_star_loss_aware_candidate();
    FCAIProfile serial = fc_profile_five_star_v521_serial_hybrid_control();
    FCAIProfile parallel =
        fc_profile_five_star_v521_parallel_hybrid_candidate();
    assert(!source.proofEngineCandidate);
    assert(!serial.proofEngineCandidate);
    assert(!parallel.proofEngineCandidate);
    assert(source.proofCandidateStagesEnabled);
    assert(serial.proofCandidateStagesEnabled);
    assert(parallel.proofCandidateStagesEnabled);
    assert(!source.parallelProofEnabled);
    assert(!serial.parallelProofEnabled);
    assert(parallel.parallelProofEnabled);
    assert(serial.proofWorkerCount == 1);
    assert(parallel.proofWorkerCount == 8);
    assert(serial.proofParallelNodeBudget == source.proofNodeBudget);
    assert(parallel.proofParallelNodeBudget == source.proofNodeBudget * 8);
    assert(serial.decisionTimeBudgetMs == 4500);
    assert(parallel.decisionTimeBudgetMs == 4500);
#define ASSERT_V521_POLICY_FIELD(field) do { \
    assert(serial.field == source.field);      \
    assert(parallel.field == source.field);    \
} while (0)
    ASSERT_V521_POLICY_FIELD(maxDepth);
    ASSERT_V521_POLICY_FIELD(quiescenceDepth);
    ASSERT_V521_POLICY_FIELD(fourDepth);
    ASSERT_V521_POLICY_FIELD(doubleThreeDepth);
    ASSERT_V521_POLICY_FIELD(forcingDepth);
    ASSERT_V521_POLICY_FIELD(candidateLimit);
    ASSERT_V521_POLICY_FIELD(nodeBudget);
    ASSERT_V521_POLICY_FIELD(timeBudgetMs);
    ASSERT_V521_POLICY_FIELD(transpositionCapacity);
    ASSERT_V521_POLICY_FIELD(attackWeight);
    ASSERT_V521_POLICY_FIELD(defenseWeight);
    ASSERT_V521_POLICY_FIELD(centerWeight);
    ASSERT_V521_POLICY_FIELD(nearBestWindow);
    ASSERT_V521_POLICY_FIELD(randomTemperature);
    ASSERT_V521_POLICY_FIELD(maxRandomCandidates);
    ASSERT_V521_POLICY_FIELD(proofEnabled);
    ASSERT_V521_POLICY_FIELD(proofCandidateStagesEnabled);
    ASSERT_V521_POLICY_FIELD(openingBookEnabled);
    ASSERT_V521_POLICY_FIELD(proofSearchClass);
    ASSERT_V521_POLICY_FIELD(proofMaxDepth);
    ASSERT_V521_POLICY_FIELD(proofNodeBudget);
    ASSERT_V521_POLICY_FIELD(proofTimeBudgetMs);
    ASSERT_V521_POLICY_FIELD(proofTranspositionCapacity);
    ASSERT_V521_POLICY_FIELD(lossAwareEnabled);
    ASSERT_V521_POLICY_FIELD(quietThreatEnabled);
    ASSERT_V521_POLICY_FIELD(proofEscapeCandidateLimit);
    ASSERT_V521_POLICY_FIELD(proofQuietRootLimit);
    ASSERT_V521_POLICY_FIELD(proofEmergencyTimeBudgetMs);
    ASSERT_V521_POLICY_FIELD(eliteCorpusEnabled);
    ASSERT_V521_POLICY_FIELD(corpusScoreMargin);
    ASSERT_V521_POLICY_FIELD(corpusMinGames);
    ASSERT_V521_POLICY_FIELD(corpusMinEvents);
#undef ASSERT_V521_POLICY_FIELD
    FCAIProfile four = fc_profile_frozen_four_star_control();
    FCAIProfile production = fc_profile_five_star();
    FCAIProfile v57 = fc_profile_five_star_v57_hybrid_candidate();
    assert(!four.parallelProofEnabled && four.proofWorkerCount == 0);
    assert(!production.parallelProofEnabled &&
           production.proofWorkerCount == 0);
    assert(strcmp(v57.version,
                  "5.7.0-white-v541-black-v521-independent-root-parallel8-5s") == 0);
    assert(v57.parallelProofEnabled && v57.proofWorkerCount == 8);
    assert(v57.proofParallelNodeBudget == v57.proofNodeBudget * 8);
    assert(v57.decisionTimeBudgetMs == 4500);
    assert(!v57.forkFirstRecoveryEnabled);
    assert(!v57.branchFirstSearchEnabled);
    assert(v57.branchFirstAdvancedFourDepthBonus == 0);
    assert(v57.branchFirstAdvancedThreeDepthBonus == 0);
    FCAIProfile forkRecovery =
        fc_profile_five_star_v57_fork_recovery_candidate();
    assert(forkRecovery.forkFirstRecoveryEnabled);
    assert(forkRecovery.parallelProofEnabled &&
           forkRecovery.incrementalLegalityEnabled &&
           forkRecovery.recoverySearchEnabled);
    char v57Snapshot[8192];
    char forkSnapshot[8192];
    assert(fc_profile_snapshot(&v57, v57Snapshot, sizeof(v57Snapshot)) > 0);
    assert(fc_profile_snapshot(&forkRecovery, forkSnapshot,
                               sizeof(forkSnapshot)) > 0);
    assert(strstr(v57Snapshot,
                  "\"forkFirstRecoveryEnabled\":false") != NULL);
    assert(strstr(forkSnapshot,
                  "\"forkFirstRecoveryEnabled\":true") != NULL);
    FCAIProfile branchFirst =
        fc_profile_five_star_v57_branch_first_candidate();
    assert(branchFirst.branchFirstSearchEnabled);
    assert(!branchFirst.parallelProofEnabled);
    assert(branchFirst.proofMaxDepth == 14);
    assert(branchFirst.branchFirstAdvancedFourDepthBonus == 2);
    assert(branchFirst.branchFirstAdvancedThreeDepthBonus == 1);
    assert(branchFirst.branchFirstTacticalDepthCap == 16);
    char branchSnapshot[8192];
    assert(fc_profile_snapshot(&branchFirst, branchSnapshot,
                               sizeof(branchSnapshot)) > 0);
    assert(strstr(branchSnapshot,
                  "\"branchFirstAdvancedFourDepthBonus\":2") != NULL);
    assert(strstr(branchSnapshot,
                  "\"branchFirstAdvancedThreeDepthBonus\":1") != NULL);
    assert(strstr(branchSnapshot,
                  "\"branchFirstTacticalDepthCap\":16") != NULL);
}

static void assert_v521_hybrid_routes_component(int side,
                                                 bool forbiddenBlack,
                                                 int blackWorkers)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 3; y <= 6; y++) board[7][y] = side;
    board[6][5] = -side;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    FCAIProfile directProfile = side == -1
        ? fc_profile_five_star_proof_engine_candidate()
        : blackWorkers > 1
        ? fc_profile_five_star_v521_parallel_hybrid_candidate()
        : fc_profile_five_star_v521_serial_hybrid_control();
    FCAnalysisResult direct;
    FCAnalysisResult hybrid;
    uint64_t seed = forbiddenBlack ? UINT64_C(0x521f0b1d)
                                   : UINT64_C(0x521f4ee5);
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
        &directProfile, seed, FC_RANDOM_EVALUATION, 8, 8, &direct));
    assert(fc_analyze_five_star_v521_hybrid_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
        blackWorkers, seed, FC_RANDOM_EVALUATION, 8, 8, &hybrid));
    assert(hybrid.x == direct.x && hybrid.y == direct.y);
    assert(hybrid.tacticalClass == direct.tacticalClass);
    assert(hybrid.proofStatus == direct.proofStatus);
    assert(hybrid.proofSearchClass == direct.proofSearchClass);
    assert(hybrid.proofCertificateVerified ==
           direct.proofCertificateVerified);
    assert(hybrid.proofWorkerCap ==
           (side == -1 ? 8 : blackWorkers));
    assert(hybrid.hybridComponent ==
           (side == -1 ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
            : blackWorkers > 1 ? FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL
                               : FC_HYBRID_COMPONENT_BLACK_V521_SERIAL));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            hybrid.x, hybrid.y, side, forbiddenBlack));
    assert(memcmp(before, board, sizeof(board)) == 0);
}

static void test_parallel_v521_profiles_and_hybrid_routing(void)
{
    assert_v521_parallel_policy_isolated();
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        assert_v521_hybrid_routes_component(-1, forbidden != 0, 1);
        assert_v521_hybrid_routes_component(-1, forbidden != 0, 8);
        assert_v521_hybrid_routes_component(1, forbidden != 0, 1);
        assert_v521_hybrid_routes_component(1, forbidden != 0, 8);
    }
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[7][7] = 1;
    FCAnalysisResult four;
    assert(fc_analyze_four_star_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, -1, false,
        7, FC_RANDOM_EVALUATION, 8, 8, &four));
    assert(four.proofWorkersLaunched == 0);
    assert(four.proofParallelJobs == 0);
}

static void test_v521_parallel_root_capability_and_deadline(void)
{
    int proofBoard[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    proofBoard[5][7] = proofBoard[6][7] = 1;
    proofBoard[7][5] = proofBoard[7][6] = 1;
    int proofBefore[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(proofBefore, proofBoard, sizeof(proofBoard));
    FCAIProfile profile =
        fc_profile_five_star_v521_parallel_hybrid_candidate();
    assert(!profile.proofEngineCandidate && profile.parallelProofEnabled);
    profile.proofMaxDepth = 9;
    profile.proofNodeBudget = 18000;
    profile.proofParallelNodeBudget = 72000;
    profile.proofWorkerCount = 8;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    FCProofResult proof;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])proofBoard, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &proof));
    assert(proof.status == FC_PROOF_PROVEN_WIN);
    assert(proof.certificateVerified);
    assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])proofBoard,
                           1, false, &proof));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.parallelBatches == 1);
    /* A fully overlapping threat set is intentionally serialized through one
     * shared DFPN session so its descendants/TT are reused safely. */
    assert(diagnostics.parallelWorkersLaunched >= 1);
    assert(diagnostics.parallelWorkersLaunched <= 8);
    assert(diagnostics.parallelWorkersLaunched <=
           diagnostics.parallelRootJobs);
    assert(diagnostics.parallelRootJobsCompleted <=
           diagnostics.parallelRootJobs);
    assert(diagnostics.parallelBudgetTokens > 0);
    assert(diagnostics.parallelGroupedRootJobs >=
           diagnostics.parallelRootJobs);
    assert(diagnostics.incrementalMakes == diagnostics.incrementalUnmakes);
    assert(memcmp(proofBoard, proofBefore, sizeof(proofBoard)) == 0);

    const int moves[][3] = {
        {7,7,1},{8,6,-1},{9,7,1},{10,7,-1},{11,7,1},{10,6,-1},
        {10,5,1},{9,8,-1},{11,6,1},{11,5,-1},{9,6,1},{8,7,-1},
        {7,6,1},{8,8,-1},{8,5,1},{9,4,-1},{8,9,1},{7,8,-1},
        {6,8,1},{10,9,-1},{10,8,1},{10,4,-1}
    };
    int deadlineBoard[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(moves) / sizeof(moves[0]); i++)
        assert(fc_make_move(deadlineBoard, moves[i][0], moves[i][1],
                            moves[i][2], false));
    int deadlineBefore[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(deadlineBefore, deadlineBoard, sizeof(deadlineBoard));
    FCAnalysisResult result;
    assert(fc_analyze_five_star_v521_hybrid_with_hint(
        (const int (*)[FC_BOARD_SIZE])deadlineBoard, 1, false, 8,
        UINT64_C(0x5218baaadb6f), FC_RANDOM_USER_GAME,
        8, 10, &result));
    assert(result.hybridComponent ==
           FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL);
    assert(result.proofWorkerCap == 8);
    assert(result.stats.elapsedMilliseconds < 5000.0);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])deadlineBoard,
                            result.x, result.y, 1, false));
    assert(result.randomEligibilityVerified);
    assert(memcmp(deadlineBefore, deadlineBoard,
                  sizeof(deadlineBoard)) == 0);
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

static void test_hybrid_black_v51_outer_deadline_returns_completed_move(void)
{
    const int moves[][3] = {
        {7,7,1},{8,6,-1},{9,7,1},{10,7,-1},{11,7,1},{10,6,-1},
        {10,5,1},{9,8,-1},{11,6,1},{11,5,-1},{9,6,1},{8,7,-1},
        {7,6,1},{8,8,-1},{8,5,1},{9,4,-1},{8,9,1},{7,8,-1},
        {6,8,1},{10,9,-1},{10,8,1},{10,4,-1}
    };
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (size_t i = 0; i < sizeof(moves) / sizeof(moves[0]); i++)
        assert(fc_make_move(board, moves[i][0], moves[i][1],
                            moves[i][2], false));
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(board));
    FCAnalysisResult result;
    assert(fc_analyze_five_star_color_hybrid_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        UINT64_C(0xbaaadb6fac868a8a), FC_RANDOM_USER_GAME,
        8, 10, &result));
    assert(result.hybridComponent == FC_HYBRID_COMPONENT_BLACK_V51);
    assert(result.stats.elapsedMilliseconds < 5000.0);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            result.x, result.y, 1, false));
    assert(result.randomEligibilityVerified);
    assert(memcmp(before, board, sizeof(board)) == 0);
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
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
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

static void test_v57_persistent_scheduler_reuses_pool_and_returns_tokens(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;
    int original[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(original, board, sizeof(original));
    FCAIProfile profile =
        fc_profile_five_star_v57_thread_scheduler_candidate();
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
    FCProofDiagnostics firstDiagnostics = fc_proof_diagnostics_get();
    assert(firstDiagnostics.parallelPoolDispatches == 1);
    assert(firstDiagnostics.parallelPoolWorkersReused >= 1);
    assert(firstDiagnostics.parallelTokenBlockClaims > 0);
    assert(firstDiagnostics.parallelTokenBlockReturns > 0);
    assert(firstDiagnostics.parallelBudgetTokens <= 72000);
    assert(memcmp(board, original, sizeof(board)) == 0);

    FCProofResult second;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &second));
    FCProofDiagnostics secondDiagnostics = fc_proof_diagnostics_get();
    assert(secondDiagnostics.parallelPoolDispatches == 1);
    assert(secondDiagnostics.parallelPoolFallbacks == 0);
    assert(second.x == first.x && second.y == first.y);
    assert(second.certificateId == first.certificateId);
    assert(second.nodes == first.nodes);
    assert(memcmp(board, original, sizeof(board)) == 0);

    char snapshot[8192];
    assert(fc_profile_snapshot(&profile, snapshot, sizeof(snapshot)) > 0);
    assert(strstr(snapshot, "\"persistentWorkerPoolEnabled\":true") != NULL);
    assert(strstr(snapshot, "\"parallelTokenBlockEnabled\":true") != NULL);
    assert(strstr(snapshot, "\"parallelTokenBlockSize\":16") != NULL);
}

static void test_v541_scheduler_profile_and_route(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;
    int original[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(original, board, sizeof(original));

    FCAIProfile baseline = fc_profile_five_star_proof_engine_candidate();
    FCAIProfile candidate =
        fc_profile_five_star_v541_thread_scheduler_candidate();
    assert(strcmp(baseline.version,
                  "5.4.1-transactional-deadline-root-parallel-5s") == 0);
    assert(!baseline.persistentWorkerPoolEnabled);
    assert(!baseline.parallelTokenBlockEnabled);
    assert(strcmp(candidate.version,
                  "5.4.2-v541-persistent-pool-token-blocks-8w-5s") == 0);
    assert(candidate.proofEngineCandidate);
    assert(candidate.parallelProofEnabled);
    assert(candidate.persistentWorkerPoolEnabled);
    assert(candidate.parallelTokenBlockEnabled);
    assert(candidate.parallelTokenBlockSize == 64);
    assert(candidate.proofMaxDepth == baseline.proofMaxDepth);
    assert(candidate.proofNodeBudget == baseline.proofNodeBudget);
    assert(candidate.proofParallelNodeBudget ==
           baseline.proofParallelNodeBudget);
    assert(candidate.decisionNodeBudget == 0);
    assert(candidate.decisionHardLimitMs == 0);
    assert(candidate.decisionLedgerVersion == 0);
    assert(candidate.decisionMemoryBudgetBytes == 0);
    assert(candidate.decisionCorpusQueryBudget == 0);
    assert(!candidate.incrementalLegalityEnabled);
    assert(!candidate.validateLegalityCache);
    assert(!candidate.recoverySearchEnabled);
    assert(candidate.decisionTimeBudgetMs == 4200);

    char snapshot[8192];
    assert(fc_profile_snapshot(&candidate, snapshot, sizeof(snapshot)) > 0);
    assert(strstr(snapshot,
                  "\"version\":\"5.4.2-v541-persistent-pool-token-blocks-8w-5s\"") != NULL);
    assert(strstr(snapshot, "\"persistentWorkerPoolEnabled\":true") != NULL);
    assert(strstr(snapshot, "\"parallelTokenBlockSize\":64") != NULL);

    FCProofResult proof;
    candidate.proofMaxDepth = 9;
    candidate.proofNodeBudget = 18000;
    candidate.proofParallelNodeBudget = 72000;
    candidate.decisionNodeBudget = 72000;
    candidate.proofWorkerCount = 4;
    candidate.proofTimeBudgetMs = 0;
    candidate.proofEmergencyTimeBudgetMs = 0;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &candidate, &proof));
    assert(proof.status == FC_PROOF_PROVEN_WIN);
    assert(proof.certificateVerified);
    assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                           1, false, &proof));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.parallelPoolDispatches == 1);
    assert(diagnostics.parallelPoolWorkersReused >= 1);
    assert(diagnostics.parallelTokenBlockClaims > 0);
    assert(diagnostics.parallelTokenBlockReturns > 0);
    assert(diagnostics.parallelBudgetTokens <= 72000);
    assert(memcmp(board, original, sizeof(board)) == 0);

    FCAnalysisResult analysis;
    assert(fc_analyze_five_star_v541_thread_scheduler_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, 4,
        UINT64_C(0x54120260816), FC_RANDOM_EVALUATION, 7, 7, &analysis));
    assert(analysis.hybridComponent ==
           FC_HYBRID_COMPONENT_V541_THREAD_SCHEDULER);
    assert(strcmp(fc_hybrid_component_version(analysis.hybridComponent),
                  candidate.version) == 0);
    assert(analysis.proofWorkerCap == 4);
    assert(analysis.stats.elapsedMilliseconds < 5000.0);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            analysis.x, analysis.y, 1, false));
    assert(memcmp(board, original, sizeof(board)) == 0);
}

static void test_v57_branch_first_recursive_pool(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    /* Two independent pairs create a real recursive reply wave without
     * making the root itself an immediate win.  The branch workers are
     * expected to be useful-or-unknown here; either outcome must retain the
     * proof verifier and serial fallback contract. */
    board[5][7] = board[6][7] = 1;
    board[7][5] = board[7][6] = 1;
    int original[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(original, board, sizeof(original));
    FCAIProfile profile = fc_profile_five_star_v57_branch_first_candidate();
    profile.proofMaxDepth = 12;
    profile.proofNodeBudget = 72000;
    profile.proofParallelNodeBudget = 288000;
    profile.proofWorkerCount = 4;
    profile.proofTimeBudgetMs = 0;
    profile.proofEmergencyTimeBudgetMs = 0;
    profile.branchFirstMinRemainingDepth = 6;
    profile.branchFirstMinBranchCount = 2;
    profile.branchFirstMaxBranches = 32;
    FCProofResult first;
    fc_proof_diagnostics_reset();
    assert(fc_test_parallel_root_proof(
        (const int (*)[FC_BOARD_SIZE])board, 1, false,
        FC_PROOF_SEARCH_VCT, &profile, &first));
    assert(first.status == FC_PROOF_PROVEN_WIN);
    assert(first.certificateVerified);
    assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                           1, false, &first));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.branchFirstPreviewBranches > 0);
    assert(diagnostics.branchFirstWaves > 0);
    assert(diagnostics.branchFirstWorkersLaunched >= 2);
    assert(diagnostics.branchFirstJobs > 0);
    assert(diagnostics.branchFirstJobsCompleted > 0);
    assert(diagnostics.branchFirstJobsCompleted <=
           diagnostics.branchFirstJobs);
    assert(diagnostics.branchFirstUsefulJobs > 0 ||
           diagnostics.branchFirstUnknownJobs > 0);
    assert(diagnostics.branchFirstMaxConcurrentWorkers >= 2);
    assert(diagnostics.branchFirstDepthExtensions > 0);
    assert(diagnostics.branchFirstAdvancedFourDepthExtensions > 0);
    assert(diagnostics.branchFirstMaxChildDepth > 0);
    assert(memcmp(board, original, sizeof(board)) == 0);

    char snapshot[8192];
    assert(fc_profile_snapshot(&profile, snapshot, sizeof(snapshot)) > 0);
    assert(strstr(snapshot, "\"branchFirstSearchEnabled\":true") != NULL);
    assert(strstr(snapshot, "\"proofMaxDepth\":12") != NULL);
}

static void test_v57_recovery_budget_contract(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    const int moves[][3] = {
        {7,7,1}, {8,8,-1}, {7,8,1}, {8,7,-1},
        {6,7,1}, {9,8,-1}, {8,6,1}, {6,8,-1},
        {9,7,1}, {7,6,-1}, {6,6,1}, {9,9,-1}
    };
    for (size_t i = 0; i < sizeof(moves) / sizeof(moves[0]); i++)
        assert(fc_make_move(board, moves[i][0], moves[i][1],
                            moves[i][2], false));
    FCAIProfile profile = fc_profile_five_star_v57_hybrid_candidate();
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        UINT64_C(0x5700c0de), FC_RANDOM_EVALUATION, 7, 9, &result));
    assert(result.decisionLedgerVersion == profile.decisionLedgerVersion);
    assert(result.decisionNodesConsumed <= profile.decisionNodeBudget);
    assert(result.stats.elapsedMilliseconds < 5000.0);
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            result.x, result.y, 1, false));
    FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
    assert(diagnostics.decisionCount >= 1);
    assert(diagnostics.parallelRootJobs >= 0);
    assert(!profile.forkFirstRecoveryEnabled);
    assert(diagnostics.forkProbeCandidatesExamined == 0);
}

static void test_decision_ledger_saturation_contract(void)
{
    FCAIProfile profile = fc_profile_five_star_v57_hybrid_candidate();
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

static void test_reversible_ledger_and_fork_first_recovery(void)
{
    FCAIProfile ledgerProfile = fc_profile_five_star_v57_hybrid_candidate();
    ledgerProfile.decisionMemoryBudgetBytes = 64;
    FCDecisionLedger ledger;
    assert(fc_decision_ledger_begin(&ledger, &ledgerProfile));
    assert(fc_decision_ledger_reserve_memory(&ledger, 32));
    assert(atomic_load_explicit(&ledger.memoryReserved,
                                memory_order_relaxed) == 32);
    assert(atomic_load_explicit(&ledger.memoryPeakReserved,
                                memory_order_relaxed) == 32);
    fc_decision_ledger_release_memory(&ledger, 32);
    fc_decision_ledger_release_memory(&ledger, 32);
    assert(atomic_load_explicit(&ledger.memoryReserved,
                                memory_order_relaxed) == 0);
    assert(atomic_load_explicit(&ledger.memoryReleased,
                                memory_order_relaxed) == 32);
    assert(fc_decision_ledger_reserve_memory(&ledger, 64));
    assert(!fc_decision_ledger_reserve_memory(&ledger, 1));
    fc_decision_ledger_release_memory(&ledger, 64);
    assert(atomic_load_explicit(&ledger.memoryReserved,
                                memory_order_relaxed) == 0);
    assert(!fc_research_worker_count_is_valid(2));
    assert(fc_research_worker_count_is_valid(1));
    assert(fc_research_worker_count_is_valid(4));
    assert(fc_research_worker_count_is_valid(8));

    /* The advisory move at (0,0) leaves both ends of the opponent's four
     * open.  Blocking one end leaves one legal immediate reply and must beat
     * the fork before any deeper proof stage is consulted. */
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 4; y <= 7; y++) board[7][y] = -1;
    board[6][6] = 1;
    FCAIProfile profile = fc_profile_five_star_v57_fork_recovery_candidate();
    profile.proofEnabled = false;
    profile.proofCandidateStagesEnabled = false;
    profile.parallelProofEnabled = false;
    profile.openingBookEnabled = false;
    profile.eliteCorpusEnabled = false;
    profile.maxDepth = 0;
    profile.quiescenceDepth = 0;
    profile.decisionTimeBudgetMs = 1000;
    FCAnalysisResult result;
    fc_proof_diagnostics_reset();
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        UINT64_C(0x464f524b54455354), FC_RANDOM_EVALUATION,
        0, 0, &result));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            result.x, result.y, 1, false));
    assert(result.forkCandidatesExamined > 0);
    assert(result.forkProbeComplete);
    assert(result.forkRiskStatus != FC_FORK_RISK_FORK);
    assert(result.forkAvoidedCount == 1);
    assert(result.x == 7 && (result.y == 3 || result.y == 8));
    assert(result.decisionStatus == FC_DECISION_UNKNOWN_OR_DEADLINE);

    FCAnalysisResult repeated;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, false, &profile,
        UINT64_C(0x464f524b54455354), FC_RANDOM_EVALUATION,
        0, 0, &repeated));
    assert(repeated.x == result.x && repeated.y == result.y);
    assert(repeated.decisionStatus == result.decisionStatus);
    assert(repeated.forkRiskStatus == result.forkRiskStatus);
}

static void test_forbidden_fork_reply_and_invalid_handoff_recovery(void)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    for (int y = 4; y <= 7; y++) board[7][y] = -1;
    board[6][6] = 1;
    FCAIProfile profile = fc_profile_five_star_v57_fork_recovery_candidate();
    profile.proofEnabled = false;
    profile.proofCandidateStagesEnabled = false;
    profile.parallelProofEnabled = false;
    profile.openingBookEnabled = false;
    profile.eliteCorpusEnabled = false;
    profile.maxDepth = 0;
    profile.quiescenceDepth = 0;
    profile.decisionTimeBudgetMs = 1000;
    FCAnalysisResult forbiddenResult;
    assert(fc_analyze_with_hint(
        (const int (*)[FC_BOARD_SIZE])board, 1, true, &profile,
        UINT64_C(0x464f524b464f5242), FC_RANDOM_EVALUATION,
        0, 0, &forbiddenResult));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                            forbiddenResult.x, forbiddenResult.y, 1, true));
    assert(forbiddenResult.forkProbeComplete);
    assert(forbiddenResult.forkRiskStatus != FC_FORK_RISK_FORK);
    assert(forbiddenResult.selectedOpponentImmediateWinCount <= 1);

    int handoffBoard[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    handoffBoard[7][7] = 1;
    FCAnalysisResult handoff;
    assert(fc_analyze_five_star_profile_with_hint(
        (const int (*)[FC_BOARD_SIZE])handoffBoard, -1, false, &profile,
        UINT64_C(0x48414e444f4646), FC_RANDOM_EVALUATION,
        -1, -1, &handoff));
    assert(fc_is_legal_move((const int (*)[FC_BOARD_SIZE])handoffBoard,
                            handoff.x, handoff.y, -1, false));
    assert(handoff.candidateCount > 1);
    assert(handoff.handoffReason == FC_HANDOFF_FOUR_STAR_UNKNOWN ||
           handoff.handoffReason == FC_HANDOFF_FOUR_STAR_INVALID ||
           handoff.handoffReason == FC_HANDOFF_GENERATED_FALLBACK);
    assert(handoff.decisionStatus == FC_DECISION_UNKNOWN_OR_DEADLINE);
    assert(!handoff.provenLoss);
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
    test_five_star_hard_deadline_returns_completed_legal_move();
    test_candidate_single_proof_session_and_counters();
    test_quiet_portfolio_is_active_but_does_not_heuristically_override();
    test_randomized_incremental_long_sequences();
    test_dense_incremental_reply_set_equivalence();
    test_opening_book_reproducibility_and_variety();
    test_threat_enumeration_and_proof_certificate();
    test_proof_determinism_and_budget_unknown();
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
    test_color_specialized_hybrid_component_equivalence();
    test_parallel_v521_profiles_and_hybrid_routing();
    test_v521_parallel_root_capability_and_deadline();
    test_multi_seed_randomness_stays_in_completed_equivalence_class();
    test_hybrid_black_v51_outer_deadline_returns_completed_move();
    test_dfpn_stalled_expanded_graph_respects_deadline();
    test_parallel_root_dfpn_isolated_and_deterministic();
    test_v57_persistent_scheduler_reuses_pool_and_returns_tokens();
    test_v541_scheduler_profile_and_route();
    test_v57_branch_first_recursive_pool();
    test_v57_recovery_budget_contract();
    test_decision_ledger_saturation_contract();
    test_reversible_ledger_and_fork_first_recovery();
    test_forbidden_fork_reply_and_invalid_handoff_recovery();
    puts("FiveChessAI tests passed");
    return 0;
}
