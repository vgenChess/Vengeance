#ifndef HASH_MANAGEMENT_H
#define HASH_MANAGEMENT_H

#include <vector>

class HashManager {

private:
    
    static int hashTableSize;
    static std::vector<HashEntry> hashTable; 
    
public:
    
    __always_inline static void clearHashTable()
    {
        hashTable.clear();
    }
    
    __always_inline static void initHashTable(int size) 
    {
        hashTable.clear();
        
        hashTableSize = (size * 1024 * 1024) / sizeof(HashEntry);
        
        hashTable = std::vector<HashEntry>(hashTableSize);
    }
    
    __always_inline HashEntry* getHashEntry(const U64 hashKey)
    {
        return &hashTable[hashKey % hashTableSize];
    }
    
    __always_inline bool probeHash(HashEntry* entry, U64 hashKey) 
    {
        if (entry == nullptr) 
        {
            return false;
        }
        
        U32 dataKey = entry->bestMove ^ entry->value 
                    ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        return (entry->key ^ dataKey) == hashKey; 
    }
    
    __always_inline void recordHash(U64 hashKey, U32 bestMove, int depth, int value, int hashf, int sEval) 
    {
        auto entry = &hashTable[hashKey % hashTableSize];
        
        U32 dataKey = entry->bestMove ^ entry->value ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        const auto isValidHash = (entry->key ^ dataKey) == hashKey; 
        
        if (isValidHash && depth < entry->depth) 
        { // Check whether to overwrite previous information
            return;           
        }
        
        // Overwrite the hash information 
        
        dataKey = bestMove ^ depth ^ value ^ hashf ^ sEval;
        
        entry->key = hashKey ^ dataKey;
        entry->value = value;
        entry->flags = hashf;
        entry->depth = depth;
        entry->bestMove = bestMove;
        entry->sEval = sEval;
    }
};


inline bool probePawnHash(int *score, U64 pawnsHashKey, PAWNS_HASH* pawnHashTable) {

    U64 key = pawnsHashKey;

    auto ptrhash = &pawnHashTable[key % U32_PAWN_HASH_TABLE_SIZE];
    
    *score = ptrhash->score;
    
    return ptrhash->key == key;
}

inline void recordPawnHash(int score, U64 pawnsHashKey, PAWNS_HASH* pawnHashTable) {
    
    PAWNS_HASH *ptrhash = &(pawnHashTable[pawnsHashKey % U32_PAWN_HASH_TABLE_SIZE]);

    ptrhash->key = pawnsHashKey;
    ptrhash->score = score;
}

inline bool probeEval(int *eval, U64 hashKey, EVAL_HASH* evalHashTable) {
    
    auto ptrhash = &evalHashTable[hashKey % U32_EVAL_HASH_TABLE_SIZE];
    
    *eval = ptrhash->score;
    
    return ptrhash->key == hashKey;
}

inline void recordEval(int eval, U64 hashKey, EVAL_HASH* evalHashTable) {
   
    EVAL_HASH *ptrhash = &evalHashTable[hashKey % U32_EVAL_HASH_TABLE_SIZE];

    ptrhash->key = hashKey;
    ptrhash->score = eval;
}

/*
int hashfull() {

    int count = 0;
    for (unsigned int i = 0; i < 1000; i++) {

        if (hashTable[i].key != 0)
            count++;
    }
    
    return count;
}*/

#endif
