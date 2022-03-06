//
//  utility.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef utility_h
#define utility_h

#include "globals.h"
#include "thread.h"

void print_bb(U64 board);

void print_board(U64 board, Thread *th);

int bitScanForward(U64 board);

bool isKingInCheck(U8 sideToMove, Thread *th);

bool isSqAttacked(U8 sq, const U8 color, Thread *th);

int divide(U8 depth, U8 sideToMove, Thread *th);

U8 squareFromAlgebricPos(char* posName);

U64 getBitboardFromSquare(int sq);

U64 bbFromAlgebricPos(char* posName);

int popCount (U64 x);

U64 flipVertical(U64 x);

char* algebricPos(U8 sq);

void clearAllBitBoards(Thread *th);

void initHashTable(int size);

void clearHashTable(void);
void clearPawnHashTable(Thread *th);
void clearEvalHashTable(Thread *th);
void clearKillerMovesTable(Thread *th);
void clearHistoryTable(Thread *th);

void initHashKey(Thread *th);
void initPawnHashKey(U8 side, Thread *th);
void initMovesHistoryTable(Thread *th);

bool isRepetition(const int ply, Thread *th);
bool isPositionDraw(Thread *th);

U64 getAttacks(const U8 stm, Thread *th);

U64 soutOne (U64 b);
U64 nortOne (U64 b);
U64 eastOne (U64 b);
U64 westOne (U64 b);
U64 noEaOne (U64 b);
U64 soEaOne (U64 b);
U64 soWeOne (U64 b);
U64 noWeOne (U64 b);
U64 wFrontSpans(U64 wpawns);
U64 bRearSpans (U64 bpawns);
U64 bFrontSpans(U64 bpawns);
U64 wRearSpans (U64 wpawns);
U64 nortFill(U64 gen);
U64 soutFill(U64 gen);
U64 wFrontFill(U64 wpawns);
U64 wRearFill (U64 wpawns);
U64 bFrontFill(U64 bpawns);
U64 bRearFill (U64 bpawns);
U64 fileFill(U64 gen);
U64 wEastAttackFrontSpans (U64 wpawns);
U64 wWestAttackFrontSpans (U64 wpawns);
U64 bEastAttackFrontSpans (U64 bpawns);
U64 bWestAttackFrontSpans (U64 bpawns);
U64 wEastAttackRearSpans (U64 wpawns);
U64 wWestAttackRearSpans (U64 wpawns);
U64 bEastAttackRearSpans (U64 bpawns);
U64 bWestAttackRearSpans (U64 bpawns);
U64 eastAttackFileFill (U64 pawns);
U64 westAttackFileFill (U64 pawns);
U64 wPawnEastAttacks(U64 wpawns);
U64 wPawnWestAttacks(U64 wpawns);
U64 bPawnEastAttacks(U64 bpawns);
U64 bPawnWestAttacks(U64 bpawns);
U64 pawnsWithEastNeighbors(U64 pawns);
U64 pawnsWithWestNeighbors(U64 pawns);

U64 islandsEastFiles(U64 f);
U64 islandsWestFiles(U64 f);

U64 inBetweenOnTheFly(U8 sq1, U8 sq2);
U64 inBetween(U8 from, U8 to);

U64 xrayRookAttacks(U64 occ, U64 blockers, U8 rookSq);
U64 xrayBishopAttacks(U64 occ, U64 blockers, U8 bishopSq);

U64 pinnedPieces(U8 kingSq, U8 side, Thread *th);
U64 pinners(U8 kingSq, U8 side, Thread *th);
U64 pinned(U64 pinners, U8 kingSq, U8 side, Thread *th);

#endif /* utility_h */
