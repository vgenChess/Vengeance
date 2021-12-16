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
#include "NnueEval.h"
#include "cerebrum.h"

NN_Network nn;

static void runBenchmark(int argc, char **argv);

int main(int argc, char **argv) {
    
    // init_piece_bb();     //TODO cleanup
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
    
    // initNNUE("nn.bin");
    
    nn_load(&nn, NN_FILE) ;

    int nProcessors = omp_get_max_threads();
    omp_set_num_threads(nProcessors);


    option_thread_count = 1;
    

    // initThreads(); 
  
    timeSet = false;
    stopped = false;
    
    
    bool bench = argc > 1 && strcmp(argv[1], "bench") == 0;

    if (bench) {

        Threads.set(1);
    
        runBenchmark(argc, argv);
    } else {

        Threads.set(option_thread_count);
    
        UciLoop();
    }
    
    if (sizeof(hashTable) > 0) {

        delete[] hashTable; 
    }


	return 0;
}

    
static void runBenchmark(int argc, char **argv) {
      
    static const char *Benchmarks[] = {
        #include "bench.csv"
        ""
    };

    int scores[256];
    double times[256];
    uint64_t nodes[256], totalNodes = 0;
    u32 bestMoves[256];
    
    int depth     = 10;
    MAX_DEPTH     = depth;


    std::chrono::steady_clock::time_point 
	start_time = std::chrono::steady_clock::now(), time;    

    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        time = std::chrono::steady_clock::now();

        std::string posStr = "position fen " + std::string(Benchmarks[i]);

        initThread.clear();
        initThread.side = parsePosition(posStr, &initThread);

        Threads.start_thinking();

        while (!Threads.stop)
        {} // Busy wait for a stop or a ponder reset


        bestMoves[i] = Threads.get_best_thread()->pvLine[Threads.get_best_thread()->depth].line[0];
        scores[i] = Threads.get_best_thread()->pvLine[Threads.get_best_thread()->depth].score;

        times[i] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - time).count();
       
        nodes[i] = Threads.getTotalNodes();

        Threads.clear();
        clearHashTable(); // Reset TT between searches
    }

    printf("\n===============================================================================\n");
    u32 bestMove;
    for (int i = 0; strcmp(Benchmarks[i], ""); i++) {

        bestMove = bestMoves[i];

        // Convert moves to typical UCI notation
        char str[10];
        str[0] = '\0';

        strcat(str, algebricPos(from_sq(bestMove)));
        strcat(str, algebricPos(to_sq(bestMove)));

        if (move_type(bestMove) == MOVE_PROMOTION) {

            switch (promType(bestMove)) {

                case PROMOTE_TO_ROOK:
                    strcat(str, "r");
                    // sideToMove ? strcat(str, "r") : strcat(str, "R");
                    break;
                case PROMOTE_TO_BISHOP:
                    strcat(str, "b");
                    // sideToMove ? strcat(str, "b") : strcat(str, "B");
                    break;
                case PROMOTE_TO_KNIGHT:
                    strcat(str, "n");
                    // sideToMove ? strcat(str, "n") : strcat(str, "N");
                    break;
                default:
                    strcat(str, "q");
                    // sideToMove ? strcat(str, "q") : strcat(str, "Q");
                    break;
            }
        }

        // Log all collected information for the current position
        printf("[# %2d] %5d cp  Best:%6s %12d nodes %12d nps\n", i + 1, scores[i],
            str, (int)nodes[i], (int)(1000.0f * nodes[i] / (times[i] + 1)));
    }

    printf("===============================================================================\n");

    double totalTime = 0;
    // Report the overall statistics
    for (int i = 0; strcmp(Benchmarks[i], ""); i++) { 
	    
	    totalNodes += nodes[i];
	    totalTime += times[i];
    }

    //std::cout<<"nodes " << totalNodes << " nps " << totalNodes / time_elapsed << std::endl;


    printf("OVERALL: %47d nodes %12d nps\n", (int)totalNodes, (int)(1000.0f * totalNodes / (totalTime + 1)));
}

 

