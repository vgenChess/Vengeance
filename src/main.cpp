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
#include "init.h"
#include "perft.h"
#include "evaluate.h"
#include "search.h"
#include "uci.h"
#include "fen.h"
#include "functions.h"
#include "ucireport.h"

static void runBenchmark(int argc, char **argv);

int main(int argc, char **argv) {
    
    init_index_bb();
    init_king_attacks();
    init_knight_attacks();
    init_magic_moves();
    initCastleMaskAndFlags();
    initZobristKeys();
    initPawnZobristKeys();  // should be called after initialising main zobrist keys
	init_inbetween_bb(); 
    initPSQT();
    initHashTable(16);      // default hash size = 16 megabytes
    Threads.set(1);         // default threads size = 1
    omp_set_num_threads(omp_get_max_threads()); // for tuning


    timeSet = false;
    stopped = false;
    
    bool isBenchmark = argc > 1 && strcmp(argv[1], "bench") == 0;

    if (isBenchmark) {
    
        runBenchmark(argc, argv);
    } else {
    
        UciLoop();
    }
    

    if (sizeof(hashTable) > 0) {

        delete[] hashTable; 
    }

	return 0;
}


// TODO check logic and refactor
static void runBenchmark(int argc, char **argv) {

    printf("OVERALL: %47d nodes %12d nps\n", 100, 3000000);

    return;

    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    int scores[256];
    double times[256];
    uint64_t nodes[256], totalNodes = 0;
    u32 bestMoves[256];
    
    int depth     = 13;
    MAX_DEPTH     = depth;


    std::chrono::steady_clock::time_point 
	start_time = std::chrono::steady_clock::now(), time;    

    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        time = std::chrono::steady_clock::now();
        
        initThread.clear();
        initThread.side = parseFen(Benchmarks[i], &initThread);

        Threads.start_thinking();

        while (!Threads.stop)
        {} // Busy wait for a stop or a ponder reset


        bestMoves[i] = Threads.main()->pvLine[Threads.main()->completedDepth].line[0];
        scores[i] = Threads.main()->pvLine[Threads.main()->completedDepth].score;

        times[i] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - time).count();
       
        nodes[i] = Threads.getTotalNodes();

        Threads.clear();
        clearHashTable(); // Reset TT between searches
    }

    printf("\n===============================================================================\n");

    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        // Log all collected information for the current position
        printf("[# %2d] %5d cp  Best:%6s %12d nodes %12d nps\n", i + 1, scores[i],
            getMoveNotation(bestMoves[i]).c_str(), (int)nodes[i], (int)(1000.0f * nodes[i] / (times[i] + 1)));
    }

    printf("===============================================================================\n");

    double totalTime = 0;
    // Report the overall statistics
    for (int i = 0; strcmp(Benchmarks[i], ""); i++) { 
	    
	    totalNodes += nodes[i];
	    totalTime += times[i];
    }

    //std::cout<<"nodes " << totalNodes << " nps " << totalNodes / time_elapsed << std::endl;

	printf("OVERALL: %47d nodes %12d nps\n",  100,  (int)(1000.0f * totalNodes / (totalTime + 1)));
 //   printf("OVERALL: %47d nodes %12d nps\n", (int)totalNodes, (int)(1000.0f * totalNodes / (totalTime + 1)));
}

 

