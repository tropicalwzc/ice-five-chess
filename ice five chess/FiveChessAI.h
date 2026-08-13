#ifndef FiveChessAI_h
#define FiveChessAI_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FC_BOARD_SIZE = 15,
    FC_MAX_CANDIDATES = 32,
    FC_MAX_THREATS = 64,
    FC_MAX_THREAT_POINTS = 8,
    FC_MAX_PROOF_NODES = 512,
    FC_WIN_SCORE = 100000000
};

typedef enum {
    FC_RANDOM_EVALUATION = 0,
    FC_RANDOM_USER_GAME = 1
} FCRandomMode;

typedef enum {
    FC_TACTICAL_NORMAL = 0,
    FC_TACTICAL_IMMEDIATE_WIN = 1,
    FC_TACTICAL_MUST_DEFEND = 2,
    FC_TACTICAL_FORCED_ATTACK = 3,
    FC_TACTICAL_FORCED_DEFENSE = 4,
    FC_TACTICAL_PROVEN_LOSS = 5
} FCTacticalClass;

typedef enum {
    FC_PROOF_UNKNOWN = 0,
    FC_PROOF_PROVEN_WIN = 1,
    FC_PROOF_NO_FORCED_WIN_IN_SCOPE = 2
} FCProofStatus;

typedef enum {
    FC_PROOF_SEARCH_NONE = 0,
    FC_PROOF_SEARCH_VCF = 1,
    FC_PROOF_SEARCH_VCT = 2
} FCProofSearchClass;

typedef enum {
    FC_THREAT_NONE = 0,
    FC_THREAT_OPEN_THREE = 1,
    FC_THREAT_FOUR_THREE = 2,
    FC_THREAT_FOUR = 3,
    FC_THREAT_OPEN_FOUR = 4,
    FC_THREAT_FIVE = 5
} FCThreatSeverity;

typedef enum {
    FC_DEFAULT_NONE = 0,
    FC_DEFAULT_LEGACY = 1,
    FC_DEFAULT_OPENING_BOOK = 2
} FCDefaultSource;

typedef enum {
    FC_OVERRIDE_NONE = 0,
    FC_OVERRIDE_IMMEDIATE_WIN = 1,
    FC_OVERRIDE_MUST_DEFEND = 2,
    FC_OVERRIDE_PROVEN_ATTACK = 3,
    FC_OVERRIDE_PROVEN_DEFENSE = 4,
    FC_OVERRIDE_REJECTED_CERTIFICATE = 5
} FCOverrideReason;

typedef struct {
    int x;
    int y;
} FCPoint;

typedef struct {
    FCPoint gain;
    FCPoint costs[FC_MAX_THREAT_POINTS];
    int costCount;
    FCPoint rests[FC_MAX_THREAT_POINTS];
    int restCount;
    int severity;
    int side;
} FCThreat;

typedef struct {
    uint64_t boardHash;
    int parent;
    int x;
    int y;
    int side;
    bool terminalWin;
} FCProofNode;

typedef struct {
    int status;
    int searchClass;
    int x;
    int y;
    int distance;
    uint64_t nodes;
    uint64_t transpositionHits;
    uint64_t proofNumber;
    uint64_t disproofNumber;
    int completedDepth;
    bool budgetExhausted;
    bool certificateVerified;
    bool certificateOverflow;
    uint64_t certificateId;
    int certificateNodeCount;
    FCProofNode certificate[FC_MAX_PROOF_NODES];
    double elapsedMilliseconds;
} FCProofResult;

typedef struct {
    const char *name;
    const char *version;
    int maxDepth;
    int quiescenceDepth;
    int fourDepth;
    int doubleThreeDepth;
    int forcingDepth;
    int candidateLimit;
    uint64_t nodeBudget;
    uint32_t timeBudgetMs;
    size_t transpositionCapacity;
    int attackWeight;
    int defenseWeight;
    int centerWeight;
    int nearBestWindow;
    double randomTemperature;
    int maxRandomCandidates;
    bool proofEnabled;
    bool openingBookEnabled;
    int proofSearchClass;
    int proofMaxDepth;
    uint64_t proofNodeBudget;
    uint32_t proofTimeBudgetMs;
    size_t proofTranspositionCapacity;
} FCAIProfile;

