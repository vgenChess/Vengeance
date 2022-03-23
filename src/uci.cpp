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
#include "uci.h"
#include "utility.h"
#include "make_unmake.h"
#include "fen.h"
#include "movegen.h"
#include "search.h"
#include "perft.h"
#include "evaluate.h"
#include "thread.h"
#include "tuner.h"
#include "ucireport.h"
#include "functions.h"
#include "see.h"
#include "time.h"

#define NAME "V0.9"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

bool quit;

int option_hash_size;
int MOVE_OVERHEAD = 300;

U8 HashManager::age;

void setOption(std::string &line) {

    if (line.rfind("setoption name Hash value", 0) == 0) {

        int hash_size = std::stoi(line.substr(26, std::string::npos));
    
        HashManager::initHashTable(hash_size);
     
        std::cout << "info string set Hash to " << hash_size << "MB\n";
    } else if (line.rfind("setoption name Threads value", 0) == 0) {

        option_thread_count = std::stoi(line.substr(29, std::string::npos));

        searchThreads.createThreadPool(option_thread_count);

        std::cout<<"info string set Threads to " << option_thread_count << "\n";
    } 
}

void UciLoop() {

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
            
            HashManager::age = 0;
            HashManager::clearHashTable();

            for (Thread *thread : searchThreads.getSearchThreads())
            {
                thread->clear();
            }
            
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
            if (token == "startpos") 
            {
                fen = START_FEN;
                is>>token;
            } else if (token == "fen") 
            {
                while (is >> token && token != "moves")
                    fen += token + " ";
            }

            U8 sideToMove = parseFen(fen, &initThread);

            std::vector<Move> moves;

            while (is>>token) 
            {
                moves.clear();

                genMoves(sideToMove, 0, moves, &initThread);
                
                for (Move m : moves) 
                {
                    if (getMoveNotation(m.move) == token) 
                    {
                        make_move(0, m.move, &initThread);
                        
                        initThread.moves_history_counter++;
                        initThread.movesHistory[initThread.moves_history_counter].hashKey = initThread.hashKey;

                        sideToMove ^= 1;
                        
                        break;
                    }
                }
            }

            initThread.side = sideToMove == WHITE ? WHITE : BLACK;   
        } 
        else if (token == "isready") 
        {
            std::cout << "readyok\n";
        } 
        else if (token == "go") 
        {
            TimeManager::sTimeManager.setStartTime(std::chrono::steady_clock::now());

            TimeManager::sTimeManager.updateTimeSet(false);

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
                
                TimeManager::sTimeManager.updateTimeSet(true);
                TimeManager::sTimeManager.updateTimePerMove(moveTime);
                
                TimeManager::sTimeManager.setStopTime(TimeManager::sTimeManager.getStartTime() 
                    + std::chrono::milliseconds(moveTime));
            } else {
                
                TimeManager::sTimeManager.updateTimeSet(time != -1 ? true : false);

                if (time != -1) {
                
                    if (movesToGo == -1) {

                        // TODO redo logic  
                        const auto total = (int)fmax(1, time + 50 * inc - MOVE_OVERHEAD);

                        TimeManager::sTimeManager.updateTimePerMove((int)fmin(time * 0.33, total / 20.0));
                    } else {
                    
                        const auto total = (int)fmax(1, time + movesToGo * inc - MOVE_OVERHEAD);
                        
                        TimeManager::sTimeManager.updateTimePerMove(total / movesToGo);
                    }
                    
                    TimeManager::sTimeManager.setStopTime(
                        TimeManager::sTimeManager.getStartTime() + std::chrono::milliseconds((int)(time * 0.75)));           
                } else {

                    TimeManager::sTimeManager.updateTimeSet(false);
                }
            }

            searchThreads.search<true>();
        } 

        else if (token == "stop") {

            SearchThread::stopSearch = true;
        }

        else if (token == "quit") {
        
            SearchThread::stopSearch = true;  
        
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
