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
#include "vtime.h"
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

int option_hash_size;
bool NNUE = false;

int totalTimeLeft;

u8 addFileNumber(char ch) {
    
    switch (ch) {

        case 'a':
            return 0;
        case 'b':
            return 1;
        case 'c':
            return 2;
        case 'd':
            return 3;
        case 'e':
            return 4;
        case 'f':
            return 5;
        case 'g':
            return 6;
        case 'h':
            return 7;
    }
    
    return 0;
}


// e2e4
u8 parsePosition (std::string str, Thread *th) {
  
    u8 sideToMove = WHITE;

    const std::string str_fen = "fen";
    const std::string str_startpos = "startpos";
    const std::string str_moves = "moves";

    std::string tempStr;


    if (str.find(str_fen) != std::string::npos) {

        int pos = str.find(str_fen);

        tempStr = str.substr(pos + 1 + str_fen.length());

        std::cout << tempStr << "\n";

        sideToMove = parseFen(tempStr, th);
    } else if (str.find(str_startpos) != std::string::npos) {

        sideToMove = parseFen(START_FEN, th);
    }


	//nn_inputs_upd_all(&nn, th); 


    initThread.moves_history_counter = 0;
    initThread.movesHistory[0].hashKey = initThread.hashKey;
    initThread.movesHistory[0].fiftyMovesCounter = 0;


    if (str.find(str_moves) != std::string::npos) {

        Move move;

        int pos = str.find(str_moves);

        tempStr = str.substr(pos + 1 + str_moves.length());

        std::vector <std::string> tokens1; 

        split(tempStr, tokens1, ' ');

        std::vector<Move> moves;

        std::string moveStr;

        for (std::vector<std::string>::iterator i = tokens1.begin(); i != tokens1.end(); ++i) {
            
            moveStr = *i;

            moves.clear();
            genMoves(0, moves, sideToMove, th);
            
            for (std::vector<Move>::iterator j = moves.begin(); j != moves.end(); ++j) {     

                move = *j;
        
                char str1[10];
                str1[0] = '\0';

                strcat(str1, algebricPos(from_sq(move.move)));
                strcat(str1, algebricPos(to_sq(move.move)));


                if (move_type(move.move) == MOVE_PROMOTION) {


                    switch (promType(move.move)) {

                        case PROMOTE_TO_ROOK:

                            strcat(str1, "r");

                            // sideToMove ? strcat(str1, "r") : strcat(str1, "R");
                            break;
                        case PROMOTE_TO_BISHOP:

                            strcat(str1, "b");
                  
                            // sideToMove ? strcat(str1, "b") : strcat(str1, "B");
                            break;
                        case PROMOTE_TO_KNIGHT:

                            strcat(str1, "n");
                  
                            // sideToMove ? strcat(str1, "n") : strcat(str1, "N");
                            break;
                        default:

                            strcat(str1, "q");
                  
                            // sideToMove ? strcat(str1, "q") : strcat(str1, "Q");
                            break;
                    }
                }

                if (moveStr == str1) {
    
                    make_move(0, move.move, th);
                    
                    initThread.moves_history_counter++;
                    initThread.movesHistory[initThread.moves_history_counter].hashKey = initThread.hashKey;

                    sideToMove ^= 1;
    
                    break;
                }
            }
        }
    }


    // print_board(th->occupied, th);


    return sideToMove;
}

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


using namespace std;


void UciLoop() {

    std::string startLine = "position startpos";
	
	initThread.isInit = true;
    initThread.side = parsePosition(startLine, &initThread);
    
    quit = false;
    
    std::string cmd, token;

    do {


        if (!getline(cin, cmd)) // Block here waiting for input or EOF 
            cmd = "quit";


        istringstream is(cmd);


        token.clear(); // Avoid a stale if getline() returns empty or blank line
        is >> skipws >> token;


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

                std::cout << tempStr << "\n";

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
                
                stopTime = startTime + std::chrono::milliseconds((int)(time * 0.75));

                timePerMove = (time / (movestogo + 2)) + inc; 

                totalTimeLeft = time;    
            } 
            




            // if (depthCurrent == -1) {
    // const std::string str_fen = "fen";
                // std::string tempStr;
                
                // int pos = str.find(str_fen);

                // tempStr = str.substr(pos + 1 + str_fen.length());

                // std::cout << tempStr << "\n";

            
            //     depthCurrent = MAX_DEPTH;
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
