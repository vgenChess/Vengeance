#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "types.h"
 
 class Zobrist {
     
 private:
     
     // http://compgroups.net/comp.lang.fortran/64-bit-kiss-rngs/601519
     U64 x=1234567890987654321ULL,c=123456123456123456ULL,
     y=362436362436362436ULL,z=1066149217761810ULL,t;
     
     #define MWC (t=(x<<58)+c, c=(x>>6), x+=t, c+=(x<t), x)
     #define XSH ( y^=(y<<13), y^=(y>>17), y^=(y<<43) )
     #define CNG ( z=6906969069LL*z+1234567 )
     #define KISS (MWC+XSH+CNG)
     
 public:
     
     static Zobrist objZobrist;
     
     U64 zobristKey[U8_MAX_PIECES][U8_MAX_SIDES][U8_MAX_SQUARES];
     U64 pawnZobristKey[U8_MAX_SQUARES];
     
     U64 KEY_EP_A_FILE;
     U64 KEY_EP_B_FILE;
     U64 KEY_EP_C_FILE;
     U64 KEY_EP_D_FILE;
     U64 KEY_EP_E_FILE;
     U64 KEY_EP_F_FILE;
     U64 KEY_EP_G_FILE;
     U64 KEY_EP_H_FILE;
     U64 KEY_SIDE_TO_MOVE;
     U64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
     U64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
     U64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
     U64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;
     
     void initZobristKeys() {
         
         for (int i = 0; i < U8_MAX_PIECES; i++) {
             for (int j = 0; j < U8_MAX_SIDES; j++) {
                 for (int k = 0; k < U8_MAX_SQUARES; k++) {
                     
                     zobristKey[i][j][k] = KISS;
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
         
         for(int i = 0; i < 64; i++)
             pawnZobristKey[i] = KISS;
     }
 };
 
#endif
