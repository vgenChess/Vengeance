//
//  utility.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <cstring>
#include <assert.h>

#include "nonslidingmoves.h"
#include "utility.h"
#include "magicmoves.h"
#include "make_unmake.h"
#include "movegen.h"
#include "perft.h"
#include "functions.h"

u64 arrInBetween[64][64];

void print_bb(u64 board) {
    
    printf(" \n ---------- ");
    int row;
    int col;
    for (row = 7; row >= 0; row--) {
        printf("\n|%d ", row + 1);
        for (col = 0; col < 8; col++) {
            if ((((u64) 1) << ((row * 8 + col)) & board)
                == (u64) 1 << (row * 8 + col)) {
                putchar('x');
            } else
                putchar('.');
        }
        printf("|");
    }
    printf("\n|  abcdefgh|\n");
    printf(" ---------- \n");
}

void print_board(u64 board, Thread *th) {

    printf("\n -------------------- \n");
    printf("|                    |");
 
    int row;
    int col;
    for (row = 7; row >= 0; row--) {
        printf("\n|%d  ", row + 1);
        for (col = 0; col < 8; col++) {
            if ((((u64) 1) << ((row * 8 + col)) & board)
                == (u64) 1 << (row * 8 + col)) {
                
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[KING])
                    putchar('K');
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[QUEEN])
                    putchar('Q');
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[BISHOPS])
                    putchar('B');
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[KNIGHTS])
                    putchar('N');
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[ROOKS])
                    putchar('R');
                if ((u64) 1 << (row * 8 + col) & th->whitePieceBB[PAWNS])
                    putchar('P');
                
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[KING])
                    putchar('k');
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[QUEEN])
                    putchar('q');
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[BISHOPS])
                    putchar('b');
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[KNIGHTS])
                    putchar('n');
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[ROOKS])
                    putchar('r');
                if ((u64) 1 << (row * 8 + col) & th->blackPieceBB[PAWNS])
                    putchar('p');
            } else {

                if (row % 2 == 0) {

                    if (col % 2 == 0)
                        putchar('.');
                    else 
                        putchar('.');   
                } else {

                    if (col % 2 == 0)
                        putchar('.');
                    else 
                        putchar('.');
                }
            }
            
            putchar(' ');
        }
        printf(" |");
    }

    printf("\n|                    |\n");
    printf("|   a b c d e f g h  |\n");
    printf(" -------------------- \n");
}


//    bitScanForward
//     @author Kim Walisch (2012)
//      @param bb bitboard to scan
//       @precondition bb != 0
//        @return index (0..63) of least significant one bit


const int index64[64] = { 0, 47, 1, 56, 48, 27, 2, 60, 57, 49, 41, 37, 28, 16,
    3, 61, 54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4, 62,
    46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45, 25, 39,
    14, 33, 19, 30, 9, 24, 13, 18, 8, 12, 7, 6, 5, 63
};

int bitScanForward(u64 board) {
    const u64 debruijn64 = 285870213051386505U;
    
    return index64[((board ^ (board - 1)) * debruijn64) >> 58];
}

// for population count
const u64 k1 = C64(0x5555555555555555); /*  -1/3   */
const u64 k2 = C64(0x3333333333333333); /*  -1/5   */
const u64 k4 = C64(0x0f0f0f0f0f0f0f0f); /*  -1/17  */
const u64 kf = C64(0x0101010101010101); /*  -1/255 */

// population count
int popCount (u64 x) {
    x =  x       - ((x >> 1)  & k1); /* put count of each 2 bits into those 2 bits */
    x = (x & k2) + ((x >> 2)  & k2); /* put count of each 4 bits into those 4 bits */
    x = (x       +  (x >> 4)) & k4 ; /* put count of each 8 bits into those 8 bits */
    x = (x * kf) >> 56; /* returns 8 most significant bits of x + (x<<8) + (x<<16) + (x<<24) + ...  */
    return (int) x;
}

/**
 * Flip a bitboard vertically about the centre ranks.
 * Rank 1 is mapped to rank 8 and vice versa.
 * @param x any bitboard
 * @return bitboard x flipped vertically
 */
u64 flipVertical(u64 x) {
    return  ( (x << 56)                           ) |
            ( (x << 40) & C64(0x00ff000000000000) ) |
            ( (x << 24) & C64(0x0000ff0000000000) ) |
            ( (x <<  8) & C64(0x000000ff00000000) ) |
            ( (x >>  8) & C64(0x00000000ff000000) ) |
            ( (x >> 24) & C64(0x0000000000ff0000) ) |
            ( (x >> 40) & C64(0x000000000000ff00) ) |
            ( (x >> 56) );
}

