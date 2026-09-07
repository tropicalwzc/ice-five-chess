#!/usr/bin/env python3
"""Verify that production-only cleanup preserves all live parameters and tactics."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "openspec/changes/prune-five-star-experimental-branches/evidence/production-before.txt"
RETIRED_ZERO_FIELDS = set(["incrementalLegalityEnabled","validateLegalityCache","recoverySearchEnabled","forkFirstRecoveryEnabled","persistentWorkerPoolEnabled","parallelTokenBlockEnabled","parallelTokenBlockSize","branchFirstSearchEnabled","branchFirstMinRemainingDepth","branchFirstMinBranchCount","branchFirstMaxBranches","branchFirstPreviewDepth","branchFirstAdvancedFourDepthBonus","branchFirstAdvancedThreeDepthBonus","branchFirstTacticalDepthCap","opponentGuardImmediateBlockEnabled","opponentGuardTwoStepForkEnabled","opponentGuardForkMaxReplies","opponentGuardForkNodeBudget","opponentGuardForkTimeBudgetMs","opponentGuardVCTOnUnknownEnabled","opponentGuardRecoveryReservedNodes","opponentGuardRecoveryReservedTimeMs","blackDoubleThreeDefenseEnabled","blackDoubleThreeMaxGains","blackDoubleThreeMaxCandidates","blackDoubleThreeTimeBudgetMs","blackDoubleThreeDefenseWeight"])

def main():
    before = BASELINE.read_text().splitlines()
    retired = [line for line in before if line.partition("=")[0] in RETIRED_ZERO_FIELDS]
    assert retired and all(line.endswith("=0") for line in retired), "Retired field was active"
    expected = [line for line in before if line.partition("=")[0] not in RETIRED_ZERO_FIELDS]
    with tempfile.TemporaryDirectory(prefix="five-star-production-") as directory:
        binary = str(Path(directory) / "snapshot")
        subprocess.run(["clang", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        str(ROOT / "ice five chess/FiveChessAI.c"),
                        str(ROOT / "tools/five_star_production_snapshot.c"), "-lm", "-o", binary], check=True)
        actual = subprocess.check_output([binary], text=True).splitlines()
    assert actual == expected, "Production profile or tactical snapshot changed"
    print("Production equivalence passed: all live fields of three profiles + 12 tactical fixtures")
    print(f"Removed {len(RETIRED_ZERO_FIELDS)} experimental fields; all were zero in every supported profile")

if __name__ == "__main__":
    main()
