//
//  nonslidingmoves.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include "nonslidingmoves.h"
#include "utility.h"

//https://chessprogramming.wikispaces.com/General+Setwise+Operations#OneStepOnly
const u64 notAFile = C64(0xfefefefefefefefe); // ~0x0101010101010101
const u64 notHFile = C64(0x7f7f7f7f7f7f7f7f); // ~0x8080808080808080

u64 arrKingAttacks[64];
u64 arrKnightAttacks[64];

//https://chessprogramming.wikispaces.com/King+Pattern

u64 get_king_attacks(int sq) {
    return arrKingAttacks[sq];
}

u64 king_attacks(u64 kingSet) {
    u64 attacks = eastOne(kingSet) | westOne(kingSet);
    kingSet |= attacks;
    attacks |= nortOne(kingSet) | soutOne(kingSet);
    
    return attacks;
}

void init_king_attacks() {
    u64 sqBB = 1;
    int sq;
    for (sq = 0; sq < 64; sq++, sqBB <<= 1)
        arrKingAttacks[sq] = king_attacks(sqBB);
}

u64 get_knight_attacks(int sq) {
    return arrKnightAttacks[sq];
}

u64 knight_attacks(u64 knights) {
    u64 west, east, attacks;
    
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
    u64 sqBB = 1;
    int sq;
    
    for (sq = 0; sq < 64; sq++, sqBB <<= 1)
        arrKnightAttacks[sq] = knight_attacks(sqBB);
}

// compute and return moves for pawns

u64 wSinglePushTargets(u64 wpawns, u64 empty) {
    return nortOne(wpawns) & empty;
}

u64 wDblPushTargets(u64 wpawns, u64 empty) {
    const u64 rank4 = C64(0x00000000FF000000);
    u64 singlePushs = wSinglePushTargets(wpawns, empty);
    
    return nortOne(singlePushs) & empty & rank4;
}

u64 bSinglePushTargets(u64 bpawns, u64 empty) {
    return soutOne(bpawns) & empty;
}

u64 bDoublePushTargets(u64 bpawns, u64 empty) {
    const u64 rank5 = C64(0x000000FF00000000);
    u64 singlePushs = bSinglePushTargets(bpawns, empty);
    
    return soutOne(singlePushs) & empty & rank5;
}

u64 wPawnsAble2Push(u64 wpawns, u64 empty) {
    
    return soutOne(empty) & wpawns;
}

u64 wPawnsAble2DblPush(u64 wpawns, u64 empty) {
    const u64 rank4 = C64(0x00000000FF000000);
    u64 emptyRank3 = soutOne(empty & rank4) & empty;
    
    return wPawnsAble2Push(wpawns, emptyRank3);
}

u64 bPawnsAble2Push(u64 bpawns, u64 empty) {
    
    return soutOne(empty) & bpawns;
}

u64 bPawnsAble2DblPush(u64 bpawns, u64 empty) {
    const u64 rank4 = C64(0x00000000FF000000);
    u64 emptyRank3 = soutOne(empty & rank4) & empty;
    
    return bPawnsAble2Push(bpawns, emptyRank3);
}

// compute and return attacks for pawns

u64 wPawnAnyAttacks(u64 wpawns) {
    return wPawnEastAttacks(wpawns) | wPawnWestAttacks(wpawns);
}

u64 wPawnDblAttacks(u64 wpawns) {
    return wPawnEastAttacks(wpawns) & wPawnWestAttacks(wpawns);
}

u64 wPawnSingleAttacks(u64 wpawns) {
    return wPawnEastAttacks(wpawns) ^ wPawnWestAttacks(wpawns);
}

u64 bPawnAnyAttacks(u64 bpawns) {
    return bPawnEastAttacks(bpawns) | bPawnWestAttacks(bpawns);
}

u64 bPawnDblAttacks(u64 bpawns) {
    return bPawnEastAttacks(bpawns) & bPawnWestAttacks(bpawns);
}

u64 bPawnSingleAttacks(u64 bpawns) {
    return bPawnEastAttacks(bpawns) ^ bPawnWestAttacks(bpawns);
}