void clearAllBitBoards(Thread *th) {
    
    th->whitePieceBB[KING] &= 0;
    th->whitePieceBB[QUEEN] &= 0;
    th->whitePieceBB[BISHOPS] &= 0;
    th->whitePieceBB[KNIGHTS] &= 0;
    th->whitePieceBB[ROOKS] &= 0;
    th->whitePieceBB[PAWNS] &= 0;
    th->whitePieceBB[PIECES] &= 0;
    
    th->blackPieceBB[KING] &= 0;
    th->blackPieceBB[QUEEN] &= 0;
    th->blackPieceBB[BISHOPS] &= 0;
    th->blackPieceBB[KNIGHTS] &= 0;
    th->blackPieceBB[ROOKS] &= 0;
    th->blackPieceBB[PAWNS] &= 0;
    th->blackPieceBB[PIECES] &= 0;
}

u64 getAttacks(const u8 stm, Thread *th) {

    u64 attacks = 0ULL, b;

    attacks |= stm ?
        bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]): 
        wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);

    b = stm ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];
    while(b) {
        
        attacks |= get_knight_attacks(GET_POSITION(b));
        POP_POSITION(b);
    }

    b = stm ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
    while(b) {

        attacks |= Bmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }

    b = stm ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];
    while(b) {
        
        attacks |= Rmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }
    
    b = stm ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];
    while(b) {
        
        attacks |= Qmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }  

    int kingSq = GET_POSITION(stm ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);

    attacks |= get_king_attacks(kingSq);

    return attacks;
} 

bool isKingInCheck(u8 side, Thread *th) {
    
    const u8 opponent = side ^ 1;      
    const int kingSq = GET_POSITION(side ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);
    
    // Staggered check to return early saving time

    u64 oppAttacks = 0ULL, b;

    oppAttacks = opponent ?
        bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]): 
        wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);

    if (oppAttacks & (1ULL << kingSq)) return true;        
    

    oppAttacks = 0ULL;        
    
    b = opponent ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];
    while(b) {
        
        oppAttacks |= get_knight_attacks(GET_POSITION(b));
        POP_POSITION(b);
    }

    if (oppAttacks & (1ULL << kingSq)) return true;        
    

    oppAttacks = 0ULL;        
    
    b = opponent ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
    while(b) {

        oppAttacks |= Bmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }
    
    if (oppAttacks & (1ULL << kingSq)) return true;        
    

    oppAttacks = 0ULL;        
    
    b = opponent ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];
    while(b) {
        
        oppAttacks |= Rmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }
    
    if (oppAttacks & (1ULL << kingSq)) return true;        
    

    oppAttacks = 0ULL;        
    
    b = opponent ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];
    while(b) {
        
        oppAttacks |= Qmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }  

    if (oppAttacks & (1ULL << kingSq)) return true;        
    

    oppAttacks = 0ULL;        

    int oppKingSq = GET_POSITION(opponent ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);

    oppAttacks = get_king_attacks(oppKingSq);

    if (oppAttacks & (1ULL << kingSq)) return true;


    return false;
}

bool isRepetition(const int ply, Thread *th) {

    bool flag = false;
    for (int i = th->moves_history_counter + ply; i >= 0; i--) {

        if (th->movesHistory[i].hashKey == th->hashKey) {

            flag = true;
            break;
        }
    }

    return flag;
}

//TODO check logic, under observation
bool isPositionDraw(Thread *th) {

    if (POPCOUNT(th->occupied) > 4) return false;

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]) == th->occupied)    return true; //kk

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[KNIGHTS]) == th->occupied) return true; //KNNk or KNk

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->blackPieceBB[KNIGHTS]) == th->occupied) return true; //Knnk or Knk

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[KNIGHTS]
        |   th->blackPieceBB[KNIGHTS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[KNIGHTS]) == 1
        &&  POPCOUNT(th->blackPieceBB[KNIGHTS]) == 1)   return true; //KNkn

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[BISHOPS]) == 1)   return true; //KBk

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1)   return true; //Kbk

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[BISHOPS]
        |   th->blackPieceBB[KNIGHTS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[BISHOPS]) == 1
        &&  POPCOUNT(th->blackPieceBB[KNIGHTS]) == 1)   return true; //KBkn

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[KNIGHTS]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[KNIGHTS]) == 1
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1)   return true; //KNkb

    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[BISHOPS]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[BISHOPS]) == 1
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1)   return true; //KBkb

    return false;
}

