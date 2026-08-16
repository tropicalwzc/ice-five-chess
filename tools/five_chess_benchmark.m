#import <Foundation/Foundation.h>

#import "../ice five chess/doublethree.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

typedef struct {
    int x;
    int y;
    int side;
    const char *engine;
    double milliseconds;
    double cpuMilliseconds;
    uint64_t peakResidentBytes;
    uint64_t nodes;
    uint64_t hits;
    int depth;
    int tacticalClass;
    int candidateCount;
    int hintX;
    int hintY;
    bool choseHint;
    bool budgetExhausted;
    int decisionStatus;
    bool fallbackUsed;
    bool candidateCoverageComplete;
    uint64_t decisionLedgerVersion;
    uint64_t decisionNodesReserved;
    uint64_t decisionNodesConsumed;
    uint64_t decisionQueriesReserved;
    uint64_t decisionQueriesConsumed;
    uint64_t decisionMemoryReserved;
    uint64_t decisionMemoryConsumed;
    uint64_t decisionMemoryPeakReserved;
    uint64_t decisionMemoryReleased;
    uint64_t decisionStageExhaustions;
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
    uint64_t proofParallelNodes;
    int proofWorkerCap;
    int proofWorkersLaunched;
    int proofParallelJobs;
    int proofParallelJobsCompleted;
    bool proofCertificateVerified;
    int opponentAfterSelectedStatus;
    int opponentAfterSelectedDistance;
    int escapeStage;
    int escapeAlternativesExamined;
    int escapeScopedDisproofCount;
    int escapeUnknownCount;
    int escapeVerifiedLossCount;
    int lossReason;
    bool corpusProtectionApplied;
    bool quietThreatSelected;
    int quietRootsExamined;
    int randomMode;
    int randomCandidateCount;
    bool randomSelectionUsed;
    uint64_t decisionSeed;
    int randomSelectedRank;
    uint64_t randomEquivalenceSignature;
    bool randomEligibilityVerified;
    int hybridComponent;
    const char *hybridComponentName;
    const char *hybridComponentVersion;
    int fourStarX;
    int fourStarY;
    int forkRiskStatus;
    int selectedOpponentImmediateWinCount;
    int forkCandidatesExamined;
    int forkSafeCandidates;
    int forkRiskyCandidates;
    int forkUnknownCandidates;
    int forkAvoidedCount;
    bool forkProbeComplete;
    int handoffReason;
    bool corpusLookup;
    int corpusPositionIndex;
    int corpusCandidateCount;
    int corpusMatchType;
    int corpusTrustTier;
    int corpusRequiredStones;
    int corpusSourceBoardMask;
    bool corpusAccepted;
    int corpusReason;
    int corpusSupportGames;
    int corpusSupportEvents;
    int corpusSupportSources;
    int corpusAcceptedCandidateCount;
    FCProofDiagnostics diagnostics;
} BenchmarkStep;

typedef struct {
    int x;
    int y;
    int side;
} BenchmarkMove;

typedef struct {
    const char *profileName;
    const char *suiteName;
    const char *outputPath;
    uint64_t masterSeed;
    bool seedWasProvided;
    bool randomUserMode;
    bool hintOnlyStrategy;
    int openingStart;
    int openingCount;
    int maxMoves;
    bool forbiddenBlack;
    bool opponentFourStar;
    int pairedPhase;
    int proofWorkerCountOverride;
} BenchmarkOptions;

static const uint64_t FC_TRAINING_MASTER_SEED = 0xA8B6C4D220260813ULL;
static const uint64_t FC_FINAL_MASTER_SEED = 0xF1CE5EED20260814ULL;
static const uint64_t FC_PROOF_FINAL_MASTER_SEED = 0xC0DEC0DE20260813ULL;

#include "../ice five chess/FiveChessOpeningBook.inc"
#include "FiveChessFormalOpenings.inc"
#include "FiveChessEliteDiagnosticPositions.inc"

static double now_milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static double process_cpu_milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static uint64_t peak_resident_bytes(void)
{
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0;
#if defined(__APPLE__)
    return (uint64_t)usage.ru_maxrss;
#else
    return (uint64_t)usage.ru_maxrss * UINT64_C(1024);
#endif
}

static uint64_t mix64(uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static uint64_t decision_seed(uint64_t master,
                              int openingId,
                              int ply,
                              int side,
                              bool newEngine)
{
    uint64_t value = master ^ ((uint64_t)(openingId + 1) << 32)
                   ^ ((uint64_t)(ply + 1) * 0x9e3779b97f4a7c15ULL)
                   ^ (side == 1 ? 0x424c41434bULL : 0x5748495445ULL)
                   ^ (newEngine ? 0x4e4557ULL : 0x4c4547414359ULL);
    return mix64(value);
}

static FCAIProfile profile_named(const char *name)
{
    if (strcmp(name, "legacy-control") == 0) {
        FCAIProfile profile = fc_profile_production();
        profile.name = "legacy-three-star-control";
        profile.version = "5224020-frozen";
        return profile;
    }
    if (strcmp(name, "a0") == 0) return fc_profile_depth_8_8_10();
    if (strcmp(name, "b1") == 0) return fc_profile_depth_10_10_12();
    if (strcmp(name, "b2") == 0) return fc_profile_depth_12_12_14();
    if (strcmp(name, "c1") == 0) return fc_profile_defense_c1();
    if (strcmp(name, "c2") == 0) return fc_profile_defense_c2();
    if (strcmp(name, "d1") == 0) return fc_profile_narrow_deep_d1();
    if (strcmp(name, "d2") == 0) return fc_profile_narrow_deep_d2();
    if (strcmp(name, "d3") == 0) return fc_profile_narrow_deep_d3();
    if (strcmp(name, "proof-book") == 0)
        return fc_profile_proof_guided(true);
    if (strcmp(name, "proof-no-book") == 0)
        return fc_profile_proof_guided(false);
    if (strcmp(name, "five-star") == 0)
        return fc_profile_five_star_proof_engine_candidate();
    if (strcmp(name, "five-star-color-hybrid") == 0)
        return fc_profile_five_star_color_hybrid_candidate();
    if (strcmp(name, "five-star-v521-hybrid-serial") == 0)
        return fc_profile_five_star_v521_serial_hybrid_control();
    if (strcmp(name, "five-star-v521-hybrid-parallel") == 0)
        return fc_profile_five_star_v521_parallel_hybrid_candidate();
    if (strcmp(name, "five-star-v57") == 0)
        return fc_profile_five_star_v57_hybrid_candidate();
    if (strcmp(name, "five-star-v57-thread-scheduler") == 0)
        return fc_profile_five_star_v57_thread_scheduler_candidate();
    if (strcmp(name, "five-star-v541-thread-scheduler") == 0)
        return fc_profile_five_star_v541_thread_scheduler_candidate();
    if (strcmp(name, "five-star-v57-branch-first") == 0)
        return fc_profile_five_star_v57_branch_first_candidate();
    if (strcmp(name, "five-star-v521") == 0)
        return fc_profile_five_star_loss_aware_candidate();
    if (strcmp(name, "four-star-control") == 0)
        return fc_profile_frozen_four_star_control();
    if (strcmp(name, "five-star-v51") == 0) {
        FCAIProfile profile = fc_profile_five_star();
        profile.name = "five-star-curated-opening-advisor-v51-control";
        return profile;
    }
    if (strcmp(name, "five-star-mn120") == 0 ||
        strcmp(name, "five-star-mn80") == 0 ||
        strcmp(name, "five-star-mn40") == 0 ||
        strcmp(name, "five-star-m0") == 0 ||
        strcmp(name, "five-star-m60") == 0 ||
        strcmp(name, "five-star-m120") == 0) {
        FCAIProfile profile = fc_profile_five_star();
        profile.name = name;
        profile.version = "5.0.0-diagnostic-margin";
        profile.corpusScoreMargin = strcmp(name, "five-star-mn120") == 0 ? -120
                                  : strcmp(name, "five-star-mn80") == 0 ? -80
                                  : strcmp(name, "five-star-mn40") == 0 ? -40
                                  : strcmp(name, "five-star-m0") == 0 ? 0
                                  : strcmp(name, "five-star-m120") == 0 ? 120 : 60;
        return profile;
    }
    if (strcmp(name, "proof-fast") == 0) {
        FCAIProfile profile = fc_profile_proof_guided(false);
        profile.name = "three-star-threat-proof-fast-diagnostic";
        profile.version = "3.0.0-vct-fast-diagnostic";
        profile.proofMaxDepth = 8;
        profile.proofNodeBudget = 6000;
        profile.proofTimeBudgetMs = 80;
        return profile;
    }
    if (strcmp(name, "proof-vcf") == 0) {
        FCAIProfile profile = fc_profile_proof_guided(false);
        profile.name = "three-star-threat-proof-vcf-diagnostic";
        profile.version = "3.0.0-vcf-only-diagnostic";
        profile.proofSearchClass = FC_PROOF_SEARCH_VCF;
        profile.proofMaxDepth = 10;
        profile.proofNodeBudget = 8000;
        profile.proofTimeBudgetMs = 80;
        return profile;
    }
    return fc_profile_production();
}

static bool parse_int(const char *text, int *value)
{
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0') return false;
    *value = (int)parsed;
    return true;
}

static bool parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed = strtoull(text, &end, 0);
    if (end == text || *end != '\0') return false;
    *value = (uint64_t)parsed;
    return true;
}

