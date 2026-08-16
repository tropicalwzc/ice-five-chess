#ifndef FiveChessAI_h
#define FiveChessAI_h

#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
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
    FC_MAX_CORPUS_CANDIDATES = 8,
    FC_POSITION_BITSET_WORDS = 4,
    FC_MAX_POSITION_DELTAS = FC_BOARD_SIZE * FC_BOARD_SIZE,
    FC_WIN_SCORE = 100000000
};

typedef enum {
    FC_RANDOM_EVALUATION = 0,
    FC_RANDOM_USER_GAME = 1
} FCRandomMode;

typedef enum {
    FC_HYBRID_COMPONENT_NONE = 0,
    FC_HYBRID_COMPONENT_BLACK_V51 = 1,
    FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE = 2,
    FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL = 3,
    FC_HYBRID_COMPONENT_BLACK_V521_SERIAL = 4,
    FC_HYBRID_COMPONENT_BLACK_V57_PARALLEL = 5,
    FC_HYBRID_COMPONENT_V57_BRANCH_FIRST = 6,
    FC_HYBRID_COMPONENT_V541_THREAD_SCHEDULER = 7
} FCHybridComponent;

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
    FC_OVERRIDE_REJECTED_CERTIFICATE = 5,
    FC_OVERRIDE_UNPROVEN_ESCAPE = 6,
    FC_OVERRIDE_LONGEST_SURVIVAL = 7,
    FC_OVERRIDE_QUIET_PROVEN_ATTACK = 8,
    FC_OVERRIDE_FORK_SAFE_RECOVERY = 9
} FCOverrideReason;

typedef enum {
    FC_ESCAPE_NONE = 0,
    FC_ESCAPE_CERTIFICATE = 1,
    FC_ESCAPE_TACTICAL = 2,
    FC_ESCAPE_ORDINARY = 3,
    FC_ESCAPE_ALL_LEGAL = 4
} FCEscapeStage;

typedef enum {
    FC_LOSS_NONE = 0,
    FC_LOSS_SELECTED_VERIFIED = 1,
    FC_LOSS_ALL_EXAMINED_VERIFIED = 2,
    FC_LOSS_BUDGET_UNKNOWN = 3,
    FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED = 4
} FCLossReason;

/* A search result is not a boolean proof.  In particular, a bounded candidate
 * list can be exhausted while legal moves still exist outside that list. */
typedef enum {
    FC_DECISION_NO_LEGAL_MOVE = 0,
    FC_DECISION_VERIFIED_WIN = 1,
    FC_DECISION_VERIFIED_LOSS = 2,
    FC_DECISION_UNKNOWN_OR_DEADLINE = 3
} FCDecisionStatus;

/* Fork annotations are tactical observations, not proof results.  UNKNOWN
 * means that the probe was interrupted before it could classify the move. */
typedef enum {
    FC_FORK_RISK_UNKNOWN = 0,
    FC_FORK_RISK_SAFE = 1,
    FC_FORK_RISK_ONE_REPLY = 2,
    FC_FORK_RISK_FORK = 3,
    FC_FORK_RISK_OWN_WIN = 4
} FCForkRisk;

typedef enum {
    FC_HANDOFF_NONE = 0,
    FC_HANDOFF_FOUR_STAR_INVALID = 1,
    FC_HANDOFF_FOUR_STAR_UNKNOWN = 2,
    FC_HANDOFF_GENERATED_FALLBACK = 3,
    FC_HANDOFF_FULL_BOARD_FALLBACK = 4
} FCHandoffReason;

typedef enum {
    FC_CORPUS_NOT_CHECKED = 0,
    FC_CORPUS_NO_POSITION = 1,
    FC_CORPUS_INSUFFICIENT_SUPPORT = 2,
    FC_CORPUS_UNIQUE_TACTICAL_OBLIGATION = 3,
    FC_CORPUS_ILLEGAL_OR_UNSAFE = 4,
    FC_CORPUS_BASELINE_PROOF_UNKNOWN = 5,
    FC_CORPUS_CANDIDATE_PROOF_UNKNOWN = 6,
    FC_CORPUS_CANDIDATE_PROVEN_UNSAFE = 7,
    FC_CORPUS_SEARCH_INCOMPARABLE = 8,
    FC_CORPUS_SCORE_MARGIN_NOT_MET = 9,
    FC_CORPUS_ACCEPTED_SUPERIOR = 10,
    FC_CORPUS_ACCEPTED_BASELINE_EQUIVALENT = 11,
    FC_CORPUS_BASELINE_PROVEN_UNSAFE = 12,
    FC_CORPUS_ACCEPTED_TRUSTED_NEAR_EQUIVALENT = 13
} FCCorpusDecisionReason;

