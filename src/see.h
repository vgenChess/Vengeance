#ifndef see_h
#define see_h

#include "globals.h"
#include "utility.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "thread.h"

template<Side sideToMove> int SEE(U32 move, Thread *th);
void debugSEE(char ch, int square);
#endif