static bool parse_options(int argc, const char *argv[], BenchmarkOptions *options)
{
    *options = (BenchmarkOptions){
        .profileName = "production", .suiteName = "smoke",
        .outputPath = NULL, .masterSeed = 0, .seedWasProvided = false,
        .randomUserMode = true, .hintOnlyStrategy = false,
        .openingStart = 0, .openingCount = 1, .maxMoves = 120,
        .forbiddenBlack = false, .opponentFourStar = false,
        .pairedPhase = 0, .proofWorkerCountOverride = 0
    };
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            options->profileName = argv[++i];
        } else if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
            options->suiteName = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            options->outputPath = argv[++i];
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &options->masterSeed)) return false;
            options->seedWasProvided = true;
        } else if (strcmp(argv[i], "--random-mode") == 0 && i + 1 < argc) {
            const char *mode = argv[++i];
            if (strcmp(mode, "user") == 0) options->randomUserMode = true;
            else if (strcmp(mode, "best") == 0) options->randomUserMode = false;
            else return false;
        } else if (strcmp(argv[i], "--strategy") == 0 && i + 1 < argc) {
            const char *strategy = argv[++i];
            if (strcmp(strategy, "hybrid") == 0) options->hintOnlyStrategy = false;
            else if (strcmp(strategy, "hint") == 0) options->hintOnlyStrategy = true;
            else return false;
        } else if (strcmp(argv[i], "--opening-start") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->openingStart)) return false;
        } else if (strcmp(argv[i], "--opening-count") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->openingCount)) return false;
        } else if (strcmp(argv[i], "--max-moves") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->maxMoves)) return false;
        } else if (strcmp(argv[i], "--forbidden-black") == 0 && i + 1 < argc) {
            int enabled = 0;
            if (!parse_int(argv[++i], &enabled)) return false;
            options->forbiddenBlack = enabled != 0;
        } else if (strcmp(argv[i], "--opponent") == 0 && i + 1 < argc) {
            const char *opponent = argv[++i];
            if (strcmp(opponent, "four-star") == 0)
                options->opponentFourStar = true;
            else if (strcmp(opponent, "legacy") == 0)
                options->opponentFourStar = false;
            else return false;
        } else if (strcmp(argv[i], "--paired-phase") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->pairedPhase) ||
                (options->pairedPhase != 0 && options->pairedPhase != 1))
                return false;
        } else if (strcmp(argv[i], "--proof-workers") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->proofWorkerCountOverride) ||
                !fc_research_worker_count_is_valid(
                    options->proofWorkerCountOverride))
                return false;
        } else {
            return false;
        }
    }
    if (strcmp(options->suiteName, "training") == 0) {
        if (!options->seedWasProvided) options->masterSeed = FC_TRAINING_MASTER_SEED;
        if (options->masterSeed == FC_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "final") == 0) {
        if (!options->seedWasProvided) options->masterSeed = FC_FINAL_MASTER_SEED;
        if (options->masterSeed != FC_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "proof-final") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = FC_PROOF_FINAL_MASTER_SEED;
        if (options->masterSeed != FC_PROOF_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "five-star-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName, "five-star-natural-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName, "five-star-hybrid-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName,
                      "five-star-parallel-v521-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName,
                      "five-star-v57-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName,
                      "five-star-v541-final") == 0) {
        if (!options->seedWasProvided) return false;
    } else if (strcmp(options->suiteName, "diagnostic") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = 0xD1A64E0571C2026ULL;
        if (options->masterSeed == FC_PROOF_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "elite-diagnostic") == 0 ||
               strcmp(options->suiteName, "elite-fixed") == 0 ||
               strcmp(options->suiteName, "elite-paired") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = UINT64_C(0xe117ed1a62026081);
        if (options->masterSeed == FC_PROOF_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "smoke") == 0) {
        if (!options->seedWasProvided) options->masterSeed = 0x534d4f4b452021ULL;
    } else {
        return false;
    }
    bool formal = strcmp(options->suiteName, "proof-final") == 0 ||
                  strcmp(options->suiteName, "five-star-final") == 0 ||
                  strcmp(options->suiteName, "five-star-natural-final") == 0 ||
                  strcmp(options->suiteName, "five-star-hybrid-final") == 0;
    formal = formal || strcmp(options->suiteName,
                              "five-star-v57-final") == 0;
    formal = formal || strcmp(options->suiteName,
                              "five-star-v541-final") == 0;
    int openingLimit = formal ? fcFormalOpeningCount : 100;
    return options->outputPath != NULL && options->openingCount > 0
        && options->openingStart >= 0 && options->maxMoves >= 16
        && options->openingStart + options->openingCount <= openingLimit;
}

static int generate_curated_opening(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int openingId,
    uint64_t masterSeed,
    bool forbiddenBlack,
    BenchmarkMove moves[8])
{
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    uint64_t identity = mix64(masterSeed ^
                              ((uint64_t)(openingId + 1) << 17));
    int lineId = (int)(identity % (uint64_t)fcOpeningLineCount);
    const FCOpeningLine *line = &fcOpeningLines[lineId];
    int maximumPrefix = line->length > 1 ? line->length - 1 : 1;
    int prefixLength = 1 + (int)((identity >> 8) %
                                 (uint64_t)maximumPrefix);
    if (prefixLength > 8) prefixLength = 8;
    int transform = (int)((identity >> 32) & 7);
    for (int ply = 0; ply < prefixLength; ply++) {
        int rawX = 7 + line->coordinates[ply * 2];
        int rawY = 7 + line->coordinates[ply * 2 + 1];
        int x = 0, y = 0;
        int side = ply % 2 == 0 ? 1 : -1;
        fc_transform_point(transform, rawX, rawY, &x, &y);
        if (!fc_make_move(board, x, y, side, forbiddenBlack)) return 0;
        moves[ply] = (BenchmarkMove){x, y, side};
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side))
            return 0;
    }
    return prefixLength;
}

static int generate_formal_opening(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int openingId,
    bool forbiddenBlack,
    BenchmarkMove moves[FC_BOARD_SIZE * FC_BOARD_SIZE])
{
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    if (openingId < 0 || openingId >= fcFormalOpeningCount) return 0;
    const FCBenchmarkOpening *opening = &fcFormalOpenings[openingId];
    for (int ply = 0; ply < opening->length; ply++) {
        int x = 7 + opening->coordinates[ply * 2];
        int y = 7 + opening->coordinates[ply * 2 + 1];
        int side = ply % 2 == 0 ? 1 : -1;
        if (!fc_make_move(board, x, y, side, forbiddenBlack)) return 0;
        moves[ply] = (BenchmarkMove){x, y, side};
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side))
            return 0;
    }
    return opening->length;
}

static int generate_elite_diagnostic_opening(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int openingId,
    bool forbiddenBlack,
    BenchmarkMove moves[FC_BOARD_SIZE * FC_BOARD_SIZE])
{
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    const FCEliteDiagnosticPosition *position =
        &fcEliteDiagnosticPositions[openingId % fcEliteDiagnosticPositionCount];
    int transform = (openingId / fcEliteDiagnosticPositionCount) & 7;
    for (int ply = 0; ply < position->length; ply++) {
        int x = position->coordinates[ply * 2];
        int y = position->coordinates[ply * 2 + 1];
        int tx = 0, ty = 0;
        int side = ply % 2 == 0 ? 1 : -1;
        fc_transform_point(transform, x, y, &tx, &ty);
        if (!fc_make_move(board, tx, ty, side, forbiddenBlack)) return 0;
        moves[ply] = (BenchmarkMove){tx, ty, side};
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, tx, ty, side))
            return 0;
    }
    return position->length;
}

