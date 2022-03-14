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
