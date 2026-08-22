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
    int recoverySource;
    int recoveryImmediateWinCount;
    int recoveryForkRisk;
    int recoveryForkRepliesExamined;
    bool recoveryForkProbeComplete;
    bool recoveryVCTEscalatedOnUnknown;
    bool recoveryFinalCandidateConsistent;
    int recoveryBaselineX;
    int recoveryBaselineY;
    int recoveryDefaultX;
    int recoveryDefaultY;
    int recoveryHandoffX;
    int recoveryHandoffY;
    int handoffReason;
    bool opponentGuardEligible;
    int opponentGuardSkipReason;
    int opponentGuardProvisionalX;
    int opponentGuardProvisionalY;
    int opponentGuardProvisionalClass;
    int opponentGuardSelectedX;
    int opponentGuardSelectedY;
    int opponentGuardSelectedClass;
    int opponentGuardVCFStatus;
    int opponentGuardVCFDistance;
    uint64_t opponentGuardVCFNodes;
    double opponentGuardVCFMilliseconds;
    bool opponentGuardVCFCertificateVerified;
    int opponentGuardVCTStatus;
    int opponentGuardVCTDistance;
    uint64_t opponentGuardVCTNodes;
    double opponentGuardVCTMilliseconds;
    bool opponentGuardVCTEligible;
    bool opponentGuardVCTCertificateVerified;
    uint32_t opponentGuardAuditedStages;
    int opponentGuardAuditedCount;
    int opponentGuardCompletedDisproofs;
    int opponentGuardUnknowns;
    int opponentGuardVerifiedLosses;
    uint64_t opponentGuardReservedNodes;
    uint64_t opponentGuardConsumedNodes;
    bool opponentGuardAvoidedVerifiedLoss;
    bool opponentGuardRollback;
    bool earlyVCFEligible;
    int earlyVCFSkipReason;
    int earlyVCFPolicy;
    int earlyVCFEffectiveDepth;
    int earlyVCFProvisionalX;
    int earlyVCFProvisionalY;
    int earlyVCFSelectedX;
    int earlyVCFSelectedY;
    int earlyVCFStatus;
    int earlyVCFDistance;
    uint64_t earlyVCFNodes;
    double earlyVCFMilliseconds;
    bool earlyVCFCertificateVerified;
    int earlyVCFAuditedCount;
    int earlyVCFVerifiedLosses;
    int earlyVCFReplacementSource;
    bool earlyVCFAvoidedVerifiedLoss;
    bool earlyVCFAdaptiveEscalated;
    bool earlyVCFRollback;
    bool earlyVCFCacheReused;
    uint64_t earlyVCFCacheHits;
    uint64_t earlyVCFConsumedNodes;
    double earlyVCFDownstreamBudgetRemainingMs;
    bool earlyVCFFinalGuardOnlyLoss;
    bool earlyVCFEvidenceMismatch;
    int doubleThreeStatus;
    bool doubleThreeScanComplete;
    bool doubleThreeScanOverflow;
    int doubleThreeGainCount;
    int doubleThreeProvisionalResidualCount;
    int doubleThreeSelectedResidualCount;
    int doubleThreeCandidatesExamined;
    int doubleThreeCandidatesEliminated;
    bool doubleThreeOwnVCFBypass;
    bool doubleThreeStructuralOverride;
    bool doubleThreeRollback;
    bool doubleThreeDeadlineAnomaly;
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
    int explicitOpeningCount;
    int explicitOpeningIds[100];
    int maxMoves;
    bool forbiddenBlack;
    bool blackOnly;
    const char *mutationSpec;
    const char *nodeId;
    const char *parentNodeId;
    const char *sampleLabel;
    bool opponentFourStar;
    bool opponentThreeStar;
    bool opponentFiveStarControl;
    bool opponentFiveStarEarlyVCFControl;
    int pairedPhase;
    int proofWorkerCountOverride;
    int guardVCFDepthOverride;
    int guardVCTDepthOverride;
    uint64_t guardVCFNodesOverride;
    uint64_t guardVCTNodesOverride;
    int guardVCFTimeMsOverride;
    int guardVCTTimeMsOverride;
    uint64_t guardReservedNodesOverride;
    int guardReservedTimeMsOverride;
    int guardStructuralVCTOverride;
    int guardMaxAlternativesOverride;
    int sentinelPolicyOverride;
    int sentinelBaseDepthOverride;
    int sentinelMaxDepthOverride;
    uint64_t sentinelNodesOverride;
    int sentinelTimeMsOverride;
    int sentinelMaxAlternativesOverride;
} BenchmarkOptions;

static const uint64_t FC_TRAINING_MASTER_SEED = 0xA8B6C4D220260813ULL;
static const uint64_t FC_FINAL_MASTER_SEED = 0xF1CE5EED20260814ULL;
static const uint64_t FC_PROOF_FINAL_MASTER_SEED = 0xC0DEC0DE20260813ULL;
static const uint64_t FC_DOUBLE_THREE_BENCHMARK_MASTER_SEED =
    UINT64_C(0xD0B1E3E20260821);

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
    if (strcmp(name, "five-star-opponent-guard") == 0)
        return fc_profile_five_star_opponent_guard_candidate();
    if (strcmp(name, "five-star-early-vcf") == 0)
        return fc_profile_five_star_early_micro_vcf_candidate();
    if (strcmp(name, "five-star-5.8.2") == 0)
        return fc_profile_five_star_black_double_three_candidate();
    if (strcmp(name, "five-star-5.8.2-recovery") == 0)
        return fc_profile_five_star_black_defense_recovery_candidate();
    if (strcmp(name, "five-star-5.8.1") == 0)
        return fc_profile_five_star_early_micro_vcf_candidate();
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

static bool parse_opening_ids(const char *text, BenchmarkOptions *options)
{
    if (text == NULL || options == NULL || *text == '\0') return false;
    size_t length = strlen(text);
    if (length >= 4096) return false;
    char buffer[4096];
    memcpy(buffer, text, length + 1);
    int count = 0;
    for (char *token = strtok(buffer, ","); token != NULL;
         token = strtok(NULL, ",")) {
        if (count >= (int)(sizeof(options->explicitOpeningIds) /
                           sizeof(options->explicitOpeningIds[0]))) {
            return false;
        }
        int openingId = -1;
        if (!parse_int(token, &openingId) || openingId < 0 ||
            openingId >= 100) return false;
        for (int i = 0; i < count; i++) {
            if (options->explicitOpeningIds[i] == openingId) return false;
        }
        options->explicitOpeningIds[count++] = openingId;
    }
    if (count <= 0) return false;
    options->explicitOpeningCount = count;
    options->openingStart = -1;
    options->openingCount = count;
    return true;
}

static bool parse_bool_mutation_value(const char *value, bool *out)
{
    int parsed = -1;
    if (!parse_int(value, &parsed) || (parsed != 0 && parsed != 1))
        return false;
    *out = parsed != 0;
    return true;
}

