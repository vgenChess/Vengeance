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
#include "cerebrum.h"
#include "ucireport.h"

#define NAME "V0.9"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

bool quit;
bool NNUE = false;

int option_hash_size;

int totalTimeLeft;

void setOption(std::string &line) {

    if (line.rfind("setoption name Hash value", 0) == 0) {

        int hash_size = std::stoi(line.substr(26, std::string::npos));
    
        initHashTable(hash_size);
     
        std::cout << "info string set Hash to " << hash_size << "MB\n";
    } else if (line.rfind("setoption name Threads value", 0) == 0) {

        option_thread_count = std::stoi(line.substr(29, std::string::npos));

        Threads.set(option_thread_count);

        std::cout<<"info string set Threads to " << option_thread_count << "\n";
    } 
}

void UciLoop() {

	initThread.isInit = true;

    initThread.clear();
    initThread.initMembers();
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
        }
        
        else if (token == "setoption") {
            
            setOption(cmd);
        } 

        else if (token == "ucinewgame") {

            clearHashTable();

            for (Thread *thread : Threads)
                thread->clear();

            initThread.clear();
        } 

        else if (token == "position") {
            
            initThread.clear();
            initThread.initMembers();

            initThread.moves_history_counter = 0;
            initThread.movesHistory[0].hashKey = initThread.hashKey;
            initThread.movesHistory[0].fiftyMovesCounter = 0;

            u8 sideToMove = WHITE;

            is>>token;

            if (token == "startpos") {

                is>>token;

                parseFen(START_FEN, &initThread);

                std::vector<Move> moves;

                while (is>>token) {

                    moves.clear();
                    genMoves(0, moves, sideToMove, &initThread);
                    
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
            } else if (token == "fen") {
                
                const std::string str_fen = "fen";
                std::string tempStr;
                
                int pos = cmd.find(str_fen);

                tempStr = cmd.substr(pos + 1 + str_fen.length());

                sideToMove = parseFen(tempStr, &initThread);
            }

            initThread.side = sideToMove;
        } 

        else if (token == "isready") {

            std::cout << "readyok\n";
        } 

        else if (token == "go") {

            startTime = std::chrono::steady_clock::now();

            timeSet = false;

            uint32_t wtime = 0, btime = 0, movetime = 0, time = 0, nodes = 0;
            uint16_t winc = 0, binc = 0, movestogo = 0, depthCurrent = 0, inc = 0;

            while (is >> token) {

                     if (token == "wtime")     is >> wtime;
                else if (token == "btime")     is >> btime;
                else if (token == "winc")      is >> winc;
                else if (token == "binc")      is >> binc;
                else if (token == "movestogo") is >> movestogo;
                else if (token == "depth")     is >> depthCurrent;
                else if (token == "nodes")     is >> nodes;
                else if (token == "movetime")  {
                    
                    is >> movetime;      

                    timeSet = true;

                    time = movetime;
                    movestogo = 1;                   
                }
            }
                
            if (wtime > 0 || btime > 0) {

                timeSet = true;

                time = initThread.side ? btime : wtime;
                inc = initThread.side ? binc : winc;         
            }   


            if (timeSet) {                

                // stopTime = startTime + std::chrono::milliseconds((int)(time * 0.75));

                // timePerMove = (time / (movestogo + 2)) + inc; 
        
                int total = fmax(1, time + movestogo * inc);

                timePerMove = fmin(time * 0.9, (0.9 * total) / fmax(1, movestogo / 2.5));

                totalTimeLeft = time;    
            
                stopTime = startTime + std::chrono::milliseconds((int)fmin(time * 0.75, timePerMove * 5.5));
            } 
          
            // if (depthCurrent == -1) {
            
                // const std::string str_fen = "fen";
                // std::string tempStr;
                
                // int pos = str.find(str_fen);

                // tempStr = str.substr(pos + 1 + str_fen.length());

                // std::cout << tempStr << "\n";

                // depthCurrent = MAX_DEPTH;
            // }
            
            // depth = depthCurrent;


            Threads.start_thinking();
        } 

        else if (token == "stop") {

            Threads.stop = true;
        }

        else if (token == "quit") {
        
            Threads.stop = true;  
        
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
            
           TUNE = 1;
            
           startTuner();
        } 

        else if (token == "writeEval") {

            writeEvalToFile();
        } 

        else if (token == "getEval") {

            getEval();
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