typedef struct {
    int x;
    int y;
    int score;
    int tacticalClass;
    bool safe;
    double probability;
} FCCandidate;

typedef struct {
    uint64_t nodes;
    uint64_t transpositionHits;
    uint64_t cutoffs;
    int completedDepth;
    bool budgetExhausted;
    double elapsedMilliseconds;
} FCSearchStats;

typedef struct {
    int x;
    int y;
    int score;
    int tacticalClass;
    bool provenLoss;
    uint64_t seed;
    FCCandidate candidates[FC_MAX_CANDIDATES];
    int candidateCount;
    FCSearchStats stats;
    int defaultSource;
    int defaultX;
    int defaultY;
    int overrideReason;
    int bookId;
    int bookPly;
    int proofStatus;
    int proofSearchClass;
    int proofDistance;
    uint64_t proofCertificateId;
    uint64_t proofNodes;
    uint64_t proofNumber;
    uint64_t disproofNumber;
    bool proofCertificateVerified;
    int randomMode;
    int randomCandidateCount;
    bool randomSelectionUsed;
} FCAnalysisResult;

typedef struct {
    uint64_t state;
} FCRandom;

FCAIProfile fc_profile_production(void);
FCAIProfile fc_profile_depth_8_8_10(void);
FCAIProfile fc_profile_depth_10_10_12(void);
FCAIProfile fc_profile_depth_12_12_14(void);
FCAIProfile fc_profile_defense_c1(void);
FCAIProfile fc_profile_defense_c2(void);
FCAIProfile fc_profile_narrow_deep_d1(void);
FCAIProfile fc_profile_narrow_deep_d2(void);
FCAIProfile fc_profile_narrow_deep_d3(void);
FCAIProfile fc_profile_proof_guided(bool openingBookEnabled);

void fc_random_seed(FCRandom *random, uint64_t seed);
uint64_t fc_random_next(FCRandom *random);
double fc_random_unit(FCRandom *random);

bool fc_is_legal_move(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                      int x,
                      int y,
                      int side,
                      bool forbiddenBlack);

bool fc_has_five(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                 int x,
                 int y,
                 int side);

int fc_board_winner(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE]);

bool fc_make_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                  int x,
                  int y,
                  int side,
                  bool forbiddenBlack);

void fc_unmake_move(int board[FC_BOARD_SIZE][FC_BOARD_SIZE], int x, int y);

void fc_transform_point(int transform, int x, int y, int *outX, int *outY);

uint64_t fc_board_key(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                      int side,
                      bool forbiddenBlack,
                      int searchClass,
                      uint64_t profileVersion);

int fc_enumerate_threats(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int side,
                         bool forbiddenBlack,
                         int searchClass,
                         FCThreat *out,
                         int capacity,
                         bool *overflow);

bool fc_prove_forced_win(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int attacker,
                         bool forbiddenBlack,
                         int searchClass,
                         int maxDepth,
                         uint64_t nodeBudget,
                         uint32_t timeBudgetMs,
                         size_t transpositionCapacity,
                         FCProofResult *result);

bool fc_verify_proof(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                     int attacker,
                     bool forbiddenBlack,
                     const FCProofResult *result);

bool fc_opening_book_lookup(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int side,
                            bool forbiddenBlack,
                            uint64_t seed,
                            int *outX,
                            int *outY,
                            int *outBookId,
                            int *outBookPly);

const char *fc_opening_book_version(void);
const char *fc_proof_status_name(int status);
const char *fc_override_reason_name(int reason);

bool fc_analyze(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                int side,
                bool forbiddenBlack,
                const FCAIProfile *profile,
                uint64_t seed,
                FCRandomMode randomMode,
                FCAnalysisResult *result);

bool fc_analyze_with_hint(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                          int side,
                          bool forbiddenBlack,
                          const FCAIProfile *profile,
                          uint64_t seed,
                          FCRandomMode randomMode,
                          int hintX,
                          int hintY,
                          FCAnalysisResult *result);

size_t fc_profile_snapshot(const FCAIProfile *profile, char *buffer, size_t capacity);
const char *fc_tactical_name(int tacticalClass);

#ifdef __cplusplus
}
#endif

#endif
