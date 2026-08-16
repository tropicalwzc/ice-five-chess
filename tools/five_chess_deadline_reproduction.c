#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "../ice five chess/FiveChessAI.h"

typedef struct {
    const char *name;
    bool forbidden;
    uint64_t master;
    int opening;
    int side;
    int hint_x;
    int hint_y;
    int move_count;
    int moves[16][3];
} Fixture;

static uint64_t mix64(uint64_t value)
{
    value += UINT64_C(0x9e3779b97f4a7c15);
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static uint64_t decision_seed(const Fixture *fixture)
{
    uint64_t value = fixture->master ^
        ((uint64_t)(fixture->opening + 1) << 32) ^
        ((uint64_t)(fixture->move_count + 1) * UINT64_C(0x9e3779b97f4a7c15)) ^
        (fixture->side == 1 ? UINT64_C(0x424c41434b) : UINT64_C(0x5748495445)) ^
        UINT64_C(0x4e4557);
    return mix64(value);
}

int main(void)
{
    const Fixture fixtures[] = {
        {
            "free-opening42-ply12", false, UINT64_C(0x882b8c2441e20eab),
            42, 1, 4, 8, 12,
            {{9,9,1},{9,7,-1},{9,4,1},{6,7,-1},{8,7,1},{8,8,-1},
             {7,9,1},{6,9,-1},{8,10,1},{6,8,-1},{6,10,1},{5,8,-1}}
        },
        {
            "forbidden-opening7-ply11", true, UINT64_C(0xe18dbabfada1c1f8),
            7, -1, 11, 11, 11,
            {{7,12,1},{7,13,-1},{9,13,1},{8,11,-1},{8,10,1},{6,10,-1},
             {9,12,1},{9,11,-1},{10,12,1},{8,12,-1},{10,10,1}}
        }
    };
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
    for (size_t fixture_index = 0;
         fixture_index < sizeof(fixtures) / sizeof(fixtures[0]);
         fixture_index++) {
        const Fixture *fixture = &fixtures[fixture_index];
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memset(board, 0, sizeof(board));
        for (int move = 0; move < fixture->move_count; move++)
            board[fixture->moves[move][0]][fixture->moves[move][1]] =
                fixture->moves[move][2];
        printf("fixture=%s seed=0x%016" PRIx64 "\n",
               fixture->name, decision_seed(fixture));
        for (int repetition = 0; repetition < 6; repetition++) {
            FCAnalysisResult result;
            bool found = fc_analyze_five_star_profile_with_hint(
                (const int (*)[FC_BOARD_SIZE])board,
                fixture->side, fixture->forbidden, &profile,
                decision_seed(fixture), FC_RANDOM_EVALUATION,
                fixture->hint_x, fixture->hint_y, &result);
            printf("rep=%d found=%d move=(%d,%d) four=(%d,%d) default=(%d,%d) "
                   "status=%d class=%d reason=%d corpus=%d corpusReason=%d "
                   "quiet=%d exhausted=%d elapsed=%.3f jobs=%d/%d\n",
                   repetition, found ? 1 : 0, result.x, result.y,
                   result.fourStarX, result.fourStarY,
                   result.defaultX, result.defaultY,
                   result.proofStatus, result.proofSearchClass,
                   result.overrideReason, result.corpusAccepted ? 1 : 0,
                   result.corpusReason, result.quietThreatSelected ? 1 : 0,
                   result.stats.budgetExhausted ? 1 : 0,
                   result.stats.elapsedMilliseconds,
                   result.proofParallelJobsCompleted,
                   result.proofParallelJobs);
        }
    }
    return 0;
}