static int generate_opening(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                            int openingId,
                            uint64_t masterSeed,
                            bool forbiddenBlack,
                            BenchmarkMove moves[8])
{
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    FCRandom random;
    fc_random_seed(&random, mix64(masterSeed ^ (uint64_t)(openingId + 1)));
    int count = 0;
    board[7][7] = 1;
    moves[count++] = (BenchmarkMove){7, 7, 1};
    for (int ply = 1; ply < 6; ply++) {
        int side = ply % 2 == 0 ? 1 : -1;
        int legalX[81];
        int legalY[81];
        int legalCount = 0;
        for (int x = 4; x <= 10; x++) {
            for (int y = 4; y <= 10; y++) {
                if (abs(x - 7) + abs(y - 7) > 5) continue;
                if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                      x, y, side, forbiddenBlack)) continue;
                board[x][y] = side;
                bool wins = fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                                        x, y, side);
                board[x][y] = 0;
                if (wins) continue;
                legalX[legalCount] = x;
                legalY[legalCount] = y;
                legalCount++;
            }
        }
        if (legalCount == 0) return 0;
        int index = (int)(fc_random_next(&random) % (uint64_t)legalCount);
        int x = legalX[index];
        int y = legalY[index];
        board[x][y] = side;
        moves[count++] = (BenchmarkMove){x, y, side};
    }
    return count;
}

static void write_json_string(FILE *output, const char *text)
{
    fputc('"', output);
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', output);
            fputc(*p, output);
        } else if (*p == '\n') {
            fputs("\\n", output);
        } else if (*p >= 0x20) {
            fputc(*p, output);
        }
    }
    fputc('"', output);
}

static void write_header(FILE *output,
                         const BenchmarkOptions *options,
                         const FCAIProfile *profile)
{
    char snapshot[2048] = {0};
    fc_profile_snapshot(profile, snapshot, sizeof(snapshot));
    fprintf(output,
            "{\"type\":\"header\",\"schemaVersion\":5,"
            "\"suite\":\"%s\",\"seedDomain\":\"%s-%s\","
            "\"randomMode\":\"%s\",\"strategy\":\"%s\","
            "\"masterSeed\":\"0x%016llx\",\"openingStart\":%d,"
            "\"openingCount\":%d,\"games\":%d,\"maxMoves\":%d,"
            "\"forbiddenBlack\":%s,\"openingMode\":\"%s\","
            "\"openingBookVersion\":\"%s\",\"eliteCorpusVersion\":\"%s\","
            "\"newProfile\":%s,\"opponentProfile\":\"%s\"}\n",
            options->suiteName, options->suiteName,
            strcmp(options->suiteName, "final") == 0 ? "v2"
            : strcmp(options->suiteName, "proof-final") == 0
                ? "proof-v2"
                : strcmp(options->suiteName, "five-star-final") == 0
                ? "five-star-v1"
                : strcmp(options->suiteName, "five-star-natural-final") == 0
                ? (options->forbiddenBlack ? "five-star-natural-forbidden-v4"
                                           : "five-star-natural-free-v4")
                : strcmp(options->suiteName, "five-star-hybrid-final") == 0
                ? (options->forbiddenBlack ? "five-star-hybrid-forbidden-v1"
                                           : "five-star-hybrid-free-v1")
                : strcmp(options->suiteName,
                         "five-star-parallel-v521-final") == 0
                ? (options->forbiddenBlack ? "parallel-v521-forbidden-v1"
                                           : "parallel-v521-free-v1")
                : strcmp(options->suiteName, "five-star-v57-final") == 0
                ? (options->forbiddenBlack ? "five-star-v57-forbidden-v1"
                                           : "five-star-v57-free-v1")
                : strcmp(options->suiteName, "five-star-v541-final") == 0
                ? (options->forbiddenBlack ? "five-star-v541-forbidden-v1"
                                           : "five-star-v541-free-v1")
                : "v1",
            options->randomUserMode ? "user-softmax" : "deterministic-best",
            options->hintOnlyStrategy ? "legacy-hint-tactical-gate" : "hybrid-deep-verified",
            (unsigned long long)options->masterSeed,
            options->openingStart, options->openingCount,
            options->openingCount *
                (strcmp(options->suiteName, "elite-paired") == 0 ? 4 : 2),
            options->maxMoves,
            options->forbiddenBlack ? "true" : "false",
            strcmp(options->suiteName, "proof-final") == 0 ||
            strcmp(options->suiteName, "five-star-final") == 0 ||
            strcmp(options->suiteName, "five-star-natural-final") == 0 ||
            strcmp(options->suiteName, "five-star-hybrid-final") == 0 ||
            strcmp(options->suiteName,
                   "five-star-parallel-v521-final") == 0 ||
            strcmp(options->suiteName, "five-star-v57-final") == 0 ||
            strcmp(options->suiteName, "five-star-v541-final") == 0
                ? "gomocup-held-out-prefix"
                : (strcmp(options->suiteName, "elite-diagnostic") == 0 ||
                   strcmp(options->suiteName, "elite-fixed") == 0 ||
                   strcmp(options->suiteName, "elite-paired") == 0)
                ? "aggregated-corpus-position-diagnostic"
                : strcmp(options->suiteName, "diagnostic") == 0
                ? "gomocup-curated-prefix" : "seeded-random",
            fc_opening_book_version(), fc_elite_corpus_version(), snapshot,
            options->opponentFourStar
                ? "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book"
                : "legacy-three-star@5224020");
}

