#include "FiveChessAI.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint64_t key;
    int score;
    int depth;
    unsigned char flag;
} FCTTEntry;

typedef struct {
    FCAIProfile profile;
    bool forbiddenBlack;
    uint64_t nodes;
    uint64_t hits;
    uint64_t cutoffs;
    bool aborted;
    double startedMilliseconds;
    FCTTEntry *table;
    size_t tableCapacity;
} FCSearchContext;

typedef struct {
    uint64_t key;
    int depth;
    unsigned char status;
} FCProofTTEntry;

typedef struct {
    int attacker;
    bool forbiddenBlack;
    int searchClass;
    int maxDepth;
    uint64_t nodeBudget;
    uint32_t timeBudgetMs;
    double startedMilliseconds;
    uint64_t nodes;
    uint64_t hits;
    bool aborted;
    bool certificateOverflow;
    FCProofNode certificate[FC_MAX_PROOF_NODES];
    int certificateCount;
    FCProofTTEntry *table;
    size_t tableCapacity;
} FCProofContext;

#define FC_PROOF_INFINITY (UINT64_MAX / 4ULL)

static uint64_t fc_proof_saturated_add(uint64_t left, uint64_t right)
{
    if (left >= FC_PROOF_INFINITY || right >= FC_PROOF_INFINITY ||
        left > FC_PROOF_INFINITY - right) return FC_PROOF_INFINITY;
    return left + right;
}

enum {
    FC_TT_EXACT = 1,
    FC_TT_LOWER = 2,
    FC_TT_UPPER = 3
};

static const int fcDirections[4][2] = {
    {1, 0}, {0, 1}, {1, 1}, {1, -1}
};

#include "FiveChessOpeningBook.inc"

static double fc_now_milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static uint64_t fc_mix64(uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

void fc_random_seed(FCRandom *random, uint64_t seed)
{
    if (random == NULL) {
        return;
    }
    random->state = seed == 0 ? 0x6a09e667f3bcc909ULL : seed;
}

uint64_t fc_random_next(FCRandom *random)
{
    uint64_t x = random->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    random->state = x;
    return x * 2685821657736338717ULL;
}

double fc_random_unit(FCRandom *random)
{
    return (double)(fc_random_next(random) >> 11) * (1.0 / 9007199254740992.0);
}

static FCAIProfile fc_make_profile(const char *name,
                                   const char *version,
                                   int maxDepth,
                                   int qDepth,
                                   int fourDepth,
                                   int doubleThreeDepth,
                                   int forcingDepth,
                                   int candidateLimit,
                                   uint64_t nodeBudget,
                                   int nearBestWindow,
                                   double temperature,
                                   int maxRandomCandidates)
{
    FCAIProfile profile;
    memset(&profile, 0, sizeof(profile));
    profile.name = name;
    profile.version = version;
    profile.maxDepth = maxDepth;
    profile.quiescenceDepth = qDepth;
    profile.fourDepth = fourDepth;
    profile.doubleThreeDepth = doubleThreeDepth;
    profile.forcingDepth = forcingDepth;
    profile.candidateLimit = candidateLimit;
    profile.nodeBudget = nodeBudget;
    profile.timeBudgetMs = 900;
    profile.transpositionCapacity = 32768;
    profile.attackWeight = 100;
    profile.defenseWeight = 112;
    profile.centerWeight = 4;
    profile.nearBestWindow = nearBestWindow;
    profile.randomTemperature = temperature;
    profile.maxRandomCandidates = maxRandomCandidates;
    return profile;
}

FCAIProfile fc_profile_depth_8_8_10(void)
{
    return fc_make_profile("three-star-a0", "1.0.0-depth-8-8-10",
                           2, 2, 8, 8, 10, 10, 5000, 650, 520.0, 3);
}

FCAIProfile fc_profile_depth_10_10_12(void)
{
    return fc_make_profile("three-star-b1", "1.0.0-depth-10-10-12",
                           3, 3, 10, 10, 12, 12, 12000, 600, 480.0, 3);
}

FCAIProfile fc_profile_depth_12_12_14(void)
{
    return fc_make_profile("three-star-b2", "1.0.0-depth-12-12-14",
                           4, 3, 12, 12, 14, 14, 26000, 560, 440.0, 3);
}

FCAIProfile fc_profile_defense_c1(void)
{
    FCAIProfile profile = fc_make_profile("three-star-c1", "1.1.0-defense-130",
                                          3, 3, 10, 10, 12, 12,
                                          14000, 300, 250.0, 3);
    profile.defenseWeight = 130;
    profile.timeBudgetMs = 850;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_defense_c2(void)
{
    FCAIProfile profile = fc_make_profile("three-star-c2", "1.1.0-defense-140",
                                          3, 3, 10, 10, 12, 14,
                                          16000, 180, 180.0, 2);
    profile.attackWeight = 105;
    profile.defenseWeight = 140;
    profile.timeBudgetMs = 850;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_narrow_deep_d1(void)
{
    FCAIProfile profile = fc_make_profile("three-star-d1", "1.2.0-depth4-top8",
                                          4, 3, 12, 12, 14, 8,
                                          22000, 60, 45.0, 2);
    profile.timeBudgetMs = 850;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_narrow_deep_d2(void)
{
    FCAIProfile profile = fc_make_profile("three-star-d2", "1.2.0-depth4-top10",
                                          4, 4, 12, 12, 14, 10,
                                          24000, 60, 45.0, 2);
    profile.timeBudgetMs = 850;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_narrow_deep_d3(void)
{
    FCAIProfile profile = fc_make_profile("three-star-d3", "1.2.0-depth4-top8-1500ms",
                                          4, 3, 12, 12, 14, 8,
                                          30000, 60, 45.0, 2);
    profile.timeBudgetMs = 1500;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_production(void)
{
    FCAIProfile profile = fc_make_profile("three-star-production",
                                          "2.2.0-legacy-hint-safe-gate",
                                          3, 3, 10, 10, 12, 12,
                                          14000, 60, 45.0, 2);
    profile.timeBudgetMs = 850;
    profile.transpositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_proof_guided(bool openingBookEnabled)
{
    FCAIProfile profile = fc_profile_production();
    profile.name = openingBookEnabled
        ? "three-star-threat-proof-book"
        : "three-star-threat-proof-no-book";
    profile.version = openingBookEnabled
        ? "3.0.0-vcf-dfpn-book"
        : "3.0.0-vcf-dfpn-no-book";
    profile.proofEnabled = true;
    profile.openingBookEnabled = openingBookEnabled;
    profile.proofSearchClass = FC_PROOF_SEARCH_VCT;
    profile.proofMaxDepth = 10;
    profile.proofNodeBudget = 18000;
    profile.proofTimeBudgetMs = 160;
    profile.proofTranspositionCapacity = 32768;
    return profile;
}

static bool fc_inside(int x, int y)
{
    return x >= 0 && x < FC_BOARD_SIZE && y >= 0 && y < FC_BOARD_SIZE;
}

static int fc_count_one_way(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int x,
                            int y,
                            int dx,
                            int dy,
                            int side)
{
    int count = 0;
    x += dx;
    y += dy;
    while (fc_inside(x, y) && board[x][y] == side) {
        count++;
        x += dx;
        y += dy;
    }
    return count;
}

static int fc_line_length(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                          int x,
                          int y,
                          int dx,
                          int dy,
                          int side)
{
    return 1 + fc_count_one_way(board, x, y, dx, dy, side)
             + fc_count_one_way(board, x, y, -dx, -dy, side);
}

bool fc_has_five(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                 int x,
                 int y,
                 int side)
{
    if (!fc_inside(x, y) || board[x][y] != side) {
        return false;
    }
    for (int direction = 0; direction < 4; direction++) {
        if (fc_line_length(board, x, y,
                           fcDirections[direction][0],
                           fcDirections[direction][1], side) >= 5) {
            return true;
        }
    }
    return false;
}

static bool fc_wins_if_placed(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                              int x,
                              int y,
                              int side)
{
    if (!fc_inside(x, y) || board[x][y] != 0) {
        return false;
    }
    board[x][y] = side;
    bool wins = fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side);
    board[x][y] = 0;
    return wins;
}

static int fc_winning_directions_after_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                            int x,
                                            int y,
                                            int side)
{
    int directions = 0;
    for (int d = 0; d < 4; d++) {
        int dx = fcDirections[d][0];
        int dy = fcDirections[d][1];
        int continuationCount = 0;
        for (int offset = -4; offset <= 4; offset++) {
            int px = x + offset * dx;
            int py = y + offset * dy;
            if (!fc_inside(px, py) || board[px][py] != 0) {
                continue;
            }
            board[px][py] = side;
            if (fc_line_length((const int (*)[FC_BOARD_SIZE])board,
                               px, py, dx, dy, side) >= 5) {
                continuationCount++;
            }
            board[px][py] = 0;
        }
        if (continuationCount > 0) {
            directions++;
        }
    }
    return directions;
}

bool fc_is_legal_move(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                      int x,
                      int y,
                      int side,
                      bool forbiddenBlack)
{
    if (!fc_inside(x, y) || board[x][y] != 0 || (side != 1 && side != -1)) {
        return false;
    }
    if (!forbiddenBlack || side != 1) {
        return true;
    }

    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    mutableBoard[x][y] = side;
    for (int d = 0; d < 4; d++) {
        if (fc_line_length((const int (*)[FC_BOARD_SIZE])mutableBoard, x, y,
                           fcDirections[d][0], fcDirections[d][1], side) > 5) {
            return false;
        }
    }
    if (fc_has_five((const int (*)[FC_BOARD_SIZE])mutableBoard, x, y, side)) {
        return true;
    }
    return fc_winning_directions_after_move(mutableBoard, x, y, side) < 2;
}

int fc_board_winner(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            int side = board[x][y];
            if (side != 0 && fc_has_five(board, x, y, side)) {
                return side;
            }
        }
    }
    return 0;
}

bool fc_make_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                  int x,
                  int y,
                  int side,
                  bool forbiddenBlack)
{
    if (board == NULL ||
        !fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                          x, y, side, forbiddenBlack)) {
        return false;
    }
    board[x][y] = side;
    return true;
}

void fc_unmake_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE], int x, int y)
{
    if (board != NULL && fc_inside(x, y)) board[x][y] = 0;
}

