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

#include "constants.h"
#include "types.h"
#include "structs.h"

extern int MAX_DEPTH;

extern HASHE *hashTable;
extern U32 HASH_TABLE_SIZE;

extern U8 rookCastleFlagMask[U8_MAX_SQUARES];

extern U64 arrInBetween[U8_MAX_SQUARES][U8_MAX_SQUARES];


#endif /* globals_h */