static void write_game(FILE *output,
                       int openingId,
                       int newColor,
                       int winner,
                       const char *termination,
                       const BenchmarkMove *moves,
                       int moveCount,
                       const BenchmarkStep *steps,
                       int stepCount,
                       const char *anomaly)
{
    fprintf(output,
            "{\"type\":\"game\",\"openingId\":%d,\"newColor\":%d,"
            "\"winner\":%d,\"termination\":",
            openingId, newColor, winner);
    write_json_string(output, termination);
    fprintf(output, ",\"moveCount\":%d,\"anomaly\":", moveCount);
    if (anomaly == NULL) fputs("null", output); else write_json_string(output, anomaly);
    fputs(",\"moves\":[", output);
    for (int i = 0; i < moveCount; i++) {
        if (i > 0) fputc(',', output);
        fprintf(output, "[%d,%d,%d]", moves[i].x, moves[i].y, moves[i].side);
    }
    fputs("],\"steps\":[", output);
    for (int i = 0; i < stepCount; i++) {
        if (i > 0) fputc(',', output);
        fprintf(output,
                "{\"x\":%d,\"y\":%d,\"side\":%d,\"engine\":\"%s\","
                "\"ms\":%.6f,\"cpuMs\":%.6f,"
                "\"peakResidentBytes\":%llu,"
                "\"nodes\":%llu,\"hits\":%llu,"
                "\"depth\":%d,\"tacticalClass\":%d,"
                "\"candidateCount\":%d,\"hintX\":%d,\"hintY\":%d,"
                "\"choseHint\":%s,\"budgetExhausted\":%s,"
                "\"decisionStatus\":%d,\"fallbackUsed\":%s,"
                "\"candidateCoverageComplete\":%s,"
                "\"decisionLedgerVersion\":%llu,"
                "\"decisionNodesReserved\":%llu,"
                "\"decisionNodesConsumed\":%llu,"
                "\"decisionQueriesReserved\":%llu,"
                "\"decisionQueriesConsumed\":%llu,"
                "\"decisionMemoryReserved\":%llu,"
                "\"decisionMemoryConsumed\":%llu,"
                "\"decisionMemoryPeakReserved\":%llu,"
                "\"decisionMemoryReleased\":%llu,"
                "\"decisionStageExhaustions\":%llu,"
                "\"defaultSource\":%d,\"defaultX\":%d,\"defaultY\":%d,"
                "\"overrideReason\":%d,\"bookId\":%d,\"bookPly\":%d,"
                "\"proofStatus\":%d,\"proofSearchClass\":%d,"
                "\"proofDistance\":%d,\"proofCertificateId\":\"0x%016llx\","
                "\"proofNodes\":%llu,\"proofNumber\":%llu,"
                "\"disproofNumber\":%llu,\"proofParallelNodes\":%llu,"
                "\"proofWorkerCap\":%d,"
                "\"proofWorkersLaunched\":%d,\"proofParallelJobs\":%d,"
                "\"proofParallelJobsCompleted\":%d,"
                "\"proofCertificateVerified\":%s,"
                "\"opponentAfterSelectedStatus\":%d,"
                "\"opponentAfterSelectedDistance\":%d,"
                "\"escapeStage\":%d,\"escapeAlternativesExamined\":%d,"
                "\"escapeScopedDisproofCount\":%d,\"escapeUnknownCount\":%d,"
                "\"escapeVerifiedLossCount\":%d,\"lossReason\":%d,"
                "\"corpusProtectionApplied\":%s,\"quietThreatSelected\":%s,"
                "\"quietRootsExamined\":%d,"
                "\"randomMode\":%d,\"randomCandidateCount\":%d,"
                "\"randomSelectionUsed\":%s,"
                "\"decisionSeed\":\"0x%016llx\","
                "\"randomSelectedRank\":%d,"
                "\"randomEquivalenceSignature\":\"0x%016llx\","
                "\"randomEligibilityVerified\":%s,"
                "\"hybridComponent\":%d,"
                "\"hybridComponentName\":\"%s\","
                "\"hybridComponentVersion\":\"%s\","
                "\"fourStarX\":%d,"
                "\"fourStarY\":%d,\"forkRiskStatus\":%d,"
                "\"selectedOpponentImmediateWinCount\":%d,"
                "\"forkCandidatesExamined\":%d,"
                "\"forkSafeCandidates\":%d,"
                "\"forkRiskyCandidates\":%d,"
                "\"forkUnknownCandidates\":%d,"
                "\"forkAvoidedCount\":%d,"
                "\"forkProbeComplete\":%s,\"handoffReason\":%d,"
                "\"corpusLookup\":%s,"
                "\"corpusPositionIndex\":%d,\"corpusCandidateCount\":%d,"
                "\"corpusMatchType\":%d,\"corpusTrustTier\":%d,"
                "\"corpusRequiredStones\":%d,\"corpusSourceBoardMask\":%d,"
                "\"corpusAccepted\":%s,\"corpusReason\":%d,"
                "\"corpusSupportGames\":%d,\"corpusSupportEvents\":%d,"
                "\"corpusSupportSources\":%d,"
                "\"corpusAcceptedCandidateCount\":%d,"
                "\"diagnostics\":{\"boardHashCellVisits\":%llu,"
                "\"threatCellScans\":%llu,\"refutationCellScans\":%llu,"
                "\"legalityCalls\":%llu,\"legalityBoardCopies\":%llu,"
                "\"forbiddenLegalityCachedChecks\":%llu,"
                "\"forbiddenLegalityCacheMismatches\":%llu,"
                "\"forbiddenLegalityCacheValidationSamples\":%llu,"
                "\"forbiddenLegalityCacheMismatchX\":%llu,"
                "\"forbiddenLegalityCacheMismatchY\":%llu,"
                "\"forbiddenLegalityCacheMismatchSide\":%llu,"
                "\"allocations\":%llu,\"allocatedBytes\":%llu,"
                "\"clearedBytes\":%llu,\"incrementalMakes\":%llu,"
                "\"incrementalUnmakes\":%llu,\"proofSessionQueries\":%llu,"
                "\"proofSessionHits\":%llu,"
                "\"proofSessionGlobalTerminations\":%llu,"
                "\"mostProvingExpansions\":%llu,"
                "\"proofGraphNodes\":%llu,\"proofGraphEdges\":%llu,"
                "\"proofGraphArenaExhaustions\":%llu,"
                "\"completedScopeDisproofs\":%llu,"
                "\"dfpnNoProgressTerminations\":%llu,"
                "\"relevanceAllLegalReplies\":%llu,"
                "\"relevanceZonePoints\":%llu,"
                "\"relevanceReplyCandidates\":%llu,"
                "\"relevanceVerifiedOmissions\":%llu,"
                "\"relevanceUnresolved\":%llu,"
                "\"relevanceFallbacks\":%llu,"
                "\"relevanceFallbackReplies\":%llu,"
                "\"iteratedRelatedZoneIntersections\":%llu,"
                "\"iteratedRelatedZoneContinuations\":%llu,"
                "\"iteratedRelatedZonePointsRemoved\":%llu,"
                "\"dependencyCombinations\":%llu,"
                "\"dependencyChainsProposed\":%llu,"
                "\"dependencyMaximumDepth\":%llu,"
                "\"threatDependencyPoints\":%llu,"
                "\"threatFiveWindowPoints\":%llu,"
                "\"threatLegalityDependencyPoints\":%llu,"
                "\"threatCertificateZonePoints\":%llu,"
                "\"stageImmediateDecisions\":%llu,"
                "\"stageMandatoryDefenseDecisions\":%llu,"
                "\"stageVCFQueries\":%llu,"
                "\"stageVCTQueries\":%llu,"
                "\"stageDependencyQueries\":%llu,"
                "\"stageQuietQueries\":%llu,"
                "\"quietEligibleDecisions\":%llu,"
                "\"quietRootsExamined\":%llu,\"quietUnknowns\":%llu,"
                "\"parallelBatches\":%llu,"
                "\"parallelWorkersLaunched\":%llu,"
                "\"parallelRootJobs\":%llu,"
                "\"parallelRootJobsCompleted\":%llu,"
                "\"parallelRootWins\":%llu,"
                "\"parallelAggregateDisproofs\":%llu,"
                "\"parallelFallbacks\":%llu,"
                "\"parallelExactDuplicateRoots\":%llu,"
                "\"parallelDependencyOverlapPairs\":%llu,"
                "\"parallelFiveWindowOverlapPairs\":%llu,"
                "\"parallelLegalityOverlapPairs\":%llu,"
                "\"parallelCertificateOverlapPairs\":%llu,"
                "\"parallelOverlapPairs\":%llu,"
                "\"parallelOverlapGroups\":%llu,"
                "\"parallelLargestOverlapGroup\":%llu,"
                "\"parallelGroupedRootJobs\":%llu,"
                "\"parallelBudgetTokens\":%llu,"
                "\"parallelMaxConcurrentWorkers\":%llu,"
                "\"parallelIndependentDispatches\":%llu,"
                "\"parallelMultiRootSingleWorkerFallbacks\":%llu,"
                "\"parallelPoolDispatches\":%llu,"
                "\"parallelPoolWorkersReused\":%llu,"
                "\"parallelPoolFallbacks\":%llu,"
                "\"parallelTokenBlockClaims\":%llu,"
                "\"parallelTokenBlockTokens\":%llu,"
                "\"parallelTokenBlockReturns\":%llu,"
                "\"branchFirstPreviewBranches\":%llu,"
                "\"branchFirstAdvancedFourPreviews\":%llu,"
                "\"branchFirstAdvancedThreePreviews\":%llu,"
                "\"branchFirstPreviewIncomplete\":%llu,"
                "\"branchFirstWaves\":%llu,"
                "\"branchFirstWorkersLaunched\":%llu,"
                "\"branchFirstJobs\":%llu,"
                "\"branchFirstJobsCompleted\":%llu,"
                "\"branchFirstVerifiedJobs\":%llu,"
                "\"branchFirstUsefulJobs\":%llu,"
                "\"branchFirstMergeFailures\":%llu,"
                "\"branchFirstUnknownJobs\":%llu,"
                "\"branchFirstMaxConcurrentWorkers\":%llu,"
                "\"branchFirstDispatchFallbacks\":%llu,"
                "\"branchFirstSerialFallbacks\":%llu,"
                "\"branchFirstDepthExtensions\":%llu,"
                "\"branchFirstAdvancedFourDepthExtensions\":%llu,"
                "\"branchFirstAdvancedThreeDepthExtensions\":%llu,"
                "\"branchFirstMaxChildDepth\":%llu,"
                "\"branchFirstDeadlineStops\":%llu,"
                "\"parallelEscapeBatches\":%llu,"
                "\"parallelEscapeWorkersLaunched\":%llu,"
                "\"parallelEscapeJobs\":%llu,"
                "\"parallelEscapeJobsCompleted\":%llu,"
                "\"parallelEscapeMaxConcurrentWorkers\":%llu,"
                "\"parallelEscapeBudgetExhausted\":%llu,"
                "\"decisionCount\":%llu,"
                "\"decisionUnknowns\":%llu,"
                "\"decisionFallbacks\":%llu,"
                "\"decisionNoLegalMoves\":%llu,"
                "\"decisionVerifiedWins\":%llu,"
                "\"decisionVerifiedLosses\":%llu,"
                "\"decisionLedgerExhaustions\":%llu,"
                "\"decisionStageRequests\":%llu,"
                "\"decisionStageReservations\":%llu,"
                "\"decisionStageAbandons\":%llu,"
                "\"parallelDuplicateRootEnumerations\":%llu,"
                "\"parallelEarlyStops\":%llu,"
                "\"dfpnPostponedSiblings\":%llu,"
                "\"dfpnDovetailRequeues\":%llu,"
                "\"decisionMemoryLiveReserved\":%llu,"
                "\"decisionMemoryPeakReserved\":%llu,"
                "\"decisionMemoryReleased\":%llu,"
                "\"forkProbeCandidatesExamined\":%llu,"
                "\"forkProbeSafeCandidates\":%llu,"
                "\"forkProbeRiskyCandidates\":%llu,"
                "\"forkProbeUnknownCandidates\":%llu,"
                "\"forkProbeAvoidedForks\":%llu,"
                "\"forkProbeIncompleteDecisions\":%llu}}",
                steps[i].x, steps[i].y, steps[i].side, steps[i].engine,
                steps[i].milliseconds,
                steps[i].cpuMilliseconds,
                (unsigned long long)steps[i].peakResidentBytes,
                (unsigned long long)steps[i].nodes,
                (unsigned long long)steps[i].hits,
                steps[i].depth, steps[i].tacticalClass,
                steps[i].candidateCount, steps[i].hintX, steps[i].hintY,
                steps[i].choseHint ? "true" : "false",
                steps[i].budgetExhausted ? "true" : "false",
                steps[i].decisionStatus,
                steps[i].fallbackUsed ? "true" : "false",
                steps[i].candidateCoverageComplete ? "true" : "false",
                (unsigned long long)steps[i].decisionLedgerVersion,
                (unsigned long long)steps[i].decisionNodesReserved,
                (unsigned long long)steps[i].decisionNodesConsumed,
                (unsigned long long)steps[i].decisionQueriesReserved,
                (unsigned long long)steps[i].decisionQueriesConsumed,
                (unsigned long long)steps[i].decisionMemoryReserved,
                (unsigned long long)steps[i].decisionMemoryConsumed,
                (unsigned long long)steps[i].decisionMemoryPeakReserved,
                (unsigned long long)steps[i].decisionMemoryReleased,
                (unsigned long long)steps[i].decisionStageExhaustions,
                steps[i].defaultSource, steps[i].defaultX, steps[i].defaultY,
                steps[i].overrideReason, steps[i].bookId, steps[i].bookPly,
                steps[i].proofStatus, steps[i].proofSearchClass,
                steps[i].proofDistance,
                (unsigned long long)steps[i].proofCertificateId,
                (unsigned long long)steps[i].proofNodes,
                (unsigned long long)steps[i].proofNumber,
                (unsigned long long)steps[i].disproofNumber,
                (unsigned long long)steps[i].proofParallelNodes,
                steps[i].proofWorkerCap,
                steps[i].proofWorkersLaunched,
                steps[i].proofParallelJobs,
                steps[i].proofParallelJobsCompleted,
                steps[i].proofCertificateVerified ? "true" : "false",
                steps[i].opponentAfterSelectedStatus,
                steps[i].opponentAfterSelectedDistance,
                steps[i].escapeStage,
                steps[i].escapeAlternativesExamined,
                steps[i].escapeScopedDisproofCount,
                steps[i].escapeUnknownCount,
                steps[i].escapeVerifiedLossCount,
                steps[i].lossReason,
                steps[i].corpusProtectionApplied ? "true" : "false",
                steps[i].quietThreatSelected ? "true" : "false",
                steps[i].quietRootsExamined,
                steps[i].randomMode, steps[i].randomCandidateCount,
                steps[i].randomSelectionUsed ? "true" : "false",
                (unsigned long long)steps[i].decisionSeed,
                steps[i].randomSelectedRank,
                (unsigned long long)steps[i].randomEquivalenceSignature,
                steps[i].randomEligibilityVerified ? "true" : "false",
                steps[i].hybridComponent,
                steps[i].hybridComponentName != NULL
                    ? steps[i].hybridComponentName : "none",
                steps[i].hybridComponentVersion != NULL
                    ? steps[i].hybridComponentVersion : "none",
                steps[i].fourStarX, steps[i].fourStarY,
                steps[i].forkRiskStatus,
                steps[i].selectedOpponentImmediateWinCount,
                steps[i].forkCandidatesExamined,
                steps[i].forkSafeCandidates,
                steps[i].forkRiskyCandidates,
                steps[i].forkUnknownCandidates,
                steps[i].forkAvoidedCount,
                steps[i].forkProbeComplete ? "true" : "false",
                steps[i].handoffReason,
                steps[i].corpusLookup ? "true" : "false",
                steps[i].corpusPositionIndex, steps[i].corpusCandidateCount,
                steps[i].corpusMatchType, steps[i].corpusTrustTier,
                steps[i].corpusRequiredStones, steps[i].corpusSourceBoardMask,
                steps[i].corpusAccepted ? "true" : "false",
                steps[i].corpusReason, steps[i].corpusSupportGames,
                steps[i].corpusSupportEvents, steps[i].corpusSupportSources,
                steps[i].corpusAcceptedCandidateCount,
                (unsigned long long)steps[i].diagnostics.boardHashCellVisits,
                (unsigned long long)steps[i].diagnostics.threatCellScans,
                (unsigned long long)steps[i].diagnostics.refutationCellScans,
                (unsigned long long)steps[i].diagnostics.legalityCalls,
                (unsigned long long)steps[i].diagnostics.legalityBoardCopies,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCachedChecks,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCacheMismatches,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCacheValidationSamples,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCacheMismatchX,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCacheMismatchY,
                (unsigned long long)steps[i].diagnostics.forbiddenLegalityCacheMismatchSide,
                (unsigned long long)steps[i].diagnostics.allocations,
                (unsigned long long)steps[i].diagnostics.allocatedBytes,
                (unsigned long long)steps[i].diagnostics.clearedBytes,
                (unsigned long long)steps[i].diagnostics.incrementalMakes,
                (unsigned long long)steps[i].diagnostics.incrementalUnmakes,
                (unsigned long long)steps[i].diagnostics.proofSessionQueries,
                (unsigned long long)steps[i].diagnostics.proofSessionHits,
                (unsigned long long)steps[i].diagnostics.proofSessionGlobalTerminations,
                (unsigned long long)steps[i].diagnostics.mostProvingExpansions,
                (unsigned long long)steps[i].diagnostics.proofGraphNodes,
                (unsigned long long)steps[i].diagnostics.proofGraphEdges,
                (unsigned long long)steps[i].diagnostics.proofGraphArenaExhaustions,
                (unsigned long long)steps[i].diagnostics.completedScopeDisproofs,
                (unsigned long long)steps[i].diagnostics.dfpnNoProgressTerminations,
                (unsigned long long)steps[i].diagnostics.relevanceAllLegalReplies,
                (unsigned long long)steps[i].diagnostics.relevanceZonePoints,
                (unsigned long long)steps[i].diagnostics.relevanceReplyCandidates,
                (unsigned long long)steps[i].diagnostics.relevanceVerifiedOmissions,
                (unsigned long long)steps[i].diagnostics.relevanceUnresolved,
                (unsigned long long)steps[i].diagnostics.relevanceFallbacks,
                (unsigned long long)steps[i].diagnostics.relevanceFallbackReplies,
                (unsigned long long)steps[i].diagnostics.iteratedRelatedZoneIntersections,
                (unsigned long long)steps[i].diagnostics.iteratedRelatedZoneContinuations,
                (unsigned long long)steps[i].diagnostics.iteratedRelatedZonePointsRemoved,
                (unsigned long long)steps[i].diagnostics.dependencyCombinations,
                (unsigned long long)steps[i].diagnostics.dependencyChainsProposed,
                (unsigned long long)steps[i].diagnostics.dependencyMaximumDepth,
                (unsigned long long)steps[i].diagnostics.threatDependencyPoints,
                (unsigned long long)steps[i].diagnostics.threatFiveWindowPoints,
                (unsigned long long)steps[i].diagnostics.threatLegalityDependencyPoints,
                (unsigned long long)steps[i].diagnostics.threatCertificateZonePoints,
                (unsigned long long)steps[i].diagnostics.stageImmediateDecisions,
                (unsigned long long)steps[i].diagnostics.stageMandatoryDefenseDecisions,
                (unsigned long long)steps[i].diagnostics.stageVCFQueries,
                (unsigned long long)steps[i].diagnostics.stageVCTQueries,
                (unsigned long long)steps[i].diagnostics.stageDependencyQueries,
                (unsigned long long)steps[i].diagnostics.stageQuietQueries,
                (unsigned long long)steps[i].diagnostics.quietEligibleDecisions,
                (unsigned long long)steps[i].diagnostics.quietRootsExamined,
                (unsigned long long)steps[i].diagnostics.quietUnknowns,
                (unsigned long long)steps[i].diagnostics.parallelBatches,
                (unsigned long long)steps[i].diagnostics.parallelWorkersLaunched,
                (unsigned long long)steps[i].diagnostics.parallelRootJobs,
                (unsigned long long)steps[i].diagnostics.parallelRootJobsCompleted,
                (unsigned long long)steps[i].diagnostics.parallelRootWins,
                (unsigned long long)steps[i].diagnostics.parallelAggregateDisproofs,
                (unsigned long long)steps[i].diagnostics.parallelFallbacks,
                (unsigned long long)steps[i].diagnostics.parallelExactDuplicateRoots,
                (unsigned long long)steps[i].diagnostics.parallelDependencyOverlapPairs,
                (unsigned long long)steps[i].diagnostics.parallelFiveWindowOverlapPairs,
                (unsigned long long)steps[i].diagnostics.parallelLegalityOverlapPairs,
                (unsigned long long)steps[i].diagnostics.parallelCertificateOverlapPairs,
                (unsigned long long)steps[i].diagnostics.parallelOverlapPairs,
                (unsigned long long)steps[i].diagnostics.parallelOverlapGroups,
                (unsigned long long)steps[i].diagnostics.parallelLargestOverlapGroup,
                (unsigned long long)steps[i].diagnostics.parallelGroupedRootJobs,
                (unsigned long long)steps[i].diagnostics.parallelBudgetTokens,
                (unsigned long long)steps[i].diagnostics.parallelMaxConcurrentWorkers,
                (unsigned long long)steps[i].diagnostics.parallelIndependentDispatches,
                (unsigned long long)steps[i].diagnostics.parallelMultiRootSingleWorkerFallbacks,
                (unsigned long long)steps[i].diagnostics.parallelPoolDispatches,
                (unsigned long long)steps[i].diagnostics.parallelPoolWorkersReused,
                (unsigned long long)steps[i].diagnostics.parallelPoolFallbacks,
                (unsigned long long)steps[i].diagnostics.parallelTokenBlockClaims,
                (unsigned long long)steps[i].diagnostics.parallelTokenBlockTokens,
                (unsigned long long)steps[i].diagnostics.parallelTokenBlockReturns,
                (unsigned long long)steps[i].diagnostics.branchFirstPreviewBranches,
                (unsigned long long)steps[i].diagnostics.branchFirstAdvancedFourPreviews,
                (unsigned long long)steps[i].diagnostics.branchFirstAdvancedThreePreviews,
                (unsigned long long)steps[i].diagnostics.branchFirstPreviewIncomplete,
                (unsigned long long)steps[i].diagnostics.branchFirstWaves,
                (unsigned long long)steps[i].diagnostics.branchFirstWorkersLaunched,
                (unsigned long long)steps[i].diagnostics.branchFirstJobs,
                (unsigned long long)steps[i].diagnostics.branchFirstJobsCompleted,
                (unsigned long long)steps[i].diagnostics.branchFirstVerifiedJobs,
                (unsigned long long)steps[i].diagnostics.branchFirstUsefulJobs,
                (unsigned long long)steps[i].diagnostics.branchFirstMergeFailures,
                (unsigned long long)steps[i].diagnostics.branchFirstUnknownJobs,
                (unsigned long long)steps[i].diagnostics.branchFirstMaxConcurrentWorkers,
                (unsigned long long)steps[i].diagnostics.branchFirstDispatchFallbacks,
                (unsigned long long)steps[i].diagnostics.branchFirstSerialFallbacks,
                (unsigned long long)steps[i].diagnostics.branchFirstDepthExtensions,
                (unsigned long long)steps[i].diagnostics.branchFirstAdvancedFourDepthExtensions,
                (unsigned long long)steps[i].diagnostics.branchFirstAdvancedThreeDepthExtensions,
                (unsigned long long)steps[i].diagnostics.branchFirstMaxChildDepth,
                (unsigned long long)steps[i].diagnostics.branchFirstDeadlineStops,
                (unsigned long long)steps[i].diagnostics.parallelEscapeBatches,
                (unsigned long long)steps[i].diagnostics.parallelEscapeWorkersLaunched,
                (unsigned long long)steps[i].diagnostics.parallelEscapeJobs,
                (unsigned long long)steps[i].diagnostics.parallelEscapeJobsCompleted,
                (unsigned long long)steps[i].diagnostics.parallelEscapeMaxConcurrentWorkers,
                (unsigned long long)steps[i].diagnostics.parallelEscapeBudgetExhausted,
                (unsigned long long)steps[i].diagnostics.decisionCount,
                (unsigned long long)steps[i].diagnostics.decisionUnknowns,
                (unsigned long long)steps[i].diagnostics.decisionFallbacks,
                (unsigned long long)steps[i].diagnostics.decisionNoLegalMoves,
                (unsigned long long)steps[i].diagnostics.decisionVerifiedWins,
                (unsigned long long)steps[i].diagnostics.decisionVerifiedLosses,
                (unsigned long long)steps[i].diagnostics.decisionLedgerExhaustions,
                (unsigned long long)steps[i].diagnostics.decisionStageRequests,
                (unsigned long long)steps[i].diagnostics.decisionStageReservations,
                (unsigned long long)steps[i].diagnostics.decisionStageAbandons,
                (unsigned long long)steps[i].diagnostics.parallelDuplicateRootEnumerations,
                (unsigned long long)steps[i].diagnostics.parallelEarlyStops,
                (unsigned long long)steps[i].diagnostics.dfpnPostponedSiblings,
                (unsigned long long)steps[i].diagnostics.dfpnDovetailRequeues,
                (unsigned long long)steps[i].diagnostics.decisionMemoryLiveReserved,
                (unsigned long long)steps[i].diagnostics.decisionMemoryPeakReserved,
                (unsigned long long)steps[i].diagnostics.decisionMemoryReleased,
                (unsigned long long)steps[i].diagnostics.forkProbeCandidatesExamined,
                (unsigned long long)steps[i].diagnostics.forkProbeSafeCandidates,
                (unsigned long long)steps[i].diagnostics.forkProbeRiskyCandidates,
                (unsigned long long)steps[i].diagnostics.forkProbeUnknownCandidates,
                (unsigned long long)steps[i].diagnostics.forkProbeAvoidedForks,
                (unsigned long long)steps[i].diagnostics.forkProbeIncompleteDecisions);
    }
    fputs("]}\n", output);
    fflush(output);
}