void fc_transform_point(int transform, int x, int y, int *outX, int *outY)
{
    int tx = x;
    int ty = y;
    int n = FC_BOARD_SIZE - 1;
    switch (transform & 7) {
        case 0: tx = x;     ty = y;     break;
        case 1: tx = y;     ty = n - x; break;
        case 2: tx = n - x; ty = n - y; break;
        case 3: tx = n - y; ty = x;     break;
        case 4: tx = n - x; ty = y;     break;
        case 5: tx = x;     ty = n - y; break;
        case 6: tx = y;     ty = x;     break;
        default: tx = n - y; ty = n - x; break;
    }
    if (outX != NULL) *outX = tx;
    if (outY != NULL) *outY = ty;
}

uint64_t fc_board_key(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                      int side,
                      bool forbiddenBlack,
                      int searchClass,
                      uint64_t profileVersion)
{
    uint64_t key = 0x243f6a8885a308d3ULL;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            int stone = board[x][y];
            if (stone == 0) continue;
            uint64_t value = (uint64_t)(x * FC_BOARD_SIZE + y + 1)
                           | ((uint64_t)(stone == 1 ? 1 : 2) << 16);
            key ^= fc_mix64(value ^ 0x9e3779b97f4a7c15ULL);
        }
    }
    key ^= fc_mix64((uint64_t)(side == 1 ? 1 : 2) << 32);
    key ^= fc_mix64((uint64_t)(forbiddenBlack ? 1 : 0) << 40);
    key ^= fc_mix64((uint64_t)(searchClass & 0xff) << 48);
    key ^= fc_mix64((uint64_t)FC_BOARD_SIZE << 56);
    key ^= fc_mix64(profileVersion);
    return key;
}

const char *fc_opening_book_version(void)
{
    return "gomocup-2024-2025-freestyle15-v2";
}

bool fc_opening_book_lookup(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int side,
                            bool forbiddenBlack,
                            uint64_t seed,
                            int *outX,
                            int *outY,
                            int *outBookId,
                            int *outBookPly)
{
    if (board == NULL || forbiddenBlack ||
        (side != 1 && side != -1)) return false;
    int stoneCount = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++)
        for (int y = 0; y < FC_BOARD_SIZE; y++)
            if (board[x][y] != 0) stoneCount++;
    if ((stoneCount % 2 == 0 ? 1 : -1) != side) return false;

    typedef struct { int x; int y; int id; int ply; } FCBookChoice;
    FCBookChoice choices[96];
    int choiceCount = 0;
    for (int lineId = 0; lineId < fcOpeningLineCount; lineId++) {
        const FCOpeningLine *line = &fcOpeningLines[lineId];
        if (stoneCount >= line->length) continue;
        for (int transform = 0; transform < 8; transform++) {
            bool matches = true;
            for (int ply = 0; ply < stoneCount; ply++) {
                int rawX = 7 + line->coordinates[ply * 2];
                int rawY = 7 + line->coordinates[ply * 2 + 1];
                int x = 0, y = 0;
                fc_transform_point(transform, rawX, rawY, &x, &y);
                int expected = ply % 2 == 0 ? 1 : -1;
                if (!fc_inside(x, y) || board[x][y] != expected) {
                    matches = false;
                    break;
                }
            }
            if (!matches) continue;
            int rawX = 7 + line->coordinates[stoneCount * 2];
            int rawY = 7 + line->coordinates[stoneCount * 2 + 1];
            int x = 0, y = 0;
            fc_transform_point(transform, rawX, rawY, &x, &y);
            if (!fc_is_legal_move(board, x, y, side, false)) continue;
            bool duplicate = false;
            for (int c = 0; c < choiceCount; c++) {
                if (choices[c].x == x && choices[c].y == y) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate && choiceCount < (int)(sizeof(choices)/sizeof(choices[0]))) {
                choices[choiceCount++] = (FCBookChoice){
                    x, y, lineId * 8 + transform, stoneCount
                };
            }
        }
    }
    if (choiceCount == 0) return false;
    FCRandom random;
    fc_random_seed(&random, seed ^ 0x4f50454e494e4755ULL);
    int selected = (int)(fc_random_next(&random) % (uint64_t)choiceCount);
    if (outX != NULL) *outX = choices[selected].x;
    if (outY != NULL) *outY = choices[selected].y;
    if (outBookId != NULL) *outBookId = choices[selected].id;
    if (outBookPly != NULL) *outBookPly = choices[selected].ply;
    return true;
}

static int fc_open_ends(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                        int x,
                        int y,
                        int dx,
                        int dy,
                        int side)
{
    int forward = fc_count_one_way(board, x, y, dx, dy, side);
    int backward = fc_count_one_way(board, x, y, -dx, -dy, side);
    int ends = 0;
    int fx = x + (forward + 1) * dx;
    int fy = y + (forward + 1) * dy;
    int bx = x - (backward + 1) * dx;
    int by = y - (backward + 1) * dy;
    if (fc_inside(fx, fy) && board[fx][fy] == 0) {
        ends++;
    }
    if (fc_inside(bx, by) && board[bx][by] == 0) {
        ends++;
    }
    return ends;
}

static int fc_shape_value(int length, int openEnds)
{
    if (length >= 5) return FC_WIN_SCORE / 2;
    if (length == 4 && openEnds == 2) return 180000;
    if (length == 4 && openEnds == 1) return 42000;
    if (length == 3 && openEnds == 2) return 13000;
    if (length == 3 && openEnds == 1) return 2600;
    if (length == 2 && openEnds == 2) return 900;
    if (length == 2 && openEnds == 1) return 180;
    if (length == 1 && openEnds == 2) return 40;
    return 4;
}

static int fc_score_stone(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                          int x,
                          int y,
                          int side)
{
    int score = 0;
    for (int d = 0; d < 4; d++) {
        int dx = fcDirections[d][0];
        int dy = fcDirections[d][1];
        int length = fc_line_length(board, x, y, dx, dy, side);
        int openEnds = fc_open_ends(board, x, y, dx, dy, side);
        score += fc_shape_value(length, openEnds);
    }
    return score;
}

static int fc_window_value(int stones)
{
    if (stones >= 5) return FC_WIN_SCORE / 2;
    if (stones == 4) return 150000;
    if (stones == 3) return 9000;
    if (stones == 2) return 650;
    if (stones == 1) return 24;
    return 0;
}

static int fc_window_score(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                           int side)
{
    int score = 0;
    for (int d = 0; d < 4; d++) {
        int dx = fcDirections[d][0];
        int dy = fcDirections[d][1];
        for (int x = 0; x < FC_BOARD_SIZE; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                int endX = x + 4 * dx;
                int endY = y + 4 * dy;
                if (!fc_inside(endX, endY)) continue;
                int stones = 0;
                bool blocked = false;
                for (int offset = 0; offset < 5; offset++) {
                    int value = board[x + offset * dx][y + offset * dy];
                    if (value == -side) {
                        blocked = true;
                        break;
                    }
                    stones += value == side;
                }
                if (!blocked) score += fc_window_value(stones);
            }
        }
    }
    return score;
}

static int fc_local_window_score(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                 int x,
                                 int y,
                                 int side)
{
    int score = 0;
    for (int d = 0; d < 4; d++) {
        int dx = fcDirections[d][0];
        int dy = fcDirections[d][1];
        for (int startOffset = -4; startOffset <= 0; startOffset++) {
            int startX = x + startOffset * dx;
            int startY = y + startOffset * dy;
            int endX = startX + 4 * dx;
            int endY = startY + 4 * dy;
            if (!fc_inside(startX, startY) || !fc_inside(endX, endY)) continue;
            int stones = 0;
            bool blocked = false;
            for (int offset = 0; offset < 5; offset++) {
                int value = board[startX + offset * dx][startY + offset * dy];
                if (value == -side) {
                    blocked = true;
                    break;
                }
                stones += value == side;
            }
            if (!blocked) score += fc_window_value(stones);
        }
    }
    return score;
}

static int fc_clamp_heuristic(int64_t value)
{
    const int64_t limit = FC_WIN_SCORE - 1000;
    if (value > limit) return (int)limit;
    if (value < -limit) return (int)-limit;
    return (int)value;
}

static int fc_move_heuristic(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                             int x,
                             int y,
                             int side,
                             const FCAIProfile *profile)
{
    if (board[x][y] != 0) {
        return -FC_WIN_SCORE;
    }
    board[x][y] = side;
    int attack = fc_score_stone((const int (*)[FC_BOARD_SIZE])board, x, y, side);
    attack += fc_local_window_score((const int (*)[FC_BOARD_SIZE])board,
                                    x, y, side);
    board[x][y] = -side;
    int defense = fc_score_stone((const int (*)[FC_BOARD_SIZE])board, x, y, -side);
    defense += fc_local_window_score((const int (*)[FC_BOARD_SIZE])board,
                                     x, y, -side);
    board[x][y] = 0;
    int center = 14 - abs(x - 7) - abs(y - 7);
    int64_t value = (int64_t)attack * profile->attackWeight / 100
                  + (int64_t)defense * profile->defenseWeight / 100
                  + (int64_t)center * profile->centerWeight;
    return fc_clamp_heuristic(value);
}

static int fc_evaluate_board(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                             int side,
                             const FCAIProfile *profile)
{
    int own = fc_window_score(board, side);
    int rival = fc_window_score(board, -side);
    int64_t value = (int64_t)own * profile->attackWeight / 100
                  - (int64_t)rival * profile->defenseWeight / 100;
    return fc_clamp_heuristic(value);
}

static int fc_board_stone_count(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    int count = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            count += board[x][y] != 0;
        }
    }
    return count;
}

static bool fc_has_neighbor(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int x,
                            int y,
                            int radius)
{
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            if (dx == 0 && dy == 0) continue;
            int px = x + dx;
            int py = y + dy;
            if (fc_inside(px, py) && board[px][py] != 0) {
                return true;
            }
        }
    }
    return false;
}