/* function to check if a kingSq is attacked */

bool isSqAttacked(u8 sq, u8 side, Thread *th) {
    
    u64 attacks;
    
    /* check if a king is attacking a sq */
    
    attacks = get_king_attacks(sq);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[KING] : th->whitePieceBB[KING])) {

        return true;
    }
    
    /* check if a queen is attacking a sq */
    
    attacks = Qmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

        return true;
    }
    
    /* check if a bishop is attacking a kingSq */
    
    attacks = Bmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS])) {

        return true;
    }
    
    /* check if a knight is attacking a kingSq */
    
    attacks = get_knight_attacks(sq);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS])) {

        return true;
    }
    
    /* check if a rook is attacking a kingSq */
    
    attacks = Rmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS])) {

        return true;
    }
    
    /* check if a pawn is attacking a kingSq */
    
    if (side == WHITE) {
        
        attacks = ((index_bb[sq] << 7) & NOT_H_FILE)
            | ((index_bb[sq] << 9) & NOT_A_FILE);
    } else {

        attacks = ((index_bb[sq] >> 7) & NOT_A_FILE)
            | ((index_bb[sq] >> 9) & NOT_H_FILE);
    }
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS])) {

        return true;
    }
    
    return false;
}

u64 getBitboardFromSquare(int sq) {
    
	return 1ULL << sq;
}

char* algebricPos(u8 sq) {
    switch (sq) {
        case 0:
            return "a1";
        case 1:
            return "b1";
        case 2:
            return "c1";
        case 3:
            return "d1";
        case 4:
            return "e1";
        case 5:
            return "f1";
        case 6:
            return "g1";
        case 7:
            return "h1";
        case 8:
            return "a2";
        case 9:
            return "b2";
        case 10:
            return "c2";
        case 11:
            return "d2";
        case 12:
            return "e2";
        case 13:
            return "f2";
        case 14:
            return "g2";
        case 15:
            return "h2";
        case 16:
            return "a3";
        case 17:
            return "b3";
        case 18:
            return "c3";
        case 19:
            return "d3";
        case 20:
            return "e3";
        case 21:
            return "f3";
        case 22:
            return "g3";
        case 23:
            return "h3";
        case 24:
            return "a4";
        case 25:
            return "b4";
        case 26:
            return "c4";
        case 27:
            return "d4";
        case 28:
            return "e4";
        case 29:
            return "f4";
        case 30:
            return "g4";
        case 31:
            return "h4";
        case 32:
            return "a5";
        case 33:
            return "b5";
        case 34:
            return "c5";
        case 35:
            return "d5";
        case 36:
            return "e5";
        case 37:
            return "f5";
        case 38:
            return "g5";
        case 39:
            return "h5";
        case 40:
            return "a6";
        case 41:
            return "b6";
        case 42:
            return "c6";
        case 43:
            return "d6";
        case 44:
            return "e6";
        case 45:
            return "f6";
        case 46:
            return "g6";
        case 47:
            return "h6";
        case 48:
            return "a7";
        case 49:
            return "b7";
        case 50:
            return "c7";
        case 51:
            return "d7";
        case 52:
            return "e7";
        case 53:
            return "f7";
        case 54:
            return "g7";
        case 55:
            return "h7";
        case 56:
            return "a8";
        case 57:
            return "b8";
        case 58:
            return "c8";
        case 59:
            return "d8";
        case 60:
            return "e8";
        case 61:
            return "f8";
        case 62:
            return "g8";
        case 63:
            return "h8";
            
        default:
            break;
    }
    
    return "";
}

