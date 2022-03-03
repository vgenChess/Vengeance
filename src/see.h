#ifndef see_h
#define see_h

#include "globals.h"
#include "utility.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "thread.h"

int SEE(U32 move, U8 sideToMove, Thread *th);
void debugSEE(char ch, int square);
#endif
