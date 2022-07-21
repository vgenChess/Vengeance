#ifndef history_h
#define history_h

#include <vector>

#include "types.h"
#include "namespaces.h"

using namespace game;

void updateHistory(Side stm, int ply, int depth, U32 bestMove, std::vector<U32> &quietMovesPlayed, GameInfo *th);
void updateCaptureHistory(int depth, U32 bestMove,std::vector<U32>&captureMovesPlayed, GameInfo *th);

#endif 
