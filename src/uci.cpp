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
#include "ucireport.h"
#include "functions.h"
#include "see.h"
#include "time.h"
#include "nnue.h"
#include "namespaces.h"

#define NAME "V0.9"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

bool quit;

int option_hash_size;
int MOVE_OVERHEAD = 300;

using namespace game;

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


    parseFen(START_FEN, initInfo);
    refresh_accumulator(initInfo, WHITE);
    refresh_accumulator(initInfo, BLACK);

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
            
            if (initInfo)
                delete initInfo;

            initInfo = new GameInfo();

            HashManager::clearHashTable();
        } else if (token == "position") {
            
            if (initInfo)
                delete initInfo;

            initInfo = new GameInfo();

            initInfo->moves_history_counter = 0;
            initInfo->movesHistory[0].hashKey = initInfo->hashKey;
            initInfo->movesHistory[0].fiftyMovesCounter = 0;

            is>>token;

            std::string fen;

            //TODO refactor logic
            if (token == "startpos") 
            {
                fen = START_FEN;
                is>>token;
            }
            else if (token == "fen")
            {
                while (is >> token && token != "moves")
                    fen += token + " ";
            }

            U8 sideToMove = parseFen(fen, initInfo);

            std::vector<Move> moves;

            while (is>>token) 
            {
                moves.clear();

                genMoves(sideToMove == WHITE ? WHITE : BLACK, 0, moves, initInfo);
                
                for (Move m : moves) 
                {
                    if (getMoveNotation(m.move) == token) 
                    {
                        make_move(0, m.move, initInfo);
                        
                        initInfo->moves_history_counter++;
                        initInfo->movesHistory[initInfo->moves_history_counter].hashKey = initInfo->hashKey;

                        sideToMove ^= 1;
                        
                        break;
                    }
                }
            }

            initInfo->stm = sideToMove == WHITE ? WHITE : BLACK;

            refresh_accumulator(initInfo, WHITE);
            refresh_accumulator(initInfo, BLACK);
        } 
        else if (token == "isready") 
        {
            std::cout << "readyok\n";
        } 
        else if (token == "go") 
        {
            tmg::timeManager.setStartTime(std::chrono::steady_clock::now());

            tmg::timeManager.updateTimeSet(false);

            uint64_t time = 0, moveTime = 0;
            int nodes = 0, inc = 0, movesToGo = -1, depthCurrent = 0;

            while (is >> token) {

                     if (token == "wtime" && initInfo->stm == WHITE)     is >> time;
                else if (token == "btime" && initInfo->stm == BLACK)     is >> time;
                else if (token == "winc"  && initInfo->stm == WHITE)     is >> inc;
                else if (token == "binc"  && initInfo->stm == BLACK)     is >> inc;
                else if (token == "movestogo")  is >> movesToGo;
                else if (token == "depth")      is >> depthCurrent;
                else if (token == "nodes")      is >> nodes;
                else if (token == "movetime")   is >> moveTime;                    
            }

            if (moveTime > 0) {
                
                tmg::timeManager.updateTimeSet(true);

                timePerMove = moveTime;

                maxTime = moveTime - 500;
            }

            else if (time > 0) {

                tmg::timeManager.updateTimeSet(true);

                movesToGo = movesToGo > -1 ? movesToGo : 40;

                const auto factor = 1.25;
                const auto target = (time - MOVE_OVERHEAD) / movesToGo + inc / 2;

                timePerMove = factor * target;

                maxTime = std::min(timePerMove * 5, (uint64_t)(time - 500));

                if (timePerMove >= time)
                    timePerMove = time - 500;

                if (timePerMove == 0)
                    timePerMove = 100;
            }

            else {
                
                tmg::timeManager.updateTimeSet(false);
            }

            threads.emplace_back(startSearch);
        }

        else if (token == "stop") {

            game::abortSearch = true;

            for (auto &th: threads) {

                if (th.joinable())
                    th.join();
            }

            threads.clear();
        }

        else if (token == "quit") {
        
            game::abortSearch = true;

            for (auto &th: threads) {

                if (th.joinable())
                    th.join();
            }

            threads.clear();

            break;
        } 





        // Debugging commands

        else if (token == "perft") {
            
            std::thread t4(startPerft, initInfo->stm, 10, initInfo);

            t4.join();
        } 

        else if (token == "divide") {

            int d = 0;
            
            while (is >> token)
             if (token == "divide")     is >> d;

            if (d <= 0) {

                printf("Depth should be greater than 0\n");
            } else {

                std::thread t5(divide, d, initInfo->stm, initInfo);

                t5.join();
            }
        } 

        else if (token == "evaluate") {
            
            print_board(initInfo->occupied, initInfo);
            printf("\n\n");

            int scoreWhite = nnueEval(WHITE, initInfo);
            
            printf("White score = %d\n", scoreWhite);
            
            printf("\n");

            int scoreBlack = nnueEval(BLACK, initInfo);

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

            debugSEE(ch, sq, initInfo);
        }
    } while(true);
}
