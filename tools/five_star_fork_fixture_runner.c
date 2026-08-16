#include "../ice five chess/FiveChessAI.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool parse_int(const char *text, int *value)
{
    char *end = NULL;
    long parsed;
    if (text == NULL || value == NULL) return false;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') return false;
    *value = (int)parsed;
    return true;
}

static bool parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;
    if (text == NULL || value == NULL) return false;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') return false;
    *value = (uint64_t)parsed;
    return true;
}

static double cpu_milliseconds(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) return 0.0;
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static bool decode_board(const char *encoded,
                         int board[FC_BOARD_SIZE][FC_BOARD_SIZE])
{
    if (encoded == NULL || board == NULL ||
        strlen(encoded) != FC_BOARD_SIZE * FC_BOARD_SIZE) return false;
    memset(board, 0, sizeof(int) * FC_BOARD_SIZE * FC_BOARD_SIZE);
    for (int x = 0; x < FC_BOARD_SIZE; x++) {
        for (int y = 0; y < FC_BOARD_SIZE; y++) {
            char cell = encoded[x * FC_BOARD_SIZE + y];
            if (cell == '.') board[x][y] = 0;
            else if (cell == 'X') board[x][y] = 1;
            else if (cell == 'O') board[x][y] = -1;
            else return false;
        }
    }
    return true;
}

static void emit_uint64(FILE *output, uint64_t value)
{
    fprintf(output, "%" PRIu64, value);
}

