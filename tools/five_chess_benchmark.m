#import <Foundation/Foundation.h>

#import "../ice five chess/doublethree.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int x;
    int y;
    int side;
    const char *engine;
    double milliseconds;
    uint64_t nodes;
    uint64_t hits;
    int depth;
    int tacticalClass;
    int candidateCount;
    int hintX;
    int hintY;
    bool choseHint;
    bool budgetExhausted;
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
} BenchmarkOptions;

static const uint64_t FC_TRAINING_MASTER_SEED = 0xA8B6C4D220260813ULL;
static const uint64_t FC_FINAL_MASTER_SEED = 0xF1CE5EED20260814ULL;
static const uint64_t FC_PROOF_FINAL_MASTER_SEED = 0xC0DEC0DE20260813ULL;

#include "../ice five chess/FiveChessOpeningBook.inc"
#include "FiveChessFormalOpenings.inc"

static double now_milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
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
    *options = (BenchmarkOptions){"production", "smoke", NULL,
                                  0, false, true, false, 0, 1, 120, false};
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
    } else if (strcmp(options->suiteName, "diagnostic") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = 0xD1A64E0571C2026ULL;
        if (options->masterSeed == FC_PROOF_FINAL_MASTER_SEED) return false;
    } else if (strcmp(options->suiteName, "smoke") == 0) {
        if (!options->seedWasProvided) options->masterSeed = 0x534d4f4b452021ULL;
    } else {
        return false;
    }
    return options->outputPath != NULL && options->openingCount > 0
        && options->openingStart >= 0 && options->maxMoves >= 16
        && options->openingStart + options->openingCount <= 100;
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
    if (openingId < 0 || openingId >= 100) return 0;
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
    char snapshot[1024] = {0};
    fc_profile_snapshot(profile, snapshot, sizeof(snapshot));
    fprintf(output,
            "{\"type\":\"header\",\"schemaVersion\":3,"
            "\"suite\":\"%s\",\"seedDomain\":\"%s-%s\","
            "\"randomMode\":\"%s\",\"strategy\":\"%s\","
            "\"masterSeed\":\"0x%016llx\",\"openingStart\":%d,"
            "\"openingCount\":%d,\"games\":%d,\"maxMoves\":%d,"
            "\"forbiddenBlack\":%s,\"openingMode\":\"%s\","
            "\"openingBookVersion\":\"%s\",\"newProfile\":%s,"
            "\"legacyProfile\":\"legacy-three-star@5224020\"}\n",
            options->suiteName, options->suiteName,
            strcmp(options->suiteName, "final") == 0 ? "v2"
            : strcmp(options->suiteName, "proof-final") == 0
                ? "proof-v2" : "v1",
            options->randomUserMode ? "user-softmax" : "deterministic-best",
            options->hintOnlyStrategy ? "legacy-hint-tactical-gate" : "hybrid-deep-verified",
            (unsigned long long)options->masterSeed,
            options->openingStart, options->openingCount,
            options->openingCount * 2, options->maxMoves,
            options->forbiddenBlack ? "true" : "false",
            strcmp(options->suiteName, "proof-final") == 0 ||
            strcmp(options->suiteName, "diagnostic") == 0
                ? "gomocup-curated-prefix" : "seeded-random",
            fc_opening_book_version(), snapshot);
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
                "\"ms\":%.6f,\"nodes\":%llu,\"hits\":%llu,"
                "\"depth\":%d,\"tacticalClass\":%d,"
                "\"candidateCount\":%d,\"hintX\":%d,\"hintY\":%d,"
                "\"choseHint\":%s,\"budgetExhausted\":%s,"
                "\"defaultSource\":%d,\"defaultX\":%d,\"defaultY\":%d,"
                "\"overrideReason\":%d,\"bookId\":%d,\"bookPly\":%d,"
                "\"proofStatus\":%d,\"proofSearchClass\":%d,"
                "\"proofDistance\":%d,\"proofCertificateId\":\"0x%016llx\","
                "\"proofNodes\":%llu,\"proofNumber\":%llu,"
                "\"disproofNumber\":%llu,\"proofCertificateVerified\":%s,"
                "\"randomMode\":%d,\"randomCandidateCount\":%d,"
                "\"randomSelectionUsed\":%s}",
                steps[i].x, steps[i].y, steps[i].side, steps[i].engine,
                steps[i].milliseconds,
                (unsigned long long)steps[i].nodes,
                (unsigned long long)steps[i].hits,
                steps[i].depth, steps[i].tacticalClass,
                steps[i].candidateCount, steps[i].hintX, steps[i].hintY,
                steps[i].choseHint ? "true" : "false",
                steps[i].budgetExhausted ? "true" : "false",
                steps[i].defaultSource, steps[i].defaultX, steps[i].defaultY,
                steps[i].overrideReason, steps[i].bookId, steps[i].bookPly,
                steps[i].proofStatus, steps[i].proofSearchClass,
                steps[i].proofDistance,
                (unsigned long long)steps[i].proofCertificateId,
                (unsigned long long)steps[i].proofNodes,
                (unsigned long long)steps[i].proofNumber,
                (unsigned long long)steps[i].disproofNumber,
                steps[i].proofCertificateVerified ? "true" : "false",
                steps[i].randomMode, steps[i].randomCandidateCount,
                steps[i].randomSelectionUsed ? "true" : "false");
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
    bool formal = strcmp(options->suiteName, "proof-final") == 0;
    bool curated = strcmp(options->suiteName, "diagnostic") == 0;
    int moveCount = formal
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
        step.engine = useNew ? "new" : "legacy";
        step.hintX = -1;
        step.hintY = -1;
        step.defaultX = -1;
        step.defaultY = -1;
        step.bookId = -1;
        step.bookPly = -1;
        uint64_t seed = decision_seed(options->masterSeed, openingId,
                                      moveCount, side, useNew);
        double started = now_milliseconds();
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
            [advisor set_legacy_random_seed:advisorSeed];
            for (int i = 0; i < moveCount; i++) {
                [advisor add_a_chess:moves[i].x pl_y:moves[i].y
                                mode:moves[i].side];
            }
            [advisor harsh_analysisboard:side];
            int hint[2] = {-1, -1};
            [advisor get_last_pos_return_color:hint];
            FCAnalysisResult analysis;
            bool found = fc_analyze_with_hint(
                                    (const int (*)[FC_BOARD_SIZE])board,
                                    side, options->forbiddenBlack, profile, seed,
                                    options->randomUserMode ? FC_RANDOM_USER_GAME
                                                            : FC_RANDOM_EVALUATION,
                                    hint[0], hint[1], &analysis);
            if (!found) {
                if (analysis.provenLoss) {
                    winner = -side;
                    termination = "proven-loss";
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
            step.proofCertificateVerified =
                analysis.proofCertificateVerified;
            step.randomMode = analysis.randomMode;
            step.randomCandidateCount = analysis.randomCandidateCount;
            step.randomSelectionUsed = analysis.randomSelectionUsed;
        } else {
            srand((unsigned int)(seed ^ (seed >> 32)));
            [legacy harsh_analysisboard:side];
            int position[2] = {-1, -1};
            [legacy get_last_pos_return_color:position];
            x = position[0];
            y = position[1];
        }
        step.milliseconds = now_milliseconds() - started;
        step.x = x;
        step.y = y;

        if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                              x, y, side, options->forbiddenBlack)) {
            anomaly = useNew ? "new engine produced illegal move"
                             : "legacy engine produced illegal move";
            termination = "anomaly";
            break;
        }
        board[x][y] = side;
        moves[moveCount++] = (BenchmarkMove){x, y, side};
        steps[stepCount++] = step;
        if (useNew) {
            [legacy add_a_chess:x pl_y:y mode:side];
        }
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, side)) {
            winner = side;
            termination = "five-in-a-row";
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
                    "usage: %s --output PATH [--profile production|legacy-control|proof-book|proof-no-book|proof-fast|proof-vcf|a0|b1|b2|c1|c2|d1|d2|d3] "
                    "[--suite smoke|training|final|diagnostic|proof-final] "
                    "[--random-mode user|best] "
                    "[--strategy hybrid|hint] "
                    "[--seed N] [--opening-start N] [--opening-count N] "
                    "[--max-moves N] [--forbidden-black 0|1]\n",
                    argv[0]);
            return 2;
        }
        FILE *output = fopen(options.outputPath, "w");
        if (output == NULL) {
            fprintf(stderr, "cannot open %s: %s\n",
                    options.outputPath, strerror(errno));
            return 2;
        }
        FCAIProfile profile = profile_named(options.profileName);
        write_header(output, &options, &profile);
        int anomalies = 0;
        for (int offset = 0; offset < options.openingCount; offset++) {
            int openingId = options.openingStart + offset;
            if (!run_game(output, &options, &profile, openingId, 1)) anomalies++;
            if (!run_game(output, &options, &profile, openingId, -1)) anomalies++;
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