typedef enum {
    FC_CORPUS_MATCH_NONE = 0,
    FC_CORPUS_MATCH_EXACT = 1,
    FC_CORPUS_MATCH_LOCAL = 2
} FCCorpusMatchType;

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
    int lineIds[4];
    uint64_t dependencyMask[FC_POSITION_BITSET_WORDS];
    uint64_t fiveWindowMask[FC_POSITION_BITSET_WORDS];
    uint64_t legalityDependencyMask[FC_POSITION_BITSET_WORDS];
    uint64_t certificateZoneMask[FC_POSITION_BITSET_WORDS];
} FCThreat;

typedef struct {
    uint64_t boardHash;
    uint64_t initialRelevanceMask[FC_POSITION_BITSET_WORDS];
    uint64_t replyRelatedZoneMask[FC_POSITION_BITSET_WORDS];
    uint64_t relatedZoneMask[FC_POSITION_BITSET_WORDS];
    uint64_t verifiedOmissionMask[FC_POSITION_BITSET_WORDS];
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
    bool proofEngineCandidate;
    /* Enables the five-star candidate-only proof stages without changing the
     * broader proof-engine profile semantics used by the original 5.4 path. */
    bool proofCandidateStagesEnabled;
    bool parallelProofEnabled;
    bool openingBookEnabled;
    int proofSearchClass;
    int proofMaxDepth;
    uint64_t proofNodeBudget;
    uint64_t proofParallelNodeBudget;
    uint32_t proofTimeBudgetMs;
    int proofWorkerCount;
    /* Nonzero only for an explicitly requested research benchmark cell. */
    int workerCountOverride;
    size_t proofTranspositionCapacity;
    size_t proofGraphNodeCapacity;
    size_t proofGraphEdgeCapacity;
    bool lossAwareEnabled;
    bool quietThreatEnabled;
    int proofEscapeCandidateLimit;
    int proofQuietRootLimit;
    uint32_t proofEmergencyTimeBudgetMs;
    uint32_t decisionTimeBudgetMs;
    bool eliteCorpusEnabled;
    /* Resource-contract fields used by research profiles.  Zero preserves the
     * historical profile behavior for frozen controls. */
    uint32_t decisionHardLimitMs;
    uint32_t decisionLedgerVersion;
    uint64_t decisionNodeBudget;
    size_t decisionMemoryBudgetBytes;
    uint32_t decisionCorpusQueryBudget;
    bool incrementalLegalityEnabled;
    bool validateLegalityCache;
    bool recoverySearchEnabled;
    /* Experimental fork-first candidate recovery is opt-in.  This is kept
     * separate from legacy legal fallback/loss-aware recovery. */
    bool forkFirstRecoveryEnabled;
    /* The v5.7 scheduler study is opt-in so the rolled-back v5.7 profile
     * remains an immutable comparison point. */
    bool persistentWorkerPoolEnabled;
    bool parallelTokenBlockEnabled;
    uint32_t parallelTokenBlockSize;
    /* The v5.7 branch-first study keeps root obligations ordered and only
     * dispatches sufficiently long recursive reply waves. */
    bool branchFirstSearchEnabled;
    int branchFirstMinRemainingDepth;
    int branchFirstMinBranchCount;
    int branchFirstMaxBranches;
    int branchFirstPreviewDepth;
    /* Tactical recursive waves may spend their parallel headroom on a
     * slightly deeper continuation.  Zero preserves the serial/default
     * profile contract. */
    int branchFirstAdvancedFourDepthBonus;
    int branchFirstAdvancedThreeDepthBonus;
    int branchFirstTacticalDepthCap;
    int corpusScoreMargin;
    int corpusMinGames;
    int corpusMinEvents;
} FCAIProfile;