static void emit_diagnostics(FILE *output, const FCProofDiagnostics *diagnostics)
{
    fprintf(output,
            "{\"parallelBatches\":");
    emit_uint64(output, diagnostics->parallelBatches);
    fprintf(output, ",\"allocations\":");
    emit_uint64(output, diagnostics->allocations);
    fprintf(output, ",\"clearedBytes\":");
    emit_uint64(output, diagnostics->clearedBytes);
    fprintf(output, ",\"proofSessionQueries\":");
    emit_uint64(output, diagnostics->proofSessionQueries);
    fprintf(output, ",\"proofSessionGlobalTerminations\":");
    emit_uint64(output, diagnostics->proofSessionGlobalTerminations);
    fprintf(output, ",\"parallelWorkersLaunched\":");
    emit_uint64(output, diagnostics->parallelWorkersLaunched);
    fprintf(output, ",\"parallelRootJobs\":");
    emit_uint64(output, diagnostics->parallelRootJobs);
    fprintf(output, ",\"parallelRootJobsCompleted\":");
    emit_uint64(output, diagnostics->parallelRootJobsCompleted);
    fprintf(output, ",\"parallelMaxConcurrentWorkers\":");
    emit_uint64(output, diagnostics->parallelMaxConcurrentWorkers);
    fprintf(output, ",\"parallelIndependentDispatches\":");
    emit_uint64(output, diagnostics->parallelIndependentDispatches);
    fprintf(output, ",\"parallelPoolDispatches\":");
    emit_uint64(output, diagnostics->parallelPoolDispatches);
    fprintf(output, ",\"parallelPoolWorkersReused\":");
    emit_uint64(output, diagnostics->parallelPoolWorkersReused);
    fprintf(output, ",\"parallelPoolFallbacks\":");
    emit_uint64(output, diagnostics->parallelPoolFallbacks);
    fprintf(output, ",\"parallelTokenBlockClaims\":");
    emit_uint64(output, diagnostics->parallelTokenBlockClaims);
    fprintf(output, ",\"parallelTokenBlockTokens\":");
    emit_uint64(output, diagnostics->parallelTokenBlockTokens);
    fprintf(output, ",\"parallelTokenBlockReturns\":");
    emit_uint64(output, diagnostics->parallelTokenBlockReturns);
    fprintf(output, ",\"branchFirstPreviewBranches\":");
    emit_uint64(output, diagnostics->branchFirstPreviewBranches);
    fprintf(output, ",\"branchFirstAdvancedFourPreviews\":");
    emit_uint64(output, diagnostics->branchFirstAdvancedFourPreviews);
    fprintf(output, ",\"branchFirstAdvancedThreePreviews\":");
    emit_uint64(output, diagnostics->branchFirstAdvancedThreePreviews);
    fprintf(output, ",\"branchFirstPreviewIncomplete\":");
    emit_uint64(output, diagnostics->branchFirstPreviewIncomplete);
    fprintf(output, ",\"branchFirstWaves\":");
    emit_uint64(output, diagnostics->branchFirstWaves);
    fprintf(output, ",\"branchFirstWorkersLaunched\":");
    emit_uint64(output, diagnostics->branchFirstWorkersLaunched);
    fprintf(output, ",\"branchFirstJobs\":");
    emit_uint64(output, diagnostics->branchFirstJobs);
    fprintf(output, ",\"branchFirstJobsCompleted\":");
    emit_uint64(output, diagnostics->branchFirstJobsCompleted);
    fprintf(output, ",\"branchFirstVerifiedJobs\":");
    emit_uint64(output, diagnostics->branchFirstVerifiedJobs);
    fprintf(output, ",\"branchFirstUsefulJobs\":");
    emit_uint64(output, diagnostics->branchFirstUsefulJobs);
    fprintf(output, ",\"branchFirstMergeFailures\":");
    emit_uint64(output, diagnostics->branchFirstMergeFailures);
    fprintf(output, ",\"branchFirstUnknownJobs\":");
    emit_uint64(output, diagnostics->branchFirstUnknownJobs);
    fprintf(output, ",\"branchFirstMaxConcurrentWorkers\":");
    emit_uint64(output, diagnostics->branchFirstMaxConcurrentWorkers);
    fprintf(output, ",\"branchFirstDispatchFallbacks\":");
    emit_uint64(output, diagnostics->branchFirstDispatchFallbacks);
    fprintf(output, ",\"branchFirstSerialFallbacks\":");
    emit_uint64(output, diagnostics->branchFirstSerialFallbacks);
    fprintf(output, ",\"branchFirstDepthExtensions\":");
    emit_uint64(output, diagnostics->branchFirstDepthExtensions);
    fprintf(output, ",\"branchFirstAdvancedFourDepthExtensions\":");
    emit_uint64(output, diagnostics->branchFirstAdvancedFourDepthExtensions);
    fprintf(output, ",\"branchFirstAdvancedThreeDepthExtensions\":");
    emit_uint64(output, diagnostics->branchFirstAdvancedThreeDepthExtensions);
    fprintf(output, ",\"branchFirstMaxChildDepth\":");
    emit_uint64(output, diagnostics->branchFirstMaxChildDepth);
    fprintf(output, ",\"branchFirstDeadlineStops\":");
    emit_uint64(output, diagnostics->branchFirstDeadlineStops);
    fprintf(output, ",\"parallelEarlyStops\":");
    emit_uint64(output, diagnostics->parallelEarlyStops);
    fprintf(output, ",\"parallelEscapeBatches\":");
    emit_uint64(output, diagnostics->parallelEscapeBatches);
    fprintf(output, ",\"parallelEscapeWorkersLaunched\":");
    emit_uint64(output, diagnostics->parallelEscapeWorkersLaunched);
    fprintf(output, ",\"parallelEscapeJobs\":");
    emit_uint64(output, diagnostics->parallelEscapeJobs);
    fprintf(output, ",\"parallelEscapeJobsCompleted\":");
    emit_uint64(output, diagnostics->parallelEscapeJobsCompleted);
    fprintf(output, ",\"parallelEscapeMaxConcurrentWorkers\":");
    emit_uint64(output, diagnostics->parallelEscapeMaxConcurrentWorkers);
    fprintf(output, ",\"decisionUnknowns\":");
    emit_uint64(output, diagnostics->decisionUnknowns);
    fprintf(output, ",\"decisionFallbacks\":");
    emit_uint64(output, diagnostics->decisionFallbacks);
    fprintf(output, ",\"decisionNoLegalMoves\":");
    emit_uint64(output, diagnostics->decisionNoLegalMoves);
    fprintf(output, ",\"decisionVerifiedWins\":");
    emit_uint64(output, diagnostics->decisionVerifiedWins);
    fprintf(output, ",\"decisionVerifiedLosses\":");
    emit_uint64(output, diagnostics->decisionVerifiedLosses);
    fprintf(output, ",\"decisionLedgerExhaustions\":");
    emit_uint64(output, diagnostics->decisionLedgerExhaustions);
    fprintf(output, ",\"decisionMemoryLiveReserved\":");
    emit_uint64(output, diagnostics->decisionMemoryLiveReserved);
    fprintf(output, ",\"decisionMemoryPeakReserved\":");
    emit_uint64(output, diagnostics->decisionMemoryPeakReserved);
    fprintf(output, ",\"decisionMemoryReleased\":");
    emit_uint64(output, diagnostics->decisionMemoryReleased);
    fprintf(output, ",\"forkProbeCandidatesExamined\":");
    emit_uint64(output, diagnostics->forkProbeCandidatesExamined);
    fprintf(output, ",\"forkProbeSafeCandidates\":");
    emit_uint64(output, diagnostics->forkProbeSafeCandidates);
    fprintf(output, ",\"forkProbeRiskyCandidates\":");
    emit_uint64(output, diagnostics->forkProbeRiskyCandidates);
    fprintf(output, ",\"forkProbeUnknownCandidates\":");
    emit_uint64(output, diagnostics->forkProbeUnknownCandidates);
    fprintf(output, ",\"forkProbeAvoidedForks\":");
    emit_uint64(output, diagnostics->forkProbeAvoidedForks);
    fprintf(output, ",\"forkProbeIncompleteDecisions\":");
    emit_uint64(output, diagnostics->forkProbeIncompleteDecisions);
    fputc('}', output);
}

