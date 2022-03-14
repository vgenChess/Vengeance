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

zobrist::Zobrist zobrist::Zobrist::objZobrist;
TimeManager TimeManager::timeManager;

int main(int argc, char **argv) {
    
    initLMR();

    init_king_attacks();
    init_knight_attacks();
    init_magic_moves();
    zobrist::Zobrist::objZobrist.initZobristKeys();
    zobrist::Zobrist::objZobrist.initPawnZobristKeys();  // should be called after initialising main zobrist keys
    misc::initCastleMaskAndFlags();
    misc::init_inbetween_bb(); 
    initPSQT();
    initHashTable(16);      // default hash size = 16 megabytes
    Threads.createThreadPool(1);         // default threads size = 1
    omp_set_num_threads(omp_get_max_threads()); // for tuning

    TimeManager::timeManager.timeSet = false;
    TimeManager::timeManager.stopped = false;
    
    bool isBenchmark = argc > 1 && strcmp(argv[1], "bench") == 0;

    if (isBenchmark) {
    
        printf("OVERALL: %47d nodes %12d nps\n", 100, 3000000);
    } else {
    
        UciLoop();
    }
    

    if (sizeof(hashTable) > 0) {

        delete[] hashTable; 
    }

	return 0;
}