u8 squareFromAlgebricPos(char* posName) {
    
    if (strcmp(posName, "a1") == 0) {
        return 0;
    } else if (strcmp(posName, "b1") == 0) {
        return 1;
    } else if (strcmp(posName, "c1") == 0) {
        return 2;
    } else if (strcmp(posName, "d1") == 0) {
        return 3;
    } else if (strcmp(posName, "e1") == 0) {
        return 4;
    } else if (strcmp(posName, "f1") == 0) {
        return 5;
    } else if (strcmp(posName, "g1") == 0) {
        return 6;
    } else if (strcmp(posName, "h1") == 0) {
        return 7;
    }
    
    if (strcmp(posName, "a2") == 0) {
        return 8;
    } else if (strcmp(posName, "b2") == 0) {
        return 9;
    } else if (strcmp(posName, "c2") == 0) {
        return 10;
    } else if (strcmp(posName, "d2") == 0) {
        return 11;
    } else if (strcmp(posName, "e2") == 0) {
        return 12;
    } else if (strcmp(posName, "f2") == 0) {
        return 13;
    } else if (strcmp(posName, "g2") == 0) {
        return 14;
    } else if (strcmp(posName, "h2") == 0) {
        return 15;
    }
    
    if (strcmp(posName, "a3") == 0) {
        return 16;
    } else if (strcmp(posName, "b3") == 0) {
        return 17;
    } else if (strcmp(posName, "c3") == 0) {
        return 18;
    } else if (strcmp(posName, "d3") == 0) {
        return 19;
    } else if (strcmp(posName, "e3") == 0) {
        return 20;
    } else if (strcmp(posName, "f3") == 0) {
        return 21;
    } else if (strcmp(posName, "g3") == 0) {
        return 22;
    } else if (strcmp(posName, "h3") == 0) {
        return 23;
    }
    
    if (strcmp(posName, "a4") == 0) {
        return 24;
    } else if (strcmp(posName, "b4") == 0) {
        return 25;
    } else if (strcmp(posName, "c4") == 0) {
        return 26;
    } else if (strcmp(posName, "d4") == 0) {
        return 27;
    } else if (strcmp(posName, "e4") == 0) {
        return 28;
    } else if (strcmp(posName, "f4") == 0) {
        return 29;
    } else if (strcmp(posName, "g4") == 0) {
        return 30;
    } else if (strcmp(posName, "h4") == 0) {
        return 31;
    }
    
    if (strcmp(posName, "a5") == 0) {
        return 32;
    } else if (strcmp(posName, "b5") == 0) {
        return 33;
    } else if (strcmp(posName, "c5") == 0) {
        return 34;
    } else if (strcmp(posName, "d5") == 0) {
        return 35;
    } else if (strcmp(posName, "e5") == 0) {
        return 36;
    } else if (strcmp(posName, "f5") == 0) {
        return 37;
    } else if (strcmp(posName, "g5") == 0) {
        return 38;
    } else if (strcmp(posName, "h5") == 0) {
        return 39;
    }
    
    if (strcmp(posName, "a6") == 0) {
        return 40;
    } else if (strcmp(posName, "b6") == 0) {
        return 41;
    } else if (strcmp(posName, "c6") == 0) {
        return 42;
    } else if (strcmp(posName, "d6") == 0) {
        return 43;
    } else if (strcmp(posName, "e6") == 0) {
        return 44;
    } else if (strcmp(posName, "f6") == 0) {
        return 45;
    } else if (strcmp(posName, "g6") == 0) {
        return 46;
    } else if (strcmp(posName, "h6") == 0) {
        return 47;
    }
    
    if (strcmp(posName, "a7") == 0) {
        return 48;
    } else if (strcmp(posName, "b7") == 0) {
        return 49;
    } else if (strcmp(posName, "c7") == 0) {
        return 50;
    } else if (strcmp(posName, "d7") == 0) {
        return 51;
    } else if (strcmp(posName, "e7") == 0) {
        return 52;
    } else if (strcmp(posName, "f7") == 0) {
        return 53;
    } else if (strcmp(posName, "g7") == 0) {
        return 54;
    } else if (strcmp(posName, "h7") == 0) {
        return 55;
    }
    
    if (strcmp(posName, "a8") == 0) {
        return 56;
    } else if (strcmp(posName, "b8") == 0) {
        return 57;
    } else if (strcmp(posName, "c8") == 0) {
        return 58;
    } else if (strcmp(posName, "d8") == 0) {
        return 59;
    } else if (strcmp(posName, "e8") == 0) {
        return 60;
    } else if (strcmp(posName, "f8") == 0) {
        return 61;
    } else if (strcmp(posName, "g8") == 0) {
        return 62;
    } else if (strcmp(posName, "h8") == 0) {
        return 63;
    }
    
    return 0;
}

