#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>

#include "globals.h"

std::string getMoveNotation(const u32 move);
void reportBestMove();
void reportCurrentMove(int side, int depth, int currentMoveNumber, u32 move);
void display(u8 sideToMove, int depth, int selDepth, int score, std::vector<u32> pvLine);

#endif