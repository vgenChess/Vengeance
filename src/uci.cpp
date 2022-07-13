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
#include "thread.h"
#include "ucireport.h"
#include "functions.h"
#include "see.h"
#include "time.h"
#include "nnue.h"

#define NAME "V0.9"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

bool quit;

int option_hash_size;
int MOVE_OVERHEAD = 300;

std::thread searchThread;

void setOption(std::string &line) {

    if (line.rfind("setoption name Hash value", 0) == 0) {

        int hash_size = std::stoi(line.substr(26, std::string::npos));
    
        HashManager::initHashTable(hash_size);
     
        std::cout << "info string set Hash to " << hash_size << "MB\n";
    } else if (line.rfind("setoption name Threads value", 0) == 0) {

        option_thread_count = std::stoi(line.substr(29, std::string::npos));

        std::cout<<"info string set Threads to " << option_thread_count << "\n";
    } 
}

void UciLoop() {

    GameInfo *gameInfo = new GameInfo();

    gameInfo->clear();
    gameInfo->init();

    parseFen(START_FEN, gameInfo);

    quit = false;
    
    std::string cmd, token;


    std::vector<std::thread> threads;

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

//             for ( GameInfo *thread : searchThreads.getSearchThreads())
//             {
//                 thread->clear();
//             }
            
            gameInfo->clear();
        } else if (token == "position") {
            
            gameInfo->clear();
            gameInfo->init();

            gameInfo->moves_history_counter = 0;
            gameInfo->movesHistory[0].hashKey = gameInfo->hashKey;
            gameInfo->movesHistory[0].fiftyMovesCounter = 0;

            gameInfo->searching = false;

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

            U8 sideToMove = parseFen(fen, gameInfo);

            std::vector<Move> moves;

            while (is>>token) 
            {
                moves.clear();

                genMoves(sideToMove == WHITE ? WHITE : BLACK, 0, moves, gameInfo);
                
                for (Move m : moves) 
                {
                    if (getMoveNotation(m.move) == token) 
                    {
                        make_move(0, m.move, gameInfo);
                        
                        gameInfo->moves_history_counter++;
                        gameInfo->movesHistory[gameInfo->moves_history_counter].hashKey = gameInfo->hashKey;

                        sideToMove ^= 1;
                        
                        break;
                    }
                }
            }

            gameInfo->stm = sideToMove == WHITE ? WHITE : BLACK;
        } 
        else if (token == "isready") 
        {
            std::cout << "readyok\n";
        } 
        else if (token == "go") 
        {
            TimeManager::sTm.setStartTime(std::chrono::steady_clock::now());

            TimeManager::sTm.updateTimeSet(false);

            int32_t time = -1, moveTime = -1, nodes = 0, inc = 0, movesToGo = -1, depthCurrent = 0;

            while (is >> token) {

                     if (token == "wtime" && gameInfo->stm == WHITE)     is >> time;
                else if (token == "btime" && gameInfo->stm == BLACK)     is >> time;
                else if (token == "winc"  && gameInfo->stm == WHITE)     is >> inc;
                else if (token == "binc"  && gameInfo->stm == BLACK)     is >> inc;
                else if (token == "movestogo")  is >> movesToGo;
                else if (token == "depth")      is >> depthCurrent;
                else if (token == "nodes")      is >> nodes;
                else if (token == "movetime")   is >> moveTime;                    
            }
            

            if (moveTime != -1) {
                
                TimeManager::sTm.updateTimeSet(true);
                TimeManager::sTm.updateTimePerMove(moveTime);
                
                TimeManager::sTm.setStopTime(TimeManager::sTm.getStartTime()
                    + std::chrono::milliseconds(moveTime));
            } else {
                
                TimeManager::sTm.updateTimeSet(time != -1 ? true : false);

                if (time != -1) {

                    int timePerMove = 0;

                    if (movesToGo == -1) {

                        // TODO redo logic  
                        const auto total = (int)fmax(1, time + 50 * inc - MOVE_OVERHEAD);

                        timePerMove = (int)fmin(time * 0.33, total / 20.0);

                        TimeManager::sTm.updateTimePerMove(timePerMove);
                    } else {

                        const auto total = (int)std::max(1.0f, (float)(time + movesToGo * inc - MOVE_OVERHEAD));

                        timePerMove = total / movesToGo;

                        TimeManager::sTm.updateTimePerMove(timePerMove);
                    }

                    int maxTime = (int)std::max(1.0, time * 0.75);

                    TimeManager::sTm.setStopTime(
                        TimeManager::sTm.getStartTime() + std::chrono::milliseconds(maxTime));
                } else {

                    TimeManager::sTm.updateTimeSet(false);
                }
            }

            threads.emplace_back(startSearch, 0, gameInfo);
        }

        else if (token == "stop") {

            GameInfo::abortSearch = true;

            for (auto &th: threads) {

                if (th.joinable())
                    th.join();
            }

            threads.clear();
        }

        else if (token == "quit") {
        
            GameInfo::abortSearch = true;

            for (auto &th: threads) {

                if (th.joinable())
                    th.join();
            }

            threads.clear();

            break;
        } 





        // Debugging commands

        else if (token == "perft") {
            
            std::thread t4(startPerft, gameInfo->stm, 10, gameInfo);

            t4.join();
        } 

        else if (token == "divide") {

            int d = 0;
            
            while (is >> token)
             if (token == "divide")     is >> d;

            if (d <= 0) {

                printf("Depth should be greater than 0\n");
            } else {

                std::thread t5(divide, d, gameInfo->stm, gameInfo);

                t5.join();
            }
        } 

        else if (token == "evaluate") {
            
            print_board(gameInfo->occupied, gameInfo);
            printf("\n\n");

            int scoreWhite = predict(WHITE, gameInfo);
            
            printf("White score = %d\n", scoreWhite);
            
            printf("\n");

            int scoreBlack = predict(BLACK, gameInfo);

            printf("Black score = %d\n", scoreBlack);
        } 


        else if (token == "see") {

            int sq;
            char ch;

            std::cout << "Enter piece ";
            std::cin >> ch;

            std::cout << "Enter destination square ";
            std::cin >> sq;

            std::cout<<"\n";

            debugSEE(ch, sq, gameInfo);
        }
    } while(true);
}
