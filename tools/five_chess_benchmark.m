// Current app-path smoke benchmark. Retired variants require their Git revision.
#import <Foundation/Foundation.h>
#import "../ice five chess/doublethree.h"

static void emit(FILE *output, NSDictionary *row) {
    NSData *data = [NSJSONSerialization dataWithJSONObject:row options:NSJSONWritingSortedKeys error:nil];
    fwrite(data.bytes, 1, data.length, output);
    fputc('\n', output);
}

int main(int argc, const char **argv) {
    @autoreleasepool {
        NSString *outputPath = nil, *profileName = @"five-star";
        int games = 1, maxPlies = 20;
        BOOL forbidden = NO;
        for (int i = 1; i < argc; i++) {
            NSString *arg = @(argv[i]);
            if ([arg isEqualToString:@"--forbidden"]) { forbidden = YES; continue; }
            if ([arg isEqualToString:@"--help"]) {
                puts("usage: five_chess_benchmark --output PATH [--profile five-star|five-star-5.8.1|four-star] [--games N] [--max-plies N] [--forbidden]");
                return 0;
            }
            if (i + 1 >= argc) { fprintf(stderr, "Missing value for %s\n", argv[i]); return 2; }
            NSString *value = @(argv[++i]);
            if ([arg isEqualToString:@"--output"]) outputPath = value;
            else if ([arg isEqualToString:@"--profile"]) profileName = value;
            else if ([arg isEqualToString:@"--games"] || [arg isEqualToString:@"--max-plies"]) {
                NSScanner *scanner = [NSScanner scannerWithString:value];
                int number = 0;
                if (![scanner scanInt:&number] || !scanner.isAtEnd || number < 1 ||
                    number > ([arg isEqualToString:@"--games"] ? 100 : 225)) {
                    fprintf(stderr, "Invalid count: %s\n", value.UTF8String); return 2;
                }
                if ([arg isEqualToString:@"--games"]) games = number; else maxPlies = number;
            } else { fprintf(stderr, "Unsupported option: %s\n", arg.UTF8String); return 2; }
        }
        BOOL fiveStar = [profileName isEqualToString:@"five-star"] ||
                        [profileName isEqualToString:@"five-star-5.8.1"];
        if (!fiveStar && ![profileName isEqualToString:@"four-star"]) {
            fprintf(stderr, "Unsupported/retired profile: %s; five stars only supports 5.8.1.\n", profileName.UTF8String);
            return 2;
        }
        if (!outputPath) { fputs("--output PATH is required\n", stderr); return 2; }
        FILE *output = fopen(outputPath.fileSystemRepresentation, "wx");
        if (!output) { perror("Cannot create output (must not already exist)"); return 2; }
        FCAIProfile profile = fiveStar ? fc_profile_five_star_early_micro_vcf_candidate() : fc_profile_proof_guided(false);
        emit(output, @{@"type": @"metadata", @"schema": @1, @"profile": @(profile.name),
                       @"version": @(profile.version), @"mode": @"app-user-game",
                       @"opening": @"center-black",
                       @"opponent": @"four-star", @"forbiddenBlack": @(forbidden)});
        int violations = 0;
        for (int g = 0; g < games; g++) {
            doublethree *game = [[doublethree alloc] init];
            [game set_banmode:forbidden ? 1 : 0];
            // Match the app's computer-first opening (or a human center move).
            [game add_a_chess:7 pl_y:7 mode:1];
            emit(output, @{@"type": @"move", @"game": @(g), @"ply": @0,
                           @"side": @1, @"x": @7, @"y": @7, @"elapsedMs": @0});
            int candidateColor = g % 2 == 0 ? 1 : -1, plies = 1;
            for (; plies < maxPlies && game.win_state == 0; plies++) {
                int color = plies % 2 == 0 ? 1 : -1;
                CFAbsoluteTime started = CFAbsoluteTimeGetCurrent();
                if (fiveStar && color == candidateColor) [game five_star_analysisboard:color];
                else [game four_star_analysisboard:color];
                double elapsed = (CFAbsoluteTimeGetCurrent() - started) * 1000.0;
                int position[2] = {-1, -1};
                int placedColor = [game get_last_pos_return_color:position];
                if (placedColor != color) { fclose(output); return 1; }
                if (fiveStar && color == candidateColor && elapsed > 5000.0) violations++;
                emit(output, @{@"type": @"move", @"game": @(g), @"ply": @(plies),
                               @"side": @(color), @"x": @(position[0]), @"y": @(position[1]),
                               @"elapsedMs": @(elapsed)});
            }
            emit(output, @{@"type": @"game", @"game": @(g), @"plies": @(plies),
                           @"candidateColor": @(candidateColor), @"winner": @(game.win_state),
                           @"truncated": @(game.win_state == 0 && plies < 225)});
        }
        emit(output, @{@"type": @"summary", @"fiveStarDeadlineViolations": @(violations)});
        fclose(output);
        return violations ? 1 : 0;
    }
}