static void emit_record(const char *id,
                        int worker_count,
                        const FCAIProfile *profile,
                        bool found,
                        bool legal,
                        double cpu_ms,
                        const FCAnalysisResult *result,
                        const FCProofDiagnostics *diagnostics)
{
    char decision_status[64];
    char fork_risk[32];
    char handoff_reason[64];
    snprintf(decision_status, sizeof(decision_status), "%s",
             fc_decision_status_name(result->decisionStatus));
    snprintf(fork_risk, sizeof(fork_risk), "%s",
             fc_fork_risk_name(result->forkRiskStatus));
    snprintf(handoff_reason, sizeof(handoff_reason), "%s",
             fc_handoff_reason_name(result->handoffReason));
    fprintf(stdout,
            "{\"type\":\"fixture\",\"id\":\"%s\","
            "\"requestedWorkers\":%d,\"actualWorkerCap\":%d,"
            "\"profileVersion\":\"%s\","
            "\"persistentWorkerPoolEnabled\":%s,"
            "\"branchFirstSearchEnabled\":%s,"
            "\"parallelTokenBlockSize\":%u,"
            "\"proofMaxDepth\":%d,"
            "\"branchFirstAdvancedFourDepthBonus\":%d,"
            "\"branchFirstAdvancedThreeDepthBonus\":%d,"
            "\"branchFirstTacticalDepthCap\":%d,"
            "\"found\":%s,\"legal\":%s,\"x\":%d,\"y\":%d,"
            "\"cpuMs\":%.3f,\"elapsedMs\":%.3f,"
            "\"decisionStatus\":%d,\"decisionStatusName\":\"%s\","
            "\"forkRisk\":%d,\"forkRiskName\":\"%s\","
            "\"forkProbeComplete\":%s,\"candidateCoverageComplete\":%s,"
            "\"forkCandidatesExamined\":%d,\"forkSafeCandidates\":%d,"
            "\"forkRiskyCandidates\":%d,\"forkUnknownCandidates\":%d,"
            "\"forkAvoidedCount\":%d,\"selectedOpponentImmediateWinCount\":%d,"
            "\"fallbackUsed\":%s,\"provenLoss\":%s,"
            "\"handoffReason\":%d,\"handoffReasonName\":\"%s\","
            "\"overrideReason\":%d,\"candidateCount\":%d,"
            "\"proofWorkerCap\":%d,\"proofWorkersLaunched\":%d,"
            "\"proofParallelJobs\":%d,\"proofParallelJobsCompleted\":%d,"
            "\"decisionMemoryReserved\":",
            id, worker_count, result->proofWorkerCap,
            profile != NULL ? profile->version : "unknown",
            profile != NULL && profile->persistentWorkerPoolEnabled
                ? "true" : "false",
            profile != NULL && profile->branchFirstSearchEnabled
                ? "true" : "false",
            profile != NULL ? profile->parallelTokenBlockSize : 0U,
            profile != NULL ? profile->proofMaxDepth : 0,
            profile != NULL ? profile->branchFirstAdvancedFourDepthBonus : 0,
            profile != NULL ? profile->branchFirstAdvancedThreeDepthBonus : 0,
            profile != NULL ? profile->branchFirstTacticalDepthCap : 0,
            found ? "true" : "false", legal ? "true" : "false",
            result->x, result->y, cpu_ms,
            result->stats.elapsedMilliseconds,
            result->decisionStatus, decision_status,
            result->forkRiskStatus, fork_risk,
            result->forkProbeComplete ? "true" : "false",
            result->candidateCoverageComplete ? "true" : "false",
            result->forkCandidatesExamined, result->forkSafeCandidates,
            result->forkRiskyCandidates, result->forkUnknownCandidates,
            result->forkAvoidedCount,
            result->selectedOpponentImmediateWinCount,
            result->fallbackUsed ? "true" : "false",
            result->provenLoss ? "true" : "false",
            result->handoffReason, handoff_reason, result->overrideReason,
            result->candidateCount, result->proofWorkerCap,
            result->proofWorkersLaunched, result->proofParallelJobs,
            result->proofParallelJobsCompleted);
    emit_uint64(stdout, result->decisionMemoryReserved);
    fprintf(stdout, ",\"decisionMemoryConsumed\":");
    emit_uint64(stdout, result->decisionMemoryConsumed);
    fprintf(stdout, ",\"decisionMemoryPeakReserved\":");
    emit_uint64(stdout, result->decisionMemoryPeakReserved);
    fprintf(stdout, ",\"decisionMemoryReleased\":");
    emit_uint64(stdout, result->decisionMemoryReleased);
    fprintf(stdout, ",\"decisionStageExhaustions\":");
    emit_uint64(stdout, result->decisionStageExhaustions);
    fprintf(stdout, ",\"diagnostics\":");
    emit_diagnostics(stdout, diagnostics);
    fputs("}\n", stdout);
}