u64 bbFromAlgebricPos(char* posName) {
    
    if (strcmp(posName, "a1") == 0) {
        return getBitboardFromSquare(0);
    } else if (strcmp(posName, "b1") == 0) {
        return getBitboardFromSquare(1);
    } else if (strcmp(posName, "c1") == 0) {
        return getBitboardFromSquare(2);
    } else if (strcmp(posName, "d1") == 0) {
        return getBitboardFromSquare(3);
    } else if (strcmp(posName, "e1") == 0) {
        return getBitboardFromSquare(4);
    } else if (strcmp(posName, "f1") == 0) {
        return getBitboardFromSquare(5);
    } else if (strcmp(posName, "g1") == 0) {
        return getBitboardFromSquare(6);
    } else if (strcmp(posName, "h1") == 0) {
        return getBitboardFromSquare(7);
    }
    
    if (strcmp(posName, "a2") == 0) {
        return getBitboardFromSquare(8);
    } else if (strcmp(posName, "b2") == 0) {
        return getBitboardFromSquare(9);
    } else if (strcmp(posName, "c2") == 0) {
        return getBitboardFromSquare(10);
    } else if (strcmp(posName, "d2") == 0) {
        return getBitboardFromSquare(11);
    } else if (strcmp(posName, "e2") == 0) {
        return getBitboardFromSquare(12);
    } else if (strcmp(posName, "f2") == 0) {
        return getBitboardFromSquare(13);
    } else if (strcmp(posName, "g2") == 0) {
        return getBitboardFromSquare(14);
    } else if (strcmp(posName, "h2") == 0) {
        return getBitboardFromSquare(15);
    }
    
    if (strcmp(posName, "a3") == 0) {
        return getBitboardFromSquare(16);
    } else if (strcmp(posName, "b3") == 0) {
        return getBitboardFromSquare(17);
    } else if (strcmp(posName, "c3") == 0) {
        return getBitboardFromSquare(18);
    } else if (strcmp(posName, "d3") == 0) {
        return getBitboardFromSquare(19);
    } else if (strcmp(posName, "e3") == 0) {
        return getBitboardFromSquare(20);
    } else if (strcmp(posName, "f3") == 0) {
        return getBitboardFromSquare(21);
    } else if (strcmp(posName, "g3") == 0) {
        return getBitboardFromSquare(22);
    } else if (strcmp(posName, "h3") == 0) {
        return getBitboardFromSquare(23);
    }
    
    if (strcmp(posName, "a4") == 0) {
        return getBitboardFromSquare(24);
    } else if (strcmp(posName, "b4") == 0) {
        return getBitboardFromSquare(25);
    } else if (strcmp(posName, "c4") == 0) {
        return getBitboardFromSquare(26);
    } else if (strcmp(posName, "d4") == 0) {
        return getBitboardFromSquare(27);
    } else if (strcmp(posName, "e4") == 0) {
        return getBitboardFromSquare(28);
    } else if (strcmp(posName, "f4") == 0) {
        return getBitboardFromSquare(29);
    } else if (strcmp(posName, "g4") == 0) {
        return getBitboardFromSquare(30);
    } else if (strcmp(posName, "h4") == 0) {
        return getBitboardFromSquare(31);
    }
    
    if (strcmp(posName, "a5") == 0) {
        return getBitboardFromSquare(32);
    } else if (strcmp(posName, "b5") == 0) {
        return getBitboardFromSquare(33);
    } else if (strcmp(posName, "c5") == 0) {
        return getBitboardFromSquare(34);
    } else if (strcmp(posName, "d5") == 0) {
        return getBitboardFromSquare(35);
    } else if (strcmp(posName, "e5") == 0) {
        return getBitboardFromSquare(36);
    } else if (strcmp(posName, "f5") == 0) {
        return getBitboardFromSquare(37);
    } else if (strcmp(posName, "g5") == 0) {
        return getBitboardFromSquare(38);
    } else if (strcmp(posName, "h5") == 0) {
        return getBitboardFromSquare(39);
    }
    
    if (strcmp(posName, "a6") == 0) {
        return getBitboardFromSquare(40);
    } else if (strcmp(posName, "b6") == 0) {
        return getBitboardFromSquare(41);
    } else if (strcmp(posName, "c6") == 0) {
        return getBitboardFromSquare(42);
    } else if (strcmp(posName, "d6") == 0) {
        return getBitboardFromSquare(43);
    } else if (strcmp(posName, "e6") == 0) {
        return getBitboardFromSquare(44);
    } else if (strcmp(posName, "f6") == 0) {
        return getBitboardFromSquare(45);
    } else if (strcmp(posName, "g6") == 0) {
        return getBitboardFromSquare(46);
    } else if (strcmp(posName, "h6") == 0) {
        return getBitboardFromSquare(47);
    }
    
    if (strcmp(posName, "a7") == 0) {
        return getBitboardFromSquare(48);
    } else if (strcmp(posName, "b7") == 0) {
        return getBitboardFromSquare(49);
    } else if (strcmp(posName, "c7") == 0) {
        return getBitboardFromSquare(50);
    } else if (strcmp(posName, "d7") == 0) {
        return getBitboardFromSquare(51);
    } else if (strcmp(posName, "e7") == 0) {
        return getBitboardFromSquare(52);
    } else if (strcmp(posName, "f7") == 0) {
        return getBitboardFromSquare(53);
    } else if (strcmp(posName, "g7") == 0) {
        return getBitboardFromSquare(54);
    } else if (strcmp(posName, "h7") == 0) {
        return getBitboardFromSquare(55);
    }
    
    if (strcmp(posName, "a8") == 0) {
        return getBitboardFromSquare(56);
    } else if (strcmp(posName, "b8") == 0) {
        return getBitboardFromSquare(57);
    } else if (strcmp(posName, "c8") == 0) {
        return getBitboardFromSquare(58);
    } else if (strcmp(posName, "d8") == 0) {
        return getBitboardFromSquare(59);
    } else if (strcmp(posName, "e8") == 0) {
        return getBitboardFromSquare(60);
    } else if (strcmp(posName, "f8") == 0) {
        return getBitboardFromSquare(61);
    } else if (strcmp(posName, "g8") == 0) {
        return getBitboardFromSquare(62);
    } else if (strcmp(posName, "h8") == 0) {
        return getBitboardFromSquare(63);
    }
    
    return 0;
}