static bool apply_one_research_mutation(FCAIProfile *profile,
                                        const char *mutationSpec)
{
    if (profile == NULL || mutationSpec == NULL ||
        strcmp(mutationSpec, "none") == 0) return true;
    const char *separator = strchr(mutationSpec, '=');
    if (separator == NULL || separator == mutationSpec ||
        separator[1] == '\0') return false;
    size_t keyLength = (size_t)(separator - mutationSpec);
    if (keyLength >= 64) return false;
    char key[64];
    memcpy(key, mutationSpec, keyLength);
    key[keyLength] = '\0';
    const char *value = separator + 1;
    bool enabled = false;
    int parsed = 0;
    if (strcmp(key, "black-double-three-weight") == 0) {
        if (!parse_int(value, &parsed) || parsed < 0 || parsed > 100)
            return false;
        profile->blackDoubleThreeDefenseEnabled = parsed > 0;
        profile->blackDoubleThreeDefenseWeight = parsed;
        profile->blackDoubleThreeMaxGains = 16;
        profile->blackDoubleThreeMaxCandidates = 32;
        profile->blackDoubleThreeTimeBudgetMs = 80;
    } else if (strcmp(key, "immediate-block") == 0) {
        if (!parse_bool_mutation_value(value, &enabled)) return false;
        profile->opponentGuardImmediateBlockEnabled = enabled;
        if (enabled) profile->opponentGuardEnabled = true;
    } else if (strcmp(key, "two-step-fork") == 0) {
        if (!parse_bool_mutation_value(value, &enabled)) return false;
        profile->opponentGuardTwoStepForkEnabled = enabled;
        if (enabled) {
            profile->opponentGuardEnabled = true;
            profile->opponentGuardForkMaxReplies = FC_BOARD_SIZE * FC_BOARD_SIZE;
            profile->opponentGuardForkNodeBudget = 120000;
            profile->opponentGuardForkTimeBudgetMs = 150;
        }
    } else if (strcmp(key, "vct-on-unknown") == 0) {
        if (!parse_bool_mutation_value(value, &enabled)) return false;
        profile->opponentGuardVCTOnUnknownEnabled = enabled;
        if (enabled) {
            profile->opponentGuardEnabled = true;
            profile->opponentGuardRecoveryReservedNodes = 112000;
            profile->opponentGuardRecoveryReservedTimeMs = 1600;
            profile->opponentGuardReservedNodes = 112000;
            profile->opponentGuardReservedTimeMs = 1600;
        }
    } else if (strcmp(key, "guard-max-alternatives") == 0) {
        if (!parse_int(value, &parsed) || parsed < 1 || parsed > 32)
            return false;
        profile->opponentGuardMaxAlternatives = parsed;
        profile->opponentGuardEnabled = true;
    } else if (strcmp(key, "recovery-ordering") == 0) {
        if (strcmp(value, "immediate-block") == 0) {
            profile->opponentGuardImmediateBlockEnabled = true;
            profile->opponentGuardEnabled = true;
        } else if (strcmp(value, "structural") == 0) {
            profile->opponentGuardImmediateBlockEnabled = false;
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

static bool apply_research_mutation(FCAIProfile *profile,
                                    const char *mutationSpec)
{
    if (profile == NULL || mutationSpec == NULL ||
        strcmp(mutationSpec, "none") == 0) return true;
    size_t length = strlen(mutationSpec);
    if (length >= 4096) return false;
    char buffer[4096];
    memcpy(buffer, mutationSpec, length + 1);
    for (char *token = strtok(buffer, ";"); token != NULL;
         token = strtok(NULL, ";")) {
        if (!apply_one_research_mutation(profile, token)) return false;
    }
    profile->name = "five-star-random-candidate";
    profile->version = mutationSpec;
    return true;
}

static int opening_id_for_offset(const BenchmarkOptions *options, int offset)
{
    if (options->explicitOpeningCount > 0)
        return options->explicitOpeningIds[offset];
    return options->openingStart + offset;
}

static bool parse_options(int argc, const char *argv[], BenchmarkOptions *options)
{
    *options = (BenchmarkOptions){
        .profileName = "production", .suiteName = "smoke",
        .outputPath = NULL, .masterSeed = 0, .seedWasProvided = false,
        .randomUserMode = true, .hintOnlyStrategy = false,
        .openingStart = 0, .openingCount = 1, .maxMoves = 120,
        .explicitOpeningCount = 0,
        .forbiddenBlack = false, .blackOnly = false,
        .mutationSpec = "none", .nodeId = NULL, .parentNodeId = NULL,
        .sampleLabel = NULL, .opponentFourStar = false,
        .opponentThreeStar = false,
        .opponentFiveStarControl = false,
        .opponentFiveStarEarlyVCFControl = false,
        .pairedPhase = 0, .proofWorkerCountOverride = 0,
        .guardVCFDepthOverride = -1, .guardVCTDepthOverride = -1,
        .guardVCFNodesOverride = 0, .guardVCTNodesOverride = 0,
        .guardVCFTimeMsOverride = -1, .guardVCTTimeMsOverride = -1,
        .guardReservedNodesOverride = 0,
        .guardReservedTimeMsOverride = -1,
        .guardStructuralVCTOverride = -1,
        .guardMaxAlternativesOverride = -1,
        .sentinelPolicyOverride = -1,
        .sentinelBaseDepthOverride = -1,
        .sentinelMaxDepthOverride = -1,
        .sentinelNodesOverride = 0,
        .sentinelTimeMsOverride = -1,
        .sentinelMaxAlternativesOverride = -1
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
        } else if (strcmp(argv[i], "--opening-ids") == 0 && i + 1 < argc) {
            if (!parse_opening_ids(argv[++i], options)) return false;
        } else if (strcmp(argv[i], "--max-moves") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->maxMoves)) return false;
        } else if (strcmp(argv[i], "--forbidden-black") == 0 && i + 1 < argc) {
            int enabled = 0;
            if (!parse_int(argv[++i], &enabled)) return false;
            options->forbiddenBlack = enabled != 0;
        } else if (strcmp(argv[i], "--black-only") == 0 && i + 1 < argc) {
            int enabled = 0;
            if (!parse_int(argv[++i], &enabled)) return false;
            options->blackOnly = enabled != 0;
        } else if (strcmp(argv[i], "--mutation") == 0 && i + 1 < argc) {
            options->mutationSpec = argv[++i];
        } else if (strcmp(argv[i], "--node-id") == 0 && i + 1 < argc) {
            options->nodeId = argv[++i];
        } else if (strcmp(argv[i], "--parent-node-id") == 0 && i + 1 < argc) {
            options->parentNodeId = argv[++i];
        } else if (strcmp(argv[i], "--sample-label") == 0 && i + 1 < argc) {
            options->sampleLabel = argv[++i];
        } else if (strcmp(argv[i], "--opponent") == 0 && i + 1 < argc) {
            const char *opponent = argv[++i];
            if (strcmp(opponent, "four-star") == 0)
                options->opponentFourStar = true;
            else if (strcmp(opponent, "three-star") == 0)
                options->opponentThreeStar = true;
            else if (strcmp(opponent, "five-star-control") == 0)
                options->opponentFiveStarControl = true;
            else if (strcmp(opponent, "five-star-5.8.1") == 0)
                options->opponentFiveStarEarlyVCFControl = true;
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
        } else if (strcmp(argv[i], "--guard-vcf-depth") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardVCFDepthOverride)) return false;
        } else if (strcmp(argv[i], "--guard-vct-depth") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardVCTDepthOverride)) return false;
        } else if (strcmp(argv[i], "--guard-vcf-nodes") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &options->guardVCFNodesOverride)) return false;
        } else if (strcmp(argv[i], "--guard-vct-nodes") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &options->guardVCTNodesOverride)) return false;
        } else if (strcmp(argv[i], "--guard-vcf-ms") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardVCFTimeMsOverride)) return false;
        } else if (strcmp(argv[i], "--guard-vct-ms") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardVCTTimeMsOverride)) return false;
        } else if (strcmp(argv[i], "--guard-reserved-nodes") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &options->guardReservedNodesOverride)) return false;
        } else if (strcmp(argv[i], "--guard-reserved-ms") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardReservedTimeMsOverride)) return false;
        } else if (strcmp(argv[i], "--guard-structural-vct") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardStructuralVCTOverride) ||
                (options->guardStructuralVCTOverride != 0 &&
                 options->guardStructuralVCTOverride != 1)) return false;
        } else if (strcmp(argv[i], "--guard-max-alternatives") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->guardMaxAlternativesOverride)) return false;
        } else if (strcmp(argv[i], "--sentinel-policy") == 0 && i + 1 < argc) {
            const char *policy = argv[++i];
            options->sentinelPolicyOverride =
                strcmp(policy, "disabled") == 0
                    ? FC_EARLY_VCF_POLICY_DISABLED
                : strcmp(policy, "fixed") == 0
                    ? FC_EARLY_VCF_POLICY_FIXED
                : strcmp(policy, "adaptive") == 0
                    ? FC_EARLY_VCF_POLICY_ADAPTIVE : -2;
            if (options->sentinelPolicyOverride == -2) return false;
        } else if (strcmp(argv[i], "--sentinel-base-depth") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->sentinelBaseDepthOverride)) return false;
        } else if (strcmp(argv[i], "--sentinel-max-depth") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->sentinelMaxDepthOverride)) return false;
        } else if (strcmp(argv[i], "--sentinel-nodes") == 0 && i + 1 < argc) {
            if (!parse_u64(argv[++i], &options->sentinelNodesOverride)) return false;
        } else if (strcmp(argv[i], "--sentinel-ms") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->sentinelTimeMsOverride)) return false;
        } else if (strcmp(argv[i], "--sentinel-max-alternatives") == 0 && i + 1 < argc) {
            if (!parse_int(argv[++i], &options->sentinelMaxAlternativesOverride)) return false;
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
    } else if (strcmp(options->suiteName, "opponent-guard-smoke") == 0) {
        if (!options->seedWasProvided ||
            options->masterSeed != UINT64_C(0x4755415244534d4b) ||
            options->openingStart < 60 ||
            options->openingStart + options->openingCount > 72 ||
            options->forbiddenBlack) return false;
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
    } else if (strcmp(options->suiteName, "five-star-double-three-small") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = FC_DOUBLE_THREE_BENCHMARK_MASTER_SEED;
    } else if (strcmp(options->suiteName,
                      "five-star-defense-recovery-standard") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = FC_DOUBLE_THREE_BENCHMARK_MASTER_SEED;
    } else if (strcmp(options->suiteName,
                      "five-star-candidate-search") == 0) {
        if (!options->seedWasProvided)
            options->masterSeed = FC_TRAINING_MASTER_SEED;
        if (options->explicitOpeningCount != 12 ||
            options->openingCount != 12 || options->openingStart != -1)
            return false;
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
    bool openingRangeValid = options->explicitOpeningCount > 0
        ? options->explicitOpeningCount == options->openingCount
        : options->openingStart >= 0 &&
          options->openingStart + options->openingCount <= openingLimit;
    return options->outputPath != NULL && options->openingCount > 0
        && options->maxMoves >= 16 && openingRangeValid;
}

