#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>

#include "globals.h"
#include "thread.h"

std::string getMoveNotation(const U32 move);
void reportBestMove();
void reportCurrentMove(int depth, int currentMoveNumber, U32 move);
void reportPV(SearchThread *th);

#endif