static bool run_game(FILE *output,
                     const BenchmarkOptions *options,
                     const FCAIProfile *profile,
                     int openingId,
                     int newColor)
{
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
    BenchmarkMove moves[FC_BOARD_SIZE * FC_BOARD_SIZE];
    BenchmarkStep steps[FC_BOARD_SIZE * FC_BOARD_SIZE];
    bool formal = strcmp(options->suiteName, "proof-final") == 0 ||
                  strcmp(options->suiteName, "five-star-final") == 0 ||
                  strcmp(options->suiteName, "five-star-natural-final") == 0 ||
                  strcmp(options->suiteName, "five-star-hybrid-final") == 0 ||
                  strcmp(options->suiteName,
                         "five-star-parallel-v521-final") == 0 ||
                  strcmp(options->suiteName, "five-star-v57-final") == 0 ||
                  strcmp(options->suiteName, "five-star-v541-final") == 0;
    bool curated = strcmp(options->suiteName, "diagnostic") == 0;
    bool elitePaired = strcmp(options->suiteName, "elite-paired") == 0;
    bool eliteFixed = strcmp(options->suiteName, "elite-fixed") == 0 ||
                      elitePaired;
    bool eliteDiagnostic = eliteFixed ||
        strcmp(options->suiteName, "elite-diagnostic") == 0;
    int moveCount = eliteDiagnostic
        ? generate_elite_diagnostic_opening(board, openingId,
                                            options->forbiddenBlack, moves)
        : formal
        ? generate_formal_opening(board, openingId,
                                  options->forbiddenBlack, moves)
        : curated
            ? generate_curated_opening(board, openingId,
                                       options->masterSeed,
                                       options->forbiddenBlack, moves)
            : generate_opening(board, openingId, options->masterSeed,
                               options->forbiddenBlack, moves);
    int stepCount = 0;
    if (moveCount == 0) {
        write_game(output, openingId, newColor, 0, "invalid-opening",
                   moves, 0, steps, 0, "opening generation failed");
        return false;
    }

    doublethree *legacy = [[doublethree alloc] init];
    [legacy set_banmode:options->forbiddenBlack ? 1 : 0];
    for (int i = 0; i < moveCount; i++) {
        [legacy add_a_chess:moves[i].x pl_y:moves[i].y mode:moves[i].side];
    }

    int winner = 0;
    const char *termination = "max-moves-draw";
    const char *anomaly = NULL;
    while (moveCount < options->maxMoves && moveCount < FC_BOARD_SIZE * FC_BOARD_SIZE) {
        int side = moveCount % 2 == 0 ? 1 : -1;
        bool useNew = side == newColor;
        int x = -1;
        int y = -1;
        BenchmarkStep step;
        memset(&step, 0, sizeof(step));
        step.side = side;
        step.engine = useNew
            ? (elitePaired ? options->profileName : "new") : "legacy";
        step.hintX = -1;
        step.hintY = -1;
        step.defaultX = -1;
        step.defaultY = -1;
        step.bookId = -1;
        step.bookPly = -1;
        step.fourStarX = -1;
        step.fourStarY = -1;
        step.corpusPositionIndex = -1;
        uint64_t seed = decision_seed(options->masterSeed, openingId,
                                      moveCount, side, useNew);
        fc_proof_diagnostics_reset();
        double started = now_milliseconds();
        double cpuStarted = process_cpu_milliseconds();
        if (useNew && strcmp(options->profileName, "legacy-control") == 0) {
            doublethree *advisor = [[doublethree alloc] init];
            [advisor set_banmode:options->forbiddenBlack ? 1 : 0];
            [advisor set_legacy_random_seed:seed];
            for (int i = 0; i < moveCount; i++) {
                [advisor add_a_chess:moves[i].x pl_y:moves[i].y
                                mode:moves[i].side];
            }
            [advisor harsh_analysisboard:side];
            int position[2] = {-1, -1};
            [advisor get_last_pos_return_color:position];
            x = position[0];
            y = position[1];
            step.hintX = x;
            step.hintY = y;
            step.choseHint = true;
            step.defaultSource = FC_DEFAULT_LEGACY;
            step.defaultX = x;
            step.defaultY = y;
        } else if (useNew) {
            doublethree *advisor = [[doublethree alloc] init];
            [advisor set_banmode:options->forbiddenBlack ? 1 : 0];
            uint64_t advisorSeed = decision_seed(options->masterSeed, openingId,
                                                  moveCount, side, false);
            if (!options->randomUserMode &&
                strncmp(options->profileName, "five-star", 9) == 0) {
                advisorSeed = fc_board_key(
                    (const int (*)[FC_BOARD_SIZE])board, side,
                    options->forbiddenBlack, FC_PROOF_SEARCH_NONE,
                    UINT64_C(0x4556414c424f4152));
            }
            [advisor set_legacy_random_seed:advisorSeed];
            for (int i = 0; i < moveCount; i++) {
                [advisor add_a_chess:moves[i].x pl_y:moves[i].y
                                mode:moves[i].side];
            }
            [advisor harsh_analysisboard:side];
            int hint[2] = {-1, -1};
            [advisor get_last_pos_return_color:hint];
            FCAnalysisResult analysis;
            FCRandomMode mode = options->randomUserMode
                ? FC_RANDOM_USER_GAME : FC_RANDOM_EVALUATION;
            bool found = strcmp(options->profileName,
                                "five-star-color-hybrid") == 0
                ? fc_analyze_five_star_color_hybrid_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack, seed, mode,
                    hint[0], hint[1], &analysis)
                : (strcmp(options->profileName,
                          "five-star-v521-hybrid-serial") == 0 ||
                   strcmp(options->profileName,
                          "five-star-v521-hybrid-parallel") == 0)
                ? fc_analyze_five_star_v521_hybrid_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack,
                    profile->workerCountOverride > 0
                        ? profile->workerCountOverride
                        : profile->proofWorkerCount,
                    seed, mode, hint[0], hint[1], &analysis)
                : strcmp(options->profileName, "five-star-v57") == 0
                ? fc_analyze_five_star_v57_hybrid_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack,
                    profile->workerCountOverride > 0
                        ? profile->workerCountOverride
                        : profile->proofWorkerCount,
                    seed, mode, hint[0], hint[1], &analysis)
                : strcmp(options->profileName,
                         "five-star-v57-thread-scheduler") == 0
                ? fc_analyze_five_star_v57_thread_scheduler_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack,
                    profile->workerCountOverride > 0
                        ? profile->workerCountOverride
                        : profile->proofWorkerCount,
                    seed, mode, hint[0], hint[1], &analysis)
                : strcmp(options->profileName,
                         "five-star-v541-thread-scheduler") == 0
                ? fc_analyze_five_star_v541_thread_scheduler_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack,
                    profile->workerCountOverride > 0
                        ? profile->workerCountOverride
                        : profile->proofWorkerCount,
                    seed, mode, hint[0], hint[1], &analysis)
                : strncmp(options->profileName, "five-star", 9) == 0
                ? fc_analyze_five_star_profile_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack, profile, seed, mode,
                    hint[0], hint[1], &analysis)
                : fc_analyze_with_hint(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, options->forbiddenBlack, profile, seed, mode,
                    hint[0], hint[1], &analysis);
            if (!found) {
                if (analysis.provenLoss) {
                    winner = -side;
                    termination = fc_loss_reason_name(analysis.lossReason);
                } else {
                    anomaly = "new engine returned no legal move";
                    termination = "anomaly";
                }
                break;
            }
            x = analysis.x;
            y = analysis.y;
            step.nodes = analysis.stats.nodes;
            step.hits = analysis.stats.transpositionHits;
            step.depth = analysis.stats.completedDepth;
            step.tacticalClass = analysis.tacticalClass;
            step.candidateCount = analysis.candidateCount;
            step.decisionStatus = analysis.decisionStatus;
            step.fallbackUsed = analysis.fallbackUsed;
            step.candidateCoverageComplete = analysis.candidateCoverageComplete;
            step.decisionLedgerVersion = analysis.decisionLedgerVersion;
            step.decisionNodesReserved = analysis.decisionNodesReserved;
            step.decisionNodesConsumed = analysis.decisionNodesConsumed;
            step.decisionQueriesReserved = analysis.decisionQueriesReserved;
            step.decisionQueriesConsumed = analysis.decisionQueriesConsumed;
            step.decisionMemoryReserved = analysis.decisionMemoryReserved;
            step.decisionMemoryConsumed = analysis.decisionMemoryConsumed;
            step.decisionMemoryPeakReserved =
                analysis.decisionMemoryPeakReserved;
            step.decisionMemoryReleased = analysis.decisionMemoryReleased;
            step.decisionStageExhaustions = analysis.decisionStageExhaustions;
            step.hintX = hint[0];
            step.hintY = hint[1];
            step.choseHint = analysis.x == hint[0] && analysis.y == hint[1];
            if (options->hintOnlyStrategy &&
                analysis.tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
                analysis.tacticalClass != FC_TACTICAL_MUST_DEFEND &&
                fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                 hint[0], hint[1], side,
                                 options->forbiddenBlack)) {
                analysis.x = hint[0];
                analysis.y = hint[1];
                step.choseHint = true;
            }
            x = analysis.x;
            y = analysis.y;
            step.budgetExhausted = analysis.stats.budgetExhausted;
            step.defaultSource = analysis.defaultSource;
            step.defaultX = analysis.defaultX;
            step.defaultY = analysis.defaultY;
            step.overrideReason = analysis.overrideReason;
            step.bookId = analysis.bookId;
            step.bookPly = analysis.bookPly;
            step.proofStatus = analysis.proofStatus;
            step.proofSearchClass = analysis.proofSearchClass;
            step.proofDistance = analysis.proofDistance;
            step.proofCertificateId = analysis.proofCertificateId;
            step.proofNodes = analysis.proofNodes;
            step.proofNumber = analysis.proofNumber;
            step.disproofNumber = analysis.disproofNumber;
            step.proofParallelNodes = analysis.proofParallelNodes;
            step.proofWorkerCap = analysis.proofWorkerCap;
            step.proofWorkersLaunched = analysis.proofWorkersLaunched;
            step.proofParallelJobs = analysis.proofParallelJobs;
            step.proofParallelJobsCompleted =
                analysis.proofParallelJobsCompleted;
            step.proofCertificateVerified =
                analysis.proofCertificateVerified;
            step.opponentAfterSelectedStatus =
                analysis.opponentAfterSelectedStatus;
            step.opponentAfterSelectedDistance =
                analysis.opponentAfterSelectedDistance;
            step.escapeStage = analysis.escapeStage;
            step.escapeAlternativesExamined =
                analysis.escapeAlternativesExamined;
            step.escapeScopedDisproofCount =
                analysis.escapeScopedDisproofCount;
            step.escapeUnknownCount = analysis.escapeUnknownCount;
            step.escapeVerifiedLossCount = analysis.escapeVerifiedLossCount;
            step.lossReason = analysis.lossReason;
            step.corpusProtectionApplied = analysis.corpusProtectionApplied;
            step.quietThreatSelected = analysis.quietThreatSelected;
            step.quietRootsExamined = analysis.quietRootsExamined;
            step.randomMode = analysis.randomMode;
            step.randomCandidateCount = analysis.randomCandidateCount;
            step.randomSelectionUsed = analysis.randomSelectionUsed;
            step.decisionSeed = analysis.seed;
            step.randomSelectedRank = analysis.randomSelectedRank;
            step.randomEquivalenceSignature =
                analysis.randomEquivalenceSignature;
            step.randomEligibilityVerified =
                analysis.randomEligibilityVerified;
            step.hybridComponent = analysis.hybridComponent;
            step.hybridComponentName =
                fc_hybrid_component_name(analysis.hybridComponent);
            step.hybridComponentVersion =
                fc_hybrid_component_version(analysis.hybridComponent);
            step.fourStarX = analysis.fourStarX;
            step.fourStarY = analysis.fourStarY;
            step.forkRiskStatus = analysis.forkRiskStatus;
            step.selectedOpponentImmediateWinCount =
                analysis.selectedOpponentImmediateWinCount;
            step.forkCandidatesExamined = analysis.forkCandidatesExamined;
            step.forkSafeCandidates = analysis.forkSafeCandidates;
            step.forkRiskyCandidates = analysis.forkRiskyCandidates;
            step.forkUnknownCandidates = analysis.forkUnknownCandidates;
            step.forkAvoidedCount = analysis.forkAvoidedCount;
            step.forkProbeComplete = analysis.forkProbeComplete;
            step.handoffReason = analysis.handoffReason;
            step.corpusLookup = analysis.corpusLookup;
            step.corpusPositionIndex = analysis.corpusPositionIndex;
            step.corpusCandidateCount = analysis.corpusCandidateCount;
            step.corpusMatchType = analysis.corpusMatchType;
            step.corpusTrustTier = analysis.corpusTrustTier;
            step.corpusRequiredStones = analysis.corpusRequiredStones;
            step.corpusSourceBoardMask = analysis.corpusSourceBoardMask;
            step.corpusAccepted = analysis.corpusAccepted;
            step.corpusReason = analysis.corpusReason;
            step.corpusSupportGames = analysis.corpusSupportGames;
            step.corpusSupportEvents = analysis.corpusSupportEvents;
            step.corpusSupportSources = analysis.corpusSupportSources;
            step.corpusAcceptedCandidateCount =
                analysis.corpusAcceptedCandidateCount;
        } else if (options->opponentFourStar) {
            doublethree *advisor = [[doublethree alloc] init];
            [advisor set_banmode:options->forbiddenBlack ? 1 : 0];
            [advisor set_legacy_random_seed:seed];
            for (int i = 0; i < moveCount; i++) {
                [advisor add_a_chess:moves[i].x pl_y:moves[i].y
                                mode:moves[i].side];
            }
            [advisor harsh_analysisboard:side];
            int hint[2] = {-1, -1};
            [advisor get_last_pos_return_color:hint];
            FCAnalysisResult analysis;
            bool found = fc_analyze_four_star_with_hint(
                (const int (*)[FC_BOARD_SIZE])board, side,
                options->forbiddenBlack, seed, FC_RANDOM_EVALUATION,
                hint[0], hint[1], &analysis);
            if (!found) {
                if (analysis.provenLoss) {
                    winner = -side;
                    termination = "proven-loss";
                } else {
                    anomaly = "four-star opponent returned no legal move";
                    termination = "anomaly";
                }
                break;
            }
            x = analysis.x;
            y = analysis.y;
            step.engine = "four-star";
            step.nodes = analysis.stats.nodes;
            step.hits = analysis.stats.transpositionHits;
            step.depth = analysis.stats.completedDepth;
            step.tacticalClass = analysis.tacticalClass;
            step.candidateCount = analysis.candidateCount;
            step.hintX = hint[0];
            step.hintY = hint[1];
            step.choseHint = x == hint[0] && y == hint[1];
            step.budgetExhausted = analysis.stats.budgetExhausted;
        } else {
            srand((unsigned int)(seed ^ (seed >> 32)));
            [legacy harsh_analysisboard:side];
            int position[2] = {-1, -1};
            [legacy get_last_pos_return_color:position];
            x = position[0];
            y = position[1];
        }
        step.diagnostics = fc_proof_diagnostics_get();
        step.cpuMilliseconds = process_cpu_milliseconds() - cpuStarted;
        step.peakResidentBytes = peak_resident_bytes();
        step.milliseconds = now_milliseconds() - started;
        step.x = x;
        step.y = y;

        if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                              x, y, side, options->forbiddenBlack)) {
            if (!useNew && !options->opponentFourStar) {
                winner = -side;
                termination = "legacy-illegal-move-loss";
            } else {
                anomaly = useNew ? "new engine produced illegal move"
                                 : "four-star engine produced illegal move";
                termination = "anomaly";
            }
            break;
        }
        board[x][y] = side;
        moves[moveCount++] = (BenchmarkMove){x, y, side};
        steps[stepCount++] = step;
        if (useNew && !options->opponentFourStar) {
            [legacy add_a_chess:x pl_y:y mode:side];
        }
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side)) {
            winner = side;
            termination = "five-in-a-row";
            break;
        }
        if (eliteFixed && useNew) {
            termination = "fixed-decision-complete";
            break;
        }
    }

    write_game(output, openingId, newColor, winner, termination,
               moves, moveCount, steps, stepCount, anomaly);
    return anomaly == NULL;
}

