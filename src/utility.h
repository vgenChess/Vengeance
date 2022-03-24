//
//  utility.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef utility_h
#define utility_h

#include "thread.h"
#include "functions.h"

void print_bb(U64 board);

void print_board(U64 board, Thread *th);

bool isSqAttacked(U8 sq, const U8 color, Thread *th);

int divide(U8 depth, U8 sideToMove, Thread *th);

U8 squareFromAlgebricPos(const char* posName);

U64 flipVertical(U64 x);

char* algebricPos(U8 sq);

void clearKillerMovesTable(Thread *th);

void initHashKey(Thread *th);
void initPawnHashKey(U8 side, Thread *th);
void initMovesHistoryTable(Thread *th);
void init_inbetween_bb();
void initCastleMaskAndFlags();

U64 getAttacks(const U8 stm, Thread *th);

inline U64 nortFill(U64 gen) 
{
   gen |= (gen <<  8);
   gen |= (gen << 16);
   gen |= (gen << 32);
   return gen;
}

inline U64 soutFill(U64 gen) 
{
   gen |= (gen >>  8);
   gen |= (gen >> 16);
   gen |= (gen >> 32);
   return gen;
}

inline U64 soutOne (U64 b) {return  b >> 8;}
inline U64 nortOne (U64 b) {return  b << 8;}
inline U64 eastOne (U64 b) {return (b << 1) & NOT_A_FILE;}
inline U64 westOne (U64 b) {return (b >> 1) & NOT_H_FILE;}
inline U64 noEaOne (U64 b) {return (b << 9) & NOT_A_FILE;}
inline U64 soEaOne (U64 b) {return (b >> 7) & NOT_A_FILE;}
inline U64 soWeOne (U64 b) {return (b >> 9) & NOT_H_FILE;}
inline U64 noWeOne (U64 b) {return (b << 7) & NOT_H_FILE;}

inline U64 wFrontSpans(U64 wpawns) {return nortOne (nortFill(wpawns));}
inline U64 bRearSpans (U64 bpawns) {return nortOne (nortFill(bpawns));}
inline U64 bFrontSpans(U64 bpawns) {return soutOne (soutFill(bpawns));}
inline U64 wRearSpans (U64 wpawns) {return soutOne (soutFill(wpawns));}

inline U64 wFrontFill(U64 wpawns) {return nortFill(wpawns);}
inline U64 wRearFill (U64 wpawns) {return soutFill(wpawns);}
inline U64 bFrontFill(U64 bpawns) {return soutFill(bpawns);}
inline U64 bRearFill (U64 bpawns) {return nortFill(bpawns);}

inline U64 fileFill(U64 gen) {return nortFill(gen) | soutFill(gen);}

inline U64 wEastAttackFrontSpans (U64 wpawns) {return eastOne(wFrontSpans(wpawns));}
inline U64 wWestAttackFrontSpans (U64 wpawns) {return westOne(wFrontSpans(wpawns));}
inline U64 bEastAttackFrontSpans (U64 bpawns) {return eastOne(bFrontSpans(bpawns));}
inline U64 bWestAttackFrontSpans (U64 bpawns) {return westOne(bFrontSpans(bpawns));}

inline U64 wEastAttackRearSpans (U64 wpawns)  {return eastOne(wRearFill(wpawns));}
inline U64 wWestAttackRearSpans (U64 wpawns)  {return westOne(wRearFill(wpawns));}
inline U64 bEastAttackRearSpans (U64 bpawns)  {return eastOne(bRearFill(bpawns));}
inline U64 bWestAttackRearSpans (U64 bpawns)  {return westOne(bRearFill(bpawns));}

inline U64 eastAttackFileFill (U64 pawns) {return eastOne(fileFill(pawns));}
inline U64 westAttackFileFill (U64 pawns) {return westOne(fileFill(pawns));}

inline U64 wPawnEastAttacks(U64 wpawns) {return noEaOne(wpawns);}
inline U64 wPawnWestAttacks(U64 wpawns) {return noWeOne(wpawns);}

inline U64 bPawnEastAttacks(U64 bpawns) {return soEaOne(bpawns);}
inline U64 bPawnWestAttacks(U64 bpawns) {return soWeOne(bpawns);}

inline U64 pawnsWithEastNeighbors(U64 pawns) {return pawns & westOne (pawns);}

inline U64 pawnsWithWestNeighbors(U64 pawns) {return pawnsWithEastNeighbors(pawns) << 1;} // * 2

inline U64 islandsEastFiles(U64 f) {return f & (f ^ (f >> 1));}
inline U64 islandsWestFiles(U64 f) {return f & (f ^ (f << 1));} // ... (f+f)

inline U64 defendedDefenders1 (U64 b) {return b & soWeOne(b) & noEaOne(b);}
inline U64 defendedDefenders2 (U64 b) {return b & soEaOne(b) & noWeOne(b);}

U64 inBetweenOnTheFly(U8 sq1, U8 sq2);

U64 inBetween(int from, int to);

U64 xrayRookAttacks(U64 occ, U64 blockers, U8 rookSq);
U64 xrayBishopAttacks(U64 occ, U64 blockers, U8 bishopSq);

U64 pinnedPieces(U8 kingSq, U8 side, Thread *th);
U64 pinners(U8 kingSq, U8 side, Thread *th);
U64 pinned(U64 pinners, U8 kingSq, U8 side, Thread *th);

#endif /* utility_h */
