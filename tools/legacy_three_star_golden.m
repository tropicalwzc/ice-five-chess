#import <Foundation/Foundation.h>

#import "../ice five chess/doublethree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    int moves[16][3];
    int moveCount;
    int side;
    unsigned int seed;
    int expectedX;
    int expectedY;
} LegacyGoldenCase;

static LegacyGoldenCase cases[] = {
    {
        "opening-cross",
        {{7,7,1},{7,8,-1},{8,7,1},{6,7,-1}},
        4, 1, 1001, 8, 8
    },
    {
        "must-block-horizontal",
        {{7,7,1},{5,4,-1},{6,7,1},{5,5,-1},{8,8,1},{5,6,-1},{9,9,1},{5,7,-1}},
        8, 1, 1002, 5, 3
    },
    {
        "white-midgame",
        {{7,7,1},{7,8,-1},{8,8,1},{6,6,-1},{8,7,1},{9,7,-1},{6,8,1}},
        7, -1, 1003, 8, 6
    }
};

static void load_case(doublethree *engine, const LegacyGoldenCase *testCase)
{
    for (int i = 0; i < testCase->moveCount; i++) {
        [engine add_a_chess:testCase->moves[i][0]
                       pl_y:testCase->moves[i][1]
                       mode:testCase->moves[i][2]];
    }
}

static void run_case(const LegacyGoldenCase *testCase, int result[2])
{
    doublethree *engine = [[doublethree alloc] init];
    load_case(engine, testCase);
    srand(testCase->seed);
    [engine harsh_analysisboard:testCase->side];
    [engine get_last_pos_return_color:result];
}

static void run_isolated_case(const LegacyGoldenCase *testCase, int result[2])
{
    doublethree *engine = [[doublethree alloc] init];
    load_case(engine, testCase);
    [engine set_legacy_random_seed:testCase->seed];
    [engine harsh_analysisboard:testCase->side];
    [engine get_last_pos_return_color:result];
}

int main(int argc, const char *argv[])
{
    @autoreleasepool {
        bool printOnly = argc == 2 && strcmp(argv[1], "--print") == 0;
        int caseCount = (int)(sizeof(cases) / sizeof(cases[0]));
        for (int i = 0; i < caseCount; i++) {
            int first[2] = {-1, -1};
            int second[2] = {-1, -1};
            int isolated[2] = {-1, -1};
            run_case(&cases[i], first);
            run_case(&cases[i], second);
            run_isolated_case(&cases[i], isolated);
            assert(first[0] == second[0] && first[1] == second[1]);
            assert(first[0] == isolated[0] && first[1] == isolated[1]);
            if (printOnly) {
                printf("%s=%d,%d\n", cases[i].name, first[0], first[1]);
            } else {
                assert(first[0] == cases[i].expectedX);
                assert(first[1] == cases[i].expectedY);
            }
        }
        if (!printOnly) puts("legacy-three-star golden tests passed");
    }
    return 0;
}