typedef struct {
    int x;
    int y;
    int score;
    int tacticalClass;
    bool safe;
    double probability;
    int opponentImmediateWinCount;
    int forkRisk;
    bool forkProbeComplete;
} FCCandidate;

typedef struct {
    int x;
    int y;
    int games;
    int events;
    int sources;
    int trustTier;
    int matchType;
    int requiredStones;
    int sourceBoardMask;
    int score;
    int completedDepth;
    int proofStatus;
    int proofSearchClass;
    int proofDistance;
    int tacticalClass;
    bool accepted;
    int reason;
} FCCorpusCandidateTelemetry;

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
    FCDecisionStatus decisionStatus;
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
    int randomSelectedRank;
    uint64_t randomEquivalenceSignature;
    bool randomEligibilityVerified;
    int hybridComponent;
    int fourStarX;
    int fourStarY;
    int handoffReason;
    int forkRiskStatus;
    int selectedOpponentImmediateWinCount;
    int forkCandidatesExamined;
    int forkSafeCandidates;
    int forkRiskyCandidates;
    int forkUnknownCandidates;
    int forkAvoidedCount;
    bool forkProbeComplete;
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
    FCCorpusCandidateTelemetry corpusCandidates[FC_MAX_CORPUS_CANDIDATES];
} FCAnalysisResult;

typedef struct {
    int x;
    int y;
    int games;
    int events;
    int sources;
    int wins;
    int draws;
    int losses;
    int trustTier;
    int matchType;
    int requiredStones;
    int sourceBoardMask;
} FCEliteCorpusMove;

typedef struct {
    uint64_t state;
} FCRandom;