int divide(u8 depth, u8 sideToMove, Thread *th) {
    
    char pieceName[2][8] = { { ' ', (char) 0, 'N', 'B', 'R', 'Q', 'K', '\0'}, { ' ', (char) 0, 'n', 'b', 'r', 'q', 'k', '\0'}};
    
    u64 total_nodes = 0;

    char pType[24];
    char* moveType;
    u64 nodes;

    std::vector<Move> moves;

    int ply = 0;
    genMoves(ply, moves, sideToMove, th);
    u8 count = 0;

    clock_t start, end;
    double cpu_time_used;
    double nps;

	Move move;
    for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i)
    {

        start = clock();
       
		move = *i;
		
        make_move(ply, move.move, th);
        
        switch(move_type(move.move)) {

            case MOVE_NORMAL:

                moveType = "MOVE_NORMAL";
            break;
            case MOVE_CAPTURE:

                moveType = "MOVE_CAPTURE";
            break;
            case MOVE_DOUBLE_PUSH:
            
                moveType = "MOVE_DOUBLE_PUSH";
            break;
            case MOVE_ENPASSANT:
            
                moveType = "MOVE_ENPASSANT";
            break;
            case MOVE_CASTLE:
            
                moveType = "MOVE_CASTLE";
            break;
            case MOVE_PROMOTION:
    
                sprintf(pType, "MOVE_PROMOTION-%d", promType(move.move));
                moveType = pType;
            break;
        }
        
        nodes = 0;

        if (!isKingInCheck(sideToMove, th)) {

            count++;

            nodes = (sideToMove ^ 1) ? perft(ply + 1, depth - 1, BLACK, th) : perft(ply + 1, depth - 1, WHITE, th);
            
            end  = clock();
            
            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
            nps = (double)(nodes / (cpu_time_used * 1000000));
            
            total_nodes = total_nodes + nodes;
            
            printf("%d)%c%s-%s, ", count, pieceName[colorType(move.move)][pieceType(move.move)],
                   algebricPos(from_sq(move.move)), algebricPos(to_sq(move.move)));
            
            printf("%llu, %s, castle flags -" BYTE_TO_BINARY_PATTERN", nps  - %7.3f MN/s\n", nodes, moveType, BYTE_TO_BINARY(th->moveStack[ply].castleFlags),  nps);
        }
        
        unmake_move(ply, move.move, th);
    }
    
    printf("Total nodes -> %llu\n", total_nodes);

    return 0;
}

void checkUp() {    
    

    std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();

    if (timeSet && timeNow.time_since_epoch() > stopTime.time_since_epoch()) {
        
        stopped = true;

        return;
    }

    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

    if (timeSet && timeSpent >= 0.8 * totalTimeLeft) {

        stopped = true;

        return;
    }
}


void initHashKey(Thread *th) {
	
    int sq;
	u64 bitboard;
	
	th->hashKey = 0ULL;
    for(int side = WHITE; side <= BLACK; side++) {
    
        for(int piece = PAWNS; piece <= KING; piece++) {
            
            bitboard = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
            while (bitboard) {
                
                sq = GET_POSITION(bitboard);
                POP_POSITION(bitboard);
                    
                assert(sq >= 0 && sq < 64);    

				th->hashKey ^= zobrist[piece][side][sq];
            }
        }
    }

    th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
    th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
    th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
    th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE; 
}

