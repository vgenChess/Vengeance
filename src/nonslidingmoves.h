//
//  nonslidingmoves.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef nonslidingmoves_h
#define nonslidingmoves_h

#include "types.h"

U64 get_king_attacks(int sq);
U64 king_attacks(U64 kingSet);
U64 get_knight_attacks(int sq);
U64 knight_attacks(U64 knightSet);

void init_king_attacks(void);
void init_knight_attacks(void);

U64 wSinglePushTargets(U64 wpawns, U64 empty);
U64 wDblPushTargets(U64 wpawns, U64 empty);

U64 bSinglePushTargets(U64 bpawns, U64 empty);
U64 bDoublePushTargets(U64 bpawns, U64 empty);

U64 wPawnsAble2Push(U64 wpawns, U64 empty);
U64 wPawnsAble2DblPush(U64 wpawns, U64 empty);

U64 bPawnsAble2Push(U64 bpawns, U64 empty);
U64 bPawnsAble2DblPush(U64 bpawns, U64 empty);

U64 wPawnAnyAttacks(U64 wpawns);
U64 wPawnDblAttacks(U64 wpawns);
U64 wPawnSingleAttacks(U64 wpawns);

U64 bPawnAnyAttacks(U64 bpawns);
U64 bPawnDblAttacks(U64 bpawns);
U64 bPawnSingleAttacks(U64 bpawns);

#endif /* nonslidingmoves_h */
