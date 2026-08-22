#include "../ice five chess/FiveChessAI.h"

#include <stdio.h>

typedef FCAIProfile (*FCProfileFactory)(void);

static FCAIProfile fc_snapshot_three_star(void)
{
    return fc_profile_production();
}

static FCAIProfile fc_snapshot_four_star(void)
{
    return fc_profile_frozen_four_star_control();
}

static FCAIProfile fc_snapshot_five_star_control(void)
{
    return fc_profile_five_star_proof_engine_candidate();
}

static FCAIProfile fc_snapshot_five_star_guard(void)
{
    return fc_profile_five_star_opponent_guard_candidate();
}

static FCAIProfile fc_snapshot_five_star_early_vcf(void)
{
    return fc_profile_five_star_early_micro_vcf_candidate();
}

static FCAIProfile fc_snapshot_five_star_black_double_three(void)
{
    return fc_profile_five_star_black_double_three_candidate();
}

static FCAIProfile fc_snapshot_five_star_black_defense_recovery(void)
{
    return fc_profile_five_star_black_defense_recovery_candidate();
}

int main(void)
{
    static const struct {
        const char *binding;
        FCProfileFactory factory;
    } profiles[] = {
        {"three-star-control", fc_snapshot_three_star},
        {"frozen-four-star-control", fc_snapshot_four_star},
        {"ui-bound-five-star-5.4.1-control", fc_snapshot_five_star_control},
        {"five-star-opponent-guard-candidate", fc_snapshot_five_star_guard},
        {"five-star-early-micro-vcf-candidate",
         fc_snapshot_five_star_early_vcf},
        {"five-star-black-double-three-5.8.2-candidate",
         fc_snapshot_five_star_black_double_three},
        {"five-star-black-defense-recovery-5.8.2-candidate",
         fc_snapshot_five_star_black_defense_recovery},
    };
    char snapshot[8192];
    puts("{");
    for (size_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++) {
        FCAIProfile profile = profiles[i].factory();
        (void)fc_profile_snapshot(&profile, snapshot, sizeof(snapshot));
        printf("  \"%s\": %s%s\n", profiles[i].binding, snapshot,
               i + 1 == sizeof(profiles) / sizeof(profiles[0]) ? "" : ",");
    }
    puts("}");
    return 0;
}
