#include "../ice five chess/FiveChessAI.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double now_ms(void)
{
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return (double)value.tv_sec * 1000.0 +
           (double)value.tv_nsec / 1000000.0;
}

static void fill_position(int board[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    static const int stones[][3] = {
        {7,7,1}, {6,7,-1}, {8,7,1}, {7,6,-1}, {8,8,1},
        {6,6,-1}, {9,9,1}, {5,5,-1}, {9,7,1}, {5,7,-1},
        {7,9,1}, {7,5,-1}, {10,8,1}, {4,8,-1}
    };
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    for (size_t i = 0; i < sizeof(stones) / sizeof(stones[0]); i++)
        board[stones[i][0]][stones[i][1]] = stones[i][2];
}

static void assert_threats_equal(const FCThreat *left,
                                 int leftCount,
                                 const FCThreat *right,
                                 int rightCount)
{
    assert(leftCount == rightCount);
    assert(memcmp(left, right, (size_t)leftCount * sizeof(FCThreat)) == 0);
}

int main(void)
{
    const int iterations = 3;
    const int hashBatch = 16;
    double referenceTotal = 0.0;
    double incrementalTotal = 0.0;
    uint64_t sink = 0;
    for (int forbidden = 0; forbidden <= 1; forbidden++) {
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        fill_position(board);
        FCIncrementalPosition position;
        assert(fc_incremental_position_init(
            &position, (const int (*)[FC_BOARD_SIZE])board,
            forbidden != 0));
        FCThreat referenceThreats[FC_MAX_THREATS];
        FCThreat candidateThreats[FC_MAX_THREATS];
        bool referenceOverflow = false;
        bool candidateOverflow = false;
        int referenceCount = fc_enumerate_threats(
            (const int (*)[FC_BOARD_SIZE])board, 1, forbidden != 0,
            FC_PROOF_SEARCH_VCT, referenceThreats, FC_MAX_THREATS,
            &referenceOverflow);
        int candidateCount = fc_incremental_enumerate_threats(
            &position, 1, FC_PROOF_SEARCH_VCT, candidateThreats,
            FC_MAX_THREATS, &candidateOverflow);
        assert(referenceOverflow == candidateOverflow);
        assert_threats_equal(referenceThreats, referenceCount,
                             candidateThreats, candidateCount);
        for (int side = -1; side <= 1; side += 2) {
            for (int x = 0; x < FC_BOARD_SIZE; x++) {
                for (int y = 0; y < FC_BOARD_SIZE; y++) {
                    assert(fc_is_legal_move(
                        (const int (*)[FC_BOARD_SIZE])board, x, y, side,
                        forbidden != 0) == fc_incremental_is_legal_move(
                        &position, x, y, side));
                }
            }
        }
        for (int iteration = 0; iteration < iterations; iteration++) {
            FCThreat batchThreats[FC_MAX_THREATS];
            bool batchOverflow = false;
            double started = now_ms();
            for (int i = 0; i < hashBatch; i++)
                sink ^= fc_board_key(
                    (const int (*)[FC_BOARD_SIZE])board,
                    (i & 1) == 0 ? 1 : -1, forbidden != 0,
                    (i & 2) == 0 ? FC_PROOF_SEARCH_VCF
                                 : FC_PROOF_SEARCH_VCT,
                    (uint64_t)(10 + (i & 3)));
            for (int x = 0; x < FC_BOARD_SIZE; x++)
                for (int y = 0; y < FC_BOARD_SIZE; y++)
                    sink += fc_is_legal_move(
                        (const int (*)[FC_BOARD_SIZE])board, x, y, 1,
                        forbidden != 0);
            sink += (uint64_t)(fc_board_winner(
                (const int (*)[FC_BOARD_SIZE])board) + 1);
            sink += (uint64_t)fc_enumerate_threats(
                (const int (*)[FC_BOARD_SIZE])board, 1, forbidden != 0,
                FC_PROOF_SEARCH_VCT, batchThreats, FC_MAX_THREATS,
                &batchOverflow);
            referenceTotal += now_ms() - started;

            batchOverflow = false;
            started = now_ms();
            for (int i = 0; i < hashBatch; i++)
                sink ^= fc_incremental_board_key(
                    &position, (i & 1) == 0 ? 1 : -1,
                    (i & 2) == 0 ? FC_PROOF_SEARCH_VCF
                                 : FC_PROOF_SEARCH_VCT,
                    (uint64_t)(10 + (i & 3)));
            for (int x = 0; x < FC_BOARD_SIZE; x++)
                for (int y = 0; y < FC_BOARD_SIZE; y++)
                    sink += fc_incremental_is_legal_move(
                        &position, x, y, 1);
            sink += (uint64_t)(fc_board_winner(
                (const int (*)[FC_BOARD_SIZE])position.board) + 1);
            sink += (uint64_t)fc_incremental_enumerate_threats(
                &position, 1, FC_PROOF_SEARCH_VCT,
                batchThreats, FC_MAX_THREATS, &batchOverflow);
            incrementalTotal += now_ms() - started;
        }
    }
    double speedup = incrementalTotal > 0.0
        ? referenceTotal / incrementalTotal : 0.0;
    printf("{\"schemaVersion\":1,\"iterationsPerRule\":%d,"
           "\"incrementalPositionBytes\":%zu,"
           "\"hashesPerIteration\":%d,\"lineScansPerIteration\":1,"
           "\"threatBatchesPerIteration\":1,"
           "\"legalityPointsPerIteration\":%d,"
           "\"semanticEquality\":true,\"referenceMs\":%.6f,"
           "\"incrementalMs\":%.6f,\"speedup\":%.6f,"
           "\"sink\":\"0x%016llx\"}\n",
           iterations, sizeof(FCIncrementalPosition), hashBatch,
           FC_BOARD_SIZE * FC_BOARD_SIZE,
           referenceTotal, incrementalTotal, speedup,
           (unsigned long long)sink);
    return 0;
}
