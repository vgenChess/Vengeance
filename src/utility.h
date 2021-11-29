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

void print_bb(u64 board);

void print_board(u64 board, Thread *th);

int bitScanForward(u64 board);

bool isKingInCheck(u8 sideToMove, Thread *th);

bool isSqAttacked(u8 sq, const u8 color, Thread *th);

int divide(u8 depth, u8 sideToMove, Thread *th);

u8 squareFromAlgebricPos(char* posName);

u64 getBitboardFromSquare(int sq);

u64 bbFromAlgebricPos(char* posName);

int popCount (u64 x);

u64 flipVertical(u64 x);

char* algebricPos(u8 sq);

// u64 rand64(void);

void clearAllBitBoards(Thread *th);

void checkUp(void);

void initHashTable(int size);

void clearHashTable(void);
void clearPawnHashTable(Thread *th);
void clearEvalHashTable(Thread *th);
void clearKillerMovesTable(Thread *th);
void clearHistoryTable(Thread *th);

void initHashKey(Thread *th);
void initPawnHashKey(u8 side, Thread *th);
void initMovesHistoryTable(Thread *th);

u64 soutOne (u64 b);
u64 nortOne (u64 b);
u64 eastOne (u64 b);
u64 westOne (u64 b);
u64 noEaOne (u64 b);
u64 soEaOne (u64 b);
u64 soWeOne (u64 b);
u64 noWeOne (u64 b);
u64 wFrontSpans(u64 wpawns);
u64 bRearSpans (u64 bpawns);
u64 bFrontSpans(u64 bpawns);
u64 wRearSpans (u64 wpawns);
u64 nortFill(u64 gen);
u64 soutFill(u64 gen);
u64 wFrontFill(u64 wpawns);
u64 wRearFill (u64 wpawns);
u64 bFrontFill(u64 bpawns);
u64 bRearFill (u64 bpawns);
u64 fileFill(u64 gen);
u64 wEastAttackFrontSpans (u64 wpawns);
u64 wWestAttackFrontSpans (u64 wpawns);
u64 bEastAttackFrontSpans (u64 bpawns);
u64 bWestAttackFrontSpans (u64 bpawns);
u64 wEastAttackRearSpans (u64 wpawns);
u64 wWestAttackRearSpans (u64 wpawns);
u64 bEastAttackRearSpans (u64 bpawns);
u64 bWestAttackRearSpans (u64 bpawns);
u64 eastAttackFileFill (u64 pawns);
u64 westAttackFileFill (u64 pawns);
u64 wPawnEastAttacks(u64 wpawns);
u64 wPawnWestAttacks(u64 wpawns);
u64 bPawnEastAttacks(u64 bpawns);
u64 bPawnWestAttacks(u64 bpawns);

u64 inBetweenOnTheFly(u8 sq1, u8 sq2);
u64 inBetween(u8 from, u8 to);

u64 xrayRookAttacks(u64 occ, u64 blockers, u8 rookSq);
u64 xrayBishopAttacks(u64 occ, u64 blockers, u8 bishopSq);

u64 pinnedPieces(u8 kingSq, u8 side, Thread *th);
u64 pinners(u8 kingSq, u8 side, Thread *th);
u64 pinned(u64 pinners, u8 kingSq, u8 side, Thread *th);

#endif /* utility_h */