static void apply_guard_overrides(FCAIProfile *profile,
                                  const BenchmarkOptions *options)
{
    if (profile == NULL || options == NULL || !profile->opponentGuardEnabled)
        return;
    if (options->guardVCFDepthOverride >= 0)
        profile->opponentGuardVCFMaxDepth = options->guardVCFDepthOverride;
    if (options->guardVCTDepthOverride >= 0)
        profile->opponentGuardVCTMaxDepth = options->guardVCTDepthOverride;
    if (options->guardVCFNodesOverride > 0)
        profile->opponentGuardVCFNodeBudget = options->guardVCFNodesOverride;
    if (options->guardVCTNodesOverride > 0)
        profile->opponentGuardVCTNodeBudget = options->guardVCTNodesOverride;
    if (options->guardVCFTimeMsOverride >= 0)
        profile->opponentGuardVCFTimeBudgetMs =
            (uint32_t)options->guardVCFTimeMsOverride;
    if (options->guardVCTTimeMsOverride >= 0)
        profile->opponentGuardVCTTimeBudgetMs =
            (uint32_t)options->guardVCTTimeMsOverride;
    if (options->guardReservedNodesOverride > 0)
        profile->opponentGuardReservedNodes =
            options->guardReservedNodesOverride;
    if (options->guardReservedTimeMsOverride >= 0)
        profile->opponentGuardReservedTimeMs =
            (uint32_t)options->guardReservedTimeMsOverride;
    if (options->guardStructuralVCTOverride >= 0)
        profile->opponentGuardStructuralVCTEnabled =
            options->guardStructuralVCTOverride != 0;
    if (options->guardMaxAlternativesOverride >= 0)
        profile->opponentGuardMaxAlternatives =
            options->guardMaxAlternativesOverride;
}

