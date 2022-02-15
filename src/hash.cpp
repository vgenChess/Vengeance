#include "hash.h"

bool probePawnHash(int *score, Thread *th) {

    u64 key = th->pawnsHashKey;

    PAWNS_HASH *ptrhash = &th->pawnHashTable[key % PAWN_HASH_TABLE_SIZE];
    
    *score = ptrhash->score;
    
    return ptrhash->key == key;
}


void recordPawnHash(int score, Thread *th) {
    
    u64 key = th->pawnsHashKey;

    PAWNS_HASH *ptrhash = &(th->pawnHashTable[key % PAWN_HASH_TABLE_SIZE]);

    ptrhash->key = key;
    ptrhash->score = score;
}


bool probeEval(int *eval, Thread *th) {
    
    EVAL_HASH *ptrhash = &th->evalHashTable[th->hashKey % EVAL_HASH_TABLE_SIZE];
    
    *eval = ptrhash->score;
    
    return ptrhash->key == th->hashKey;
}


void recordEval(int eval, Thread *th) {
   
    EVAL_HASH *ptrhash = &th->evalHashTable[th->hashKey % EVAL_HASH_TABLE_SIZE];

    ptrhash->key = th->hashKey;
    ptrhash->score = eval;
}


bool probeHash(int *sEval, int *ttDepth, int *ttValue, int *ttBound, u32 *ttMove, Thread *th) {
    
    HASHE *phashe = &hashTable[th->hashKey % HASH_TABLE_SIZE];
    
    u32 dataKey = phashe->bestMove ^ phashe->value ^ phashe->depth ^ phashe->flags ^ phashe->sEval;
    
    bool isValidHash = (phashe->key ^ dataKey) == th->hashKey; 

    if (isValidHash) {

        *ttDepth = phashe->depth;
        *ttValue = phashe->value;
        *ttBound = phashe->flags; 

        *sEval = phashe->sEval;
		*ttMove = phashe->bestMove;
    }

    return isValidHash;
}

bool probeHashNew(HASHE *tt, Thread *th) {
    
    u32 dataKey = tt->bestMove ^ tt->value ^ tt->depth ^ tt->flags ^ tt->sEval;
    
    return (tt->key ^ dataKey) == th->hashKey; 
}


void recordHash(u32 bestMove, int depth, int value, int hashf, int sEval, Thread *th) {
   
    
    HASHE *phashe = &hashTable[th->hashKey % HASH_TABLE_SIZE];

    
    u32 dataKey = phashe->bestMove ^ phashe->value ^ phashe->depth ^ phashe->flags ^ phashe->sEval;
    
    
    bool isValidHash = (phashe->key ^ dataKey) == th->hashKey; 

    if (isValidHash && depth < phashe->depth) { // Check whether to overwrite previous information

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

int hashfull() {

    int count = 0;
    for (unsigned int i = 0; i < 1000; i++) {

        if (hashTable[i].key != 0)
            count++;
    }
    
    return count;
}