void initPawnHashKey(u8 side, Thread *th) {

    u64 bitboard = th->blackPieceBB[PAWNS] | th->whitePieceBB[PAWNS];
    
    th->pawnsHashKey = 0ULL;

    u8 sq;
    while(bitboard) {

        sq = GET_POSITION(bitboard);
        POP_POSITION(bitboard);

        th->pawnsHashKey ^= pawnZobristKey[sq];
    }

    if (side) th->hashKey ^= KEY_SIDE_TO_MOVE;
}



HASHE *hashTable;
u32 HASH_TABLE_SIZE;

void initHashTable(int size) {

    if (sizeof(hashTable) > 0) {

        delete[] hashTable; 
    }

    int sizeOfSingleEntry = sizeof(HASHE);
    HASH_TABLE_SIZE = (size * 1024 * 1024) / sizeOfSingleEntry;

    
    hashTable = new HASHE[HASH_TABLE_SIZE];

    clearHashTable();
}


void clearHashTable() {
	
    std::memset(hashTable, 0, HASH_TABLE_SIZE * sizeof(HASHE));
}

// void clearPawnHashTable(Thread *th) {

//     std::memset(th->pawnHashTable, 0, PAWN_HASH_TABLE_SIZE * sizeof(PAWNS_HASH));
// }

// void clearEvalHashTable(Thread *th) {

//     std::memset(th->evalHashTable, 0, EVAL_HASH_TABLE_SIZE * sizeof(EVAL_HASH));
// }


void initMovesHistoryTable(Thread *th) {
	
	for (int i = 0; i < MAX_PLY; i++) {
		
		th->movesHistory[i].hashKey = 0ULL;
	}

    th->movesHistory[0].fiftyMovesCounter = 0;
	
    th->moves_history_counter = 0;
}

void clearKillerMovesTable(Thread *th) {
	
	for (int i = 0; i < MAX_PLY; i++) {

        th->moveStack[i].killerMoves[0] = NO_MOVE;
        th->moveStack[i].killerMoves[1] = NO_MOVE;
	}
}

void clearHistoryTable(Thread *th) {
    
	for(u8 side = WHITE; side <= BLACK; side++) {
		for(u8 from = 0; from < 64; from++) {
			for(u8 to = 0; to < 64; to++) {
	
				th->historyScore[side][from][to] = 0;
			}
		}
	}
}



u64 nortFill(u64 gen) {
   gen |= (gen <<  8);
   gen |= (gen << 16);
   gen |= (gen << 32);
   return gen;
}

u64 soutFill(u64 gen) {
   gen |= (gen >>  8);
   gen |= (gen >> 16);
   gen |= (gen >> 32);
   return gen;
}

u64 soutOne (u64 b) {return  b >> 8;}
u64 nortOne (u64 b) {return  b << 8;}
u64 eastOne (u64 b) {return (b << 1) & NOT_A_FILE;}
u64 westOne (u64 b) {return (b >> 1) & NOT_H_FILE;}
u64 noEaOne (u64 b) {return (b << 9) & NOT_A_FILE;}
u64 soEaOne (u64 b) {return (b >> 7) & NOT_A_FILE;}
u64 soWeOne (u64 b) {return (b >> 9) & NOT_H_FILE;}
u64 noWeOne (u64 b) {return (b << 7) & NOT_H_FILE;}

u64 wFrontSpans(u64 wpawns) {return nortOne (nortFill(wpawns));}
u64 bRearSpans (u64 bpawns) {return nortOne (nortFill(bpawns));}
u64 bFrontSpans(u64 bpawns) {return soutOne (soutFill(bpawns));}
u64 wRearSpans (u64 wpawns) {return soutOne (soutFill(wpawns));}

u64 wFrontFill(u64 wpawns) {return nortFill(wpawns);}
u64 wRearFill (u64 wpawns) {return soutFill(wpawns);}

u64 bFrontFill(u64 bpawns) {return soutFill(bpawns);}
u64 bRearFill (u64 bpawns) {return nortFill(bpawns);}

u64 fileFill(u64 gen) {
   return nortFill(gen) | soutFill(gen);
}

