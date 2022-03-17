#ifndef HASH_MANAGEMENT_H
#define HASH_MANAGEMENT_H

#include <vector>
#include "structs.h"
#include "constants.h"

class HashManager {

private:
        
    static int hashTableSize;
    
    // Since hash table is shared between threads,
    // it is declared as static to make it accessible, 
    // for all threads.
    static std::vector<HashEntry> hashTable; 

public:
    
    __always_inline static void initHashTable(int size) 
    {
        hashTable.clear();
        hashTableSize = (size * 1024 * 1024) / sizeof(HashEntry);
        hashTable = std::vector<HashEntry>(hashTableSize);
    }
    
    __always_inline static void clearHashTable()
    {
        hashTable.clear();
    }
    
    __always_inline HashEntry* getHashEntry(const U64 key)
    {
        return &hashTable[key % hashTableSize];
    }
    __always_inline bool probeHash(HashEntry* entry, U64 key) 
    {
        if (entry == nullptr) 
        {
            return false;
        }
        
        U32 dataKey = entry->bestMove ^ entry->value 
                    ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        return (entry->key ^ dataKey) == key; 
    }
    
    __always_inline void recordHash(U64 key, U32 bestMove, int depth, int value, int hashf, int sEval) 
    {
        auto entry = &hashTable[key % hashTableSize];
        
        U32 dataKey = entry->bestMove ^ entry->value ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        const auto isValidHash = (entry->key ^ dataKey) == key; 
        
        if (isValidHash && depth < entry->depth) 
        { // Check whether to overwrite previous information
            return;           
        }
        
        // Overwrite the hash information 
        
        dataKey = bestMove ^ depth ^ value ^ hashf ^ sEval;
        
        entry->key = key ^ dataKey;
        entry->value = value;
        entry->flags = hashf;
        entry->depth = depth;
        entry->bestMove = bestMove;
        entry->sEval = sEval;
    }
    
    
    // Check for valid entries of the first 1000 entries of hash table
    
    __always_inline int hashfull() 
    {
        int count = 0;
        for (unsigned int i = 0; i < 1000; i++) 
        {
            if (hashTable[i].key != 0)
                count++;
        }
        
        return count;
    }
};


__always_inline int checkEvalHashTable(U64 key, EvalHashEntry* evalHashTable) {
    
    auto pEntry = &evalHashTable[key % U16_EVAL_HASH_TABLE_RECORDS];
    
    return pEntry->key == key ? pEntry->score : I32_UNKNOWN;
}

__always_inline void recordEvalHashTable(int eval, U64 key, EvalHashEntry* evalHashTable) {
    
    auto pEntry = &evalHashTable[key % U16_EVAL_HASH_TABLE_RECORDS];
    
    pEntry->key = key;
    pEntry->score = eval;
}

__always_inline bool probePawnHash(int *score, U64 key, PawnsHashEntry* pawnsHashTable) {
    
    auto pEntry = &pawnsHashTable[key % U16_PAWN_HASH_TABLE_RECORDS];
    
    *score = pEntry->score;
    
    return pEntry->key == key;
}

__always_inline void recordPawnHash(int score, U64 key, PawnsHashEntry* pawnsHashTable) {
    
    auto pEntry = &pawnsHashTable[key % U16_PAWN_HASH_TABLE_RECORDS];
    
    pEntry->key = key;
    pEntry->score = score;
}

#endif