static int fc_compare_candidate(const void *left, const void *right)
{
    const FCCandidate *a = (const FCCandidate *)left;
    const FCCandidate *b = (const FCCandidate *)right;
    int aRank = a->tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ? 0
              : a->tacticalClass == FC_TACTICAL_MUST_DEFEND ? 1
              : a->tacticalClass == FC_TACTICAL_FORCED_ATTACK ? 2
              : a->tacticalClass == FC_TACTICAL_FORCED_DEFENSE ? 3 : 4;
    int bRank = b->tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ? 0
              : b->tacticalClass == FC_TACTICAL_MUST_DEFEND ? 1
              : b->tacticalClass == FC_TACTICAL_FORCED_ATTACK ? 2
              : b->tacticalClass == FC_TACTICAL_FORCED_DEFENSE ? 3 : 4;
    if (aRank != bRank) {
        return aRank < bRank ? -1 : 1;
    }
    if (a->score != b->score) {
        return a->score > b->score ? -1 : 1;
    }
    if (a->x != b->x) return a->x < b->x ? -1 : 1;
    if (a->y != b->y) return a->y < b->y ? -1 : 1;
    return 0;
}

/* Once a complete search iteration has produced scores, the deeper result is
 * authoritative.  Tactical class remains a stable tie breaker only. */
static int fc_compare_searched_candidate(const void *left, const void *right)
{
    const FCCandidate *a = (const FCCandidate *)left;
    const FCCandidate *b = (const FCCandidate *)right;
    if (a->score != b->score) {
        return a->score > b->score ? -1 : 1;
    }
    return fc_compare_candidate(left, right);
}

static int fc_count_immediate_wins(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                   int side,
                                   bool forbiddenBlack,
                                   FCCandidate *moves,
                                   int capacity)
{
    int count = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack)) {
                continue;
            }
            if (fc_wins_if_placed(board, x, y, side)) {
                if (moves != NULL && count < capacity) {
                    moves[count].x = x;
                    moves[count].y = y;
                    moves[count].score = FC_WIN_SCORE - count;
                    moves[count].tacticalClass = FC_TACTICAL_IMMEDIATE_WIN;
                    moves[count].safe = true;
                    moves[count].probability = 0.0;
                }
                count++;
            }
        }
    }
    return count;
}

static bool fc_same_point(FCPoint a, FCPoint b)
{
    return a.x == b.x && a.y == b.y;
}

static bool fc_add_point(FCPoint *points,
                         int *count,
                         int capacity,
                         int x,
                         int y)
{
    FCPoint point = {x, y};
    for (int i = 0; i < *count; i++) {
        if (fc_same_point(points[i], point)) return true;
    }
    if (*count >= capacity) return false;
    points[(*count)++] = point;
    return true;
}

static int fc_count_local_immediate_wins(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int originX,
    int originY,
    int side,
    bool forbiddenBlack,
    FCCandidate *moves,
    int capacity)
{
    FCPoint checked[4 * 9];
    int checkedCount = 0;
    int count = 0;
    for (int direction = 0; direction < 4; direction++) {
        int dx = fcDirections[direction][0];
        int dy = fcDirections[direction][1];
        for (int offset = -4; offset <= 4; offset++) {
            if (offset == 0) continue;
            int x = originX + offset * dx;
            int y = originY + offset * dy;
            if (!fc_inside(x, y)) continue;
            bool duplicate = false;
            for (int i = 0; i < checkedCount; i++) {
                if (checked[i].x == x && checked[i].y == y) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
            checked[checkedCount++] = (FCPoint){x, y};
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack) ||
                !fc_wins_if_placed(board, x, y, side)) continue;
            if (moves != NULL && count < capacity) {
                moves[count] = (FCCandidate){
                    x, y, FC_WIN_SCORE - count,
                    FC_TACTICAL_IMMEDIATE_WIN, true, 0.0
                };
            }
            count++;
        }
    }
    return count;
}

static int fc_count_open_four_creators(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                       int side,
                                       bool forbiddenBlack,
                                       FCPoint *points,
                                       int capacity,
                                       bool *overflow)
{
    int count = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack)) continue;
            board[x][y] = side;
            int wins = fc_count_local_immediate_wins(
                board, x, y, side, forbiddenBlack, NULL, 0);
            board[x][y] = 0;
            if (wins < 2) continue;
            if (points != NULL && count < capacity) {
                points[count] = (FCPoint){x, y};
            } else if (points != NULL && overflow != NULL) {
                *overflow = true;
            }
            count++;
        }
    }
    return count;
}

static int fc_count_local_open_four_creators(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int originX,
    int originY,
    int side,
    bool forbiddenBlack,
    FCPoint *points,
    int capacity,
    bool *overflow)
{
    FCPoint checked[4 * 9];
    int checkedCount = 0;
    int count = 0;
    for (int direction = 0; direction < 4; direction++) {
        int dx = fcDirections[direction][0];
        int dy = fcDirections[direction][1];
        for (int offset = -4; offset <= 4; offset++) {
            if (offset == 0) continue;
            int x = originX + offset * dx;
            int y = originY + offset * dy;
            if (!fc_inside(x, y)) continue;
            bool duplicate = false;
            for (int i = 0; i < checkedCount; i++) {
                if (checked[i].x == x && checked[i].y == y) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
            checked[checkedCount++] = (FCPoint){x, y};
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack)) continue;
            board[x][y] = side;
            int wins = fc_count_local_immediate_wins(
                board, x, y, side, forbiddenBlack, NULL, 0);
            board[x][y] = 0;
            if (wins < 2) continue;
            if (points != NULL && count < capacity)
                points[count] = (FCPoint){x, y};
            else if (points != NULL && overflow != NULL)
                *overflow = true;
            count++;
        }
    }
    return count;
}

static int fc_count_surviving_creators(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCPoint *points,
    int pointCount)
{
    int count = 0;
    for (int i = 0; i < pointCount; i++) {
        int x = points[i].x;
        int y = points[i].y;
        if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                              x, y, side, forbiddenBlack)) continue;
        board[x][y] = side;
        int wins = fc_count_local_immediate_wins(
            board, x, y, side, forbiddenBlack, NULL, 0);
        board[x][y] = 0;
        if (wins >= 2) count++;
    }
    return count;
}

static bool fc_has_vct_root_threat(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack)
{
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_has_neighbor((const int (*)[FC_BOARD_SIZE])board,
                                 x, y, 4) ||
                !fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack)) continue;
            board[x][y] = side;
            int creators = fc_count_local_open_four_creators(
                board, x, y, side, forbiddenBlack, NULL, 0, NULL);
            board[x][y] = 0;
            if (creators > 0) return true;
        }
    }
    return false;
}

static int fc_compare_threat(const void *left, const void *right)
{
    const FCThreat *a = (const FCThreat *)left;
    const FCThreat *b = (const FCThreat *)right;
    if (a->severity != b->severity) return a->severity > b->severity ? -1 : 1;
    if (a->costCount != b->costCount) return a->costCount < b->costCount ? -1 : 1;
    if (a->gain.x != b->gain.x) return a->gain.x < b->gain.x ? -1 : 1;
    return a->gain.y < b->gain.y ? -1 : a->gain.y > b->gain.y;
}

int fc_enumerate_threats(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int side,
                         bool forbiddenBlack,
                         int searchClass,
                         FCThreat *out,
                         int capacity,
                         bool *overflow)
{
    if (overflow != NULL) *overflow = false;
    if (board == NULL || out == NULL || capacity <= 0 ||
        (side != 1 && side != -1)) return 0;
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    FCCandidate existingWins[FC_MAX_CANDIDATES];
    int existingWinCount = fc_count_immediate_wins(
        mutableBoard, side, forbiddenBlack,
        existingWins, FC_MAX_CANDIDATES);
    FCPoint existingCreators[FC_MAX_THREAT_POINTS];
    int existingCreatorCount = 0;
    if (existingWinCount == 0 && searchClass >= FC_PROOF_SEARCH_VCT) {
        existingCreatorCount = fc_count_open_four_creators(
            mutableBoard, side, forbiddenBlack,
            existingCreators, FC_MAX_THREAT_POINTS, NULL);
        if (existingCreatorCount > FC_MAX_THREAT_POINTS)
            existingCreatorCount = FC_MAX_THREAT_POINTS;
    }
    int count = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])mutableBoard,
                                  x, y, side, forbiddenBlack)) continue;
            mutableBoard[x][y] = side;
            FCThreat threat;
            memset(&threat, 0, sizeof(threat));
            threat.gain = (FCPoint){x, y};
            threat.side = side;
            if (fc_has_five((const int (*)[FC_BOARD_SIZE])mutableBoard,
                            x, y, side)) {
                threat.severity = FC_THREAT_FIVE;
            } else if (existingWinCount > 0) {
                /* A different move cannot be part of a forcing proof when an
                 * immediate win was already available before the gain. */
                mutableBoard[x][y] = 0;
                continue;
            } else {
                FCCandidate wins[FC_MAX_CANDIDATES];
                int winCount = fc_count_local_immediate_wins(
                    mutableBoard, x, y, side, forbiddenBlack,
                    wins, FC_MAX_CANDIDATES);
                for (int i = 0; i < winCount && i < FC_MAX_CANDIDATES; i++) {
                    if (!fc_add_point(threat.costs, &threat.costCount,
                                      FC_MAX_THREAT_POINTS,
                                      wins[i].x, wins[i].y) && overflow != NULL) {
                        *overflow = true;
                    }
                }
                if (winCount >= 2) threat.severity = FC_THREAT_OPEN_FOUR;
                else if (winCount == 1) threat.severity = FC_THREAT_FOUR;
                if (searchClass >= FC_PROOF_SEARCH_VCT && winCount <= 1) {
                    bool restOverflow = false;
                    int creators = fc_count_local_open_four_creators(
                        mutableBoard, x, y, side, forbiddenBlack,
                        threat.rests, FC_MAX_THREAT_POINTS, &restOverflow);
                    int newRestCount = 0;
                    int storedCreators = creators < FC_MAX_THREAT_POINTS
                        ? creators : FC_MAX_THREAT_POINTS;
                    for (int rest = 0; rest < storedCreators; rest++) {
                        bool existed = false;
                        for (int old = 0; old < existingCreatorCount; old++) {
                            if (fc_same_point(threat.rests[rest],
                                              existingCreators[old])) {
                                existed = true;
                                break;
                            }
                        }
                        if (!existed) threat.rests[newRestCount++] =
                            threat.rests[rest];
                    }
                    creators = newRestCount;
                    threat.restCount = newRestCount;
                    if (restOverflow && overflow != NULL) *overflow = true;
                    if (winCount == 1 && creators > 0)
                        threat.severity = FC_THREAT_FOUR_THREE;
                    else if (winCount == 0 && creators >= 2)
                        threat.severity = FC_THREAT_FOUR_THREE;
                    else if (winCount == 0 && creators == 1)
                        threat.severity = FC_THREAT_OPEN_THREE;
                    if (winCount == 0 && threat.severity != FC_THREAT_NONE) {
                        for (int dx = 0; dx < FC_BOARD_SIZE; dx++) {
                            for (int dy = 0; dy < FC_BOARD_SIZE; dy++) {
                                if (!fc_is_legal_move(
                                        (const int (*)[FC_BOARD_SIZE])mutableBoard,
                                        dx, dy, -side, forbiddenBlack)) continue;
                                mutableBoard[dx][dy] = -side;
                                bool defenderWins = fc_has_five(
                                    (const int (*)[FC_BOARD_SIZE])mutableBoard,
                                    dx, dy, -side);
                                int remaining = defenderWins ? 0
                                    : fc_count_open_four_creators(
                                        mutableBoard, side, forbiddenBlack,
                                        NULL, 0, NULL);
                                mutableBoard[dx][dy] = 0;
                                if (!defenderWins && remaining > 0) continue;
                                if (!fc_add_point(threat.costs,
                                                  &threat.costCount,
                                                  FC_MAX_THREAT_POINTS,
                                                  dx, dy) && overflow != NULL) {
                                    *overflow = true;
                                }
                            }
                        }
                    }
                }
            }
            mutableBoard[x][y] = 0;
            if (threat.severity == FC_THREAT_NONE) continue;
            if (count < capacity) out[count] = threat;
            else if (overflow != NULL) *overflow = true;
            count++;
        }
    }
    int stored = count < capacity ? count : capacity;
    qsort(out, (size_t)stored, sizeof(FCThreat), fc_compare_threat);
    return stored;
}

