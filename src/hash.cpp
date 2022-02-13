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


bool probeHash(int *eval_static, int *ttDepth, int *ttValue, int *ttBound, u32 *ttMove, Thread *th) {
    
    HASHE *phashe = &hashTable[th->hashKey % HASH_TABLE_SIZE];
    
    u32 dataKey = phashe->bestMove ^ phashe->value ^ phashe->depth ^ phashe->flags ^ phashe->eval_static;
    
    bool isValidHash = (phashe->key ^ dataKey) == th->hashKey; 

    if (isValidHash) {

        *ttDepth = phashe->depth;
        *ttValue = phashe->value;
        *ttBound = phashe->flags; 

        *eval_static = phashe->eval_static;
		*ttMove = phashe->bestMove;
    }

    return isValidHash;
}


void recordHash(u32 bestMove, int depth, int value, int hashf, int eval_static, Thread *th) {
   
    
    HASHE *phashe = &hashTable[th->hashKey % HASH_TABLE_SIZE];

    
    u32 dataKey = phashe->bestMove ^ phashe->value ^ phashe->depth ^ phashe->flags ^ phashe->eval_static;
    
    
    bool isValidHash = (phashe->key ^ dataKey) == th->hashKey; 

    if (isValidHash && depth < phashe->depth) { // Check whether to overwrite previous information

        return;           
    }

    // Overwrite the hash information 

    dataKey = bestMove ^ depth ^ value ^ hashf ^ eval_static;
   
    phashe->key = th->hashKey ^ dataKey;
    phashe->value = value;
    phashe->flags = hashf;
    phashe->depth = depth;
    phashe->bestMove = bestMove;
    phashe->eval_static = eval_static;
}

int hashfull() {

    int count = 0;
    for (unsigned int i = 0; i < 1000; i++) {

        if (hashTable[i].key != 0)
            count++;
    }
    
    return count;
}