int main(int argc, const char *argv[])
{
    @autoreleasepool {
        BenchmarkOptions options;
        if (!parse_options(argc, argv, &options)) {
            fprintf(stderr,
                    "usage: %s --output PATH [--profile production|legacy-control|proof-book|proof-no-book|four-star-control|five-star|five-star-color-hybrid|five-star-v521-hybrid-serial|five-star-v521-hybrid-parallel|five-star-v57|five-star-v57-thread-scheduler|five-star-v541-thread-scheduler|five-star-v57-branch-first|five-star-v51|five-star-v521|five-star-mn120|five-star-mn80|five-star-mn40|five-star-m0|five-star-m60|five-star-m120|proof-fast|proof-vcf|a0|b1|b2|c1|c2|d1|d2|d3] "
                    "[--suite smoke|training|final|diagnostic|elite-diagnostic|elite-fixed|elite-paired|proof-final|five-star-final|five-star-natural-final|five-star-hybrid-final|five-star-parallel-v521-final|five-star-v57-final|five-star-v541-final] "
                    "[--random-mode user|best] "
                    "[--strategy hybrid|hint] "
                    "[--seed N] [--opening-start N] [--opening-count N] "
                    "[--max-moves N] [--forbidden-black 0|1] "
                    "[--opponent legacy|four-star] [--paired-phase 0|1] "
                "[--proof-workers 1|4|8]\n",
                    argv[0]);
            return 2;
        }
        FILE *output = fopen(options.outputPath, "w");
        if (output == NULL) {
            fprintf(stderr, "cannot open %s: %s\n",
                    options.outputPath, strerror(errno));
            return 2;
        }
        bool paired = strcmp(options.suiteName, "elite-paired") == 0;
        FCAIProfile profile = paired
            ? profile_named("five-star")
            : profile_named(options.profileName);
        if (options.proofWorkerCountOverride > 0 &&
            (profile.proofEngineCandidate || profile.parallelProofEnabled ||
             profile.branchFirstSearchEnabled))
            profile.workerCountOverride = options.proofWorkerCountOverride;
        write_header(output, &options, &profile);
        BenchmarkOptions controlOptions = options;
        controlOptions.profileName = "five-star-v51";
        BenchmarkOptions candidateOptions = options;
        candidateOptions.profileName = "five-star";
        FCAIProfile controlProfile = profile_named(controlOptions.profileName);
        FCAIProfile candidateProfile = profile_named(candidateOptions.profileName);
        if (options.proofWorkerCountOverride > 0)
            candidateProfile.workerCountOverride =
                options.proofWorkerCountOverride;
        if (paired) {
            char controlSnapshot[2048] = {0};
            char candidateSnapshot[2048] = {0};
            fc_profile_snapshot(&controlProfile, controlSnapshot,
                                sizeof(controlSnapshot));
            fc_profile_snapshot(&candidateProfile, candidateSnapshot,
                                sizeof(candidateSnapshot));
            fprintf(output,
                    "{\"type\":\"paired-profiles\",\"control\":%s,"
                    "\"candidate\":%s,\"order\":"
                    "\"alternating-by-opening-and-color\"}\n",
                    controlSnapshot, candidateSnapshot);
        }
        int anomalies = 0;
        for (int offset = 0; offset < options.openingCount; offset++) {
            int openingId = options.openingStart + offset;
            if (paired) {
                const int colors[2] = {1, -1};
                for (int colorIndex = 0; colorIndex < 2; colorIndex++) {
                    int color = colors[colorIndex];
                    bool candidateFirst =
                        (((offset * 2 + colorIndex) ^
                          options.pairedPhase) & 1) != 0;
                    if (candidateFirst) {
                        if (!run_game(output, &candidateOptions,
                                      &candidateProfile, openingId, color))
                            anomalies++;
                        if (!run_game(output, &controlOptions,
                                      &controlProfile, openingId, color))
                            anomalies++;
                    } else {
                        if (!run_game(output, &controlOptions,
                                      &controlProfile, openingId, color))
                            anomalies++;
                        if (!run_game(output, &candidateOptions,
                                      &candidateProfile, openingId, color))
                            anomalies++;
                    }
                }
            } else {
                if (!run_game(output, &options, &profile, openingId, 1))
                    anomalies++;
                if (!run_game(output, &options, &profile, openingId, -1))
                    anomalies++;
            }
            fprintf(stderr, "opening %d complete (%d/%d)\n",
                    openingId, offset + 1, options.openingCount);
        }
        fclose(output);
        if (anomalies > 0) {
            fprintf(stderr, "%d anomalous games recorded\n", anomalies);
            return 3;
        }
    }
    return 0;
}
