#ifndef history_h
#define history_h

#include <vector>

#include "globals.h"
#include "thread.h"

void updateHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &quietMovesPlayed, Thread *th);
void updateCaptureHistory(int ply, int side, int depth, u32 bestMove,std::vector<u32>&captureMovesPlayed, Thread *th);

#endif 