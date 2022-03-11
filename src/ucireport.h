#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>

#include "globals.h"
#include "thread.h"

std::string getMoveNotation(const U32 move);
void reportBestMove();
void reportPV(SearchThread *th);

#endif