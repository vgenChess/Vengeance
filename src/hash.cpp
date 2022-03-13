#include "hash.h"
#include "constants.h"

bool probePawnHash(int *score, Thread *th) {

    U64 key = th->pawnsHashKey;

    PAWNS_HASH *ptrhash = &th->pawnHashTable[key % U32_PAWN_HASH_TABLE_SIZE];
    
    *score = ptrhash->score;
    
    return ptrhash->key == key;
}


void recordPawnHash(int score, Thread *th) {
    
    U64 key = th->pawnsHashKey;

    PAWNS_HASH *ptrhash = &(th->pawnHashTable[key % U32_PAWN_HASH_TABLE_SIZE]);

    ptrhash->key = key;
    ptrhash->score = score;
}


bool probeEval(int *eval, Thread *th) {
    
    EVAL_HASH *ptrhash = &th->evalHashTable[th->hashKey % U32_EVAL_HASH_TABLE_SIZE];
    
    *eval = ptrhash->score;
    
    return ptrhash->key == th->hashKey;
}


void recordEval(int eval, Thread *th) {
   
    EVAL_HASH *ptrhash = &th->evalHashTable[th->hashKey % U32_EVAL_HASH_TABLE_SIZE];

    ptrhash->key = th->hashKey;
    ptrhash->score = eval;
}


int hashfull() {

    int count = 0;
    for (unsigned int i = 0; i < 1000; i++) {

        if (hashTable[i].key != 0)
            count++;
    }
    
    return count;
}

