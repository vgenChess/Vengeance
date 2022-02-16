#ifndef hash_h
#define hash_h

#include "globals.h"
#include "thread.h"

bool probeHash(HASHE *tt, Thread *th);
void recordHash(u32 bestMove, int depth, int value, int hashf, int eval_static, Thread *th);

bool probePawnHash(int *score, Thread *th);
void recordPawnHash(int score, Thread *th);

bool probeEval(int *eval, Thread *th);
void recordEval(int eval, Thread *th);

int hashfull();

#endif
