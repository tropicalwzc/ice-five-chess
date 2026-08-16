#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../ice five chess/FiveChessAI.h"

#ifndef FC_FORMAL_OPENINGS_INCLUDE
#define FC_FORMAL_OPENINGS_INCLUDE "FiveChessFormalOpenings.inc"
#endif
#include FC_FORMAL_OPENINGS_INCLUDE

static bool validate_rule(bool forbidden_black)
{
    for (int opening_id = 0; opening_id < fcFormalOpeningCount; opening_id++) {
        const FCBenchmarkOpening *opening = &fcFormalOpenings[opening_id];
        int board[FC_BOARD_SIZE][FC_BOARD_SIZE];
        memset(board, 0, sizeof(board));
        if (opening->length < 4 || opening->length > 8 ||
            opening->length != opening->prefixLength) {
            fprintf(stderr, "opening %d has invalid short-prefix metadata\n",
                    opening_id);
            return false;
        }
        for (int ply = 0; ply < opening->length; ply++) {
            int x = 7 + opening->coordinates[ply * 2];
            int y = 7 + opening->coordinates[ply * 2 + 1];
            int side = (ply & 1) == 0 ? 1 : -1;
            if (!fc_make_move(board, x, y, side, forbidden_black)) {
                fprintf(stderr,
                        "opening %d ply %d is illegal (forbidden=%d)\n",
                        opening_id, ply, forbidden_black ? 1 : 0);
                return false;
            }
            if (fc_has_five((const int (*)[FC_BOARD_SIZE])board,
                            x, y, side)) {
                fprintf(stderr,
                        "opening %d ply %d is already terminal (forbidden=%d)\n",
                        opening_id, ply, forbidden_black ? 1 : 0);
                return false;
            }
        }
    }
    return true;
}

int main(void)
{
    if (fcFormalOpeningCount != 50) {
        fprintf(stderr, "expected exactly 50 identities, got %d\n",
                fcFormalOpeningCount);
        return 1;
    }
    if (!validate_rule(false) || !validate_rule(true)) return 1;
    printf("validated %d short opening identities under both rule modes\n",
           fcFormalOpeningCount);
    return 0;
}