static void apply_sentinel_overrides(FCAIProfile *profile,
                                     const BenchmarkOptions *options)
{
    if (profile == NULL || options == NULL) return;
    if (options->sentinelPolicyOverride >= 0) {
        profile->earlyVCFSentinelPolicy =
            options->sentinelPolicyOverride;
        profile->earlyVCFSentinelEnabled =
            options->sentinelPolicyOverride !=
                FC_EARLY_VCF_POLICY_DISABLED;
    }
    if (!profile->earlyVCFSentinelEnabled) return;
    if (options->sentinelBaseDepthOverride > 0)
        profile->earlyVCFBaseDepth =
            options->sentinelBaseDepthOverride;
    if (options->sentinelMaxDepthOverride > 0)
        profile->earlyVCFMaxDepth =
            options->sentinelMaxDepthOverride;
    if (options->sentinelNodesOverride > 0)
        profile->earlyVCFNodeBudget = options->sentinelNodesOverride;
    if (options->sentinelTimeMsOverride > 0)
        profile->earlyVCFTimeBudgetMs =
            (uint32_t)options->sentinelTimeMsOverride;
    if (options->sentinelMaxAlternativesOverride >= 0)
        profile->earlyVCFMaxAlternatives =
            options->sentinelMaxAlternativesOverride;
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

static uint64_t opponent_guard_smoke_random_next(uint64_t *state)
{
    uint64_t value = *state;
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

static int generate_opponent_guard_smoke_opening(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int openingId,
    uint64_t masterSeed,
    BenchmarkMove moves[FC_BOARD_SIZE * FC_BOARD_SIZE])
{
    if (openingId < 60 || openingId >= 72 ||
        masterSeed != UINT64_C(0x4755415244534d4b)) return 0;
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    uint64_t state = mix64(masterSeed ^ (uint64_t)(openingId + 1));
    assert(fc_make_move(board, 7, 7, 1, false));
    moves[0] = (BenchmarkMove){7, 7, 1};
    for (int ply = 1; ply < 6; ply++) {
        FCPoint legal[49];
        int legalCount = 0;
        for (int x = 4; x <= 10; x++) {
            for (int y = 4; y <= 10; y++) {
                if (abs(x - 7) + abs(y - 7) > 5 || board[x][y] != 0)
                    continue;
                legal[legalCount++] = (FCPoint){x, y};
            }
        }
        uint64_t value = opponent_guard_smoke_random_next(&state);
        FCPoint selected = legal[value % (uint64_t)legalCount];
        int side = (ply & 1) == 0 ? 1 : -1;
        if (!fc_make_move(board, selected.x, selected.y, side, false))
            return 0;
        moves[ply] = (BenchmarkMove){selected.x, selected.y, side};
    }
    return 6;
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
    char snapshot[8192] = {0};
    char opponentSnapshot[8192] = {0};
    fc_profile_snapshot(profile, snapshot, sizeof(snapshot));
    if (options->opponentFourStar) {
        FCAIProfile opponent = fc_profile_frozen_four_star_control();
        fc_profile_snapshot(&opponent, opponentSnapshot,
                            sizeof(opponentSnapshot));
    }
    fprintf(output,
            "{\"type\":\"header\",\"schemaVersion\":%d,"
            "\"suite\":\"%s\",\"seedDomain\":\"%s-%s\","
            "\"randomMode\":\"%s\",\"strategy\":\"%s\","
            "\"masterSeed\":\"0x%016llx\",\"openingStart\":%d,"
            "\"openingCount\":%d,\"games\":%d,\"maxMoves\":%d,"
            "\"forbiddenBlack\":%s,\"openingMode\":\"%s\","
            "\"openingBookVersion\":\"%s\",\"eliteCorpusVersion\":\"%s\","
            "\"newProfile\":%s,\"opponentProfile\":\"%s\"",
            options->explicitOpeningCount > 0 ? 6 : 5,
            options->suiteName,
            options->suiteName,
            strcmp(options->suiteName, "final") == 0 ? "v2"
            : strcmp(options->suiteName, "proof-final") == 0
                ? "proof-v2"
                : strcmp(options->suiteName, "five-star-final") == 0
                ? "five-star-v1"
                : strcmp(options->suiteName, "five-star-natural-final") == 0
                ? (options->forbiddenBlack ? "five-star-natural-forbidden-v4"
                                           : "five-star-natural-free-v4")
                : strcmp(options->suiteName, "opponent-guard-smoke") == 0
                ? "opponent-guard-smoke-v1"
                : strcmp(options->suiteName,
                         "five-star-defense-recovery-standard") == 0
                ? "five-star-defense-recovery-v1"
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
                (options->blackOnly ? 1
                 : strcmp(options->suiteName, "elite-paired") == 0 ? 4 : 2),
            options->maxMoves,
            options->forbiddenBlack ? "true" : "false",
            strcmp(options->suiteName, "proof-final") == 0 ||
            strcmp(options->suiteName, "five-star-final") == 0 ||
            strcmp(options->suiteName, "five-star-natural-final") == 0 ||
            strcmp(options->suiteName, "opponent-guard-smoke") == 0 ||
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
                ? "gomocup-curated-prefix"
                : strcmp(options->suiteName,
                         "five-star-candidate-search") == 0
                ? "seeded-random-explicit" : "seeded-random",
            fc_opening_book_version(), fc_elite_corpus_version(), snapshot,
            options->opponentFiveStarEarlyVCFControl
                ? "five-star-early-micro-vcf@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a"
                : options->opponentFiveStarControl
                ? "five-star-incremental-dfpn-candidate@5.4.1-transactional-deadline-root-parallel-5s"
                : options->opponentThreeStar
                ? "three-star-production@2.2.0-legacy-hint-safe-gate"
                : options->opponentFourStar
                ? "four-star-proof-guided-no-book@3.0.0-vcf-dfpn-no-book"
                : "legacy-three-star@5224020");
    if (options->opponentFourStar) {
        fputs(",\"opponentProfileSnapshot\":", output);
        fputs(opponentSnapshot, output);
    }
    if (strcmp(options->suiteName, "five-star-candidate-search") == 0) {
        fputs(",\"openingPoolVersion\":\"generated-training-v1\","
              "\"randomSeedAlgorithm\":\"splitmix64-v1\","
              "\"benchmarkIdentity\":\"five_chess_benchmark\"", output);
    } else if (strcmp(options->suiteName,
                      "five-star-natural-final") == 0) {
        fputs(",\"openingPoolVersion\":\"gomocup-formal-held-out-v4\","
              "\"benchmarkIdentity\":\"five_chess_benchmark\"", output);
    }
    if (options->explicitOpeningCount > 0) {
        fputs(",\"openingIds\":[", output);
        for (int i = 0; i < options->explicitOpeningCount; i++) {
            if (i > 0) fputc(',', output);
            fprintf(output, "%d", options->explicitOpeningIds[i]);
        }
        fputs("]", output);
    }
    fprintf(output, ",\"blackOnly\":%s",
            options->blackOnly ? "true" : "false");
    if (options->mutationSpec != NULL) {
        fputs(",\"mutation\":", output);
        write_json_string(output, options->mutationSpec);
    }
    if (options->nodeId != NULL) {
        fputs(",\"nodeId\":", output);
        write_json_string(output, options->nodeId);
    }
    if (options->parentNodeId != NULL) {
        fputs(",\"parentNodeId\":", output);
        write_json_string(output, options->parentNodeId);
    }
    if (options->sampleLabel != NULL) {
        fputs(",\"sampleLabel\":", output);
        write_json_string(output, options->sampleLabel);
    }
    fputs("}\n", output);
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
                "\"forkProbeComplete\":%s,"
                "\"recoverySource\":%d,"
                "\"recoveryImmediateWinCount\":%d,"
                "\"recoveryForkRisk\":%d,"
                "\"recoveryForkRepliesExamined\":%d,"
                "\"recoveryForkProbeComplete\":%s,"
                "\"recoveryVCTEscalatedOnUnknown\":%s,"
                "\"recoveryFinalCandidateConsistent\":%s,"
                "\"recoveryBaselineX\":%d,"
                "\"recoveryBaselineY\":%d,"
                "\"recoveryDefaultX\":%d,"
                "\"recoveryDefaultY\":%d,"
                "\"recoveryHandoffX\":%d,"
                "\"recoveryHandoffY\":%d,"
                "\"handoffReason\":%d,"
                "\"opponentGuard\":{\"eligible\":%s,"
                "\"skipReason\":%d,\"provisionalX\":%d,"
                "\"provisionalY\":%d,\"provisionalClass\":%d,"
                "\"selectedX\":%d,\"selectedY\":%d,"
                "\"selectedClass\":%d,\"vcfStatus\":%d,"
                "\"vcfDistance\":%d,\"vcfNodes\":%llu,"
                "\"vcfMs\":%.6f,\"vcfCertificateVerified\":%s,"
                "\"vctStatus\":%d,\"vctDistance\":%d,"
                "\"vctNodes\":%llu,\"vctMs\":%.6f,"
                "\"vctEligible\":%s,"
                "\"vctCertificateVerified\":%s,"
                "\"auditedStages\":%u,\"auditedCount\":%d,"
                "\"completedDisproofs\":%d,\"unknowns\":%d,"
                "\"verifiedLosses\":%d,\"reservedNodes\":%llu,"
                "\"consumedNodes\":%llu,\"avoidedVerifiedLoss\":%s,"
                "\"rollback\":%s},"
                "\"earlyVCF\":{\"eligible\":%s,\"skipReason\":%d,"
                "\"policy\":%d,\"effectiveDepth\":%d,"
                "\"provisionalX\":%d,\"provisionalY\":%d,"
                "\"selectedX\":%d,\"selectedY\":%d,"
                "\"status\":%d,\"distance\":%d,\"nodes\":%llu,"
                "\"ms\":%.6f,\"certificateVerified\":%s,"
                "\"auditedCount\":%d,\"verifiedLosses\":%d,"
                "\"replacementSource\":%d,"
                "\"avoidedVerifiedLoss\":%s,"
                "\"adaptiveEscalated\":%s,\"rollback\":%s,"
                "\"cacheReused\":%s,\"cacheHits\":%llu,"
                "\"consumedNodes\":%llu,"
                "\"downstreamBudgetRemainingMs\":%.6f,"
                "\"finalGuardOnlyLoss\":%s,"
                "\"evidenceMismatch\":%s},"
                "\"doubleThree\":{\"status\":%d,"
                "\"scanComplete\":%s,\"scanOverflow\":%s,"
                "\"gainCount\":%d,\"provisionalResidual\":%d,"
                "\"selectedResidual\":%d,\"candidatesExamined\":%d,"
                "\"candidatesEliminated\":%d,\"ownVCFBypass\":%s,"
                "\"structuralOverride\":%s,\"rollback\":%s,"
                "\"deadlineAnomaly\":%s},"
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
                "\"forkProbeIncompleteDecisions\":%llu,"
                "\"opponentGuardEligibleDecisions\":%llu,"
                "\"opponentGuardSkippedDisabled\":%llu,"
                "\"opponentGuardSkippedImmediateWins\":%llu,"
                "\"opponentGuardSkippedVerifiedOwnWins\":%llu,"
                "\"opponentGuardSkippedNoResource\":%llu,"
                "\"opponentGuardVCFQueries\":%llu,"
                "\"opponentGuardVCTQueries\":%llu,"
                "\"opponentGuardVCTStructuralSkips\":%llu,"
                "\"opponentGuardCandidatesAudited\":%llu,"
                "\"opponentGuardCompletedDisproofs\":%llu,"
                "\"opponentGuardUnknowns\":%llu,"
                "\"opponentGuardVerifiedLosses\":%llu,"
                "\"opponentGuardAvoidedVerifiedLosses\":%llu,"
                "\"opponentGuardRollbacks\":%llu,"
                "\"earlyVCFEligibleDecisions\":%llu,"
                "\"earlyVCFSkippedDisabled\":%llu,"
                "\"earlyVCFSkippedImmediateWins\":%llu,"
                "\"earlyVCFSkippedNoResource\":%llu,"
                "\"earlyVCFQueries\":%llu,"
                "\"earlyVCFCompletedProofs\":%llu,"
                "\"earlyVCFCompletedDisproofs\":%llu,"
                "\"earlyVCFUnknowns\":%llu,"
                "\"earlyVCFVerifiedLosses\":%llu,"
                "\"earlyVCFAvoidedVerifiedLosses\":%llu,"
                "\"earlyVCFAdaptiveEscalations\":%llu,"
                "\"earlyVCFCacheHits\":%llu,"
                "\"earlyVCFFreshNodes\":%llu,"
                "\"earlyVCFFinalGuardOnlyLosses\":%llu,"
                "\"earlyVCFDeadlineExhaustions\":%llu,"
                "\"earlyVCFRollbacks\":%llu,"
                "\"earlyVCFEvidenceMismatches\":%llu,"
                "\"doubleThreeScans\":%llu,"
                "\"doubleThreeCompleteScans\":%llu,"
                "\"doubleThreeIncompleteScans\":%llu,"
                "\"doubleThreeGains\":%llu,"
                "\"doubleThreeCandidatesExamined\":%llu,"
                "\"doubleThreeCandidatesEliminated\":%llu,"
                "\"doubleThreeStructuralOverrides\":%llu,"
                "\"doubleThreeOwnVCFBypasses\":%llu,"
                "\"doubleThreeRollbacks\":%llu,"
                "\"doubleThreeDeadlineAnomalies\":%llu}}",
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
                steps[i].recoverySource,
                steps[i].recoveryImmediateWinCount,
                steps[i].recoveryForkRisk,
                steps[i].recoveryForkRepliesExamined,
                steps[i].recoveryForkProbeComplete ? "true" : "false",
                steps[i].recoveryVCTEscalatedOnUnknown ? "true" : "false",
                steps[i].recoveryFinalCandidateConsistent ? "true" : "false",
                steps[i].recoveryBaselineX,
                steps[i].recoveryBaselineY,
                steps[i].recoveryDefaultX,
                steps[i].recoveryDefaultY,
                steps[i].recoveryHandoffX,
                steps[i].recoveryHandoffY,
                steps[i].handoffReason,
                steps[i].opponentGuardEligible ? "true" : "false",
                steps[i].opponentGuardSkipReason,
                steps[i].opponentGuardProvisionalX,
                steps[i].opponentGuardProvisionalY,
                steps[i].opponentGuardProvisionalClass,
                steps[i].opponentGuardSelectedX,
                steps[i].opponentGuardSelectedY,
                steps[i].opponentGuardSelectedClass,
                steps[i].opponentGuardVCFStatus,
                steps[i].opponentGuardVCFDistance,
                (unsigned long long)steps[i].opponentGuardVCFNodes,
                steps[i].opponentGuardVCFMilliseconds,
                steps[i].opponentGuardVCFCertificateVerified
                    ? "true" : "false",
                steps[i].opponentGuardVCTStatus,
                steps[i].opponentGuardVCTDistance,
                (unsigned long long)steps[i].opponentGuardVCTNodes,
                steps[i].opponentGuardVCTMilliseconds,
                steps[i].opponentGuardVCTEligible ? "true" : "false",
                steps[i].opponentGuardVCTCertificateVerified
                    ? "true" : "false",
                steps[i].opponentGuardAuditedStages,
                steps[i].opponentGuardAuditedCount,
                steps[i].opponentGuardCompletedDisproofs,
                steps[i].opponentGuardUnknowns,
                steps[i].opponentGuardVerifiedLosses,
                (unsigned long long)steps[i].opponentGuardReservedNodes,
                (unsigned long long)steps[i].opponentGuardConsumedNodes,
                steps[i].opponentGuardAvoidedVerifiedLoss
                    ? "true" : "false",
                steps[i].opponentGuardRollback ? "true" : "false",
                steps[i].earlyVCFEligible ? "true" : "false",
                steps[i].earlyVCFSkipReason,
                steps[i].earlyVCFPolicy,
                steps[i].earlyVCFEffectiveDepth,
                steps[i].earlyVCFProvisionalX,
                steps[i].earlyVCFProvisionalY,
                steps[i].earlyVCFSelectedX,
                steps[i].earlyVCFSelectedY,
                steps[i].earlyVCFStatus,
                steps[i].earlyVCFDistance,
                (unsigned long long)steps[i].earlyVCFNodes,
                steps[i].earlyVCFMilliseconds,
                steps[i].earlyVCFCertificateVerified ? "true" : "false",
                steps[i].earlyVCFAuditedCount,
                steps[i].earlyVCFVerifiedLosses,
                steps[i].earlyVCFReplacementSource,
                steps[i].earlyVCFAvoidedVerifiedLoss ? "true" : "false",
                steps[i].earlyVCFAdaptiveEscalated ? "true" : "false",
                steps[i].earlyVCFRollback ? "true" : "false",
                steps[i].earlyVCFCacheReused ? "true" : "false",
                (unsigned long long)steps[i].earlyVCFCacheHits,
                (unsigned long long)steps[i].earlyVCFConsumedNodes,
                steps[i].earlyVCFDownstreamBudgetRemainingMs,
                steps[i].earlyVCFFinalGuardOnlyLoss ? "true" : "false",
                steps[i].earlyVCFEvidenceMismatch ? "true" : "false",
                steps[i].doubleThreeStatus,
                steps[i].doubleThreeScanComplete ? "true" : "false",
                steps[i].doubleThreeScanOverflow ? "true" : "false",
                steps[i].doubleThreeGainCount,
                steps[i].doubleThreeProvisionalResidualCount,
                steps[i].doubleThreeSelectedResidualCount,
                steps[i].doubleThreeCandidatesExamined,
                steps[i].doubleThreeCandidatesEliminated,
                steps[i].doubleThreeOwnVCFBypass ? "true" : "false",
                steps[i].doubleThreeStructuralOverride ? "true" : "false",
                steps[i].doubleThreeRollback ? "true" : "false",
                steps[i].doubleThreeDeadlineAnomaly ? "true" : "false",
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
                (unsigned long long)steps[i].diagnostics.forkProbeIncompleteDecisions,
                (unsigned long long)steps[i].diagnostics.opponentGuardEligibleDecisions,
                (unsigned long long)steps[i].diagnostics.opponentGuardSkippedDisabled,
                (unsigned long long)steps[i].diagnostics.opponentGuardSkippedImmediateWins,
                (unsigned long long)steps[i].diagnostics.opponentGuardSkippedVerifiedOwnWins,
                (unsigned long long)steps[i].diagnostics.opponentGuardSkippedNoResource,
                (unsigned long long)steps[i].diagnostics.opponentGuardVCFQueries,
                (unsigned long long)steps[i].diagnostics.opponentGuardVCTQueries,
                (unsigned long long)steps[i].diagnostics.opponentGuardVCTStructuralSkips,
                (unsigned long long)steps[i].diagnostics.opponentGuardCandidatesAudited,
                (unsigned long long)steps[i].diagnostics.opponentGuardCompletedDisproofs,
                (unsigned long long)steps[i].diagnostics.opponentGuardUnknowns,
                (unsigned long long)steps[i].diagnostics.opponentGuardVerifiedLosses,
                (unsigned long long)steps[i].diagnostics.opponentGuardAvoidedVerifiedLosses,
                (unsigned long long)steps[i].diagnostics.opponentGuardRollbacks,
                (unsigned long long)steps[i].diagnostics.earlyVCFEligibleDecisions,
                (unsigned long long)steps[i].diagnostics.earlyVCFSkippedDisabled,
                (unsigned long long)steps[i].diagnostics.earlyVCFSkippedImmediateWins,
                (unsigned long long)steps[i].diagnostics.earlyVCFSkippedNoResource,
                (unsigned long long)steps[i].diagnostics.earlyVCFQueries,
                (unsigned long long)steps[i].diagnostics.earlyVCFCompletedProofs,
                (unsigned long long)steps[i].diagnostics.earlyVCFCompletedDisproofs,
                (unsigned long long)steps[i].diagnostics.earlyVCFUnknowns,
                (unsigned long long)steps[i].diagnostics.earlyVCFVerifiedLosses,
                (unsigned long long)steps[i].diagnostics.earlyVCFAvoidedVerifiedLosses,
                (unsigned long long)steps[i].diagnostics.earlyVCFAdaptiveEscalations,
                (unsigned long long)steps[i].diagnostics.earlyVCFCacheHits,
                (unsigned long long)steps[i].diagnostics.earlyVCFFreshNodes,
                (unsigned long long)steps[i].diagnostics.earlyVCFFinalGuardOnlyLosses,
                (unsigned long long)steps[i].diagnostics.earlyVCFDeadlineExhaustions,
                (unsigned long long)steps[i].diagnostics.earlyVCFRollbacks,
                (unsigned long long)steps[i].diagnostics.earlyVCFEvidenceMismatches,
                (unsigned long long)steps[i].diagnostics.doubleThreeScans,
                (unsigned long long)steps[i].diagnostics.doubleThreeCompleteScans,
                (unsigned long long)steps[i].diagnostics.doubleThreeIncompleteScans,
                (unsigned long long)steps[i].diagnostics.doubleThreeGains,
                (unsigned long long)steps[i].diagnostics.doubleThreeCandidatesExamined,
                (unsigned long long)steps[i].diagnostics.doubleThreeCandidatesEliminated,
                (unsigned long long)steps[i].diagnostics.doubleThreeStructuralOverrides,
                (unsigned long long)steps[i].diagnostics.doubleThreeOwnVCFBypasses,
                (unsigned long long)steps[i].diagnostics.doubleThreeRollbacks,
                (unsigned long long)steps[i].diagnostics.doubleThreeDeadlineAnomalies);
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
    bool opponentGuardSmoke =
        strcmp(options->suiteName, "opponent-guard-smoke") == 0;
    bool elitePaired = strcmp(options->suiteName, "elite-paired") == 0;
    bool eliteFixed = strcmp(options->suiteName, "elite-fixed") == 0 ||
                      elitePaired;
    bool eliteDiagnostic = eliteFixed ||
        strcmp(options->suiteName, "elite-diagnostic") == 0;
    int moveCount = eliteDiagnostic
        ? generate_elite_diagnostic_opening(board, openingId,
                                            options->forbiddenBlack, moves)
        : opponentGuardSmoke
        ? generate_opponent_guard_smoke_opening(
              board, openingId, options->masterSeed, moves)
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
            step.recoverySource = analysis.recoverySource;
            step.recoveryImmediateWinCount =
                analysis.recoveryImmediateWinCount;
            step.recoveryForkRisk = analysis.recoveryForkRisk;
            step.recoveryForkRepliesExamined =
                analysis.recoveryForkRepliesExamined;
            step.recoveryForkProbeComplete =
                analysis.recoveryForkProbeComplete;
            step.recoveryVCTEscalatedOnUnknown =
                analysis.recoveryVCTEscalatedOnUnknown;
            step.recoveryFinalCandidateConsistent =
                analysis.recoveryFinalCandidateConsistent;
            step.recoveryBaselineX = analysis.recoveryBaselineX;
            step.recoveryBaselineY = analysis.recoveryBaselineY;
            step.recoveryDefaultX = analysis.recoveryDefaultX;
            step.recoveryDefaultY = analysis.recoveryDefaultY;
            step.recoveryHandoffX = analysis.recoveryHandoffX;
            step.recoveryHandoffY = analysis.recoveryHandoffY;
            step.handoffReason = analysis.handoffReason;
            step.opponentGuardEligible = analysis.opponentGuardEligible;
            step.opponentGuardSkipReason = analysis.opponentGuardSkipReason;
            step.opponentGuardProvisionalX =
                analysis.opponentGuardProvisionalX;
            step.opponentGuardProvisionalY =
                analysis.opponentGuardProvisionalY;
            step.opponentGuardProvisionalClass =
                analysis.opponentGuardProvisionalClass;
            step.opponentGuardSelectedX = analysis.opponentGuardSelectedX;
            step.opponentGuardSelectedY = analysis.opponentGuardSelectedY;
            step.opponentGuardSelectedClass =
                analysis.opponentGuardSelectedClass;
            step.opponentGuardVCFStatus = analysis.opponentGuardVCFStatus;
            step.opponentGuardVCFDistance =
                analysis.opponentGuardVCFDistance;
            step.opponentGuardVCFNodes = analysis.opponentGuardVCFNodes;
            step.opponentGuardVCFMilliseconds =
                analysis.opponentGuardVCFMilliseconds;
            step.opponentGuardVCFCertificateVerified =
                analysis.opponentGuardVCFCertificateVerified;
            step.opponentGuardVCTStatus = analysis.opponentGuardVCTStatus;
            step.opponentGuardVCTDistance =
                analysis.opponentGuardVCTDistance;
            step.opponentGuardVCTNodes = analysis.opponentGuardVCTNodes;
            step.opponentGuardVCTMilliseconds =
                analysis.opponentGuardVCTMilliseconds;
            step.opponentGuardVCTEligible =
                analysis.opponentGuardVCTEligible;
            step.opponentGuardVCTCertificateVerified =
                analysis.opponentGuardVCTCertificateVerified;
            step.opponentGuardAuditedStages =
                analysis.opponentGuardAuditedStages;
            step.opponentGuardAuditedCount =
                analysis.opponentGuardAuditedCount;
            step.opponentGuardCompletedDisproofs =
                analysis.opponentGuardCompletedDisproofs;
            step.opponentGuardUnknowns = analysis.opponentGuardUnknowns;
            step.opponentGuardVerifiedLosses =
                analysis.opponentGuardVerifiedLosses;
            step.opponentGuardReservedNodes =
                analysis.opponentGuardReservedNodes;
            step.opponentGuardConsumedNodes =
                analysis.opponentGuardConsumedNodes;
            step.opponentGuardAvoidedVerifiedLoss =
                analysis.opponentGuardAvoidedVerifiedLoss;
            step.opponentGuardRollback = analysis.opponentGuardRollback;
            step.earlyVCFEligible = analysis.earlyVCFEligible;
            step.earlyVCFSkipReason = analysis.earlyVCFSkipReason;
            step.earlyVCFPolicy = analysis.earlyVCFPolicy;
            step.earlyVCFEffectiveDepth = analysis.earlyVCFEffectiveDepth;
            step.earlyVCFProvisionalX = analysis.earlyVCFProvisionalX;
            step.earlyVCFProvisionalY = analysis.earlyVCFProvisionalY;
            step.earlyVCFSelectedX = analysis.earlyVCFSelectedX;
            step.earlyVCFSelectedY = analysis.earlyVCFSelectedY;
            step.earlyVCFStatus = analysis.earlyVCFStatus;
            step.earlyVCFDistance = analysis.earlyVCFDistance;
            step.earlyVCFNodes = analysis.earlyVCFNodes;
            step.earlyVCFMilliseconds = analysis.earlyVCFMilliseconds;
            step.earlyVCFCertificateVerified =
                analysis.earlyVCFCertificateVerified;
            step.earlyVCFAuditedCount = analysis.earlyVCFAuditedCount;
            step.earlyVCFVerifiedLosses = analysis.earlyVCFVerifiedLosses;
            step.earlyVCFReplacementSource =
                analysis.earlyVCFReplacementSource;
            step.earlyVCFAvoidedVerifiedLoss =
                analysis.earlyVCFAvoidedVerifiedLoss;
            step.earlyVCFAdaptiveEscalated =
                analysis.earlyVCFAdaptiveEscalated;
            step.earlyVCFRollback = analysis.earlyVCFRollback;
            step.earlyVCFCacheReused = analysis.earlyVCFCacheReused;
            step.earlyVCFCacheHits = analysis.earlyVCFCacheHits;
            step.earlyVCFConsumedNodes = analysis.earlyVCFConsumedNodes;
            step.earlyVCFDownstreamBudgetRemainingMs =
                analysis.earlyVCFDownstreamBudgetRemainingMs;
            step.earlyVCFFinalGuardOnlyLoss =
                analysis.earlyVCFFinalGuardOnlyLoss;
            step.earlyVCFEvidenceMismatch =
                analysis.earlyVCFEvidenceMismatch;
            step.doubleThreeStatus = analysis.doubleThreeStatus;
            step.doubleThreeScanComplete = analysis.doubleThreeScanComplete;
            step.doubleThreeScanOverflow = analysis.doubleThreeScanOverflow;
            step.doubleThreeGainCount = analysis.doubleThreeGainCount;
            step.doubleThreeProvisionalResidualCount =
                analysis.doubleThreeProvisionalResidualCount;
            step.doubleThreeSelectedResidualCount =
                analysis.doubleThreeSelectedResidualCount;
            step.doubleThreeCandidatesExamined =
                analysis.doubleThreeCandidatesExamined;
            step.doubleThreeCandidatesEliminated =
                analysis.doubleThreeCandidatesEliminated;
            step.doubleThreeOwnVCFBypass = analysis.doubleThreeOwnVCFBypass;
            step.doubleThreeStructuralOverride =
                analysis.doubleThreeStructuralOverride;
            step.doubleThreeRollback = analysis.doubleThreeRollback;
            step.doubleThreeDeadlineAnomaly =
                analysis.doubleThreeDeadlineAnomaly;
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
        } else if (options->opponentFiveStarControl ||
                   options->opponentFiveStarEarlyVCFControl) {
            doublethree *advisor = [[doublethree alloc] init];
            [advisor set_banmode:options->forbiddenBlack ? 1 : 0];
            uint64_t advisorSeed = fc_board_key(
                (const int (*)[FC_BOARD_SIZE])board, side,
                options->forbiddenBlack, FC_PROOF_SEARCH_NONE,
                UINT64_C(0x4556414c424f4152));
            [advisor set_legacy_random_seed:advisorSeed];
            for (int i = 0; i < moveCount; i++) {
                [advisor add_a_chess:moves[i].x pl_y:moves[i].y
                                mode:moves[i].side];
            }
            [advisor harsh_analysisboard:side];
            int hint[2] = {-1, -1};
            [advisor get_last_pos_return_color:hint];
            FCAIProfile control = options->opponentFiveStarEarlyVCFControl
                ? fc_profile_five_star_early_micro_vcf_candidate()
                : fc_profile_five_star_proof_engine_candidate();
            FCAnalysisResult analysis;
            bool found = fc_analyze_five_star_profile_with_hint(
                (const int (*)[FC_BOARD_SIZE])board, side,
                options->forbiddenBlack, &control, seed,
                FC_RANDOM_EVALUATION, hint[0], hint[1], &analysis);
            if (!found) {
                if (analysis.provenLoss) {
                    winner = -side;
                    termination = fc_loss_reason_name(analysis.lossReason);
                } else {
                    anomaly = "five-star control returned no legal move";
                    termination = "anomaly";
                }
                break;
            }
            x = analysis.x;
            y = analysis.y;
            step.engine = options->opponentFiveStarEarlyVCFControl
                ? "five-star-5.8.1-control" : "five-star-control";
            step.nodes = analysis.stats.nodes;
            step.hits = analysis.stats.transpositionHits;
            step.depth = analysis.stats.completedDepth;
            step.tacticalClass = analysis.tacticalClass;
            step.candidateCount = analysis.candidateCount;
            step.hintX = hint[0];
            step.hintY = hint[1];
            step.choseHint = x == hint[0] && y == hint[1];
            step.budgetExhausted = analysis.stats.budgetExhausted;
            step.decisionStatus = analysis.decisionStatus;
            step.proofStatus = analysis.proofStatus;
            step.proofSearchClass = analysis.proofSearchClass;
            step.proofDistance = analysis.proofDistance;
            step.proofCertificateId = analysis.proofCertificateId;
            step.proofCertificateVerified =
                analysis.proofCertificateVerified;
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
        } else if (options->opponentThreeStar) {
            FCAIProfile threeStar = fc_profile_production();
            FCAnalysisResult analysis;
            bool found = fc_analyze_with_hint(
                (const int (*)[FC_BOARD_SIZE])board, side,
                options->forbiddenBlack, &threeStar, seed,
                FC_RANDOM_EVALUATION, -1, -1, &analysis);
            if (!found) {
                if (analysis.provenLoss) {
                    winner = -side;
                    termination = fc_loss_reason_name(analysis.lossReason);
                } else {
                    anomaly = "three-star opponent returned no legal move";
                    termination = "anomaly";
                }
                break;
            }
            x = analysis.x;
            y = analysis.y;
            step.engine = "three-star";
            step.nodes = analysis.stats.nodes;
            step.hits = analysis.stats.transpositionHits;
            step.depth = analysis.stats.completedDepth;
            step.tacticalClass = analysis.tacticalClass;
            step.candidateCount = analysis.candidateCount;
            step.budgetExhausted = analysis.stats.budgetExhausted;
            step.decisionStatus = analysis.decisionStatus;
            step.defaultSource = analysis.defaultSource;
            step.defaultX = analysis.defaultX;
            step.defaultY = analysis.defaultY;
            step.overrideReason = analysis.overrideReason;
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
            if (!useNew && !options->opponentFourStar &&
                !options->opponentFiveStarControl &&
                !options->opponentFiveStarEarlyVCFControl) {
                winner = -side;
                termination = "legacy-illegal-move-loss";
            } else {
                anomaly = useNew ? "new engine produced illegal move"
                                 : options->opponentFiveStarControl
                                 ? "five-star control produced illegal move"
                                 : options->opponentFiveStarEarlyVCFControl
                                 ? "5.8.1 control produced illegal move"
                                 : "four-star engine produced illegal move";
                termination = "anomaly";
            }
            break;
        }
        board[x][y] = side;
        moves[moveCount++] = (BenchmarkMove){x, y, side};
        steps[stepCount++] = step;
        if (useNew && !options->opponentFourStar &&
            !options->opponentFiveStarControl &&
            !options->opponentFiveStarEarlyVCFControl) {
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
                    "usage: %s --output PATH [--profile production|legacy-control|proof-book|proof-no-book|four-star-control|five-star|five-star-opponent-guard|five-star-early-vcf|five-star-5.8.2|five-star-5.8.2-recovery|five-star-5.8.1|five-star-color-hybrid|five-star-v521-hybrid-serial|five-star-v521-hybrid-parallel|five-star-v57|five-star-v57-thread-scheduler|five-star-v541-thread-scheduler|five-star-v57-branch-first|five-star-v51|five-star-v521|five-star-mn120|five-star-mn80|five-star-mn40|five-star-m0|five-star-m60|five-star-m120|proof-fast|proof-vcf|a0|b1|b2|c1|c2|d1|d2|d3] "
                    "[--suite smoke|training|final|diagnostic|elite-diagnostic|elite-fixed|elite-paired|proof-final|five-star-final|five-star-natural-final|opponent-guard-smoke|five-star-double-three-small|five-star-defense-recovery-standard|five-star-candidate-search|five-star-hybrid-final|five-star-parallel-v521-final|five-star-v57-final|five-star-v541-final] "
                    "[--random-mode user|best] "
                    "[--strategy hybrid|hint] "
                    "[--seed N] [--opening-start N] [--opening-count N] [--opening-ids CSV] "
                    "[--max-moves N] [--forbidden-black 0|1] [--black-only 0|1] "
                    "[--mutation KEY=VALUE] [--node-id ID] [--parent-node-id ID] [--sample-label LABEL] "
                    "[--opponent legacy|three-star|four-star|five-star-control|five-star-5.8.1] [--paired-phase 0|1] "
                "[--proof-workers 1|4|8] [--guard-vcf-depth N] [--guard-vct-depth N] [--guard-vcf-nodes N] [--guard-vct-nodes N] [--guard-vcf-ms N] [--guard-vct-ms N] [--guard-reserved-nodes N] [--guard-reserved-ms N] [--guard-structural-vct 0|1] [--guard-max-alternatives N] [--sentinel-policy disabled|fixed|adaptive] [--sentinel-base-depth N] [--sentinel-max-depth N] [--sentinel-nodes N] [--sentinel-ms N] [--sentinel-max-alternatives N]\n",
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
        if (!apply_research_mutation(&profile, options.mutationSpec)) {
            fprintf(stderr, "invalid research mutation: %s\n",
                    options.mutationSpec == NULL ? "(null)"
                                                 : options.mutationSpec);
            fclose(output);
            return 2;
        }
        apply_guard_overrides(&profile, &options);
        apply_sentinel_overrides(&profile, &options);
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
            int openingId = opening_id_for_offset(&options, offset);
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
                if (options.blackOnly) {
                    if (!run_game(output, &options, &profile, openingId, -1))
                        anomalies++;
                } else {
                    if (!run_game(output, &options, &profile, openingId, 1))
                        anomalies++;
                    if (!run_game(output, &options, &profile, openingId, -1))
                        anomalies++;
                }
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
