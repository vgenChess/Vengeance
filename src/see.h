#ifndef see_h
#define see_h

#include "globals.h"
#include "utility.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "thread.h"

int see(u32 move, u8 side, Thread *th);
int SEE(u32 move, u8 sideToMove, Thread *th);
int newSEE(u32 move, u8 sideToMove, Thread *th);
#endif