typedef struct {
    uint64_t boardHashCellVisits;
    uint64_t threatCellScans;
    uint64_t refutationCellScans;
    uint64_t legalityCalls;
    uint64_t legalityBoardCopies;
    uint64_t forbiddenLegalityCachedChecks;
    uint64_t forbiddenLegalityCacheMismatches;
    uint64_t forbiddenLegalityCacheValidationSamples;
    uint64_t forbiddenLegalityCacheMismatchX;
    uint64_t forbiddenLegalityCacheMismatchY;
    uint64_t forbiddenLegalityCacheMismatchSide;
    uint64_t allocations;
    uint64_t allocatedBytes;
    uint64_t clearedBytes;
    uint64_t incrementalInitializations;
    uint64_t incrementalMakes;
    uint64_t incrementalUnmakes;
    uint64_t incrementalDirtyLines;
    uint64_t proofSessionQueries;
    uint64_t proofSessionHits;
    uint64_t proofSessionGlobalTerminations;
    uint64_t mostProvingExpansions;
    uint64_t proofGraphNodes;
    uint64_t proofGraphEdges;
    uint64_t proofGraphArenaExhaustions;
    uint64_t completedScopeDisproofs;
    uint64_t dfpnNoProgressTerminations;
    uint64_t relevanceAllLegalReplies;
    uint64_t relevanceZonePoints;
    uint64_t relevanceReplyCandidates;
    uint64_t relevanceVerifiedOmissions;
    uint64_t relevanceUnresolved;
    uint64_t relevanceFallbacks;
    uint64_t relevanceFallbackReplies;
    uint64_t iteratedRelatedZoneIntersections;
    uint64_t iteratedRelatedZoneContinuations;
    uint64_t iteratedRelatedZonePointsRemoved;
    uint64_t dependencyCombinations;
    uint64_t dependencyChainsProposed;
    uint64_t dependencyMaximumDepth;
    uint64_t threatDependencyPoints;
    uint64_t threatFiveWindowPoints;
    uint64_t threatLegalityDependencyPoints;
    uint64_t threatCertificateZonePoints;
    uint64_t stageImmediateDecisions;
    uint64_t stageMandatoryDefenseDecisions;
    uint64_t stageVCFQueries;
    uint64_t stageVCTQueries;
    uint64_t stageDependencyQueries;
    uint64_t stageQuietQueries;
    uint64_t quietEligibleDecisions;
    uint64_t quietRootsExamined;
    uint64_t quietCompletedProofs;
    uint64_t quietCompletedDisproofs;
    uint64_t quietUnknowns;
    uint64_t parallelBatches;
    uint64_t parallelWorkersLaunched;
    uint64_t parallelRootJobs;
    uint64_t parallelRootJobsCompleted;
    uint64_t parallelRootWins;
    uint64_t parallelAggregateDisproofs;
    uint64_t parallelFallbacks;
    uint64_t parallelExactDuplicateRoots;
    uint64_t parallelDependencyOverlapPairs;
    uint64_t parallelFiveWindowOverlapPairs;
    uint64_t parallelLegalityOverlapPairs;
    uint64_t parallelCertificateOverlapPairs;
    uint64_t parallelOverlapPairs;
    uint64_t parallelOverlapGroups;
    uint64_t parallelLargestOverlapGroup;
    uint64_t parallelGroupedRootJobs;
    uint64_t parallelBudgetTokens;
    uint64_t parallelMaxConcurrentWorkers;
    uint64_t parallelIndependentDispatches;
    uint64_t parallelMultiRootSingleWorkerFallbacks;
    /* Persistent-scheduler telemetry: a dispatch is one pool task wave;
     * workersReused counts pool slots that completed it; fallbacks count
     * waves that could not use the requested pool path.  These counters do
     * not imply additional proof work. */
    uint64_t parallelPoolDispatches;
    uint64_t parallelPoolWorkersReused;
    uint64_t parallelPoolFallbacks;
    /* Token-block telemetry is separate from parallelBudgetTokens: claims
     * reserve bounded aggregate/ledger capacity, tokens counts reservations,
     * and returns counts unused reservations released at query/job exit. */
    uint64_t parallelTokenBlockClaims;
    uint64_t parallelTokenBlockTokens;
    uint64_t parallelTokenBlockReturns;
    uint64_t parallelEscapeBatches;
    uint64_t parallelEscapeWorkersLaunched;
    uint64_t parallelEscapeJobs;
    uint64_t parallelEscapeJobsCompleted;
    uint64_t parallelEscapeMaxConcurrentWorkers;
    uint64_t parallelEscapeBudgetExhausted;
    uint64_t decisionCount;
    uint64_t decisionUnknowns;
    uint64_t decisionFallbacks;
    uint64_t decisionNoLegalMoves;
    uint64_t decisionVerifiedWins;
    uint64_t decisionVerifiedLosses;
    uint64_t decisionLedgerExhaustions;
    uint64_t decisionStageRequests;
    uint64_t decisionStageReservations;
    uint64_t decisionStageAbandons;
    uint64_t parallelDuplicateRootEnumerations;
    uint64_t parallelEarlyStops;
    uint64_t dfpnPostponedSiblings;
    uint64_t dfpnDovetailRequeues;
    uint64_t decisionMemoryLiveReserved;
    uint64_t decisionMemoryPeakReserved;
    uint64_t decisionMemoryReleased;
    uint64_t forkProbeCandidatesExamined;
    uint64_t forkProbeSafeCandidates;
    uint64_t forkProbeRiskyCandidates;
    uint64_t forkProbeUnknownCandidates;
    uint64_t forkProbeAvoidedForks;
    uint64_t forkProbeIncompleteDecisions;
    uint64_t branchFirstPreviewBranches;
    uint64_t branchFirstAdvancedFourPreviews;
    uint64_t branchFirstAdvancedThreePreviews;
    uint64_t branchFirstPreviewIncomplete;
    uint64_t branchFirstWaves;
    uint64_t branchFirstWorkersLaunched;
    uint64_t branchFirstJobs;
    uint64_t branchFirstJobsCompleted;
    uint64_t branchFirstVerifiedJobs;
    uint64_t branchFirstUsefulJobs;
    uint64_t branchFirstMergeFailures;
    uint64_t branchFirstUnknownJobs;
    uint64_t branchFirstMaxConcurrentWorkers;
    uint64_t branchFirstDispatchFallbacks;
    uint64_t branchFirstSerialFallbacks;
    uint64_t branchFirstDepthExtensions;
    uint64_t branchFirstAdvancedFourDepthExtensions;
    uint64_t branchFirstAdvancedThreeDepthExtensions;
    uint64_t branchFirstMaxChildDepth;
    uint64_t branchFirstDeadlineStops;
} FCProofDiagnostics;

