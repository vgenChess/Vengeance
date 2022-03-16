#ifndef hash_h
#define hash_h

#include "types.h"
#include "thread.h"
#include "structs.h"

__always_inline bool probeHash(HASHE *tt, Thread *th) {
    
    if (tt == NULL) 
    {
        return false;
    }
    
    U32 dataKey = tt->bestMove ^ tt->value ^ tt->depth ^ tt->flags ^ tt->sEval;
    
    return (tt->key ^ dataKey) == th->hashKey; 
}

__always_inline void recordHash(U32 bestMove, int depth, int value, int hashf, int sEval, Thread *th) 
{
	HASHE *phashe = &hashTable[th->hashKey % HASH_TABLE_SIZE];
    
    U32 dataKey = phashe->bestMove ^ phashe->value ^ phashe->depth ^ phashe->flags ^ phashe->sEval;
    
    bool isValidHash = (phashe->key ^ dataKey) == th->hashKey; 

    if (isValidHash && depth < phashe->depth) 
    { // Check whether to overwrite previous information

        return;           
    }

    // Overwrite the hash information 

    dataKey = bestMove ^ depth ^ value ^ hashf ^ sEval;
   
    phashe->key = th->hashKey ^ dataKey;
    phashe->value = value;
    phashe->flags = hashf;
    phashe->depth = depth;
    phashe->bestMove = bestMove;
    phashe->sEval = sEval;
}

bool probePawnHash(int *score, Thread *th);
void recordPawnHash(int score, Thread *th);

bool probeEval(int *eval, Thread *th);
void recordEval(int eval, Thread *th);

int hashfull();

#endif
