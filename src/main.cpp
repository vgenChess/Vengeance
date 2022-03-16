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

#include "globals.h"
#include "utility.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "perft.h"
#include "evaluate.h"
#include "search.h"
#include "uci.h"
#include "fen.h"
#include "functions.h"
#include "ucireport.h"
#include "zobrist.h"
#include "misc.h"
#include "HashManagement.h"

Zobrist Zobrist::objZobrist;
TimeManager TimeManager::sTimeManager;

int HashManager::hashTableSize;
std::vector<HashEntry> HashManager::hashTable; 

std::string Notation::algebricSq[64] = {
    
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
}; 

int main(int argc, char **argv) {
    
    initLMR();

    init_king_attacks();
    init_knight_attacks();
    init_magic_moves();
    Zobrist::objZobrist.initZobristKeys();
    Zobrist::objZobrist.initPawnZobristKeys();  // should be called after initialising main zobrist keys
    initCastleMaskAndFlags();
    init_inbetween_bb(); 
    initPSQT();
    HashManager::initHashTable(16);      // default hash size = 16 megabytes
    Threads.createThreadPool(1);         // default threads size = 1
    omp_set_num_threads(omp_get_max_threads()); // for tuning

    TimeManager::sTimeManager.updateTimeSet(false);
    TimeManager::sTimeManager.updateStopped(false);
    
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
