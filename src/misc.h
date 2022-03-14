#ifndef MISC_H
#define MISC_H

namespace vgen_misc {

    void init_inbetween_bb() {
        for (int i = 0; i < 64; i++) {
            for(int j = 0; j < 64; j++) {

                arrInBetween[i][j] = inBetweenOnTheFly(i, j);
            }
        }
    }

    void initCastleMaskAndFlags() {

        for (int i = 0; i < 64; i++) {
            rookCastleFlagMask[i] = 15;
        }

        rookCastleFlagMask[0] ^= CASTLE_FLAG_WHITE_QUEEN;
        rookCastleFlagMask[7] ^= CASTLE_FLAG_WHITE_KING;
        rookCastleFlagMask[56] ^= CASTLE_FLAG_BLACK_QUEEN;
        rookCastleFlagMask[63] ^= CASTLE_FLAG_BLACK_KING;
    }
}


#endif
