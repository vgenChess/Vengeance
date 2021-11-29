//
//  nonslidingmoves.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef nonslidingmoves_h
#define nonslidingmoves_h

#include "globals.h"
#include "evaluate.h"

u64 get_king_attacks(int sq);
u64 king_attacks(u64 kingSet);
u64 get_knight_attacks(int sq);
u64 knight_attacks(u64 knightSet);

void init_king_attacks(void);
void init_knight_attacks(void);

// https://chessprogramming.wikispaces.com/Pawn+Pushes+%28Bitboards%29

u64 wSinglePushTargets(u64 wpawns, u64 empty);
u64 wDblPushTargets(u64 wpawns, u64 empty);

u64 bSinglePushTargets(u64 bpawns, u64 empty);
u64 bDoublePushTargets(u64 bpawns, u64 empty);

u64 wPawnsAble2Push(u64 wpawns, u64 empty);
u64 wPawnsAble2DblPush(u64 wpawns, u64 empty);

u64 bPawnsAble2Push(u64 bpawns, u64 empty);
u64 bPawnsAble2DblPush(u64 bpawns, u64 empty);

u64 wPawnAnyAttacks(u64 wpawns);
u64 wPawnDblAttacks(u64 wpawns);
u64 wPawnSingleAttacks(u64 wpawns);

u64 bPawnAnyAttacks(u64 bpawns);
u64 bPawnDblAttacks(u64 bpawns);
u64 bPawnSingleAttacks(u64 bpawns);

#endif /* nonslidingmoves_h */