static bool fc_proof_budget_available(FCProofContext *context)
{
    if (context->nodes >= context->nodeBudget) {
        context->aborted = true;
        return false;
    }
    if (context->timeBudgetMs > 0 &&
        fc_now_milliseconds() - context->startedMilliseconds >=
            context->timeBudgetMs) {
        context->aborted = true;
        return false;
    }
    return true;
}

static int fc_proof_append(FCProofContext *context,
                           uint64_t boardHash,
                           int parent,
                           int x,
                           int y,
                           int side,
                           bool terminalWin)
{
    if (context->certificateCount >= FC_MAX_PROOF_NODES) {
        context->certificateOverflow = true;
        return -1;
    }
    int index = context->certificateCount++;
    context->certificate[index] = (FCProofNode){
        boardHash, parent, x, y, side, terminalWin
    };
    return index;
}

static int fc_generate_refutations(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                   int attacker,
                                   bool forbiddenBlack,
                                   int searchClass,
                                   FCPoint *out,
                                   int capacity)
{
    FCCandidate immediatePoints[FC_MAX_CANDIDATES];
    int immediate = fc_count_immediate_wins(
        board, attacker, forbiddenBlack,
        immediatePoints, FC_MAX_CANDIDATES);
    FCPoint creatorPoints[FC_MAX_THREAT_POINTS];
    int creators = immediate == 0 && searchClass >= FC_PROOF_SEARCH_VCT
        ? fc_count_open_four_creators(board, attacker, forbiddenBlack,
                                      creatorPoints,
                                      FC_MAX_THREAT_POINTS, NULL)
        : 0;
    int storedCreators = creators < FC_MAX_THREAT_POINTS
        ? creators : FC_MAX_THREAT_POINTS;
    int count = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, -attacker, forbiddenBlack)) continue;
            board[x][y] = -attacker;
            bool defenderWins = fc_has_five(
                (const int (*)[FC_BOARD_SIZE])board, x, y, -attacker);
            int defenderCounterWins = defenderWins ? 0
                : fc_count_local_immediate_wins(
                    board, x, y, -attacker, forbiddenBlack, NULL, 0);
            int remaining = 0;
            if (!defenderWins && immediate > 0) {
                remaining = fc_count_immediate_wins(
                    board, attacker, forbiddenBlack, NULL, 0);
            } else if (!defenderWins && creators > 0) {
                remaining = creators <= FC_MAX_THREAT_POINTS
                    ? fc_count_surviving_creators(
                        board, attacker, forbiddenBlack,
                        creatorPoints, storedCreators)
                    : fc_count_open_four_creators(
                        board, attacker, forbiddenBlack, NULL, 0, NULL);
            }
            board[x][y] = 0;
            bool blocksThreatPoint = false;
            for (int i = 0; i < immediate && i < FC_MAX_CANDIDATES; i++) {
                if (immediatePoints[i].x == x && immediatePoints[i].y == y) {
                    blocksThreatPoint = true;
                    break;
                }
            }
            for (int i = 0; !blocksThreatPoint && i < storedCreators; i++) {
                if (creatorPoints[i].x == x && creatorPoints[i].y == y) {
                    blocksThreatPoint = true;
                    break;
                }
            }
            bool validCounterThreat = immediate == 0 && creators > 0 &&
                                      defenderCounterWins > 0;
            bool removesAllThreats = remaining == 0;
            if (!defenderWins && !validCounterThreat && !blocksThreatPoint &&
                !removesAllThreats)
                continue;
            if (count < capacity) out[count] = (FCPoint){x, y};
            count++;
        }
    }
    return count < capacity ? count : capacity;
}

static int fc_proof_search_attacker(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                    int remainingDepth,
                                    int parent,
                                    FCProofContext *context,
                                    int *distance,
                                    uint64_t *proofNumber,
                                    uint64_t *disproofNumber)
{
    if (proofNumber != NULL) *proofNumber = 1;
    if (disproofNumber != NULL) *disproofNumber = 1;
    if (!fc_proof_budget_available(context)) return FC_PROOF_UNKNOWN;
    context->nodes++;
    if (remainingDepth <= 0) {
        if (proofNumber != NULL) *proofNumber = FC_PROOF_INFINITY;
        if (disproofNumber != NULL) *disproofNumber = 0;
        return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }

    /* A forcing line cannot ignore an immediate defender win.  The attacker
     * may continue only when it can end the game on this move. */
    int ownImmediate = fc_count_immediate_wins(
        board, context->attacker, context->forbiddenBlack, NULL, 0);
    int defenderImmediate = fc_count_immediate_wins(
        board, -context->attacker, context->forbiddenBlack, NULL, 0);
    if (defenderImmediate > 0 && ownImmediate == 0) {
        if (proofNumber != NULL) *proofNumber = FC_PROOF_INFINITY;
        if (disproofNumber != NULL) *disproofNumber = 0;
        return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }

    uint64_t key = fc_board_key(
        (const int (*)[FC_BOARD_SIZE])board, context->attacker,
        context->forbiddenBlack, context->searchClass,
        (uint64_t)context->maxDepth);
    FCProofTTEntry *entry = NULL;
    if (context->tableCapacity > 0) {
        entry = &context->table[key % context->tableCapacity];
        if (entry->key == key && entry->depth >= remainingDepth) {
            context->hits++;
            if (entry->status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
                if (proofNumber != NULL) *proofNumber = FC_PROOF_INFINITY;
                if (disproofNumber != NULL) *disproofNumber = 0;
                return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
            }
        }
    }

    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int threatCount = fc_enumerate_threats(
        (const int (*)[FC_BOARD_SIZE])board, context->attacker,
        context->forbiddenBlack, context->searchClass,
        threats, FC_MAX_THREATS, &overflow);
    bool sawUnknown = overflow;
    int bestDistance = 0;
    uint64_t rootProof = FC_PROOF_INFINITY;
    uint64_t rootDisproof = 0;
    for (int i = 0; i < threatCount; i++) {
        FCThreat *threat = &threats[i];
        int checkpoint = context->certificateCount;
        uint64_t beforeHash = fc_board_key(
            (const int (*)[FC_BOARD_SIZE])board, context->attacker,
            context->forbiddenBlack, context->searchClass,
            (uint64_t)context->maxDepth);
        int attackNode = fc_proof_append(
            context, beforeHash, parent, threat->gain.x, threat->gain.y,
            context->attacker, false);
        if (attackNode < 0) return FC_PROOF_UNKNOWN;
        if (!fc_make_move(board, threat->gain.x, threat->gain.y,
                          context->attacker, context->forbiddenBlack)) {
            context->certificateCount = checkpoint;
            continue;
        }
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                        threat->gain.x, threat->gain.y, context->attacker)) {
            context->certificate[attackNode].terminalWin = true;
            fc_unmake_move(board, threat->gain.x, threat->gain.y);
            if (distance != NULL) *distance = 1;
            if (proofNumber != NULL) *proofNumber = 0;
            if (disproofNumber != NULL) *disproofNumber = FC_PROOF_INFINITY;
            return FC_PROOF_PROVEN_WIN;
        }

        FCPoint refutations[FC_BOARD_SIZE * FC_BOARD_SIZE];
        int refutationCount = fc_generate_refutations(
            board, context->attacker, context->forbiddenBlack,
            context->searchClass, refutations,
            FC_BOARD_SIZE * FC_BOARD_SIZE);
        bool allProven = true;
        int maximumChildDistance = 0;
        uint64_t edgeProof = 0;
        uint64_t edgeDisproof = FC_PROOF_INFINITY;
        for (int r = 0; r < refutationCount; r++) {
            FCPoint reply = refutations[r];
            uint64_t replyHash = fc_board_key(
                (const int (*)[FC_BOARD_SIZE])board, -context->attacker,
                context->forbiddenBlack, context->searchClass,
                (uint64_t)context->maxDepth);
            int defenseNode = fc_proof_append(
                context, replyHash, attackNode, reply.x, reply.y,
                -context->attacker, false);
            if (defenseNode < 0) {
                allProven = false;
                sawUnknown = true;
                edgeProof = fc_proof_saturated_add(edgeProof, 1);
                if (edgeDisproof > 1) edgeDisproof = 1;
                break;
            }
            if (!fc_make_move(board, reply.x, reply.y, -context->attacker,
                              context->forbiddenBlack)) {
                allProven = false;
                break;
            }
            if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                            reply.x, reply.y, -context->attacker)) {
                fc_unmake_move(board, reply.x, reply.y);
                allProven = false;
                edgeProof = FC_PROOF_INFINITY;
                edgeDisproof = 0;
                break;
            }
            int childDistance = 0;
            uint64_t childProof = 1;
            uint64_t childDisproof = 1;
            int child = fc_proof_search_attacker(
                board, remainingDepth - 2, defenseNode,
                context, &childDistance, &childProof, &childDisproof);
            fc_unmake_move(board, reply.x, reply.y);
            edgeProof = fc_proof_saturated_add(edgeProof, childProof);
            if (childDisproof < edgeDisproof)
                edgeDisproof = childDisproof;
            if (child != FC_PROOF_PROVEN_WIN) {
                if (child == FC_PROOF_UNKNOWN) sawUnknown = true;
                allProven = false;
                break;
            }
            if (childDistance > maximumChildDistance)
                maximumChildDistance = childDistance;
        }
        fc_unmake_move(board, threat->gain.x, threat->gain.y);
        if (allProven) {
            bestDistance = refutationCount == 0 ? 3
                         : maximumChildDistance + 2;
            if (distance != NULL) *distance = bestDistance;
            if (proofNumber != NULL) *proofNumber = 0;
            if (disproofNumber != NULL) *disproofNumber = FC_PROOF_INFINITY;
            return FC_PROOF_PROVEN_WIN;
        }
        if (edgeProof < rootProof) rootProof = edgeProof;
        rootDisproof = fc_proof_saturated_add(rootDisproof, edgeDisproof);
        context->certificateCount = checkpoint;
        if (context->aborted || context->certificateOverflow)
            return FC_PROOF_UNKNOWN;
    }
    if (threatCount == 0 && !sawUnknown) {
        rootProof = FC_PROOF_INFINITY;
        rootDisproof = 0;
    }
    if (proofNumber != NULL) *proofNumber = sawUnknown || context->aborted
        ? (rootProof == FC_PROOF_INFINITY ? 1 : rootProof) : rootProof;
    if (disproofNumber != NULL) *disproofNumber = sawUnknown || context->aborted
        ? (rootDisproof == 0 ? 1 : rootDisproof) : rootDisproof;
    if (entry != NULL && !sawUnknown && !context->aborted) {
        entry->key = key;
        entry->depth = remainingDepth;
        entry->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }
    return sawUnknown || context->aborted
        ? FC_PROOF_UNKNOWN : FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
}

