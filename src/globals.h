//
//  globals.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef globals_h
#define globals_h

#include <stdbool.h>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <ratio>
#include <chrono>

#include "constants.h"
#include "types.h"
#include "structs.h"

extern int MAX_DEPTH;

// for perft
extern U64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;

extern U64 KEY_SIDE_TO_MOVE;

extern U64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
extern U64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
extern U64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
extern U64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;

extern U64 KEY_EP_A_FILE;
extern U64 KEY_EP_B_FILE;
extern U64 KEY_EP_C_FILE;
extern U64 KEY_EP_D_FILE;
extern U64 KEY_EP_E_FILE;
extern U64 KEY_EP_F_FILE;
extern U64 KEY_EP_G_FILE;
extern U64 KEY_EP_H_FILE;

// zobrist keys

extern U64 zobrist[U8_MAX_PIECES][U8_MAX_SIDES][U8_MAX_SQUARES];
extern U64 pawnZobristKey[U8_MAX_SQUARES];

extern HASHE *hashTable;
extern U32 HASH_TABLE_SIZE;

extern U8 rookCastleFlagMask[U8_MAX_SQUARES];

// for time management

extern bool timeSet;
extern bool stopped;

extern int timePerMove;

extern std::chrono::steady_clock::time_point startTime;
extern std::chrono::steady_clock::time_point stopTime;

extern U64 arrInBetween[U8_MAX_SQUARES][U8_MAX_SQUARES];


#endif /* globals_h */
