#ifndef history_h
#define history_h

#include <vector>

#include "types.h"
#include "thread.h"

void updateHistory(int ply, int side, int depth, U32 bestMove, std::vector<U32> &quietMovesPlayed, Thread *th);
void updateCaptureHistory(int depth, U32 bestMove,std::vector<U32>&captureMovesPlayed, Thread *th);

#endif 