static uint64_t fc_certificate_id(const FCProofNode *nodes, int count)
{
    uint64_t value = 0x13198a2e03707344ULL;
    for (int i = 0; i < count; i++) {
        value ^= fc_mix64(nodes[i].boardHash
            ^ ((uint64_t)(nodes[i].x + 1) << 8)
            ^ ((uint64_t)(nodes[i].y + 1) << 16)
            ^ ((uint64_t)(nodes[i].side + 2) << 24)
            ^ ((uint64_t)(nodes[i].parent + 2) << 32)
            ^ ((uint64_t)nodes[i].terminalWin << 63));
    }
    return value;
}

static int fc_direct_children(const FCProofResult *result,
                              int parent,
                              int *indices,
                              int capacity)
{
    int count = 0;
    for (int i = 0; i < result->certificateNodeCount; i++) {
        if (result->certificate[i].parent != parent) continue;
        if (count < capacity) indices[count] = i;
        count++;
    }
    return count < capacity ? count : capacity;
}

static bool fc_verify_attack_node(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                  int nodeIndex,
                                  int attacker,
                                  bool forbiddenBlack,
                                  const FCProofResult *result)
{
    const FCProofNode *node = &result->certificate[nodeIndex];
    if (node->side != attacker ||
        node->boardHash != fc_board_key(
            (const int (*)[FC_BOARD_SIZE])board, attacker, forbiddenBlack,
            result->searchClass, (uint64_t)result->completedDepth) ||
        !fc_make_move(board, node->x, node->y, attacker, forbiddenBlack)) {
        return false;
    }
    bool wins = fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                            node->x, node->y, attacker);
    if (node->terminalWin) {
        fc_unmake_move(board, node->x, node->y);
        return wins;
    }
    if (wins) {
        fc_unmake_move(board, node->x, node->y);
        return false;
    }
    FCPoint expected[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int expectedCount = fc_generate_refutations(
        board, attacker, forbiddenBlack, result->searchClass,
        expected, FC_BOARD_SIZE * FC_BOARD_SIZE);
    int children[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int childCount = fc_direct_children(
        result, nodeIndex, children, FC_BOARD_SIZE * FC_BOARD_SIZE);
    if (childCount != expectedCount) {
        fc_unmake_move(board, node->x, node->y);
        return false;
    }
    for (int e = 0; e < expectedCount; e++) {
        int found = -1;
        for (int c = 0; c < childCount; c++) {
            const FCProofNode *child = &result->certificate[children[c]];
            if (child->x == expected[e].x && child->y == expected[e].y) {
                found = children[c];
                break;
            }
        }
        if (found < 0) {
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
        const FCProofNode *defense = &result->certificate[found];
        if (defense->side != -attacker ||
            defense->boardHash != fc_board_key(
                (const int (*)[FC_BOARD_SIZE])board, -attacker,
                forbiddenBlack, result->searchClass,
                (uint64_t)result->completedDepth)) {
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
        if (!fc_make_move(board, defense->x, defense->y, -attacker,
                          forbiddenBlack)) {
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                        defense->x, defense->y, -attacker)) {
            fc_unmake_move(board, defense->x, defense->y);
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
        int continuations[2];
        int continuationCount = fc_direct_children(
            result, found, continuations, 2);
        bool valid = continuationCount == 1 &&
            fc_verify_attack_node(board, continuations[0], attacker,
                                  forbiddenBlack, result);
        fc_unmake_move(board, defense->x, defense->y);
        if (!valid) {
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
    }
    fc_unmake_move(board, node->x, node->y);
    return true;
}

bool fc_verify_proof(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                     int attacker,
                     bool forbiddenBlack,
                     const FCProofResult *result)
{
    if (board == NULL || result == NULL ||
        result->status != FC_PROOF_PROVEN_WIN ||
        result->certificateNodeCount <= 0 ||
        result->certificateNodeCount > FC_MAX_PROOF_NODES ||
        result->certificateId != fc_certificate_id(
            result->certificate, result->certificateNodeCount)) return false;
    int roots[2];
    int rootCount = fc_direct_children(result, -1, roots, 2);
    if (rootCount != 1) return false;
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    bool valid = fc_verify_attack_node(mutableBoard, roots[0], attacker,
                                       forbiddenBlack, result);
    return valid && memcmp(mutableBoard, board, sizeof(mutableBoard)) == 0;
}

bool fc_prove_forced_win(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int attacker,
                         bool forbiddenBlack,
                         int searchClass,
                         int maxDepth,
                         uint64_t nodeBudget,
                         uint32_t timeBudgetMs,
                         size_t transpositionCapacity,
                         FCProofResult *result)
{
    if (board == NULL || result == NULL ||
        (attacker != 1 && attacker != -1) || maxDepth <= 0 ||
        nodeBudget == 0) return false;
    memset(result, 0, sizeof(*result));
    result->x = -1;
    result->y = -1;
    result->searchClass = searchClass;
    result->completedDepth = maxDepth;
    FCProofContext context;
    memset(&context, 0, sizeof(context));
    context.attacker = attacker;
    context.forbiddenBlack = forbiddenBlack;
    context.searchClass = FC_PROOF_SEARCH_VCF;
    context.maxDepth = maxDepth;
    context.nodeBudget = nodeBudget;
    context.timeBudgetMs = timeBudgetMs;
    context.startedMilliseconds = fc_now_milliseconds();
    context.tableCapacity = transpositionCapacity;
    if (context.tableCapacity > 0) {
        context.table = calloc(context.tableCapacity,
                               sizeof(FCProofTTEntry));
        if (context.table == NULL) context.tableCapacity = 0;
    }
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    int distance = 0;
    uint64_t proofNumber = 1;
    uint64_t disproofNumber = 1;
    int status = fc_proof_search_attacker(
        mutableBoard, maxDepth, -1, &context, &distance,
        &proofNumber, &disproofNumber);
    if (status != FC_PROOF_PROVEN_WIN &&
        status != FC_PROOF_UNKNOWN &&
        searchClass >= FC_PROOF_SEARCH_VCT) {
        context.searchClass = FC_PROOF_SEARCH_VCT;
        context.certificateCount = 0;
        if (fc_has_vct_root_threat(mutableBoard, attacker,
                                   forbiddenBlack)) {
            status = fc_proof_search_attacker(
                mutableBoard, maxDepth, -1, &context, &distance,
                &proofNumber, &disproofNumber);
        } else {
            status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
            proofNumber = FC_PROOF_INFINITY;
            disproofNumber = 0;
        }
    }
    result->status = status;
    result->searchClass = context.searchClass;
    result->distance = distance;
    result->nodes = context.nodes;
    result->transpositionHits = context.hits;
    result->budgetExhausted = context.aborted;
    result->certificateOverflow = context.certificateOverflow;
    result->elapsedMilliseconds = fc_now_milliseconds()
                                - context.startedMilliseconds;
    result->proofNumber = proofNumber;
    result->disproofNumber = disproofNumber;
    if (status == FC_PROOF_PROVEN_WIN &&
        !context.certificateOverflow && context.certificateCount > 0) {
        result->certificateNodeCount = context.certificateCount;
        memcpy(result->certificate, context.certificate,
               (size_t)context.certificateCount * sizeof(FCProofNode));
        result->x = result->certificate[0].x;
        result->y = result->certificate[0].y;
        result->certificateId = fc_certificate_id(
            result->certificate, result->certificateNodeCount);
        result->certificateVerified = fc_verify_proof(
            board, attacker, forbiddenBlack, result);
        if (!result->certificateVerified) {
            result->status = FC_PROOF_UNKNOWN;
            result->x = -1;
            result->y = -1;
        }
    }
    free(context.table);
    return result->status == FC_PROOF_PROVEN_WIN;
}

static bool fc_move_is_safe(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int x,
                            int y,
                            int side,
                            bool forbiddenBlack)
{
    board[x][y] = side;
    bool ownWin = fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side);
    int rivalWins = ownWin ? 0 : fc_count_immediate_wins(board, -side,
                                                         forbiddenBlack, NULL, 0);
    board[x][y] = 0;
    return ownWin || rivalWins == 0;
}

static int fc_local_winning_replies(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                    int originX,
                                    int originY,
                                    int side,
                                    bool forbiddenBlack)
{
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    int replies = 0;
    for (int d = 0; d < 4; d++) {
        int dx = fcDirections[d][0];
        int dy = fcDirections[d][1];
        for (int offset = -4; offset <= 4; offset++) {
            if (offset == 0) continue;
            int x = originX + offset * dx;
            int y = originY + offset * dy;
            if (!fc_inside(x, y) ||
                !fc_is_legal_move((const int (*)[FC_BOARD_SIZE])mutableBoard,
                                  x, y, side, forbiddenBlack)) continue;
            if (fc_wins_if_placed(mutableBoard, x, y, side)) replies++;
        }
    }
    return replies;
}

static int fc_immediate_replies_after_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                           int x,
                                           int y,
                                           int side,
                                           bool forbiddenBlack)
{
    board[x][y] = side;
    int replies = fc_local_winning_replies(
        (const int (*)[FC_BOARD_SIZE])board, x, y, side, forbiddenBlack);
    board[x][y] = 0;
    return replies;
}

static int fc_generate_candidates(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                  int side,
                                  bool forbiddenBlack,
                                  const FCAIProfile *profile,
                                  FCCandidate *out,
                                  int capacity,
                                  bool forcingOnly)
{
    FCCandidate wins[FC_MAX_CANDIDATES];
    int winCount = fc_count_immediate_wins(board, side, forbiddenBlack,
                                           wins, FC_MAX_CANDIDATES);
    if (winCount > 0) {
        int stored = winCount < capacity ? winCount : capacity;
        memcpy(out, wins, (size_t)stored * sizeof(FCCandidate));
        qsort(out, (size_t)stored, sizeof(FCCandidate), fc_compare_candidate);
        return stored;
    }

    FCCandidate rivalWins[FC_MAX_CANDIDATES];
    int rivalWinCount = fc_count_immediate_wins(board, -side, forbiddenBlack,
                                                rivalWins, FC_MAX_CANDIDATES);
    if (rivalWinCount > 0) {
        int count = 0;
        for (int x = 0; x < FC_BOARD_SIZE && count < capacity; x++) {
            for (int y = 0; y < FC_BOARD_SIZE && count < capacity; y++) {
                if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                      x, y, side, forbiddenBlack)) continue;
                board[x][y] = side;
                int remaining = fc_count_immediate_wins(board, -side,
                                                        forbiddenBlack, NULL, 0);
                bool ownWin = fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                                          x, y, side);
                board[x][y] = 0;
                if (remaining == 0 || ownWin) {
                    out[count].x = x;
                    out[count].y = y;
                    out[count].score = FC_WIN_SCORE / 2
                                     + fc_move_heuristic(board, x, y, side, profile);
                    out[count].tacticalClass = FC_TACTICAL_MUST_DEFEND;
                    out[count].safe = true;
                    out[count].probability = 0.0;
                    count++;
                }
            }
        }
        qsort(out, (size_t)count, sizeof(FCCandidate), fc_compare_candidate);
        return count;
    }

    if (forcingOnly) {
        int count = 0;
        for (int x = 0; x < FC_BOARD_SIZE && count < capacity; x++) {
            for (int y = 0; y < FC_BOARD_SIZE && count < capacity; y++) {
                if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                      x, y, side, forbiddenBlack)) continue;
                int score = fc_move_heuristic(board, x, y, side, profile);
                int ownReplies = fc_immediate_replies_after_move(board, x, y,
                                                                  side,
                                                                  forbiddenBlack);
                int rivalReplies = fc_immediate_replies_after_move(board, x, y,
                                                                    -side,
                                                                    forbiddenBlack);
                if (ownReplies == 0 && rivalReplies == 0) continue;
                out[count] = (FCCandidate){x, y, score,
                                           ownReplies > 0
                                               ? FC_TACTICAL_FORCED_ATTACK
                                               : FC_TACTICAL_FORCED_DEFENSE,
                                           true, 0.0};
                count++;
            }
        }
        qsort(out, (size_t)count, sizeof(FCCandidate), fc_compare_candidate);
        return count;
    }

    int stoneCount = fc_board_stone_count((const int (*)[FC_BOARD_SIZE])board);
    if (stoneCount == 0 && capacity > 0) {
        out[0] = (FCCandidate){7, 7, 1000, FC_TACTICAL_NORMAL, true, 0.0};
        return 1;
    }

    FCCandidate all[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int allCount = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_has_neighbor((const int (*)[FC_BOARD_SIZE])board, x, y, 2)) continue;
            if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                  x, y, side, forbiddenBlack)) continue;
            int score = fc_move_heuristic(board, x, y, side, profile);
            bool safe = fc_move_is_safe(board, x, y, side, forbiddenBlack);
            int ownReplies = fc_immediate_replies_after_move(board, x, y, side,
                                                              forbiddenBlack);
            int rivalReplies = fc_immediate_replies_after_move(board, x, y, -side,
                                                                forbiddenBlack);
            int tacticalClass = ownReplies > 0 ? FC_TACTICAL_FORCED_ATTACK
                              : rivalReplies > 0 ? FC_TACTICAL_FORCED_DEFENSE
                              : FC_TACTICAL_NORMAL;
            all[allCount++] = (FCCandidate){x, y, score,
                                            tacticalClass,
                                            safe, 0.0};
        }
    }
    qsort(all, (size_t)allCount, sizeof(FCCandidate), fc_compare_candidate);
    int normalLimit = profile->candidateLimit;
    if (normalLimit <= 0 || normalLimit > capacity) normalLimit = capacity;
    int count = 0;
    int normalCount = 0;
    for (int i = 0; i < allCount && count < capacity; i++) {
        bool tactical = all[i].tacticalClass != FC_TACTICAL_NORMAL;
        if (!tactical && normalCount >= normalLimit) continue;
        out[count++] = all[i];
        if (!tactical) normalCount++;
    }
    return count;
}

