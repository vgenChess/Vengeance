//
//  uci.c
//  Vgen
//
//  Created by Amar Thapa on 25/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
#include <cmath>

#include "types.h"
#include "globals.h"
#include "uci.h"
#include "utility.h"
#include "make_unmake.h"
#include "fen.h"
#include "movegen.h"
#include "search.h"
#include "perft.h"
#include "hash.h"
#include "evaluate.h"
#include "thread.h"
#include "tuner.h"
#include "ucireport.h"
#include "functions.h"
#include "see.h"

#define NAME "V0.9"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

bool quit;
bool NNUE = false;

int option_hash_size;

int MOVE_OVERHEAD = 300;

void setOption(std::string &line) {

    if (line.rfind("setoption name Hash value", 0) == 0) {

        int hash_size = std::stoi(line.substr(26, std::string::npos));
    
        initHashTable(hash_size);
     
        std::cout << "info string set Hash to " << hash_size << "MB\n";
    } else if (line.rfind("setoption name Threads value", 0) == 0) {

        option_thread_count = std::stoi(line.substr(29, std::string::npos));

        Threads.createThreadPool(option_thread_count);

        std::cout<<"info string set Threads to " << option_thread_count << "\n";
    } 
}

void UciLoop() {

	initThread.isInit = true;

    initThread.clear();
    initThread.init();
    parseFen(START_FEN, &initThread);
    
    quit = false;
    
    std::string cmd, token;

    do {

        if (!getline(std::cin, cmd)) // Block here waiting for input or EOF 
            cmd = "quit";

        std::istringstream is(cmd);

        token.clear(); // Avoid a stale if getline() returns empty or blank line
        is >> std::skipws >> token;


        if (token == "uci") {
         
            std::cout << "id name Vengeance " << NAME << "\n";
            std::cout << "id author Amar Thapa\n";
            std::cout << "option name Hash type spin default 16 min 2 max 131072\n";
            std::cout << "option name Threads type spin default 1 min 1 max 2048\n";
            std::cout << "uciok\n";
        } else if (token == "setoption") {
            
            setOption(cmd);
        } else if (token == "ucinewgame") {

            clearHashTable();

            for (Thread *thread : Threads.getSearchThreads())
                thread->clear();

            initThread.clear();
        } else if (token == "position") {
            
            initThread.clear();
            initThread.init();

            initThread.moves_history_counter = 0;
            initThread.movesHistory[0].hashKey = initThread.hashKey;
            initThread.movesHistory[0].fiftyMovesCounter = 0;

            is>>token;

            std::string fen;

            //TODO refactor logic
            if (token == "startpos") {

                fen = START_FEN;
                is>>token;
            } else if (token == "fen") {

                while (is >> token && token != "moves")
                    fen += token + " ";
            }

            U8 sideToMove = parseFen(fen, &initThread);

            std::vector<Move> moves;

            while (is>>token) {

                moves.clear();

                genMoves(sideToMove, 0, moves, &initThread);
                
                for (Move m : moves) {

                    if (getMoveNotation(m.move) == token) {

                        make_move(0, m.move, &initThread);
                        
                        initThread.moves_history_counter++;
                        initThread.movesHistory[initThread.moves_history_counter].hashKey = initThread.hashKey;

                        sideToMove ^= 1;
                        
                        break;
                    }
                }
            }

            initThread.side = sideToMove == WHITE ? WHITE : BLACK;
            
        } else if (token == "isready") {

            std::cout << "readyok\n";
        } else if (token == "go") {

            startTime = std::chrono::steady_clock::now();

            timeSet = false;

            int32_t time = -1, moveTime = -1, nodes = 0, inc = 0, movesToGo = -1, depthCurrent = 0;

            while (is >> token) {

                     if (token == "wtime" && initThread.side == WHITE)     is >> time;
                else if (token == "btime" && initThread.side == BLACK)     is >> time;
                else if (token == "winc"  && initThread.side == WHITE)     is >> inc;
                else if (token == "binc"  && initThread.side == BLACK)     is >> inc;
                else if (token == "movestogo")  is >> movesToGo;
                else if (token == "depth")      is >> depthCurrent;
                else if (token == "nodes")      is >> nodes;
                else if (token == "movetime")   is >> moveTime;                    
            }
            

            if (moveTime != -1) {
                
                timeSet = true;

                timePerMove = moveTime;

                stopTime = startTime + std::chrono::milliseconds(moveTime);
            } else {

                if (time != -1) {
                
                    timeSet = true;

                    if (movesToGo == -1) {

                        int total = (int)fmax(1, time + 50 * inc - MOVE_OVERHEAD);

                        timePerMove = (int)fmin(time * 0.33, total / 20.0);
                    } else {
                    
                        int total = (int)fmax(1, time + movesToGo * inc - MOVE_OVERHEAD);

                        timePerMove = total / movesToGo;
                    }

                    stopTime = startTime + std::chrono::milliseconds((int)fmin(time * 0.75, timePerMove * 5.5));           
                } else {

                    timeSet = false;
                }
            }

            Threads.start_thinking();
        } 

        else if (token == "stop") {

            SearchThread::stop = true;
        }

        else if (token == "quit") {
        
            SearchThread::stop = true;  
        
            break;
        } 





        // Debugging commands

        else if (token == "perft") {
            
            std::thread t4(startPerft, initThread.side, 10, &initThread);

            t4.join();
        } 

        else if (token == "divide") {

            int d = 0;
            
            while (is >> token)
             if (token == "divide")     is >> d;

            if (d <= 0) {

                printf("Depth should be greater than 0\n");
            } else {

                std::thread t5(divide, d, initThread.side, &initThread);

                t5.join();
            }
        } 

        else if (token == "evaluate") {
            
            print_board(initThread.occupied, &initThread);
            printf("\n\n");

            int scoreWhite = fullEval(WHITE, &initThread);
            
            printf("White score = %d\n", scoreWhite);
            
            printf("\n");

            int scoreBlack = fullEval(BLACK, &initThread);

            printf("Black score = %d\n", scoreBlack);
        } 

        else if (token == "tune") {
            
            #if defined(TUNE)
                std::cout<<"starting tuner..."<<std::endl; 
                startTuner();
            #else
                std::cout<<"Not a tuning build. TUNE not set." << std::endl;
            #endif
        } 

        else if (token == "see") {

            int sq;
            char ch;

            std::cout << "Enter piece ";
            std::cin >> ch;

            std::cout << "Enter destination square ";
            std::cin >> sq;

            std::cout<<"\n";

            debugSEE(ch, sq);
        }
    } while(true);
}
