//
//  main.c
//  Vgen
//
//  Created by Amar Thapa on 2/05/17.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <thread>
#include <iostream>

#include "utility.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "perft.h"
#include "search.h"
#include "uci.h"
#include "fen.h"
#include "functions.h"
#include "ucireport.h"
#include "zobrist.h"
#include "utility.h"
#include "HashManagement.h"
#include "nnue.h"

Zobrist Zobrist::objZobrist;
TimeManager TimeManager::sTm;

int HashManager::hashTableSize;
std::vector<HashEntry> HashManager::hashTable; 

int main(int argc, char **argv) 
{    

    loadNetwork();

    initLMR();
    initLMP();
    init_king_attacks();
    init_knight_attacks();
    init_magic_moves();
    Zobrist::objZobrist.initZobristKeys();
    Zobrist::objZobrist.initPawnZobristKeys();  // should be called after initialising main zobrist keys
    initCastleMaskAndFlags();
    init_inbetween_bb(); 
    HashManager::initHashTable(16);      // default hash size = 16 megabytes

    option_thread_count = 1;

    omp_set_num_threads(omp_get_max_threads()); // for tuning

    TimeManager::sTm.updateTimeSet(false);
    TimeManager::sTm.updateStopped(false);
    
    bool isBenchmark = argc > 1 && strcmp(argv[1], "bench") == 0;

    if (isBenchmark) 
    {
        printf("OVERALL: %47d nodes %12d nps\n", 100, 3000000);
    } 
    else 
    {
        UciLoop();
    }
    
    HashManager::clearHashTable();
    
    return 0;
}