int main(int argc, char **argv)
{
    bool baseline = false;
    bool scheduler = false;
    bool v541_baseline = false;
    bool v541_scheduler = false;
    bool branch_first = false;
    int token_block_size = 0;
    if (argc < 3 || strcmp(argv[1], "--worker-count") != 0) {
        fprintf(stderr,
                "usage: %s --worker-count 1|4|8 [--baseline|--scheduler|"
                "--v541-baseline|--v541-scheduler|--branch-first] "
                "[--token-block-size N]\n",
                argv[0]);
        return 2;
    }
    for (int argument = 3; argument < argc; argument++) {
        if (strcmp(argv[argument], "--baseline") == 0) {
            baseline = true;
        } else if (strcmp(argv[argument], "--scheduler") == 0) {
            scheduler = true;
        } else if (strcmp(argv[argument], "--v541-baseline") == 0) {
            v541_baseline = true;
        } else if (strcmp(argv[argument], "--v541-scheduler") == 0) {
            v541_scheduler = true;
        } else if (strcmp(argv[argument], "--branch-first") == 0) {
            branch_first = true;
        } else if (strcmp(argv[argument], "--token-block-size") == 0 &&
                   argument + 1 < argc &&
                   parse_int(argv[++argument], &token_block_size)) {
            if (token_block_size <= 0 || token_block_size > 1048576) {
                fprintf(stderr, "token block size must be in 1..1048576\n");
                return 2;
            }
        } else {
            fprintf(stderr,
                    "usage: %s --worker-count 1|4|8 [--baseline|--scheduler|"
                    "--v541-baseline|--v541-scheduler|--branch-first] "
                    "[--token-block-size N]\n",
                    argv[0]);
            return 2;
        }
    }
    if ((baseline ? 1 : 0) + (scheduler ? 1 : 0) +
            (v541_baseline ? 1 : 0) + (v541_scheduler ? 1 : 0) +
            (branch_first ? 1 : 0) > 1) {
        fprintf(stderr, "choose at most one profile mode\n");
        return 2;
    }
    int worker_count = 0;
    if (!parse_int(argv[2], &worker_count) ||
        !fc_research_worker_count_is_valid(worker_count)) {
        fprintf(stderr, "invalid worker count: %s (expected 1, 4, or 8)\n",
                argv[2]);
        return 2;
    }

    FCAIProfile profile = baseline
        ? fc_profile_five_star_v57_hybrid_candidate()
        : scheduler
        ? fc_profile_five_star_v57_thread_scheduler_candidate()
        : v541_baseline
        ? fc_profile_five_star_proof_engine_candidate()
        : v541_scheduler
        ? fc_profile_five_star_v541_thread_scheduler_candidate()
        : branch_first
        ? fc_profile_five_star_v57_branch_first_candidate()
        : fc_profile_five_star_v57_fork_recovery_candidate();
    if (token_block_size > 0)
        profile.parallelTokenBlockSize = (uint32_t)token_block_size;
    profile.workerCountOverride = worker_count;
    char line[FC_BOARD_SIZE * FC_BOARD_SIZE + 512];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;
        char *id = strtok(line, "\t");
        char *forbidden_text = strtok(NULL, "\t");
        char *side_text = strtok(NULL, "\t");
        char *hint_x_text = strtok(NULL, "\t");
        char *hint_y_text = strtok(NULL, "\t");
        char *seed_text = strtok(NULL, "\t");
        char *board_text = strtok(NULL, "\t");
        int forbidden_value = 0;
        int side = 0;
        int hint_x = 0;
        int hint_y = 0;
        uint64_t seed = 0;
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        if (id == NULL || forbidden_text == NULL || side_text == NULL ||
            hint_x_text == NULL || hint_y_text == NULL || seed_text == NULL ||
            board_text == NULL ||
            !parse_int(forbidden_text, &forbidden_value) ||
            !parse_int(side_text, &side) || !parse_int(hint_x_text, &hint_x) ||
            !parse_int(hint_y_text, &hint_y) || !parse_u64(seed_text, &seed) ||
            (side != 1 && side != -1) ||
            !decode_board(board_text, board)) {
            fprintf(stderr, "invalid fixture input line\n");
            return 3;
        }
        bool forbidden = forbidden_value != 0;
        FCAnalysisResult result;
        fc_proof_diagnostics_reset();
        double cpu_started = cpu_milliseconds();
        bool found = v541_scheduler
            ? fc_analyze_five_star_v541_thread_scheduler_with_hint(
                (const int (*)[FC_BOARD_SIZE])board, side, forbidden,
                worker_count, seed, FC_RANDOM_EVALUATION,
                hint_x, hint_y, &result)
            : fc_analyze_five_star_profile_with_hint(
                (const int (*)[FC_BOARD_SIZE])board, side, forbidden,
                &profile, seed, FC_RANDOM_EVALUATION, hint_x, hint_y, &result);
        double cpu_ms = cpu_milliseconds() - cpu_started;
        bool legal = found && fc_is_legal_move(
            (const int (*)[FC_BOARD_SIZE])board, result.x, result.y,
            side, forbidden);
        FCProofDiagnostics diagnostics = fc_proof_diagnostics_get();
        emit_record(id, worker_count, &profile, found, legal, cpu_ms,
                    &result, &diagnostics);
    }
    return ferror(stdin) ? 4 : 0;
}