u64 wEastAttackFrontSpans (u64 wpawns) {return eastOne(wFrontSpans(wpawns));}
u64 wWestAttackFrontSpans (u64 wpawns) {return westOne(wFrontSpans(wpawns));}
u64 bEastAttackFrontSpans (u64 bpawns) {return eastOne(bFrontSpans(bpawns));}
u64 bWestAttackFrontSpans (u64 bpawns) {return westOne(bFrontSpans(bpawns));}

u64 wEastAttackRearSpans (u64 wpawns)  {return eastOne(wRearFill(wpawns));}
u64 wWestAttackRearSpans (u64 wpawns)  {return westOne(wRearFill(wpawns));}
u64 bEastAttackRearSpans (u64 bpawns)  {return eastOne(bRearFill(bpawns));}
u64 bWestAttackRearSpans (u64 bpawns)  {return westOne(bRearFill(bpawns));}

u64 eastAttackFileFill (u64 pawns) {return eastOne(fileFill(pawns));}
u64 westAttackFileFill (u64 pawns) {return westOne(fileFill(pawns));}

u64 wPawnEastAttacks(u64 wpawns) {return noEaOne(wpawns);}
u64 wPawnWestAttacks(u64 wpawns) {return noWeOne(wpawns);}

u64 bPawnEastAttacks(u64 bpawns) {return soEaOne(bpawns);}
u64 bPawnWestAttacks(u64 bpawns) {return soWeOne(bpawns);}

u64 inBetweenOnTheFly(u8 sq1, u8 sq2) {

   const U64 m1   = C64(-1);
   const U64 a2a7 = C64(0x0001010101010100);
   const U64 b2g7 = C64(0x0040201008040200);
   const U64 h1b7 = C64(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
   u64 btwn, line, rank, file;

   btwn  = (m1 << sq1) ^ (m1 << sq2);
   file  =   (sq2 & 7) - (sq1   & 7);
   rank  =  ((sq2 | 7) -  sq1) >> 3 ;
   line  =      (   (file  &  7) - 1) & a2a7; /* a2a7 if same file */
   line += 2 * ((   (rank  &  7) - 1) >> 58); /* b1g1 if same rank */
   line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
   line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
   line *= btwn & -btwn; /* mul acts like shift by smaller kingSq */
   return line & btwn;   /* return the bits on that line in-between */
}

u64 inBetween(u8 from, u8 to) {
   return arrInBetween[from][to];
}

u64 xrayRookAttacks(u64 occ, u64 blockers, u8 rookSq) {
   u64 attacks = Rmagic(rookSq, occ);
   blockers &= attacks;
   return attacks ^ Rmagic(rookSq, occ ^ blockers);
}

u64 xrayBishopAttacks(u64 occ, u64 blockers, u8 bishopSq) {
   u64 attacks = Bmagic(bishopSq, occ);
   blockers &= attacks;
   return attacks ^ Bmagic(bishopSq, occ ^ blockers);
}

u64 pinners(u8 kingSq, u8 side, Thread *th) {

    u64 pinners1 = xrayRookAttacks(th->occupied, side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES], kingSq)
        & (side ^ 1 ? th->blackPieceBB[ROOKS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[ROOKS] | th->whitePieceBB[QUEEN]);
    
    u64 pinners2 = xrayBishopAttacks(th->occupied, (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]), kingSq) 
        & (side ^ 1 ? th->blackPieceBB[BISHOPS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[BISHOPS] | th->whitePieceBB[QUEEN]);
    
    return pinners1 | pinners2;
}

u64 pinnedPieces(u8 kingSq, u8 side, Thread *th) {

    u64 pinned = 0ULL;
    
    u64 pinner = xrayRookAttacks(th->occupied, side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES], kingSq)
        & (side ^ 1 ? th->blackPieceBB[ROOKS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[ROOKS] | th->whitePieceBB[QUEEN]);
    while ( pinner ) {

        int sq  = GET_POSITION(pinner);
        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
        POP_POSITION(pinner);
    }
    
    pinner = xrayBishopAttacks(th->occupied, (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]), kingSq) 
        & (side ^ 1 ? th->blackPieceBB[BISHOPS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[BISHOPS] | th->whitePieceBB[QUEEN]);
    while ( pinner ) {
        int sq  = GET_POSITION(pinner);
        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
        POP_POSITION(pinner);
    }

    return pinned;
}

u64 pinned(u64 pinners, u8 kingSq, u8 side, Thread *th) {

    u64 pinned = 0ULL;
    
    while ( pinners ) {

        int sq  = GET_POSITION(pinners);

        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

        POP_POSITION(pinners);
    }

    return pinned;
}




