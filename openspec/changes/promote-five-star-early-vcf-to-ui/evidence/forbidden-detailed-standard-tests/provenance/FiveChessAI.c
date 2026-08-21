#include "FiveChessAI.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FC_PARALLEL_MAX_WORKERS 8
#define FC_PARALLEL_TOKEN_BLOCK_DEFAULT 16U

#if defined(__clang__) || defined(__GNUC__)
#define FC_NOINLINE __attribute__((noinline))
#else
#define FC_NOINLINE
#endif

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
    uint64_t verification;
    int depth;
    uint32_t generation;
    unsigned char status;
} FCProofTTEntry;

typedef struct {
    uint64_t key;
    uint64_t verification;
    uint64_t proofNumber;
    uint64_t disproofNumber;
    int depth;
    uint32_t graphIndexPlusOne;
    uint32_t generation;
    unsigned char status;
} FCDFPNTTEntry;

typedef enum {
    FC_DFPN_OR_NODE = 1,
    FC_DFPN_AND_NODE = 2
} FCDFPNNodeType;

typedef struct {
    uint64_t key;
    uint64_t verification;
    uint64_t proofNumber;
    uint64_t disproofNumber;
    int remainingDepth;
    int firstEdge;
    int edgeCount;
    int selectedEdge;
    int distance;
    uint64_t relevanceMask[FC_POSITION_BITSET_WORDS];
    uint64_t replyRelatedZoneMask[FC_POSITION_BITSET_WORDS];
    uint64_t relatedZoneMask[FC_POSITION_BITSET_WORDS];
    uint64_t verifiedOmissionMask[FC_POSITION_BITSET_WORDS];
    unsigned short relatedZoneContinuationCount;
    unsigned short relatedZoneIntersectionCount;
    unsigned short relatedZonePointsRemoved;
    unsigned char type;
    unsigned char threatSeverity;
    unsigned char status;
    bool expanded;
    bool enumerationComplete;
    bool relatedZoneComplete;
} FCDFPNNode;

typedef struct {
    int child;
    short x;
    short y;
} FCDFPNEdge;

/* A postponed edge is a resumable DFPN obligation, not a loss result.  The
 * queue is bounded because the proof graph itself is bounded; insertion and
 * revisit ordering are explicit so a research run is reproducible regardless
 * of how often an expanded sibling makes no progress. */
typedef struct {
    int edge;
    uint64_t proofThreshold;
    uint64_t disproofThreshold;
    uint64_t revisitEpoch;
    uint64_t insertionIndex;
} FCPostponedEdge;

#define FC_DFPN_NODE_CAPACITY 32768
#define FC_DFPN_EDGE_CAPACITY 131072
#define FC_DFPN_POSTPONED_CAPACITY 256

typedef struct {
    void *memory;
    FCIncrementalPosition position;
    int rootBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    FCDFPNTTEntry *dfpnTable;
    size_t tableCapacity;
    FCDFPNNode *graphNodes;
    size_t graphNodeCapacity;
    size_t graphNodeCount;
    FCDFPNEdge *graphEdges;
    size_t graphEdgeCapacity;
    size_t graphEdgeCount;
    uint64_t nodeBudget;
    uint64_t nodesUsed;
    uint32_t timeBudgetMs;
    double startedMilliseconds;
    uint32_t generation;
    bool rootPositionReady;
    bool positionBasedOnRoot;
    bool forbiddenBlack;
    bool exhausted;
    FCDecisionLedger *ledger;
    uint64_t memoryReservationBytes;
    bool memoryReservationHeld;
} FCProofSession;

enum { FC_EARLY_VCF_CACHE_CAPACITY = 4 };

typedef struct {
    bool valid;
    uint64_t positionKey;
    int side;
    bool forbiddenBlack;
    int searchClass;
    int completedDepth;
    const char *engineVersion;
    FCProofResult proof;
} FCEarlyVCFCacheEntry;

typedef struct {
    FCEarlyVCFCacheEntry entries[FC_EARLY_VCF_CACHE_CAPACITY];
    int count;
    uint64_t hits;
} FCEarlyVCFDecisionCache;

static _Thread_local FCProofSession *fcActiveProofSession;
static _Thread_local FCEarlyVCFDecisionCache *fcActiveEarlyVCFCache;
static _Thread_local uint32_t fcProofGeneration;
static _Thread_local double fcActiveDecisionDeadlineMilliseconds;
static _Thread_local const FCAIProfile *fcActiveBranchFirstProfile;
static _Thread_local int fcProofRootFilterX = -1;
static _Thread_local int fcProofRootFilterY = -1;
/* A parallel root job carries one immutable, coordinator-enumerated gain.
 * Workers use this pointer to enter proof below that gain instead of
 * re-enumerating the complete root threat set.  It is TLS because every
 * worker owns its proof session and mutable position. */
static _Thread_local const FCThreat *fcActiveRootThreat;
static _Thread_local uint64_t fcDecisionParallelNodes;
static _Thread_local int fcDecisionWorkersLaunched;
static _Thread_local int fcDecisionParallelJobs;
static _Thread_local int fcDecisionParallelJobsCompleted;
/* A worker proof query receives a private session, but node tokens are
 * accounted against one aggregate decision budget.  Keeping the counter in
 * TLS avoids passing a dispatcher object through every DFPN helper. */
static _Thread_local _Atomic uint64_t *fcActiveParallelNodeCounter;
static _Thread_local uint64_t fcActiveParallelNodeBudget;
static _Thread_local FCDecisionLedger *fcActiveDecisionLedger;
static _Thread_local uint64_t fcParallelNodeTokensRemaining;
static _Thread_local uint64_t fcDecisionLedgerNodeTokensRemaining;
static _Thread_local uint32_t fcActiveParallelNodeBlockSize;
static _Thread_local bool fcActiveParallelTokenBlocksEnabled;
static _Thread_local bool fcWorkerPoolTaskActive;

typedef void (*FCWorkerPoolTask)(void *context, int workerSlot);

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t workAvailable;
    pthread_cond_t workComplete;
    pthread_t threads[FC_PARALLEL_MAX_WORKERS];
    int threadCount;
    bool initialized;
    bool shuttingDown;
    bool taskActive;
    uint64_t generation;
    FCWorkerPoolTask task;
    void *taskContext;
    int requestedWorkers;
    int completedWorkers;
} FCWorkerPool;

typedef struct {
    FCWorkerPool *pool;
    int workerSlot;
} FCWorkerPoolSlot;

static FCWorkerPool fcWorkerPool;
static FCWorkerPoolSlot fcWorkerPoolSlots[FC_PARALLEL_MAX_WORKERS];
static pthread_once_t fcWorkerPoolOnce = PTHREAD_ONCE_INIT;

static void *fc_worker_pool_main(void *opaque)
{
    FCWorkerPoolSlot *slot = opaque;
    FCWorkerPool *pool = slot->pool;
    uint64_t seenGeneration = 0;
    for (;;) {
        pthread_mutex_lock(&pool->mutex);
        while (!pool->shuttingDown &&
               (!pool->taskActive ||
                pool->generation == seenGeneration ||
                slot->workerSlot >= pool->requestedWorkers)) {
            (void)pthread_cond_wait(&pool->workAvailable, &pool->mutex);
        }
        if (pool->shuttingDown) {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
        seenGeneration = pool->generation;
        FCWorkerPoolTask task = pool->task;
        void *context = pool->taskContext;
        pthread_mutex_unlock(&pool->mutex);

        fcWorkerPoolTaskActive = true;
        if (task != NULL) task(context, slot->workerSlot);
        fcWorkerPoolTaskActive = false;

        pthread_mutex_lock(&pool->mutex);
        pool->completedWorkers++;
        if (pool->completedWorkers >= pool->requestedWorkers)
            (void)pthread_cond_signal(&pool->workComplete);
        pthread_mutex_unlock(&pool->mutex);
    }
}

static void fc_worker_pool_bootstrap(void)
{
    if (pthread_mutex_init(&fcWorkerPool.mutex, NULL) != 0) return;
    if (pthread_cond_init(&fcWorkerPool.workAvailable, NULL) != 0) return;
    if (pthread_cond_init(&fcWorkerPool.workComplete, NULL) != 0) return;
    fcWorkerPool.initialized = true;
    for (int i = 0; i < FC_PARALLEL_MAX_WORKERS; i++) {
        fcWorkerPoolSlots[i].pool = &fcWorkerPool;
        fcWorkerPoolSlots[i].workerSlot = i;
        if (pthread_create(&fcWorkerPool.threads[i], NULL,
                           fc_worker_pool_main, &fcWorkerPoolSlots[i]) != 0)
            break;
        fcWorkerPool.threadCount++;
    }
    if (fcWorkerPool.threadCount == 0)
        fcWorkerPool.initialized = false;
}

static bool fc_worker_pool_ensure(void)
{
    (void)pthread_once(&fcWorkerPoolOnce, fc_worker_pool_bootstrap);
    return fcWorkerPool.initialized && fcWorkerPool.threadCount > 0;
}

/* Returns the number of pool slots that completed this batch.  A zero return
 * is deliberately a safe signal to use the legacy direct/threaded fallback. */
static int fc_worker_pool_dispatch(FCWorkerPoolTask task,
                                   void *context,
                                   int requestedWorkers)
{
    if (task == NULL || requestedWorkers <= 0 || fcWorkerPoolTaskActive ||
        !fc_worker_pool_ensure()) return 0;
    pthread_mutex_lock(&fcWorkerPool.mutex);
    if (fcWorkerPool.taskActive || fcWorkerPool.threadCount <= 0) {
        pthread_mutex_unlock(&fcWorkerPool.mutex);
        return 0;
    }
    int workers = requestedWorkers < fcWorkerPool.threadCount
        ? requestedWorkers : fcWorkerPool.threadCount;
    if (workers <= 0) {
        pthread_mutex_unlock(&fcWorkerPool.mutex);
        return 0;
    }
    fcWorkerPool.generation++;
    if (fcWorkerPool.generation == 0) fcWorkerPool.generation++;
    fcWorkerPool.task = task;
    fcWorkerPool.taskContext = context;
    fcWorkerPool.requestedWorkers = workers;
    fcWorkerPool.completedWorkers = 0;
    fcWorkerPool.taskActive = true;
    (void)pthread_cond_broadcast(&fcWorkerPool.workAvailable);
    while (fcWorkerPool.completedWorkers < workers)
        (void)pthread_cond_wait(&fcWorkerPool.workComplete,
                                &fcWorkerPool.mutex);
    fcWorkerPool.taskActive = false;
    fcWorkerPool.task = NULL;
    fcWorkerPool.taskContext = NULL;
    fcWorkerPool.requestedWorkers = 0;
    fcWorkerPool.completedWorkers = 0;
    (void)pthread_cond_broadcast(&fcWorkerPool.workAvailable);
    pthread_mutex_unlock(&fcWorkerPool.mutex);
    return workers;
}

typedef struct {
    int attacker;
    bool forbiddenBlack;
    int searchClass;
    int maxDepth;
    uint64_t nodeBudget;
    uint64_t queryNodeBudget;
    uint32_t timeBudgetMs;
    uint32_t queryTimeBudgetMs;
    double startedMilliseconds;
    uint64_t nodes;
    uint64_t hits;
    bool aborted;
    bool noProgress;
    bool certificateOverflow;
    FCProofNode certificate[FC_MAX_PROOF_NODES];
    int certificateCount;
    FCProofTTEntry *table;
    size_t tableCapacity;
    FCIncrementalPosition *position;
    FCProofSession *session;
    uint32_t generation;
    int dfpnRoot;
    FCPostponedEdge postponedEdges[FC_DFPN_POSTPONED_CAPACITY];
    int postponedCount;
    uint64_t postponedInsertionIndex;
    bool branchFirstEnabled;
    int branchFirstMinRemainingDepth;
    int branchFirstMinBranchCount;
    int branchFirstMaxBranches;
    int branchFirstPreviewDepth;
    bool branchWaveDispatched;
} FCProofContext;

typedef struct {
    bool attempted;
    bool allProven;
    bool sawUnknown;
    int maximumChildDistance;
    uint64_t edgeProof;
    uint64_t edgeDisproof;
} FCBranchWaveSummary;

static void fc_branch_first_order_threats(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    FCThreat *threats,
    int threatCount,
    int *order);

static bool fc_branch_first_try_wave(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    const FCPoint *refutations,
    int refutationCount,
    int childDepth,
    int attackNode,
    int attackSeverity,
    FCBranchWaveSummary *summary);

#define FC_PROOF_INFINITY (UINT64_MAX / 4ULL)
#define FC_PROOF_ALGORITHM_VERSION UINT64_C(0x0005000300010001)

static uint64_t fc_proof_key_version(int completedDepth)
{
    return FC_PROOF_ALGORITHM_VERSION ^
           ((uint64_t)(unsigned int)completedDepth << 32);
}

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
#include "FiveChessEliteCorpus.inc"

static _Thread_local FCProofDiagnostics fcProofDiagnostics;

static uint32_t fc_parallel_token_block_size(void)
{
    return fcActiveParallelNodeBlockSize > 0
        ? fcActiveParallelNodeBlockSize
        : FC_PARALLEL_TOKEN_BLOCK_DEFAULT;
}

static bool fc_claim_token_block(_Atomic uint64_t *counter,
                                 uint64_t limit,
                                 uint64_t *remaining)
{
    if (counter == NULL || remaining == NULL || limit == 0) return false;
    uint64_t prior = atomic_load_explicit(counter, memory_order_relaxed);
    uint64_t block = fc_parallel_token_block_size();
    for (;;) {
        if (prior >= limit) return false;
        uint64_t available = limit - prior;
        uint64_t claim = available < (uint64_t)block
            ? available : (uint64_t)block;
        uint64_t next = prior + claim;
        if (atomic_compare_exchange_weak_explicit(
                counter, &prior, next, memory_order_acq_rel,
                memory_order_relaxed)) {
            *remaining = claim;
            fcProofDiagnostics.parallelTokenBlockClaims++;
            fcProofDiagnostics.parallelTokenBlockTokens += claim;
            return true;
        }
    }
}

static bool fc_parallel_node_token_take(void)
{
    if (fcActiveParallelNodeCounter == NULL ||
        fcActiveParallelNodeBudget == 0) return true;
    if (fcActiveParallelTokenBlocksEnabled) {
        if (fcParallelNodeTokensRemaining == 0 &&
            !fc_claim_token_block(fcActiveParallelNodeCounter,
                                  fcActiveParallelNodeBudget,
                                  &fcParallelNodeTokensRemaining))
            return false;
        fcParallelNodeTokensRemaining--;
        fcProofDiagnostics.parallelBudgetTokens++;
        return true;
    }
    uint64_t prior = atomic_fetch_add_explicit(
        fcActiveParallelNodeCounter, 1, memory_order_relaxed);
    if (prior >= fcActiveParallelNodeBudget) {
        atomic_fetch_sub_explicit(
            fcActiveParallelNodeCounter, 1, memory_order_relaxed);
        return false;
    }
    fcProofDiagnostics.parallelBudgetTokens++;
    return true;
}

static void fc_parallel_node_token_return_one(void)
{
    if (fcActiveParallelTokenBlocksEnabled &&
        fcParallelNodeTokensRemaining < UINT64_MAX)
        fcParallelNodeTokensRemaining++;
}

static void fc_parallel_node_token_flush(void)
{
    if (fcActiveParallelTokenBlocksEnabled &&
        fcActiveParallelNodeCounter != NULL &&
        fcParallelNodeTokensRemaining > 0) {
        atomic_fetch_sub_explicit(fcActiveParallelNodeCounter,
                                  fcParallelNodeTokensRemaining,
                                  memory_order_acq_rel);
        fcProofDiagnostics.parallelTokenBlockReturns +=
            fcParallelNodeTokensRemaining;
        fcParallelNodeTokensRemaining = 0;
    }
    if (fcActiveParallelTokenBlocksEnabled &&
        fcActiveDecisionLedger != NULL &&
        fcDecisionLedgerNodeTokensRemaining > 0) {
        atomic_fetch_sub_explicit(
            &fcActiveDecisionLedger->nodesConsumed,
            fcDecisionLedgerNodeTokensRemaining, memory_order_acq_rel);
        fcProofDiagnostics.parallelTokenBlockReturns +=
            fcDecisionLedgerNodeTokensRemaining;
        fcDecisionLedgerNodeTokensRemaining = 0;
    }
}

static void fc_proof_diagnostics_merge(FCProofDiagnostics *destination,
                                       const FCProofDiagnostics *source)
{
    if (destination == NULL || source == NULL) return;
    _Static_assert(sizeof(FCProofDiagnostics) % sizeof(uint64_t) == 0,
                   "proof diagnostics must contain uint64 counters only");
    uint64_t priorMaximumDepth = destination->dependencyMaximumDepth;
    size_t count = sizeof(*destination) / sizeof(uint64_t);
    uint64_t out[sizeof(*destination) / sizeof(uint64_t)];
    uint64_t in[sizeof(*source) / sizeof(uint64_t)];
    memcpy(out, destination, sizeof(out));
    memcpy(in, source, sizeof(in));
    for (size_t i = 0; i < count; i++) out[i] += in[i];
    memcpy(destination, out, sizeof(out));
    destination->dependencyMaximumDepth =
        priorMaximumDepth > source->dependencyMaximumDepth
            ? priorMaximumDepth : source->dependencyMaximumDepth;
}

void fc_proof_diagnostics_reset(void)
{
    memset(&fcProofDiagnostics, 0, sizeof(fcProofDiagnostics));
}

FCProofDiagnostics fc_proof_diagnostics_get(void)
{
    return fcProofDiagnostics;
}

static double fc_now_milliseconds(void);

static bool fc_ledger_add_overflow_u64(uint64_t left,
                                       uint64_t right,
                                       uint64_t *out)
{
    if (UINT64_MAX - left < right) return true;
    if (out != NULL) *out = left + right;
    return false;
}

bool fc_decision_ledger_begin(FCDecisionLedger *ledger,
                              const FCAIProfile *profile)
{
    if (ledger == NULL || profile == NULL) return false;
    memset(ledger, 0, sizeof(*ledger));
    atomic_init(&ledger->nodesConsumed, 0);
    atomic_init(&ledger->memoryConsumed, 0);
    atomic_init(&ledger->memoryPeakReserved, 0);
    atomic_init(&ledger->memoryReleased, 0);
    atomic_init(&ledger->queriesConsumed, 0);
    atomic_init(&ledger->nodesReserved, 0);
    atomic_init(&ledger->memoryReserved, 0);
    atomic_init(&ledger->queriesReserved, 0);
    atomic_init(&ledger->guardNodesConsumed, 0);
    atomic_init(&ledger->guardReservationActive, false);
    atomic_init(&ledger->guardPhaseActive, false);
    atomic_init(&ledger->exhausted, false);
    ledger->startedMilliseconds = fc_now_milliseconds();
    uint32_t internal = profile->decisionTimeBudgetMs;
    if (internal == 0) internal = profile->timeBudgetMs;
    if (internal == 0) internal = 4500;
    if (internal > 4500) internal = 4500;
    uint32_t hard = profile->decisionHardLimitMs;
    if (hard == 0) hard = 5000;
    if (hard < internal) hard = internal;
    ledger->internalDeadlineMilliseconds =
        ledger->startedMilliseconds + (double)internal;
    ledger->ordinaryDeadlineMilliseconds =
        ledger->internalDeadlineMilliseconds;
    if (profile->opponentGuardEnabled &&
        profile->opponentGuardReservedTimeMs > 0 &&
        profile->opponentGuardReservedTimeMs < internal) {
        ledger->ordinaryDeadlineMilliseconds -=
            (double)profile->opponentGuardReservedTimeMs;
    }
    ledger->hardDeadlineMilliseconds =
        ledger->startedMilliseconds + (double)hard;
    ledger->nodeBudget = profile->decisionNodeBudget > 0
        ? profile->decisionNodeBudget
        : profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget
        : profile->nodeBudget;
    if (ledger->nodeBudget == 0) ledger->nodeBudget = UINT64_MAX;
    ledger->memoryBudgetBytes = profile->decisionMemoryBudgetBytes;
    ledger->queryBudget = profile->decisionCorpusQueryBudget > 0
        ? profile->decisionCorpusQueryBudget : UINT32_MAX;
    ledger->version = profile->decisionLedgerVersion > 0
        ? profile->decisionLedgerVersion : 1;
    fcProofDiagnostics.decisionCount++;
    return true;
}

bool fc_decision_ledger_deadline_reached(FCDecisionLedger *ledger)
{
    if (ledger == NULL) return false;
    double now = fc_now_milliseconds();
    bool guardPhase = atomic_load_explicit(
        &ledger->guardPhaseActive, memory_order_acquire);
    double deadline = guardPhase ? ledger->internalDeadlineMilliseconds
                                 : ledger->ordinaryDeadlineMilliseconds;
    if (now < deadline) return false;
    bool guardReserved = atomic_load_explicit(
        &ledger->guardReservationActive, memory_order_acquire);
    if (guardPhase || !guardReserved)
        atomic_store_explicit(&ledger->exhausted, true,
                              memory_order_release);
    fcProofDiagnostics.decisionLedgerExhaustions++;
    return true;
}

bool fc_decision_ledger_reserve_guard(FCDecisionLedger *ledger,
                                      uint64_t requestedNodes)
{
    if (ledger == NULL || requestedNodes == 0) return false;
    fcProofDiagnostics.decisionStageRequests++;
    if (requestedNodes > ledger->nodeBudget ||
        atomic_load_explicit(&ledger->guardReservationActive,
                             memory_order_acquire)) {
        fcProofDiagnostics.decisionStageAbandons++;
        return false;
    }
    ledger->guardNodeReservation = requestedNodes;
    atomic_store_explicit(&ledger->guardNodesConsumed, 0,
                          memory_order_release);
    atomic_store_explicit(&ledger->guardReservationActive, true,
                          memory_order_release);
    fcProofDiagnostics.decisionStageReservations += requestedNodes;
    return true;
}

bool fc_decision_ledger_enter_guard(FCDecisionLedger *ledger)
{
    if (ledger == NULL ||
        !atomic_load_explicit(&ledger->guardReservationActive,
                              memory_order_acquire) ||
        fc_now_milliseconds() >= ledger->internalDeadlineMilliseconds)
        return false;
    atomic_store_explicit(&ledger->guardPhaseActive, true,
                          memory_order_release);
    return true;
}

void fc_decision_ledger_leave_guard(FCDecisionLedger *ledger)
{
    if (ledger == NULL) return;
    atomic_store_explicit(&ledger->guardPhaseActive, false,
                          memory_order_release);
}

void fc_decision_ledger_release_guard(FCDecisionLedger *ledger)
{
    if (ledger == NULL) return;
    fc_decision_ledger_leave_guard(ledger);
    atomic_store_explicit(&ledger->guardReservationActive, false,
                          memory_order_release);
}

bool fc_decision_ledger_reserve_nodes(FCDecisionLedger *ledger,
                                      uint64_t requested)
{
    if (ledger == NULL || requested == 0) return true;
    fcProofDiagnostics.decisionStageRequests++;
    bool guardPhase = atomic_load_explicit(
        &ledger->guardPhaseActive, memory_order_acquire);
    bool guardReserved = atomic_load_explicit(
        &ledger->guardReservationActive, memory_order_acquire);
    if (guardPhase) {
        uint64_t consumed = atomic_load_explicit(
            &ledger->guardNodesConsumed, memory_order_relaxed);
        if (!guardReserved || consumed > ledger->guardNodeReservation ||
            requested > ledger->guardNodeReservation - consumed) {
            fcProofDiagnostics.decisionStageAbandons++;
            return false;
        }
        fcProofDiagnostics.decisionStageReservations += requested;
        return true;
    }
    uint64_t prior = atomic_fetch_add_explicit(
        &ledger->nodesReserved, requested, memory_order_relaxed);
    uint64_t next = 0;
    bool overflow = fc_ledger_add_overflow_u64(prior, requested, &next);
    uint64_t limit = guardReserved
        ? ledger->nodeBudget - ledger->guardNodeReservation
        : ledger->nodeBudget;
    if (overflow || next > limit) {
        atomic_fetch_sub_explicit(
            &ledger->nodesReserved, requested, memory_order_relaxed);
        if (!guardReserved)
            atomic_store_explicit(&ledger->exhausted, true,
                                  memory_order_release);
        fcProofDiagnostics.decisionLedgerExhaustions++;
        fcProofDiagnostics.decisionStageAbandons++;
        return false;
    }
    fcProofDiagnostics.decisionStageReservations += requested;
    return true;
}

bool fc_decision_ledger_reserve_memory(FCDecisionLedger *ledger,
                                       uint64_t requested)
{
    if (ledger == NULL || requested == 0) return true;
    fcProofDiagnostics.decisionStageRequests++;
    uint64_t prior = atomic_load_explicit(&ledger->memoryReserved,
                                          memory_order_relaxed);
    uint64_t next = 0;
    for (;;) {
        if (fc_ledger_add_overflow_u64(prior, requested, &next) ||
            (ledger->memoryBudgetBytes > 0 &&
             (prior > ledger->memoryBudgetBytes ||
              requested > ledger->memoryBudgetBytes - prior))) {
            atomic_store_explicit(&ledger->exhausted, true,
                                  memory_order_release);
            fcProofDiagnostics.decisionLedgerExhaustions++;
            fcProofDiagnostics.decisionStageAbandons++;
            return false;
        }
        if (atomic_compare_exchange_weak_explicit(
                &ledger->memoryReserved, &prior, next,
                memory_order_acq_rel, memory_order_relaxed)) break;
    }
    uint64_t peak = atomic_load_explicit(&ledger->memoryPeakReserved,
                                        memory_order_relaxed);
    while (next > peak &&
           !atomic_compare_exchange_weak_explicit(
               &ledger->memoryPeakReserved, &peak, next,
               memory_order_relaxed, memory_order_relaxed)) {
        /* The failed compare-exchange refreshes peak. */
    }
    fcProofDiagnostics.decisionStageReservations += requested;
    return true;
}

void fc_decision_ledger_release_memory(FCDecisionLedger *ledger,
                                       uint64_t released)
{
    if (ledger == NULL || released == 0) return;
    uint64_t prior = atomic_load_explicit(&ledger->memoryReserved,
                                          memory_order_relaxed);
    for (;;) {
        if (prior < released) {
            /* A release token can never consume another session's bytes. */
            fcProofDiagnostics.decisionStageAbandons++;
            return;
        }
        uint64_t next = prior - released;
        if (atomic_compare_exchange_weak_explicit(
                &ledger->memoryReserved, &prior, next,
                memory_order_acq_rel, memory_order_relaxed)) {
            atomic_fetch_add_explicit(&ledger->memoryReleased, released,
                                      memory_order_relaxed);
            fcProofDiagnostics.decisionMemoryReleased += released;
            return;
        }
    }
}

bool fc_decision_ledger_reserve_queries(FCDecisionLedger *ledger,
                                        uint32_t requested)
{
    if (ledger == NULL || requested == 0) return true;
    fcProofDiagnostics.decisionStageRequests++;
    uint32_t prior = atomic_fetch_add_explicit(
        &ledger->queriesReserved, requested, memory_order_relaxed);
    if (UINT32_MAX - prior < requested ||
        prior + requested > ledger->queryBudget) {
        atomic_fetch_sub_explicit(
            &ledger->queriesReserved, requested, memory_order_relaxed);
        atomic_store_explicit(&ledger->exhausted, true, memory_order_release);
        fcProofDiagnostics.decisionLedgerExhaustions++;
        fcProofDiagnostics.decisionStageAbandons++;
        return false;
    }
    fcProofDiagnostics.decisionStageReservations += requested;
    return true;
}

bool fc_decision_ledger_consume_node(FCDecisionLedger *ledger)
{
    if (ledger == NULL) return true;
    if (fc_decision_ledger_deadline_reached(ledger)) return false;
    bool guardPhase = atomic_load_explicit(
        &ledger->guardPhaseActive, memory_order_acquire);
    bool guardReserved = atomic_load_explicit(
        &ledger->guardReservationActive, memory_order_acquire);
    if (guardPhase) {
        uint64_t guardPrior = atomic_fetch_add_explicit(
            &ledger->guardNodesConsumed, 1, memory_order_relaxed);
        if (guardPrior >= ledger->guardNodeReservation) {
            atomic_fetch_sub_explicit(&ledger->guardNodesConsumed, 1,
                                      memory_order_relaxed);
            atomic_store_explicit(&ledger->exhausted, true,
                                  memory_order_release);
            fcProofDiagnostics.decisionLedgerExhaustions++;
            return false;
        }
    }
    if (!guardPhase && fcActiveParallelTokenBlocksEnabled &&
        fcActiveDecisionLedger == ledger) {
        uint64_t ordinaryBudget = guardReserved
            ? ledger->nodeBudget - ledger->guardNodeReservation
            : ledger->nodeBudget;
        if (fcDecisionLedgerNodeTokensRemaining == 0 &&
            !fc_claim_token_block(&ledger->nodesConsumed,
                                  ordinaryBudget,
                                  &fcDecisionLedgerNodeTokensRemaining)) {
            if (!guardReserved)
                atomic_store_explicit(&ledger->exhausted, true,
                                      memory_order_release);
            fcProofDiagnostics.decisionLedgerExhaustions++;
            return false;
        }
        fcDecisionLedgerNodeTokensRemaining--;
        return true;
    }
    uint64_t prior = atomic_fetch_add_explicit(
        &ledger->nodesConsumed, 1, memory_order_relaxed);
    uint64_t limit = !guardPhase && guardReserved
        ? ledger->nodeBudget - ledger->guardNodeReservation
        : ledger->nodeBudget;
    if (prior >= limit) {
        atomic_fetch_sub_explicit(
            &ledger->nodesConsumed, 1, memory_order_relaxed);
        if (guardPhase)
            atomic_fetch_sub_explicit(&ledger->guardNodesConsumed, 1,
                                      memory_order_relaxed);
        if (guardPhase || !guardReserved)
            atomic_store_explicit(&ledger->exhausted, true,
                                  memory_order_release);
        fcProofDiagnostics.decisionLedgerExhaustions++;
        return false;
    }
    return true;
}

bool fc_decision_ledger_consume_query(FCDecisionLedger *ledger)
{
    if (ledger == NULL) return true;
    uint32_t prior = atomic_fetch_add_explicit(
        &ledger->queriesConsumed, 1, memory_order_relaxed);
    if (prior >= ledger->queryBudget) {
        atomic_fetch_sub_explicit(
            &ledger->queriesConsumed, 1, memory_order_relaxed);
        atomic_store_explicit(&ledger->exhausted, true, memory_order_release);
        fcProofDiagnostics.decisionLedgerExhaustions++;
        return false;
    }
    return true;
}

bool fc_decision_ledger_consume_memory(FCDecisionLedger *ledger,
                                       uint64_t bytes)
{
    if (ledger == NULL || bytes == 0) return true;
    uint64_t prior = atomic_fetch_add_explicit(
        &ledger->memoryConsumed, bytes, memory_order_relaxed);
    if (ledger->memoryBudgetBytes > 0 &&
        (prior > ledger->memoryBudgetBytes ||
         bytes > ledger->memoryBudgetBytes - prior)) {
        atomic_fetch_sub_explicit(
            &ledger->memoryConsumed, bytes, memory_order_relaxed);
        atomic_store_explicit(&ledger->exhausted, true, memory_order_release);
        fcProofDiagnostics.decisionLedgerExhaustions++;
        return false;
    }
    return true;
}

static double fc_now_milliseconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool fc_profile_proof_path_enabled(const FCAIProfile *profile)
{
    return profile != NULL &&
           (profile->proofEngineCandidate ||
            profile->proofCandidateStagesEnabled);
}

static int fc_effective_worker_count(const FCAIProfile *profile)
{
    if (profile == NULL) return 1;
    int count = profile->proofWorkerCount;
    if (profile->workerCountOverride > 0) {
        count = fc_research_worker_count_is_valid(
                    profile->workerCountOverride)
            ? profile->workerCountOverride : 1;
    }
    if (count < 1) count = 1;
    if (count > 8) count = 8;
    return count;
}

bool fc_research_worker_count_is_valid(int workerCount)
{
    return workerCount == 1 || workerCount == 4 || workerCount == 8;
}

static bool fc_decision_deadline_reached(void)
{
    if (fcActiveDecisionLedger != NULL &&
        fc_now_milliseconds() >=
            fcActiveDecisionLedger->internalDeadlineMilliseconds) {
        fcActiveDecisionLedger->exhausted = true;
        return true;
    }
    return fcActiveDecisionDeadlineMilliseconds > 0.0 &&
           fc_now_milliseconds() >= fcActiveDecisionDeadlineMilliseconds;
}

static bool fc_global_proof_deadline_reached(void)
{
    if (fc_decision_deadline_reached()) {
        if (fcActiveProofSession != NULL)
            fcActiveProofSession->exhausted = true;
        return true;
    }
    if (fcActiveProofSession == NULL ||
        fcActiveProofSession->timeBudgetMs == 0) return false;
    if (fc_now_milliseconds() - fcActiveProofSession->startedMilliseconds <
        fcActiveProofSession->timeBudgetMs) return false;
    fcActiveProofSession->exhausted = true;
    return true;
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

FCAIProfile fc_profile_frozen_four_star_control(void)
{
    FCAIProfile profile = fc_profile_proof_guided(false);
    profile.name = "four-star-frozen-control";
    profile.version = "4.0.0-frozen-vcf-vct-control";
    return profile;
}

FCAIProfile fc_profile_five_star(void)
{
    FCAIProfile profile = fc_profile_proof_guided(false);
    profile.name = "five-star-curated-opening-advisor";
    profile.version = "5.1.0-elite-rule-partitioned-local-v2";
    profile.eliteCorpusEnabled = true;
    profile.corpusScoreMargin = -80;
    profile.corpusMinGames = 2;
    profile.corpusMinEvents = 1;
    profile.proofMaxDepth = 12;
    profile.proofNodeBudget = 36000;
    profile.proofTimeBudgetMs = 320;
    profile.proofTranspositionCapacity = 65536;
    return profile;
}

FCAIProfile fc_profile_five_star_loss_aware_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star();
    profile.name = "five-star-loss-aware-candidate";
    profile.version = "5.2.1-certificate-dependency-widening";
    profile.proofCandidateStagesEnabled = true;
    profile.lossAwareEnabled = true;
    profile.quietThreatEnabled = true;
    /* Zero means progressively inspect every legal escape until the frozen
     * emergency wall budget expires.  Certificate dependencies remain first,
     * so this is wider without turning the search into an untargeted scan. */
    profile.proofEscapeCandidateLimit = 0;
    profile.proofQuietRootLimit = 3;
    profile.proofEmergencyTimeBudgetMs = 1100;
    return profile;
}

FCAIProfile fc_profile_five_star_proof_engine_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star();
    profile.name = "five-star-incremental-dfpn-candidate";
    profile.version = "5.4.1-transactional-deadline-root-parallel-5s";
    profile.proofEngineCandidate = true;
    profile.proofCandidateStagesEnabled = true;
    profile.parallelProofEnabled = true;
    profile.lossAwareEnabled = true;
    profile.quietThreatEnabled = true;
    profile.proofEscapeCandidateLimit = 8;
    profile.proofQuietRootLimit = 3;
    /* This is a single-decision safety ceiling, not a fresh allowance for
     * each candidate.  The session implementation enforces the shared cap. */
    profile.proofParallelNodeBudget = 288000;
    profile.proofWorkerCount = 8;
    profile.proofTimeBudgetMs = 1200;
    profile.proofEmergencyTimeBudgetMs = 4200;
    /* Stop search with a small result-finalization reserve so the complete
     * player-visible decision remains below the five-second hard limit. */
    profile.decisionTimeBudgetMs = 4500;
    return profile;
}

FCAIProfile fc_profile_five_star_opponent_guard_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
    profile.name = "five-star-opponent-forcing-guard-candidate";
    profile.version = "5.8.0-vcf-first-opponent-guard-4w";
    profile.opponentGuardEnabled = true;
    profile.opponentGuardVCFMaxDepth = 9;
    profile.opponentGuardVCFNodeBudget = 48000;
    profile.opponentGuardVCFTimeBudgetMs = 500;
    profile.opponentGuardVCTMaxDepth = 10;
    profile.opponentGuardVCTNodeBudget = 48000;
    profile.opponentGuardVCTTimeBudgetMs = 700;
    profile.opponentGuardReservedNodes = 96000;
    profile.opponentGuardReservedTimeMs = 1400;
    profile.opponentGuardStructuralVCTEnabled = true;
    profile.opponentGuardMaxAlternatives = 8;
    profile.proofWorkerCount = 4;
    profile.proofParallelNodeBudget = 192000;
    profile.decisionHardLimitMs = 5000;
    profile.decisionLedgerVersion = 2;
    profile.decisionNodeBudget = 288000;
    return profile;
}

FCAIProfile fc_profile_five_star_early_micro_vcf_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_opponent_guard_candidate();
    profile.name = "five-star-early-micro-vcf-sentinel-candidate";
    profile.version = "5.8.1-early-micro-vcf-adaptive-16k-80ms-2a";
    profile.earlyVCFSentinelEnabled = true;
    profile.earlyVCFSentinelPolicy = FC_EARLY_VCF_POLICY_ADAPTIVE;
    profile.earlyVCFBaseDepth = 5;
    profile.earlyVCFMaxDepth = 7;
    profile.earlyVCFNodeBudget = 16000;
    profile.earlyVCFTimeBudgetMs = 80;
    profile.earlyVCFMaxAlternatives = 2;
    return profile;
}

FCAIProfile fc_profile_five_star_v541_thread_scheduler_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
    profile.name = "five-star-v541-persistent-thread-scheduler";
    profile.version = "5.4.2-v541-persistent-pool-token-blocks-8w-5s";
    profile.persistentWorkerPoolEnabled = true;
    profile.parallelTokenBlockEnabled = true;
    /* The v5.7 fixed-position scan found 64 to be the best measured
     * amortization point for this scheduler on the current machine. */
    profile.parallelTokenBlockSize = 64;
    /* Persistent workers still need to close private DFPN arenas after the
     * internal deadline.  Leave a bounded finalization slice so a heavy root
     * batch cannot cross the player-visible five-second gate. */
    profile.proofEmergencyTimeBudgetMs = 3800;
    /* The persistent-pool coordinator has a larger join/merge tail than the
     * historical per-decision path; reserve 300 ms for that finalization. */
    profile.decisionTimeBudgetMs = 4200;
    /* Keep the original 5.4.1 resource semantics.  The scheduler study is
     * intended to isolate pool/session/token overhead; importing v5.7's
     * ledger and recovery layers would measure extra policy work instead. */
    return profile;
}

FCAIProfile fc_profile_five_star_color_hybrid_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_proof_engine_candidate();
    profile.name = "five-star-color-specialized-hybrid-candidate";
    profile.version = "5.5.1-white-proof-engine-black-v51-stochastic-5s";
    return profile;
}

FCAIProfile fc_profile_five_star_v521_serial_hybrid_control(void)
{
    FCAIProfile profile = fc_profile_five_star_loss_aware_candidate();
    profile.name = "five-star-v521-serial-black-hybrid-control";
    profile.version = "5.6.2-white-v541-black-v521-serial1-active-proof-5s";
    profile.parallelProofEnabled = false;
    profile.proofWorkerCount = 1;
    profile.proofParallelNodeBudget = profile.proofNodeBudget;
    profile.decisionTimeBudgetMs = 4500;
    return profile;
}

FCAIProfile fc_profile_five_star_v521_parallel_hybrid_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_loss_aware_candidate();
    profile.name = "five-star-v521-parallel-black-hybrid-candidate";
    profile.version = "5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s";
    profile.parallelProofEnabled = true;
    profile.proofWorkerCount = 8;
    profile.proofParallelNodeBudget = profile.proofNodeBudget * UINT64_C(8);
    profile.decisionTimeBudgetMs = 4500;
    profile.decisionHardLimitMs = 5000;
    profile.decisionLedgerVersion = 1;
    profile.decisionNodeBudget = profile.proofParallelNodeBudget;
    profile.decisionCorpusQueryBudget = FC_MAX_CORPUS_CANDIDATES * 2;
    profile.incrementalLegalityEnabled = true;
    profile.validateLegalityCache = true;
    profile.recoverySearchEnabled = true;
    return profile;
}

FCAIProfile fc_profile_five_star_v57_hybrid_candidate(void)
{
    /* 5.7 keeps the color-specialized search contract that was measured in
     * 5.6, but gives the candidate an independent identity.  The black
     * component is still the loss-aware v5.2.1 search; the improvement in
     * this change is its root/escape dispatcher and legality cache rather
     * than a silent rewrite of the historical profile. */
    FCAIProfile profile = fc_profile_five_star_v521_parallel_hybrid_candidate();
    profile.name = "five-star-v57-white-v541-black-v521";
    profile.version = "5.7.0-white-v541-black-v521-independent-root-parallel8-5s";
    profile.parallelProofEnabled = true;
    profile.proofWorkerCount = 8;
    profile.proofParallelNodeBudget = profile.proofNodeBudget * UINT64_C(8);
    profile.decisionTimeBudgetMs = 4500;
    profile.decisionHardLimitMs = 5000;
    profile.decisionLedgerVersion = 1;
    profile.decisionNodeBudget = profile.proofParallelNodeBudget;
    profile.decisionMemoryBudgetBytes = 256U * 1024U * 1024U;
    profile.decisionCorpusQueryBudget = FC_MAX_CORPUS_CANDIDATES * 2;
    profile.incrementalLegalityEnabled = true;
    profile.validateLegalityCache = true;
    profile.recoverySearchEnabled = true;
    profile.forkFirstRecoveryEnabled = false;
    return profile;
}

FCAIProfile fc_profile_five_star_v57_fork_recovery_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_v57_hybrid_candidate();
    profile.name = "five-star-v57-fork-recovery-opt-in";
    profile.version = "5.7.0-fork-recovery-ledger-opt-in";
    profile.forkFirstRecoveryEnabled = true;
    return profile;
}

FCAIProfile fc_profile_five_star_v57_thread_scheduler_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_v57_hybrid_candidate();
    profile.name = "five-star-v57-persistent-thread-scheduler";
    profile.version = "5.7.1-persistent-pool-token-blocks-8w";
    profile.persistentWorkerPoolEnabled = true;
    profile.parallelTokenBlockEnabled = true;
    profile.parallelTokenBlockSize = 16;
    return profile;
}

FCAIProfile fc_profile_five_star_v57_branch_first_candidate(void)
{
    FCAIProfile profile = fc_profile_five_star_v57_hybrid_candidate();
    profile.name = "five-star-v57-branch-first-recursive";
    profile.version = "5.7.2-branch-first-preview-recursive-pool-14d-5s";
    /* Root obligations are intentionally serial.  The worker pool is used
     * only by the long-recursion branch wave below the root. */
    profile.parallelProofEnabled = false;
    profile.persistentWorkerPoolEnabled = true;
    profile.parallelTokenBlockEnabled = true;
    profile.parallelTokenBlockSize = 16;
    profile.proofWorkerCount = 8;
    profile.proofMaxDepth = 14;
    profile.proofNodeBudget = 72000;
    profile.proofParallelNodeBudget = profile.proofNodeBudget * UINT64_C(8);
    profile.proofTimeBudgetMs = 1300;
    profile.proofEmergencyTimeBudgetMs = 4200;
    profile.decisionTimeBudgetMs = 4500;
    profile.decisionHardLimitMs = 5000;
    profile.decisionLedgerVersion = 1;
    profile.decisionNodeBudget = profile.proofParallelNodeBudget;
    profile.decisionMemoryBudgetBytes = 256U * 1024U * 1024U;
    profile.branchFirstSearchEnabled = true;
    profile.branchFirstMinRemainingDepth = 10;
    profile.branchFirstMinBranchCount = 2;
    profile.branchFirstMaxBranches = 32;
    profile.branchFirstPreviewDepth = 2;
    /* Spend the branch pool's extra headroom on forcing continuations.  The
     * base proof horizon remains 14; only an eligible recursive wave may
     * reach the tactical cap, and all work still uses the shared ledger. */
    profile.branchFirstAdvancedFourDepthBonus = 2;
    profile.branchFirstAdvancedThreeDepthBonus = 1;
    profile.branchFirstTacticalDepthCap = 16;
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
    fcProofDiagnostics.legalityCalls++;
    if (!fc_inside(x, y) || board[x][y] != 0 || (side != 1 && side != -1)) {
        return false;
    }
    if (!forbiddenBlack || side != 1) {
        return true;
    }

    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    fcProofDiagnostics.legalityBoardCopies++;
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
            fcProofDiagnostics.boardHashCellVisits++;
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

static uint64_t fc_stone_hash_component(int x, int y, int side)
{
    uint64_t value = (uint64_t)(x * FC_BOARD_SIZE + y + 1)
                   | ((uint64_t)(side == 1 ? 1 : 2) << 16);
    return fc_mix64(value ^ UINT64_C(0x9e3779b97f4a7c15));
}

static uint64_t fc_stone_lock_component(int x, int y, int side)
{
    uint64_t value = (uint64_t)(x * FC_BOARD_SIZE + y + 1)
                   | ((uint64_t)(side == 1 ? 1 : 2) << 20);
    return fc_mix64(value ^ UINT64_C(0xd1b54a32d192ed03));
}

static void fc_bitset_set(uint64_t bits[FC_POSITION_BITSET_WORDS], int index)
{
    bits[index >> 6] |= UINT64_C(1) << (index & 63);
}

static void fc_bitset_clear(uint64_t bits[FC_POSITION_BITSET_WORDS], int index)
{
    bits[index >> 6] &= ~(UINT64_C(1) << (index & 63));
}

static bool fc_bitset_has(
    const uint64_t bits[FC_POSITION_BITSET_WORDS], int index)
{
    return (bits[index >> 6] & (UINT64_C(1) << (index & 63))) != 0;
}

/* Forbidden black legality is local to the four lines crossing the move.
 * This mirrors fc_is_legal_move exactly, but mutates only the candidate and
 * temporary continuation cells instead of copying the complete board. */
static bool fc_incremental_forbidden_black_is_legal(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int x,
    int y)
{
    if (!fc_inside(x, y) || board[x][y] != 0) return false;
    board[x][y] = 1;
    bool overline = false;
    for (int direction = 0; direction < 4; direction++) {
        if (fc_line_length((const int (*)[FC_BOARD_SIZE])board, x, y,
                           fcDirections[direction][0],
                           fcDirections[direction][1], 1) > 5) {
            overline = true;
            break;
        }
    }
    if (overline) {
        board[x][y] = 0;
        return false;
    }
    if (fc_has_five((const int (*)[FC_BOARD_SIZE])board, x, y, 1)) {
        board[x][y] = 0;
        return true;
    }
    int winningDirections = 0;
    for (int direction = 0; direction < 4; direction++) {
        int dx = fcDirections[direction][0];
        int dy = fcDirections[direction][1];
        int continuationCount = 0;
        for (int offset = -4; offset <= 4; offset++) {
            int px = x + offset * dx;
            int py = y + offset * dy;
            if (!fc_inside(px, py) || board[px][py] != 0) continue;
            board[px][py] = 1;
            if (fc_line_length((const int (*)[FC_BOARD_SIZE])board,
                               px, py, dx, dy, 1) >= 5)
                continuationCount++;
            board[px][py] = 0;
        }
        if (continuationCount > 0) winningDirections++;
    }
    board[x][y] = 0;
    return winningDirections < 2;
}

static void fc_incremental_refresh_tactical_cell_side(
    FCIncrementalPosition *position,
    int x,
    int y,
    int sideIndex)
{
    int index = x * FC_BOARD_SIZE + y;
    fc_bitset_clear(position->legalMoves[sideIndex], index);
    fc_bitset_clear(position->immediateWins[sideIndex], index);
    if (position->board[x][y] != 0) return;
    int side = sideIndex == 0 ? 1 : -1;
    bool legal = position->forbiddenBlack && side == 1
        ? fc_incremental_forbidden_black_is_legal(
            position->board, x, y)
        : fc_is_legal_move(
            (const int (*)[FC_BOARD_SIZE])position->board,
            x, y, side, position->forbiddenBlack);
    if (position->forbiddenBlack && side == 1)
        fcProofDiagnostics.forbiddenLegalityCachedChecks++;
    if (!legal) return;
    fc_bitset_set(position->legalMoves[sideIndex], index);
    if (fc_wins_if_placed(position->board, x, y, side))
        fc_bitset_set(position->immediateWins[sideIndex], index);
}

static void fc_incremental_refresh_tactical_cell(
    FCIncrementalPosition *position,
    int x,
    int y)
{
    fc_incremental_refresh_tactical_cell_side(position, x, y, 0);
    fc_incremental_refresh_tactical_cell_side(position, x, y, 1);
}

static void fc_incremental_refresh_crossing_tactics(
    FCIncrementalPosition *position,
    int moveX,
    int moveY)
{
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (x != moveX && y != moveY &&
                x - y != moveX - moveY && x + y != moveX + moveY)
                continue;
            fc_incremental_refresh_tactical_cell(position, x, y);
        }
    }
}

static int fc_incremental_count_immediate_wins(
    const FCIncrementalPosition *position,
    int side,
    FCCandidate *out,
    int capacity)
{
    int sideIndex = side == 1 ? 0 : 1;
    int count = 0;
    for (int index = 0; index < FC_BOARD_SIZE * FC_BOARD_SIZE; index++) {
        if (!fc_bitset_has(position->immediateWins[sideIndex], index))
            continue;
        if (count < capacity && out != NULL) {
            out[count].x = index / FC_BOARD_SIZE;
            out[count].y = index % FC_BOARD_SIZE;
        }
        count++;
    }
    return count;
}

static void fc_incremental_add_frontier(FCIncrementalPosition *position,
                                        int originX,
                                        int originY)
{
    for (int dx = -4; dx <= 4; dx++) {
        for (int dy = -4; dy <= 4; dy++) {
            int x = originX + dx;
            int y = originY + dy;
            if (!fc_inside(x, y) || position->board[x][y] != 0) continue;
            fc_bitset_set(position->frontier, x * FC_BOARD_SIZE + y);
        }
    }
}

static void fc_incremental_touch_lines(FCIncrementalPosition *position,
                                       int x,
                                       int y)
{
    const int ids[4] = {
        y,
        x,
        x - y + FC_BOARD_SIZE - 1,
        x + y
    };
    for (int direction = 0; direction < 4; direction++) {
        position->lineRevision[direction][ids[direction]]++;
        fcProofDiagnostics.incrementalDirtyLines++;
    }
}

static int fc_incremental_line_id(int direction, int x, int y)
{
    if (direction == 0) return y;
    if (direction == 1) return x;
    if (direction == 2) return x - y + FC_BOARD_SIZE - 1;
    return x + y;
}

static uint32_t fc_incremental_encode_line(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int direction,
    int lineId)
{
    uint32_t code = 0;
    int shift = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (fc_incremental_line_id(direction, x, y) != lineId)
                continue;
            unsigned int value = board[x][y] == 1 ? 1U
                               : board[x][y] == -1 ? 2U : 0U;
            code |= value << shift;
            shift += 2;
        }
    }
    return code;
}

static void fc_incremental_refresh_line_codes(
    FCIncrementalPosition *position,
    int x,
    int y)
{
    for (int direction = 0; direction < 4; direction++) {
        int lineId = fc_incremental_line_id(direction, x, y);
        int ordinal = direction == 0 ? x
                    : direction == 1 ? y
                    : x - (lineId > FC_BOARD_SIZE - 1
                           ? lineId - (FC_BOARD_SIZE - 1) : 0);
        int shift = ordinal * 2;
        uint32_t mask = UINT32_C(3) << shift;
        unsigned int value = position->board[x][y] == 1 ? 1U
                           : position->board[x][y] == -1 ? 2U : 0U;
        position->lineCode[direction][lineId] =
            (position->lineCode[direction][lineId] & ~mask) |
            ((uint32_t)value << shift);
    }
}

bool fc_incremental_position_init(
    FCIncrementalPosition *position,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    bool forbiddenBlack)
{
    if (position == NULL || board == NULL) return false;
    memset(position, 0, sizeof(*position));
    position->stoneHash = UINT64_C(0x243f6a8885a308d3);
    position->stoneLock = UINT64_C(0x13198a2e03707344);
    position->forbiddenBlack = forbiddenBlack;
    position->complete = true;
    position->forbiddenLegalityCacheValid = true;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            int side = board[x][y];
            if (side != 0 && side != 1 && side != -1) {
                position->complete = false;
                return false;
            }
            position->board[x][y] = side;
            if (side == 0) continue;
            position->stoneCount++;
            position->stoneHash ^= fc_stone_hash_component(x, y, side);
            position->stoneLock ^= fc_stone_lock_component(x, y, side);
            fc_bitset_set(position->occupied, x * FC_BOARD_SIZE + y);
        }
    }
    for (int x = 0; x < FC_BOARD_SIZE; x++)
        for (int y = 0; y < FC_BOARD_SIZE; y++)
            if (position->board[x][y] != 0)
                fc_incremental_add_frontier(position, x, y);
    for (int direction = 0; direction < 4; direction++)
        for (int lineId = 0;
             lineId < FC_BOARD_SIZE * 2 - 1; lineId++)
            position->lineCode[direction][lineId] =
                fc_incremental_encode_line(
                    (const int (*)[FC_BOARD_SIZE])position->board,
                    direction, lineId);
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            fc_incremental_refresh_tactical_cell(position, x, y);
        }
    }
    position->legalMaskComplete[0] = true;
    position->immediateWinMaskComplete[0] = true;
    position->legalMaskComplete[1] = true;
    position->immediateWinMaskComplete[1] = true;
    fcProofDiagnostics.incrementalInitializations++;
    return true;
}

bool fc_incremental_position_make(FCIncrementalPosition *position,
                                  int x,
                                  int y,
                                  int side)
{
    if (position == NULL || !position->complete ||
        position->deltaCount >= FC_MAX_POSITION_DELTAS) return false;
    bool legal = fc_incremental_is_legal_move(position, x, y, side);
    if (!legal) return false;
    FCPositionDelta *delta = &position->deltas[position->deltaCount++];
    delta->x = x;
    delta->y = y;
    delta->side = side;
    delta->previousHash = position->stoneHash;
    delta->previousLock = position->stoneLock;
    memcpy(delta->previousFrontier, position->frontier,
           sizeof(delta->previousFrontier));
    const int lineIds[4] = {
        y, x, x - y + FC_BOARD_SIZE - 1, x + y
    };
    for (int direction = 0; direction < 4; direction++) {
        delta->previousLineRevision[direction] =
            position->lineRevision[direction][lineIds[direction]];
        delta->previousLineCode[direction] =
            position->lineCode[direction][lineIds[direction]];
    }
    position->board[x][y] = side;
    memset(position->threatCacheValid, 0,
           sizeof(position->threatCacheValid));
    position->stoneHash ^= fc_stone_hash_component(x, y, side);
    position->stoneLock ^= fc_stone_lock_component(x, y, side);
    position->stoneCount++;
    fc_bitset_set(position->occupied, x * FC_BOARD_SIZE + y);
    fc_bitset_clear(position->frontier, x * FC_BOARD_SIZE + y);
    fc_incremental_add_frontier(position, x, y);
    fc_incremental_touch_lines(position, x, y);
    fc_incremental_refresh_line_codes(position, x, y);
    fc_incremental_refresh_crossing_tactics(position, x, y);
    fcProofDiagnostics.incrementalMakes++;
    return true;
}

bool fc_incremental_position_unmake(FCIncrementalPosition *position)
{
    if (position == NULL || !position->complete || position->deltaCount <= 0)
        return false;
    FCPositionDelta delta = position->deltas[--position->deltaCount];
    if (position->board[delta.x][delta.y] != delta.side) {
        position->complete = false;
        return false;
    }
    position->board[delta.x][delta.y] = 0;
    memset(position->threatCacheValid, 0,
           sizeof(position->threatCacheValid));
    position->stoneHash = delta.previousHash;
    position->stoneLock = delta.previousLock;
    position->stoneCount--;
    fc_bitset_clear(position->occupied,
                    delta.x * FC_BOARD_SIZE + delta.y);
    memcpy(position->frontier, delta.previousFrontier,
           sizeof(position->frontier));
    const int lineIds[4] = {
        delta.y,
        delta.x,
        delta.x - delta.y + FC_BOARD_SIZE - 1,
        delta.x + delta.y
    };
    for (int direction = 0; direction < 4; direction++) {
        position->lineRevision[direction][lineIds[direction]] =
            delta.previousLineRevision[direction];
        position->lineCode[direction][lineIds[direction]] =
            delta.previousLineCode[direction];
        fcProofDiagnostics.incrementalDirtyLines++;
    }
    fc_incremental_refresh_crossing_tactics(position, delta.x, delta.y);
    fcProofDiagnostics.incrementalUnmakes++;
    return true;
}

bool fc_incremental_is_legal_move(const FCIncrementalPosition *position,
                                  int x,
                                  int y,
                                  int side)
{
    if (position == NULL || !position->complete || !fc_inside(x, y) ||
        (side != 1 && side != -1)) return false;
    int sideIndex = side == 1 ? 0 : 1;
    if (!position->legalMaskComplete[sideIndex] ||
        !position->forbiddenLegalityCacheValid) {
        return fc_is_legal_move(
            (const int (*)[FC_BOARD_SIZE])position->board,
            x, y, side, position->forbiddenBlack);
    }
    int index = x * FC_BOARD_SIZE + y;
    return fc_bitset_has(position->legalMoves[sideIndex], index);
}

bool fc_incremental_validate_legality_cache(
    FCIncrementalPosition *position)
{
    if (position == NULL || !position->complete) return false;
    bool matches = true;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (position->board[x][y] != 0) continue;
            for (int sideIndex = 0; sideIndex < 2; sideIndex++) {
                int side = sideIndex == 0 ? 1 : -1;
                fcProofDiagnostics.forbiddenLegalityCacheValidationSamples++;
                bool expected = fc_is_legal_move(
                    (const int (*)[FC_BOARD_SIZE])position->board,
                    x, y, side, position->forbiddenBlack);
                bool actual = fc_bitset_has(
                    position->legalMoves[sideIndex],
                    x * FC_BOARD_SIZE + y);
                if (position->legalMaskComplete[sideIndex] &&
                    expected != actual) {
                    matches = false;
                    fcProofDiagnostics.forbiddenLegalityCacheMismatches++;
                    fcProofDiagnostics.forbiddenLegalityCacheMismatchX =
                        (uint64_t)x;
                    fcProofDiagnostics.forbiddenLegalityCacheMismatchY =
                        (uint64_t)y;
                    fcProofDiagnostics.forbiddenLegalityCacheMismatchSide =
                        (uint64_t)(side == 1 ? 1 : 2);
                }
            }
        }
    }
    if (!matches) position->forbiddenLegalityCacheValid = false;
    return matches;
}

uint64_t fc_incremental_board_key(const FCIncrementalPosition *position,
                                  int side,
                                  int searchClass,
                                  uint64_t profileVersion)
{
    if (position == NULL || !position->complete) return 0;
    uint64_t key = position->stoneHash;
    key ^= fc_mix64((uint64_t)(side == 1 ? 1 : 2) << 32);
    key ^= fc_mix64((uint64_t)(position->forbiddenBlack ? 1 : 0) << 40);
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

const char *fc_elite_corpus_version(void)
{
    return "gomocup-elite-openings-2020-2026-rule-partitioned-v2";
}

static uint64_t fc_fnv1a(const unsigned char *bytes,
                         size_t length,
                         uint64_t seed)
{
    uint64_t value = seed;
    for (size_t i = 0; i < length; i++) {
        value ^= bytes[i];
        value *= UINT64_C(0x100000001b3);
    }
    return value;
}

static int fc_inverse_transform_id(int transform)
{
    switch (transform & 7) {
        case 1: return 3;
        case 3: return 1;
        default: return transform & 7;
    }
}

static bool fc_canonical_corpus_key(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    uint64_t *keyA,
    uint64_t *keyB,
    int *selectedTransform,
    int *selectedShiftX,
    int *selectedShiftY,
    int *stoneCount,
    bool forbiddenBlack)
{
    unsigned char best[FC_BOARD_SIZE * FC_BOARD_SIZE + 2];
    bool hasBest = false;
    int count = 0;
    int minimumEdge = FC_BOARD_SIZE;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (board[x][y] == 0) continue;
            count++;
            int edge = x;
            if (y < edge) edge = y;
            if (FC_BOARD_SIZE - 1 - x < edge) edge = FC_BOARD_SIZE - 1 - x;
            if (FC_BOARD_SIZE - 1 - y < edge) edge = FC_BOARD_SIZE - 1 - y;
            if (edge < minimumEdge) minimumEdge = edge;
        }
    }
    if (count == 0 || count > 28 || minimumEdge < 2) return false;
    for (int transform = 0; transform < 8; transform++) {
        int minX = FC_BOARD_SIZE;
        int minY = FC_BOARD_SIZE;
        for (int x = 0; x < FC_BOARD_SIZE; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                if (board[x][y] == 0) continue;
                int tx = 0, ty = 0;
                fc_transform_point(transform, x, y, &tx, &ty);
                if (tx < minX) minX = tx;
                if (ty < minY) minY = ty;
            }
        }
        unsigned char candidate[FC_BOARD_SIZE * FC_BOARD_SIZE + 2];
        memset(candidate, 0, sizeof(candidate));
        for (int x = 0; x < FC_BOARD_SIZE; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                int value = board[x][y];
                if (value == 0) continue;
                int tx = 0, ty = 0;
                fc_transform_point(transform, x, y, &tx, &ty);
                candidate[(tx - minX) * FC_BOARD_SIZE + ty - minY] =
                    (unsigned char)(value == 1 ? 1 : 2);
            }
        }
        candidate[FC_BOARD_SIZE * FC_BOARD_SIZE] =
            (unsigned char)(side == 1 ? 1 : 2);
        candidate[FC_BOARD_SIZE * FC_BOARD_SIZE + 1] =
            (unsigned char)(forbiddenBlack ? 1 : 0);
        if (!hasBest || memcmp(candidate, best, sizeof(best)) < 0) {
            memcpy(best, candidate, sizeof(best));
            hasBest = true;
            if (selectedTransform != NULL) *selectedTransform = transform;
            if (selectedShiftX != NULL) *selectedShiftX = minX;
            if (selectedShiftY != NULL) *selectedShiftY = minY;
        }
    }
    if (!hasBest) return false;
    if (keyA != NULL)
        *keyA = fc_fnv1a(best, sizeof(best), UINT64_C(0xcbf29ce484222325));
    if (keyB != NULL)
        *keyB = fc_fnv1a(best, sizeof(best), UINT64_C(0x84222325cbf29ce4));
    if (stoneCount != NULL) *stoneCount = count;
    return true;
}

static int fc_elite_move_compare(const void *leftValue, const void *rightValue)
{
    const FCEliteCorpusMove *left = (const FCEliteCorpusMove *)leftValue;
    const FCEliteCorpusMove *right = (const FCEliteCorpusMove *)rightValue;
    if (left->trustTier != right->trustTier)
        return right->trustTier - left->trustTier;
    if (left->events != right->events) return right->events - left->events;
    if (left->games != right->games) return right->games - left->games;
    if (left->requiredStones != right->requiredStones)
        return right->requiredStones - left->requiredStones;
    if (left->x != right->x) return left->x - right->x;
    return left->y - right->y;
}

static void fc_merge_elite_move(FCEliteCorpusMove *moves,
                                int *count,
                                int capacity,
                                FCEliteCorpusMove candidate)
{
    for (int i = 0; i < *count; i++) {
        if (moves[i].x != candidate.x || moves[i].y != candidate.y) continue;
        if (fc_elite_move_compare(&candidate, &moves[i]) < 0)
            moves[i] = candidate;
        return;
    }
    if (*count < capacity) moves[(*count)++] = candidate;
}

static void fc_transform_offset(int transform,
                                int dx,
                                int dy,
                                int *outDX,
                                int *outDY)
{
    static const int matrix[8][4] = {
        {1, 0, 0, 1}, {0, 1, -1, 0}, {-1, 0, 0, -1}, {0, -1, 1, 0},
        {-1, 0, 0, 1}, {1, 0, 0, -1}, {0, 1, 1, 0}, {0, -1, -1, 0}
    };
    const int *m = matrix[transform & 7];
    *outDX = m[0] * dx + m[1] * dy;
    *outDY = m[2] * dx + m[3] * dy;
}

static int fc_elite_local_lookup(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    FCEliteCorpusMove *out,
    int capacity,
    int *outPatternIndex)
{
    int stones = 0;
    for (int x = 0; x < FC_BOARD_SIZE; x++)
        for (int y = 0; y < FC_BOARD_SIZE; y++)
            if (board[x][y] != 0) stones++;
    if (stones < 4 || stones > 28) return 0;
    int count = 0;
    int firstPattern = -1;
    int rule = forbiddenBlack ? 1 : 0;
    for (int patternIndex = 0; patternIndex < fcEliteLocalPatternCount;
         patternIndex++) {
        const FCEliteLocalPattern *pattern =
            &fcEliteLocalPatterns[patternIndex];
        if (pattern->rule != rule || pattern->side != side ||
            pattern->stoneCount == 0) continue;
        const FCEliteLocalStone *first =
            &fcEliteLocalStones[pattern->stoneStart];
        for (int transform = 0; transform < 8; transform++) {
            int firstDX = 0, firstDY = 0;
            fc_transform_offset(transform, first->dx, first->dy,
                                &firstDX, &firstDY);
            for (int bx = 0; bx < FC_BOARD_SIZE; bx++) {
                for (int by = 0; by < FC_BOARD_SIZE; by++) {
                    if (board[bx][by] != first->side) continue;
                    int candidateX = bx - firstDX;
                    int candidateY = by - firstDY;
                    if (!fc_inside(candidateX, candidateY) ||
                        board[candidateX][candidateY] != 0) continue;
                    int edge = candidateX;
                    if (candidateY < edge) edge = candidateY;
                    if (FC_BOARD_SIZE - 1 - candidateX < edge)
                        edge = FC_BOARD_SIZE - 1 - candidateX;
                    if (FC_BOARD_SIZE - 1 - candidateY < edge)
                        edge = FC_BOARD_SIZE - 1 - candidateY;
                    int boundaryClass = edge >= 5 ? 0 : 1;
                    if (boundaryClass != pattern->boundaryClass) continue;
                    bool matches = true;
                    for (int stoneIndex = 0;
                         stoneIndex < pattern->stoneCount; stoneIndex++) {
                        const FCEliteLocalStone *required =
                            &fcEliteLocalStones[pattern->stoneStart + stoneIndex];
                        int dx = 0, dy = 0;
                        fc_transform_offset(transform, required->dx,
                                            required->dy, &dx, &dy);
                        int x = candidateX + dx;
                        int y = candidateY + dy;
                        if (!fc_inside(x, y) || board[x][y] != required->side) {
                            matches = false;
                            break;
                        }
                    }
                    if (!matches || !fc_is_legal_move(board, candidateX,
                                                       candidateY, side,
                                                       forbiddenBlack)) continue;
                    FCEliteCorpusMove move = {
                        candidateX, candidateY, pattern->games, pattern->events,
                        pattern->sources, pattern->wins, pattern->draws,
                        pattern->losses, pattern->trustTier, FC_CORPUS_MATCH_LOCAL,
                        pattern->stoneCount, pattern->sourceBoardMask
                    };
                    fc_merge_elite_move(out, &count, capacity, move);
                    if (firstPattern < 0) firstPattern = patternIndex;
                }
            }
        }
    }
    qsort(out, (size_t)count, sizeof(out[0]), fc_elite_move_compare);
    if (outPatternIndex != NULL) *outPatternIndex = firstPattern;
    return count;
}

int fc_elite_corpus_lookup_detailed(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    FCEliteCorpusMove *out,
    int capacity,
    int *outPositionIndex,
    int *outMatchType)
{
    if (outPositionIndex != NULL) *outPositionIndex = -1;
    if (outMatchType != NULL) *outMatchType = FC_CORPUS_MATCH_NONE;
    if (board == NULL || out == NULL || capacity <= 0 ||
        (side != 1 && side != -1)) return 0;
    uint64_t keyA = 0, keyB = 0;
    int transform = 0, shiftX = 0, shiftY = 0, stones = 0;
    if (!fc_canonical_corpus_key(board, side, &keyA, &keyB,
                                 &transform, &shiftX, &shiftY, &stones,
                                 forbiddenBlack)) {
        int localIndex = -1;
        int localCount = fc_elite_local_lookup(board, side, forbiddenBlack,
                                               out, capacity, &localIndex);
        if (localCount > 0) {
            if (outPositionIndex != NULL) *outPositionIndex = -2 - localIndex;
            if (outMatchType != NULL) *outMatchType = FC_CORPUS_MATCH_LOCAL;
        }
        return localCount;
    }
    int rule = forbiddenBlack ? 1 : 0;
    int low = 0;
    int high = fcEliteCorpusPositionCount;
    int found = -1;
    while (low < high) {
        int middle = low + (high - low) / 2;
        const FCEliteCorpusPosition *position = &fcEliteCorpusPositions[middle];
        bool before = position->keyA < keyA ||
            (position->keyA == keyA && position->keyB < keyB) ||
            (position->keyA == keyA && position->keyB == keyB &&
             position->stoneCount < stones) ||
            (position->keyA == keyA && position->keyB == keyB &&
             position->stoneCount == stones && position->rule < rule);
        bool equal = position->keyA == keyA && position->keyB == keyB &&
                     position->stoneCount == stones && position->rule == rule;
        if (equal) {
            found = middle;
            break;
        }
        if (before) low = middle + 1; else high = middle;
    }
    if (found < 0) {
        int localIndex = -1;
        int localCount = fc_elite_local_lookup(board, side, forbiddenBlack,
                                               out, capacity, &localIndex);
        if (localCount > 0) {
            if (outPositionIndex != NULL) *outPositionIndex = -2 - localIndex;
            if (outMatchType != NULL) *outMatchType = FC_CORPUS_MATCH_LOCAL;
        }
        return localCount;
    }
    if (outPositionIndex != NULL) *outPositionIndex = found;
    if (outMatchType != NULL) *outMatchType = FC_CORPUS_MATCH_EXACT;
    const FCEliteCorpusPosition *position = &fcEliteCorpusPositions[found];
    int stored = position->candidateCount < capacity
        ? position->candidateCount : capacity;
    int inverse = fc_inverse_transform_id(transform);
    for (int i = 0; i < stored; i++) {
        const FCEliteCorpusCandidate *candidate =
            &fcEliteCorpusCandidates[position->candidateStart + i];
        int transformedX = candidate->x + shiftX;
        int transformedY = candidate->y + shiftY;
        int x = 0, y = 0;
        fc_transform_point(inverse, transformedX, transformedY, &x, &y);
        out[i] = (FCEliteCorpusMove){
            x, y, candidate->games, candidate->events, candidate->sources,
            candidate->wins, candidate->draws, candidate->losses,
            candidate->trustTier, FC_CORPUS_MATCH_EXACT, stones,
            candidate->sourceBoardMask
        };
    }
    return stored;
}

int fc_elite_corpus_lookup(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                           int side,
                           bool forbiddenBlack,
                           FCEliteCorpusMove *out,
                           int capacity,
                           int *outPositionIndex)
{
    return fc_elite_corpus_lookup_detailed(board, side, forbiddenBlack, out,
                                           capacity, outPositionIndex, NULL);
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
            if (fc_global_proof_deadline_reached()) return count;
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
                    moves[count].opponentImmediateWinCount = 0;
                    moves[count].forkRisk = FC_FORK_RISK_OWN_WIN;
                    moves[count].forkProbeComplete = true;
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
                    FC_TACTICAL_IMMEDIATE_WIN, true, 0.0, 0,
                    FC_FORK_RISK_OWN_WIN, true
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
            if (fc_global_proof_deadline_reached()) {
                if (overflow != NULL) *overflow = true;
                return count;
            }
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
            if (fc_global_proof_deadline_reached()) {
                if (overflow != NULL) *overflow = true;
                return count;
            }
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
            if (fc_global_proof_deadline_reached()) return false;
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

static bool fc_threat_contains_point(const FCPoint *points,
                                     int count,
                                     FCPoint point)
{
    for (int i = 0; i < count; i++)
        if (fc_same_point(points[i], point)) return true;
    return false;
}

bool fc_threats_dependency_compatible(const FCThreat *left,
                                      const FCThreat *right)
{
    if (fc_same_point(left->gain, right->gain)) return false;
    bool linked = fc_threat_contains_point(
        left->rests, left->restCount, right->gain) ||
        fc_threat_contains_point(
            right->rests, right->restCount, left->gain);
    if (!linked) return false;
    if (fc_threat_contains_point(
            left->costs, left->costCount, right->gain) ||
        fc_threat_contains_point(
            right->costs, right->costCount, left->gain)) return false;
    for (int i = 0; i < left->restCount; i++)
        if (fc_threat_contains_point(
                right->costs, right->costCount, left->rests[i]))
            return false;
    for (int i = 0; i < right->restCount; i++)
        if (fc_threat_contains_point(
                left->costs, left->costCount, right->rests[i]))
            return false;
    return true;
}

typedef struct {
    uint64_t outgoing[FC_MAX_THREATS];
    int longest[FC_MAX_THREATS];
    int count;
} FCThreatDependencyDAG;

static bool fc_dependency_path_exists(const FCThreatDependencyDAG *dag,
                                      int start,
                                      int target)
{
    uint64_t visited = 0;
    int stack[FC_MAX_THREATS];
    int stackCount = 0;
    stack[stackCount++] = start;
    while (stackCount > 0) {
        int node = stack[--stackCount];
        if (node == target) return true;
        uint64_t bit = UINT64_C(1) << node;
        if ((visited & bit) != 0) continue;
        visited |= bit;
        uint64_t edges = dag->outgoing[node];
        for (int child = 0; child < dag->count; child++) {
            if ((edges & (UINT64_C(1) << child)) == 0) continue;
            if (stackCount < FC_MAX_THREATS)
                stack[stackCount++] = child;
        }
    }
    return false;
}

static void fc_dependency_add_edge(FCThreatDependencyDAG *dag,
                                   int source,
                                   int destination)
{
    if (source == destination ||
        fc_dependency_path_exists(dag, destination, source)) return;
    uint64_t bit = UINT64_C(1) << destination;
    if ((dag->outgoing[source] & bit) != 0) return;
    dag->outgoing[source] |= bit;
    fcProofDiagnostics.dependencyCombinations++;
}

static int fc_dependency_longest_from(FCThreatDependencyDAG *dag, int node)
{
    if (dag->longest[node] > 0) return dag->longest[node];
    int longest = 1;
    uint64_t edges = dag->outgoing[node];
    for (int child = 0; child < dag->count; child++) {
        if ((edges & (UINT64_C(1) << child)) == 0) continue;
        int candidate = 1 + fc_dependency_longest_from(dag, child);
        if (candidate > longest) longest = candidate;
    }
    dag->longest[node] = longest;
    return longest;
}

static void fc_build_threat_dependency_dag(const FCThreat *threats,
                                           int count,
                                           FCThreatDependencyDAG *dag)
{
    memset(dag, 0, sizeof(*dag));
    dag->count = count;
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (!fc_threats_dependency_compatible(
                    &threats[i], &threats[j])) continue;
            bool iToJ = fc_threat_contains_point(
                threats[i].rests, threats[i].restCount, threats[j].gain);
            bool jToI = fc_threat_contains_point(
                threats[j].rests, threats[j].restCount, threats[i].gain);
            if (iToJ) fc_dependency_add_edge(dag, i, j);
            if (jToI) fc_dependency_add_edge(dag, j, i);
        }
    }
    for (int i = 0; i < count; i++) {
        int depth = fc_dependency_longest_from(dag, i);
        if (depth > 1) fcProofDiagnostics.dependencyChainsProposed++;
        if ((uint64_t)depth > fcProofDiagnostics.dependencyMaximumDepth)
            fcProofDiagnostics.dependencyMaximumDepth = (uint64_t)depth;
    }
}

static void fc_order_threats_by_dependency(const FCThreat *threats,
                                           int count,
                                           int searchClass,
                                           int order[FC_MAX_THREATS])
{
    FCThreatDependencyDAG dag;
    for (int i = 0; i < count; i++) order[i] = i;
    if (searchClass < FC_PROOF_SEARCH_VCT || count <= 1) return;
    fc_build_threat_dependency_dag(threats, count, &dag);
    for (int i = 1; i < count; i++) {
        int value = order[i];
        int j = i;
        while (j > 0 &&
               threats[value].severity == threats[order[j - 1]].severity &&
               dag.longest[value] > dag.longest[order[j - 1]]) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = value;
    }
}

static void fc_threat_mask_point(
    uint64_t mask[FC_POSITION_BITSET_WORDS],
    int x,
    int y)
{
    if (fc_inside(x, y)) fc_bitset_set(mask, x * FC_BOARD_SIZE + y);
}

static void fc_threat_mask_or(
    uint64_t destination[FC_POSITION_BITSET_WORDS],
    const uint64_t source[FC_POSITION_BITSET_WORDS])
{
    for (int i = 0; i < FC_POSITION_BITSET_WORDS; i++)
        destination[i] |= source[i];
}

static void fc_threat_mask_and(
    uint64_t destination[FC_POSITION_BITSET_WORDS],
    const uint64_t source[FC_POSITION_BITSET_WORDS])
{
    for (int i = 0; i < FC_POSITION_BITSET_WORDS; i++)
        destination[i] &= source[i];
}

static bool fc_threat_mask_equal(
    const uint64_t left[FC_POSITION_BITSET_WORDS],
    const uint64_t right[FC_POSITION_BITSET_WORDS])
{
    return memcmp(left, right,
                  sizeof(uint64_t) * FC_POSITION_BITSET_WORDS) == 0;
}

static uint64_t fc_threat_mask_count(
    const uint64_t mask[FC_POSITION_BITSET_WORDS])
{
    uint64_t count = 0;
    for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
        uint64_t value = mask[word];
        while (value != 0) {
            value &= value - 1;
            count++;
        }
    }
    return count;
}

static void fc_populate_threat_dependencies(FCThreat *threat,
                                            bool forbiddenBlack)
{
    threat->lineIds[0] = threat->gain.x;
    threat->lineIds[1] = threat->gain.y;
    threat->lineIds[2] = threat->gain.x - threat->gain.y +
                         FC_BOARD_SIZE - 1;
    threat->lineIds[3] = threat->gain.x + threat->gain.y;
    fc_threat_mask_point(threat->dependencyMask,
                         threat->gain.x, threat->gain.y);
    for (int i = 0; i < threat->costCount; i++)
        fc_threat_mask_point(threat->dependencyMask,
                             threat->costs[i].x, threat->costs[i].y);
    for (int i = 0; i < threat->restCount; i++)
        fc_threat_mask_point(threat->dependencyMask,
                             threat->rests[i].x, threat->rests[i].y);

    for (int direction = 0; direction < 4; direction++) {
        int dx = fcDirections[direction][0];
        int dy = fcDirections[direction][1];
        for (int startOffset = -4; startOffset <= 0; startOffset++) {
            int startX = threat->gain.x + startOffset * dx;
            int startY = threat->gain.y + startOffset * dy;
            int endX = startX + 4 * dx;
            int endY = startY + 4 * dy;
            if (!fc_inside(startX, startY) || !fc_inside(endX, endY))
                continue;
            for (int step = 0; step < 5; step++)
                fc_threat_mask_point(threat->fiveWindowMask,
                                     startX + step * dx,
                                     startY + step * dy);
        }
        if (forbiddenBlack && threat->side == 1) {
            for (int offset = -FC_BOARD_SIZE + 1;
                 offset < FC_BOARD_SIZE; offset++) {
                int x = threat->gain.x + offset * dx;
                int y = threat->gain.y + offset * dy;
                fc_threat_mask_point(threat->legalityDependencyMask, x, y);
            }
        }
    }
    fc_threat_mask_or(threat->dependencyMask, threat->fiveWindowMask);
    fc_threat_mask_or(threat->dependencyMask,
                      threat->legalityDependencyMask);
    fc_threat_mask_or(threat->certificateZoneMask,
                      threat->dependencyMask);
    fcProofDiagnostics.threatDependencyPoints +=
        fc_threat_mask_count(threat->dependencyMask);
    fcProofDiagnostics.threatFiveWindowPoints +=
        fc_threat_mask_count(threat->fiveWindowMask);
    fcProofDiagnostics.threatLegalityDependencyPoints +=
        fc_threat_mask_count(threat->legalityDependencyMask);
    fcProofDiagnostics.threatCertificateZonePoints +=
        fc_threat_mask_count(threat->certificateZoneMask);
}

static int fc_enumerate_threats_internal(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int searchClass,
    FCThreat *out,
    int capacity,
    bool *overflow,
    const FCIncrementalPosition *position)
{
    if (overflow != NULL) *overflow = false;
    if (board == NULL || out == NULL || capacity <= 0 ||
        (side != 1 && side != -1)) return 0;
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    FCCandidate existingWins[FC_MAX_CANDIDATES];
    int existingWinCount = position != NULL
        ? fc_incremental_count_immediate_wins(
            position, side, existingWins, FC_MAX_CANDIDATES)
        : fc_count_immediate_wins(
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
            if (fc_global_proof_deadline_reached()) {
                if (overflow != NULL) *overflow = true;
                int partial = count < capacity ? count : capacity;
                qsort(out, (size_t)partial, sizeof(FCThreat),
                      fc_compare_threat);
                return partial;
            }
            fcProofDiagnostics.threatCellScans++;
            int cellIndex = x * FC_BOARD_SIZE + y;
            if (position != NULL) {
                if (!fc_bitset_has(position->frontier, cellIndex)) continue;
            } else if (!fc_has_neighbor(
                           (const int (*)[FC_BOARD_SIZE])mutableBoard,
                           x, y, 4)) {
                continue;
            }
            bool legal = position != NULL
                ? fc_incremental_is_legal_move(position, x, y, side)
                : fc_is_legal_move(
                    (const int (*)[FC_BOARD_SIZE])mutableBoard,
                    x, y, side, forbiddenBlack);
            if (!legal) continue;
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
                                if (fc_global_proof_deadline_reached()) {
                                    if (overflow != NULL) *overflow = true;
                                    mutableBoard[x][y] = 0;
                                    int partial = count < capacity
                                        ? count : capacity;
                                    qsort(out, (size_t)partial,
                                          sizeof(FCThreat),
                                          fc_compare_threat);
                                    return partial;
                                }
                                /* The gain is held only in mutableBoard for
                                 * this local probe; the incremental position
                                 * still describes the parent.  Use the
                                 * reference oracle until that temporary move
                                 * is made through the reversible position. */
                                if (!fc_is_legal_move(
                                        (const int (*)[FC_BOARD_SIZE])mutableBoard,
                                        dx, dy, -side, forbiddenBlack)) continue;
                                mutableBoard[dx][dy] = -side;
                                bool defenderWins = fc_has_five(
                                    (const int (*)[FC_BOARD_SIZE])mutableBoard,
                                    dx, dy, -side);
                                int remaining = defenderWins ? 0
                                    : restOverflow
                                    ? fc_count_open_four_creators(
                                        mutableBoard, side, forbiddenBlack,
                                        NULL, 0, NULL)
                                    : fc_count_surviving_creators(
                                        mutableBoard, side, forbiddenBlack,
                                        threat.rests, threat.restCount);
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
            fc_populate_threat_dependencies(&threat, forbiddenBlack);
            if (count < capacity) out[count] = threat;
            else if (overflow != NULL) *overflow = true;
            count++;
        }
    }
    int stored = count < capacity ? count : capacity;
    qsort(out, (size_t)stored, sizeof(FCThreat), fc_compare_threat);
    return stored;
}

int fc_enumerate_threats(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int side,
                         bool forbiddenBlack,
                         int searchClass,
                         FCThreat *out,
                         int capacity,
                         bool *overflow)
{
    return fc_enumerate_threats_internal(
        board, side, forbiddenBlack, searchClass,
        out, capacity, overflow, NULL);
}

int fc_incremental_enumerate_threats(
    FCIncrementalPosition *position,
    int side,
    int searchClass,
    FCThreat *out,
    int capacity,
    bool *overflow)
{
    if (position == NULL || !position->complete) return 0;
    int sideIndex = side == 1 ? 0 : 1;
    int classIndex = searchClass >= FC_PROOF_SEARCH_VCT ? 1 : 0;
    if (!position->threatCacheValid[sideIndex][classIndex]) {
        bool cachedOverflow = false;
        int count = fc_enumerate_threats_internal(
            (const int (*)[FC_BOARD_SIZE])position->board,
            side, position->forbiddenBlack, searchClass,
            position->threatCache[sideIndex][classIndex],
            FC_MAX_THREATS, &cachedOverflow, position);
        position->threatCacheCount[sideIndex][classIndex] =
            (uint8_t)count;
        position->threatCacheOverflow[sideIndex][classIndex] =
            cachedOverflow;
        position->threatCacheValid[sideIndex][classIndex] =
            !fc_global_proof_deadline_reached();
    }
    int cachedCount = position->threatCacheCount[sideIndex][classIndex];
    int copied = cachedCount < capacity ? cachedCount : capacity;
    if (out != NULL && copied > 0) {
        memcpy(out, position->threatCache[sideIndex][classIndex],
               (size_t)copied * sizeof(FCThreat));
    }
    if (overflow != NULL) {
        *overflow = position->threatCacheOverflow[sideIndex][classIndex] ||
                    cachedCount > capacity;
    }
    return copied;
}

static bool fc_proof_budget_available(FCProofContext *context)
{
    if (fc_decision_deadline_reached()) {
        context->aborted = true;
        if (context->session != NULL) context->session->exhausted = true;
        return false;
    }
    if (context->nodes >= context->queryNodeBudget) {
        context->aborted = true;
        return false;
    }
    uint64_t priorNodes = context->session != NULL
        ? context->session->nodesUsed : 0;
    if (priorNodes + context->nodes >= context->nodeBudget) {
        context->aborted = true;
        if (context->session != NULL) context->session->exhausted = true;
        return false;
    }
    double timeStarted = context->session != NULL
        ? context->session->startedMilliseconds
        : context->startedMilliseconds;
    uint32_t timeBudget = context->session != NULL
        ? context->session->timeBudgetMs : context->timeBudgetMs;
    if (context->queryTimeBudgetMs > 0 &&
        (timeBudget == 0 || context->queryTimeBudgetMs < timeBudget) &&
        fc_now_milliseconds() - context->startedMilliseconds >=
            context->queryTimeBudgetMs) {
        context->aborted = true;
        return false;
    }
    if (timeBudget > 0 &&
        fc_now_milliseconds() - timeStarted >= timeBudget) {
        context->aborted = true;
        if (context->session != NULL) context->session->exhausted = true;
        return false;
    }
    if (!fc_parallel_node_token_take()) {
        context->aborted = true;
        if (context->session != NULL) context->session->exhausted = true;
        return false;
    }
    if (fcActiveDecisionLedger != NULL &&
        !fc_decision_ledger_consume_node(fcActiveDecisionLedger)) {
        fc_parallel_node_token_return_one();
        context->aborted = true;
        if (context->session != NULL) context->session->exhausted = true;
        return false;
    }
    return true;
}

static bool fc_proof_session_begin(
    FCProofSession *session,
    const FCAIProfile *profile,
    const int rootBoard[FC_BOARD_SIZE][FC_BOARD_SIZE],
    bool forbiddenBlack)
{
    memset(session, 0, sizeof(*session));
    session->ledger = fcActiveDecisionLedger;
    session->tableCapacity = profile->proofTranspositionCapacity;
    session->nodeBudget = profile->proofNodeBudget;
    /* Research decisions use the top-level ledger deadline.  The historical
     * emergency allowance remains available only to frozen, non-ledger
     * callers; it must never extend a 5.7 decision past its reservation. */
    session->timeBudgetMs = fcActiveDecisionLedger == NULL &&
        profile->proofEmergencyTimeBudgetMs > 0
        ? profile->proofEmergencyTimeBudgetMs : profile->proofTimeBudgetMs;
    session->startedMilliseconds = fc_now_milliseconds();
    fcProofGeneration++;
    if (fcProofGeneration == 0) fcProofGeneration++;
    session->generation = fcProofGeneration;
    session->forbiddenBlack = forbiddenBlack;
    if (rootBoard != NULL) {
        memcpy(session->rootBoard, rootBoard, sizeof(session->rootBoard));
        session->rootPositionReady = fc_incremental_position_init(
            &session->position, rootBoard, forbiddenBlack);
        session->positionBasedOnRoot = session->rootPositionReady;
        if (!session->rootPositionReady) return false;
        if (profile->validateLegalityCache &&
            !fc_incremental_validate_legality_cache(&session->position)) {
            /* Keep the position usable, but force all subsequent legality
             * queries through the reference oracle for this decision. */
            session->position.forbiddenLegalityCacheValid = false;
        }
    }
    if (session->tableCapacity > 0) {
        session->graphNodeCapacity = profile->proofGraphNodeCapacity > 0
            ? profile->proofGraphNodeCapacity : FC_DFPN_NODE_CAPACITY;
        session->graphEdgeCapacity = profile->proofGraphEdgeCapacity > 0
            ? profile->proofGraphEdgeCapacity : FC_DFPN_EDGE_CAPACITY;
        if (session->graphNodeCapacity > FC_DFPN_NODE_CAPACITY)
            session->graphNodeCapacity = FC_DFPN_NODE_CAPACITY;
        if (session->graphEdgeCapacity > FC_DFPN_EDGE_CAPACITY)
            session->graphEdgeCapacity = FC_DFPN_EDGE_CAPACITY;
        size_t tableBytes = session->tableCapacity * sizeof(FCDFPNTTEntry);
        size_t nodeBytes = session->graphNodeCapacity * sizeof(FCDFPNNode);
        size_t edgeBytes = session->graphEdgeCapacity * sizeof(FCDFPNEdge);
        size_t totalBytes = tableBytes + nodeBytes + edgeBytes;
        if (session->ledger != NULL &&
            !fc_decision_ledger_reserve_memory(
                session->ledger, (uint64_t)totalBytes)) {
            session->exhausted = true;
            session->tableCapacity = 0;
            session->graphNodeCapacity = 0;
            session->graphEdgeCapacity = 0;
            fcProofDiagnostics.decisionStageAbandons++;
            fcActiveProofSession = session;
            return true;
        }
        if (session->ledger != NULL) {
            session->memoryReservationBytes = (uint64_t)totalBytes;
            session->memoryReservationHeld = true;
        }
        session->memory = malloc(totalBytes);
        fcProofDiagnostics.allocations++;
        fcProofDiagnostics.allocatedBytes += totalBytes;
        if (session->memory != NULL) {
            unsigned char *cursor = session->memory;
            session->dfpnTable = (FCDFPNTTEntry *)cursor;
            memset(session->dfpnTable, 0, tableBytes);
            fcProofDiagnostics.clearedBytes += tableBytes;
            cursor += tableBytes;
            session->graphNodes = (FCDFPNNode *)cursor;
            cursor += nodeBytes;
            session->graphEdges = (FCDFPNEdge *)cursor;
        } else {
            if (session->memoryReservationHeld) {
                fc_decision_ledger_release_memory(
                    session->ledger, session->memoryReservationBytes);
                session->memoryReservationBytes = 0;
                session->memoryReservationHeld = false;
            }
            session->tableCapacity = 0;
            session->graphNodeCapacity = 0;
            session->graphEdgeCapacity = 0;
        }
    }
    fcActiveProofSession = session;
    return true;
}

static FCIncrementalPosition *fc_proof_session_prepare_position(
    FCProofSession *session,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    bool forbiddenBlack)
{
    if (session == NULL || board == NULL || !session->rootPositionReady ||
        forbiddenBlack != session->forbiddenBlack) return NULL;
    if (!session->positionBasedOnRoot) {
        if (!fc_incremental_position_init(
                &session->position,
                (const int (*)[FC_BOARD_SIZE])session->rootBoard,
                forbiddenBlack)) return NULL;
        session->positionBasedOnRoot = true;
    }
    while (session->position.deltaCount > 0) {
        if (!fc_incremental_position_unmake(&session->position)) return NULL;
    }
    if (memcmp(session->position.board, session->rootBoard,
               sizeof(session->rootBoard)) != 0) {
        if (!fc_incremental_position_init(
                &session->position,
                (const int (*)[FC_BOARD_SIZE])session->rootBoard,
                forbiddenBlack)) return NULL;
    }
    int differenceCount = 0;
    int differenceX = -1;
    int differenceY = -1;
    int differenceSide = 0;
    bool additionsOnly = true;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (board[x][y] == session->rootBoard[x][y]) continue;
            differenceCount++;
            differenceX = x;
            differenceY = y;
            differenceSide = board[x][y];
            if (session->rootBoard[x][y] != 0 ||
                (differenceSide != 1 && differenceSide != -1))
                additionsOnly = false;
        }
    }
    if (differenceCount == 0) return &session->position;
    if (differenceCount == 1 && additionsOnly &&
        fc_incremental_position_make(
            &session->position, differenceX, differenceY, differenceSide))
        return &session->position;

    /* Arbitrary proof queries remain supported, but are explicitly detached
     * from the reusable root and re-rooted on the next session query. */
    if (!fc_incremental_position_init(&session->position, board,
                                      forbiddenBlack)) return NULL;
    session->positionBasedOnRoot = false;
    return &session->position;
}

/* Reuse the private DFPN arena for the next sibling obligation.  A reused
 * session is not a reused search: the graph is logically retired by a new
 * generation, counters are reset to the per-job ceiling, and the incremental
 * position is re-rooted before the caller enters fc_prove_forced_win().  This
 * keeps the allocation/TT initialization cost out of short parallel waves
 * without allowing proof state to leak from one job into another. */
static bool fc_proof_session_reset_for_query(
    FCProofSession *session,
    const FCAIProfile *profile,
    const int rootBoard[FC_BOARD_SIZE][FC_BOARD_SIZE],
    bool forbiddenBlack)
{
    if (session == NULL || profile == NULL || rootBoard == NULL ||
        !session->rootPositionReady ||
        session->forbiddenBlack != forbiddenBlack) return false;
    if (session->exhausted)
        fcProofDiagnostics.proofSessionGlobalTerminations++;

    bool sameRoot = memcmp(session->rootBoard, rootBoard,
                           sizeof(session->rootBoard)) == 0;
    if (sameRoot) {
        if (fc_proof_session_prepare_position(
                session, rootBoard, forbiddenBlack) == NULL) return false;
    } else {
        memcpy(session->rootBoard, rootBoard, sizeof(session->rootBoard));
        if (!fc_incremental_position_init(
                &session->position, rootBoard, forbiddenBlack)) return false;
        session->rootPositionReady = true;
        session->positionBasedOnRoot = true;
        session->forbiddenBlack = forbiddenBlack;
    }
    if (profile->validateLegalityCache &&
        !fc_incremental_validate_legality_cache(&session->position)) {
        session->position.forbiddenLegalityCacheValid = false;
    }

    session->nodesUsed = 0;
    session->startedMilliseconds = fc_now_milliseconds();
    session->timeBudgetMs = fcActiveDecisionLedger == NULL &&
        profile->proofEmergencyTimeBudgetMs > 0
        ? profile->proofEmergencyTimeBudgetMs : profile->proofTimeBudgetMs;
    session->graphNodeCount = 0;
    session->graphEdgeCount = 0;
    session->exhausted = false;
    fcProofGeneration++;
    if (fcProofGeneration == 0) fcProofGeneration++;
    session->generation = fcProofGeneration;
    fcActiveProofSession = session;
    return true;
}

static void fc_proof_session_end(FCProofSession *session,
                                 FCProofSession *previous)
{
    if (session == NULL) return;
    if (session->exhausted)
        fcProofDiagnostics.proofSessionGlobalTerminations++;
    free(session->memory);
    session->memory = NULL;
    if (session->memoryReservationHeld) {
        fc_decision_ledger_release_memory(
            session->ledger, session->memoryReservationBytes);
        session->memoryReservationBytes = 0;
        session->memoryReservationHeld = false;
    }
    fcActiveProofSession = previous;
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
    FCProofNode *node = &context->certificate[index];
    memset(node, 0, sizeof(*node));
    node->boardHash = boardHash;
    node->parent = parent;
    node->x = x;
    node->y = y;
    node->side = side;
    node->terminalWin = terminalWin;
    return index;
}

static uint64_t fc_proof_board_key(FCProofContext *context,
                                   const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                   int side)
{
    if (context->position != NULL &&
        board == (const int (*)[FC_BOARD_SIZE])context->position->board) {
        return fc_incremental_board_key(
            context->position, side, context->searchClass,
            fc_proof_key_version(context->maxDepth));
    }
    return fc_board_key(board, side, context->forbiddenBlack,
                        context->searchClass,
                        fc_proof_key_version(context->maxDepth));
}

static uint64_t fc_proof_board_verification(FCProofContext *context,
                                            uint64_t key)
{
    if (context->position == NULL) return key;
    uint64_t scope = ((uint64_t)(context->attacker == 1 ? 1 : 2) << 8) ^
                     ((uint64_t)context->searchClass << 16) ^
                     ((uint64_t)context->maxDepth << 24) ^
                     ((uint64_t)context->forbiddenBlack << 40);
    return fc_mix64(context->position->stoneLock ^ scope ^
                    FC_PROOF_ALGORITHM_VERSION);
}

static bool fc_proof_make_move(FCProofContext *context,
                               int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                               int x,
                               int y,
                               int side)
{
    if (context->position != NULL &&
        board == context->position->board)
        return fc_incremental_position_make(context->position, x, y, side);
    return fc_make_move(board, x, y, side, context->forbiddenBlack);
}

static void fc_proof_unmake_move(FCProofContext *context,
                                 int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                 int x,
                                 int y)
{
    if (context->position != NULL && board == context->position->board) {
        bool restored = fc_incremental_position_unmake(context->position);
        if (!restored) context->aborted = true;
        return;
    }
    fc_unmake_move(board, x, y);
}

static int fc_generate_refutations_internal(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    FCPoint *out,
    int capacity,
    const FCIncrementalPosition *position,
    const uint64_t initialRelevance[FC_POSITION_BITSET_WORDS],
    int threatSeverity,
    uint64_t outRelatedZone[FC_POSITION_BITSET_WORDS],
    uint64_t outVerifiedOmissions[FC_POSITION_BITSET_WORDS])
{
    (void)threatSeverity;
    uint64_t relevance[FC_POSITION_BITSET_WORDS] = {0};
    if (outRelatedZone != NULL)
        memset(outRelatedZone, 0,
               sizeof(uint64_t) * FC_POSITION_BITSET_WORDS);
    if (outVerifiedOmissions != NULL)
        memset(outVerifiedOmissions, 0,
               sizeof(uint64_t) * FC_POSITION_BITSET_WORDS);
    if (initialRelevance != NULL) {
        for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++)
            relevance[word] = initialRelevance[word];
    }
    FCCandidate immediatePoints[FC_MAX_CANDIDATES];
    int immediate = position != NULL
        ? fc_incremental_count_immediate_wins(
            position, attacker, immediatePoints, FC_MAX_CANDIDATES)
        : fc_count_immediate_wins(
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
    bool allLegalFallback = forbiddenBlack && attacker == 1 &&
                            initialRelevance == NULL;
    if (allLegalFallback) fcProofDiagnostics.relevanceFallbacks++;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (fc_global_proof_deadline_reached()) {
                fcProofDiagnostics.relevanceUnresolved++;
                return -1;
            }
            fcProofDiagnostics.refutationCellScans++;
            if (board[x][y] != 0) continue;
            int cellIndex = x * FC_BOARD_SIZE + y;
            bool initiallyRelevant = fc_bitset_has(relevance, cellIndex);
            if (!forbiddenBlack && !initiallyRelevant && !fc_has_neighbor(
                    (const int (*)[FC_BOARD_SIZE])board, x, y, 4)) {
                /* With no stone in any potential five-window, this move
                 * cannot win, counter-force, occupy a threat dependency, or
                 * affect another intersection's legality in freestyle play. */
                fcProofDiagnostics.relevanceAllLegalReplies++;
                fcProofDiagnostics.relevanceVerifiedOmissions++;
                if (outVerifiedOmissions != NULL)
                    fc_bitset_set(outVerifiedOmissions, cellIndex);
                continue;
            }
            bool legal = position != NULL
                ? fc_incremental_is_legal_move(position, x, y, -attacker)
                : fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                   x, y, -attacker, forbiddenBlack);
            if (!legal) continue;
            fcProofDiagnostics.relevanceAllLegalReplies++;
            if (allLegalFallback) {
                fcProofDiagnostics.relevanceFallbackReplies++;
                fcProofDiagnostics.relevanceReplyCandidates++;
                fc_bitset_set(relevance, cellIndex);
                if (count < capacity) out[count] = (FCPoint){x, y};
                count++;
                continue;
            }
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
            if (defenderWins || validCounterThreat)
                fc_bitset_set(relevance, cellIndex);
            bool removesAllThreats = remaining == 0;
            if (!defenderWins && !validCounterThreat && !blocksThreatPoint &&
                !removesAllThreats) {
                fcProofDiagnostics.relevanceVerifiedOmissions++;
                if (outVerifiedOmissions != NULL)
                    fc_bitset_set(outVerifiedOmissions, cellIndex);
                continue;
            }
            fcProofDiagnostics.relevanceReplyCandidates++;
            if (count < capacity) out[count] = (FCPoint){x, y};
            count++;
        }
    }
    fcProofDiagnostics.relevanceZonePoints +=
        fc_threat_mask_count(relevance);
    if (outRelatedZone != NULL)
        memcpy(outRelatedZone, relevance,
               sizeof(uint64_t) * FC_POSITION_BITSET_WORDS);
    return count < capacity ? count : capacity;
}

static int fc_generate_refutations(int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                   int attacker,
                                   bool forbiddenBlack,
                                   int searchClass,
                                   FCPoint *out,
                                   int capacity)
{
    return fc_generate_refutations_internal(
        board, attacker, forbiddenBlack, searchClass,
        out, capacity, NULL, NULL, FC_THREAT_NONE, NULL, NULL);
}

int fc_reference_generate_refutations(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    FCPoint *out,
    int capacity)
{
    if (board == NULL || out == NULL || capacity <= 0) return 0;
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    return fc_generate_refutations(
        mutableBoard, attacker, forbiddenBlack,
        searchClass, out, capacity);
}

int fc_incremental_generate_refutations(
    FCIncrementalPosition *position,
    int attacker,
    int searchClass,
    FCPoint *out,
    int capacity)
{
    if (position == NULL || !position->complete ||
        out == NULL || capacity <= 0) return 0;
    return fc_generate_refutations_internal(
        position->board, attacker, position->forbiddenBlack,
        searchClass, out, capacity, position,
        NULL, FC_THREAT_NONE, NULL, NULL);
}

void fc_dfpn_frontier_estimate(int maxDepth,
                               int remainingDepth,
                               int branchingHint,
                               uint64_t *proofNumber,
                               uint64_t *disproofNumber)
{
    if (proofNumber == NULL || disproofNumber == NULL) return;
    int spent = maxDepth - remainingDepth;
    if (spent < 0) spent = 0;
    int branch = branchingHint > 4096 ? 4096 : branchingHint;
    if (branch < 1) branch = 1;
    *proofNumber = (uint64_t)branch;
    *disproofNumber = (uint64_t)(1 + spent / 4);
}

static void fc_dfpn_frontier_values(const FCProofContext *context,
                                    int remainingDepth,
                                    int branchingHint,
                                    uint64_t *proofNumber,
                                    uint64_t *disproofNumber)
{
    fc_dfpn_frontier_estimate(context->maxDepth, remainingDepth,
                              branchingHint, proofNumber, disproofNumber);
}

static uint64_t fc_dfpn_node_key(FCProofContext *context,
                                 const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                 int remainingDepth,
                                 int type)
{
    int side = type == FC_DFPN_OR_NODE
        ? context->attacker : -context->attacker;
    uint64_t base = fc_proof_board_key(context, board, side);
    uint64_t scope = ((uint64_t)(unsigned int)(remainingDepth + 1) << 40) ^
                     ((uint64_t)(unsigned int)type << 56) ^
                     UINT64_C(0x4446504e47524150);
    /* Root filters define independent obligations.  Include the filter only
     * while creating the root node; descendants deliberately keep the normal
     * board key so an overlap group can reuse their TT/DFPN progress. */
    if (type == FC_DFPN_OR_NODE && context->dfpnRoot < 0 &&
        fcProofRootFilterX >= 0 && fcProofRootFilterY >= 0) {
        scope ^= ((uint64_t)(unsigned int)(fcProofRootFilterX + 1) << 8) ^
                 ((uint64_t)(unsigned int)(fcProofRootFilterY + 1) << 16) ^
                 UINT64_C(0x524f4f5446494c54);
    }
    return fc_mix64(base ^ scope);
}

static uint64_t fc_dfpn_node_verification(FCProofContext *context,
                                          int remainingDepth,
                                          int type,
                                          uint64_t key)
{
    uint64_t verification = fc_proof_board_verification(context, key);
    return fc_mix64(verification ^
                    ((uint64_t)(unsigned int)(remainingDepth + 1) << 36) ^
                    ((uint64_t)(unsigned int)type << 60));
}

static void fc_dfpn_sync_tt(FCProofContext *context, int nodeIndex)
{
    FCProofSession *session = context->session;
    if (session == NULL || context->tableCapacity == 0 || nodeIndex < 0)
        return;
    FCDFPNNode *node = &session->graphNodes[nodeIndex];
    FCDFPNTTEntry *entry = &session->dfpnTable[
        node->key % context->tableCapacity];
    if (entry->generation != context->generation || entry->key != node->key ||
        entry->verification != node->verification ||
        entry->graphIndexPlusOne != (uint32_t)(nodeIndex + 1)) return;
    entry->depth = node->remainingDepth;
    entry->status = (unsigned char)node->status;
    entry->proofNumber = node->proofNumber;
    entry->disproofNumber = node->disproofNumber;
}

static int fc_dfpn_find_or_create(
    FCProofContext *context,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int remainingDepth,
    int type,
    int branchingHint)
{
    FCProofSession *session = context->session;
    if (session == NULL || session->graphNodes == NULL) return -1;
    uint64_t key = fc_dfpn_node_key(
        context, board, remainingDepth, type);
    uint64_t verification = fc_dfpn_node_verification(
        context, remainingDepth, type, key);
    FCDFPNTTEntry *entry = context->tableCapacity > 0
        ? &session->dfpnTable[key % context->tableCapacity] : NULL;
    if (entry != NULL && entry->generation == context->generation &&
        entry->key == key && entry->verification == verification &&
        entry->depth == remainingDepth && entry->graphIndexPlusOne > 0) {
        int index = (int)entry->graphIndexPlusOne - 1;
        if (index >= 0 && (size_t)index < session->graphNodeCount) {
            FCDFPNNode *node = &session->graphNodes[index];
            if (node->key == key && node->verification == verification &&
                node->remainingDepth == remainingDepth &&
                node->type == type) {
                context->hits++;
                return index;
            }
        }
    }
    if (session->graphNodeCount >= session->graphNodeCapacity) {
        context->aborted = true;
        session->exhausted = true;
        fcProofDiagnostics.proofGraphArenaExhaustions++;
        return -1;
    }
    int index = (int)session->graphNodeCount++;
    FCDFPNNode *node = &session->graphNodes[index];
    memset(node, 0, sizeof(*node));
    node->key = key;
    node->verification = verification;
    node->remainingDepth = remainingDepth;
    node->firstEdge = -1;
    node->selectedEdge = -1;
    node->type = (unsigned char)type;
    node->status = FC_PROOF_UNKNOWN;
    fc_dfpn_frontier_values(context, remainingDepth, branchingHint,
                            &node->proofNumber, &node->disproofNumber);
    fcProofDiagnostics.proofGraphNodes++;
    if (entry != NULL) {
        entry->key = key;
        entry->verification = verification;
        entry->proofNumber = node->proofNumber;
        entry->disproofNumber = node->disproofNumber;
        entry->depth = remainingDepth;
        entry->graphIndexPlusOne = (uint32_t)(index + 1);
        entry->generation = context->generation;
        entry->status = FC_PROOF_UNKNOWN;
    }
    return index;
}

static bool fc_dfpn_append_edge(FCProofContext *context,
                                FCDFPNNode *node,
                                int child,
                                int x,
                                int y)
{
    FCProofSession *session = context->session;
    if (session == NULL || child < 0 ||
        session->graphEdgeCount >= session->graphEdgeCapacity) {
        context->aborted = true;
        if (session != NULL) session->exhausted = true;
        fcProofDiagnostics.proofGraphArenaExhaustions++;
        return false;
    }
    if (node->edgeCount == 0)
        node->firstEdge = (int)session->graphEdgeCount;
    else if (node->firstEdge + node->edgeCount !=
             (int)session->graphEdgeCount) {
        context->aborted = true;
        session->exhausted = true;
        fcProofDiagnostics.proofGraphArenaExhaustions++;
        return false;
    }
    session->graphEdges[session->graphEdgeCount++] =
        (FCDFPNEdge){child, (short)x, (short)y};
    node->edgeCount++;
    fcProofDiagnostics.proofGraphEdges++;
    return true;
}

static void fc_dfpn_set_terminal(FCProofContext *context,
                                 int nodeIndex,
                                 int status,
                                 int distance)
{
    FCDFPNNode *node = &context->session->graphNodes[nodeIndex];
    node->expanded = true;
    node->enumerationComplete = true;
    node->status = (unsigned char)status;
    node->distance = distance;
    if (status == FC_PROOF_PROVEN_WIN) {
        node->proofNumber = 0;
        node->disproofNumber = FC_PROOF_INFINITY;
        memcpy(node->relatedZoneMask, node->relevanceMask,
               sizeof(node->relatedZoneMask));
        node->relatedZoneComplete = true;
    } else {
        node->proofNumber = FC_PROOF_INFINITY;
        node->disproofNumber = 0;
        node->relatedZoneComplete = false;
    }
    fc_dfpn_sync_tt(context, nodeIndex);
}

static void fc_dfpn_update_related_zone(FCProofContext *context,
                                        int nodeIndex)
{
    FCProofSession *session = context->session;
    FCDFPNNode *node = &session->graphNodes[nodeIndex];
    if (node->proofNumber != 0) {
        node->relatedZoneComplete = false;
        node->relatedZoneContinuationCount = 0;
        return;
    }
    if (node->type == FC_DFPN_AND_NODE) {
        if (!node->enumerationComplete) {
            node->relatedZoneComplete = false;
            return;
        }
        uint64_t combined[FC_POSITION_BITSET_WORDS];
        memcpy(combined, node->replyRelatedZoneMask, sizeof(combined));
        for (int i = 0; i < node->edgeCount; i++) {
            FCDFPNEdge *edge = &session->graphEdges[node->firstEdge + i];
            FCDFPNNode *child = &session->graphNodes[edge->child];
            if (child->proofNumber != 0 || !child->relatedZoneComplete) {
                node->relatedZoneComplete = false;
                return;
            }
            fc_threat_mask_or(combined, child->relatedZoneMask);
            fc_bitset_set(combined,
                          edge->x * FC_BOARD_SIZE + edge->y);
        }
        memcpy(node->relatedZoneMask, combined, sizeof(combined));
        node->relatedZoneComplete = true;
        return;
    }

    uint64_t combined[FC_POSITION_BITSET_WORDS] = {0};
    uint64_t firstZone[FC_POSITION_BITSET_WORDS] = {0};
    int continuationCount = 0;
    for (int i = 0; i < node->edgeCount; i++) {
        FCDFPNEdge *edge = &session->graphEdges[node->firstEdge + i];
        FCDFPNNode *child = &session->graphNodes[edge->child];
        if (child->proofNumber != 0 || !child->relatedZoneComplete)
            continue;
        bool duplicateGain = false;
        for (int previous = 0; previous < i; previous++) {
            FCDFPNEdge *other = &session->graphEdges[
                node->firstEdge + previous];
            FCDFPNNode *otherChild = &session->graphNodes[other->child];
            if (otherChild->proofNumber == 0 &&
                otherChild->relatedZoneComplete &&
                other->x == edge->x && other->y == edge->y) {
                duplicateGain = true;
                break;
            }
        }
        if (duplicateGain) continue;
        if (continuationCount == 0) {
            memcpy(combined, child->relatedZoneMask, sizeof(combined));
            memcpy(firstZone, child->relatedZoneMask, sizeof(firstZone));
        } else {
            fc_threat_mask_and(combined, child->relatedZoneMask);
        }
        continuationCount++;
    }
    if (continuationCount == 0) {
        node->relatedZoneComplete = false;
        return;
    }
    int intersections = continuationCount - 1;
    uint64_t firstCount = fc_threat_mask_count(firstZone);
    uint64_t combinedCount = fc_threat_mask_count(combined);
    int pointsRemoved = firstCount > combinedCount
        ? (int)(firstCount - combinedCount) : 0;
    if (intersections > node->relatedZoneIntersectionCount) {
        fcProofDiagnostics.iteratedRelatedZoneIntersections +=
            (uint64_t)(intersections - node->relatedZoneIntersectionCount);
    }
    if (continuationCount > node->relatedZoneContinuationCount) {
        fcProofDiagnostics.iteratedRelatedZoneContinuations +=
            (uint64_t)(continuationCount -
                       node->relatedZoneContinuationCount);
    }
    if (pointsRemoved > node->relatedZonePointsRemoved) {
        fcProofDiagnostics.iteratedRelatedZonePointsRemoved +=
            (uint64_t)(pointsRemoved - node->relatedZonePointsRemoved);
    }
    node->relatedZoneIntersectionCount = (unsigned short)intersections;
    node->relatedZoneContinuationCount =
        (unsigned short)continuationCount;
    node->relatedZonePointsRemoved = (unsigned short)pointsRemoved;
    memcpy(node->relatedZoneMask, combined, sizeof(combined));
    node->relatedZoneComplete = true;
}

static void fc_dfpn_recompute(FCProofContext *context, int nodeIndex)
{
    FCProofSession *session = context->session;
    FCDFPNNode *node = &session->graphNodes[nodeIndex];
    if (!node->expanded) return;
    uint64_t proof = node->type == FC_DFPN_OR_NODE
        ? FC_PROOF_INFINITY : 0;
    uint64_t disproof = node->type == FC_DFPN_OR_NODE
        ? 0 : FC_PROOF_INFINITY;
    int selected = -1;
    int distance = 0;
    for (int i = 0; i < node->edgeCount; i++) {
        int edgeIndex = node->firstEdge + i;
        FCDFPNNode *child =
            &session->graphNodes[session->graphEdges[edgeIndex].child];
        if (node->type == FC_DFPN_OR_NODE) {
            if (child->proofNumber < proof) {
                proof = child->proofNumber;
                selected = edgeIndex;
            }
            disproof = fc_proof_saturated_add(
                disproof, child->disproofNumber);
        } else {
            proof = fc_proof_saturated_add(proof, child->proofNumber);
            if (child->disproofNumber < disproof) {
                disproof = child->disproofNumber;
                selected = edgeIndex;
            }
            if (child->distance + 1 > distance)
                distance = child->distance + 1;
        }
    }
    if (!node->enumerationComplete) {
        uint64_t frontierProof = 1;
        uint64_t frontierDisproof = 1;
        if (node->type == FC_DFPN_OR_NODE) {
            if (frontierProof < proof) {
                proof = frontierProof;
                selected = -1;
            }
            disproof = fc_proof_saturated_add(disproof, frontierDisproof);
        } else {
            proof = fc_proof_saturated_add(proof, frontierProof);
            if (frontierDisproof < disproof) {
                disproof = frontierDisproof;
                selected = -1;
            }
        }
    }
    if (node->edgeCount == 0 && node->enumerationComplete) {
        if (node->type == FC_DFPN_OR_NODE) {
            proof = FC_PROOF_INFINITY;
            disproof = 0;
        } else {
            proof = 0;
            disproof = FC_PROOF_INFINITY;
            distance = 2;
        }
    }
    node->proofNumber = proof;
    node->disproofNumber = disproof;
    node->selectedEdge = selected;
    node->status = proof == 0 ? FC_PROOF_PROVEN_WIN
                 : disproof == 0 ? FC_PROOF_NO_FORCED_WIN_IN_SCOPE
                 : FC_PROOF_UNKNOWN;
    if (node->type == FC_DFPN_OR_NODE && selected >= 0 && proof == 0)
        distance = session->graphNodes[
            session->graphEdges[selected].child].distance + 1;
    node->distance = distance;
    fc_dfpn_update_related_zone(context, nodeIndex);
    fc_dfpn_sync_tt(context, nodeIndex);
}

static bool fc_dfpn_expand_or(FCProofContext *context,
                              int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                              int nodeIndex)
{
    FCDFPNNode *node = &context->session->graphNodes[nodeIndex];
    if (!fc_proof_budget_available(context)) return false;
    context->nodes++;
    fcProofDiagnostics.mostProvingExpansions++;
    if (node->remainingDepth <= 0) {
        fc_dfpn_set_terminal(context, nodeIndex,
                             FC_PROOF_NO_FORCED_WIN_IN_SCOPE, 0);
        return true;
    }
    int ownImmediate = context->position != NULL
        ? fc_incremental_count_immediate_wins(
            context->position, context->attacker, NULL, 0)
        : fc_count_immediate_wins(
            board, context->attacker, context->forbiddenBlack, NULL, 0);
    int defenderImmediate = context->position != NULL
        ? fc_incremental_count_immediate_wins(
            context->position, -context->attacker, NULL, 0)
        : fc_count_immediate_wins(
            board, -context->attacker, context->forbiddenBlack, NULL, 0);
    if (defenderImmediate > 0 && ownImmediate == 0) {
        fc_dfpn_set_terminal(context, nodeIndex,
                             FC_PROOF_NO_FORCED_WIN_IN_SCOPE, 0);
        return true;
    }
    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int threatCount = 0;
    if (nodeIndex == context->dfpnRoot && fcActiveRootThreat != NULL) {
        /* The coordinator already paid for root enumeration.  A worker gets
         * exactly one immutable gain record and starts proof below it. */
        threats[0] = *fcActiveRootThreat;
        threatCount = 1;
    } else {
        threatCount = context->position != NULL
            ? fc_incremental_enumerate_threats(
                context->position, context->attacker, context->searchClass,
                threats, FC_MAX_THREATS, &overflow)
            : fc_enumerate_threats(
                (const int (*)[FC_BOARD_SIZE])board, context->attacker,
                context->forbiddenBlack, context->searchClass,
                threats, FC_MAX_THREATS, &overflow);
    }
    node->expanded = true;
    node->enumerationComplete = !overflow &&
        !(context->session != NULL && context->session->exhausted);
    int threatOrder[FC_MAX_THREATS];
    fc_order_threats_by_dependency(
        threats, threatCount, context->searchClass, threatOrder);
    for (int orderIndex = 0; orderIndex < threatCount; orderIndex++) {
        int i = threatOrder[orderIndex];
        FCThreat *threat = &threats[i];
        if (nodeIndex == context->dfpnRoot &&
            fcProofRootFilterX >= 0 && fcProofRootFilterY >= 0 &&
            (threat->gain.x != fcProofRootFilterX ||
             threat->gain.y != fcProofRootFilterY)) continue;
        if (!fc_proof_make_move(context, board, threat->gain.x,
                                threat->gain.y, context->attacker)) {
            node->enumerationComplete = false;
            continue;
        }
        int child = fc_dfpn_find_or_create(
            context, (const int (*)[FC_BOARD_SIZE])board,
            node->remainingDepth, FC_DFPN_AND_NODE,
            1 + orderIndex * 64);
        if (child >= 0) {
            FCDFPNNode *childNode =
                &context->session->graphNodes[child];
            for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++)
                childNode->relevanceMask[word] |=
                    threat->certificateZoneMask[word];
            if (threat->severity > childNode->threatSeverity)
                childNode->threatSeverity =
                    (unsigned char)threat->severity;
        }
        bool terminal = fc_has_five(
            (const int (*)[FC_BOARD_SIZE])board,
            threat->gain.x, threat->gain.y, context->attacker);
        fc_proof_unmake_move(context, board, threat->gain.x, threat->gain.y);
        if (child < 0 || !fc_dfpn_append_edge(
                context, node, child, threat->gain.x, threat->gain.y)) {
            node->enumerationComplete = false;
            break;
        }
        if (terminal)
            fc_dfpn_set_terminal(context, child, FC_PROOF_PROVEN_WIN, 0);
    }
    fc_dfpn_recompute(context, nodeIndex);
    return !context->aborted;
}

static bool fc_dfpn_expand_and(FCProofContext *context,
                               int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                               int nodeIndex)
{
    FCDFPNNode *node = &context->session->graphNodes[nodeIndex];
    if (!fc_proof_budget_available(context)) return false;
    context->nodes++;
    fcProofDiagnostics.mostProvingExpansions++;
    FCPoint refutations[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int count = fc_generate_refutations_internal(
        board, context->attacker, context->forbiddenBlack,
        context->searchClass, refutations,
        FC_BOARD_SIZE * FC_BOARD_SIZE, context->position,
        node->relevanceMask, node->threatSeverity,
        node->replyRelatedZoneMask, node->verifiedOmissionMask);
    node->expanded = true;
    node->enumerationComplete = count >= 0 &&
        !(context->session != NULL && context->session->exhausted);
    if (count < 0) count = 0;
    for (int i = 0; i < count; i++) {
        FCPoint reply = refutations[i];
        if (!fc_proof_make_move(context, board, reply.x, reply.y,
                                -context->attacker)) {
            node->enumerationComplete = false;
            continue;
        }
        int childDepth = node->remainingDepth - 2;
        int child = fc_dfpn_find_or_create(
            context, (const int (*)[FC_BOARD_SIZE])board,
            childDepth, FC_DFPN_OR_NODE, 1);
        bool defenderWins = fc_has_five(
            (const int (*)[FC_BOARD_SIZE])board,
            reply.x, reply.y, -context->attacker);
        fc_proof_unmake_move(context, board, reply.x, reply.y);
        if (child < 0 || !fc_dfpn_append_edge(
                context, node, child, reply.x, reply.y)) {
            node->enumerationComplete = false;
            break;
        }
        if (defenderWins)
            fc_dfpn_set_terminal(context, child,
                                 FC_PROOF_NO_FORCED_WIN_IN_SCOPE, 0);
    }
    fc_dfpn_recompute(context, nodeIndex);
    return !context->aborted;
}

static uint64_t fc_dfpn_sibling_sum(FCProofContext *context,
                                    const FCDFPNNode *node,
                                    int selectedEdge,
                                    bool proofValues)
{
    uint64_t sum = 0;
    for (int i = 0; i < node->edgeCount; i++) {
        int edgeIndex = node->firstEdge + i;
        if (edgeIndex == selectedEdge) continue;
        FCDFPNNode *child = &context->session->graphNodes[
            context->session->graphEdges[edgeIndex].child];
        sum = fc_proof_saturated_add(
            sum, proofValues ? child->proofNumber : child->disproofNumber);
    }
    if (!node->enumerationComplete)
        sum = fc_proof_saturated_add(sum, 1);
    return sum;
}

static uint64_t fc_dfpn_second_value(FCProofContext *context,
                                     const FCDFPNNode *node,
                                     int selectedEdge,
                                     bool proofValues)
{
    uint64_t second = FC_PROOF_INFINITY;
    for (int i = 0; i < node->edgeCount; i++) {
        int edgeIndex = node->firstEdge + i;
        if (edgeIndex == selectedEdge) continue;
        FCDFPNNode *child = &context->session->graphNodes[
            context->session->graphEdges[edgeIndex].child];
        uint64_t value = proofValues
            ? child->proofNumber : child->disproofNumber;
        if (value < second) second = value;
    }
    if (!node->enumerationComplete && second > 1) second = 1;
    return second;
}

static uint64_t fc_dfpn_residual_threshold(uint64_t parentThreshold,
                                           uint64_t siblingSum)
{
    if (parentThreshold >= FC_PROOF_INFINITY) return FC_PROOF_INFINITY;
    if (siblingSum >= parentThreshold) return 1;
    return parentThreshold - siblingSum;
}

static int fc_dfpn_postponed_index(const FCProofContext *context,
                                   int edge)
{
    if (context == NULL) return -1;
    for (int i = 0; i < context->postponedCount; i++) {
        if (context->postponedEdges[i].edge == edge) return i;
    }
    return -1;
}

static void fc_dfpn_remove_postponed(FCProofContext *context, int index)
{
    if (context == NULL || index < 0 || index >= context->postponedCount)
        return;
    int last = --context->postponedCount;
    if (index != last)
        context->postponedEdges[index] = context->postponedEdges[last];
}

static bool fc_dfpn_enqueue_postponed(FCProofContext *context,
                                      int edge,
                                      uint64_t proofThreshold,
                                      uint64_t disproofThreshold)
{
    if (context == NULL || edge < 0) return false;
    int existing = fc_dfpn_postponed_index(context, edge);
    uint64_t revisit = fc_proof_saturated_add(
        context->nodes, UINT64_C(2));
    if (existing >= 0) {
        FCPostponedEdge *entry = &context->postponedEdges[existing];
        /* Keep the more permissive threshold and the earliest deterministic
         * revisit point.  This prevents a shallow slice from starving a
         * previously queued obligation. */
        if (proofThreshold < entry->proofThreshold)
            entry->proofThreshold = proofThreshold;
        if (disproofThreshold < entry->disproofThreshold)
            entry->disproofThreshold = disproofThreshold;
        if (revisit < entry->revisitEpoch) entry->revisitEpoch = revisit;
        return true;
    }
    if (context->postponedCount >= FC_DFPN_POSTPONED_CAPACITY)
        return false;
    FCPostponedEdge *entry = &context->postponedEdges[
        context->postponedCount++];
    entry->edge = edge;
    entry->proofThreshold = proofThreshold;
    entry->disproofThreshold = disproofThreshold;
    entry->revisitEpoch = revisit;
    entry->insertionIndex = context->postponedInsertionIndex++;
    return true;
}

static int fc_dfpn_select_edge(FCProofContext *context,
                               const FCDFPNNode *node)
{
    if (node == NULL || node->edgeCount <= 0) return -1;
    int selected = -1;
    uint64_t best = FC_PROOF_INFINITY;
    short bestX = SHRT_MAX;
    short bestY = SHRT_MAX;
    for (int i = 0; i < node->edgeCount; i++) {
        int edgeIndex = node->firstEdge + i;
        int postponed = fc_dfpn_postponed_index(context, edgeIndex);
        if (postponed >= 0 &&
            context->nodes < context->postponedEdges[postponed].revisitEpoch)
            continue;
        FCDFPNNode *child = &context->session->graphNodes[
            context->session->graphEdges[edgeIndex].child];
        uint64_t value = node->type == FC_DFPN_OR_NODE
            ? child->proofNumber : child->disproofNumber;
        short x = context->session->graphEdges[edgeIndex].x;
        short y = context->session->graphEdges[edgeIndex].y;
        if (value < best ||
            (value == best && (x < bestX ||
                               (x == bestX && y < bestY)))) {
            best = value;
            selected = edgeIndex;
            bestX = x;
            bestY = y;
        }
    }
    if (selected < 0 && context->postponedCount > 0) {
        /* All siblings are waiting for a later slice.  Requeue the oldest
         * eligible edge belonging to this node, then select it. */
        int oldest = -1;
        for (int i = 0; i < node->edgeCount; i++) {
            int edgeIndex = node->firstEdge + i;
            int postponed = fc_dfpn_postponed_index(context, edgeIndex);
            if (postponed < 0) continue;
            if (oldest < 0 ||
                context->postponedEdges[postponed].revisitEpoch <
                    context->postponedEdges[oldest].revisitEpoch ||
                (context->postponedEdges[postponed].revisitEpoch ==
                     context->postponedEdges[oldest].revisitEpoch &&
                 context->postponedEdges[postponed].insertionIndex <
                     context->postponedEdges[oldest].insertionIndex))
                oldest = postponed;
        }
        if (oldest < 0) return -1;
        context->postponedEdges[oldest].revisitEpoch = context->nodes;
        fcProofDiagnostics.dfpnDovetailRequeues++;
        return fc_dfpn_select_edge(context, node);
    }
    return selected;
}

static void fc_dfpn_search(FCProofContext *context,
                           int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                           int nodeIndex,
                           uint64_t proofThreshold,
                           uint64_t disproofThreshold)
{
    FCDFPNNode *node = &context->session->graphNodes[nodeIndex];
    if (!node->expanded) {
        bool expanded = node->type == FC_DFPN_OR_NODE
            ? fc_dfpn_expand_or(context, board, nodeIndex)
            : fc_dfpn_expand_and(context, board, nodeIndex);
        if (!expanded) return;
    }
    while (!context->aborted && node->proofNumber < proofThreshold &&
           node->disproofNumber < disproofThreshold &&
           node->status == FC_PROOF_UNKNOWN) {
        /* Expanded DFPN graphs can revisit the same most-proving edge without
         * allocating another node.  Expansion-only budget checks therefore do
         * not bound this loop.  Poll the shared decision/session deadline on
         * every iteration so an already-expanded cycle unwinds honestly as
         * unknown and the caller can return its best completed legal move. */
        if (!fc_proof_budget_available(context)) break;
        uint64_t priorNodeCount = context->nodes;
        uint64_t priorProofNumber = node->proofNumber;
        uint64_t priorDisproofNumber = node->disproofNumber;
        int priorSelectedEdge = node->selectedEdge;
        int selected = fc_dfpn_select_edge(context, node);
        if (selected < 0) break;
        FCDFPNEdge edge = context->session->graphEdges[selected];
        int postponedIndex = fc_dfpn_postponed_index(context, selected);
        uint64_t childProofThreshold = proofThreshold;
        uint64_t childDisproofThreshold = disproofThreshold;
        if (node->type == FC_DFPN_OR_NODE) {
            uint64_t second = fc_dfpn_second_value(
                context, node, selected, true);
            uint64_t secondPlusOne = fc_proof_saturated_add(second, 1);
            if (childProofThreshold > secondPlusOne)
                childProofThreshold = secondPlusOne;
            childDisproofThreshold = fc_dfpn_residual_threshold(
                disproofThreshold,
                fc_dfpn_sibling_sum(context, node, selected, false));
        } else {
            childProofThreshold = fc_dfpn_residual_threshold(
                proofThreshold,
                fc_dfpn_sibling_sum(context, node, selected, true));
            uint64_t second = fc_dfpn_second_value(
                context, node, selected, false);
            uint64_t secondPlusOne = fc_proof_saturated_add(second, 1);
            if (childDisproofThreshold > secondPlusOne)
                childDisproofThreshold = secondPlusOne;
        }
        if (postponedIndex >= 0) {
            const FCPostponedEdge *postponed =
                &context->postponedEdges[postponedIndex];
            if (postponed->proofThreshold < childProofThreshold)
                childProofThreshold = postponed->proofThreshold;
            if (postponed->disproofThreshold < childDisproofThreshold)
                childDisproofThreshold = postponed->disproofThreshold;
            fc_dfpn_remove_postponed(context, postponedIndex);
        }
        if (!fc_proof_make_move(context, board, edge.x, edge.y,
                                node->type == FC_DFPN_OR_NODE
                                    ? context->attacker
                                    : -context->attacker)) {
            context->aborted = true;
            break;
        }
        fc_dfpn_search(context, board, edge.child,
                       childProofThreshold, childDisproofThreshold);
        fc_proof_unmake_move(context, board, edge.x, edge.y);
        fc_dfpn_recompute(context, nodeIndex);
        node = &context->session->graphNodes[nodeIndex];
        if (!context->aborted && context->nodes == priorNodeCount &&
            node->proofNumber == priorProofNumber &&
            node->disproofNumber == priorDisproofNumber &&
            node->selectedEdge == priorSelectedEdge) {
            if (fcActiveDecisionLedger == NULL) {
                /* Preserve the frozen proof-engine contract.  The postponed
                 * sibling scheduler is enabled only for the research
                 * recovery profile, where its telemetry is part of the
                 * decision ledger. */
                context->noProgress = true;
                fcProofDiagnostics.dfpnNoProgressTerminations++;
                break;
            }
            if (!fc_dfpn_enqueue_postponed(
                    context, selected, childProofThreshold,
                    childDisproofThreshold)) {
                context->noProgress = true;
                fcProofDiagnostics.dfpnNoProgressTerminations++;
                break;
            }
            fcProofDiagnostics.dfpnPostponedSiblings++;
            /* Give a deterministic sibling a turn.  If all siblings are
             * postponed, the selector performs a dovetail requeue; the
             * shared budget/deadline still terminates the session honestly. */
            continue;
        }
    }
}

bool fc_test_dfpn_stalled_graph_respects_deadline(double *elapsedMilliseconds)
{
    FCProofSession session;
    FCProofContext context;
    FCDFPNNode nodes[2];
    FCDFPNEdge edges[1];
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{0}};
    memset(&session, 0, sizeof(session));
    memset(&context, 0, sizeof(context));
    memset(nodes, 0, sizeof(nodes));
    session.graphNodes = nodes;
    session.graphNodeCapacity = 2;
    session.graphNodeCount = 2;
    session.graphEdges = edges;
    session.graphEdgeCapacity = 1;
    session.graphEdgeCount = 1;
    session.nodeBudget = UINT64_MAX;
    session.timeBudgetMs = 2;
    session.startedMilliseconds = fc_now_milliseconds();
    context.attacker = 1;
    context.maxDepth = 4;
    context.nodeBudget = UINT64_MAX;
    context.queryNodeBudget = UINT64_MAX;
    context.startedMilliseconds = session.startedMilliseconds;
    context.session = &session;
    nodes[0].type = FC_DFPN_OR_NODE;
    nodes[0].expanded = true;
    nodes[0].enumerationComplete = true;
    nodes[0].status = FC_PROOF_UNKNOWN;
    nodes[0].proofNumber = 1;
    nodes[0].disproofNumber = 1;
    nodes[0].firstEdge = 0;
    nodes[0].edgeCount = 1;
    nodes[1].type = FC_DFPN_OR_NODE;
    nodes[1].expanded = true;
    nodes[1].enumerationComplete = false;
    nodes[1].status = FC_PROOF_UNKNOWN;
    nodes[1].proofNumber = 1;
    nodes[1].disproofNumber = 1;
    edges[0] = (FCDFPNEdge){.child = 1, .x = 7, .y = 7};
    fc_dfpn_search(&context, board, 0,
                   FC_PROOF_INFINITY, FC_PROOF_INFINITY);
    double elapsed = fc_now_milliseconds() - session.startedMilliseconds;
    if (elapsedMilliseconds != NULL) *elapsedMilliseconds = elapsed;
    return ((context.aborted && session.exhausted) || context.noProgress) &&
           elapsed < 250.0 &&
           board[7][7] == 0;
}

static bool fc_dfpn_build_certificate(FCProofContext *context,
                                      int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                      int nodeIndex,
                                      int parent);

static bool fc_dfpn_build_attack_certificate(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int edgeIndex,
    int parent)
{
    FCProofSession *session = context->session;
    int checkpoint = context->certificateCount;
    FCDFPNEdge attack = session->graphEdges[edgeIndex];
    uint64_t attackHash = fc_proof_board_key(
        context, (const int (*)[FC_BOARD_SIZE])board, context->attacker);
    int attackCertificate = fc_proof_append(
        context, attackHash, parent, attack.x, attack.y,
        context->attacker, false);
    if (attackCertificate < 0 || !fc_proof_make_move(
            context, board, attack.x, attack.y, context->attacker)) {
        context->certificateCount = checkpoint;
        return false;
    }
    FCDFPNNode *andNode = &session->graphNodes[attack.child];
    memcpy(context->certificate[attackCertificate].initialRelevanceMask,
           andNode->relevanceMask, sizeof(andNode->relevanceMask));
    memcpy(context->certificate[attackCertificate].replyRelatedZoneMask,
           andNode->replyRelatedZoneMask,
           sizeof(andNode->replyRelatedZoneMask));
    memcpy(context->certificate[attackCertificate].relatedZoneMask,
           andNode->relatedZoneMask, sizeof(andNode->relatedZoneMask));
    memcpy(context->certificate[attackCertificate].verifiedOmissionMask,
           andNode->verifiedOmissionMask,
           sizeof(andNode->verifiedOmissionMask));
    if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                    attack.x, attack.y, context->attacker)) {
        context->certificate[attackCertificate].terminalWin = true;
        fc_proof_unmake_move(context, board, attack.x, attack.y);
        return true;
    }
    bool valid = andNode->type == FC_DFPN_AND_NODE &&
                 andNode->proofNumber == 0 &&
                 andNode->enumerationComplete;
    for (int i = 0; valid && i < andNode->edgeCount; i++) {
        FCDFPNEdge defense = session->graphEdges[andNode->firstEdge + i];
        FCDFPNNode *continuation = &session->graphNodes[defense.child];
        if (continuation->proofNumber != 0) {
            valid = false;
            break;
        }
        uint64_t defenseHash = fc_proof_board_key(
            context, (const int (*)[FC_BOARD_SIZE])board,
            -context->attacker);
        int defenseCertificate = fc_proof_append(
            context, defenseHash, attackCertificate,
            defense.x, defense.y, -context->attacker, false);
        if (defenseCertificate < 0 || !fc_proof_make_move(
                context, board, defense.x, defense.y,
                -context->attacker)) {
            valid = false;
            break;
        }
        valid = fc_dfpn_build_certificate(
            context, board, defense.child, defenseCertificate);
        fc_proof_unmake_move(context, board, defense.x, defense.y);
    }
    fc_proof_unmake_move(context, board, attack.x, attack.y);
    if (!valid) context->certificateCount = checkpoint;
    return valid;
}

static bool fc_dfpn_build_certificate(FCProofContext *context,
                                      int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                                      int nodeIndex,
                                      int parent)
{
    FCProofSession *session = context->session;
    FCDFPNNode *node = &session->graphNodes[nodeIndex];
    if (node->type != FC_DFPN_OR_NODE || node->proofNumber != 0 ||
        !node->relatedZoneComplete) return false;
    int checkpoint = context->certificateCount;
    int built = 0;
    bool retainIndependentContinuations =
        node->relatedZoneContinuationCount >= 2 &&
        node->relatedZonePointsRemoved > 0;
    for (int i = 0; i < node->edgeCount; i++) {
        int candidate = node->firstEdge + i;
        FCDFPNEdge *edge = &session->graphEdges[candidate];
        FCDFPNNode *child = &session->graphNodes[edge->child];
        if (child->proofNumber != 0 || !child->relatedZoneComplete)
            continue;
        bool duplicateGain = false;
        for (int previous = 0; previous < i; previous++) {
            FCDFPNEdge *other = &session->graphEdges[
                node->firstEdge + previous];
            FCDFPNNode *otherChild = &session->graphNodes[other->child];
            if (otherChild->proofNumber == 0 &&
                otherChild->relatedZoneComplete &&
                other->x == edge->x && other->y == edge->y) {
                duplicateGain = true;
                break;
            }
        }
        if (duplicateGain) continue;
        if (!retainIndependentContinuations &&
            candidate != node->selectedEdge && built > 0) continue;
        if (!fc_dfpn_build_attack_certificate(
                context, board, candidate, parent)) {
            context->certificateCount = checkpoint;
            return false;
        }
        built++;
        if (!retainIndependentContinuations) break;
    }
    if (built == 0) {
        context->certificateCount = checkpoint;
        return false;
    }
    if (retainIndependentContinuations &&
        built != node->relatedZoneContinuationCount) {
        context->certificateCount = checkpoint;
        return false;
    }
    return true;
}

static int fc_dfpn_prove(FCProofContext *context,
                         int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                         int remainingDepth,
                         int *distance,
                         uint64_t *proofNumber,
                         uint64_t *disproofNumber)
{
    int root = fc_dfpn_find_or_create(
        context, (const int (*)[FC_BOARD_SIZE])board,
        remainingDepth, FC_DFPN_OR_NODE, 1);
    context->dfpnRoot = root;
    if (root < 0) return FC_PROOF_UNKNOWN;
    fc_dfpn_search(context, board, root,
                   FC_PROOF_INFINITY, FC_PROOF_INFINITY);
    FCDFPNNode *node = &context->session->graphNodes[root];
    *proofNumber = node->proofNumber;
    *disproofNumber = node->disproofNumber;
    *distance = node->distance;
    if (context->aborted) return FC_PROOF_UNKNOWN;
    if (node->proofNumber == 0) {
        context->certificateCount = 0;
        if (!fc_dfpn_build_certificate(context, board, root, -1)) {
            context->certificateCount = 0;
            return FC_PROOF_UNKNOWN;
        }
        return FC_PROOF_PROVEN_WIN;
    }
    if (node->disproofNumber == 0 && node->enumerationComplete) {
        fcProofDiagnostics.completedScopeDisproofs++;
        return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }
    return FC_PROOF_UNKNOWN;
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
    int ownImmediate = context->position != NULL
        ? fc_incremental_count_immediate_wins(
            context->position, context->attacker, NULL, 0)
        : fc_count_immediate_wins(
            board, context->attacker, context->forbiddenBlack, NULL, 0);
    int defenderImmediate = context->position != NULL
        ? fc_incremental_count_immediate_wins(
            context->position, -context->attacker, NULL, 0)
        : fc_count_immediate_wins(
            board, -context->attacker, context->forbiddenBlack, NULL, 0);
    if (defenderImmediate > 0 && ownImmediate == 0) {
        if (proofNumber != NULL) *proofNumber = FC_PROOF_INFINITY;
        if (disproofNumber != NULL) *disproofNumber = 0;
        return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }

    uint64_t key = fc_proof_board_key(
        context, (const int (*)[FC_BOARD_SIZE])board, context->attacker);
    FCProofTTEntry *entry = NULL;
    uint64_t verification = fc_proof_board_verification(context, key);
    if (context->tableCapacity > 0) {
        entry = &context->table[key % context->tableCapacity];
        if (entry->generation == context->generation &&
            entry->key == key && entry->verification == verification &&
            entry->depth >= remainingDepth) {
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
    int threatCount = 0;
    if (parent == -1 && fcActiveRootThreat != NULL) {
        /* Single-gain root jobs bypass the all-root enumeration path. */
        threats[0] = *fcActiveRootThreat;
        threatCount = 1;
    } else {
        threatCount = context->position != NULL
            ? fc_incremental_enumerate_threats(
                context->position, context->attacker, context->searchClass,
                threats, FC_MAX_THREATS, &overflow)
            : fc_enumerate_threats(
                (const int (*)[FC_BOARD_SIZE])board, context->attacker,
                context->forbiddenBlack, context->searchClass,
                threats, FC_MAX_THREATS, &overflow);
    }
    bool sawUnknown = overflow;
    int bestDistance = 0;
    uint64_t rootProof = FC_PROOF_INFINITY;
    uint64_t rootDisproof = 0;
    int threatOrder[FC_MAX_THREATS];
    if (context->branchFirstEnabled && parent == -1) {
        fc_branch_first_order_threats(
            context, board, threats, threatCount, threatOrder);
    } else {
        fc_order_threats_by_dependency(
            threats, threatCount,
            context->session != NULL ? context->searchClass
                                     : FC_PROOF_SEARCH_NONE,
            threatOrder);
    }
    for (int orderIndex = 0; orderIndex < threatCount; orderIndex++) {
        int i = threatOrder[orderIndex];
        FCThreat *threat = &threats[i];
        int checkpoint = context->certificateCount;
        uint64_t beforeHash = fc_proof_board_key(
            context, (const int (*)[FC_BOARD_SIZE])board,
            context->attacker);
        int attackNode = fc_proof_append(
            context, beforeHash, parent, threat->gain.x, threat->gain.y,
            context->attacker, false);
        if (attackNode < 0) return FC_PROOF_UNKNOWN;
        if (!fc_proof_make_move(context, board, threat->gain.x,
                                threat->gain.y, context->attacker)) {
            context->certificateCount = checkpoint;
            continue;
        }
        if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                        threat->gain.x, threat->gain.y, context->attacker)) {
            context->certificate[attackNode].terminalWin = true;
            fc_proof_unmake_move(context, board,
                                 threat->gain.x, threat->gain.y);
            if (distance != NULL) *distance = 1;
            if (proofNumber != NULL) *proofNumber = 0;
            if (disproofNumber != NULL) *disproofNumber = FC_PROOF_INFINITY;
            return FC_PROOF_PROVEN_WIN;
        }

        FCPoint refutations[FC_BOARD_SIZE * FC_BOARD_SIZE];
        int refutationCount = fc_generate_refutations_internal(
            board, context->attacker, context->forbiddenBlack,
            context->searchClass, refutations,
            FC_BOARD_SIZE * FC_BOARD_SIZE, context->position,
            threat->certificateZoneMask, threat->severity,
            context->certificate[attackNode].replyRelatedZoneMask,
            context->certificate[attackNode].verifiedOmissionMask);
        memcpy(context->certificate[attackNode].initialRelevanceMask,
               threat->certificateZoneMask,
               sizeof(threat->certificateZoneMask));
        memcpy(context->certificate[attackNode].relatedZoneMask,
               context->certificate[attackNode].replyRelatedZoneMask,
               sizeof(context->certificate[attackNode].relatedZoneMask));
        if (refutationCount < 0) {
            fc_proof_unmake_move(context, board,
                                 threat->gain.x, threat->gain.y);
            context->certificateCount = checkpoint;
            context->aborted = true;
            return FC_PROOF_UNKNOWN;
        }
        bool branchWaveEligible = context->branchFirstEnabled &&
            !context->branchWaveDispatched && parent != -1 &&
            remainingDepth >= context->branchFirstMinRemainingDepth &&
            refutationCount >= context->branchFirstMinBranchCount &&
            (context->branchFirstMaxBranches <= 0 ||
             refutationCount <= context->branchFirstMaxBranches) &&
            fc_effective_worker_count(fcActiveBranchFirstProfile) > 1 &&
            !fcWorkerPoolTaskActive && !fc_decision_deadline_reached();
        if (branchWaveEligible) {
            FCBranchWaveSummary branchSummary;
            memset(&branchSummary, 0, sizeof(branchSummary));
            context->branchWaveDispatched = true;
            if (fc_branch_first_try_wave(
                    context, board, refutations, refutationCount,
                    remainingDepth - 2, attackNode, threat->severity,
                    &branchSummary)) {
                if (branchSummary.allProven) {
                    bestDistance = refutationCount == 0 ? 3
                        : branchSummary.maximumChildDistance + 2;
                    fc_proof_unmake_move(context, board,
                                         threat->gain.x, threat->gain.y);
                    if (distance != NULL) *distance = bestDistance;
                    if (proofNumber != NULL) *proofNumber = 0;
                    if (disproofNumber != NULL)
                        *disproofNumber = FC_PROOF_INFINITY;
                    return FC_PROOF_PROVEN_WIN;
                }
                rootProof = branchSummary.edgeProof < rootProof
                    ? branchSummary.edgeProof : rootProof;
                rootDisproof = fc_proof_saturated_add(
                    rootDisproof, branchSummary.edgeDisproof);
                sawUnknown = sawUnknown || branchSummary.sawUnknown;
                fc_proof_unmake_move(context, board,
                                     threat->gain.x, threat->gain.y);
                context->certificateCount = checkpoint;
                if (context->aborted || context->certificateOverflow)
                    return FC_PROOF_UNKNOWN;
                continue;
            }
        }
        bool allProven = true;
        int maximumChildDistance = 0;
        uint64_t edgeProof = 0;
        uint64_t edgeDisproof = FC_PROOF_INFINITY;
        for (int r = 0; r < refutationCount; r++) {
            FCPoint reply = refutations[r];
            uint64_t replyHash = fc_proof_board_key(
                context, (const int (*)[FC_BOARD_SIZE])board,
                -context->attacker);
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
            if (!fc_proof_make_move(context, board, reply.x, reply.y,
                                    -context->attacker)) {
                allProven = false;
                break;
            }
            if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                            reply.x, reply.y, -context->attacker)) {
                fc_proof_unmake_move(context, board, reply.x, reply.y);
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
            fc_proof_unmake_move(context, board, reply.x, reply.y);
            edgeProof = fc_proof_saturated_add(edgeProof, childProof);
            if (childDisproof < edgeDisproof)
                edgeDisproof = childDisproof;
            if (child != FC_PROOF_PROVEN_WIN) {
                if (child == FC_PROOF_UNKNOWN) sawUnknown = true;
                allProven = false;
                break;
            }
            fc_bitset_set(
                context->certificate[attackNode].relatedZoneMask,
                reply.x * FC_BOARD_SIZE + reply.y);
            for (int certificateIndex = defenseNode + 1;
                 certificateIndex < context->certificateCount;
                 certificateIndex++) {
                if (context->certificate[certificateIndex].parent ==
                    defenseNode) {
                    fc_threat_mask_or(
                        context->certificate[attackNode].relatedZoneMask,
                        context->certificate[certificateIndex]
                            .relatedZoneMask);
                }
            }
            if (childDistance > maximumChildDistance)
                maximumChildDistance = childDistance;
        }
        fc_proof_unmake_move(context, board,
                             threat->gain.x, threat->gain.y);
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
        entry->verification = verification;
        entry->depth = remainingDepth;
        entry->generation = context->generation;
        entry->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    }
    return sawUnknown || context->aborted
        ? FC_PROOF_UNKNOWN : FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
}

static int fc_branch_first_preview_score(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    const FCThreat *threat,
    bool *complete)
{
    if (complete != NULL) *complete = true;
    if (context == NULL || board == NULL || threat == NULL) {
        if (complete != NULL) *complete = false;
        return INT_MIN;
    }
    if (fc_global_proof_deadline_reached()) {
        if (complete != NULL) *complete = false;
        return INT_MIN;
    }
    int preview[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(preview, board, sizeof(preview));
    if (!fc_is_legal_move((const int (*)[FC_BOARD_SIZE])preview,
                          threat->gain.x, threat->gain.y,
                          context->attacker, context->forbiddenBlack)) {
        if (complete != NULL) *complete = false;
        return INT_MIN;
    }
    preview[threat->gain.x][threat->gain.y] = context->attacker;
    int ownImmediate = fc_count_local_immediate_wins(
        preview, threat->gain.x, threat->gain.y, context->attacker,
        context->forbiddenBlack, NULL, 0);
    int advancedThree = fc_count_local_open_four_creators(
        preview, threat->gain.x, threat->gain.y, context->attacker,
        context->forbiddenBlack, NULL, FC_MAX_THREAT_POINTS, NULL);
    int opponentImmediate = fc_count_immediate_wins(
        preview, -context->attacker, context->forbiddenBlack, NULL, 0);
    int dependencyPoints = (int)fc_threat_mask_count(
        threat->certificateZoneMask);
    int score = threat->severity * 1000000;
    score += ownImmediate * 20000;
    score += threat->costCount * 4000;
    score += advancedThree * 2500;
    score += threat->restCount * 1000;
    score += dependencyPoints * 8;
    score -= opponentImmediate * 16000;
    if (context->branchFirstPreviewDepth > 1) {
        int shallowReplies = fc_count_local_immediate_wins(
            preview, threat->gain.x, threat->gain.y, -context->attacker,
            context->forbiddenBlack, NULL, 0);
        score -= shallowReplies * 1200;
    }
    if (fc_global_proof_deadline_reached() && complete != NULL)
        *complete = false;
    return score;
}

static void fc_branch_first_order_threats(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    FCThreat *threats,
    int threatCount,
    int *order)
{
    if (order == NULL || threatCount <= 0) return;
    fc_order_threats_by_dependency(
        threats, threatCount, context != NULL ? context->searchClass
                                              : FC_PROOF_SEARCH_NONE,
        order);
    if (context == NULL || !context->branchFirstEnabled) return;
    int scores[FC_MAX_THREATS];
    bool complete = true;
    for (int index = 0; index < threatCount; index++) {
        int threatIndex = order[index];
        bool previewComplete = true;
        scores[threatIndex] = fc_branch_first_preview_score(
            context, board, &threats[threatIndex], &previewComplete);
        fcProofDiagnostics.branchFirstPreviewBranches++;
        if (threats[threatIndex].severity == FC_THREAT_FOUR_THREE ||
            threats[threatIndex].severity == FC_THREAT_OPEN_FOUR ||
            threats[threatIndex].severity == FC_THREAT_FOUR)
            fcProofDiagnostics.branchFirstAdvancedFourPreviews++;
        if (threats[threatIndex].severity == FC_THREAT_OPEN_THREE)
            fcProofDiagnostics.branchFirstAdvancedThreePreviews++;
        if (!previewComplete) complete = false;
        if (!complete) break;
    }
    if (!complete) {
        fcProofDiagnostics.branchFirstPreviewIncomplete++;
        return;
    }
    /* The dependency order is already canonical.  Stable insertion sorting
     * keeps equal preview scores reproducible without coordinate races. */
    for (int i = 1; i < threatCount; i++) {
        int value = order[i];
        int j = i;
        while (j > 0 && scores[order[j - 1]] < scores[value]) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = value;
    }
}

static bool fc_branch_first_is_advanced_four(int severity)
{
    return severity == FC_THREAT_FOUR_THREE ||
           severity == FC_THREAT_FOUR ||
           severity == FC_THREAT_OPEN_FOUR;
}

static bool fc_branch_first_is_advanced_three(int severity)
{
    return severity == FC_THREAT_OPEN_THREE;
}

static int fc_branch_first_child_depth(
    const FCAIProfile *profile,
    int baseDepth,
    int attackSeverity)
{
    if (profile == NULL || baseDepth <= 0) return baseDepth;
    int bonus = fc_branch_first_is_advanced_four(attackSeverity)
        ? profile->branchFirstAdvancedFourDepthBonus
        : fc_branch_first_is_advanced_three(attackSeverity)
        ? profile->branchFirstAdvancedThreeDepthBonus : 0;
    if (bonus <= 0) return baseDepth;
    int depth = baseDepth;
    if (bonus > INT_MAX - depth) depth = INT_MAX;
    else depth += bonus;
    int cap = profile->branchFirstTacticalDepthCap;
    if (cap > baseDepth && depth > cap) depth = cap;
    return depth;
}

typedef struct FCBranchFirstBatch FCBranchFirstBatch;

typedef struct {
    FCBranchFirstBatch *batch;
    FCProofDiagnostics diagnostics;
} FCBranchFirstWorker;

struct FCBranchFirstBatch {
    const int (*board)[FC_BOARD_SIZE];
    int attacker;
    bool forbiddenBlack;
    int searchClass;
    int childDepth;
    uint64_t perJobNodeBudget;
    uint64_t aggregateNodeBudget;
    uint32_t perJobTimeBudgetMs;
    size_t transpositionCapacity;
    FCAIProfile profile;
    FCDecisionLedger *ledger;
    const FCPoint *replies;
    int replyCount;
    FCProofResult *results;
    bool *completed;
    _Atomic int nextJob;
    _Atomic int completedJobs;
    _Atomic int activeWorkers;
    _Atomic int maxConcurrentWorkers;
    _Atomic uint64_t consumedNodeTokens;
    double absoluteDeadlineMilliseconds;
};

static void fc_branch_first_initialize_unknown(
    FCProofResult *result,
    int searchClass,
    int childDepth)
{
    if (result == NULL) return;
    memset(result, 0, sizeof(*result));
    result->x = -1;
    result->y = -1;
    result->status = FC_PROOF_UNKNOWN;
    result->searchClass = searchClass;
    result->completedDepth = childDepth;
    result->budgetExhausted = true;
}

static void *fc_branch_first_worker_main(void *opaque)
{
    FCBranchFirstWorker *worker = opaque;
    if (worker == NULL || worker->batch == NULL) return NULL;
    FCBranchFirstBatch *batch = worker->batch;
    double priorDeadline = fcActiveDecisionDeadlineMilliseconds;
    const FCAIProfile *priorBranchProfile = fcActiveBranchFirstProfile;
    int priorFilterX = fcProofRootFilterX;
    int priorFilterY = fcProofRootFilterY;
    const FCThreat *priorRootThreat = fcActiveRootThreat;
    FCDecisionLedger *priorLedger = fcActiveDecisionLedger;
    _Atomic uint64_t *priorParallelCounter = fcActiveParallelNodeCounter;
    uint64_t priorParallelBudget = fcActiveParallelNodeBudget;
    uint64_t priorParallelTokens = fcParallelNodeTokensRemaining;
    uint64_t priorLedgerTokens = fcDecisionLedgerNodeTokensRemaining;
    uint32_t priorBlockSize = fcActiveParallelNodeBlockSize;
    bool priorBlocksEnabled = fcActiveParallelTokenBlocksEnabled;
    FCProofSession *priorSession = fcActiveProofSession;
    fc_proof_diagnostics_reset();
    fcActiveBranchFirstProfile = NULL;
    fcProofRootFilterX = -1;
    fcProofRootFilterY = -1;
    fcActiveRootThreat = NULL;
    fcActiveDecisionDeadlineMilliseconds = batch->absoluteDeadlineMilliseconds;
    fcActiveDecisionLedger = batch->ledger;
    fcActiveParallelNodeCounter = &batch->consumedNodeTokens;
    fcActiveParallelNodeBudget = batch->aggregateNodeBudget;
    fcParallelNodeTokensRemaining = 0;
    fcDecisionLedgerNodeTokensRemaining = 0;
    fcActiveParallelNodeBlockSize = batch->profile.parallelTokenBlockSize;
    fcActiveParallelTokenBlocksEnabled =
        batch->profile.parallelTokenBlockEnabled;
    int active = atomic_fetch_add_explicit(
        &batch->activeWorkers, 1, memory_order_acq_rel) + 1;
    int observed = atomic_load_explicit(
        &batch->maxConcurrentWorkers, memory_order_relaxed);
    while (active > observed &&
           !atomic_compare_exchange_weak_explicit(
               &batch->maxConcurrentWorkers, &observed, active,
               memory_order_relaxed, memory_order_relaxed)) {
        /* The failed compare-exchange refreshes observed. */
    }

    FCAIProfile jobProfile = batch->profile;
    jobProfile.parallelProofEnabled = false;
    jobProfile.persistentWorkerPoolEnabled = false;
    jobProfile.branchFirstSearchEnabled = false;
    jobProfile.proofWorkerCount = 1;
    jobProfile.proofParallelNodeBudget = 0;
    jobProfile.proofMaxDepth = batch->childDepth;
    jobProfile.proofNodeBudget = batch->perJobNodeBudget;
    jobProfile.proofTimeBudgetMs = batch->perJobTimeBudgetMs;
    jobProfile.proofEmergencyTimeBudgetMs = batch->perJobTimeBudgetMs;
    FCProofSession session;
    FCProofSession *previousSession = fcActiveProofSession;
    bool sessionReady = fc_proof_session_begin(
        &session, &jobProfile, batch->board, batch->forbiddenBlack);
    if (!sessionReady) {
        for (;;) {
            int index = atomic_fetch_add_explicit(
                &batch->nextJob, 1, memory_order_relaxed);
            if (index >= batch->replyCount) break;
            fc_branch_first_initialize_unknown(
                &batch->results[index], batch->searchClass,
                batch->childDepth);
            batch->completed[index] = true;
            atomic_fetch_add_explicit(
                &batch->completedJobs, 1, memory_order_relaxed);
        }
    } else {
        for (;;) {
            if (fc_decision_deadline_reached()) {
                fcProofDiagnostics.branchFirstDeadlineStops++;
                break;
            }
            int index = atomic_fetch_add_explicit(
                &batch->nextJob, 1, memory_order_relaxed);
            if (index >= batch->replyCount) break;
            FCProofResult *result = &batch->results[index];
            fc_branch_first_initialize_unknown(
                result, batch->searchClass, batch->childDepth);
            int alternative[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(alternative, batch->board, sizeof(alternative));
            if (!fc_make_move(alternative, batch->replies[index].x,
                              batch->replies[index].y,
                              -batch->attacker, batch->forbiddenBlack)) {
                batch->completed[index] = true;
                atomic_fetch_add_explicit(
                    &batch->completedJobs, 1, memory_order_relaxed);
                continue;
            }
            if (fc_has_five(
                    (const int (*)[FC_BOARD_SIZE])alternative,
                    batch->replies[index].x, batch->replies[index].y,
                    -batch->attacker)) {
                result->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
                result->proofNumber = FC_PROOF_INFINITY;
                result->disproofNumber = 0;
                result->budgetExhausted = false;
                batch->completed[index] = true;
                atomic_fetch_add_explicit(
                    &batch->completedJobs, 1, memory_order_relaxed);
                continue;
            }
            if (!fc_proof_session_reset_for_query(
                    &session, &jobProfile,
                    (const int (*)[FC_BOARD_SIZE])alternative,
                    batch->forbiddenBlack)) {
                fcProofDiagnostics.branchFirstDeadlineStops++;
                break;
            }
            session.nodeBudget = batch->perJobNodeBudget;
            (void)fc_prove_forced_win(
                (const int (*)[FC_BOARD_SIZE])alternative,
                batch->attacker, batch->forbiddenBlack,
                batch->searchClass, batch->childDepth,
                batch->perJobNodeBudget, batch->perJobTimeBudgetMs,
                batch->transpositionCapacity, result);
            fc_parallel_node_token_flush();
            batch->completed[index] = true;
            atomic_fetch_add_explicit(
                &batch->completedJobs, 1, memory_order_relaxed);
        }
        fc_proof_session_end(&session, previousSession);
    }
    fc_parallel_node_token_flush();
    worker->diagnostics = fc_proof_diagnostics_get();
    fcActiveProofSession = priorSession;
    fcActiveBranchFirstProfile = priorBranchProfile;
    fcProofRootFilterX = priorFilterX;
    fcProofRootFilterY = priorFilterY;
    fcActiveRootThreat = priorRootThreat;
    fcActiveDecisionDeadlineMilliseconds = priorDeadline;
    fcActiveParallelNodeCounter = priorParallelCounter;
    fcActiveParallelNodeBudget = priorParallelBudget;
    fcActiveDecisionLedger = priorLedger;
    fcParallelNodeTokensRemaining = priorParallelTokens;
    fcDecisionLedgerNodeTokensRemaining = priorLedgerTokens;
    fcActiveParallelNodeBlockSize = priorBlockSize;
    fcActiveParallelTokenBlocksEnabled = priorBlocksEnabled;
    atomic_fetch_sub_explicit(&batch->activeWorkers, 1, memory_order_acq_rel);
    return NULL;
}

typedef struct {
    FCBranchFirstBatch *batch;
    FCBranchFirstWorker *workers;
} FCBranchFirstPoolTask;

static void fc_branch_first_pool_task(void *opaque, int workerSlot)
{
    FCBranchFirstPoolTask *task = opaque;
    if (task == NULL || task->batch == NULL || task->workers == NULL ||
        workerSlot < 0 || workerSlot >= FC_PARALLEL_MAX_WORKERS) return;
    task->workers[workerSlot].batch = task->batch;
    (void)fc_branch_first_worker_main(&task->workers[workerSlot]);
}

static bool fc_branch_first_merge_certificate(
    FCProofContext *context,
    const FCProofResult *result,
    int defenseNode,
    const int branchBoard[FC_BOARD_SIZE][FC_BOARD_SIZE],
    uint64_t outRelatedZone[FC_POSITION_BITSET_WORDS])
{
    if (context == NULL || result == NULL ||
        branchBoard == NULL ||
        result->certificateNodeCount <= 0 ||
        result->certificateNodeCount > FC_MAX_PROOF_NODES ||
        !result->certificateVerified) return false;
    int remap[FC_MAX_PROOF_NODES];
    memset(remap, 0xff, sizeof(remap));
    for (int i = 0; i < result->certificateNodeCount; i++) {
        const FCProofNode *source = &result->certificate[i];
        int parent = source->parent < 0 ? defenseNode
                    : source->parent < result->certificateNodeCount
                    ? remap[source->parent] : -1;
        if (parent < defenseNode ||
            (source->parent >= 0 && remap[source->parent] < defenseNode))
            return false;
        /* Worker certificates are hashed with their private child depth.
         * The merged certificate is verified with the coordinator's root
         * depth, so replay the source path and normalize every hash to the
         * parent context instead of weakening the verifier. */
        int path[FC_MAX_PROOF_NODES];
        int pathCount = 0;
        int current = i;
        while (current >= 0 && current < result->certificateNodeCount &&
               pathCount < FC_MAX_PROOF_NODES) {
            path[pathCount++] = current;
            int next = result->certificate[current].parent;
            if (next >= current) return false;
            current = next;
        }
        if (current >= 0 || pathCount <= 0) return false;
        int replay[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(replay, branchBoard, sizeof(replay));
        for (int pathIndex = pathCount - 1; pathIndex >= 0; pathIndex--) {
            const FCProofNode *pathNode =
                &result->certificate[path[pathIndex]];
            if (!fc_make_move(replay, pathNode->x, pathNode->y,
                              pathNode->side, context->forbiddenBlack))
                return false;
        }
        memcpy(replay, branchBoard, sizeof(replay));
        for (int pathIndex = pathCount - 1; pathIndex >= 0; pathIndex--) {
            const FCProofNode *pathNode =
                &result->certificate[path[pathIndex]];
            uint64_t boardHash = fc_board_key(
                (const int (*)[FC_BOARD_SIZE])replay, pathNode->side,
                context->forbiddenBlack, context->searchClass,
                fc_proof_key_version(context->maxDepth));
            if (!fc_make_move(replay, pathNode->x, pathNode->y,
                              pathNode->side, context->forbiddenBlack))
                return false;
            if (path[pathIndex] == i) {
                /* The node's pre-move board is the state at this point. */
                source = &result->certificate[i];
                int index = fc_proof_append(
                    context, boardHash, parent,
                    source->x, source->y, source->side,
                    source->terminalWin);
                if (index < 0) return false;
                context->certificate[index] = *source;
                context->certificate[index].boardHash = boardHash;
                context->certificate[index].parent = parent;
                remap[i] = index;
                break;
            }
        }
    }
    if (outRelatedZone != NULL) {
        memcpy(outRelatedZone,
               result->certificate[0].relatedZoneMask,
               sizeof(result->certificate[0].relatedZoneMask));
    }
    return true;
}

static bool fc_branch_first_try_wave(
    FCProofContext *context,
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    const FCPoint *refutations,
    int refutationCount,
    int childDepth,
    int attackNode,
    int attackSeverity,
    FCBranchWaveSummary *summary)
{
    if (summary != NULL) memset(summary, 0, sizeof(*summary));
    if (context == NULL || board == NULL || refutations == NULL ||
        refutationCount <= 0 || summary == NULL ||
        fcActiveBranchFirstProfile == NULL || fcWorkerPoolTaskActive) {
        return false;
    }
    const FCAIProfile *profile = fcActiveBranchFirstProfile;
    int workerCount = fc_effective_worker_count(profile);
    if (workerCount <= 1 || fc_decision_deadline_reached()) {
        fcProofDiagnostics.branchFirstDispatchFallbacks++;
        return false;
    }
    if (workerCount > refutationCount) workerCount = refutationCount;
    uint64_t aggregateBudget = profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
    uint64_t perJobBudget = profile->proofNodeBudget > 0
        ? profile->proofNodeBudget : aggregateBudget;
    if (aggregateBudget == 0) aggregateBudget = UINT64_MAX;
    if (perJobBudget == 0 || perJobBudget > aggregateBudget)
        perJobBudget = aggregateBudget;
    int effectiveChildDepth = fc_branch_first_child_depth(
        profile, childDepth, attackSeverity);
    uint32_t perJobTime = profile->proofTimeBudgetMs;
    if (perJobTime == 0 && profile->proofEmergencyTimeBudgetMs > 0)
        perJobTime = profile->proofEmergencyTimeBudgetMs;
    double deadline = fcActiveDecisionDeadlineMilliseconds;
    if (deadline <= 0.0 && fcActiveDecisionLedger != NULL)
        deadline = fcActiveDecisionLedger->internalDeadlineMilliseconds;
    if (deadline <= 0.0 && perJobTime > 0)
        deadline = fc_now_milliseconds() + (double)perJobTime;

    FCProofResult *results = calloc(
        (size_t)refutationCount, sizeof(*results));
    bool *completed = calloc((size_t)refutationCount, sizeof(*completed));
    FCBranchFirstWorker *workers = calloc(
        (size_t)workerCount, sizeof(*workers));
    if (results == NULL || completed == NULL || workers == NULL) {
        free(results);
        free(completed);
        free(workers);
        fcProofDiagnostics.branchFirstDispatchFallbacks++;
        return false;
    }

    int defenseNodes[FC_BOARD_SIZE * FC_BOARD_SIZE];
    for (int index = 0; index < refutationCount; index++) {
        uint64_t replyHash = fc_proof_board_key(
            context, (const int (*)[FC_BOARD_SIZE])board,
            -context->attacker);
        defenseNodes[index] = fc_proof_append(
            context, replyHash, attackNode,
            refutations[index].x, refutations[index].y,
            -context->attacker, false);
        if (defenseNodes[index] < 0) {
            free(results);
            free(completed);
            free(workers);
            summary->attempted = true;
            summary->allProven = false;
            summary->sawUnknown = true;
            summary->edgeProof = 1;
            summary->edgeDisproof = 1;
            return true;
        }
    }

    FCBranchFirstBatch batch;
    memset(&batch, 0, sizeof(batch));
    batch.board = (const int (*)[FC_BOARD_SIZE])board;
    batch.attacker = context->attacker;
    batch.forbiddenBlack = context->forbiddenBlack;
    batch.searchClass = context->searchClass;
    batch.childDepth = effectiveChildDepth;
    batch.perJobNodeBudget = perJobBudget;
    batch.aggregateNodeBudget = aggregateBudget;
    batch.perJobTimeBudgetMs = perJobTime;
    batch.transpositionCapacity = profile->proofTranspositionCapacity;
    batch.profile = *profile;
    batch.ledger = fcActiveDecisionLedger;
    batch.replies = refutations;
    batch.replyCount = refutationCount;
    batch.results = results;
    batch.completed = completed;
    atomic_init(&batch.nextJob, 0);
    atomic_init(&batch.completedJobs, 0);
    atomic_init(&batch.activeWorkers, 0);
    atomic_init(&batch.maxConcurrentWorkers, 0);
    atomic_init(&batch.consumedNodeTokens, 0);
    batch.absoluteDeadlineMilliseconds = deadline;
    FCBranchFirstPoolTask poolTask = {.batch = &batch, .workers = workers};
    int launched = fc_worker_pool_dispatch(
        fc_branch_first_pool_task, &poolTask, workerCount);
    if (launched <= 0) {
        fcProofDiagnostics.branchFirstDispatchFallbacks++;
        free(results);
        free(completed);
        free(workers);
        return false;
    }
    if (effectiveChildDepth > childDepth) {
        fcProofDiagnostics.branchFirstDepthExtensions++;
        if (fc_branch_first_is_advanced_four(attackSeverity))
            fcProofDiagnostics.branchFirstAdvancedFourDepthExtensions++;
        else if (fc_branch_first_is_advanced_three(attackSeverity))
            fcProofDiagnostics.branchFirstAdvancedThreeDepthExtensions++;
    }
    if ((uint64_t)effectiveChildDepth >
        fcProofDiagnostics.branchFirstMaxChildDepth)
        fcProofDiagnostics.branchFirstMaxChildDepth =
            (uint64_t)effectiveChildDepth;
    fcProofDiagnostics.branchFirstWaves++;
    fcProofDiagnostics.branchFirstWorkersLaunched += (uint64_t)launched;
    fcProofDiagnostics.branchFirstJobs += (uint64_t)refutationCount;
    fcDecisionWorkersLaunched += launched;
    fcDecisionParallelJobs += refutationCount;
    for (int index = 0; index < launched; index++) {
        fc_proof_diagnostics_merge(
            &fcProofDiagnostics, &workers[index].diagnostics);
    }
    uint64_t maximumConcurrent = (uint64_t)atomic_load_explicit(
        &batch.maxConcurrentWorkers, memory_order_relaxed);
    if (maximumConcurrent > fcProofDiagnostics.branchFirstMaxConcurrentWorkers)
        fcProofDiagnostics.branchFirstMaxConcurrentWorkers = maximumConcurrent;
    int completedJobs = (int)atomic_load_explicit(
        &batch.completedJobs, memory_order_relaxed);
    fcProofDiagnostics.branchFirstJobsCompleted += (uint64_t)completedJobs;
    fcDecisionParallelJobsCompleted += completedJobs;

    summary->attempted = true;
    summary->allProven = true;
    summary->edgeProof = 0;
    summary->edgeDisproof = FC_PROOF_INFINITY;
    for (int index = 0; index < refutationCount; index++) {
        FCProofResult *result = &results[index];
        if (!completed[index]) {
            summary->allProven = false;
            summary->sawUnknown = true;
            summary->edgeProof = fc_proof_saturated_add(
                summary->edgeProof, 1);
            if (summary->edgeDisproof > 1) summary->edgeDisproof = 1;
            continue;
        }
        if (result->status == FC_PROOF_PROVEN_WIN &&
            result->certificateVerified) {
            fcProofDiagnostics.branchFirstVerifiedJobs++;
            uint64_t relatedZone[FC_POSITION_BITSET_WORDS] = {0};
            int branchBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(branchBoard, board, sizeof(branchBoard));
            if (!fc_make_move(branchBoard, refutations[index].x,
                              refutations[index].y, -context->attacker,
                              context->forbiddenBlack)) {
                fcProofDiagnostics.branchFirstMergeFailures++;
                summary->allProven = false;
                summary->sawUnknown = true;
                summary->edgeProof = fc_proof_saturated_add(
                    summary->edgeProof, 1);
                if (summary->edgeDisproof > 1) summary->edgeDisproof = 1;
                continue;
            }
            if (!fc_branch_first_merge_certificate(
                    context, result, defenseNodes[index],
                    (const int (*)[FC_BOARD_SIZE])branchBoard,
                    relatedZone)) {
                fcProofDiagnostics.branchFirstMergeFailures++;
                summary->allProven = false;
                summary->sawUnknown = true;
                summary->edgeProof = fc_proof_saturated_add(
                    summary->edgeProof, 1);
                if (summary->edgeDisproof > 1) summary->edgeDisproof = 1;
                continue;
            }
            fc_bitset_set(context->certificate[attackNode].relatedZoneMask,
                          refutations[index].x * FC_BOARD_SIZE +
                          refutations[index].y);
            fc_threat_mask_or(
                context->certificate[attackNode].relatedZoneMask,
                relatedZone);
            fcProofDiagnostics.branchFirstUsefulJobs++;
            if (result->distance > summary->maximumChildDistance)
                summary->maximumChildDistance = result->distance;
            continue;
        }
        summary->allProven = false;
        fcProofDiagnostics.branchFirstUnknownJobs++;
        if (result->status == FC_PROOF_UNKNOWN || result->budgetExhausted)
            summary->sawUnknown = true;
        uint64_t proof = result->proofNumber > 0
            ? result->proofNumber : 1;
        uint64_t disproof = result->disproofNumber > 0
            ? result->disproofNumber : 1;
        summary->edgeProof = fc_proof_saturated_add(
            summary->edgeProof, proof);
        if (disproof < summary->edgeDisproof)
            summary->edgeDisproof = disproof;
    }
    if (fc_decision_deadline_reached()) {
        summary->sawUnknown = true;
        fcProofDiagnostics.branchFirstDeadlineStops++;
    }
    free(results);
    free(completed);
    free(workers);
    return true;
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
        for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
            value ^= fc_mix64(
                nodes[i].initialRelevanceMask[word] ^
                (nodes[i].replyRelatedZoneMask[word] << 1) ^
                (nodes[i].relatedZoneMask[word] << 2) ^
                (nodes[i].verifiedOmissionMask[word] << 3) ^
                ((uint64_t)(word + 1) << 56));
        }
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

static bool fc_verify_or_children(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int parent,
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *result,
    uint64_t outRelatedZone[FC_POSITION_BITSET_WORDS]);

static bool fc_verify_attack_node(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int nodeIndex,
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *result,
    uint64_t outRelatedZone[FC_POSITION_BITSET_WORDS])
{
    const FCProofNode *node = &result->certificate[nodeIndex];
    if (node->side != attacker ||
        node->boardHash != fc_board_key(
            (const int (*)[FC_BOARD_SIZE])board, attacker, forbiddenBlack,
            result->searchClass,
            fc_proof_key_version(result->completedDepth)) ||
        !fc_make_move(board, node->x, node->y, attacker, forbiddenBlack)) {
        return false;
    }
    bool wins = fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                            node->x, node->y, attacker);
    if (node->terminalWin) {
        bool zoneValid = fc_threat_mask_equal(
            node->initialRelevanceMask, node->relatedZoneMask);
        fc_unmake_move(board, node->x, node->y);
        if (zoneValid && outRelatedZone != NULL)
            memcpy(outRelatedZone, node->relatedZoneMask,
                   sizeof(node->relatedZoneMask));
        return wins && zoneValid;
    }
    if (wins) {
        fc_unmake_move(board, node->x, node->y);
        return false;
    }
    FCPoint expected[FC_BOARD_SIZE * FC_BOARD_SIZE];
    uint64_t replayedRelatedZone[FC_POSITION_BITSET_WORDS] = {0};
    uint64_t replayedOmissions[FC_POSITION_BITSET_WORDS] = {0};
    int expectedCount = fc_generate_refutations_internal(
        board, attacker, forbiddenBlack, result->searchClass,
        expected, FC_BOARD_SIZE * FC_BOARD_SIZE, NULL,
        node->initialRelevanceMask, FC_THREAT_NONE,
        replayedRelatedZone, replayedOmissions);
    if (expectedCount < 0 ||
        memcmp(replayedRelatedZone, node->replyRelatedZoneMask,
               sizeof(replayedRelatedZone)) != 0 ||
        memcmp(replayedOmissions, node->verifiedOmissionMask,
               sizeof(replayedOmissions)) != 0) {
        fc_unmake_move(board, node->x, node->y);
        return false;
    }
    uint64_t derivedRelatedZone[FC_POSITION_BITSET_WORDS];
    memcpy(derivedRelatedZone, node->replyRelatedZoneMask,
           sizeof(derivedRelatedZone));
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
                fc_proof_key_version(result->completedDepth))) {
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
        uint64_t continuationZone[FC_POSITION_BITSET_WORDS] = {0};
        bool valid = fc_verify_or_children(
            board, found, attacker, forbiddenBlack, result,
            continuationZone);
        fc_unmake_move(board, defense->x, defense->y);
        if (!valid) {
            fc_unmake_move(board, node->x, node->y);
            return false;
        }
        fc_bitset_set(derivedRelatedZone,
                      defense->x * FC_BOARD_SIZE + defense->y);
        fc_threat_mask_or(derivedRelatedZone, continuationZone);
    }
    bool zoneValid = fc_threat_mask_equal(
        derivedRelatedZone, node->relatedZoneMask);
    fc_unmake_move(board, node->x, node->y);
    if (zoneValid && outRelatedZone != NULL)
        memcpy(outRelatedZone, derivedRelatedZone,
               sizeof(derivedRelatedZone));
    return zoneValid;
}

static bool fc_verify_or_children(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int parent,
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *result,
    uint64_t outRelatedZone[FC_POSITION_BITSET_WORDS])
{
    int children[FC_MAX_PROOF_NODES];
    int childCount = fc_direct_children(
        result, parent, children, FC_MAX_PROOF_NODES);
    if (childCount <= 0) return false;
    bool haveZone = false;
    int previousX[FC_MAX_THREATS];
    int previousY[FC_MAX_THREATS];
    int uniqueCount = 0;
    uint64_t intersection[FC_POSITION_BITSET_WORDS] = {0};
    for (int c = 0; c < childCount; c++) {
        const FCProofNode *child = &result->certificate[children[c]];
        bool duplicate = false;
        for (int i = 0; i < uniqueCount; i++) {
            if (previousX[i] == child->x && previousY[i] == child->y) {
                duplicate = true;
                break;
            }
        }
        if (duplicate || uniqueCount >= FC_MAX_THREATS) return false;
        previousX[uniqueCount] = child->x;
        previousY[uniqueCount] = child->y;
        uniqueCount++;
        uint64_t childZone[FC_POSITION_BITSET_WORDS] = {0};
        if (!fc_verify_attack_node(board, children[c], attacker,
                                   forbiddenBlack, result, childZone))
            return false;
        if (!haveZone) {
            memcpy(intersection, childZone, sizeof(intersection));
            haveZone = true;
        } else {
            fc_threat_mask_and(intersection, childZone);
        }
    }
    if (!haveZone) return false;
    if (outRelatedZone != NULL)
        memcpy(outRelatedZone, intersection, sizeof(intersection));
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
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    uint64_t rootZone[FC_POSITION_BITSET_WORDS] = {0};
    bool valid = fc_verify_or_children(
        mutableBoard, -1, attacker, forbiddenBlack, result, rootZone);
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
    context.queryNodeBudget = nodeBudget;
    context.timeBudgetMs = timeBudgetMs;
    context.queryTimeBudgetMs = timeBudgetMs;
    context.startedMilliseconds = fc_now_milliseconds();
    context.session = fcActiveProofSession;
    context.dfpnRoot = -1;
    context.branchFirstEnabled = fcActiveBranchFirstProfile != NULL &&
        fcActiveBranchFirstProfile->branchFirstSearchEnabled;
    context.branchFirstMinRemainingDepth = context.branchFirstEnabled
        ? fcActiveBranchFirstProfile->branchFirstMinRemainingDepth : 0;
    context.branchFirstMinBranchCount = context.branchFirstEnabled
        ? fcActiveBranchFirstProfile->branchFirstMinBranchCount : 0;
    context.branchFirstMaxBranches = context.branchFirstEnabled
        ? fcActiveBranchFirstProfile->branchFirstMaxBranches : 0;
    context.branchFirstPreviewDepth = context.branchFirstEnabled
        ? fcActiveBranchFirstProfile->branchFirstPreviewDepth : 0;
    context.branchWaveDispatched = false;
    fcProofDiagnostics.proofSessionQueries++;
    bool borrowedTable = context.session != NULL;
    bool branchRecursive = context.branchFirstEnabled;
    if (borrowedTable) {
        context.nodeBudget = context.session->nodeBudget;
        context.tableCapacity = context.session->tableCapacity;
        context.generation = context.session->generation;
        context.table = NULL;
    } else {
        context.tableCapacity = transpositionCapacity;
        context.generation = 1;
    }
    if ((!borrowedTable || branchRecursive) && context.tableCapacity > 0) {
        context.table = calloc(context.tableCapacity,
                               sizeof(FCProofTTEntry));
        fcProofDiagnostics.allocations++;
        fcProofDiagnostics.allocatedBytes += context.tableCapacity *
                                             sizeof(FCProofTTEntry);
        fcProofDiagnostics.clearedBytes += context.tableCapacity *
                                           sizeof(FCProofTTEntry);
        if (context.table == NULL) context.tableCapacity = 0;
    }
    int referenceBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    int (*mutableBoard)[FC_BOARD_SIZE] = referenceBoard;
    bool useIncrementalPosition = borrowedTable &&
        fc_board_stone_count(board) >= 14;
    if (useIncrementalPosition) {
        context.position = fc_proof_session_prepare_position(
            context.session, board, forbiddenBlack);
        if (context.position == NULL)
            return false;
        mutableBoard = context.position->board;
    } else {
        memcpy(referenceBoard, board, sizeof(referenceBoard));
    }
    int distance = 0;
    uint64_t proofNumber = 1;
    uint64_t disproofNumber = 1;
    int status = borrowedTable && !branchRecursive
        ? fc_dfpn_prove(&context, mutableBoard, maxDepth, &distance,
                        &proofNumber, &disproofNumber)
        : fc_proof_search_attacker(
            mutableBoard, maxDepth, -1, &context, &distance,
            &proofNumber, &disproofNumber);
    if (status != FC_PROOF_PROVEN_WIN &&
        status != FC_PROOF_UNKNOWN &&
        searchClass >= FC_PROOF_SEARCH_VCT) {
        context.searchClass = FC_PROOF_SEARCH_VCT;
        context.certificateCount = 0;
        if (fc_has_vct_root_threat(mutableBoard, attacker,
                                   forbiddenBlack)) {
            status = borrowedTable && !branchRecursive
                ? fc_dfpn_prove(
                    &context, mutableBoard, maxDepth, &distance,
                    &proofNumber, &disproofNumber)
                : fc_proof_search_attacker(
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
    if (context.session != NULL) {
        context.session->nodesUsed += context.nodes;
        fcProofDiagnostics.proofSessionHits += context.hits;
        if (branchRecursive) free(context.table);
    } else {
        free(context.table);
    }
    return result->status == FC_PROOF_PROVEN_WIN;
}

bool fc_verify_scoped_disproof(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *result)
{
    if (board == NULL || result == NULL || fcActiveProofSession != NULL ||
        result->status != FC_PROOF_NO_FORCED_WIN_IN_SCOPE ||
        result->completedDepth <= 0 ||
        result->searchClass < FC_PROOF_SEARCH_VCF) return false;
    int before[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(before, board, sizeof(before));
    uint64_t replayBudget = result->nodes > 0
        ? result->nodes * UINT64_C(64) + UINT64_C(4096)
        : UINT64_C(100000);
    if (replayBudget < UINT64_C(100000))
        replayBudget = UINT64_C(100000);
    FCProofResult replay;
    bool replayedWin = fc_prove_forced_win(
        board, attacker, forbiddenBlack, result->searchClass,
        result->completedDepth, replayBudget, 0, 4096, &replay);
    return !replayedWin &&
           replay.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE &&
           replay.searchClass == result->searchClass &&
           replay.completedDepth == result->completedDepth &&
           memcmp(before, board, sizeof(before)) == 0;
}

static bool fc_verify_scoped_disproof_isolated(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *result)
{
    FCProofSession *previous = fcActiveProofSession;
    fcActiveProofSession = NULL;
    bool verified = fc_verify_scoped_disproof(
        board, attacker, forbiddenBlack, result);
    fcActiveProofSession = previous;
    return verified;
}

bool fc_prove_forced_win_candidate_session(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *result)
{
    if (!fc_profile_proof_path_enabled(profile)) return false;
    FCProofSession session;
    FCProofSession *previous = fcActiveProofSession;
    if (!fc_proof_session_begin(
            &session, profile, board, forbiddenBlack)) return false;
    bool proven = fc_prove_forced_win(
        board, attacker, forbiddenBlack, searchClass,
        profile->proofMaxDepth, profile->proofNodeBudget,
        profile->proofTimeBudgetMs,
        profile->proofTranspositionCapacity, result);
    fc_proof_session_end(&session, previous);
    return proven;
}

bool fc_test_candidate_proof_session_reuse(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *first,
    FCProofResult *second)
{
    if (board == NULL || profile == NULL || first == NULL || second == NULL ||
        !fc_profile_proof_path_enabled(profile)) return false;
    FCProofSession session;
    FCProofSession *previous = fcActiveProofSession;
    if (!fc_proof_session_begin(
            &session, profile, board, forbiddenBlack)) return false;
    bool firstProven = fc_prove_forced_win(
        board, attacker, forbiddenBlack, searchClass,
        profile->proofMaxDepth, profile->proofNodeBudget,
        profile->proofTimeBudgetMs,
        profile->proofTranspositionCapacity, first);
    bool secondProven = fc_prove_forced_win(
        board, attacker, forbiddenBlack, searchClass,
        profile->proofMaxDepth, profile->proofNodeBudget,
        profile->proofTimeBudgetMs,
        profile->proofTranspositionCapacity, second);
    fc_proof_session_end(&session, previous);
    return firstProven == secondProven && first->status == second->status &&
           first->searchClass == second->searchClass &&
           first->distance == second->distance;
}

typedef struct FCParallelProofBatch FCParallelProofBatch;

typedef struct {
    FCPoint gain;
    /* Full coordinator record, including cost/rest metadata.  Keeping the
     * record (rather than just the coordinate) lets the worker seed its
     * first proof node without another root scan. */
    FCThreat threat;
    uint64_t dependencyMask[FC_POSITION_BITSET_WORDS];
    uint64_t fiveWindowMask[FC_POSITION_BITSET_WORDS];
    uint64_t legalityDependencyMask[FC_POSITION_BITSET_WORDS];
    uint64_t certificateZoneMask[FC_POSITION_BITSET_WORDS];
    int overlapGroup;
} FCParallelRoot;

typedef struct {
    FCParallelProofBatch *batch;
    FCProofDiagnostics diagnostics;
} FCParallelProofWorker;

struct FCParallelProofBatch {
    const int (*board)[FC_BOARD_SIZE];
    int attacker;
    bool forbiddenBlack;
    int searchClass;
    int maxDepth;
    uint64_t perJobNodeBudget;
    uint32_t perJobTimeBudgetMs;
    size_t transpositionCapacity;
    FCAIProfile profile;
    FCDecisionLedger *ledger;
    FCParallelRoot roots[FC_MAX_THREATS];
    int rootCount;
    int groupCount;
    int groupStarts[FC_MAX_THREATS];
    int groupLengths[FC_MAX_THREATS];
    int groupRootIndices[FC_MAX_THREATS];
    atomic_int nextGroup;
    _Atomic uint64_t consumedNodeTokens;
    _Atomic int activeWorkers;
    _Atomic int maxConcurrentWorkers;
    _Atomic bool stopRequested;
    uint64_t aggregateNodeBudget;
    uint64_t perJobNodeCeiling;
    double absoluteDeadlineMilliseconds;
    FCProofResult *results;
};

static void *fc_parallel_proof_worker_main(void *opaque)
{
    FCParallelProofWorker *worker = opaque;
    FCParallelProofBatch *batch = worker->batch;
    double priorDeadline = fcActiveDecisionDeadlineMilliseconds;
    int priorFilterX = fcProofRootFilterX;
    int priorFilterY = fcProofRootFilterY;
    const FCThreat *priorRootThreat = fcActiveRootThreat;
    FCDecisionLedger *priorLedger = fcActiveDecisionLedger;
    _Atomic uint64_t *priorParallelCounter = fcActiveParallelNodeCounter;
    uint64_t priorParallelBudget = fcActiveParallelNodeBudget;
    uint64_t priorParallelTokens = fcParallelNodeTokensRemaining;
    uint64_t priorLedgerTokens = fcDecisionLedgerNodeTokensRemaining;
    uint32_t priorBlockSize = fcActiveParallelNodeBlockSize;
    bool priorBlocksEnabled = fcActiveParallelTokenBlocksEnabled;
    fc_proof_diagnostics_reset();
    fcActiveDecisionDeadlineMilliseconds =
        batch->absoluteDeadlineMilliseconds;
    fcActiveDecisionLedger = batch->ledger;
    fcActiveParallelNodeCounter = &batch->consumedNodeTokens;
    fcActiveParallelNodeBudget = batch->aggregateNodeBudget;
    fcParallelNodeTokensRemaining = 0;
    fcDecisionLedgerNodeTokensRemaining = 0;
    fcActiveParallelNodeBlockSize = batch->profile.parallelTokenBlockSize;
    fcActiveParallelTokenBlocksEnabled =
        batch->profile.parallelTokenBlockEnabled;
    int active = atomic_fetch_add_explicit(
        &batch->activeWorkers, 1, memory_order_acq_rel) + 1;
    int observed = atomic_load_explicit(
        &batch->maxConcurrentWorkers, memory_order_relaxed);
    while (active > observed &&
           !atomic_compare_exchange_weak_explicit(
               &batch->maxConcurrentWorkers, &observed, active,
               memory_order_relaxed, memory_order_relaxed)) {
        /* The failed compare-exchange refreshes observed. */
    }
    bool reuseSession = batch->profile.persistentWorkerPoolEnabled;
    FCAIProfile jobProfile = batch->profile;
    jobProfile.parallelProofEnabled = false;
    jobProfile.proofWorkerCount = 1;
    jobProfile.proofParallelNodeBudget = 0;
    jobProfile.proofNodeBudget = batch->perJobNodeCeiling;
    jobProfile.proofTimeBudgetMs = batch->perJobTimeBudgetMs;
    jobProfile.proofEmergencyTimeBudgetMs = batch->perJobTimeBudgetMs;
    FCProofSession session;
    FCProofSession *previousSession = fcActiveProofSession;
    bool sessionReady = reuseSession && fc_proof_session_begin(
        &session, &jobProfile, batch->board, batch->forbiddenBlack);
    bool queryUsable = sessionReady;
    bool queryStarted = false;
    for (;;) {
        if (atomic_load_explicit(&batch->stopRequested,
                                 memory_order_acquire))
            break;
        if (atomic_load_explicit(&batch->consumedNodeTokens,
                                 memory_order_relaxed) >=
            batch->aggregateNodeBudget)
            break;
        int group = atomic_fetch_add_explicit(
            &batch->nextGroup, 1, memory_order_relaxed);
        if (group >= batch->groupCount) break;
        int start = batch->groupStarts[group];
        int length = batch->groupLengths[group];
        uint64_t groupNodeBudget = batch->perJobNodeCeiling;
        if (length > 1 && groupNodeBudget <=
            UINT64_MAX / (uint64_t)length)
            groupNodeBudget *= (uint64_t)length;
        if (groupNodeBudget > batch->aggregateNodeBudget)
            groupNodeBudget = batch->aggregateNodeBudget;
        if (!reuseSession) {
            jobProfile.proofNodeBudget = groupNodeBudget;
            sessionReady = fc_proof_session_begin(
                &session, &jobProfile, batch->board,
                batch->forbiddenBlack);
            queryUsable = sessionReady;
            queryStarted = false;
        }
        if (sessionReady) {
            for (int offset = 0; offset < length; offset++) {
                if (atomic_load_explicit(&batch->stopRequested,
                                         memory_order_acquire))
                    break;
                if (atomic_load_explicit(&batch->consumedNodeTokens,
                                         memory_order_relaxed) >=
                    batch->aggregateNodeBudget)
                    break;
                int index = batch->groupRootIndices[start + offset];
                if (!queryUsable) {
                    memset(&batch->results[index], 0,
                           sizeof(batch->results[index]));
                    batch->results[index].x = -1;
                    batch->results[index].y = -1;
                    batch->results[index].status = FC_PROOF_UNKNOWN;
                    batch->results[index].searchClass = batch->searchClass;
                    batch->results[index].completedDepth = batch->maxDepth;
                    batch->results[index].budgetExhausted = true;
                    continue;
                }
                session.nodeBudget = groupNodeBudget;
                if (queryStarted && !fc_proof_session_reset_for_query(
                        &session, &jobProfile, batch->board,
                        batch->forbiddenBlack)) {
                    queryUsable = false;
                    memset(&batch->results[index], 0,
                           sizeof(batch->results[index]));
                    batch->results[index].x = -1;
                    batch->results[index].y = -1;
                    batch->results[index].status = FC_PROOF_UNKNOWN;
                    batch->results[index].searchClass = batch->searchClass;
                    batch->results[index].completedDepth = batch->maxDepth;
                    batch->results[index].budgetExhausted = true;
                    continue;
                }
                queryStarted = true;
                fcProofRootFilterX = batch->roots[index].gain.x;
                fcProofRootFilterY = batch->roots[index].gain.y;
                fcActiveRootThreat = &batch->roots[index].threat;
                (void)fc_prove_forced_win(
                    batch->board, batch->attacker, batch->forbiddenBlack,
                    batch->searchClass, batch->maxDepth,
                    groupNodeBudget, batch->perJobTimeBudgetMs,
                    batch->transpositionCapacity, &batch->results[index]);
                fc_parallel_node_token_flush();
                if (fcActiveDecisionLedger != NULL &&
                    batch->results[index].status == FC_PROOF_PROVEN_WIN &&
                    batch->results[index].certificateVerified) {
                    atomic_store_explicit(&batch->stopRequested, true,
                                          memory_order_release);
                    fcProofDiagnostics.parallelEarlyStops++;
                }
            }
        } else {
            for (int offset = 0; offset < length; offset++) {
                int index = batch->groupRootIndices[start + offset];
                memset(&batch->results[index], 0,
                       sizeof(batch->results[index]));
                batch->results[index].x = -1;
                batch->results[index].y = -1;
                batch->results[index].status = FC_PROOF_UNKNOWN;
                batch->results[index].searchClass = batch->searchClass;
                batch->results[index].completedDepth = batch->maxDepth;
                batch->results[index].budgetExhausted = true;
            }
        }
        if (!reuseSession && sessionReady) {
            fc_proof_session_end(&session, previousSession);
            sessionReady = false;
        }
    }
    if (reuseSession && sessionReady)
        fc_proof_session_end(&session, previousSession);
    fc_parallel_node_token_flush();
    worker->diagnostics = fc_proof_diagnostics_get();
    fcProofRootFilterX = priorFilterX;
    fcProofRootFilterY = priorFilterY;
    fcActiveRootThreat = priorRootThreat;
    fcActiveDecisionLedger = priorLedger;
    fcActiveDecisionDeadlineMilliseconds = priorDeadline;
    fcActiveParallelNodeCounter = priorParallelCounter;
    fcActiveParallelNodeBudget = priorParallelBudget;
    fcParallelNodeTokensRemaining = priorParallelTokens;
    fcDecisionLedgerNodeTokensRemaining = priorLedgerTokens;
    fcActiveParallelNodeBlockSize = priorBlockSize;
    fcActiveParallelTokenBlocksEnabled = priorBlocksEnabled;
    atomic_fetch_sub_explicit(&batch->activeWorkers, 1, memory_order_acq_rel);
    return NULL;
}

typedef struct {
    FCParallelProofBatch *batch;
    FCParallelProofWorker *workers;
} FCParallelProofPoolTask;

static void fc_parallel_proof_pool_task(void *opaque, int workerSlot)
{
    FCParallelProofPoolTask *task = opaque;
    if (task == NULL || task->batch == NULL || task->workers == NULL ||
        workerSlot < 0 || workerSlot >= FC_PARALLEL_MAX_WORKERS) return;
    task->workers[workerSlot].batch = task->batch;
    (void)fc_parallel_proof_worker_main(&task->workers[workerSlot]);
}

static bool fc_parallel_prove_forced_win(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    int maxDepth,
    uint64_t aggregateNodeBudget,
    uint32_t timeBudgetMs,
    const FCAIProfile *profile,
    FCProofResult *result)
{
    if (result != NULL && fcActiveDecisionLedger != NULL &&
        !fc_decision_ledger_reserve_nodes(fcActiveDecisionLedger, 1)) {
        memset(result, 0, sizeof(*result));
        result->x = -1;
        result->y = -1;
        result->status = FC_PROOF_UNKNOWN;
        result->searchClass = searchClass;
        result->completedDepth = maxDepth;
        result->budgetExhausted = true;
        return false;
    }
    if (profile != NULL && result != NULL &&
        profile->branchFirstSearchEnabled) {
        const FCAIProfile *previousBranchProfile =
            fcActiveBranchFirstProfile;
        fcActiveBranchFirstProfile = profile;
        bool proven = fc_prove_forced_win(
            board, attacker, forbiddenBlack, searchClass, maxDepth,
            aggregateNodeBudget, timeBudgetMs,
            profile->proofTranspositionCapacity, result);
        fcActiveBranchFirstProfile = previousBranchProfile;
        return proven;
    }
    if (profile == NULL || result == NULL ||
        !profile->parallelProofEnabled || fc_effective_worker_count(profile) <= 1) {
        return fc_prove_forced_win(
            board, attacker, forbiddenBlack, searchClass, maxDepth,
            aggregateNodeBudget, timeBudgetMs,
            profile != NULL ? profile->proofTranspositionCapacity : 0,
            result);
    }
    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int threatCount = fc_enumerate_threats(
        board, attacker, forbiddenBlack, searchClass,
        threats, FC_MAX_THREATS, &overflow);
    int order[FC_MAX_THREATS];
    fc_order_threats_by_dependency(threats, threatCount, searchClass, order);
    FCParallelRoot roots[FC_MAX_THREATS];
    int rootCount = 0;
    for (int orderIndex = 0; orderIndex < threatCount; orderIndex++) {
        FCThreat *threat = &threats[order[orderIndex]];
        FCPoint gain = threat->gain;
        bool duplicate = false;
        for (int previous = 0; previous < rootCount; previous++) {
            if (roots[previous].gain.x == gain.x &&
                roots[previous].gain.y == gain.y) {
                duplicate = true;
                fc_threat_mask_or(roots[previous].dependencyMask,
                                  threat->dependencyMask);
                fc_threat_mask_or(roots[previous].fiveWindowMask,
                                  threat->fiveWindowMask);
                fc_threat_mask_or(roots[previous].legalityDependencyMask,
                                  threat->legalityDependencyMask);
                fc_threat_mask_or(roots[previous].certificateZoneMask,
                                  threat->certificateZoneMask);
                fcProofDiagnostics.parallelExactDuplicateRoots++;
                break;
            }
        }
        if (!duplicate) {
            FCParallelRoot *root = &roots[rootCount++];
            memset(root, 0, sizeof(*root));
            root->gain = gain;
            root->threat = *threat;
            memcpy(root->dependencyMask, threat->dependencyMask,
                   sizeof(root->dependencyMask));
            memcpy(root->fiveWindowMask, threat->fiveWindowMask,
                   sizeof(root->fiveWindowMask));
            memcpy(root->legalityDependencyMask,
                   threat->legalityDependencyMask,
                   sizeof(root->legalityDependencyMask));
            memcpy(root->certificateZoneMask, threat->certificateZoneMask,
                   sizeof(root->certificateZoneMask));
        }
    }

    /* Threats with different gains can still describe the same proof
     * obligation.  Audit all four dependency domains and form connected
     * overlap groups.  The grouping is deliberately conservative: it only
     * affects scheduling/telemetry; every distinct gain remains an
     * independently checked root, so an unresolved group can never be
     * mistaken for aggregate disproof. */
    int parent[FC_MAX_THREATS];
    for (int i = 0; i < rootCount; i++) parent[i] = i;
    for (int i = 0; i < rootCount; i++) {
        for (int j = i + 1; j < rootCount; j++) {
            bool dependencyOverlap = false;
            bool fiveWindowOverlap = false;
            bool legalityOverlap = false;
            bool certificateOverlap = false;
            for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
                dependencyOverlap = dependencyOverlap ||
                    ((roots[i].dependencyMask[word] &
                      roots[j].dependencyMask[word]) != 0);
                fiveWindowOverlap = fiveWindowOverlap ||
                    ((roots[i].fiveWindowMask[word] &
                      roots[j].fiveWindowMask[word]) != 0);
                legalityOverlap = legalityOverlap ||
                    ((roots[i].legalityDependencyMask[word] &
                      roots[j].legalityDependencyMask[word]) != 0);
                certificateOverlap = certificateOverlap ||
                    ((roots[i].certificateZoneMask[word] &
                      roots[j].certificateZoneMask[word]) != 0);
            }
            if (dependencyOverlap)
                fcProofDiagnostics.parallelDependencyOverlapPairs++;
            if (fiveWindowOverlap)
                fcProofDiagnostics.parallelFiveWindowOverlapPairs++;
            if (legalityOverlap)
                fcProofDiagnostics.parallelLegalityOverlapPairs++;
            if (certificateOverlap)
                fcProofDiagnostics.parallelCertificateOverlapPairs++;
            if (!(dependencyOverlap || fiveWindowOverlap ||
                  legalityOverlap || certificateOverlap)) continue;
            fcProofDiagnostics.parallelOverlapPairs++;
            int left = i;
            while (parent[left] != left) left = parent[left];
            int right = j;
            while (parent[right] != right) right = parent[right];
            if (left != right) parent[right] = left;
        }
    }
    int groupCount = 0;
    int groupSizes[FC_MAX_THREATS] = {0};
    for (int i = 0; i < rootCount; i++) {
        int root = i;
        while (parent[root] != root) root = parent[root];
        roots[i].overlapGroup = root;
        groupSizes[root]++;
    }
    for (int i = 0; i < rootCount; i++) {
        if (groupSizes[i] <= 0) continue;
        groupCount++;
        if ((uint64_t)groupSizes[i] >
            fcProofDiagnostics.parallelLargestOverlapGroup)
            fcProofDiagnostics.parallelLargestOverlapGroup =
                (uint64_t)groupSizes[i];
    }
    int compactGroupForRoot[FC_MAX_THREATS];
    for (int i = 0; i < FC_MAX_THREATS; i++)
        compactGroupForRoot[i] = -1;
    int compactGroupCount = 0;
    int compactGroupSizes[FC_MAX_THREATS] = {0};
    for (int i = 0; i < rootCount; i++) {
        int representative = roots[i].overlapGroup;
        if (compactGroupForRoot[representative] < 0)
            compactGroupForRoot[representative] = compactGroupCount++;
        int compact = compactGroupForRoot[representative];
        roots[i].overlapGroup = compact;
        compactGroupSizes[compact]++;
    }
    fcProofDiagnostics.parallelOverlapGroups += (uint64_t)groupCount;
    fcProofDiagnostics.parallelGroupedRootJobs += (uint64_t)rootCount;
    if (rootCount < 2 || fc_decision_deadline_reached()) {
        fcProofDiagnostics.parallelFallbacks++;
        return fc_prove_forced_win(
            board, attacker, forbiddenBlack, searchClass, maxDepth,
            aggregateNodeBudget, timeBudgetMs,
            profile->proofTranspositionCapacity, result);
    }
    int workerCount = fc_effective_worker_count(profile);
    if (workerCount > rootCount) workerCount = rootCount;
    if (workerCount > 8) workerCount = 8;
    uint64_t declaredParallelBudget = profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget : aggregateNodeBudget;
    if (aggregateNodeBudget > 0 &&
        aggregateNodeBudget < declaredParallelBudget)
        declaredParallelBudget = aggregateNodeBudget;
    /* A root is now allowed a full single-session ceiling instead of being
     * throttled by the total number of shallow roots.  The dispatcher keeps
     * one atomic token budget across all workers, so deeper waves are free to
     * continue when an earlier root finishes early while the aggregate cap is
     * still exact. */
    uint64_t perJobNodeBudget = profile->proofNodeBudget > 0
        ? profile->proofNodeBudget : declaredParallelBudget;
    if (perJobNodeBudget > declaredParallelBudget)
        perJobNodeBudget = declaredParallelBudget;
    if (perJobNodeBudget == 0) perJobNodeBudget = 1;
    uint32_t perJobTimeBudgetMs = timeBudgetMs;
    if (fcActiveDecisionLedger == NULL &&
        profile->proofEmergencyTimeBudgetMs > perJobTimeBudgetMs)
        perJobTimeBudgetMs = profile->proofEmergencyTimeBudgetMs;
    FCProofResult *results = calloc((size_t)rootCount, sizeof(*results));
    pthread_t *threads = profile->persistentWorkerPoolEnabled
        ? NULL : calloc((size_t)workerCount, sizeof(*threads));
    FCParallelProofWorker *workers = calloc(
        (size_t)workerCount, sizeof(*workers));
    if (results == NULL || workers == NULL ||
        (!profile->persistentWorkerPoolEnabled && threads == NULL)) {
        free(results);
        free(threads);
        free(workers);
        fcProofDiagnostics.parallelFallbacks++;
        return fc_prove_forced_win(
            board, attacker, forbiddenBlack, searchClass, maxDepth,
            aggregateNodeBudget, timeBudgetMs,
            profile->proofTranspositionCapacity, result);
    }
    FCParallelProofBatch batch;
    memset(&batch, 0, sizeof(batch));
    batch.board = board;
    batch.attacker = attacker;
    batch.forbiddenBlack = forbiddenBlack;
    batch.searchClass = searchClass;
    batch.maxDepth = maxDepth;
    batch.perJobNodeBudget = perJobNodeBudget;
    batch.perJobTimeBudgetMs = perJobTimeBudgetMs;
    batch.transpositionCapacity = profile->proofTranspositionCapacity;
    batch.profile = *profile;
    batch.ledger = fcActiveDecisionLedger;
    memcpy(batch.roots, roots, (size_t)rootCount * sizeof(roots[0]));
    batch.rootCount = rootCount;
    /* Overlap components remain metadata for provenance and future TT policy,
     * but each distinct gain is an independent proof job.  Dynamic workers
     * claim one root at a time so conservative legality masks cannot serialize
     * the whole batch. */
    batch.groupCount = rootCount;
    for (int root = 0; root < rootCount; root++) {
        batch.groupStarts[root] = root;
        batch.groupLengths[root] = 1;
        batch.groupRootIndices[root] = root;
    }
    atomic_init(&batch.nextGroup, 0);
    atomic_init(&batch.consumedNodeTokens, 0);
    atomic_init(&batch.activeWorkers, 0);
    atomic_init(&batch.maxConcurrentWorkers, 0);
    atomic_init(&batch.stopRequested, false);
    batch.aggregateNodeBudget = declaredParallelBudget;
    batch.perJobNodeCeiling = perJobNodeBudget;
    batch.absoluteDeadlineMilliseconds =
        fcActiveDecisionDeadlineMilliseconds > 0.0
            ? fcActiveDecisionDeadlineMilliseconds
            : perJobTimeBudgetMs > 0
            ? fc_now_milliseconds() + (double)perJobTimeBudgetMs
            : 0.0;
    batch.results = results;
    int launched = 0;
    if (profile->persistentWorkerPoolEnabled) {
        FCParallelProofPoolTask poolTask = {
            .batch = &batch, .workers = workers
        };
        launched = fc_worker_pool_dispatch(
            fc_parallel_proof_pool_task, &poolTask, workerCount);
        if (launched > 0) {
            fcProofDiagnostics.parallelPoolDispatches++;
            fcProofDiagnostics.parallelPoolWorkersReused +=
                (uint64_t)launched;
            if (launched < workerCount)
                fcProofDiagnostics.parallelPoolFallbacks++;
        } else {
            fcProofDiagnostics.parallelPoolFallbacks++;
        }
    } else {
        for (int i = 0; i < workerCount; i++) {
            workers[i].batch = &batch;
            if (pthread_create(&threads[i], NULL,
                               fc_parallel_proof_worker_main,
                               &workers[i]) != 0) break;
            launched++;
        }
    }
    if (launched == 0) {
        free(results);
        free(threads);
        free(workers);
        fcProofDiagnostics.parallelFallbacks++;
        return fc_prove_forced_win(
            board, attacker, forbiddenBlack, searchClass, maxDepth,
            aggregateNodeBudget, timeBudgetMs,
            profile->proofTranspositionCapacity, result);
    }
    if (!profile->persistentWorkerPoolEnabled) {
        for (int i = 0; i < launched; i++)
            (void)pthread_join(threads[i], NULL);
    }
    for (int i = 0; i < launched; i++) {
        fc_proof_diagnostics_merge(
            &fcProofDiagnostics, &workers[i].diagnostics);
    }
    uint64_t aggregateNodes = 0;
    uint64_t aggregateHits = 0;
    int completedJobs = 0;
    int winningRoot = -1;
    bool anyBudgetExhausted = false;
    bool allDisproved = !overflow;
    for (int i = 0; i < rootCount; i++) {
        aggregateNodes += results[i].nodes;
        aggregateHits += results[i].transpositionHits;
        bool completed = !results[i].budgetExhausted &&
                         results[i].status != FC_PROOF_UNKNOWN;
        if (completed) completedJobs++;
        if (winningRoot < 0 &&
            results[i].status == FC_PROOF_PROVEN_WIN &&
            results[i].certificateVerified)
            winningRoot = i;
        if (results[i].status != FC_PROOF_NO_FORCED_WIN_IN_SCOPE ||
            results[i].budgetExhausted)
            allDisproved = false;
        anyBudgetExhausted = anyBudgetExhausted ||
                             results[i].budgetExhausted;
    }
    memset(result, 0, sizeof(*result));
    result->x = -1;
    result->y = -1;
    result->searchClass = searchClass;
    result->completedDepth = maxDepth;
    if (winningRoot >= 0) {
        *result = results[winningRoot];
        fcProofDiagnostics.parallelRootWins++;
    } else if (allDisproved && completedJobs == rootCount) {
        result->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
        result->proofNumber = FC_PROOF_INFINITY;
        result->disproofNumber = 0;
        fcProofDiagnostics.parallelAggregateDisproofs++;
    } else {
        result->status = FC_PROOF_UNKNOWN;
        result->proofNumber = 1;
        result->disproofNumber = 1;
    }
    /* For a verified win the certificate is the selected deterministic
     * result; aborted sibling work depends on thread completion order and
     * must not perturb the published proof counters.  Aggregate activity is
     * retained in diagnostics/parallelNodes telemetry. */
    result->nodes = winningRoot >= 0 ? results[winningRoot].nodes
                                    : aggregateNodes;
    result->transpositionHits = winningRoot >= 0
        ? results[winningRoot].transpositionHits : aggregateHits;
    result->budgetExhausted = anyBudgetExhausted ||
                              fc_decision_deadline_reached();
    fcProofDiagnostics.parallelBatches++;
    fcProofDiagnostics.parallelWorkersLaunched += (uint64_t)launched;
    uint64_t maxConcurrentWorkers = (uint64_t)atomic_load_explicit(
        &batch.maxConcurrentWorkers, memory_order_relaxed);
    if (maxConcurrentWorkers >
        fcProofDiagnostics.parallelMaxConcurrentWorkers)
        fcProofDiagnostics.parallelMaxConcurrentWorkers =
            maxConcurrentWorkers;
    if (rootCount > 1 && launched <= 1)
        fcProofDiagnostics.parallelMultiRootSingleWorkerFallbacks++;
    if (rootCount > 1 && launched > 1)
        fcProofDiagnostics.parallelIndependentDispatches++;
    fcProofDiagnostics.parallelRootJobs += (uint64_t)rootCount;
    fcProofDiagnostics.parallelRootJobsCompleted +=
        (uint64_t)completedJobs;
    fcDecisionParallelNodes += aggregateNodes;
    fcDecisionWorkersLaunched += launched;
    fcDecisionParallelJobs += rootCount;
    fcDecisionParallelJobsCompleted += completedJobs;
    bool proven = result->status == FC_PROOF_PROVEN_WIN &&
                  result->certificateVerified;
    free(results);
    free(threads);
    free(workers);
    return proven;
}

bool fc_test_parallel_root_proof(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    int searchClass,
    const FCAIProfile *profile,
    FCProofResult *result)
{
    if (profile == NULL || (!profile->parallelProofEnabled &&
                            !profile->branchFirstSearchEnabled)) return false;
    uint64_t budget = profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
    return fc_parallel_prove_forced_win(
        board, attacker, forbiddenBlack, searchClass,
        profile->proofMaxDepth, budget, profile->proofTimeBudgetMs,
        profile, result);
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
                                           true, 0.0, 0,
                                           FC_FORK_RISK_UNKNOWN, false};
                count++;
            }
        }
        qsort(out, (size_t)count, sizeof(FCCandidate), fc_compare_candidate);
        return count;
    }

    int stoneCount = fc_board_stone_count((const int (*)[FC_BOARD_SIZE])board);
    if (stoneCount == 0 && capacity > 0) {
        out[0] = (FCCandidate){7, 7, 1000, FC_TACTICAL_NORMAL, true, 0.0,
                               0, FC_FORK_RISK_UNKNOWN, false};
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
                                            safe, 0.0, 0,
                                            FC_FORK_RISK_UNKNOWN, false};
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

typedef struct {
    int opponentImmediateWinCount;
    FCForkRisk risk;
    bool classificationComplete;
    bool boardCoverageComplete;
} FCForkProbe;

typedef struct {
    FCCandidate all[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int allCount;
    int candidatesExamined;
    int safeCandidates;
    int riskyCandidates;
    int unknownCandidates;
    bool allProbesComplete;
    bool candidateCoverageComplete;
} FCForkCandidateSet;

static bool fc_fork_risk_is_nonfork(FCForkRisk risk)
{
    return risk == FC_FORK_RISK_SAFE ||
           risk == FC_FORK_RISK_ONE_REPLY ||
           risk == FC_FORK_RISK_OWN_WIN;
}

/* The probe is intentionally separate from fc_immediate_replies_after_move.
 * The latter is a local continuation helper for the side passed to it; this
 * routine first places the candidate side, then scans legal moves for the
 * opponent on that resulting board.  Two replies are sufficient to classify
 * a fork, while zero/one requires a complete scan. */
static FCForkProbe fc_probe_opponent_replies_after_placement(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int x,
    int y,
    int side,
    bool forbiddenBlack)
{
    FCForkProbe probe = {
        .opponentImmediateWinCount = -1,
        .risk = FC_FORK_RISK_UNKNOWN,
        .classificationComplete = false,
        .boardCoverageComplete = false
    };
    if (board == NULL || !fc_is_legal_move(board, x, y, side,
                                           forbiddenBlack)) return probe;

    int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(after, board, sizeof(after));
    if (!fc_make_move(after, x, y, side, forbiddenBlack)) return probe;
    if (fc_has_five((const int (*)[FC_BOARD_SIZE])after, x, y, side)) {
        probe.opponentImmediateWinCount = 0;
        probe.risk = FC_FORK_RISK_OWN_WIN;
        probe.classificationComplete = true;
        probe.boardCoverageComplete = true;
        return probe;
    }

    int replies = 0;
    for (int replyX = 0; replyX < FC_BOARD_SIZE; replyX++) {
        for (int replyY = 0; replyY < FC_BOARD_SIZE; replyY++) {
            if (fc_global_proof_deadline_reached()) return probe;
            if (!fc_is_legal_move(
                    (const int (*)[FC_BOARD_SIZE])after,
                    replyX, replyY, -side, forbiddenBlack)) continue;
            if (!fc_wins_if_placed(after, replyX, replyY, -side)) continue;
            replies++;
            if (replies >= 2) {
                probe.opponentImmediateWinCount = 2;
                probe.risk = FC_FORK_RISK_FORK;
                /* We do not claim that every legal reply was enumerated, but
                 * the fork classification itself is complete once two wins
                 * are found.  Candidate-universe coverage is tracked by the
                 * caller separately. */
                probe.classificationComplete = true;
                probe.boardCoverageComplete = false;
                return probe;
            }
        }
    }
    probe.opponentImmediateWinCount = replies;
    probe.risk = replies == 0 ? FC_FORK_RISK_SAFE
                 : FC_FORK_RISK_ONE_REPLY;
    probe.classificationComplete = true;
    probe.boardCoverageComplete = true;
    return probe;
}

static int fc_fork_candidate_rank(const FCCandidate *candidate)
{
    if (candidate == NULL) return 6;
    if (candidate->tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
        candidate->forkRisk == FC_FORK_RISK_OWN_WIN) return 0;
    if (candidate->tacticalClass == FC_TACTICAL_MUST_DEFEND) return 1;
    if (candidate->forkRisk == FC_FORK_RISK_SAFE &&
        candidate->tacticalClass != FC_TACTICAL_NORMAL) return 2;
    if (candidate->forkRisk == FC_FORK_RISK_SAFE) return 3;
    if (candidate->forkRisk == FC_FORK_RISK_ONE_REPLY) return 4;
    if (candidate->forkRisk == FC_FORK_RISK_FORK) return 5;
    return 6;
}

static int fc_compare_fork_candidate(const void *left, const void *right)
{
    const FCCandidate *a = (const FCCandidate *)left;
    const FCCandidate *b = (const FCCandidate *)right;
    int aRank = fc_fork_candidate_rank(a);
    int bRank = fc_fork_candidate_rank(b);
    if (aRank != bRank) return aRank < bRank ? -1 : 1;
    int aReplies = a->opponentImmediateWinCount < 0
        ? FC_BOARD_SIZE * FC_BOARD_SIZE + 1
        : a->opponentImmediateWinCount;
    int bReplies = b->opponentImmediateWinCount < 0
        ? FC_BOARD_SIZE * FC_BOARD_SIZE + 1
        : b->opponentImmediateWinCount;
    if (aReplies != bReplies) return aReplies < bReplies ? -1 : 1;
    if (a->score != b->score) return a->score > b->score ? -1 : 1;
    if (a->x != b->x) return a->x < b->x ? -1 : 1;
    if (a->y != b->y) return a->y < b->y ? -1 : 1;
    return 0;
}

static bool fc_append_fork_candidate(FCForkCandidateSet *set,
                                     FCCandidate candidate)
{
    if (set == NULL || !fc_inside(candidate.x, candidate.y)) return false;
    for (int i = 0; i < set->allCount; i++) {
        if (set->all[i].x != candidate.x ||
            set->all[i].y != candidate.y) continue;
        if (fc_compare_candidate(&candidate, &set->all[i]) < 0)
            set->all[i] = candidate;
        return false;
    }
    if (set->allCount >= FC_BOARD_SIZE * FC_BOARD_SIZE) return false;
    set->all[set->allCount++] = candidate;
    return true;
}

static bool fc_append_fork_move(
    FCForkCandidateSet *set,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int x,
    int y,
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile)
{
    if (board == NULL || profile == NULL ||
        !fc_is_legal_move(board, x, y, side, forbiddenBlack)) return false;
    int tacticalClass = fc_wins_if_placed(
        (int (*)[FC_BOARD_SIZE])board, x, y, side)
        ? FC_TACTICAL_IMMEDIATE_WIN : FC_TACTICAL_NORMAL;
    FCCandidate candidate = {
        .x = x,
        .y = y,
        .score = fc_move_heuristic((int (*)[FC_BOARD_SIZE])board,
                                   x, y, side, profile),
        .tacticalClass = tacticalClass,
        .safe = true,
        .probability = 0.0,
        .opponentImmediateWinCount = -1,
        .forkRisk = FC_FORK_RISK_UNKNOWN,
        .forkProbeComplete = false
    };
    return fc_append_fork_candidate(set, candidate);
}

static void fc_annotate_fork_candidate(
    FCForkCandidateSet *set,
    FCCandidate *candidate,
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    bool currentOpponentThreat)
{
    if (set == NULL || candidate == NULL || board == NULL) return;
    FCForkProbe probe = fc_probe_opponent_replies_after_placement(
        board, candidate->x, candidate->y, side, forbiddenBlack);
    candidate->opponentImmediateWinCount = probe.opponentImmediateWinCount;
    candidate->forkRisk = probe.risk;
    candidate->forkProbeComplete = probe.classificationComplete;
    if (probe.risk == FC_FORK_RISK_OWN_WIN) {
        candidate->tacticalClass = FC_TACTICAL_IMMEDIATE_WIN;
        candidate->safe = true;
    } else if (currentOpponentThreat &&
               probe.risk == FC_FORK_RISK_SAFE) {
        candidate->tacticalClass = FC_TACTICAL_MUST_DEFEND;
        candidate->safe = true;
    } else {
        candidate->safe = probe.risk == FC_FORK_RISK_SAFE;
    }
    set->candidatesExamined++;
    fcProofDiagnostics.forkProbeCandidatesExamined++;
    if (!probe.classificationComplete) {
        set->unknownCandidates++;
        fcProofDiagnostics.forkProbeUnknownCandidates++;
    } else if (fc_fork_risk_is_nonfork(probe.risk)) {
        set->safeCandidates++;
        fcProofDiagnostics.forkProbeSafeCandidates++;
    } else {
        set->riskyCandidates++;
        fcProofDiagnostics.forkProbeRiskyCandidates++;
    }
}

/* Build a deterministic tactical pool.  The first pass contains generated
 * tactical candidates, both advisory points, and every legal relevance-zone
 * point.  A complete canonical board pass is only attempted when the first
 * pass has no classified non-fork move. */
static bool fc_apply_fork_candidate_layer(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    FCCandidate *moves,
    int *ioCount,
    int capacity,
    int preferredX,
    int preferredY,
    int secondaryX,
    int secondaryY,
    FCAnalysisResult *telemetry)
{
    if (board == NULL || profile == NULL || moves == NULL || ioCount == NULL ||
        capacity <= 0 || !profile->forkFirstRecoveryEnabled) return false;

    FCForkCandidateSet set;
    memset(&set, 0, sizeof(set));
    for (int i = 0; i < *ioCount; i++)
        (void)fc_append_fork_candidate(&set, moves[i]);
    (void)fc_append_fork_move(&set, board, preferredX, preferredY, side,
                              forbiddenBlack, profile);
    (void)fc_append_fork_move(&set, board, secondaryX, secondaryY, side,
                              forbiddenBlack, profile);
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_has_neighbor(board, x, y, 4)) continue;
            (void)fc_append_fork_move(&set, board, x, y, side,
                                      forbiddenBlack, profile);
        }
    }

    int rivalThreats = fc_count_immediate_wins(
        (int (*)[FC_BOARD_SIZE])board, -side, forbiddenBlack, NULL, 0);
    bool currentOpponentThreat = rivalThreats > 0;
    int initialCount = set.allCount;
    bool initialComplete = true;
    for (int i = 0; i < initialCount; i++) {
        if (fc_decision_deadline_reached()) {
            initialComplete = false;
            break;
        }
        fc_annotate_fork_candidate(
            &set, &set.all[i], board, side, forbiddenBlack,
            currentOpponentThreat);
    }
    bool hasNonFork = false;
    for (int i = 0; i < initialCount; i++) {
        if (fc_fork_risk_is_nonfork(set.all[i].forkRisk)) {
            hasNonFork = true;
            break;
        }
    }
    set.allProbesComplete = initialComplete;
    set.candidateCoverageComplete = false;

    if (initialComplete && !hasNonFork &&
        !fc_decision_deadline_reached()) {
        bool fullComplete = true;
        for (int x = 0; x < FC_BOARD_SIZE && fullComplete; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                if (fc_decision_deadline_reached()) {
                    fullComplete = false;
                    break;
                }
                if (!fc_is_legal_move(board, x, y, side,
                                      forbiddenBlack)) continue;
                bool added = fc_append_fork_move(
                    &set, board, x, y, side, forbiddenBlack, profile);
                if (added) {
                    fc_annotate_fork_candidate(
                        &set, &set.all[set.allCount - 1], board, side,
                        forbiddenBlack, currentOpponentThreat);
                }
            }
        }
        set.allProbesComplete = fullComplete;
        set.candidateCoverageComplete = fullComplete;
        if (!fullComplete) fcProofDiagnostics.forkProbeIncompleteDecisions++;
    } else if (!initialComplete) {
        fcProofDiagnostics.forkProbeIncompleteDecisions++;
    }

    qsort(set.all, (size_t)set.allCount, sizeof(set.all[0]),
          fc_compare_fork_candidate);
    int outputCount = set.allCount < capacity ? set.allCount : capacity;
    memcpy(moves, set.all, (size_t)outputCount * sizeof(moves[0]));
    /* Keep the advisory handoff in the projected result even when the
     * canonical ranking truncates the candidate list.  The selected move is
     * allowed to displace a risky advisory point, but telemetry must still be
     * able to prove that a fork was avoided rather than treating an omitted
     * handoff as an unobserved candidate. */
    if (outputCount > 0 && fc_inside(preferredX, preferredY)) {
        bool preferredPresent = false;
        int preferredIndex = -1;
        for (int i = 0; i < set.allCount; i++) {
            if (set.all[i].x != preferredX || set.all[i].y != preferredY)
                continue;
            preferredIndex = i;
            break;
        }
        for (int i = 0; i < outputCount; i++) {
            if (moves[i].x == preferredX && moves[i].y == preferredY) {
                preferredPresent = true;
                break;
            }
        }
        if (!preferredPresent && preferredIndex >= 0) {
            moves[outputCount - 1] = set.all[preferredIndex];
            qsort(moves, (size_t)outputCount, sizeof(moves[0]),
                  fc_compare_fork_candidate);
        }
    }
    *ioCount = outputCount;
    if (telemetry != NULL) {
        telemetry->forkCandidatesExamined = set.candidatesExamined;
        telemetry->forkSafeCandidates = set.safeCandidates;
        telemetry->forkRiskyCandidates = set.riskyCandidates;
        telemetry->forkUnknownCandidates = set.unknownCandidates;
        telemetry->forkProbeComplete = set.allProbesComplete;
        telemetry->candidateCoverageComplete =
            set.candidateCoverageComplete;
    }
    return outputCount > 0;
}

static void fc_record_selected_fork_telemetry(FCAnalysisResult *result,
                                              int preferredX,
                                              int preferredY)
{
    if (result == NULL) return;
    const FCCandidate *selected = NULL;
    const FCCandidate *preferred = NULL;
    for (int i = 0; i < result->candidateCount; i++) {
        const FCCandidate *candidate = &result->candidates[i];
        if (candidate->x == result->x && candidate->y == result->y)
            selected = candidate;
        if (candidate->x == preferredX && candidate->y == preferredY)
            preferred = candidate;
    }
    if (selected != NULL) {
        result->forkRiskStatus = selected->forkRisk;
        result->selectedOpponentImmediateWinCount =
            selected->opponentImmediateWinCount;
    }
    if (selected != NULL && preferred != NULL &&
        preferred->forkRisk == FC_FORK_RISK_FORK &&
        fc_fork_risk_is_nonfork(selected->forkRisk) &&
        !(selected->x == preferred->x && selected->y == preferred->y)) {
        result->forkAvoidedCount++;
        fcProofDiagnostics.forkProbeAvoidedForks++;
    }
}

/* Recovery must never infer a global loss from a relevance-truncated list.
 * This scan is deliberately canonical (x then y) so a timeout has a stable
 * fallback independent of worker completion order. */
static bool fc_find_full_board_legal_fallback(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int *outX,
    int *outY)
{
    if (outX != NULL) *outX = -1;
    if (outY != NULL) *outY = -1;
    if (board == NULL || (side != 1 && side != -1)) return false;
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            if (!fc_is_legal_move(board, x, y, side, forbiddenBlack)) continue;
            if (outX != NULL) *outX = x;
            if (outY != NULL) *outY = y;
            return true;
        }
    }
    return false;
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
    if (fc_decision_deadline_reached()) {
        context->aborted = true;
        return false;
    }
    if (context->nodes >= context->profile.nodeBudget) {
        context->aborted = true;
        return false;
    }
    if (fcActiveDecisionLedger != NULL &&
        !fc_decision_ledger_consume_node(fcActiveDecisionLedger)) {
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
    bool forkRecovery = profile != NULL && profile->forkFirstRecoveryEnabled;
    int firstSafe = -1;
    for (int i = 0; i < count; i++) {
        if (moves[i].safe ||
            (forkRecovery && fc_fork_risk_is_nonfork(moves[i].forkRisk))) {
            firstSafe = i;
            break;
        }
    }
    if (firstSafe < 0) {
        /* A bounded probe may know only that every examined move is risky or
         * unknown.  The caller still needs a legal deterministic move; the
         * fork-ranked order supplies the least-known-risk entry. */
        return count > 0 ? 0 : -1;
    }
    if (forkRecovery) {
        moves[firstSafe].probability = 1.0;
        return firstSafe;
    }
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

typedef struct {
    int x;
    int y;
    int stage;
    int score;
} FCEscapeCandidate;

static bool fc_append_escape_candidate(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int x,
    int y,
    int stage,
    int score,
    FCEscapeCandidate *out,
    int *count,
    int capacity)
{
    if (*count >= capacity ||
        !fc_is_legal_move(board, x, y, side, forbiddenBlack)) return false;
    for (int i = 0; i < *count; i++) {
        if (out[i].x != x || out[i].y != y) continue;
        if (stage < out[i].stage) out[i].stage = stage;
        if (score > out[i].score) out[i].score = score;
        return false;
    }
    out[(*count)++] = (FCEscapeCandidate){x, y, stage, score};
    return true;
}

static int fc_compare_escape_candidate(const void *left, const void *right)
{
    const FCEscapeCandidate *a = left;
    const FCEscapeCandidate *b = right;
    if (a->stage != b->stage) return a->stage < b->stage ? -1 : 1;
    if (a->score != b->score) return a->score > b->score ? -1 : 1;
    if (a->x != b->x) return a->x < b->x ? -1 : 1;
    return a->y < b->y ? -1 : a->y > b->y;
}

static void fc_append_certificate_line_dependencies(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int gainX,
    int gainY,
    const FCAIProfile *profile,
    FCEscapeCandidate *out,
    int *count,
    int capacity)
{
    /* The certificate stores played gain/cost nodes, not the rest squares of
     * every supporting five-window.  Recover those local dependencies from
     * the pre-decision board.  A move in one of these squares can invalidate
     * several later certificate branches before the opponent plays the gain. */
    int opponent = -side;
    for (int direction = 0; direction < 4; direction++) {
        int dx = fcDirections[direction][0];
        int dy = fcDirections[direction][1];
        for (int offset = -4; offset <= 0; offset++) {
            int opponentStones = 0;
            int ownStones = 0;
            bool containsGain = false;
            for (int step = 0; step < 5; step++) {
                int x = gainX + (offset + step) * dx;
                int y = gainY + (offset + step) * dy;
                if (!fc_inside(x, y)) {
                    ownStones++;
                    continue;
                }
                containsGain = containsGain || (x == gainX && y == gainY);
                opponentStones += board[x][y] == opponent;
                ownStones += board[x][y] == side;
            }
            if (!containsGain || ownStones > 0 || opponentStones < 2) continue;
            for (int step = 0; step < 5; step++) {
                int x = gainX + (offset + step) * dx;
                int y = gainY + (offset + step) * dy;
                if (!fc_inside(x, y) || board[x][y] != 0) continue;
                int dependencyScore = FC_WIN_SCORE / 2 +
                    opponentStones * 100000 +
                    fc_move_heuristic(board, x, y, side, profile);
                (void)fc_append_escape_candidate(
                    board, side, forbiddenBlack, x, y,
                    FC_ESCAPE_CERTIFICATE, dependencyScore,
                    out, count, capacity);
            }
        }
    }
}

static int fc_build_escape_candidates(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    const FCProofResult *opponentProof,
    const FCCandidate *baseMoves,
    int baseCount,
    int defaultX,
    int defaultY,
    FCEscapeCandidate *out,
    int capacity)
{
    int count = 0;
    if (opponentProof != NULL && opponentProof->certificateVerified) {
        for (int i = 0; i < opponentProof->certificateNodeCount; i++) {
            const FCProofNode *node = &opponentProof->certificate[i];
            if (node->x == defaultX && node->y == defaultY) continue;
            (void)fc_append_escape_candidate(
                board, side, forbiddenBlack,
                node->x, node->y, FC_ESCAPE_CERTIFICATE,
                FC_WIN_SCORE / 2 +
                    fc_move_heuristic(board, node->x, node->y, side, profile),
                out, &count, capacity);
            fc_append_certificate_line_dependencies(
                board, side, forbiddenBlack,
                node->x, node->y, profile, out, &count, capacity);
        }
    }
    for (int i = 0; i < baseCount; i++) {
        if (baseMoves[i].x == defaultX && baseMoves[i].y == defaultY) continue;
        int stage = baseMoves[i].tacticalClass == FC_TACTICAL_NORMAL
            ? FC_ESCAPE_ORDINARY : FC_ESCAPE_TACTICAL;
        (void)fc_append_escape_candidate(
            (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
            baseMoves[i].x, baseMoves[i].y, stage, baseMoves[i].score,
            out, &count, capacity);
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int x = 0; x < FC_BOARD_SIZE; x++) {
            for (int y = 0; y < FC_BOARD_SIZE; y++) {
                if (x == defaultX && y == defaultY) continue;
                bool nearby = fc_has_neighbor(
                    (const int (*)[FC_BOARD_SIZE])board, x, y, 2);
                if ((pass == 0) != nearby) continue;
                int stage = pass == 0 ? FC_ESCAPE_ORDINARY
                                      : FC_ESCAPE_ALL_LEGAL;
                (void)fc_append_escape_candidate(
                    (const int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack,
                    x, y, stage, fc_move_heuristic(board, x, y, side, profile),
                    out, &count, capacity);
            }
        }
    }
    qsort(out, (size_t)count, sizeof(out[0]), fc_compare_escape_candidate);
    return count;
}

static bool fc_proof_is_verified_loss(const FCProofResult *proof)
{
    return proof->status == FC_PROOF_PROVEN_WIN &&
           proof->certificateVerified;
}

/* Loss-aware escape queries are independent proof obligations.  They must
 * not share a mutable DFPN session, but they do share the same absolute
 * decision deadline and aggregate node token budget. */
typedef struct FCEscapeProofBatch FCEscapeProofBatch;

typedef struct {
    FCEscapeProofBatch *batch;
    FCProofDiagnostics diagnostics;
} FCEscapeProofWorker;

struct FCEscapeProofBatch {
    const int (*board)[FC_BOARD_SIZE];
    int attacker;
    bool forbiddenBlack;
    int searchClass;
    int maxDepth;
    uint64_t perJobNodeBudget;
    uint64_t aggregateNodeBudget;
    uint32_t perJobTimeBudgetMs;
    size_t transpositionCapacity;
    FCAIProfile profile;
    FCDecisionLedger *ledger;
    FCEscapeCandidate *candidates;
    int candidateCount;
    FCProofResult *results;
    bool *completed;
    bool *safe;
    _Atomic int nextJob;
    _Atomic int completedJobs;
    _Atomic int activeWorkers;
    _Atomic int maxConcurrentWorkers;
    _Atomic uint64_t consumedNodeTokens;
    _Atomic bool stopRequested;
    double absoluteDeadlineMilliseconds;
};

static bool fc_replay_scoped_disproof_in_place(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *proof);

static void *fc_escape_proof_worker_main(void *opaque)
{
    FCEscapeProofWorker *worker = opaque;
    FCEscapeProofBatch *batch = worker->batch;
    double priorDeadline = fcActiveDecisionDeadlineMilliseconds;
    int priorFilterX = fcProofRootFilterX;
    int priorFilterY = fcProofRootFilterY;
    _Atomic uint64_t *priorParallelCounter = fcActiveParallelNodeCounter;
    uint64_t priorParallelBudget = fcActiveParallelNodeBudget;
    FCDecisionLedger *priorLedger = fcActiveDecisionLedger;
    uint64_t priorParallelTokens = fcParallelNodeTokensRemaining;
    uint64_t priorLedgerTokens = fcDecisionLedgerNodeTokensRemaining;
    uint32_t priorBlockSize = fcActiveParallelNodeBlockSize;
    bool priorBlocksEnabled = fcActiveParallelTokenBlocksEnabled;
    fc_proof_diagnostics_reset();
    fcActiveDecisionDeadlineMilliseconds =
        batch->absoluteDeadlineMilliseconds;
    fcActiveDecisionLedger = batch->ledger;
    fcActiveParallelNodeCounter = &batch->consumedNodeTokens;
    fcActiveParallelNodeBudget = batch->aggregateNodeBudget;
    fcParallelNodeTokensRemaining = 0;
    fcDecisionLedgerNodeTokensRemaining = 0;
    fcActiveParallelNodeBlockSize = batch->profile.parallelTokenBlockSize;
    fcActiveParallelTokenBlocksEnabled =
        batch->profile.parallelTokenBlockEnabled;
    int active = atomic_fetch_add_explicit(
        &batch->activeWorkers, 1, memory_order_acq_rel) + 1;
    int observed = atomic_load_explicit(
        &batch->maxConcurrentWorkers, memory_order_relaxed);
    while (active > observed &&
           !atomic_compare_exchange_weak_explicit(
               &batch->maxConcurrentWorkers, &observed, active,
               memory_order_relaxed, memory_order_relaxed)) {
        /* The failed compare-exchange refreshes observed. */
    }

    bool reuseSession = batch->profile.persistentWorkerPoolEnabled;
    FCAIProfile jobProfile = batch->profile;
    jobProfile.parallelProofEnabled = false;
    jobProfile.proofWorkerCount = 1;
    jobProfile.proofParallelNodeBudget = 0;
    jobProfile.proofNodeBudget = batch->perJobNodeBudget;
    jobProfile.proofTimeBudgetMs = batch->perJobTimeBudgetMs;
    jobProfile.proofEmergencyTimeBudgetMs = batch->perJobTimeBudgetMs;
    FCProofSession session;
    FCProofSession *previousSession = fcActiveProofSession;
    bool sessionReady = false;

    for (;;) {
        if (atomic_load_explicit(&batch->stopRequested,
                                 memory_order_acquire))
            break;
        if (fc_now_milliseconds() >= batch->absoluteDeadlineMilliseconds)
            break;
        int index = atomic_fetch_add_explicit(
            &batch->nextJob, 1, memory_order_relaxed);
        if (index >= batch->candidateCount) break;
        FCEscapeCandidate candidate = batch->candidates[index];
        int alternative[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(alternative, batch->board, sizeof(alternative));
        memset(&batch->results[index], 0,
               sizeof(batch->results[index]));
        batch->results[index].x = -1;
        batch->results[index].y = -1;
        batch->results[index].searchClass = batch->searchClass;
        batch->results[index].completedDepth = batch->maxDepth;
        bool safe = fc_move_is_safe(
            alternative, candidate.x, candidate.y,
            batch->attacker, batch->forbiddenBlack);
        batch->safe[index] = safe;
        if (safe && fc_make_move(
                alternative, candidate.x, candidate.y,
                batch->attacker, batch->forbiddenBlack)) {
            bool queryReady = sessionReady;
            if (!reuseSession || !sessionReady) {
                sessionReady = fc_proof_session_begin(
                    &session, &jobProfile,
                    (const int (*)[FC_BOARD_SIZE])alternative,
                    batch->forbiddenBlack);
                queryReady = sessionReady;
            } else if (!fc_proof_session_reset_for_query(
                           &session, &jobProfile,
                           (const int (*)[FC_BOARD_SIZE])alternative,
                           batch->forbiddenBlack)) {
                fc_proof_session_end(&session, previousSession);
                sessionReady = false;
                queryReady = false;
            }
            if (queryReady) {
                fcProofRootFilterX = -1;
                fcProofRootFilterY = -1;
                (void)fc_prove_forced_win(
                    (const int (*)[FC_BOARD_SIZE])alternative,
                    -batch->attacker, batch->forbiddenBlack,
                    batch->searchClass, batch->maxDepth,
                    batch->perJobNodeBudget, batch->perJobTimeBudgetMs,
                    batch->transpositionCapacity,
                    &batch->results[index]);
                fc_parallel_node_token_flush();
                if (fcActiveDecisionLedger != NULL &&
                    batch->results[index].status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE &&
                    fc_replay_scoped_disproof_in_place(
                        (const int (*)[FC_BOARD_SIZE])alternative,
                        -batch->attacker, batch->forbiddenBlack,
                        &batch->results[index])) {
                    atomic_store_explicit(&batch->stopRequested, true,
                                          memory_order_release);
                    fcProofDiagnostics.parallelEarlyStops++;
                }
            } else {
                batch->results[index].status = FC_PROOF_UNKNOWN;
                batch->results[index].budgetExhausted = true;
            }
            if (!reuseSession && sessionReady) {
                fc_proof_session_end(&session, previousSession);
                sessionReady = false;
            }
        } else {
            batch->results[index].status = FC_PROOF_UNKNOWN;
        }
        batch->completed[index] = true;
        atomic_fetch_add_explicit(
            &batch->completedJobs, 1, memory_order_relaxed);
        fc_parallel_node_token_flush();
    }

    if (reuseSession && sessionReady)
        fc_proof_session_end(&session, previousSession);
    fc_parallel_node_token_flush();
    worker->diagnostics = fc_proof_diagnostics_get();
    fcProofRootFilterX = priorFilterX;
    fcProofRootFilterY = priorFilterY;
    fcActiveDecisionDeadlineMilliseconds = priorDeadline;
    fcActiveParallelNodeCounter = priorParallelCounter;
    fcActiveParallelNodeBudget = priorParallelBudget;
    fcActiveDecisionLedger = priorLedger;
    fcParallelNodeTokensRemaining = priorParallelTokens;
    fcDecisionLedgerNodeTokensRemaining = priorLedgerTokens;
    fcActiveParallelNodeBlockSize = priorBlockSize;
    fcActiveParallelTokenBlocksEnabled = priorBlocksEnabled;
    atomic_fetch_sub_explicit(&batch->activeWorkers, 1, memory_order_acq_rel);
    return NULL;
}

typedef struct {
    FCEscapeProofBatch *batch;
    FCEscapeProofWorker *workers;
} FCEscapeProofPoolTask;

static void fc_escape_proof_pool_task(void *opaque, int workerSlot)
{
    FCEscapeProofPoolTask *task = opaque;
    if (task == NULL || task->batch == NULL || task->workers == NULL ||
        workerSlot < 0 || workerSlot >= FC_PARALLEL_MAX_WORKERS) return;
    task->workers[workerSlot].batch = task->batch;
    (void)fc_escape_proof_worker_main(&task->workers[workerSlot]);
}

static int fc_parallel_escape_search(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    FCEscapeCandidate *candidates,
    int candidateCount,
    uint64_t aggregateNodeBudget,
    uint32_t reservedTimeBudgetMs,
    FCProofResult *results,
    bool *completed,
    bool *safe)
{
    if (board == NULL || profile == NULL || candidates == NULL ||
        results == NULL || completed == NULL || safe == NULL ||
        candidateCount <= 0) return 0;
    memset(results, 0, (size_t)candidateCount * sizeof(results[0]));
    memset(completed, 0, (size_t)candidateCount * sizeof(completed[0]));
    memset(safe, 0, (size_t)candidateCount * sizeof(safe[0]));
    double now = fc_now_milliseconds();
    double deadline = fcActiveDecisionDeadlineMilliseconds;
    if (reservedTimeBudgetMs > 0) {
        double reserved = now + (double)reservedTimeBudgetMs;
        if (deadline <= 0.0 || reserved < deadline) deadline = reserved;
    }
    if (deadline > 0.0 && now >= deadline) {
        fcProofDiagnostics.parallelEscapeBudgetExhausted++;
        return 0;
    }
    int workerCount = profile->parallelProofEnabled
        ? fc_effective_worker_count(profile) : 1;
    if (workerCount < 1) workerCount = 1;
    if (workerCount > 8) workerCount = 8;
    if (workerCount > candidateCount) workerCount = candidateCount;
    uint64_t perJobBudget = profile->proofNodeBudget / 2;
    if (perJobBudget == 0) perJobBudget = 1;
    if (aggregateNodeBudget == 0) aggregateNodeBudget = perJobBudget;
    FCEscapeProofBatch batch;
    memset(&batch, 0, sizeof(batch));
    batch.board = board;
    batch.attacker = side;
    batch.forbiddenBlack = forbiddenBlack;
    batch.searchClass = profile->proofSearchClass;
    batch.maxDepth = profile->proofMaxDepth;
    batch.perJobNodeBudget = perJobBudget;
    batch.aggregateNodeBudget = aggregateNodeBudget;
    batch.perJobTimeBudgetMs = profile->proofTimeBudgetMs;
    if (batch.perJobTimeBudgetMs == 0 ||
        (reservedTimeBudgetMs > 0 &&
         batch.perJobTimeBudgetMs > reservedTimeBudgetMs))
        batch.perJobTimeBudgetMs = reservedTimeBudgetMs;
    if (batch.perJobTimeBudgetMs == 0) batch.perJobTimeBudgetMs = 1;
    batch.transpositionCapacity = profile->proofTranspositionCapacity;
    batch.profile = *profile;
    batch.ledger = fcActiveDecisionLedger;
    batch.candidates = candidates;
    batch.candidateCount = candidateCount;
    batch.results = results;
    batch.completed = completed;
    batch.safe = safe;
    atomic_init(&batch.nextJob, 0);
    atomic_init(&batch.completedJobs, 0);
    atomic_init(&batch.activeWorkers, 0);
    atomic_init(&batch.maxConcurrentWorkers, 0);
    atomic_init(&batch.consumedNodeTokens, 0);
    atomic_init(&batch.stopRequested, false);
    batch.absoluteDeadlineMilliseconds = deadline;

    FCProofDiagnostics priorDiagnostics = fcProofDiagnostics;
    pthread_t threads[FC_PARALLEL_MAX_WORKERS];
    FCEscapeProofWorker workers[FC_PARALLEL_MAX_WORKERS];
    int launched = 0;
    if (profile->persistentWorkerPoolEnabled) {
        FCEscapeProofPoolTask poolTask = {
            .batch = &batch, .workers = workers
        };
        launched = fc_worker_pool_dispatch(
            fc_escape_proof_pool_task, &poolTask, workerCount);
        if (launched > 0) {
            fcProofDiagnostics.parallelPoolDispatches++;
            fcProofDiagnostics.parallelPoolWorkersReused +=
                (uint64_t)launched;
            if (launched < workerCount)
                fcProofDiagnostics.parallelPoolFallbacks++;
        } else {
            fcProofDiagnostics.parallelPoolFallbacks++;
        }
    } else {
        for (int i = 0; i < workerCount; i++) {
            workers[i].batch = &batch;
            if (pthread_create(&threads[i], NULL,
                               fc_escape_proof_worker_main,
                               &workers[i]) != 0) break;
            launched++;
        }
    }
    if (launched == 0) {
        workers[0].batch = &batch;
        (void)fc_escape_proof_worker_main(&workers[0]);
        FCProofDiagnostics workerDiagnostics = workers[0].diagnostics;
        fcProofDiagnostics = priorDiagnostics;
        fc_proof_diagnostics_merge(&fcProofDiagnostics, &workerDiagnostics);
    } else {
        if (!profile->persistentWorkerPoolEnabled) {
            for (int i = 0; i < launched; i++)
                (void)pthread_join(threads[i], NULL);
        }
        for (int i = 0; i < launched; i++) {
            fc_proof_diagnostics_merge(
                &fcProofDiagnostics, &workers[i].diagnostics);
        }
    }
    fcProofDiagnostics.parallelEscapeBatches++;
    fcProofDiagnostics.parallelEscapeWorkersLaunched +=
        (uint64_t)launched;
    fcProofDiagnostics.parallelEscapeJobs +=
        (uint64_t)atomic_load_explicit(&batch.nextJob, memory_order_relaxed);
    fcProofDiagnostics.parallelEscapeJobsCompleted +=
        (uint64_t)atomic_load_explicit(&batch.completedJobs,
                                       memory_order_relaxed);
    uint64_t maxConcurrent = (uint64_t)atomic_load_explicit(
        &batch.maxConcurrentWorkers, memory_order_relaxed);
    if (maxConcurrent > fcProofDiagnostics.parallelEscapeMaxConcurrentWorkers)
        fcProofDiagnostics.parallelEscapeMaxConcurrentWorkers = maxConcurrent;
    if (fc_now_milliseconds() >= deadline ||
        atomic_load_explicit(&batch.consumedNodeTokens,
                             memory_order_relaxed) >= aggregateNodeBudget)
        fcProofDiagnostics.parallelEscapeBudgetExhausted++;
    return atomic_load_explicit(&batch.completedJobs, memory_order_relaxed);
}

static bool fc_replay_scoped_disproof_in_place(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCProofResult *proof)
{
    FCProofSession *previousSession = fcActiveProofSession;
    fcActiveProofSession = NULL;
    bool valid = fc_verify_scoped_disproof(
        board, attacker, forbiddenBlack, proof);
    fcActiveProofSession = previousSession;
    return valid;
}

static int fc_quiet_dependency_count(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int x,
    int y,
    int side,
    bool forbiddenBlack)
{
    if (!fc_make_move(board, x, y, side, forbiddenBlack)) return 0;
    bool terminal = fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                                x, y, side);
    int dependencies = 0;
    if (!terminal) {
        for (int d = 0; d < 4; d++) {
            int stones = 0;
            int empty = 0;
            for (int offset = -4; offset <= 4; offset++) {
                int px = x + offset * fcDirections[d][0];
                int py = y + offset * fcDirections[d][1];
                if (!fc_inside(px, py)) continue;
                stones += board[px][py] == side;
                empty += board[px][py] == 0;
            }
            if (stones >= 3 && empty >= 2) dependencies++;
        }
    }
    fc_unmake_move(board, x, y);
    return dependencies;
}

static void fc_examine_quiet_roots(
    int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    const FCCandidate *candidates,
    int candidateCount,
    FCAnalysisResult *result)
{
    int limit = profile->proofQuietRootLimit;
    if (limit <= 0) return;
    bool eligible = false;
    for (int i = 0; i < candidateCount && result->quietRootsExamined < limit;
         i++) {
        if (!candidates[i].safe ||
            candidates[i].tacticalClass != FC_TACTICAL_NORMAL ||
            fc_quiet_dependency_count(board, candidates[i].x,
                                      candidates[i].y, side,
                                      forbiddenBlack) < 2) continue;
        if (!eligible) {
            eligible = true;
            fcProofDiagnostics.quietEligibleDecisions++;
            fcProofDiagnostics.stageQuietQueries++;
        }
        int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(after, board, sizeof(after));
        if (!fc_make_move(after, candidates[i].x, candidates[i].y,
                          side, forbiddenBlack)) continue;
        result->quietRootsExamined++;
        fcProofDiagnostics.quietRootsExamined++;
        FCProofResult counterProof;
        uint64_t quietBudget = profile->proofParallelNodeBudget > 0
            ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
        uint64_t quietNodes = quietBudget / 8;
        if (quietNodes == 0) quietNodes = 1;
        uint32_t quietTime = profile->proofTimeBudgetMs;
        uint32_t quietMaximum = fc_profile_proof_path_enabled(profile)
            ? 600 : 80;
        if (quietTime == 0 || quietTime > quietMaximum)
            quietTime = quietMaximum;
        (void)fc_parallel_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])after, -side, forbiddenBlack,
            profile->proofSearchClass, profile->proofMaxDepth,
            quietNodes, quietTime, profile, &counterProof);
        if (counterProof.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
            fcProofDiagnostics.quietCompletedDisproofs++;
            bool replacesVerifiedLoss =
                result->tacticalClass == FC_TACTICAL_FORCED_DEFENSE &&
                result->proofStatus == FC_PROOF_PROVEN_WIN &&
                result->proofCertificateVerified &&
                counterProof.searchClass >= result->proofSearchClass;
            if (replacesVerifiedLoss) {
                result->x = candidates[i].x;
                result->y = candidates[i].y;
                result->tacticalClass = FC_TACTICAL_FORCED_DEFENSE;
                result->overrideReason = FC_OVERRIDE_PROVEN_DEFENSE;
                result->proofStatus = counterProof.status;
                result->proofSearchClass = counterProof.searchClass;
                result->proofDistance = counterProof.distance;
                result->proofNodes = counterProof.nodes;
                result->proofNumber = counterProof.proofNumber;
                result->disproofNumber = counterProof.disproofNumber;
                result->proofCertificateId = 0;
                result->proofCertificateVerified = false;
                result->opponentAfterSelectedStatus = counterProof.status;
                result->opponentAfterSelectedDistance =
                    counterProof.distance;
                result->quietThreatSelected = true;
                result->lossReason = FC_LOSS_NONE;
                return;
            }
        } else if (counterProof.status == FC_PROOF_PROVEN_WIN &&
                   counterProof.certificateVerified) {
            fcProofDiagnostics.quietCompletedProofs++;
        } else {
            fcProofDiagnostics.quietUnknowns++;
        }
        /* A quiet structural root is diagnostic-only until an own-win
         * certificate exists.  Counter-disproving the opponent alone is not
         * sufficient evidence to replace the frozen baseline. */
    }
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
    result->randomSelectedRank = 0;
    result->randomEligibilityVerified = true;
    result->hybridComponent = FC_HYBRID_COMPONENT_NONE;
    result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
    result->candidateCoverageComplete = false;

    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    FCSearchContext context;
    memset(&context, 0, sizeof(context));
    context.profile = *profile;
    context.forbiddenBlack = forbiddenBlack;
    context.startedMilliseconds = fc_now_milliseconds();
    context.tableCapacity = fc_profile_proof_path_enabled(profile)
        ? 0 : profile->transpositionCapacity;
    if (context.tableCapacity > 0) {
        context.table = calloc(context.tableCapacity, sizeof(FCTTEntry));
        fcProofDiagnostics.allocations++;
        fcProofDiagnostics.allocatedBytes += context.tableCapacity *
                                             sizeof(FCTTEntry);
        fcProofDiagnostics.clearedBytes += context.tableCapacity *
                                           sizeof(FCTTEntry);
        if (context.table == NULL) context.tableCapacity = 0;
    }
    FCProofSession proofSession;
    FCProofSession *previousProofSession = fcActiveProofSession;
    bool proofSessionStarted = false;
    if (fc_profile_proof_path_enabled(profile) &&
        fcActiveProofSession == NULL) {
        proofSessionStarted = fc_proof_session_begin(
            &proofSession, profile, board, forbiddenBlack);
    }

    FCCandidate baseMoves[FC_MAX_CANDIDATES];
    int count = fc_generate_candidates(mutableBoard, side, forbiddenBlack,
                                       profile, baseMoves,
                                       FC_MAX_CANDIDATES, false);
    bool forkLayerActive = false;
    if (count == 0 && profile->forkFirstRecoveryEnabled) {
        forkLayerActive = fc_apply_fork_candidate_layer(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side,
            forbiddenBlack, profile, baseMoves, &count,
            FC_MAX_CANDIDATES, hintX, hintY, -1, -1, result);
    }
    if (count == 0) {
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        free(context.table);
        if (profile->recoverySearchEnabled) {
            int fallbackX = -1;
            int fallbackY = -1;
            if (fc_find_full_board_legal_fallback(
                    (const int (*)[FC_BOARD_SIZE])mutableBoard,
                    side, forbiddenBlack, &fallbackX, &fallbackY)) {
                result->x = fallbackX;
                result->y = fallbackY;
                result->fallbackUsed = true;
                result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
                result->lossReason = FC_LOSS_BUDGET_UNKNOWN;
                result->stats.budgetExhausted = true;
                result->stats.elapsedMilliseconds =
                    fc_now_milliseconds() - context.startedMilliseconds;
                fcProofDiagnostics.decisionUnknowns++;
                fcProofDiagnostics.decisionFallbacks++;
                return true;
            }
            result->decisionStatus = FC_DECISION_NO_LEGAL_MOVE;
            result->provenLoss = true;
            result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
            result->lossReason = FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED;
            fcProofDiagnostics.decisionNoLegalMoves++;
            return false;
        }
        /* Frozen profiles retain their historical boolean contract. */
        result->decisionStatus = FC_DECISION_VERIFIED_LOSS;
        result->provenLoss = true;
        result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
        result->lossReason = FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED;
        fcProofDiagnostics.decisionVerifiedLosses++;
        return false;
    }

    int bookX = -1;
    int bookY = -1;
    int bookId = -1;
    int bookPly = -1;
    bool bookQueryAllowed = !profile->openingBookEnabled ||
        fcActiveDecisionLedger == NULL ||
        (fc_decision_ledger_reserve_queries(fcActiveDecisionLedger, 1) &&
         fc_decision_ledger_consume_query(fcActiveDecisionLedger));
    bool hasBook = profile->openingBookEnabled && bookQueryAllowed &&
        fc_opening_book_lookup(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side,
            forbiddenBlack, seed, &bookX, &bookY, &bookId, &bookPly);
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
                0.0, 0, FC_FORK_RISK_UNKNOWN, false
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
                0.0, 0, FC_FORK_RISK_UNKNOWN, false
            };
            qsort(baseMoves, (size_t)count, sizeof(FCCandidate),
                  fc_compare_candidate);
        }
    }

    if (profile->forkFirstRecoveryEnabled && !forkLayerActive) {
        forkLayerActive = fc_apply_fork_candidate_layer(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side,
            forbiddenBlack, profile, baseMoves, &count,
            FC_MAX_CANDIDATES,
            useHint ? hintX : -1, useHint ? hintY : -1,
            hasBook ? bookX : -1, hasBook ? bookY : -1, result);
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
    int advisoryDefaultX = defaultX;
    int advisoryDefaultY = defaultY;
    if (forkLayerActive &&
        fc_fork_risk_is_nonfork((FCForkRisk)baseMoves[0].forkRisk)) {
        defaultX = baseMoves[0].x;
        defaultY = baseMoves[0].y;
    }
    result->defaultSource = defaultSource;
    result->defaultX = advisoryDefaultX;
    result->defaultY = advisoryDefaultY;
    result->bookId = hasBook ? bookId : -1;
    result->bookPly = hasBook ? bookPly : -1;

    if (profile->proofEnabled &&
        tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
        tacticalClass != FC_TACTICAL_MUST_DEFEND) {
        FCProofResult ownProof;
        uint64_t ownBudget = profile->proofParallelNodeBudget > 0
            ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
        bool ownWin = fc_parallel_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side, forbiddenBlack,
            profile->proofSearchClass, profile->proofMaxDepth,
            ownBudget, profile->proofTimeBudgetMs, profile, &ownProof);
        int selectedX = defaultX;
        int selectedY = defaultY;
        int overrideReason = forkLayerActive &&
            (defaultX != advisoryDefaultX || defaultY != advisoryDefaultY)
            ? FC_OVERRIDE_FORK_SAFE_RECOVERY : FC_OVERRIDE_NONE;
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
            bool opponentWins = defaultLegal &&
                fc_parallel_prove_forced_win(
                (const int (*)[FC_BOARD_SIZE])afterDefault, -side,
                forbiddenBlack, profile->proofSearchClass,
                profile->proofMaxDepth, ownBudget,
                profile->proofTimeBudgetMs, profile, &opponentProof);
            if (opponentWins && opponentProof.certificateVerified) {
                bool escaped = false;
                if (profile->lossAwareEnabled) {
                    FCEscapeCandidate options[FC_BOARD_SIZE * FC_BOARD_SIZE];
                    int optionCount = fc_build_escape_candidates(
                        mutableBoard, side, forbiddenBlack, profile,
                        &opponentProof, baseMoves, count, defaultX, defaultY,
                        options, FC_BOARD_SIZE * FC_BOARD_SIZE);
                    int limit = profile->proofEscapeCandidateLimit > 0
                        ? profile->proofEscapeCandidateLimit : optionCount;
                    if (limit > optionCount) limit = optionCount;
                    int bestUnknown = -1;
                    int bestLoss = -1;
                    FCProofResult bestUnknownProof;
                    FCProofResult bestLossProof;
                    memset(&bestUnknownProof, 0, sizeof(bestUnknownProof));
                    memset(&bestLossProof, 0, sizeof(bestLossProof));
                    FCProofResult *escapeProofs = calloc(
                        (size_t)limit, sizeof(*escapeProofs));
                    bool *escapeCompleted = calloc(
                        (size_t)limit, sizeof(*escapeCompleted));
                    bool *escapeSafe = calloc(
                        (size_t)limit, sizeof(*escapeSafe));
                    uint64_t escapeBudget = profile->proofParallelNodeBudget > 0
                        ? profile->proofParallelNodeBudget / 2
                        : profile->proofNodeBudget / 2;
                    if (escapeProofs != NULL && escapeCompleted != NULL &&
                        escapeSafe != NULL) {
                        uint32_t escapeReserve = fcActiveDecisionLedger != NULL
                            ? profile->proofTimeBudgetMs
                            : profile->proofEmergencyTimeBudgetMs;
                        (void)fc_parallel_escape_search(
                            (const int (*)[FC_BOARD_SIZE])mutableBoard,
                            side, forbiddenBlack, profile, options, limit,
                            escapeBudget, escapeReserve,
                            escapeProofs, escapeCompleted, escapeSafe);
                    }
                    if (escapeProofs != NULL && escapeCompleted != NULL &&
                        escapeSafe != NULL) for (int i = 0; i < limit; i++) {
                        if (!escapeCompleted[i]) continue;
                        int x = options[i].x;
                        int y = options[i].y;
                        result->escapeAlternativesExamined++;
                        if (!escapeSafe[i]) {
                            /* An immediately losing legal move is a completed
                             * loss class even though it needs no VCT query. */
                            result->escapeVerifiedLossCount++;
                            continue;
                        }
                        FCProofResult replyProof = escapeProofs[i];
                        if (replyProof.status ==
                                FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
                            int replayBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
                            memcpy(replayBoard, mutableBoard,
                                   sizeof(replayBoard));
                            bool replayLegal = fc_make_move(
                                replayBoard, x, y, side, forbiddenBlack);
                            if (!replayLegal ||
                                !fc_replay_scoped_disproof_in_place(
                                    (const int (*)[FC_BOARD_SIZE])replayBoard,
                                    -side, forbiddenBlack, &replyProof)) {
                                replyProof.status = FC_PROOF_UNKNOWN;
                            }
                        }
                        if (replyProof.status ==
                                FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
                            result->escapeScopedDisproofCount++;
                            selectedX = x;
                            selectedY = y;
                            selectedProof = replyProof;
                            overrideReason = FC_OVERRIDE_PROVEN_DEFENSE;
                            result->escapeStage = options[i].stage;
                            escaped = true;
                            break;
                        }
                        if (fc_proof_is_verified_loss(&replyProof)) {
                            result->escapeVerifiedLossCount++;
                            if (bestLoss < 0 ||
                                replyProof.distance > bestLossProof.distance) {
                                bestLoss = i;
                                bestLossProof = replyProof;
                            }
                        } else {
                            result->escapeUnknownCount++;
                            if (bestUnknown < 0 ||
                                replyProof.proofNumber >
                                    bestUnknownProof.proofNumber) {
                                bestUnknown = i;
                                bestUnknownProof = replyProof;
                            }
                        }
                    }
                    free(escapeProofs);
                    free(escapeCompleted);
                    free(escapeSafe);
                    if (!escaped && bestUnknown >= 0) {
                        selectedX = options[bestUnknown].x;
                        selectedY = options[bestUnknown].y;
                        selectedProof = bestUnknownProof;
                        overrideReason = FC_OVERRIDE_UNPROVEN_ESCAPE;
                        result->escapeStage = options[bestUnknown].stage;
                        result->lossReason = FC_LOSS_BUDGET_UNKNOWN;
                        escaped = true;
                    } else if (!escaped && bestLoss >= 0 &&
                               bestLossProof.distance > opponentProof.distance) {
                        selectedX = options[bestLoss].x;
                        selectedY = options[bestLoss].y;
                        selectedProof = bestLossProof;
                        overrideReason = FC_OVERRIDE_LONGEST_SURVIVAL;
                        result->escapeStage = options[bestLoss].stage;
                        result->lossReason =
                            result->escapeAlternativesExamined == optionCount &&
                            result->escapeUnknownCount == 0
                                ? FC_LOSS_ALL_EXAMINED_VERIFIED
                                : FC_LOSS_SELECTED_VERIFIED;
                        escaped = true;
                    } else if (!escaped) {
                        selectedProof = opponentProof;
                        result->lossReason = result->escapeUnknownCount > 0
                            ? FC_LOSS_BUDGET_UNKNOWN
                            : FC_LOSS_SELECTED_VERIFIED;
                    }
                    tacticalClass = FC_TACTICAL_FORCED_DEFENSE;
                    result->opponentAfterSelectedStatus = selectedProof.status;
                    result->opponentAfterSelectedDistance =
                        selectedProof.distance;
                } else {
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
                        (void)fc_parallel_prove_forced_win(
                            (const int (*)[FC_BOARD_SIZE])alternative, -side,
                            forbiddenBlack, profile->proofSearchClass,
                            profile->proofMaxDepth, profile->proofNodeBudget,
                            profile->proofTimeBudgetMs, profile, &replyProof);
                        if (replyProof.status !=
                                FC_PROOF_NO_FORCED_WIN_IN_SCOPE) continue;
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
            } else if (profile->quietThreatEnabled && defaultLegal &&
                       opponentProof.status != FC_PROOF_UNKNOWN) {
                fc_examine_quiet_roots(
                    mutableBoard, side, forbiddenBlack, profile,
                    baseMoves, count, result);
            }
        }
        result->x = selectedX;
        result->y = selectedY;
        result->tacticalClass = tacticalClass;
        result->overrideReason = overrideReason;
        if (forkLayerActive &&
            (result->x != advisoryDefaultX || result->y != advisoryDefaultY) &&
            result->tacticalClass == FC_TACTICAL_NORMAL &&
            result->overrideReason == FC_OVERRIDE_NONE)
            result->overrideReason = FC_OVERRIDE_FORK_SAFE_RECOVERY;
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
        result->randomEligibilityVerified = !result->randomSelectionUsed;
        result->stats.nodes = selectedProof.nodes;
        result->stats.transpositionHits = selectedProof.transpositionHits;
        result->stats.completedDepth = selectedProof.completedDepth;
        result->stats.budgetExhausted = selectedProof.budgetExhausted;
        result->stats.elapsedMilliseconds = fc_now_milliseconds()
                                            - context.startedMilliseconds;
        result->decisionStatus = ownWin &&
            selectedProof.status == FC_PROOF_PROVEN_WIN &&
            selectedProof.certificateVerified
            ? FC_DECISION_VERIFIED_WIN
            : FC_DECISION_UNKNOWN_OR_DEADLINE;
        if (result->decisionStatus == FC_DECISION_UNKNOWN_OR_DEADLINE)
            fcProofDiagnostics.decisionUnknowns++;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        free(context.table);
        if (forkLayerActive)
            fc_record_selected_fork_telemetry(
                result, advisoryDefaultX, advisoryDefaultY);
        bool legal = fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                                      result->x, result->y, side,
                                      forbiddenBlack);
        if (!legal && profile->recoverySearchEnabled) {
            if (fc_find_full_board_legal_fallback(
                    (const int (*)[FC_BOARD_SIZE])board,
                    side, forbiddenBlack, &result->x, &result->y)) {
                result->fallbackUsed = true;
                result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
                result->provenLoss = false;
                result->stats.budgetExhausted = true;
                fcProofDiagnostics.decisionFallbacks++;
                return true;
            }
            result->decisionStatus = FC_DECISION_NO_LEGAL_MOVE;
            result->provenLoss = true;
            fcProofDiagnostics.decisionNoLegalMoves++;
            return false;
        }
        return legal;
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
          forkLayerActive ? fc_compare_fork_candidate
                          : fc_compare_searched_candidate);
    for (int i = 0; i < count; i++) {
        if (forkLayerActive) {
            completed[i].safe = completed[i].forkRisk == FC_FORK_RISK_SAFE ||
                                completed[i].forkRisk == FC_FORK_RISK_OWN_WIN;
        } else {
            completed[i].safe = fc_move_is_safe(mutableBoard,
                                                completed[i].x,
                                                completed[i].y,
                                                side, forbiddenBlack);
        }
        if (completed[i].score <= -FC_WIN_SCORE + 1000) {
            completed[i].safe = false;
        }
    }
    bool anySafe = false;
    for (int i = 0; i < count; i++) {
        anySafe = anySafe || completed[i].safe ||
            (forkLayerActive &&
             fc_fork_risk_is_nonfork(completed[i].forkRisk));
    }
    if (!anySafe && !forkLayerActive) {
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        free(context.table);
        if (profile->recoverySearchEnabled) {
            int fallbackX = -1;
            int fallbackY = -1;
            if (fc_find_full_board_legal_fallback(
                    (const int (*)[FC_BOARD_SIZE])mutableBoard,
                    side, forbiddenBlack, &fallbackX, &fallbackY)) {
                result->x = fallbackX;
                result->y = fallbackY;
                result->fallbackUsed = true;
                result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
                result->lossReason = FC_LOSS_BUDGET_UNKNOWN;
                result->stats.budgetExhausted = true;
                result->stats.elapsedMilliseconds =
                    fc_now_milliseconds() - context.startedMilliseconds;
                fcProofDiagnostics.decisionUnknowns++;
                fcProofDiagnostics.decisionFallbacks++;
                return true;
            }
            result->decisionStatus = FC_DECISION_NO_LEGAL_MOVE;
            result->provenLoss = true;
            result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
            result->lossReason = FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED;
            fcProofDiagnostics.decisionNoLegalMoves++;
            return false;
        }
        /* Frozen profiles retain their historical boolean contract. */
        result->decisionStatus = FC_DECISION_VERIFIED_LOSS;
        result->provenLoss = true;
        result->tacticalClass = FC_TACTICAL_PROVEN_LOSS;
        result->lossReason = FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED;
        fcProofDiagnostics.decisionVerifiedLosses++;
        return false;
    }
    int selected = fc_choose_candidate(completed, count, tacticalClass,
                                       fc_board_stone_count(board), profile,
                                       seed, randomMode);
    if (selected < 0 || selected >= count) selected = 0;
    result->x = completed[selected].x;
    result->y = completed[selected].y;
    result->score = completed[selected].score;
    result->tacticalClass = tacticalClass;
    result->overrideReason = tacticalClass == FC_TACTICAL_IMMEDIATE_WIN
        ? FC_OVERRIDE_IMMEDIATE_WIN
        : tacticalClass == FC_TACTICAL_MUST_DEFEND
        ? FC_OVERRIDE_MUST_DEFEND : FC_OVERRIDE_NONE;
    if (forkLayerActive && result->overrideReason == FC_OVERRIDE_NONE &&
        (result->x != advisoryDefaultX || result->y != advisoryDefaultY))
        result->overrideReason = FC_OVERRIDE_FORK_SAFE_RECOVERY;
    result->candidateCount = count;
    result->randomCandidateCount = count;
    result->randomSelectionUsed = randomMode == FC_RANDOM_USER_GAME &&
                                  selected != 0;
    result->randomSelectedRank = selected;
    result->randomEligibilityVerified = !result->randomSelectionUsed;
    memcpy(result->candidates, completed, (size_t)count * sizeof(FCCandidate));
    result->stats.nodes = context.nodes;
    result->stats.transpositionHits = context.hits;
    result->stats.cutoffs = context.cutoffs;
    result->stats.completedDepth = completedDepth;
    result->stats.budgetExhausted = context.aborted;
    result->stats.elapsedMilliseconds = fc_now_milliseconds()
                                            - context.startedMilliseconds;
    result->candidateCoverageComplete = forkLayerActive
        ? result->candidateCoverageComplete && !context.aborted
        : !context.aborted &&
          (tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
           tacticalClass == FC_TACTICAL_MUST_DEFEND);
    if (tacticalClass == FC_TACTICAL_IMMEDIATE_WIN) {
        result->decisionStatus = FC_DECISION_VERIFIED_WIN;
        fcProofDiagnostics.decisionVerifiedWins++;
    } else {
        result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
        /* A heuristic/partially searched result is still unknown even when
         * it finished before the clock.  Count the certainty state, not only
         * wall-clock aborts, so diagnostics do not imply a proof. */
        fcProofDiagnostics.decisionUnknowns++;
    }
    if (proofSessionStarted)
        fc_proof_session_end(&proofSession, previousProofSession);
    free(context.table);
    if (!forkLayerActive && (hasBook || useHint) &&
        tacticalClass != FC_TACTICAL_IMMEDIATE_WIN &&
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
    if (forkLayerActive)
        fc_record_selected_fork_telemetry(
            result, advisoryDefaultX, advisoryDefaultY);
    bool legal = fc_inside(result->x, result->y) &&
        fc_is_legal_move((const int (*)[FC_BOARD_SIZE])board,
                         result->x, result->y, side, forbiddenBlack);
    if (!legal && profile->recoverySearchEnabled) {
        if (fc_find_full_board_legal_fallback(
                (const int (*)[FC_BOARD_SIZE])board,
                side, forbiddenBlack, &result->x, &result->y)) {
            result->fallbackUsed = true;
            result->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
            result->provenLoss = false;
            result->stats.budgetExhausted = true;
            fcProofDiagnostics.decisionFallbacks++;
            return true;
        }
        result->decisionStatus = FC_DECISION_NO_LEGAL_MOVE;
        result->provenLoss = true;
        fcProofDiagnostics.decisionNoLegalMoves++;
        return false;
    }
    return legal;
}

static void fc_finalize_random_telemetry(FCAnalysisResult *result)
{
    if (result == NULL) return;
    const FCCorpusCandidateTelemetry *selectedCorpus = NULL;
    for (int i = 0; i < result->corpusCandidateCount; i++) {
        if (result->corpusCandidates[i].x == result->x &&
            result->corpusCandidates[i].y == result->y) {
            selectedCorpus = &result->corpusCandidates[i];
            break;
        }
    }
    int proofStatus = selectedCorpus != NULL
        ? selectedCorpus->proofStatus : result->proofStatus;
    int proofSearchClass = selectedCorpus != NULL
        ? selectedCorpus->proofSearchClass : result->proofSearchClass;
    int proofDistance = selectedCorpus != NULL
        ? selectedCorpus->proofDistance : result->proofDistance;
    int completedDepth = selectedCorpus != NULL
        ? selectedCorpus->completedDepth : result->stats.completedDepth;
    int tacticalClass = selectedCorpus != NULL
        ? selectedCorpus->tacticalClass : result->tacticalClass;
    int trustTier = selectedCorpus != NULL
        ? selectedCorpus->trustTier : result->corpusTrustTier;
    int games = selectedCorpus != NULL
        ? selectedCorpus->games : result->corpusSupportGames;
    int events = selectedCorpus != NULL
        ? selectedCorpus->events : result->corpusSupportEvents;
    int sources = selectedCorpus != NULL
        ? selectedCorpus->sources : result->corpusSupportSources;
    int acceptance = selectedCorpus != NULL
        ? (selectedCorpus->accepted ? selectedCorpus->reason : -1)
        : (result->provenLoss ? result->lossReason : 0);
    bool selectedSafe = false;
    for (int i = 0; i < result->candidateCount; i++) {
        if (result->candidates[i].x == result->x &&
            result->candidates[i].y == result->y) {
            selectedSafe = result->candidates[i].safe;
            break;
        }
    }
    if (selectedCorpus != NULL && selectedCorpus->accepted)
        selectedSafe = true;
    uint64_t value = UINT64_C(0x53544f4348455155);
    value ^= (uint64_t)(proofStatus & 0xff) << 0;
    value ^= (uint64_t)(proofSearchClass & 0xff) << 8;
    value ^= (uint64_t)(proofDistance & 0xffff) << 16;
    value ^= (uint64_t)(tacticalClass & 0xff) << 32;
    value ^= (uint64_t)(trustTier & 0xff) << 40;
    value ^= (uint64_t)(result->lossReason & 0xff) << 48;
    value ^= (uint64_t)(result->provenLoss ? 1 : 0) << 56;
    value ^= (uint64_t)(selectedSafe ? 1 : 0) << 57;
    value ^= fc_mix64((uint64_t)(uint32_t)completedDepth);
    value ^= fc_mix64((uint64_t)(uint32_t)games << 1);
    value ^= fc_mix64((uint64_t)(uint32_t)events << 2);
    value ^= fc_mix64((uint64_t)(uint32_t)sources << 3);
    value ^= fc_mix64((uint64_t)(uint32_t)acceptance << 4);
    result->randomEquivalenceSignature = fc_mix64(value);
}

bool fc_analyze(const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
                int side,
                bool forbiddenBlack,
                const FCAIProfile *profile,
                uint64_t seed,
                FCRandomMode randomMode,
                FCAnalysisResult *result)
{
    bool found = fc_analyze_internal(board, side, forbiddenBlack, profile,
                                     seed, randomMode, -1, -1, false, result);
    fc_finalize_random_telemetry(result);
    return found;
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
    bool found = fc_analyze_internal(board, side, forbiddenBlack, profile,
                                     seed, randomMode, hintX, hintY, true,
                                     result);
    fc_finalize_random_telemetry(result);
    return found;
}

bool fc_analyze_four_star_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    FCAIProfile profile = fc_profile_frozen_four_star_control();
    bool found = fc_analyze_internal(board, side, forbiddenBlack, &profile,
                                     seed, randomMode, hintX, hintY, true,
                                     result);
    fc_finalize_random_telemetry(result);
    return found;
}

static int fc_completed_score_for_move(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    int x,
    int y,
    int *completedDepth,
    bool *completed,
    bool *safe)
{
    FCAIProfile profile = fc_profile_proof_guided(false);
    profile.proofEnabled = false;
    profile.eliteCorpusEnabled = false;
    FCAnalysisResult analysis;
    bool found = fc_analyze_internal(board, side, forbiddenBlack, &profile,
                                     seed, FC_RANDOM_EVALUATION,
                                     x, y, true, &analysis);
    if (completedDepth != NULL) *completedDepth = analysis.stats.completedDepth;
    if (completed != NULL) *completed = found && !analysis.stats.budgetExhausted &&
        (analysis.stats.completedDepth == profile.maxDepth ||
         analysis.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
         analysis.tacticalClass == FC_TACTICAL_MUST_DEFEND);
    int result = -FC_WIN_SCORE;
    bool moveSafe = false;
    for (int i = 0; i < analysis.candidateCount; i++) {
        if (analysis.candidates[i].x != x || analysis.candidates[i].y != y)
            continue;
        result = analysis.candidates[i].score;
        moveSafe = analysis.candidates[i].safe;
        break;
    }
    if (safe != NULL) *safe = moveSafe;
    return result;
}

static int fc_opponent_proof_after_move(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    int x,
    int y,
    FCProofResult *proof)
{
    int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(after, board, sizeof(after));
    memset(proof, 0, sizeof(*proof));
    if (!fc_make_move(after, x, y, side, forbiddenBlack))
        return FC_PROOF_UNKNOWN;
    if (fc_has_five((const int (*)[FC_BOARD_SIZE])after, x, y, side))
        return FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    uint64_t proofBudget = profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
    (void)fc_parallel_prove_forced_win(
        (const int (*)[FC_BOARD_SIZE])after, -side, forbiddenBlack,
        profile->proofSearchClass, profile->proofMaxDepth,
        proofBudget, profile->proofTimeBudgetMs, profile, proof);
    return proof->status;
}

static bool fc_opponent_guard_vct_signal(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int opponent,
    bool forbiddenBlack)
{
    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int count = fc_enumerate_threats(
        board, opponent, forbiddenBlack, FC_PROOF_SEARCH_VCT,
        threats, FC_MAX_THREATS, &overflow);
    if (overflow) return true;
    for (int i = 0; i < count; i++) {
        if (threats[i].severity >= FC_THREAT_OPEN_THREE)
            return true;
        for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
            if (threats[i].dependencyMask[word] != 0 ||
                threats[i].certificateZoneMask[word] != 0)
                return true;
        }
    }
    return false;
}

static bool fc_early_vcf_forcing_signal(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int opponent,
    bool forbiddenBlack)
{
    FCThreat threats[FC_MAX_THREATS];
    bool overflow = false;
    int count = fc_enumerate_threats(
        board, opponent, forbiddenBlack, FC_PROOF_SEARCH_VCF,
        threats, FC_MAX_THREATS, &overflow);
    if (overflow) return true;
    for (int i = 0; i < count; i++) {
        if (threats[i].severity >= FC_THREAT_FOUR)
            return true;
        for (int word = 0; word < FC_POSITION_BITSET_WORDS; word++) {
            if (threats[i].dependencyMask[word] != 0 ||
                threats[i].certificateZoneMask[word] != 0)
                return true;
        }
    }
    return false;
}

static uint64_t fc_early_vcf_position_key(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCAIProfile *profile)
{
    return fc_board_hash(board, attacker, forbiddenBlack,
                         0, 0, true, profile);
}

static void fc_early_vcf_cache_store(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    const FCProofResult *proof)
{
    if (fcActiveEarlyVCFCache == NULL || profile == NULL || proof == NULL ||
        proof->status != FC_PROOF_PROVEN_WIN ||
        !proof->certificateVerified) return;
    uint64_t key = fc_early_vcf_position_key(
        board, attacker, forbiddenBlack, profile);
    int index = fcActiveEarlyVCFCache->count;
    if (index >= FC_EARLY_VCF_CACHE_CAPACITY)
        index = FC_EARLY_VCF_CACHE_CAPACITY - 1;
    else
        fcActiveEarlyVCFCache->count++;
    FCEarlyVCFCacheEntry *entry = &fcActiveEarlyVCFCache->entries[index];
    *entry = (FCEarlyVCFCacheEntry){
        .valid = true,
        .positionKey = key,
        .side = attacker,
        .forbiddenBlack = forbiddenBlack,
        .searchClass = FC_PROOF_SEARCH_VCF,
        .completedDepth = proof->completedDepth,
        .engineVersion = profile->version,
        .proof = *proof
    };
}

static bool fc_early_vcf_cache_lookup_verified_win(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int attacker,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    FCProofResult *proof)
{
    if (fcActiveEarlyVCFCache == NULL || profile == NULL || proof == NULL)
        return false;
    uint64_t key = fc_early_vcf_position_key(
        board, attacker, forbiddenBlack, profile);
    for (int i = 0; i < fcActiveEarlyVCFCache->count; i++) {
        FCEarlyVCFCacheEntry *entry =
            &fcActiveEarlyVCFCache->entries[i];
        if (!entry->valid || entry->positionKey != key ||
            entry->side != attacker ||
            entry->forbiddenBlack != forbiddenBlack ||
            entry->searchClass != FC_PROOF_SEARCH_VCF ||
            entry->engineVersion == NULL || profile->version == NULL ||
            strcmp(entry->engineVersion, profile->version) != 0)
            continue;
        *proof = entry->proof;
        proof->elapsedMilliseconds = 0.0;
        if (!fc_verify_proof(board, attacker, forbiddenBlack, proof))
            continue;
        fcActiveEarlyVCFCache->hits++;
        fcProofDiagnostics.earlyVCFCacheHits++;
        return true;
    }
    return false;
}

bool fc_audit_opponent_micro_vcf_after_move(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    int x,
    int y,
    FCOpponentGuardAudit *audit,
    int *effectiveDepth,
    bool *adaptiveEscalated)
{
    if (board == NULL || profile == NULL || audit == NULL) return false;
    memset(audit, 0, sizeof(*audit));
    audit->status = FC_PROOF_UNKNOWN;
    audit->completedClass = FC_GUARD_CLASS_UNKNOWN;
    audit->vcf.x = audit->vcf.y = -1;
    audit->vct.x = audit->vct.y = -1;
    if (effectiveDepth != NULL) *effectiveDepth = 0;
    if (adaptiveEscalated != NULL) *adaptiveEscalated = false;
    int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(after, board, sizeof(after));
    if (!fc_make_move(after, x, y, side, forbiddenBlack)) {
        audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
        return false;
    }
    audit->placementLegal = true;
    audit->ownImmediateWin = fc_has_five(
        (const int (*)[FC_BOARD_SIZE])after, x, y, side);
    audit->immediatelySafe = audit->ownImmediateWin ||
        fc_count_immediate_wins(after, -side, forbiddenBlack, NULL, 0) == 0;
    if (audit->ownImmediateWin) {
        audit->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
        audit->completedClass = FC_GUARD_CLASS_OWN_VERIFIED_WIN;
        after[x][y] = 0;
        audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
        return audit->boardRestored;
    }

    int depth = profile->earlyVCFBaseDepth;
    bool escalated = false;
    if (depth <= 0) depth = 5;
    if (profile->earlyVCFSentinelPolicy == FC_EARLY_VCF_POLICY_ADAPTIVE &&
        profile->earlyVCFMaxDepth > depth &&
        fc_early_vcf_forcing_signal(
            (const int (*)[FC_BOARD_SIZE])after, -side,
            forbiddenBlack)) {
        depth = profile->earlyVCFMaxDepth;
        escalated = true;
        fcProofDiagnostics.earlyVCFAdaptiveEscalations++;
    } else if (profile->earlyVCFSentinelPolicy ==
                   FC_EARLY_VCF_POLICY_FIXED &&
               profile->earlyVCFMaxDepth > 0) {
        depth = profile->earlyVCFMaxDepth;
    }
    if (effectiveDepth != NULL) *effectiveDepth = depth;
    if (adaptiveEscalated != NULL) *adaptiveEscalated = escalated;

    FCAIProfile queryProfile = *profile;
    queryProfile.parallelProofEnabled = false;
    queryProfile.proofWorkerCount = 1;
    queryProfile.workerCountOverride = 1;
    queryProfile.branchFirstSearchEnabled = false;
    queryProfile.persistentWorkerPoolEnabled = false;
    uint64_t nodeBudget = profile->earlyVCFNodeBudget;
    uint32_t timeBudget = profile->earlyVCFTimeBudgetMs;
    if (nodeBudget == 0 || timeBudget == 0) {
        audit->vcf.status = FC_PROOF_UNKNOWN;
        audit->vcf.searchClass = FC_PROOF_SEARCH_VCF;
        audit->vcf.completedDepth = depth;
        audit->vcf.budgetExhausted = true;
    } else {
        fcProofDiagnostics.earlyVCFQueries++;
        (void)fc_parallel_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])after, -side,
            forbiddenBlack, FC_PROOF_SEARCH_VCF, depth,
            nodeBudget, timeBudget, &queryProfile, &audit->vcf);
    }
    if (audit->vcf.status == FC_PROOF_PROVEN_WIN) {
        if (!audit->vcf.certificateVerified ||
            !fc_verify_proof((const int (*)[FC_BOARD_SIZE])after,
                             -side, forbiddenBlack, &audit->vcf)) {
            audit->vcf.status = FC_PROOF_UNKNOWN;
            audit->vcf.certificateVerified = false;
        }
    }
    audit->status = audit->vcf.status;
    audit->completedSearchClass = FC_PROOF_SEARCH_VCF;
    if (audit->vcf.status == FC_PROOF_PROVEN_WIN) {
        audit->completedClass = FC_GUARD_CLASS_VERIFIED_LOSS;
        fcProofDiagnostics.earlyVCFCompletedProofs++;
        fcProofDiagnostics.earlyVCFVerifiedLosses++;
        fc_early_vcf_cache_store(
            (const int (*)[FC_BOARD_SIZE])after, -side,
            forbiddenBlack, profile, &audit->vcf);
    } else if (audit->vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
        audit->completedClass = FC_GUARD_CLASS_SCOPED_DISPROOF;
        fcProofDiagnostics.earlyVCFCompletedDisproofs++;
    } else {
        audit->completedClass = audit->immediatelySafe
            ? FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN
            : FC_GUARD_CLASS_UNKNOWN;
        fcProofDiagnostics.earlyVCFUnknowns++;
        if (audit->vcf.budgetExhausted)
            fcProofDiagnostics.earlyVCFDeadlineExhaustions++;
    }
    fcProofDiagnostics.earlyVCFFreshNodes += audit->vcf.nodes;
    after[x][y] = 0;
    audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
    if (!audit->boardRestored) {
        audit->status = FC_PROOF_UNKNOWN;
        audit->completedClass = FC_GUARD_CLASS_UNKNOWN;
        fcProofDiagnostics.earlyVCFRollbacks++;
    }
    return audit->placementLegal && audit->boardRestored;
}

bool fc_audit_opponent_after_move(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    int x,
    int y,
    FCOpponentGuardAudit *audit)
{
    if (board == NULL || profile == NULL || audit == NULL) return false;
    memset(audit, 0, sizeof(*audit));
    audit->status = FC_PROOF_UNKNOWN;
    audit->completedClass = FC_GUARD_CLASS_UNKNOWN;
    audit->vcf.x = audit->vcf.y = -1;
    audit->vct.x = audit->vct.y = -1;
    int after[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(after, board, sizeof(after));
    if (!fc_make_move(after, x, y, side, forbiddenBlack)) {
        audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
        return false;
    }
    audit->placementLegal = true;
    audit->ownImmediateWin = fc_has_five(
        (const int (*)[FC_BOARD_SIZE])after, x, y, side);
    audit->immediatelySafe = audit->ownImmediateWin ||
        fc_count_immediate_wins(after, -side, forbiddenBlack, NULL, 0) == 0;
    if (audit->ownImmediateWin) {
        audit->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
        audit->completedClass = FC_GUARD_CLASS_OWN_VERIFIED_WIN;
        after[x][y] = 0;
        audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
        return audit->boardRestored;
    }

    bool reusedEarlyProof = fc_early_vcf_cache_lookup_verified_win(
        (const int (*)[FC_BOARD_SIZE])after, -side, forbiddenBlack,
        profile, &audit->vcf);
    double started = fc_now_milliseconds();
    if (!reusedEarlyProof) {
        fcProofDiagnostics.opponentGuardVCFQueries++;
        (void)fc_parallel_prove_forced_win(
            (const int (*)[FC_BOARD_SIZE])after, -side, forbiddenBlack,
            FC_PROOF_SEARCH_VCF, profile->opponentGuardVCFMaxDepth,
            profile->opponentGuardVCFNodeBudget,
            profile->opponentGuardVCFTimeBudgetMs, profile, &audit->vcf);
        audit->vcf.elapsedMilliseconds =
            fc_now_milliseconds() - started;
    }
    if (audit->vcf.status == FC_PROOF_PROVEN_WIN) {
        if (!audit->vcf.certificateVerified ||
            !fc_verify_proof((const int (*)[FC_BOARD_SIZE])after,
                             -side, forbiddenBlack, &audit->vcf)) {
            audit->vcf.status = FC_PROOF_UNKNOWN;
            audit->vcf.certificateVerified = false;
        }
    } else if (audit->vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE &&
               !fc_verify_scoped_disproof_isolated(
                   (const int (*)[FC_BOARD_SIZE])after, -side,
                   forbiddenBlack, &audit->vcf)) {
        audit->vcf.status = FC_PROOF_UNKNOWN;
    }
    audit->status = audit->vcf.status;
    if (audit->vcf.status == FC_PROOF_PROVEN_WIN) {
        audit->completedClass = FC_GUARD_CLASS_VERIFIED_LOSS;
        audit->completedSearchClass = FC_PROOF_SEARCH_VCF;
    } else if (audit->vcf.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
        audit->completedClass = FC_GUARD_CLASS_SCOPED_DISPROOF;
        audit->completedSearchClass = FC_PROOF_SEARCH_VCF;
        audit->vctEligible = !profile->opponentGuardStructuralVCTEnabled ||
            fc_opponent_guard_vct_signal(
                (const int (*)[FC_BOARD_SIZE])after, -side,
                forbiddenBlack);
        if (!audit->vctEligible) {
            fcProofDiagnostics.opponentGuardVCTStructuralSkips++;
        } else if (!fc_decision_deadline_reached()) {
            fcProofDiagnostics.opponentGuardVCTQueries++;
            started = fc_now_milliseconds();
            (void)fc_parallel_prove_forced_win(
                (const int (*)[FC_BOARD_SIZE])after, -side,
                forbiddenBlack, FC_PROOF_SEARCH_VCT,
                profile->opponentGuardVCTMaxDepth,
                profile->opponentGuardVCTNodeBudget,
                profile->opponentGuardVCTTimeBudgetMs, profile,
                &audit->vct);
            audit->vct.elapsedMilliseconds =
                fc_now_milliseconds() - started;
            if (audit->vct.status == FC_PROOF_PROVEN_WIN) {
                if (!audit->vct.certificateVerified ||
                    !fc_verify_proof(
                        (const int (*)[FC_BOARD_SIZE])after, -side,
                        forbiddenBlack, &audit->vct)) {
                    audit->vct.status = FC_PROOF_UNKNOWN;
                    audit->vct.certificateVerified = false;
                }
            } else if (audit->vct.status ==
                           FC_PROOF_NO_FORCED_WIN_IN_SCOPE &&
                       !fc_verify_scoped_disproof_isolated(
                           (const int (*)[FC_BOARD_SIZE])after, -side,
                           forbiddenBlack, &audit->vct)) {
                audit->vct.status = FC_PROOF_UNKNOWN;
            }
            if (audit->vct.status == FC_PROOF_PROVEN_WIN) {
                audit->status = FC_PROOF_PROVEN_WIN;
                audit->completedClass = FC_GUARD_CLASS_VERIFIED_LOSS;
                audit->completedSearchClass = FC_PROOF_SEARCH_VCT;
            } else if (audit->vct.status ==
                       FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
                audit->status = FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
                audit->completedClass = FC_GUARD_CLASS_SCOPED_DISPROOF;
                audit->completedSearchClass = FC_PROOF_SEARCH_VCT;
            } else {
                audit->status = FC_PROOF_UNKNOWN;
                audit->completedClass = audit->immediatelySafe
                    ? FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN
                    : FC_GUARD_CLASS_UNKNOWN;
                audit->completedSearchClass = FC_PROOF_SEARCH_VCF;
            }
        }
    } else {
        audit->completedClass = audit->immediatelySafe
            ? FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN
            : FC_GUARD_CLASS_UNKNOWN;
    }
    after[x][y] = 0;
    audit->boardRestored = memcmp(after, board, sizeof(after)) == 0;
    if (!audit->boardRestored) {
        audit->status = FC_PROOF_UNKNOWN;
        audit->completedClass = FC_GUARD_CLASS_UNKNOWN;
    }
    return audit->placementLegal && audit->boardRestored;
}

static const FCProofResult *fc_guard_completed_proof(
    const FCOpponentGuardAudit *audit)
{
    if (audit->completedSearchClass == FC_PROOF_SEARCH_VCT)
        return &audit->vct;
    return &audit->vcf;
}

static bool fc_guard_audit_better(const FCOpponentGuardAudit *candidate,
                                  const FCOpponentGuardAudit *current)
{
    if (candidate->completedClass != current->completedClass)
        return candidate->completedClass > current->completedClass;
    if (candidate->completedClass == FC_GUARD_CLASS_SCOPED_DISPROOF &&
        candidate->completedSearchClass != current->completedSearchClass)
        return candidate->completedSearchClass > current->completedSearchClass;
    if (candidate->completedClass == FC_GUARD_CLASS_VERIFIED_LOSS) {
        const FCProofResult *candidateProof =
            fc_guard_completed_proof(candidate);
        const FCProofResult *currentProof = fc_guard_completed_proof(current);
        return candidateProof->distance > currentProof->distance;
    }
    return false;
}

bool fc_test_opponent_guard_audit_better(
    const FCOpponentGuardAudit *candidate,
    const FCOpponentGuardAudit *current)
{
    if (candidate == NULL || current == NULL) return false;
    return fc_guard_audit_better(candidate, current);
}

static bool fc_guard_audit_matches_or_exceeds(
    const FCOpponentGuardAudit *candidate,
    int completedClass,
    int completedSearchClass,
    int completedDistance)
{
    if (candidate->completedClass != completedClass)
        return candidate->completedClass > completedClass;
    if (completedClass == FC_GUARD_CLASS_SCOPED_DISPROOF)
        return candidate->completedSearchClass >= completedSearchClass;
    if (completedClass == FC_GUARD_CLASS_VERIFIED_LOSS)
        return fc_guard_completed_proof(candidate)->distance >=
               completedDistance;
    return true;
}

static void fc_guard_count_audit(FCAnalysisResult *result,
                                 const FCOpponentGuardAudit *audit,
                                 uint32_t stage)
{
    result->opponentGuardAuditedStages |= stage;
    result->opponentGuardAuditedCount++;
    fcProofDiagnostics.opponentGuardCandidatesAudited++;
    if (audit->completedClass == FC_GUARD_CLASS_SCOPED_DISPROOF) {
        result->opponentGuardCompletedDisproofs++;
        fcProofDiagnostics.opponentGuardCompletedDisproofs++;
    } else if (audit->completedClass == FC_GUARD_CLASS_VERIFIED_LOSS) {
        result->opponentGuardVerifiedLosses++;
        fcProofDiagnostics.opponentGuardVerifiedLosses++;
    } else {
        result->opponentGuardUnknowns++;
        fcProofDiagnostics.opponentGuardUnknowns++;
    }
}

static uint32_t fc_guard_stage_for_escape(int stage)
{
    switch (stage) {
        case FC_ESCAPE_CERTIFICATE: return FC_GUARD_STAGE_CERTIFICATE;
        case FC_ESCAPE_TACTICAL: return FC_GUARD_STAGE_TACTICAL;
        case FC_ESCAPE_ORDINARY: return FC_GUARD_STAGE_ORDINARY;
        case FC_ESCAPE_ALL_LEGAL: return FC_GUARD_STAGE_ALL_LEGAL;
        default: return FC_GUARD_STAGE_NONE;
    }
}

static void fc_guard_publish_selected(FCAnalysisResult *result,
                                      const FCOpponentGuardAudit *audit)
{
    const FCProofResult *proof = fc_guard_completed_proof(audit);
    result->opponentGuardSelectedClass = audit->completedClass;
    result->opponentGuardVCFStatus = audit->vcf.status;
    result->opponentGuardVCFDistance = audit->vcf.distance;
    result->opponentGuardVCFNodes = audit->vcf.nodes;
    result->opponentGuardVCFMilliseconds =
        audit->vcf.elapsedMilliseconds;
    result->opponentGuardVCFCertificateVerified =
        audit->vcf.certificateVerified;
    result->opponentGuardVCTEligible = audit->vctEligible;
    result->opponentGuardVCTStatus = audit->vct.status;
    result->opponentGuardVCTDistance = audit->vct.distance;
    result->opponentGuardVCTNodes = audit->vct.nodes;
    result->opponentGuardVCTMilliseconds =
        audit->vct.elapsedMilliseconds;
    result->opponentGuardVCTCertificateVerified =
        audit->vct.certificateVerified;
    result->opponentAfterSelectedStatus = audit->status;
    result->opponentAfterSelectedDistance = proof->distance;
    result->proofStatus = audit->status;
    result->proofSearchClass = audit->completedSearchClass;
    result->proofDistance = proof->distance;
    result->proofCertificateId = proof->certificateId;
    result->proofCertificateVerified = proof->certificateVerified;
    result->proofNodes = proof->nodes;
    result->proofNumber = proof->proofNumber;
    result->disproofNumber = proof->disproofNumber;
}

static int fc_early_vcf_audit_rank(const FCOpponentGuardAudit *audit)
{
    if (audit == NULL) return 0;
    switch (audit->completedClass) {
        case FC_GUARD_CLASS_OWN_VERIFIED_WIN: return 5;
        case FC_GUARD_CLASS_SCOPED_DISPROOF: return 4;
        case FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN: return 3;
        case FC_GUARD_CLASS_UNKNOWN: return 2;
        case FC_GUARD_CLASS_VERIFIED_LOSS: return 1;
        default: return 0;
    }
}

static bool fc_early_vcf_audit_better(
    const FCOpponentGuardAudit *candidate,
    const FCOpponentGuardAudit *current)
{
    int candidateRank = fc_early_vcf_audit_rank(candidate);
    int currentRank = fc_early_vcf_audit_rank(current);
    if (candidateRank != currentRank) return candidateRank > currentRank;
    if (candidate != NULL && current != NULL &&
        candidate->completedClass == FC_GUARD_CLASS_VERIFIED_LOSS)
        return candidate->vcf.distance > current->vcf.distance;
    return false;
}

static void fc_early_vcf_adopt_candidate(
    FCAnalysisResult *baseline,
    const FCCandidate *baseMoves,
    int baseCount,
    int x,
    int y,
    const FCOpponentGuardAudit *audit)
{
    baseline->x = x;
    baseline->y = y;
    baseline->provenLoss = false;
    baseline->decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
    baseline->proofStatus = FC_PROOF_UNKNOWN;
    baseline->proofSearchClass = FC_PROOF_SEARCH_NONE;
    baseline->proofDistance = 0;
    baseline->proofCertificateId = 0;
    baseline->proofNodes = 0;
    baseline->proofNumber = 0;
    baseline->disproofNumber = 0;
    baseline->proofCertificateVerified = false;
    baseline->opponentAfterSelectedStatus = FC_PROOF_UNKNOWN;
    baseline->opponentAfterSelectedDistance = 0;
    baseline->lossReason = FC_LOSS_BUDGET_UNKNOWN;
    baseline->tacticalClass = audit != NULL && audit->ownImmediateWin
        ? FC_TACTICAL_IMMEDIATE_WIN : FC_TACTICAL_NORMAL;
    for (int i = 0; i < baseCount; i++) {
        if (baseMoves[i].x != x || baseMoves[i].y != y) continue;
        baseline->score = baseMoves[i].score;
        if (audit == NULL || !audit->ownImmediateWin)
            baseline->tacticalClass = baseMoves[i].tacticalClass;
        break;
    }
}

static FC_NOINLINE bool fc_apply_early_vcf_sentinel(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    const FCCandidate *baseMoves,
    int baseCount,
    FCAnalysisResult *baseline)
{
    baseline->earlyVCFPolicy = profile->earlyVCFSentinelPolicy;
    baseline->earlyVCFProvisionalX = baseline->x;
    baseline->earlyVCFProvisionalY = baseline->y;
    baseline->earlyVCFSelectedX = baseline->x;
    baseline->earlyVCFSelectedY = baseline->y;
    if (!profile->earlyVCFSentinelEnabled) {
        baseline->earlyVCFSkipReason = FC_GUARD_SKIP_DISABLED;
        fcProofDiagnostics.earlyVCFSkippedDisabled++;
        return true;
    }
    bool immediateWin = baseline->tacticalClass ==
        FC_TACTICAL_IMMEDIATE_WIN &&
        fc_is_legal_move(board, baseline->x, baseline->y, side,
                         forbiddenBlack);
    if (immediateWin) {
        baseline->earlyVCFSkipReason = FC_GUARD_SKIP_IMMEDIATE_WIN;
        fcProofDiagnostics.earlyVCFSkippedImmediateWins++;
        return true;
    }
    if (fcActiveDecisionLedger == NULL ||
        !fc_decision_ledger_enter_guard(fcActiveDecisionLedger)) {
        baseline->earlyVCFSkipReason = FC_GUARD_SKIP_NO_RESERVED_RESOURCE;
        fcProofDiagnostics.earlyVCFSkippedNoResource++;
        return true;
    }

    baseline->earlyVCFEligible = true;
    fcProofDiagnostics.earlyVCFEligibleDecisions++;
    double previousDeadline = fcActiveDecisionDeadlineMilliseconds;
    fcActiveDecisionDeadlineMilliseconds =
        fcActiveDecisionLedger->internalDeadlineMilliseconds;
    double started = fc_now_milliseconds();
    uint64_t guardNodesBefore = atomic_load_explicit(
        &fcActiveDecisionLedger->guardNodesConsumed,
        memory_order_relaxed);

    FCAIProfile queryProfile = *profile;
    FCOpponentGuardAudit provisional;
    int provisionalDepth = 0;
    bool provisionalEscalated = false;
    bool audited = fc_audit_opponent_micro_vcf_after_move(
        board, side, forbiddenBlack, &queryProfile,
        baseline->x, baseline->y, &provisional,
        &provisionalDepth, &provisionalEscalated);
    if (!audited) {
        baseline->earlyVCFSkipReason = FC_GUARD_SKIP_ILLEGAL_PROVISIONAL;
        baseline->earlyVCFRollback = true;
        fcProofDiagnostics.earlyVCFRollbacks++;
        fc_decision_ledger_leave_guard(fcActiveDecisionLedger);
        fcActiveDecisionDeadlineMilliseconds = previousDeadline;
        return false;
    }

    baseline->earlyVCFEffectiveDepth = provisionalDepth;
    baseline->earlyVCFAdaptiveEscalated = provisionalEscalated;
    baseline->earlyVCFStatus = provisional.vcf.status;
    baseline->earlyVCFDistance = provisional.vcf.distance;
    baseline->earlyVCFCertificateVerified =
        provisional.vcf.certificateVerified;
    baseline->earlyVCFAuditedCount = 1;
    baseline->earlyVCFVerifiedLosses =
        provisional.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS ? 1 : 0;
    baseline->earlyVCFNodes = provisional.vcf.nodes;
    baseline->earlyVCFCacheHits = provisional.vcf.transpositionHits;

    FCOpponentGuardAudit selectedAudit = provisional;
    int selectedX = baseline->x;
    int selectedY = baseline->y;
    int selectedSource = FC_ESCAPE_NONE;
    int alternatives = 0;
    int maxAlternatives = profile->earlyVCFMaxAlternatives;
    if (maxAlternatives < 0) maxAlternatives = 0;
    if (provisional.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS &&
        maxAlternatives > 0) {
        int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(mutableBoard, board, sizeof(mutableBoard));
        FCEscapeCandidate options[FC_BOARD_SIZE * FC_BOARD_SIZE];
        int optionCount = fc_build_escape_candidates(
            mutableBoard, side, forbiddenBlack, profile,
            &provisional.vcf, baseMoves, baseCount,
            baseline->x, baseline->y,
            options, FC_BOARD_SIZE * FC_BOARD_SIZE);
        bool examined[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{false}};
        examined[baseline->x][baseline->y] = true;
        for (int i = 0; i < optionCount && alternatives < maxAlternatives;
             i++) {
            double elapsed = fc_now_milliseconds() - started;
            uint64_t consumed = baseline->earlyVCFNodes;
            if (elapsed >= (double)profile->earlyVCFTimeBudgetMs ||
                consumed >= profile->earlyVCFNodeBudget ||
                fc_decision_deadline_reached()) break;
            int x = options[i].x;
            int y = options[i].y;
            if (examined[x][y]) continue;
            examined[x][y] = true;
            queryProfile.earlyVCFNodeBudget =
                profile->earlyVCFNodeBudget - consumed;
            double remainingMs =
                (double)profile->earlyVCFTimeBudgetMs - elapsed;
            queryProfile.earlyVCFTimeBudgetMs =
                remainingMs > 1.0 ? (uint32_t)remainingMs : 1;
            FCOpponentGuardAudit candidateAudit;
            int candidateDepth = 0;
            bool candidateEscalated = false;
            if (!fc_audit_opponent_micro_vcf_after_move(
                    board, side, forbiddenBlack, &queryProfile,
                    x, y, &candidateAudit,
                    &candidateDepth, &candidateEscalated))
                continue;
            alternatives++;
            baseline->earlyVCFAuditedCount++;
            baseline->earlyVCFNodes += candidateAudit.vcf.nodes;
            baseline->earlyVCFCacheHits +=
                candidateAudit.vcf.transpositionHits;
            baseline->earlyVCFAdaptiveEscalated =
                baseline->earlyVCFAdaptiveEscalated ||
                candidateEscalated;
            if (candidateAudit.completedClass ==
                FC_GUARD_CLASS_VERIFIED_LOSS)
                baseline->earlyVCFVerifiedLosses++;
            if (fc_early_vcf_audit_better(
                    &candidateAudit, &selectedAudit)) {
                selectedAudit = candidateAudit;
                selectedX = x;
                selectedY = y;
                selectedSource = options[i].stage;
            }
            if (selectedAudit.completedClass !=
                FC_GUARD_CLASS_VERIFIED_LOSS) break;
        }
    }

    baseline->earlyVCFSelectedX = selectedX;
    baseline->earlyVCFSelectedY = selectedY;
    baseline->earlyVCFReplacementSource = selectedSource;
    if (selectedX != baseline->earlyVCFProvisionalX ||
        selectedY != baseline->earlyVCFProvisionalY) {
        baseline->earlyVCFAvoidedVerifiedLoss =
            provisional.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS;
        if (baseline->earlyVCFAvoidedVerifiedLoss) {
            fcProofDiagnostics.earlyVCFAvoidedVerifiedLosses++;
            fc_early_vcf_adopt_candidate(
                baseline, baseMoves, baseCount,
                selectedX, selectedY, &selectedAudit);
            baseline->overrideReason =
                selectedAudit.completedClass ==
                    FC_GUARD_CLASS_SCOPED_DISPROOF
                ? FC_OVERRIDE_PROVEN_DEFENSE
                : selectedAudit.completedClass ==
                      FC_GUARD_CLASS_VERIFIED_LOSS
                ? FC_OVERRIDE_LONGEST_SURVIVAL
                : FC_OVERRIDE_UNPROVEN_ESCAPE;
            baseline->escapeStage = selectedSource;
            baseline->escapeAlternativesExamined += alternatives;
        }
    }
    baseline->earlyVCFMilliseconds = fc_now_milliseconds() - started;
    baseline->earlyVCFConsumedNodes = atomic_load_explicit(
        &fcActiveDecisionLedger->guardNodesConsumed,
        memory_order_relaxed) - guardNodesBefore;
    double remaining = fcActiveDecisionLedger->ordinaryDeadlineMilliseconds -
        fc_now_milliseconds();
    baseline->earlyVCFDownstreamBudgetRemainingMs =
        remaining > 0.0 ? remaining : 0.0;
    fc_decision_ledger_leave_guard(fcActiveDecisionLedger);
    fcActiveDecisionDeadlineMilliseconds = previousDeadline;
    return true;
}

static FC_NOINLINE bool fc_apply_opponent_guard(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    const FCCandidate *baseMoves,
    int baseCount,
    FCAnalysisResult *baseline)
{
    if (!profile->opponentGuardEnabled) {
        baseline->opponentGuardSkipReason = FC_GUARD_SKIP_DISABLED;
        fcProofDiagnostics.opponentGuardSkippedDisabled++;
        return true;
    }
    baseline->opponentGuardProvisionalX = baseline->x;
    baseline->opponentGuardProvisionalY = baseline->y;
    baseline->opponentGuardSelectedX = baseline->x;
    baseline->opponentGuardSelectedY = baseline->y;
    bool immediateWin = baseline->tacticalClass ==
        FC_TACTICAL_IMMEDIATE_WIN &&
        fc_is_legal_move(board, baseline->x, baseline->y, side,
                         forbiddenBlack);
    if (immediateWin) {
        baseline->opponentGuardSkipReason =
            FC_GUARD_SKIP_IMMEDIATE_WIN;
        baseline->opponentGuardSelectedClass =
            FC_GUARD_CLASS_OWN_VERIFIED_WIN;
        fcProofDiagnostics.opponentGuardSkippedImmediateWins++;
        if (fcActiveDecisionLedger != NULL)
            fc_decision_ledger_release_guard(fcActiveDecisionLedger);
        return true;
    }
    bool verifiedOwnWin =
        baseline->tacticalClass == FC_TACTICAL_FORCED_ATTACK &&
        baseline->proofStatus == FC_PROOF_PROVEN_WIN &&
        baseline->proofCertificateVerified;
    if (verifiedOwnWin) {
        baseline->opponentGuardSkipReason =
            FC_GUARD_SKIP_VERIFIED_OWN_WIN;
        baseline->opponentGuardSelectedClass =
            FC_GUARD_CLASS_OWN_VERIFIED_WIN;
        fcProofDiagnostics.opponentGuardSkippedVerifiedOwnWins++;
        if (fcActiveDecisionLedger != NULL)
            fc_decision_ledger_release_guard(fcActiveDecisionLedger);
        return true;
    }
    if (fcActiveDecisionLedger == NULL ||
        !fc_decision_ledger_enter_guard(fcActiveDecisionLedger)) {
        baseline->opponentGuardSkipReason =
            FC_GUARD_SKIP_NO_RESERVED_RESOURCE;
        fcProofDiagnostics.opponentGuardSkippedNoResource++;
        return true;
    }
    baseline->opponentGuardEligible = true;
    fcProofDiagnostics.opponentGuardEligibleDecisions++;
    double previousDeadline = fcActiveDecisionDeadlineMilliseconds;
    fcActiveDecisionDeadlineMilliseconds =
        fcActiveDecisionLedger->internalDeadlineMilliseconds;
    uint64_t guardNodesBefore = atomic_load_explicit(
        &fcActiveDecisionLedger->guardNodesConsumed,
        memory_order_relaxed);

    FCOpponentGuardAudit provisional;
    bool audited = fc_audit_opponent_after_move(
        board, side, forbiddenBlack, profile,
        baseline->x, baseline->y, &provisional);
    if (!audited) {
        baseline->opponentGuardSkipReason =
            FC_GUARD_SKIP_ILLEGAL_PROVISIONAL;
        baseline->opponentGuardRollback = true;
        fcProofDiagnostics.opponentGuardRollbacks++;
        fc_decision_ledger_leave_guard(fcActiveDecisionLedger);
        fcActiveDecisionDeadlineMilliseconds = previousDeadline;
        return false;
    }
    fc_guard_count_audit(baseline, &provisional,
                         FC_GUARD_STAGE_PROVISIONAL);
    baseline->opponentGuardProvisionalClass = provisional.completedClass;
    FCOpponentGuardAudit selectedAudit = provisional;
    int selectedX = baseline->x;
    int selectedY = baseline->y;
    int selectedStage = FC_ESCAPE_NONE;
    bool mandatory = baseline->tacticalClass == FC_TACTICAL_MUST_DEFEND;
    bool examined[FC_BOARD_SIZE][FC_BOARD_SIZE] = {{false}};
    examined[selectedX][selectedY] = true;
    int maxAlternatives = profile->opponentGuardMaxAlternatives;
    if (maxAlternatives < 0) maxAlternatives = 0;
    int alternatives = 0;

    /* Mandatory blocks are a continuation portfolio, not a one-ply safety
     * assertion. Preserve their generated deterministic tactical order. */
    if (mandatory) {
        for (int i = 0; i < baseCount && alternatives < maxAlternatives; i++) {
            if (baseMoves[i].tacticalClass != FC_TACTICAL_MUST_DEFEND ||
                examined[baseMoves[i].x][baseMoves[i].y]) continue;
            FCOpponentGuardAudit candidateAudit;
            if (!fc_audit_opponent_after_move(
                    board, side, forbiddenBlack, profile,
                    baseMoves[i].x, baseMoves[i].y, &candidateAudit))
                continue;
            examined[baseMoves[i].x][baseMoves[i].y] = true;
            alternatives++;
            fc_guard_count_audit(baseline, &candidateAudit,
                                 FC_GUARD_STAGE_MANDATORY);
            if (fc_guard_audit_better(&candidateAudit, &selectedAudit)) {
                selectedAudit = candidateAudit;
                selectedX = baseMoves[i].x;
                selectedY = baseMoves[i].y;
                selectedStage = FC_ESCAPE_TACTICAL;
            }
        }
    }

    if ((provisional.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS ||
         selectedAudit.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS) &&
        alternatives < maxAlternatives &&
        !fc_decision_deadline_reached()) {
        int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(mutableBoard, board, sizeof(mutableBoard));
        const FCProofResult *lossProof =
            fc_guard_completed_proof(&provisional);
        FCEscapeCandidate options[FC_BOARD_SIZE * FC_BOARD_SIZE];
        int optionCount = fc_build_escape_candidates(
            mutableBoard, side, forbiddenBlack, profile, lossProof,
            baseMoves, baseCount, baseline->x, baseline->y,
            options, FC_BOARD_SIZE * FC_BOARD_SIZE);
        for (int i = 0; i < optionCount && alternatives < maxAlternatives;
             i++) {
            int x = options[i].x;
            int y = options[i].y;
            if (examined[x][y]) continue;
            FCOpponentGuardAudit candidateAudit;
            if (!fc_audit_opponent_after_move(
                    board, side, forbiddenBlack, profile,
                    x, y, &candidateAudit)) continue;
            examined[x][y] = true;
            alternatives++;
            fc_guard_count_audit(
                baseline, &candidateAudit,
                fc_guard_stage_for_escape(options[i].stage));
            if (fc_guard_audit_better(&candidateAudit, &selectedAudit)) {
                selectedAudit = candidateAudit;
                selectedX = x;
                selectedY = y;
                selectedStage = options[i].stage;
            }
            if (selectedAudit.completedClass ==
                    FC_GUARD_CLASS_SCOPED_DISPROOF &&
                !mandatory) break;
        }
    }

    baseline->x = selectedX;
    baseline->y = selectedY;
    baseline->opponentGuardSelectedX = selectedX;
    baseline->opponentGuardSelectedY = selectedY;
    baseline->escapeAlternativesExamined += alternatives;
    if (selectedStage != FC_ESCAPE_NONE) baseline->escapeStage = selectedStage;
    if (selectedX != baseline->opponentGuardProvisionalX ||
        selectedY != baseline->opponentGuardProvisionalY) {
        baseline->opponentGuardAvoidedVerifiedLoss =
            provisional.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS;
        if (baseline->opponentGuardAvoidedVerifiedLoss)
            fcProofDiagnostics.opponentGuardAvoidedVerifiedLosses++;
        baseline->overrideReason =
            selectedAudit.completedClass == FC_GUARD_CLASS_SCOPED_DISPROOF
                ? FC_OVERRIDE_PROVEN_DEFENSE
                : selectedAudit.completedClass ==
                      FC_GUARD_CLASS_VERIFIED_LOSS
                ? FC_OVERRIDE_LONGEST_SURVIVAL
                : FC_OVERRIDE_UNPROVEN_ESCAPE;
        if (!mandatory)
            baseline->tacticalClass = FC_TACTICAL_FORCED_DEFENSE;
    }
    if (selectedAudit.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS) {
        baseline->lossReason = FC_LOSS_SELECTED_VERIFIED;
    } else if (selectedAudit.completedClass ==
               FC_GUARD_CLASS_IMMEDIATELY_SAFE_UNKNOWN) {
        baseline->lossReason = FC_LOSS_BUDGET_UNKNOWN;
    } else {
        baseline->lossReason = FC_LOSS_NONE;
    }
    fc_guard_publish_selected(baseline, &selectedAudit);
    baseline->opponentGuardConsumedNodes =
        atomic_load_explicit(
            &fcActiveDecisionLedger->guardNodesConsumed,
            memory_order_relaxed) - guardNodesBefore;
    baseline->opponentGuardReservedNodes =
        fcActiveDecisionLedger->guardNodeReservation;
    fc_decision_ledger_leave_guard(fcActiveDecisionLedger);
    fcActiveDecisionDeadlineMilliseconds = previousDeadline;
    return true;
}

static void fc_refine_frozen_verified_loss(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    FCAnalysisResult *baseline)
{
    int mutableBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(mutableBoard, board, sizeof(mutableBoard));
    FCProofResult frozenLoss;
    int status = fc_opponent_proof_after_move(
        board, side, forbiddenBlack, profile,
        baseline->x, baseline->y, &frozenLoss);
    if (status != FC_PROOF_PROVEN_WIN || !frozenLoss.certificateVerified)
        return;

    FCCandidate baseMoves[FC_MAX_CANDIDATES];
    int baseCount = fc_generate_candidates(
        mutableBoard, side, forbiddenBlack, profile,
        baseMoves, FC_MAX_CANDIDATES, false);
    if (baseCount <= 0) return;
    FCEscapeCandidate options[FC_BOARD_SIZE * FC_BOARD_SIZE];
    int optionCount = fc_build_escape_candidates(
        mutableBoard, side, forbiddenBlack, profile, &frozenLoss,
        baseMoves, baseCount, baseline->x, baseline->y,
        options, FC_BOARD_SIZE * FC_BOARD_SIZE);
    int limit = profile->proofEscapeCandidateLimit > 0
        ? profile->proofEscapeCandidateLimit : optionCount;
    if (limit > optionCount) limit = optionCount;
    int bestUnknown = -1;
    int bestLoss = -1;
    FCProofResult bestUnknownProof;
    FCProofResult bestLossProof;
    memset(&bestUnknownProof, 0, sizeof(bestUnknownProof));
    memset(&bestLossProof, 0, sizeof(bestLossProof));
    FCProofResult *escapeProofs = calloc(
        (size_t)limit, sizeof(*escapeProofs));
    bool *escapeCompleted = calloc(
        (size_t)limit, sizeof(*escapeCompleted));
    bool *escapeSafe = calloc(
        (size_t)limit, sizeof(*escapeSafe));
    uint64_t escapeBudget = profile->proofParallelNodeBudget > 0
        ? profile->proofParallelNodeBudget / 2
        : profile->proofNodeBudget / 2;
    if (escapeProofs != NULL && escapeCompleted != NULL &&
        escapeSafe != NULL) {
        (void)fc_parallel_escape_search(
            (const int (*)[FC_BOARD_SIZE])mutableBoard, side,
            forbiddenBlack, profile, options, limit, escapeBudget,
            fcActiveDecisionLedger != NULL
                ? profile->proofTimeBudgetMs
                : profile->proofEmergencyTimeBudgetMs,
            escapeProofs, escapeCompleted, escapeSafe);
    }
    if (escapeProofs != NULL && escapeCompleted != NULL &&
        escapeSafe != NULL) for (int i = 0; i < limit; i++) {
        if (!escapeCompleted[i]) continue;
        int x = options[i].x;
        int y = options[i].y;
        baseline->escapeAlternativesExamined++;
        if (!escapeSafe[i]) {
            baseline->escapeVerifiedLossCount++;
            continue;
        }
        FCProofResult reply = escapeProofs[i];
        if (reply.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
            int replayBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(replayBoard, mutableBoard, sizeof(replayBoard));
            if (!fc_make_move(replayBoard, x, y, side, forbiddenBlack) ||
                !fc_replay_scoped_disproof_in_place(
                    (const int (*)[FC_BOARD_SIZE])replayBoard,
                    -side, forbiddenBlack, &reply)) {
                reply.status = FC_PROOF_UNKNOWN;
            }
        }
        if (reply.status == FC_PROOF_NO_FORCED_WIN_IN_SCOPE) {
            baseline->x = x;
            baseline->y = y;
            baseline->overrideReason = FC_OVERRIDE_PROVEN_DEFENSE;
            baseline->escapeStage = options[i].stage;
            baseline->escapeScopedDisproofCount++;
            baseline->proofStatus = reply.status;
            baseline->proofSearchClass = reply.searchClass;
            baseline->proofDistance = reply.distance;
            baseline->proofNodes = reply.nodes;
            baseline->proofNumber = reply.proofNumber;
            baseline->disproofNumber = reply.disproofNumber;
            baseline->stats.nodes = reply.nodes;
            baseline->stats.transpositionHits = reply.transpositionHits;
            baseline->stats.budgetExhausted = reply.budgetExhausted;
            baseline->opponentAfterSelectedStatus = reply.status;
            baseline->opponentAfterSelectedDistance = reply.distance;
            baseline->lossReason = FC_LOSS_NONE;
            free(escapeProofs);
            free(escapeCompleted);
            free(escapeSafe);
            return;
        }
        if (reply.status == FC_PROOF_PROVEN_WIN &&
            reply.certificateVerified) {
            baseline->escapeVerifiedLossCount++;
            if (bestLoss < 0 || reply.distance > bestLossProof.distance) {
                bestLoss = i;
                bestLossProof = reply;
            }
        } else {
            baseline->escapeUnknownCount++;
            if (bestUnknown < 0 ||
                reply.proofNumber > bestUnknownProof.proofNumber) {
                bestUnknown = i;
                bestUnknownProof = reply;
            }
        }
    }
    free(escapeProofs);
    free(escapeCompleted);
    free(escapeSafe);
    if (bestUnknown >= 0) {
        baseline->x = options[bestUnknown].x;
        baseline->y = options[bestUnknown].y;
        baseline->overrideReason = FC_OVERRIDE_UNPROVEN_ESCAPE;
        baseline->escapeStage = options[bestUnknown].stage;
        baseline->proofStatus = bestUnknownProof.status;
        baseline->proofSearchClass = bestUnknownProof.searchClass;
        baseline->proofNumber = bestUnknownProof.proofNumber;
        baseline->disproofNumber = bestUnknownProof.disproofNumber;
        baseline->proofNodes = bestUnknownProof.nodes;
        baseline->stats.nodes = bestUnknownProof.nodes;
        baseline->stats.budgetExhausted = bestUnknownProof.budgetExhausted;
        baseline->lossReason = FC_LOSS_BUDGET_UNKNOWN;
    } else if (bestLoss >= 0 &&
               bestLossProof.distance > frozenLoss.distance) {
        baseline->x = options[bestLoss].x;
        baseline->y = options[bestLoss].y;
        baseline->overrideReason = FC_OVERRIDE_LONGEST_SURVIVAL;
        baseline->escapeStage = options[bestLoss].stage;
        baseline->proofStatus = bestLossProof.status;
        baseline->proofSearchClass = bestLossProof.searchClass;
        baseline->proofDistance = bestLossProof.distance;
        baseline->proofCertificateId = bestLossProof.certificateId;
        baseline->proofCertificateVerified =
            bestLossProof.certificateVerified;
        baseline->proofNodes = bestLossProof.nodes;
        baseline->proofNumber = bestLossProof.proofNumber;
        baseline->disproofNumber = bestLossProof.disproofNumber;
        baseline->lossReason = FC_LOSS_SELECTED_VERIFIED;
    }
}

static bool fc_candidate_own_win_eligible(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    FCAnalysisResult *baseline)
{
    if (baseline->tacticalClass != FC_TACTICAL_NORMAL ||
        fc_board_stone_count(board) < 10 ||
        fc_decision_deadline_reached()) return false;
    int rootBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
    memcpy(rootBoard, board, sizeof(rootBoard));
    return fc_has_vct_root_threat(rootBoard, side, forbiddenBlack);
}

static bool fc_corpus_random_equivalent(
    const FCCorpusCandidateTelemetry *best,
    int bestScore,
    const FCCorpusCandidateTelemetry *alternative,
    int alternativeScore)
{
    if (best == NULL || alternative == NULL ||
        !best->accepted || !alternative->accepted) return false;
    return best->games == alternative->games &&
        best->events == alternative->events &&
        best->sources == alternative->sources &&
        best->trustTier == alternative->trustTier &&
        best->matchType == alternative->matchType &&
        best->requiredStones == alternative->requiredStones &&
        best->sourceBoardMask == alternative->sourceBoardMask &&
        best->proofStatus == alternative->proofStatus &&
        best->proofSearchClass == alternative->proofSearchClass &&
        best->proofDistance == alternative->proofDistance &&
        best->completedDepth == alternative->completedDepth &&
        best->tacticalClass == alternative->tacticalClass &&
        best->reason == alternative->reason &&
        abs(alternativeScore - bestScore) <= 10;
}

bool fc_test_corpus_random_equivalent(
    const FCCorpusCandidateTelemetry *best,
    int bestScore,
    const FCCorpusCandidateTelemetry *alternative,
    int alternativeScore)
{
    return fc_corpus_random_equivalent(
        best, bestScore, alternative, alternativeScore);
}

int fc_test_select_corpus_random_equivalent(
    const FCCorpusCandidateTelemetry *candidates,
    const int *scores,
    int count,
    uint64_t seed)
{
    if (candidates == NULL || scores == NULL || count <= 0 ||
        count > FC_MAX_CORPUS_CANDIDATES) return -1;
    int equivalent[FC_MAX_CORPUS_CANDIDATES];
    int equivalentCount = 0;
    for (int i = 0; i < count; i++) {
        if (fc_corpus_random_equivalent(
                &candidates[0], scores[0], &candidates[i], scores[i]))
            equivalent[equivalentCount++] = i;
    }
    if (equivalentCount == 0) return -1;
    FCRandom random;
    fc_random_seed(&random, seed ^ UINT64_C(0x454c495445524e44));
    return equivalent[fc_random_next(&random) % (uint64_t)equivalentCount];
}

static void fc_apply_candidate_own_win(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    FCAnalysisResult *baseline)
{
    const int classes[2] = {FC_PROOF_SEARCH_VCF, FC_PROOF_SEARCH_VCT};
    FCProofResult proof;
    bool proven = false;
    for (int stage = 0; stage < 2; stage++) {
        int searchClass = classes[stage];
        uint64_t stageBudget = profile->proofParallelNodeBudget > 0
            ? profile->proofParallelNodeBudget : profile->proofNodeBudget;
        uint64_t stageNodes = stageBudget / (stage == 0 ? 3 : 1);
        if (stageNodes == 0) stageNodes = 1;
        uint32_t stageTime = profile->proofTimeBudgetMs;
        uint32_t maximumTime = fc_profile_proof_path_enabled(profile)
            ? (stage == 0 ? 500U : 1200U)
            : (stage == 0 ? 80U : 140U);
        if (stageTime == 0 || stageTime > maximumTime)
            stageTime = maximumTime;
        if (searchClass == FC_PROOF_SEARCH_VCF)
            fcProofDiagnostics.stageVCFQueries++;
        else
            fcProofDiagnostics.stageVCTQueries++;
        uint64_t combinationsBefore =
            fcProofDiagnostics.dependencyCombinations;
        proven = fc_parallel_prove_forced_win(
            board, side, forbiddenBlack, searchClass,
            profile->proofMaxDepth, stageNodes, stageTime,
            profile, &proof) &&
            proof.certificateVerified;
        if (searchClass == FC_PROOF_SEARCH_VCT &&
            fcProofDiagnostics.dependencyCombinations > combinationsBefore)
            fcProofDiagnostics.stageDependencyQueries++;
        if (proven || fc_global_proof_deadline_reached()) break;
    }
    if (!proven) return;
    int priorX = baseline->x;
    int priorY = baseline->y;
    baseline->x = proof.x;
    baseline->y = proof.y;
    baseline->tacticalClass = FC_TACTICAL_FORCED_ATTACK;
    baseline->overrideReason = proof.x == priorX && proof.y == priorY
        ? FC_OVERRIDE_NONE : FC_OVERRIDE_PROVEN_ATTACK;
    baseline->proofStatus = proof.status;
    baseline->proofSearchClass = proof.searchClass;
    baseline->proofDistance = proof.distance;
    baseline->proofCertificateId = proof.certificateId;
    baseline->proofNodes = proof.nodes;
    baseline->proofNumber = proof.proofNumber;
    baseline->disproofNumber = proof.disproofNumber;
    baseline->proofCertificateVerified = proof.certificateVerified;
    baseline->stats.nodes = proof.nodes;
    baseline->stats.transpositionHits = proof.transpositionHits;
    baseline->stats.completedDepth = proof.completedDepth;
    baseline->stats.budgetExhausted = proof.budgetExhausted;
}

static void fc_record_decision_ledger(const FCDecisionLedger *ledger,
                                      FCAnalysisResult *result)
{
    if (ledger == NULL || result == NULL) return;
    result->decisionLedgerVersion = ledger->version;
    result->decisionNodesReserved = atomic_load_explicit(
        &ledger->nodesReserved, memory_order_relaxed);
    result->decisionNodesConsumed = atomic_load_explicit(
        &ledger->nodesConsumed, memory_order_relaxed);
    result->decisionQueriesReserved = atomic_load_explicit(
        &ledger->queriesReserved, memory_order_relaxed);
    result->decisionQueriesConsumed = atomic_load_explicit(
        &ledger->queriesConsumed, memory_order_relaxed);
    result->decisionMemoryReserved = atomic_load_explicit(
        &ledger->memoryReserved, memory_order_relaxed);
    result->decisionMemoryConsumed = atomic_load_explicit(
        &ledger->memoryConsumed, memory_order_relaxed);
    result->decisionMemoryPeakReserved = atomic_load_explicit(
        &ledger->memoryPeakReserved, memory_order_relaxed);
    result->decisionMemoryReleased = atomic_load_explicit(
        &ledger->memoryReleased, memory_order_relaxed);
    result->opponentGuardReservedNodes = ledger->guardNodeReservation;
    result->opponentGuardConsumedNodes = atomic_load_explicit(
        &ledger->guardNodesConsumed, memory_order_relaxed);
    fcProofDiagnostics.decisionMemoryLiveReserved =
        result->decisionMemoryReserved;
    fcProofDiagnostics.decisionMemoryPeakReserved =
        result->decisionMemoryPeakReserved;
    fcProofDiagnostics.decisionMemoryReleased =
        result->decisionMemoryReleased;
    bool exhausted = atomic_load_explicit(&ledger->exhausted,
                                          memory_order_acquire);
    result->decisionStageExhaustions = exhausted
        ? fcProofDiagnostics.decisionLedgerExhaustions : 0;
    if (result->decisionNodesConsumed > 0 || result->decisionQueriesConsumed > 0 ||
        result->decisionMemoryConsumed > 0)
        result->stats.budgetExhausted = result->stats.budgetExhausted ||
                                        exhausted;
}

static bool fc_analyze_five_star_profile_with_hint_internal(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (result == NULL || profile == NULL)
        return false;
    double started = fc_now_milliseconds();
    FCAnalysisResult fourStar;
    bool fourFound = fc_analyze_four_star_with_hint(
        board, side, forbiddenBlack, seed, randomMode,
        hintX, hintY, &fourStar);
    FCAnalysisResult baseline;
    bool handoffValid = fourFound && fourStar.x >= 0 && fourStar.y >= 0 &&
        fc_is_legal_move(board, fourStar.x, fourStar.y, side,
                         forbiddenBlack);
    if (handoffValid) {
        baseline = fourStar;
        baseline.handoffReason =
            fourStar.decisionStatus == FC_DECISION_UNKNOWN_OR_DEADLINE
                ? FC_HANDOFF_FOUR_STAR_UNKNOWN : FC_HANDOFF_NONE;
    } else {
        /* The frozen handoff is advisory.  A failed/truncated four-star
         * query must not bypass the candidate-only stages below. */
        memset(&baseline, 0, sizeof(baseline));
        baseline.x = -1;
        baseline.y = -1;
        baseline.fourStarX = fourStar.x;
        baseline.fourStarY = fourStar.y;
        baseline.seed = seed;
        baseline.randomMode = randomMode;
        baseline.randomEligibilityVerified = true;
        baseline.decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
        baseline.handoffReason = FC_HANDOFF_FOUR_STAR_INVALID;
        baseline.lossReason = FC_LOSS_BUDGET_UNKNOWN;
        baseline.tacticalClass = FC_TACTICAL_NORMAL;
        baseline.defaultX = -1;
        baseline.defaultY = -1;
        baseline.defaultSource = FC_DEFAULT_NONE;
        baseline.stats.completedDepth = 0;
        fcProofDiagnostics.decisionUnknowns++;
    }

    /* Rebuild the handoff pool from the current board even when the frozen
     * result was incomplete.  This keeps the four-star candidate set and the
     * generated tactical/relevance set available to the fork layer instead of
     * collapsing recovery to the first legal coordinate. */
    FCCandidate handoffCandidates[FC_MAX_CANDIDATES];
    FCForkCandidateSet handoffSeed;
    memset(&handoffSeed, 0, sizeof(handoffSeed));
    FCCandidate generatedCandidates[FC_MAX_CANDIDATES];
    int generatedCount = fc_generate_candidates(
        (int (*)[FC_BOARD_SIZE])board, side, forbiddenBlack, profile,
        generatedCandidates, FC_MAX_CANDIDATES, false);
    for (int i = 0; i < generatedCount; i++)
        (void)fc_append_fork_candidate(&handoffSeed, generatedCandidates[i]);
    if (handoffValid) {
        for (int i = 0; i < fourStar.candidateCount &&
                    i < FC_MAX_CANDIDATES; i++) {
            (void)fc_append_fork_candidate(&handoffSeed,
                                           fourStar.candidates[i]);
        }
    }
    int handoffCount = handoffSeed.allCount < FC_MAX_CANDIDATES
        ? handoffSeed.allCount : FC_MAX_CANDIDATES;
    if (handoffCount > 0) {
        memcpy(handoffCandidates, handoffSeed.all,
               (size_t)handoffCount * sizeof(handoffCandidates[0]));
    }
    bool forkLayerActive = profile->forkFirstRecoveryEnabled &&
        fc_apply_fork_candidate_layer(
            board, side, forbiddenBlack, profile, handoffCandidates,
            &handoffCount, FC_MAX_CANDIDATES,
            handoffValid ? fourStar.x : hintX,
            handoffValid ? fourStar.y : hintY,
            hintX, hintY, &baseline);
    if (forkLayerActive && handoffCount > 0) {
        baseline.candidateCount = handoffCount;
        memcpy(baseline.candidates, handoffCandidates,
               (size_t)handoffCount * sizeof(handoffCandidates[0]));
        baseline.x = handoffCandidates[0].x;
        baseline.y = handoffCandidates[0].y;
        baseline.score = handoffCandidates[0].score;
        baseline.tacticalClass = handoffCandidates[0].tacticalClass;
        baseline.defaultX = handoffValid ? fourStar.x : hintX;
        baseline.defaultY = handoffValid ? fourStar.y : hintY;
        baseline.defaultSource = handoffValid
            ? FC_DEFAULT_LEGACY : FC_DEFAULT_NONE;
        if (!handoffValid)
            baseline.handoffReason = FC_HANDOFF_GENERATED_FALLBACK;
        if ((baseline.x != baseline.defaultX ||
             baseline.y != baseline.defaultY) &&
            baseline.tacticalClass == FC_TACTICAL_NORMAL) {
            baseline.overrideReason = FC_OVERRIDE_FORK_SAFE_RECOVERY;
        }
        fc_record_selected_fork_telemetry(
            &baseline, baseline.defaultX, baseline.defaultY);
    } else if (!handoffValid) {
        /* The complete legality scan is reserved for the genuinely empty
         * candidate case.  It is never described as a verified loss. */
        if (!fc_find_full_board_legal_fallback(
                board, side, forbiddenBlack, &baseline.x, &baseline.y)) {
            baseline.decisionStatus = FC_DECISION_NO_LEGAL_MOVE;
            baseline.provenLoss = true;
            baseline.tacticalClass = FC_TACTICAL_PROVEN_LOSS;
            baseline.lossReason = FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED;
            baseline.handoffReason = FC_HANDOFF_FULL_BOARD_FALLBACK;
            *result = baseline;
            fcProofDiagnostics.decisionNoLegalMoves++;
            return false;
        }
        baseline.fallbackUsed = true;
        baseline.handoffReason = FC_HANDOFF_FULL_BOARD_FALLBACK;
        baseline.decisionStatus = FC_DECISION_UNKNOWN_OR_DEADLINE;
        baseline.lossReason = FC_LOSS_BUDGET_UNKNOWN;
        baseline.stats.budgetExhausted = true;
        fcProofDiagnostics.decisionFallbacks++;
    }
    FCProofSession proofSession;
    FCProofSession *previousProofSession = fcActiveProofSession;
    bool proofSessionStarted = false;
    bool preliminaryOwnCandidateEligible =
        fc_profile_proof_path_enabled(profile) &&
        fc_candidate_own_win_eligible(
            board, side, forbiddenBlack, &baseline);
    if ((profile->earlyVCFSentinelEnabled ||
         preliminaryOwnCandidateEligible) &&
        fcActiveProofSession == NULL) {
        proofSessionStarted = fc_proof_session_begin(
            &proofSession, profile, board, forbiddenBlack);
    }
    if (profile->earlyVCFSentinelEnabled) {
        /* FCAnalysisResult contains the complete candidate/corpus telemetry
         * arrays.  Use the caller-owned result as the transactional scratch
         * snapshot so enabling the sentinel does not add another very large
         * object to this already deep analysis stack frame (notably under
         * ASan redzones).  The public result is published from baseline below
         * on every completed path. */
        *result = baseline;
        if (!fc_apply_early_vcf_sentinel(
                board, side, forbiddenBlack, profile,
                handoffCandidates, handoffCount, &baseline)) {
            baseline = *result;
            baseline.earlyVCFEligible = true;
            baseline.earlyVCFRollback = true;
            baseline.earlyVCFSkipReason =
                FC_GUARD_SKIP_ILLEGAL_PROVISIONAL;
            baseline.stats.budgetExhausted = true;
        }
        /* A shallow scoped disproof cannot satisfy a later, broader query.
         * Keep the allocation, but retire the sentinel generation before
         * own-proof and final-guard work. Verified wins are reused only via
         * the separately keyed and independently replayed result cache. */
        if (proofSessionStarted && !fc_proof_session_reset_for_query(
                &proofSession, profile, board, forbiddenBlack)) {
            fc_proof_session_end(&proofSession, previousProofSession);
            proofSessionStarted = fc_proof_session_begin(
                &proofSession, profile, board, forbiddenBlack);
        }
    }
    if (baseline.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN)
        fcProofDiagnostics.stageImmediateDecisions++;
    else if (baseline.tacticalClass == FC_TACTICAL_MUST_DEFEND)
        fcProofDiagnostics.stageMandatoryDefenseDecisions++;
    bool ownCandidateEligible = fc_profile_proof_path_enabled(profile) &&
        fc_candidate_own_win_eligible(
            board, side, forbiddenBlack, &baseline);
    if (ownCandidateEligible && !proofSessionStarted &&
        fcActiveProofSession == NULL) {
        proofSessionStarted = fc_proof_session_begin(
            &proofSession, profile, board, forbiddenBlack);
    }
    if (ownCandidateEligible) {
        fc_apply_candidate_own_win(
            board, side, forbiddenBlack, profile, &baseline);
        if (profile->quietThreatEnabled &&
            baseline.tacticalClass == FC_TACTICAL_NORMAL &&
            !fc_decision_deadline_reached()) {
            FCAnalysisResult completedBeforeQuiet = baseline;
            int quietBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(quietBoard, board, sizeof(quietBoard));
            FCCandidate quietCandidates[FC_MAX_CANDIDATES];
            int quietCount = fc_generate_candidates(
                quietBoard, side, forbiddenBlack, profile,
                quietCandidates, FC_MAX_CANDIDATES, false);
            fc_examine_quiet_roots(
                quietBoard, side, forbiddenBlack, profile,
                quietCandidates, quietCount, &baseline);
            if (fc_decision_deadline_reached()) {
                baseline = completedBeforeQuiet;
                baseline.stats.budgetExhausted = true;
            }
        }
    }
    bool needsLossAwareRetry = profile->lossAwareEnabled &&
        baseline.tacticalClass == FC_TACTICAL_FORCED_DEFENSE &&
        baseline.proofStatus == FC_PROOF_PROVEN_WIN &&
        baseline.proofCertificateVerified &&
        baseline.overrideReason == FC_OVERRIDE_NONE;
    if (needsLossAwareRetry) {
        FCAnalysisResult completedBeforeLossAware = baseline;
        if (!proofSessionStarted && fcActiveProofSession == NULL)
            proofSessionStarted = fc_proof_session_begin(
                &proofSession, profile, board, forbiddenBlack);
        fc_refine_frozen_verified_loss(
            board, side, forbiddenBlack, profile, &baseline);
        if (fc_decision_deadline_reached()) {
            baseline = completedBeforeLossAware;
            baseline.stats.budgetExhausted = true;
        }
    }
    bool quietDefenseEligible = profile->quietThreatEnabled &&
        baseline.tacticalClass == FC_TACTICAL_FORCED_DEFENSE &&
        baseline.proofStatus == FC_PROOF_PROVEN_WIN &&
        baseline.proofCertificateVerified &&
        !fc_decision_deadline_reached();
    if (quietDefenseEligible) {
        FCAnalysisResult completedBeforeQuietDefense = baseline;
        if (!proofSessionStarted && fcActiveProofSession == NULL)
            proofSessionStarted = fc_proof_session_begin(
                &proofSession, profile, board, forbiddenBlack);
        int quietBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memcpy(quietBoard, board, sizeof(quietBoard));
        FCCandidate quietCandidates[FC_MAX_CANDIDATES];
        int quietCount = fc_generate_candidates(
            quietBoard, side, forbiddenBlack, profile,
            quietCandidates, FC_MAX_CANDIDATES, false);
        fc_examine_quiet_roots(
            quietBoard, side, forbiddenBlack, profile,
            quietCandidates, quietCount, &baseline);
        if (fc_decision_deadline_reached()) {
            baseline = completedBeforeQuietDefense;
            baseline.stats.budgetExhausted = true;
        }
    }
    if (profile->opponentGuardEnabled) {
        if (profile->earlyVCFSentinelEnabled && proofSessionStarted &&
            !fc_proof_session_reset_for_query(
                &proofSession, profile, board, forbiddenBlack)) {
            fc_proof_session_end(&proofSession, previousProofSession);
            proofSessionStarted = fc_proof_session_begin(
                &proofSession, profile, board, forbiddenBlack);
        }
        FCAnalysisResult completedBeforeGuard = baseline;
        if (!fc_apply_opponent_guard(
                board, side, forbiddenBlack, profile,
                generatedCandidates, generatedCount, &baseline)) {
            baseline = completedBeforeGuard;
            baseline.opponentGuardEligible = true;
            baseline.opponentGuardRollback = true;
            baseline.opponentGuardSkipReason =
                FC_GUARD_SKIP_ILLEGAL_PROVISIONAL;
            baseline.stats.budgetExhausted = true;
        }
        baseline.earlyVCFCacheReused =
            fcActiveEarlyVCFCache != NULL &&
            fcActiveEarlyVCFCache->hits > 0;
        if (baseline.earlyVCFEligible &&
            !baseline.earlyVCFAvoidedVerifiedLoss &&
            baseline.opponentGuardProvisionalClass ==
                FC_GUARD_CLASS_VERIFIED_LOSS) {
            baseline.earlyVCFFinalGuardOnlyLoss = true;
            fcProofDiagnostics.earlyVCFFinalGuardOnlyLosses++;
        }
        if (baseline.earlyVCFCacheReused &&
            baseline.opponentGuardProvisionalClass !=
                FC_GUARD_CLASS_VERIFIED_LOSS) {
            baseline.earlyVCFEvidenceMismatch = true;
            fcProofDiagnostics.earlyVCFEvidenceMismatches++;
        }
    }
    *result = baseline;
    result->fourStarX = fourStar.x;
    result->fourStarY = fourStar.y;
    if (forkLayerActive)
        fc_record_selected_fork_telemetry(
            result, baseline.defaultX, baseline.defaultY);
    result->corpusPositionIndex = -1;
    result->corpusReason = FC_CORPUS_NOT_CHECKED;
    result->corpusMatchType = FC_CORPUS_MATCH_NONE;
    if (!profile->eliteCorpusEnabled) {
        result->corpusReason = FC_CORPUS_NOT_CHECKED;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return fc_is_legal_move(board, result->x, result->y, side,
                                forbiddenBlack);
    }
    if (fc_decision_deadline_reached()) {
        result->stats.budgetExhausted = true;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return fc_is_legal_move(board, result->x, result->y, side,
                                forbiddenBlack);
    }

    FCEliteCorpusMove corpus[FC_MAX_CORPUS_CANDIDATES];
    int positionIndex = -1;
    int matchType = FC_CORPUS_MATCH_NONE;
    bool corpusQueryAllowed = fcActiveDecisionLedger == NULL ||
        (fc_decision_ledger_reserve_queries(fcActiveDecisionLedger, 1) &&
         fc_decision_ledger_consume_query(fcActiveDecisionLedger));
    int count = corpusQueryAllowed ? fc_elite_corpus_lookup_detailed(
        board, side, forbiddenBlack, corpus, FC_MAX_CORPUS_CANDIDATES,
        &positionIndex, &matchType) : 0;
    result->corpusLookup = count > 0;
    result->corpusPositionIndex = positionIndex;
    result->corpusCandidateCount = count;
    result->corpusMatchType = matchType;
    if (count > 0) {
        result->corpusTrustTier = corpus[0].trustTier;
        result->corpusRequiredStones = corpus[0].requiredStones;
        result->corpusSourceBoardMask = corpus[0].sourceBoardMask;
    }
    if (count == 0) {
        result->corpusReason = corpusQueryAllowed
            ? FC_CORPUS_NO_POSITION : FC_CORPUS_BASELINE_PROOF_UNKNOWN;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return true;
    }
    bool verifiedForcedDefense =
        baseline.tacticalClass == FC_TACTICAL_FORCED_DEFENSE &&
        baseline.overrideReason == FC_OVERRIDE_PROVEN_DEFENSE &&
        baseline.proofStatus == FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
    if (baseline.tacticalClass == FC_TACTICAL_IMMEDIATE_WIN ||
        baseline.tacticalClass == FC_TACTICAL_MUST_DEFEND ||
        baseline.tacticalClass == FC_TACTICAL_FORCED_ATTACK ||
        (profile->lossAwareEnabled && verifiedForcedDefense)) {
        result->corpusReason = FC_CORPUS_UNIQUE_TACTICAL_OBLIGATION;
        result->corpusProtectionApplied = profile->lossAwareEnabled &&
            verifiedForcedDefense;
        for (int i = 0; i < count; i++) {
            result->corpusCandidates[i] = (FCCorpusCandidateTelemetry){
                .x = corpus[i].x, .y = corpus[i].y,
                .games = corpus[i].games, .events = corpus[i].events,
                .sources = corpus[i].sources, .trustTier = corpus[i].trustTier,
                .matchType = corpus[i].matchType,
                .requiredStones = corpus[i].requiredStones,
                .sourceBoardMask = corpus[i].sourceBoardMask,
                .proofStatus = FC_PROOF_UNKNOWN,
                .proofSearchClass = profile->proofSearchClass,
                .tacticalClass = baseline.tacticalClass,
                .reason = FC_CORPUS_UNIQUE_TACTICAL_OBLIGATION
            };
        }
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return true;
    }

    /* The reversible forbidden-black legality cache is oracle-equivalent and
     * is now safe to use for shared multi-candidate DFPN sessions.  Any
     * mismatch invalidates the cache and the incremental path falls back to
     * fc_is_legal_move for that position. */
    if (fc_profile_proof_path_enabled(profile) && count > 1 &&
        !proofSessionStarted &&
        fcActiveProofSession == NULL) {
        proofSessionStarted = fc_proof_session_begin(
            &proofSession, profile, board, forbiddenBlack);
    }

    typedef struct {
        int x;
        int y;
        int score;
        int completedDepth;
        bool completed;
        bool safe;
    } FCCompletedScoreCacheEntry;
    FCCompletedScoreCacheEntry completedScoreCache[
        FC_MAX_CORPUS_CANDIDATES + 1];
    int completedScoreCacheCount = 0;

    FCProofResult baselineProof;
    int baselineProofStatus = fc_opponent_proof_after_move(
        board, side, forbiddenBlack, profile,
        baseline.x, baseline.y, &baselineProof);
    if (fc_decision_deadline_reached()) {
        result->stats.budgetExhausted = true;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return fc_is_legal_move(board, result->x, result->y, side,
                                forbiddenBlack);
    }
    bool baselineProvenUnsafe = baselineProofStatus == FC_PROOF_PROVEN_WIN &&
        baselineProof.certificateVerified;
    bool baselineProofUnknown = baselineProofStatus == FC_PROOF_UNKNOWN ||
        (baselineProofStatus == FC_PROOF_PROVEN_WIN &&
         !baselineProof.certificateVerified);
    int baselineDepth = 0;
    bool baselineCompleted = false;
    bool baselineSafe = false;
    int baselineScore = fc_completed_score_for_move(
        board, side, forbiddenBlack, seed ^ UINT64_C(0x4653544152434f4d),
        baseline.x, baseline.y, &baselineDepth, &baselineCompleted,
        &baselineSafe);
    completedScoreCache[completedScoreCacheCount++] =
        (FCCompletedScoreCacheEntry){
            baseline.x, baseline.y, baselineScore, baselineDepth,
            baselineCompleted, baselineSafe
        };
    if (fc_decision_deadline_reached()) {
        result->stats.budgetExhausted = true;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return fc_is_legal_move(board, result->x, result->y, side,
                                forbiddenBlack);
    }
    if (!baselineProvenUnsafe && (!baselineCompleted || !baselineSafe)) {
        result->corpusReason = FC_CORPUS_SEARCH_INCOMPARABLE;
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return true;
    }

    typedef struct { int index; int score; } FCAccepted;
    FCAccepted accepted[FC_MAX_CORPUS_CANDIDATES];
    int acceptedCount = 0;
    int baselineCorpusIndex = -1;
    int lastReason = FC_CORPUS_SCORE_MARGIN_NOT_MET;
    for (int i = 0; i < count; i++) {
        if (fc_decision_deadline_reached()) {
            result->stats.budgetExhausted = true;
            break;
        }
        FCCorpusCandidateTelemetry *telemetry = &result->corpusCandidates[i];
        *telemetry = (FCCorpusCandidateTelemetry){
            .x = corpus[i].x, .y = corpus[i].y,
            .games = corpus[i].games, .events = corpus[i].events,
            .sources = corpus[i].sources, .trustTier = corpus[i].trustTier,
            .matchType = corpus[i].matchType,
            .requiredStones = corpus[i].requiredStones,
            .sourceBoardMask = corpus[i].sourceBoardMask,
            .score = -FC_WIN_SCORE, .proofStatus = FC_PROOF_UNKNOWN,
            .proofSearchClass = profile->proofSearchClass,
            .tacticalClass = baseline.tacticalClass,
            .reason = FC_CORPUS_NOT_CHECKED
        };
        if (corpus[i].games < profile->corpusMinGames ||
            corpus[i].events < profile->corpusMinEvents) {
            telemetry->reason = FC_CORPUS_INSUFFICIENT_SUPPORT;
            lastReason = telemetry->reason;
            continue;
        }
        if (!fc_is_legal_move(board, corpus[i].x, corpus[i].y, side,
                              forbiddenBlack)) {
            telemetry->reason = FC_CORPUS_ILLEGAL_OR_UNSAFE;
            lastReason = telemetry->reason;
            continue;
        }
        if (!baselineProvenUnsafe &&
            corpus[i].x == baseline.x && corpus[i].y == baseline.y) {
            telemetry->score = baselineScore;
            telemetry->completedDepth = baselineDepth;
            telemetry->proofStatus = baselineProofStatus;
            telemetry->proofSearchClass = baselineProof.searchClass;
            telemetry->proofDistance = baselineProof.distance;
            telemetry->accepted = true;
            telemetry->reason = FC_CORPUS_ACCEPTED_BASELINE_EQUIVALENT;
            baselineCorpusIndex = i;
            continue;
        }
        FCProofResult candidateProof;
        int proofStatus = fc_opponent_proof_after_move(
            board, side, forbiddenBlack, profile,
            corpus[i].x, corpus[i].y, &candidateProof);
        telemetry->proofStatus = proofStatus;
        telemetry->proofSearchClass = candidateProof.searchClass;
        telemetry->proofDistance = candidateProof.distance;
        if (proofStatus == FC_PROOF_PROVEN_WIN &&
            candidateProof.certificateVerified) {
            telemetry->reason = FC_CORPUS_CANDIDATE_PROVEN_UNSAFE;
            lastReason = telemetry->reason;
            continue;
        }
        bool candidateProofUnknown = proofStatus !=
            FC_PROOF_NO_FORCED_WIN_IN_SCOPE;
        if (candidateProofUnknown && corpus[i].trustTier < 1) {
            telemetry->reason = FC_CORPUS_CANDIDATE_PROOF_UNKNOWN;
            lastReason = telemetry->reason;
            continue;
        }
        if (baselineProvenUnsafe) {
            int safetyBoard[FC_BOARD_SIZE][FC_BOARD_SIZE];
            memcpy(safetyBoard, board, sizeof(safetyBoard));
            if (!fc_move_is_safe(safetyBoard, corpus[i].x, corpus[i].y,
                                 side, forbiddenBlack)) {
                telemetry->reason = FC_CORPUS_ILLEGAL_OR_UNSAFE;
                lastReason = telemetry->reason;
                continue;
            }
            telemetry->score = 0;
            telemetry->completedDepth = candidateProof.completedDepth;
            telemetry->accepted = true;
            telemetry->reason = FC_CORPUS_ACCEPTED_SUPERIOR;
            accepted[acceptedCount++] = (FCAccepted){i, 0};
            continue;
        }
        int completedDepth = 0;
        bool completed = false;
        bool safe = false;
        int candidateScore = -FC_WIN_SCORE;
        int cachedScoreIndex = -1;
        for (int cacheIndex = 0;
             cacheIndex < completedScoreCacheCount; cacheIndex++) {
            if (completedScoreCache[cacheIndex].x == corpus[i].x &&
                completedScoreCache[cacheIndex].y == corpus[i].y) {
                cachedScoreIndex = cacheIndex;
                break;
            }
        }
        if (cachedScoreIndex >= 0) {
            FCCompletedScoreCacheEntry cached =
                completedScoreCache[cachedScoreIndex];
            candidateScore = cached.score;
            completedDepth = cached.completedDepth;
            completed = cached.completed;
            safe = cached.safe;
        } else {
            candidateScore = fc_completed_score_for_move(
                board, side, forbiddenBlack,
                seed ^ UINT64_C(0x4653544152434f4d),
                corpus[i].x, corpus[i].y, &completedDepth, &completed, &safe);
            if (completedScoreCacheCount < FC_MAX_CORPUS_CANDIDATES + 1) {
                completedScoreCache[completedScoreCacheCount++] =
                    (FCCompletedScoreCacheEntry){
                        corpus[i].x, corpus[i].y, candidateScore,
                        completedDepth, completed, safe
                    };
            }
        }
        telemetry->score = candidateScore;
        telemetry->completedDepth = completedDepth;
        if (!completed || completedDepth != baselineDepth) {
            telemetry->reason = FC_CORPUS_SEARCH_INCOMPARABLE;
            lastReason = telemetry->reason;
            continue;
        }
        if (!safe) {
            telemetry->reason = FC_CORPUS_ILLEGAL_OR_UNSAFE;
            lastReason = telemetry->reason;
            continue;
        }
        if (candidateScore < baselineScore + profile->corpusScoreMargin) {
            telemetry->reason = FC_CORPUS_SCORE_MARGIN_NOT_MET;
            lastReason = telemetry->reason;
            continue;
        }
        telemetry->accepted = true;
        telemetry->reason = candidateProofUnknown || baselineProofUnknown ||
                            candidateScore < baselineScore
            ? FC_CORPUS_ACCEPTED_TRUSTED_NEAR_EQUIVALENT
            : FC_CORPUS_ACCEPTED_SUPERIOR;
        accepted[acceptedCount++] = (FCAccepted){i, candidateScore};
    }

    if (acceptedCount == 0) {
        if (baselineCorpusIndex >= 0) {
            FCEliteCorpusMove chosen = corpus[baselineCorpusIndex];
            result->corpusAccepted = true;
            result->corpusReason = FC_CORPUS_ACCEPTED_BASELINE_EQUIVALENT;
            result->corpusSupportGames = chosen.games;
            result->corpusSupportEvents = chosen.events;
            result->corpusSupportSources = chosen.sources;
            result->corpusTrustTier = chosen.trustTier;
            result->corpusRequiredStones = chosen.requiredStones;
            result->corpusSourceBoardMask = chosen.sourceBoardMask;
            result->corpusAcceptedCandidateCount = 1;
        } else {
            result->corpusReason = lastReason;
        }
        result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
        if (proofSessionStarted)
            fc_proof_session_end(&proofSession, previousProofSession);
        return true;
    }

    int best = 0;
    for (int i = 1; i < acceptedCount; i++) {
        FCEliteCorpusMove left = corpus[accepted[i].index];
        FCEliteCorpusMove right = corpus[accepted[best].index];
        if (accepted[i].score > accepted[best].score ||
            (accepted[i].score == accepted[best].score && left.games > right.games))
            best = i;
    }
    int equivalent[FC_MAX_CORPUS_CANDIDATES];
    int equivalentCount = 0;
    FCEliteCorpusMove bestMove = corpus[accepted[best].index];
    for (int i = 0; i < acceptedCount; i++) {
        FCEliteCorpusMove move = corpus[accepted[i].index];
        FCCorpusCandidateTelemetry *candidateTelemetry =
            &result->corpusCandidates[accepted[i].index];
        FCCorpusCandidateTelemetry *bestTelemetry =
            &result->corpusCandidates[accepted[best].index];
        (void)move;
        (void)bestMove;
        if (fc_corpus_random_equivalent(
                bestTelemetry, accepted[best].score,
                candidateTelemetry, accepted[i].score))
            equivalent[equivalentCount++] = i;
    }
    int selected = best;
    int selectedRank = 0;
    for (int i = 0; i < equivalentCount; i++) {
        if (equivalent[i] == best) {
            selectedRank = i;
            break;
        }
    }
    if (randomMode == FC_RANDOM_USER_GAME && equivalentCount > 1) {
        FCRandom random;
        fc_random_seed(&random, seed ^ UINT64_C(0x454c495445524e44));
        selectedRank = (int)(fc_random_next(&random) %
                             (uint64_t)equivalentCount);
        selected = equivalent[selectedRank];
        result->randomSelectionUsed = selected != best;
    }
    int selectedIndex = accepted[selected].index;
    FCEliteCorpusMove chosen = corpus[selectedIndex];
    if (profile->opponentGuardEnabled &&
        (chosen.x != result->x || chosen.y != result->y)) {
        bool guardReady = fcActiveDecisionLedger != NULL &&
            fc_decision_ledger_enter_guard(fcActiveDecisionLedger);
        double previousDeadline = fcActiveDecisionDeadlineMilliseconds;
        FCOpponentGuardAudit corpusAudit;
        bool guardAccepted = false;
        if (guardReady) {
            fcActiveDecisionDeadlineMilliseconds =
                fcActiveDecisionLedger->internalDeadlineMilliseconds;
            bool audited = fc_audit_opponent_after_move(
                board, side, forbiddenBlack, profile,
                chosen.x, chosen.y, &corpusAudit);
            if (audited) {
                fc_guard_count_audit(
                    result, &corpusAudit, FC_GUARD_STAGE_CORPUS);
                guardAccepted = fc_guard_audit_matches_or_exceeds(
                    &corpusAudit, result->opponentGuardSelectedClass,
                    result->proofSearchClass, result->proofDistance);
            }
            fc_decision_ledger_leave_guard(fcActiveDecisionLedger);
            fcActiveDecisionDeadlineMilliseconds = previousDeadline;
        }
        if (!guardAccepted) {
            result->corpusCandidates[selectedIndex].accepted = false;
            result->corpusCandidates[selectedIndex].reason =
                guardReady &&
                corpusAudit.completedClass == FC_GUARD_CLASS_VERIFIED_LOSS
                    ? FC_CORPUS_CANDIDATE_PROVEN_UNSAFE
                    : FC_CORPUS_SEARCH_INCOMPARABLE;
            result->corpusReason =
                result->corpusCandidates[selectedIndex].reason;
            result->corpusAccepted = false;
            result->stats.elapsedMilliseconds =
                fc_now_milliseconds() - started;
            if (proofSessionStarted)
                fc_proof_session_end(&proofSession, previousProofSession);
            return true;
        }
        result->opponentGuardSelectedX = chosen.x;
        result->opponentGuardSelectedY = chosen.y;
        fc_guard_publish_selected(result, &corpusAudit);
    }
    result->x = chosen.x;
    result->y = chosen.y;
    result->score = accepted[selected].score;
    result->corpusAccepted = true;
    result->corpusReason = result->corpusCandidates[selectedIndex].reason;
    result->corpusSupportGames = chosen.games;
    result->corpusSupportEvents = chosen.events;
    result->corpusSupportSources = chosen.sources;
    result->corpusTrustTier = chosen.trustTier;
    result->corpusRequiredStones = chosen.requiredStones;
    result->corpusSourceBoardMask = chosen.sourceBoardMask;
    result->corpusAcceptedCandidateCount = equivalentCount;
    result->randomCandidateCount = equivalentCount;
    result->randomSelectedRank = selectedRank;
    result->randomEligibilityVerified = true;
    result->stats.elapsedMilliseconds = fc_now_milliseconds() - started;
    bool legal = fc_is_legal_move(board, result->x, result->y, side,
                                  forbiddenBlack);
    if (proofSessionStarted)
        fc_proof_session_end(&proofSession, previousProofSession);
    return legal;
}

bool fc_analyze_five_star_profile_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    const FCAIProfile *profile,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (profile == NULL)
        return false;
    double previousDeadline = fcActiveDecisionDeadlineMilliseconds;
    uint64_t previousParallelNodes = fcDecisionParallelNodes;
    int previousWorkers = fcDecisionWorkersLaunched;
    int previousJobs = fcDecisionParallelJobs;
    int previousCompletedJobs = fcDecisionParallelJobsCompleted;
    FCDecisionLedger ledger;
    FCDecisionLedger *previousLedger = fcActiveDecisionLedger;
    bool ledgerStarted = false;
    fcDecisionParallelNodes = 0;
    fcDecisionWorkersLaunched = 0;
    fcDecisionParallelJobs = 0;
    fcDecisionParallelJobsCompleted = 0;
    if (previousLedger == NULL &&
        (profile->recoverySearchEnabled ||
         profile->decisionLedgerVersion > 0 ||
         profile->decisionHardLimitMs > 0)) {
        ledgerStarted = fc_decision_ledger_begin(&ledger, profile);
        if (ledgerStarted) {
            fcActiveDecisionLedger = &ledger;
            if (profile->opponentGuardEnabled)
                (void)fc_decision_ledger_reserve_guard(
                    &ledger, profile->opponentGuardReservedNodes);
        }
    }
    if (ledgerStarted) {
        fcActiveDecisionDeadlineMilliseconds =
            profile->opponentGuardEnabled
                ? ledger.ordinaryDeadlineMilliseconds
                : ledger.internalDeadlineMilliseconds;
    } else if (profile->decisionTimeBudgetMs > 0) {
        double requestedDeadline = fc_now_milliseconds() +
                                   profile->decisionTimeBudgetMs;
        if (previousDeadline <= 0.0 || requestedDeadline < previousDeadline)
            fcActiveDecisionDeadlineMilliseconds = requestedDeadline;
    }
    FCEarlyVCFDecisionCache *earlyCache = NULL;
    FCEarlyVCFDecisionCache *previousEarlyCache = fcActiveEarlyVCFCache;
    bool earlyCacheStarted = profile->earlyVCFSentinelEnabled &&
        previousEarlyCache == NULL;
    bool earlyCacheMemoryReserved = false;
    if (earlyCacheStarted) {
        uint64_t cacheBytes = (uint64_t)sizeof(*earlyCache);
        if (fcActiveDecisionLedger == NULL ||
            fc_decision_ledger_reserve_memory(
                fcActiveDecisionLedger, cacheBytes)) {
            earlyCacheMemoryReserved = fcActiveDecisionLedger != NULL;
            earlyCache = calloc(1, sizeof(*earlyCache));
            fcProofDiagnostics.allocations++;
            fcProofDiagnostics.allocatedBytes += cacheBytes;
            if (earlyCache != NULL) {
                fcProofDiagnostics.clearedBytes += cacheBytes;
                fcActiveEarlyVCFCache = earlyCache;
            } else if (earlyCacheMemoryReserved) {
                fc_decision_ledger_release_memory(
                    fcActiveDecisionLedger, cacheBytes);
                earlyCacheMemoryReserved = false;
            }
        }
    }
    bool found = fc_analyze_five_star_profile_with_hint_internal(
        board, side, forbiddenBlack, profile, seed, randomMode,
        hintX, hintY, result);
    if (earlyCacheStarted) {
        fcActiveEarlyVCFCache = previousEarlyCache;
        free(earlyCache);
        if (earlyCacheMemoryReserved) {
            fc_decision_ledger_release_memory(
                fcActiveDecisionLedger,
                (uint64_t)sizeof(*earlyCache));
        }
    }
    if (result != NULL) {
        result->proofParallelNodes = fcDecisionParallelNodes;
        result->proofWorkerCap = (profile->parallelProofEnabled ||
                                  profile->branchFirstSearchEnabled)
            ? fc_effective_worker_count(profile) : 1;
        if (result->proofWorkerCap < 1) result->proofWorkerCap = 1;
        if (result->proofWorkerCap > 8) result->proofWorkerCap = 8;
        result->proofWorkersLaunched = fcDecisionWorkersLaunched;
        result->proofParallelJobs = fcDecisionParallelJobs;
        result->proofParallelJobsCompleted =
            fcDecisionParallelJobsCompleted;
        if (profile->branchFirstSearchEnabled)
            result->hybridComponent = FC_HYBRID_COMPONENT_V57_BRANCH_FIRST;
        if (ledgerStarted) fc_record_decision_ledger(&ledger, result);
        fc_finalize_random_telemetry(result);
    }
    if (ledgerStarted)
        fc_decision_ledger_release_guard(&ledger);
    fcDecisionParallelNodes = previousParallelNodes;
    fcDecisionWorkersLaunched = previousWorkers;
    fcDecisionParallelJobs = previousJobs;
    fcDecisionParallelJobsCompleted = previousCompletedJobs;
    fcActiveDecisionDeadlineMilliseconds = previousDeadline;
    fcActiveDecisionLedger = previousLedger;
    return found;
}

bool fc_analyze_five_star_color_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (side != 1 && side != -1) return false;
    FCAIProfile component = side == -1
        ? fc_profile_five_star_proof_engine_candidate()
        : fc_profile_five_star();
    /* Production v5.1 has no player-visible whole-decision deadline of its
     * own.  The hybrid preserves its search/profile parameters but adds an
     * outer reserve so corpus/proof cleanup still returns before 5 seconds. */
    if (side == 1) component.decisionTimeBudgetMs = 4300;
    bool found = fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &component, seed, randomMode,
        hintX, hintY, result);
    /* Frozen components predate proof-equivalent user randomness and may
     * occasionally choose a merely score-near baseline alternative.  The
     * candidate-only hybrid keeps the component search intact but vetoes
     * such an unverified policy choice and returns that same component's
     * best completed result.  Corpus alternatives already carry a complete
     * equivalence audit and therefore remain eligible for seeded variation. */
    if (found && result != NULL && randomMode == FC_RANDOM_USER_GAME &&
        result->randomSelectionUsed && !result->randomEligibilityVerified) {
        found = fc_analyze_five_star_profile_with_hint(
            board, side, forbiddenBlack, &component, seed,
            FC_RANDOM_EVALUATION, hintX, hintY, result);
        if (result != NULL) {
            result->seed = seed;
            result->randomMode = FC_RANDOM_USER_GAME;
            result->randomCandidateCount = 1;
            result->randomSelectionUsed = false;
            result->randomSelectedRank = 0;
            result->randomEligibilityVerified = true;
            fc_finalize_random_telemetry(result);
        }
    }
    if (result != NULL) {
        result->hybridComponent = side == -1
            ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
            : FC_HYBRID_COMPONENT_BLACK_V51;
    }
    return found;
}

bool fc_analyze_five_star_v521_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int blackWorkerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (side != 1 && side != -1) return false;
    if (!fc_research_worker_count_is_valid(blackWorkerCount))
        blackWorkerCount = 1;
    bool parallelBlack = blackWorkerCount > 1;
    FCAIProfile component = side == -1
        ? fc_profile_five_star_proof_engine_candidate()
        : parallelBlack
        ? fc_profile_five_star_v521_parallel_hybrid_candidate()
        : fc_profile_five_star_v521_serial_hybrid_control();
    if (side == 1) {
        component.parallelProofEnabled = parallelBlack;
        component.proofWorkerCount = blackWorkerCount;
        component.proofParallelNodeBudget = component.proofNodeBudget *
            (uint64_t)blackWorkerCount;
    }
    bool found = fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &component, seed, randomMode,
        hintX, hintY, result);
    if (found && result != NULL && randomMode == FC_RANDOM_USER_GAME &&
        result->randomSelectionUsed && !result->randomEligibilityVerified) {
        found = fc_analyze_five_star_profile_with_hint(
            board, side, forbiddenBlack, &component, seed,
            FC_RANDOM_EVALUATION, hintX, hintY, result);
        if (result != NULL) {
            result->seed = seed;
            result->randomMode = FC_RANDOM_USER_GAME;
            result->randomCandidateCount = 1;
            result->randomSelectionUsed = false;
            result->randomSelectedRank = 0;
            result->randomEligibilityVerified = true;
            fc_finalize_random_telemetry(result);
        }
    }
    if (result != NULL) {
        result->hybridComponent = side == -1
            ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
            : parallelBlack
            ? FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL
            : FC_HYBRID_COMPONENT_BLACK_V521_SERIAL;
    }
    return found;
}

bool fc_analyze_five_star_v57_hybrid_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int blackWorkerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (side != 1 && side != -1) return false;
    if (!fc_research_worker_count_is_valid(blackWorkerCount))
        blackWorkerCount = 1;
    FCAIProfile component = side == -1
        ? fc_profile_five_star_proof_engine_candidate()
        : fc_profile_five_star_v57_hybrid_candidate();
    if (side == -1) {
        /* Keep the measured white proof parameters, but expose the same
         * reversible decision/recovery contract as the v5.7 black path.  The
         * old 5.4 component otherwise bypasses fork recovery on exactly the
         * white fallback positions in the audit. */
        FCAIProfile recovery = fc_profile_five_star_v57_hybrid_candidate();
        component.decisionHardLimitMs = recovery.decisionHardLimitMs;
        component.decisionLedgerVersion = recovery.decisionLedgerVersion;
        component.decisionNodeBudget = recovery.decisionNodeBudget;
        component.decisionMemoryBudgetBytes =
            recovery.decisionMemoryBudgetBytes;
        component.decisionCorpusQueryBudget =
            recovery.decisionCorpusQueryBudget;
        component.incrementalLegalityEnabled =
            recovery.incrementalLegalityEnabled;
        component.validateLegalityCache = recovery.validateLegalityCache;
        component.recoverySearchEnabled = recovery.recoverySearchEnabled;
    }
    if (side == 1) {
        component.parallelProofEnabled = blackWorkerCount > 1;
        component.proofWorkerCount = blackWorkerCount;
        component.proofParallelNodeBudget = component.proofNodeBudget *
            (uint64_t)blackWorkerCount;
    }
    bool found = fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &component, seed, randomMode,
        hintX, hintY, result);
    if (found && result != NULL && randomMode == FC_RANDOM_USER_GAME &&
        result->randomSelectionUsed && !result->randomEligibilityVerified) {
        found = fc_analyze_five_star_profile_with_hint(
            board, side, forbiddenBlack, &component, seed,
            FC_RANDOM_EVALUATION, hintX, hintY, result);
        if (result != NULL) {
            result->seed = seed;
            result->randomMode = FC_RANDOM_USER_GAME;
            result->randomCandidateCount = 1;
            result->randomSelectionUsed = false;
            result->randomSelectedRank = 0;
            result->randomEligibilityVerified = true;
            fc_finalize_random_telemetry(result);
        }
    }
    if (result != NULL) {
        result->hybridComponent = side == -1
            ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
            : FC_HYBRID_COMPONENT_BLACK_V57_PARALLEL;
    }
    return found;
}

bool fc_analyze_five_star_v57_thread_scheduler_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int workerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (side != 1 && side != -1) return false;
    if (!fc_research_worker_count_is_valid(workerCount)) workerCount = 1;
    FCAIProfile scheduler =
        fc_profile_five_star_v57_thread_scheduler_candidate();
    FCAIProfile component = side == -1
        ? fc_profile_five_star_proof_engine_candidate() : scheduler;
    if (side == -1) {
        /* Preserve the measured v5.4 white search parameters while applying
         * only the scheduler study to its dispatcher and shared ledger. */
        component.decisionHardLimitMs = scheduler.decisionHardLimitMs;
        component.decisionLedgerVersion = scheduler.decisionLedgerVersion;
        component.decisionNodeBudget = scheduler.decisionNodeBudget;
        component.decisionMemoryBudgetBytes =
            scheduler.decisionMemoryBudgetBytes;
        component.decisionCorpusQueryBudget =
            scheduler.decisionCorpusQueryBudget;
        component.incrementalLegalityEnabled =
            scheduler.incrementalLegalityEnabled;
        component.validateLegalityCache = scheduler.validateLegalityCache;
        component.recoverySearchEnabled = scheduler.recoverySearchEnabled;
        component.persistentWorkerPoolEnabled =
            scheduler.persistentWorkerPoolEnabled;
        component.parallelTokenBlockEnabled =
            scheduler.parallelTokenBlockEnabled;
        component.parallelTokenBlockSize = scheduler.parallelTokenBlockSize;
    }
    if (side == 1) {
        component.parallelProofEnabled = workerCount > 1;
        component.proofWorkerCount = workerCount;
        component.proofParallelNodeBudget = component.proofNodeBudget *
            (uint64_t)workerCount;
    }
    bool found = fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &component, seed, randomMode,
        hintX, hintY, result);
    if (found && result != NULL && randomMode == FC_RANDOM_USER_GAME &&
        result->randomSelectionUsed && !result->randomEligibilityVerified) {
        found = fc_analyze_five_star_profile_with_hint(
            board, side, forbiddenBlack, &component, seed,
            FC_RANDOM_EVALUATION, hintX, hintY, result);
        if (result != NULL) {
            result->seed = seed;
            result->randomMode = FC_RANDOM_USER_GAME;
            result->randomCandidateCount = 1;
            result->randomSelectionUsed = false;
            result->randomSelectedRank = 0;
            result->randomEligibilityVerified = true;
            fc_finalize_random_telemetry(result);
        }
    }
    if (result != NULL)
        result->hybridComponent = side == -1
            ? FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE
            : FC_HYBRID_COMPONENT_BLACK_V57_PARALLEL;
    return found;
}

bool fc_analyze_five_star_v541_thread_scheduler_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    int workerCount,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    if (side != 1 && side != -1) return false;
    if (!fc_research_worker_count_is_valid(workerCount)) workerCount = 1;
    FCAIProfile component =
        fc_profile_five_star_v541_thread_scheduler_candidate();
    component.workerCountOverride = workerCount;
    component.proofWorkerCount = workerCount;
    bool found = fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &component, seed, randomMode,
        hintX, hintY, result);
    if (found && result != NULL && randomMode == FC_RANDOM_USER_GAME &&
        result->randomSelectionUsed && !result->randomEligibilityVerified) {
        found = fc_analyze_five_star_profile_with_hint(
            board, side, forbiddenBlack, &component, seed,
            FC_RANDOM_EVALUATION, hintX, hintY, result);
        if (result != NULL) {
            result->seed = seed;
            result->randomMode = FC_RANDOM_USER_GAME;
            result->randomCandidateCount = 1;
            result->randomSelectionUsed = false;
            result->randomSelectedRank = 0;
            result->randomEligibilityVerified = true;
            fc_finalize_random_telemetry(result);
        }
    }
    if (result != NULL)
        result->hybridComponent = FC_HYBRID_COMPONENT_V541_THREAD_SCHEDULER;
    return found;
}

const char *fc_hybrid_component_name(int component)
{
    switch (component) {
        case FC_HYBRID_COMPONENT_BLACK_V51:
            return "black-production-v51";
        case FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE:
            return "white-proof-engine";
        case FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL:
            return "black-v521-parallel-root-8w";
        case FC_HYBRID_COMPONENT_BLACK_V521_SERIAL:
            return "black-v521-serial-1w-control";
        case FC_HYBRID_COMPONENT_BLACK_V57_PARALLEL:
            return "black-v521-independent-root-parallel-8w-5s";
        case FC_HYBRID_COMPONENT_V57_BRANCH_FIRST:
            return "v57-branch-first-recursive-8w-5s";
        case FC_HYBRID_COMPONENT_V541_THREAD_SCHEDULER:
            return "v541-persistent-pool-token-blocks-8w-5s";
        default:
            return "none";
    }
}

const char *fc_hybrid_component_version(int component)
{
    switch (component) {
        case FC_HYBRID_COMPONENT_BLACK_V51:
            return "5.1.0-elite-rule-partitioned-local-v2";
        case FC_HYBRID_COMPONENT_WHITE_PROOF_ENGINE:
            return "5.4.1-transactional-deadline-root-parallel-5s";
        case FC_HYBRID_COMPONENT_BLACK_V521_PARALLEL:
            return "5.2.3-overlap-aware-deeper-root-parallel-8w-5s";
        case FC_HYBRID_COMPONENT_BLACK_V521_SERIAL:
            return "5.2.3-active-proof-serial-1w-5s-control";
        case FC_HYBRID_COMPONENT_BLACK_V57_PARALLEL:
            return "5.7.0-white-v541-black-v521-independent-root-parallel8-5s";
        case FC_HYBRID_COMPONENT_V57_BRANCH_FIRST:
            return "5.7.2-branch-first-preview-recursive-pool-14d-5s";
        case FC_HYBRID_COMPONENT_V541_THREAD_SCHEDULER:
            return "5.4.2-v541-persistent-pool-token-blocks-8w-5s";
        default:
            return "none";
    }
}

bool fc_analyze_five_star_with_hint(
    const int board[FC_BOARD_SIZE][FC_BOARD_SIZE],
    int side,
    bool forbiddenBlack,
    uint64_t seed,
    FCRandomMode randomMode,
    int hintX,
    int hintY,
    FCAnalysisResult *result)
{
    FCAIProfile profile = fc_profile_five_star();
    return fc_analyze_five_star_profile_with_hint(
        board, side, forbiddenBlack, &profile, seed, randomMode,
        hintX, hintY, result);
}

size_t fc_profile_snapshot(const FCAIProfile *profile, char *buffer, size_t capacity)
{
    if (profile == NULL || buffer == NULL || capacity == 0) return 0;
    size_t graphNodeCapacity = profile->proofGraphNodeCapacity > 0
        ? profile->proofGraphNodeCapacity : FC_DFPN_NODE_CAPACITY;
    size_t graphEdgeCapacity = profile->proofGraphEdgeCapacity > 0
        ? profile->proofGraphEdgeCapacity : FC_DFPN_EDGE_CAPACITY;
    if (graphNodeCapacity > FC_DFPN_NODE_CAPACITY)
        graphNodeCapacity = FC_DFPN_NODE_CAPACITY;
    if (graphEdgeCapacity > FC_DFPN_EDGE_CAPACITY)
        graphEdgeCapacity = FC_DFPN_EDGE_CAPACITY;
    size_t proofSessionMemoryBytes =
        profile->proofTranspositionCapacity * sizeof(FCDFPNTTEntry) +
        graphNodeCapacity * sizeof(FCDFPNNode) +
        graphEdgeCapacity * sizeof(FCDFPNEdge);
    size_t proofParallelPeakMemoryBytes = proofSessionMemoryBytes *
        (size_t)fc_effective_worker_count(profile);
    int length = snprintf(buffer, capacity,
        "{\"name\":\"%s\",\"version\":\"%s\",\"maxDepth\":%d,"
        "\"quiescenceDepth\":%d,\"fourDepth\":%d,"
        "\"doubleThreeDepth\":%d,\"forcingDepth\":%d,"
        "\"candidateLimit\":%d,\"nodeBudget\":%llu,"
        "\"timeBudgetMs\":%u,\"transpositionCapacity\":%zu,"
        "\"attackWeight\":%d,\"defenseWeight\":%d,"
        "\"centerWeight\":%d,\"nearBestWindow\":%d,"
        "\"randomTemperature\":%.3f,\"maxRandomCandidates\":%d,"
        "\"proofEnabled\":%s,\"proofEngineCandidate\":%s,"
        "\"proofCandidateStagesEnabled\":%s,"
        "\"parallelProofEnabled\":%s,"
        "\"openingBookEnabled\":%s,"
        "\"proofSearchClass\":%d,\"proofMaxDepth\":%d,"
        "\"proofNodeBudget\":%llu,\"proofParallelNodeBudget\":%llu,"
        "\"proofTimeBudgetMs\":%u,\"proofWorkerCount\":%d,"
        "\"workerCountOverride\":%d,"
        "\"proofTranspositionCapacity\":%zu,"
        "\"proofGraphNodeCapacity\":%zu,"
        "\"proofGraphEdgeCapacity\":%zu,"
        "\"proofSessionMemoryBytes\":%zu,"
        "\"proofParallelPeakMemoryBytes\":%zu,"
        "\"lossAwareEnabled\":%s,\"quietThreatEnabled\":%s,"
        "\"proofEscapeCandidateLimit\":%d,\"proofQuietRootLimit\":%d,"
        "\"proofEmergencyTimeBudgetMs\":%u,"
        "\"decisionTimeBudgetMs\":%u,"
        "\"decisionHardLimitMs\":%u,\"decisionLedgerVersion\":%u,"
        "\"decisionNodeBudget\":%llu,\"decisionMemoryBudgetBytes\":%zu,"
        "\"decisionCorpusQueryBudget\":%u,"
        "\"incrementalLegalityEnabled\":%s,"
        "\"validateLegalityCache\":%s,\"recoverySearchEnabled\":%s,"
        "\"forkFirstRecoveryEnabled\":%s,"
        "\"persistentWorkerPoolEnabled\":%s,"
        "\"parallelTokenBlockEnabled\":%s,"
        "\"parallelTokenBlockSize\":%u,"
        "\"branchFirstSearchEnabled\":%s,"
        "\"branchFirstMinRemainingDepth\":%d,"
        "\"branchFirstMinBranchCount\":%d,"
        "\"branchFirstMaxBranches\":%d,"
        "\"branchFirstPreviewDepth\":%d,"
        "\"branchFirstAdvancedFourDepthBonus\":%d,"
        "\"branchFirstAdvancedThreeDepthBonus\":%d,"
        "\"branchFirstTacticalDepthCap\":%d,"
        "\"opponentGuardEnabled\":%s,"
        "\"opponentGuardVCFMaxDepth\":%d,"
        "\"opponentGuardVCFNodeBudget\":%llu,"
        "\"opponentGuardVCFTimeBudgetMs\":%u,"
        "\"opponentGuardVCTMaxDepth\":%d,"
        "\"opponentGuardVCTNodeBudget\":%llu,"
        "\"opponentGuardVCTTimeBudgetMs\":%u,"
        "\"opponentGuardReservedNodes\":%llu,"
        "\"opponentGuardReservedTimeMs\":%u,"
        "\"opponentGuardStructuralVCTEnabled\":%s,"
        "\"opponentGuardMaxAlternatives\":%d,"
        "\"earlyVCFSentinelEnabled\":%s,"
        "\"earlyVCFSentinelPolicy\":%d,"
        "\"earlyVCFBaseDepth\":%d,"
        "\"earlyVCFMaxDepth\":%d,"
        "\"earlyVCFNodeBudget\":%llu,"
        "\"earlyVCFTimeBudgetMs\":%u,"
        "\"earlyVCFMaxAlternatives\":%d,"
        "\"eliteCorpusEnabled\":%s,\"corpusVersion\":\"%s\","
        "\"corpusScoreMargin\":%d,\"corpusMinGames\":%d,"
        "\"corpusMinEvents\":%d}",
        profile->name, profile->version, profile->maxDepth,
        profile->quiescenceDepth, profile->fourDepth,
        profile->doubleThreeDepth, profile->forcingDepth,
        profile->candidateLimit, (unsigned long long)profile->nodeBudget,
        profile->timeBudgetMs, profile->transpositionCapacity,
        profile->attackWeight, profile->defenseWeight,
        profile->centerWeight, profile->nearBestWindow,
        profile->randomTemperature, profile->maxRandomCandidates,
        profile->proofEnabled ? "true" : "false",
        profile->proofEngineCandidate ? "true" : "false",
        profile->proofCandidateStagesEnabled ? "true" : "false",
        profile->parallelProofEnabled ? "true" : "false",
        profile->openingBookEnabled ? "true" : "false",
        profile->proofSearchClass, profile->proofMaxDepth,
        (unsigned long long)profile->proofNodeBudget,
        (unsigned long long)profile->proofParallelNodeBudget,
        profile->proofTimeBudgetMs, profile->proofWorkerCount,
        profile->workerCountOverride,
        profile->proofTranspositionCapacity,
        profile->proofGraphNodeCapacity, profile->proofGraphEdgeCapacity,
        proofSessionMemoryBytes, proofParallelPeakMemoryBytes,
        profile->lossAwareEnabled ? "true" : "false",
        profile->quietThreatEnabled ? "true" : "false",
        profile->proofEscapeCandidateLimit, profile->proofQuietRootLimit,
        profile->proofEmergencyTimeBudgetMs,
        profile->decisionTimeBudgetMs,
        profile->decisionHardLimitMs,
        profile->decisionLedgerVersion,
        (unsigned long long)profile->decisionNodeBudget,
        profile->decisionMemoryBudgetBytes,
        profile->decisionCorpusQueryBudget,
        profile->incrementalLegalityEnabled ? "true" : "false",
        profile->validateLegalityCache ? "true" : "false",
        profile->recoverySearchEnabled ? "true" : "false",
        profile->forkFirstRecoveryEnabled ? "true" : "false",
        profile->persistentWorkerPoolEnabled ? "true" : "false",
        profile->parallelTokenBlockEnabled ? "true" : "false",
        profile->parallelTokenBlockSize,
        profile->branchFirstSearchEnabled ? "true" : "false",
        profile->branchFirstMinRemainingDepth,
        profile->branchFirstMinBranchCount,
        profile->branchFirstMaxBranches,
        profile->branchFirstPreviewDepth,
        profile->branchFirstAdvancedFourDepthBonus,
        profile->branchFirstAdvancedThreeDepthBonus,
        profile->branchFirstTacticalDepthCap,
        profile->opponentGuardEnabled ? "true" : "false",
        profile->opponentGuardVCFMaxDepth,
        (unsigned long long)profile->opponentGuardVCFNodeBudget,
        profile->opponentGuardVCFTimeBudgetMs,
        profile->opponentGuardVCTMaxDepth,
        (unsigned long long)profile->opponentGuardVCTNodeBudget,
        profile->opponentGuardVCTTimeBudgetMs,
        (unsigned long long)profile->opponentGuardReservedNodes,
        profile->opponentGuardReservedTimeMs,
        profile->opponentGuardStructuralVCTEnabled ? "true" : "false",
        profile->opponentGuardMaxAlternatives,
        profile->earlyVCFSentinelEnabled ? "true" : "false",
        profile->earlyVCFSentinelPolicy,
        profile->earlyVCFBaseDepth,
        profile->earlyVCFMaxDepth,
        (unsigned long long)profile->earlyVCFNodeBudget,
        profile->earlyVCFTimeBudgetMs,
        profile->earlyVCFMaxAlternatives,
        profile->eliteCorpusEnabled ? "true" : "false",
        profile->eliteCorpusEnabled ? fc_elite_corpus_version() : "none",
        profile->corpusScoreMargin, profile->corpusMinGames,
        profile->corpusMinEvents);
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
        case FC_OVERRIDE_UNPROVEN_ESCAPE: return "unproven-escape";
        case FC_OVERRIDE_LONGEST_SURVIVAL: return "longest-survival";
        case FC_OVERRIDE_QUIET_PROVEN_ATTACK: return "quiet-proven-attack";
        case FC_OVERRIDE_FORK_SAFE_RECOVERY: return "fork-safe-recovery";
        default: return "none";
    }
}

const char *fc_decision_status_name(int status)
{
    switch (status) {
        case FC_DECISION_VERIFIED_WIN: return "verified-win";
        case FC_DECISION_VERIFIED_LOSS: return "verified-loss";
        case FC_DECISION_UNKNOWN_OR_DEADLINE: return "unknown-or-deadline";
        case FC_DECISION_NO_LEGAL_MOVE: return "no-legal-move";
        default: return "unknown-status";
    }
}

const char *fc_fork_risk_name(int risk)
{
    switch (risk) {
        case FC_FORK_RISK_SAFE: return "safe";
        case FC_FORK_RISK_ONE_REPLY: return "one-reply";
        case FC_FORK_RISK_FORK: return "fork";
        case FC_FORK_RISK_OWN_WIN: return "own-win";
        default: return "unknown-or-deadline";
    }
}

const char *fc_handoff_reason_name(int reason)
{
    switch (reason) {
        case FC_HANDOFF_FOUR_STAR_INVALID: return "four-star-invalid";
        case FC_HANDOFF_FOUR_STAR_UNKNOWN: return "four-star-unknown";
        case FC_HANDOFF_GENERATED_FALLBACK: return "generated-candidate-recovery";
        case FC_HANDOFF_FULL_BOARD_FALLBACK: return "full-board-fallback";
        default: return "none";
    }
}

const char *fc_escape_stage_name(int stage)
{
    switch (stage) {
        case FC_ESCAPE_CERTIFICATE: return "certificate";
        case FC_ESCAPE_TACTICAL: return "tactical";
        case FC_ESCAPE_ORDINARY: return "ordinary";
        case FC_ESCAPE_ALL_LEGAL: return "all-legal";
        default: return "none";
    }
}

const char *fc_loss_reason_name(int reason)
{
    switch (reason) {
        case FC_LOSS_SELECTED_VERIFIED: return "selected-verified-losing";
        case FC_LOSS_ALL_EXAMINED_VERIFIED:
            return "all-examined-verified-losing";
        case FC_LOSS_BUDGET_UNKNOWN: return "budget-unknown";
        case FC_LOSS_NO_IMMEDIATE_SAFE_GENERATED:
            return "no-immediate-safe-generated";
        default: return "none";
    }
}

const char *fc_corpus_reason_name(int reason)
{
    switch (reason) {
        case FC_CORPUS_NO_POSITION: return "no-position";
        case FC_CORPUS_INSUFFICIENT_SUPPORT: return "insufficient-support";
        case FC_CORPUS_UNIQUE_TACTICAL_OBLIGATION: return "unique-tactical-obligation";
        case FC_CORPUS_ILLEGAL_OR_UNSAFE: return "illegal-or-unsafe";
        case FC_CORPUS_BASELINE_PROOF_UNKNOWN: return "baseline-proof-unknown";
        case FC_CORPUS_CANDIDATE_PROOF_UNKNOWN: return "candidate-proof-unknown";
        case FC_CORPUS_CANDIDATE_PROVEN_UNSAFE: return "candidate-proven-unsafe";
        case FC_CORPUS_SEARCH_INCOMPARABLE: return "search-incomparable";
        case FC_CORPUS_SCORE_MARGIN_NOT_MET: return "score-margin-not-met";
        case FC_CORPUS_ACCEPTED_SUPERIOR: return "accepted-superior";
        case FC_CORPUS_ACCEPTED_BASELINE_EQUIVALENT: return "accepted-baseline-equivalent";
        case FC_CORPUS_BASELINE_PROVEN_UNSAFE: return "baseline-proven-unsafe";
        case FC_CORPUS_ACCEPTED_TRUSTED_NEAR_EQUIVALENT:
            return "accepted-trusted-near-equivalent";
        default: return "not-checked";
    }
}