static uint64_t fc_board_hash(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                              int side,
                              bool forbiddenBlack,
                              int depth,
                              int qDepth,
                              bool forcingOnly,
                              const FCAIProfile *profile)
{
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            uint64_t value = (uint64_t)(board[x][y] + 1)
                           | ((uint64_t)(x * FC_BOARD_SIZE + y) << 2);
            hash ^= fc_mix64(value);
            hash *= 0x100000001b3ULL;
        }
    }
    hash ^= fc_mix64((uint64_t)(side + 2)
                     | ((uint64_t)(depth & 0xff) << 8)
                     | ((uint64_t)(qDepth & 0xff) << 16)
                     | ((uint64_t)forcingOnly << 24));
    hash ^= forbiddenBlack ? 0xa5a5a5a5a5a5a5a5ULL : 0ULL;
    if (profile->version != NULL) {
        for (const char *p = profile->version; *p != '\0'; p++) {
            hash ^= (unsigned char)*p;
            hash *= 0x100000001b3ULL;
        }
    }
    return hash;
}

static bool fc_budget_available(FCSearchContext *context)
{
    if (context->nodes >= context->profile.nodeBudget) {
        context->aborted = true;
        return false;
    }
    if (context->profile.timeBudgetMs > 0 &&
        fc_now_milliseconds() - context->startedMilliseconds
            >= context->profile.timeBudgetMs) {
        context->aborted = true;
        return false;
    }
    return true;
}

static int fc_negamax(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                      int side,
                      int depth,
                      int qDepth,
                      int alpha,
                      int beta,
                      int lastX,
                      int lastY,
                      FCSearchContext *context)
{
    if (!fc_budget_available(context)) {
        return fc_evaluate_board((const int (*)[FC_BOARD_SIZE])board,
                                 side, &context->profile);
    }
    context->nodes++;

    if (lastX >= 0 && fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                                  lastX, lastY, -side)) {
        return -FC_WIN_SCORE + (context->profile.maxDepth - depth);
    }

    if (depth <= 0 && qDepth <= 0) {
        return fc_evaluate_board((const int (*)[FC_BOARD_SIZE])board,
                                 side, &context->profile);
    }

    int effectiveDepth = depth > 0 ? depth : qDepth;
    bool forcingOnly = depth <= 0;
    uint64_t key = fc_board_hash((const int (*)[FC_BOARD_SIZE])board,
                                 side, context->forbiddenBlack,
                                 depth, qDepth, forcingOnly,
                                 &context->profile);
    FCTTEntry *entry = NULL;
    if (context->tableCapacity > 0) {
        entry = &context->table[key % context->tableCapacity];
        if (entry->key == key && entry->depth >= effectiveDepth) {
            context->hits++;
            if (entry->flag == FC_TT_EXACT) return entry->score;
            if (entry->flag == FC_TT_LOWER && entry->score > alpha) alpha = entry->score;
            if (entry->flag == FC_TT_UPPER && entry->score < beta) beta = entry->score;
            if (alpha >= beta) return entry->score;
        }
    }

    FCCandidate moves[FC_MAX_CANDIDATES];
    int count = fc_generate_candidates(board, side, context->forbiddenBlack,
                                       &context->profile, moves,
                                       FC_MAX_CANDIDATES, forcingOnly);
    if (count == 0) {
        return fc_evaluate_board((const int (*)[FC_BOARD_SIZE])board,
                                 side, &context->profile);
    }

    int originalAlpha = alpha;
    int best = -FC_WIN_SCORE;
    int nextDepth = depth > 0 ? depth - 1 : 0;
    int nextQDepth = depth > 0 ? qDepth : qDepth - 1;
    for (int i = 0; i < count; i++) {
        int x = moves[i].x;
        int y = moves[i].y;
        board[x][y] = side;
        int score = -fc_negamax(board, -side, nextDepth, nextQDepth,
                                -beta, -alpha, x, y, context);
        board[x][y] = 0;
        if (context->aborted) return best == -FC_WIN_SCORE ? score : best;
        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            context->cutoffs++;
            break;
        }
    }

    if (entry != NULL && !context->aborted) {
        entry->key = key;
        entry->depth = effectiveDepth;
        entry->score = best;
        entry->flag = best <= originalAlpha ? FC_TT_UPPER
                    : best >= beta ? FC_TT_LOWER : FC_TT_EXACT;
    }
    return best;
}

