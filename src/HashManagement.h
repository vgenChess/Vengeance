#ifndef HASH_MANAGEMENT_H
#define HASH_MANAGEMENT_H

#include <vector>
#include <string.h>

#include "structs.h"
#include "constants.h"
#include "namespaces.h"

class HashManager 
{
    
public:
    
    inline static void initHashTable(int size) 
    {
        delete[] tt::hashTable;

        tt::size = (size * 1024 * 1024) / sizeof(HashEntry);

        tt::hashTable = new HashEntry[tt::size];
    }
    
    inline static void clearHashTable()
    {
        std::memset(tt::hashTable, 0, sizeof(HashEntry) * tt::size);
    }
    
    inline HashEntry* getHashEntry(const U64 key)
    {
        return &tt::hashTable[key % tt::size];
    }
    
    inline bool probeHash(HashEntry* entry, U64 key) 
    {
        if (entry == nullptr) 
        {
            return false;
        }
        
        U32 dataKey = entry->bestMove ^ entry->value ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        return (entry->key ^ dataKey) == key; 
    }
    
    inline void recordHash(U64 key, U32 bestMove, int depth, int value, int hashf, int sEval) 
    {
        auto entry = &tt::hashTable[key % tt::size];
        
        U32 dataKey = entry->bestMove ^ entry->value ^ entry->depth ^ entry->flags ^ entry->sEval;
        
        const auto isValidHash = (entry->key ^ dataKey) == key; 
        
        if ((tt::age - entry->age) < HASH_AGE && isValidHash && depth < entry->depth)
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
        entry->age = tt::age;
    }
    
    
    // Check for valid entries of the first 1000 entries of hash table
    
    inline int hashfull() 
    {
        int count = 0;
        for (unsigned int i = 0; i < 1000; i++) 
        {
            if (tt::hashTable[i].key != 0)
                count++;
        }
        
        return count;
    }

    inline static void deleteHashTable() {

        delete[] tt::hashTable;
    }
};

#endif
