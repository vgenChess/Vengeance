//
//  nonslidingmoves.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include "functions.h"
#include "nonslidingmoves.h"
#include "utility.h"

U64 arrKingAttacks[64];
U64 arrKnightAttacks[64];

U64 get_king_attacks(int sq) {
    return arrKingAttacks[sq];
}

U64 king_attacks(U64 kingSet) {
    U64 attacks = eastOne(kingSet) | westOne(kingSet);
    kingSet |= attacks;
    attacks |= nortOne(kingSet) | soutOne(kingSet);
    
    return attacks;
}

void init_king_attacks() {
    U64 sqBB = 1;
    int sq;
    for (sq = 0; sq < 64; sq++, sqBB <<= 1)
        arrKingAttacks[sq] = king_attacks(sqBB);
}

U64 get_knight_attacks(int sq) {
    return arrKnightAttacks[sq];
}

U64 knight_attacks(U64 knights) {
    U64 west, east, attacks;
    
    east     = eastOne (knights);
    west     = westOne (knights);
    attacks  = (east | west) << 16;
    attacks |= (east | west) >> 16;
    east     = eastOne (east);
    west     = westOne (west);
    attacks |= (east | west) <<  8;
    attacks |= (east | west) >>  8;
    
    return attacks;
}

void init_knight_attacks() {
    U64 sqBB = 1;
    int sq;
    
    for (sq = 0; sq < 64; sq++, sqBB <<= 1)
        arrKnightAttacks[sq] = knight_attacks(sqBB);
}

// compute and return moves for pawns

U64 wSinglePushTargets(U64 wpawns, U64 empty) {
    return nortOne(wpawns) & empty;
}

U64 wDblPushTargets(U64 wpawns, U64 empty) {
    U64 singlePushs = wSinglePushTargets(wpawns, empty);
    
    return nortOne(singlePushs) & empty & RANK_4;
}

U64 bSinglePushTargets(U64 bpawns, U64 empty) {
    return soutOne(bpawns) & empty;
}

U64 bDoublePushTargets(U64 bpawns, U64 empty) {
    U64 singlePushs = bSinglePushTargets(bpawns, empty);
    
    return soutOne(singlePushs) & empty & RANK_5;
}

U64 wPawnsAble2Push(U64 wpawns, U64 empty) {
    
    return soutOne(empty) & wpawns;
}

U64 wPawnsAble2DblPush(U64 wpawns, U64 empty) {
    U64 emptyRank3 = soutOne(empty & RANK_4) & empty;
    
    return wPawnsAble2Push(wpawns, emptyRank3);
}

U64 bPawnsAble2Push(U64 bpawns, U64 empty) {
    
    return soutOne(empty) & bpawns;
}

U64 bPawnsAble2DblPush(U64 bpawns, U64 empty) {
    U64 emptyRank3 = soutOne(empty & RANK_4) & empty;
    
    return bPawnsAble2Push(bpawns, emptyRank3);
}

// compute and return attacks for pawns

U64 wPawnAnyAttacks(U64 wpawns) {
    return wPawnEastAttacks(wpawns) | wPawnWestAttacks(wpawns);
}

U64 wPawnDblAttacks(U64 wpawns) {
    return wPawnEastAttacks(wpawns) & wPawnWestAttacks(wpawns);
}

U64 wPawnSingleAttacks(U64 wpawns) {
    return wPawnEastAttacks(wpawns) ^ wPawnWestAttacks(wpawns);
}

U64 bPawnAnyAttacks(U64 bpawns) {
    return bPawnEastAttacks(bpawns) | bPawnWestAttacks(bpawns);
}

U64 bPawnDblAttacks(U64 bpawns) {
    return bPawnEastAttacks(bpawns) & bPawnWestAttacks(bpawns);
}

U64 bPawnSingleAttacks(U64 bpawns) {
    return bPawnEastAttacks(bpawns) ^ bPawnWestAttacks(bpawns);
}