static int fc_search_root(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                          int side,
                          int depth,
                          FCSearchContext *context,
                          FCCandidate *moves,
                          int count)
{
    int best = -FC_WIN_SCORE;
    int alpha = -FC_WIN_SCORE;
    int beta = FC_WIN_SCORE;
    for (int i = 0; i < count; i++) {
        if (!fc_budget_available(context)) return best;
        int x = moves[i].x;
        int y = moves[i].y;
        board[x][y] = side;
        int score;
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side)) {
            score = FC_WIN_SCORE - i;
        } else {
            int replyCount = fc_local_winning_replies(
                (const int (*)[FC_BOARD_SIZE])board, x, y, side,
                context->forbiddenBlack);
            int pathDepth = replyCount >= 2
                          ? context->profile.doubleThreeDepth
                          : replyCount == 1
                          ? context->profile.fourDepth
                          : context->profile.quiescenceDepth;
            if (moves[i].tacticalClass == FC_TACTICAL_FORCED_DEFENSE &&
                context->profile.forcingDepth > pathDepth) {
                pathDepth = context->profile.forcingDepth;
            }
            /* A path stage is a threat, forced reply, and continuation. */
            int targetedQDepth = (pathDepth + 2) / 3;
            if (targetedQDepth < context->profile.quiescenceDepth)
                targetedQDepth = context->profile.quiescenceDepth;
            if (targetedQDepth > 5) targetedQDepth = 5;
            score = -fc_negamax(board, -side, depth - 1,
                                targetedQDepth,
                                -beta, -alpha, x, y, context);
        }
        board[x][y] = 0;
        if (context->aborted) return best;
        moves[i].score = score;
        if (score > best) best = score;
        if (score > alpha) alpha = score;
    }
    qsort(moves, (size_t)count, sizeof(FCCandidate),
          fc_compare_searched_candidate);
    return best;
}

static int fc_choose_candidate(FCCandidate *moves,
                               int count,
                               int tacticalClass,
                               int stoneCount,
                               const FCAIProfile *profile,
                               uint64_t seed,
                               FCRandomMode randomMode)
{
    int firstSafe = -1;
    for (int i = 0; i < count; i++) {
        if (moves[i].safe) {
            firstSafe = i;
            break;
        }
    }
    if (firstSafe < 0) return 0;
    if (count <= 1 || tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
        tacticalClass == FC_TACTICAL_MUST_DEFEND) {
        moves[firstSafe].probability = 1.0;
        return firstSafe;
    }
    if (randomMode == FC_RANDOM_EVALUATION) {
        moves[firstSafe].probability = 1.0;
        return firstSafe;
    }

    int eligible[FC_MAX_CANDIDATES];
    int eligibleCount = 0;
    int best = moves[firstSafe].score;
    int maxEligible = profile->maxRandomCandidates;
    if (maxEligible < 1) maxEligible = 1;
    for (int i = 0; i < count && eligibleCount < maxEligible; i++) {
        if (!moves[i].safe) continue;
        if (best - moves[i].score > profile->nearBestWindow) continue;
        eligible[eligibleCount++] = i;
    }
    if (eligibleCount <= 1) {
        moves[firstSafe].probability = 1.0;
        return firstSafe;
    }

    double sharpness = tacticalClass >= FC_TACTICAL_FORCED_ATTACK ? 0.55 : 1.0;
    double openingFactor = stoneCount < 12 ? 1.2 : (stoneCount > 45 ? 0.75 : 1.0);
    double temperature = profile->randomTemperature * sharpness * openingFactor;
    if (temperature < 1.0) temperature = 1.0;
    double weights[FC_MAX_CANDIDATES];
    double total = 0.0;
    for (int i = 0; i < eligibleCount; i++) {
        int index = eligible[i];
        double exponent = (double)(moves[index].score - best) / temperature;
        if (exponent < -40.0) exponent = -40.0;
        weights[i] = exp(exponent);
        total += weights[i];
    }
    for (int i = 0; i < eligibleCount; i++) {
        moves[eligible[i]].probability = weights[i] / total;
    }

    FCRandom random;
    fc_random_seed(&random, seed);
    double target = fc_random_unit(&random) * total;
    double running = 0.0;
    for (int i = 0; i < eligibleCount; i++) {
        running += weights[i];
        if (target <= running) return eligible[i];
    }
    return eligible[eligibleCount - 1];
}

static bool fc_analyze_internal(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                int side,
                                bool forbiddenBlack,
                                const FCAIProfile *profile,
                                uint64_t seed,
                                FCRandomMode randomMode,
                                int hintX,
                                int hintY,
                                bool useHint,
                                FCAnalysisResult *result)
{
    if (board == NULL || profile == NULL || result == NULL ||
        (side != 1 && side != -1)) {
        return false;
    }
    memset(result, 0, sizeof(*result));
    result->x = -1;
    result->y = -1;
    result->seed = seed;
    result->randomMode = randomMode;

    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    FCSearchContext context;
    memset(&context, 0, sizeof(context));
    context.profile = *profile;
    context.forbiddenBlack = forbiddenBlack;
    context.startedMilliseconds = fc_now_milliseconds();
    context.tableCapacity = profile->transpositionCapacity;
    if (context.tableCapacity > 0) {
        context.table = calloc(context.tableCapacity, sizeof(FCTTEntry));
        if (context.table == NULL) context.tableCapacity = 0;
    }

    FCCandidate baseMoves[FC_MAX_CANDIDATES];
    int count = fc_generate_candidates(mutableBoard, side, forbiddenBlack,
                                       profile, baseMoves,
                                       FC_MAX_CANDIDATES, false);
    if (count == 0) {
        free(context.table);
        result->provenLoss = true;
        result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
        return false;
    }

    int bookX = -1;
    int bookY = -1;
    int bookId = -1;
    int bookPly = -1;
    bool hasBook = profile->openingBookEnabled && fc_opening_book_lookup(
        (const int (*)[FC_BOARD_SIZE])mutableBoard, side, forbiddenBlack,
        seed, &bookX, &bookY, &bookId, &bookPly);
    if (hasBook &&
        baseMoves[0].tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
        baseMoves[0].tacticalClass != FC_TACTICAL_MUST_DEFEND) {
        bool present = false;
        for (int i = 0; i < count; i++)
            present = present || (baseMoves[i].x == bookX && baseMoves[i].y == bookY);
        if (!present) {
            int index = count < FC_MAX_CANDIDATES ? count++ : count - 1;
            baseMoves[index] = (FCCandidate){
                bookX, bookY,
                fc_move_heuristic(mutableBoard, bookX, bookY, side, profile),
                FC_TACTICAL_NORMAL,
                fc_move_is_safe(mutableBoard, bookX, bookY, side, forbiddenBlack),
                0.0
            };
            qsort(baseMoves, (size_t)count, sizeof(FCCandidate),
                  fc_compare_candidate);
        }
    }

    if (useHint &&
        baseMoves[0].tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
        baseMoves[0].tacticalClass != FC_TACTICAL_MUST_DEFEND &&
        fc_is_legal_move((const int (*)[FC_BOARD_SIZE])mutableBoard,
                         hintX, hintY, side, forbiddenBlack)) {
        bool present = false;
        for (int i = 0; i < count; i++) {
            present = present || (baseMoves[i].x == hintX && baseMoves[i].y == hintY);
        }
        if (!present) {
            int index = count < FC_MAX_CANDIDATES ? count++ : count - 1;
            int ownReplies = fc_immediate_replies_after_move(
                mutableBoard, hintX, hintY, side, forbiddenBlack);
            int rivalReplies = fc_immediate_replies_after_move(
                mutableBoard, hintX, hintY, -side, forbiddenBlack);
            baseMoves[index] = (FCCandidate){
                hintX, hintY,
                fc_move_heuristic(mutableBoard, hintX, hintY, side, profile),
                ownReplies > 0 ? FC_TACTICAL_FORCED_ATTACK
                : rivalReplies > 0 ? FC_TACTICAL_FORCED_DEFENSE
                : FC_TACTICAL_NORMAL,
                fc_move_is_safe(mutableBoard, hintX, hintY, side, forbiddenBlack),
                0.0
            };
            qsort(baseMoves, (size_t)count, sizeof(FCCandidate),
                  fc_compare_candidate);
        }
    }

    int tacticalClass = baseMoves[0].tacticalClass;
    int defaultX = hasBook ? bookX : hintX;
    int defaultY = hasBook ? bookY : hintY;
    int defaultSource = hasBook ? FC_DEFAULT_OPENING_BOOK
                      : useHint ? FC_DEFAULT_LEGACY : FC_DEFAULT_NONE;
    if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])mutableBoard,
                          defaultX, defaultY, side, forbiddenBlack)) {
        defaultX = baseMoves[0].x;
        defaultY = baseMoves[0].y;
        defaultSource = FC_DEFAULT_NONE;
    }
    result->defaultSource = defaultSource;
    result->defaultX = defaultX;
    result->defaultY = defaultY;
    result->bookId = hasBook ? bookId : -1;
    result->bookPly = hasBook ? bookPly : -1;

    if (profile->proofEnabled &&
        tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
        tacticalClass != FC_TACTICAL_MUST_DEFEND) {
        FCProofResult ownProof;
        bool ownWin = fc_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side, forbiddenBlack,
            profile->proofSearchClass, profile->proofMaxDepth,
            profile->proofNodeBudget, profile->proofTimeBudgetMs,
            profile->proofTranspositionCapacity, &ownProof);
        int selectedX = defaultX;
        int selectedY = defaultY;
        int overrideReason = FC_OVERRIDE_NONE;
        FCProofResult selectedProof = ownProof;
        if (ownWin && ownProof.certificateVerified) {
            selectedX = ownProof.x;
            selectedY = ownProof.y;
            overrideReason = (selectedX == defaultX && selectedY == defaultY)
                ? FC_OVERRIDE_NONE : FC_OVERRIDE_PROVEN_ATTACK;
            tacticalClass = FC_TACTICAL_FORCED_ATTACK;
        } else {
            int afterDefault[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(afterDefault, mutableBoard, sizeof(afterDefault));
            FCProofResult opponentProof;
            memset(&opponentProof, 0, sizeof(opponentProof));
            bool defaultLegal = fc_make_move(
                afterDefault, defaultX, defaultY, side, forbiddenBlack);
            bool opponentWins = defaultLegal && fc_prove_forced_win(
                (const int (*)[FC_BOARD_SIZE])afterDefault, -side,
                forbiddenBlack, profile->proofSearchClass,
                profile->proofMaxDepth, profile->proofNodeBudget,
                profile->proofTimeBudgetMs,
                profile->proofTranspositionCapacity, &opponentProof);
            if (opponentWins && opponentProof.certificateVerified) {
                bool escaped = false;
                int examined = 0;
                for (int i = 0; i < count && examined < 6; i++) {
                    int x = baseMoves[i].x;
                    int y = baseMoves[i].y;
                    if (x == defaultX && y == defaultY) continue;
                    if (!baseMoves[i].safe) continue;
                    int alternative[FC_BOARD_SIZE][FC_BOARD_SIZE];
                    memcpy(alternative, mutableBoard, sizeof(alternative));
                    if (!fc_make_move(alternative, x, y, side,
                                      forbiddenBlack)) continue;
                    examined++;
                    FCProofResult replyProof;
                    (void)fc_prove_forced_win(
                        (const int (*)[FC_BOARD_SIZE])alternative, -side,
                        forbiddenBlack, profile->proofSearchClass,
                        profile->proofMaxDepth, profile->proofNodeBudget,
                        profile->proofTimeBudgetMs,
                        profile->proofTranspositionCapacity, &replyProof);
                    if (replyProof.status != FC_PROOF_NO_FORCED_WIN_IN_SCOPE)
                        continue;
                    selectedX = x;
                    selectedY = y;
                    selectedProof = opponentProof;
                    overrideReason = FC_OVERRIDE_PROVEN_DEFENSE;
                    tacticalClass = FC_TACTICAL_FORCED_DEFENSE;
                    escaped = true;
                    break;
                }
                if (!escaped) selectedProof = opponentProof;
            }
        }
        result->x = selectedX;
        result->y = selectedY;
        result->tacticalClass = tacticalClass;
        result->overrideReason = overrideReason;
        result->candidateCount = count;
        memcpy(result->candidates, baseMoves,
               (size_t)count * sizeof(FCCandidate));
        result->proofStatus = selectedProof.status;
        result->proofSearchClass = selectedProof.searchClass;
        result->proofDistance = selectedProof.distance;
        result->proofCertificateId = selectedProof.certificateId;
        result->proofNodes = selectedProof.nodes;
        result->proofNumber = selectedProof.proofNumber;
        result->disproofNumber = selectedProof.disproofNumber;
        result->proofCertificateVerified = selectedProof.certificateVerified;
        result->randomCandidateCount = hasBook ? 2 : 1;
        result->randomSelectionUsed = hasBook &&
            randomMode == FC_RANDOM_USER_GAME;
        result->stats.nodes = selectedProof.nodes;
        result->stats.transpositionHits = selectedProof.transpositionHits;
        result->stats.completedDepth = selectedProof.completedDepth;
        result->stats.budgetExhausted = selectedProof.budgetExhausted;
        result->stats.elapsedMilliseconds = fc_now_milliseconds()
                                            - context.startedMilliseconds;
        free(context.table);
        return fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                result->x, result->y, side,
                                forbiddenBlack);
    }
    FCCandidate completed[FC_MAX_CANDIDATES];
    memcpy(completed, baseMoves, (size_t)count * sizeof(FCCandidate));
    int completedDepth = 0;

    if (tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
        tacticalClass == FC_TACTICAL_MUST_DEFEND) {
        completedDepth = 1;
    } else {
        for (int depth = 1; depth <= profile->maxDepth; depth++) {
            FCCandidate iteration[FC_MAX_CANDIDATES];
            memcpy(iteration, baseMoves, (size_t)count * sizeof(FCCandidate));
            context.aborted = false;
            (void)fc_search_root(mutableBoard, side, depth,
                                 &context, iteration, count);
            if (context.aborted) break;
            memcpy(completed, iteration, (size_t)count * sizeof(FCCandidate));
            completedDepth = depth;
        }
    }

    qsort(completed, (size_t)count, sizeof(FCCandidate),
          fc_compare_searched_candidate);
    for (int i = 0; i < count; i++) {
        completed[i].safe = fc_move_is_safe(mutableBoard,
                                            completed[i].x,
                                            completed[i].y,
                                            side, forbiddenBlack);
        if (completed[i].score <= -FC_WIN_SCORE + 1000) {
            completed[i].safe = false;
        }
    }
    bool anySafe = false;
    for (int i = 0; i < count; i++) anySafe = anySafe || completed[i].safe;
    if (!anySafe) {
        free(context.table);
        result->provenLoss = true;
        result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
        return false;
    }
    int selected = fc_choose_candidate(completed, count, tacticalClass,
                                       fc_board_stone_count(board), profile,
                                       seed, randomMode);
    result->x = completed[selected].x;
    result->y = completed[selected].y;
    result->score = completed[selected].score;
    result->tacticalClass = tacticalClass;
    result->overrideReason = tacticalClass == FC_TACTICAL_IMMEDIATE_WIN
        ? FC_OVERRIDE_IMMEDIATE_WIN
        : tacticalClass == FC_TACTICAL_MUST_DEFEND
        ? FC_OVERRIDE_MUST_DEFEND : FC_OVERRIDE_NONE;
    result->candidateCount = count;
    result->randomCandidateCount = count;
    result->randomSelectionUsed = randomMode == FC_RANDOM_USER_GAME &&
                                  selected != 0;
    memcpy(result->candidates, completed, (size_t)count * sizeof(FCCandidate));
    result->stats.nodes = context.nodes;
    result->stats.transpositionHits = context.hits;
    result->stats.cutoffs = context.cutoffs;
    result->stats.completedDepth = completedDepth;
    result->stats.budgetExhausted = context.aborted;
    result->stats.elapsedMilliseconds = fc_now_milliseconds()
                                            - context.startedMilliseconds;
    free(context.table);
    if ((hasBook || useHint) && tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
        tacticalClass != FC_TACTICAL_MUST_DEFEND) {
        for (int i = 0; i < result->candidateCount; i++) {
            FCCandidate *candidate = &result->candidates[i];
            if (candidate->x != defaultX || candidate->y != defaultY)
                continue;
            /* Preserve the frozen legacy move unless it fails the tactical
             * safety gate.  Heuristic score gaps alone are not proof. */
            if (candidate->safe) {
                result->x = candidate->x;
                result->y = candidate->y;
                result->score = candidate->score;
            }
            break;
        }
    }
    return fc_inside(result->x, result->y);
}

