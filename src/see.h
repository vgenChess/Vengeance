#ifndef see_h
#define see_h

#include "utility.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "thread.h"

int SEE(Side sideToMove, U32 move, Thread *th);
void debugSEE(char ch, int square);
#endif