/* One resource ledger is shared by all stages of a five-star move.  Memory
 * reservation is live concurrent usage; it is deliberately separate from the
 * cumulative memoryConsumed counter. */
typedef struct {
    double startedMilliseconds;
    double internalDeadlineMilliseconds;
    double hardDeadlineMilliseconds;
    uint64_t nodeBudget;
    uint64_t memoryBudgetBytes;
    uint32_t queryBudget;
    _Atomic uint64_t nodesConsumed;
    _Atomic uint64_t memoryConsumed;
    _Atomic uint64_t memoryPeakReserved;
    _Atomic uint64_t memoryReleased;
    _Atomic uint32_t queriesConsumed;
    _Atomic uint64_t nodesReserved;
    _Atomic uint64_t memoryReserved;
    _Atomic uint32_t queriesReserved;
    uint32_t version;
    /* Written by proof/escape workers and read by the coordinator. */
    _Atomic bool exhausted;
} FCDecisionLedger;

typedef struct {
    int x;
    int y;
    int side;
    uint64_t previousHash;
    uint64_t previousLock;
    uint64_t previousFrontier[FC_POSITION_BITSET_WORDS];
    uint16_t previousLineRevision[4];
    uint32_t previousLineCode[4];
} FCPositionDelta;

typedef struct {
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
    uint64_t stoneHash;
    uint64_t stoneLock;
    uint64_t occupied[FC_POSITION_BITSET_WORDS];
    uint64_t frontier[FC_POSITION_BITSET_WORDS];
    uint64_t legalMoves[2][FC_POSITION_BITSET_WORDS];
    uint64_t immediateWins[2][FC_POSITION_BITSET_WORDS];
    bool legalMaskComplete[2];
    bool immediateWinMaskComplete[2];
    uint16_t lineRevision[4][FC_BOARD_SIZE * 2 - 1];
    uint32_t lineCode[4][FC_BOARD_SIZE * 2 - 1];
    FCThreat threatCache[2][2][FC_MAX_THREATS];
    uint8_t threatCacheCount[2][2];
    bool threatCacheOverflow[2][2];
    bool threatCacheValid[2][2];
    int stoneCount;
    bool forbiddenBlack;
    bool complete;
    bool forbiddenLegalityCacheValid;
    FCPositionDelta deltas[FC_MAX_POSITION_DELTAS];
    int deltaCount;
} FCIncrementalPosition;

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
FCAIProfile fc_profile_frozen_four_star_control(void);
FCAIProfile fc_profile_five_star(void);
FCAIProfile fc_profile_five_star_loss_aware_candidate(void);
FCAIProfile fc_profile_five_star_proof_engine_candidate(void);
FCAIProfile fc_profile_five_star_v541_thread_scheduler_candidate(void);
FCAIProfile fc_profile_five_star_color_hybrid_candidate(void);
FCAIProfile fc_profile_five_star_v521_serial_hybrid_control(void);
FCAIProfile fc_profile_five_star_v521_parallel_hybrid_candidate(void);
FCAIProfile fc_profile_five_star_v57_hybrid_candidate(void);
FCAIProfile fc_profile_five_star_v57_fork_recovery_candidate(void);
FCAIProfile fc_profile_five_star_v57_thread_scheduler_candidate(void);
FCAIProfile fc_profile_five_star_v57_branch_first_candidate(void);

void fc_proof_diagnostics_reset(void);
FCProofDiagnostics fc_proof_diagnostics_get(void);

bool fc_decision_ledger_begin(FCDecisionLedger *ledger,
                              const FCAIProfile *profile);
bool fc_decision_ledger_deadline_reached(FCDecisionLedger *ledger);
bool fc_decision_ledger_reserve_nodes(FCDecisionLedger *ledger,
                                      uint64_t requested);
bool fc_decision_ledger_reserve_memory(FCDecisionLedger *ledger,
                                       uint64_t requested);
void fc_decision_ledger_release_memory(FCDecisionLedger *ledger,
                                       uint64_t released);
bool fc_decision_ledger_reserve_queries(FCDecisionLedger *ledger,
                                        uint32_t requested);