bool fc_analyze(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                int side,
                bool forbiddenBlack,
                const FCAIProfile *profile,
                uint64_t seed,
                FCRandomMode randomMode,
                FCAnalysisResult *result)
{
    return fc_analyze_internal(board, side, forbiddenBlack, profile, seed,
                               randomMode, -1, -1, false, result);
}

bool fc_analyze_with_hint(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                          int side,
                          bool forbiddenBlack,
                          const FCAIProfile *profile,
                          uint64_t seed,
                          FCRandomMode randomMode,
                          int hintX,
                          int hintY,
                          FCAnalysisResult *result)
{
    return fc_analyze_internal(board, side, forbiddenBlack, profile, seed,
                               randomMode, hintX, hintY, true, result);
}

size_t fc_profile_snapshot(const FCAIProfile *profile, char *buffer, size_t capacity)
{
    if (profile == NULL || buffer == NULL || capacity == 0) return 0;
    int length = snprintf(buffer, capacity,
        "{\"name\":\"%s\",\"version\":\"%s\",\"maxDepth\":%d,"
        "\"quiescenceDepth\":%d,\"fourDepth\":%d,"
        "\"doubleThreeDepth\":%d,\"forcingDepth\":%d,"
        "\"candidateLimit\":%d,\"nodeBudget\":%llu,"
        "\"timeBudgetMs\":%u,\"transpositionCapacity\":%zu,"
        "\"attackWeight\":%d,\"defenseWeight\":%d,"
        "\"centerWeight\":%d,\"nearBestWindow\":%d,"
        "\"randomTemperature\":%.3f,\"maxRandomCandidates\":%d,"
        "\"proofEnabled\":%s,\"openingBookEnabled\":%s,"
        "\"proofSearchClass\":%d,\"proofMaxDepth\":%d,"
        "\"proofNodeBudget\":%llu,\"proofTimeBudgetMs\":%u,"
        "\"proofTranspositionCapacity\":%zu}",
        profile->name, profile->version, profile->maxDepth,
        profile->quiescenceDepth, profile->fourDepth,
        profile->doubleThreeDepth, profile->forcingDepth,
        profile->candidateLimit, (unsigned long long)profile->nodeBudget,
        profile->timeBudgetMs, profile->transpositionCapacity,
        profile->attackWeight, profile->defenseWeight,
        profile->centerWeight, profile->nearBestWindow,
        profile->randomTemperature, profile->maxRandomCandidates,
        profile->proofEnabled ? "true" : "false",
        profile->openingBookEnabled ? "true" : "false",
        profile->proofSearchClass, profile->proofMaxDepth,
        (unsigned long long)profile->proofNodeBudget,
        profile->proofTimeBudgetMs, profile->proofTranspositionCapacity);
    if (length < 0) return 0;
    return (size_t)length < capacity ? (size_t)length : capacity - 1;
}

const char *fc_tactical_name(int tacticalClass)
{
    switch (tacticalClass) {
        case FC_TACTICAL_IMMEDIATE_WIN: return "immediate-win";
        case FC_TACTICAL_MUST_DEFEND: return "must-defend";
        case FC_TACTICAL_FORCED_ATTACK: return "forced-attack";
        case FC_TACTICAL_FORCED_DEFENSE: return "forced-defense";
        case FC_TACTICAL_PROVEN_LOSS: return "proven-loss";
        default: return "normal";
    }
}

const char *fc_proof_status_name(int status)
{
    switch (status) {
        case FC_PROOF_PROVEN_WIN: return "proven-win";
        case FC_PROOF_NO_FORCED_WIN_IN_SCOPE: return "no-forced-win-in-scope";
        default: return "unknown";
    }
}

const char *fc_override_reason_name(int reason)
{
    switch (reason) {
        case FC_OVERRIDE_IMMEDIATE_WIN: return "immediate-win";
        case FC_OVERRIDE_MUST_DEFEND: return "must-defend";
        case FC_OVERRIDE_PROVEN_ATTACK: return "proven-attack";
        case FC_OVERRIDE_PROVEN_DEFENSE: return "proven-defense";
        case FC_OVERRIDE_REJECTED_CERTIFICATE: return "rejected-certificate";
        default: return "none";
    }
}
