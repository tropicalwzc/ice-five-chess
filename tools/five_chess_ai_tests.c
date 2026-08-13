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
        assert(fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                               1, false, &proof));
        assert(memcmp(before, board, sizeof(board)) == 0);

        FCProofResult missingBranch = proof;
        missingBranch.certificate[1].parent = -7;
        /* Recompute is intentionally impossible through the public API;
         * changing the DAG must invalidate its certificate identifier. */
        assert(!fc_verify_proof((const int (*)[FC_BOARD_SIZE])board,
                                1, false, &missingBranch));
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

    int counterWin[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    counterWin[5][7] = counterWin[6][7] = counterWin[7][7] = 1;
    counterWin[4][4] = counterWin[5][4] = counterWin[6][4] = -1;
    counterWin[7][4] = -1;
    assert(!fc_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])counterWin, 1, false,
        FC_PROOF_SEARCH_VCF, 7, 10000, 0, 4096, &noWin));
    assert(noWin.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE);
}

static void test_crossing_duplicate_and_forbidden_symmetries(void)
{
    int crossing[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    crossing[5][7] = crossing[6][7] = 1;
    crossing[7][5] = crossing[7][6] = 1;
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
    test_opening_book_reproducibility_and_variety();
    test_threat_enumeration_and_proof_certificate();
    test_proof_determinism_and_budget_unknown();
    test_table_driven_threat_patterns_all_symmetries();
    test_vcf_and_node_multi_defense_certificate();
    test_crossing_duplicate_and_forbidden_symmetries();
    test_vct_proof_and_integration_gates();
    puts("FiveChessAI tests passed");
    return 0;
}
