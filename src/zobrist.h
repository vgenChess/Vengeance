#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"

namespace vgen_zobrist {
    
// http://compgroups.net/comp.lang.fortran/64-bit-kiss-rngs/601519
U64 x=1234567890987654321ULL,c=123456123456123456ULL,
y=362436362436362436ULL,z=1066149217761810ULL,t;

#define MWC (t=(x<<58)+c, c=(x>>6), x+=t, c+=(x<t), x)
#define XSH ( y^=(y<<13), y^=(y>>17), y^=(y<<43) )
#define CNG ( z=6906969069LL*z+1234567 )
#define KISS (MWC+XSH+CNG)

void initZobristKeys() {

    // DUMMY 0
    // PAWNS 1
    // KNIGHTS 2
    // BISHOPS 3
    // ROOKS 4
    // QUEEN 5
    // KING 6
    // PIECES 7
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 64; k++) {

                zobrist[i][j][k] = KISS;
            }            
        }
    }
    
    KEY_SIDE_TO_MOVE = KISS;

	KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE = KISS;
	KEY_FLAG_WHITE_CASTLE_KING_SIDE = KISS;
	KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE = KISS;
	KEY_FLAG_BLACK_CASTLE_KING_SIDE = KISS; 
	
	KEY_EP_A_FILE = KISS;
	KEY_EP_B_FILE = KISS;
	KEY_EP_C_FILE = KISS;
	KEY_EP_D_FILE = KISS;
	KEY_EP_E_FILE = KISS;
	KEY_EP_F_FILE = KISS;
	KEY_EP_G_FILE = KISS;
	KEY_EP_H_FILE = KISS;
}


void initPawnZobristKeys() {

    for(int i = 0; i < 64; i++) {

        pawnZobristKey[i] = KISS;
    }
}

}

#endif