bool fc_decision_ledger_consume_node(FCDecisionLedger *ledger);
bool fc_decision_ledger_consume_query(FCDecisionLedger *ledger);
bool fc_decision_ledger_consume_memory(FCDecisionLedger *ledger,
                                       uint64_t bytes);

/* The fixed worker matrix used by research replay cells.  Production and
 * historical callers may still use their existing worker profile fields. */
bool fc_research_worker_count_is_valid(int workerCount);

bool fc_incremental_position_init(
    FCIncrementalPosition *position,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    bool forbiddenBlack);

bool fc_incremental_position_make(FCIncrementalPosition *position,
                                  int x,
                                  int y,
                                  int side);

bool fc_incremental_position_unmake(FCIncrementalPosition *position);

bool fc_incremental_is_legal_move(const FCIncrementalPosition *position,
                                  int x,
                                  int y,
                                  int side);

bool fc_incremental_validate_legality_cache(
    FCIncrementalPosition *position);

uint64_t fc_incremental_board_key(const FCIncrementalPosition *position,
                                  int side,
                                  int searchClass,
                                  uint64_t profileVersion);

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

int fc_incremental_enumerate_threats(
    FCIncrementalPosition *position,
    int side,
    int searchClass,
    FCThreat *out,
    int capacity,
    bool *overflow);

int fc_reference_generate_refutations(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    FCPoint *out,
    int capacity);

int fc_incremental_generate_refutations(
    FCIncrementalPosition *position,
    int attacker,
    int searchClass,
    FCPoint *out,
    int capacity);

bool fc_threats_dependency_compatible(const FCThreat *left,
                                      const FCThreat *right);

void fc_dfpn_frontier_estimate(int maxDepth,
                               int remainingDepth,
                               int branchingHint,
                               uint64_t *proofNumber,
                               uint64_t *disproofNumber);

bool fc_prove_forced_win(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int attacker,
                         bool forbiddenBlack,
                         int searchClass,
                         int maxDepth,
                         uint64_t nodeBudget,
                         uint32_t timeBudgetMs,
                         size_t transpositionCapacity,
                         FCProofResult *result);

bool fc_prove_forced_win_candidate_session(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *result);

bool fc_test_candidate_proof_session_reuse(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *first,
    FCProofResult *second);

bool fc_test_dfpn_stalled_graph_respects_deadline(double *elapsedMilliseconds);

bool fc_test_corpus_random_equivalent(
    const FCCorpusCandidateTelemetry *best,
    int bestScore,
    const FCCorpusCandidateTelemetry *alternative,
    int alternativeScore);

int fc_test_select_corpus_random_equivalent(
    const FCCorpusCandidateTelemetry *candidates,
    const int *scores,
    int count,
    uint64_t seed);

bool fc_test_parallel_root_proof(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *result);

bool fc_verify_proof(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                     int attacker,
                     bool forbiddenBlack,
                     const FCProofResult *result);

bool fc_verify_scoped_disproof(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
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
const char *fc_escape_stage_name(int stage);
const char *fc_loss_reason_name(int reason);
const char *fc_decision_status_name(int status);
const char *fc_fork_risk_name(int risk);
const char *fc_handoff_reason_name(int reason);
const char *fc_corpus_reason_name(int reason);
const char *fc_elite_corpus_version(void);

int fc_elite_corpus_lookup(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                           int side,
                           bool forbiddenBlack,
                           FCEliteCorpusMove *out,
                           int capacity,
                           int *outPositionIndex);

int fc_elite_corpus_lookup_detailed(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    FCEliteCorpusMove *out,
    int capacity,
    int *outPositionIndex,
    int *outMatchType);

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

bool fc_analyze_four_star_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_profile_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_color_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_v521_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int blackWorkerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_v57_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int blackWorkerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_v57_thread_scheduler_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int workerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

bool fc_analyze_five_star_v541_thread_scheduler_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int workerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result);

const char *fc_hybrid_component_name(int component);
const char *fc_hybrid_component_version(int component);

size_t fc_profile_snapshot(const FCAIProfile *profile, char *buffer, size_t capacity);
const char *fc_tactical_name(int tacticalClass);

#ifdef __cplusplus
}
#endif

#endif
