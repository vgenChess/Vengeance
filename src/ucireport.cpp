
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "ucireport.h"
#include "utility.h"
#include "thread.h"

std::string getMoveNotation(const u32 move) {

	char str[10];
	str[0] = '\0';

	strcat(str, algebricPos(from_sq(move)));
	strcat(str, algebricPos(to_sq(move)));

    if (move_type(move) == MOVE_PROMOTION) {

    	u8 pt = promType(move);

        if  (pt == PROMOTE_TO_ROOK) {
                
            strcat(str, "r");
        }
        else if (pt == PROMOTE_TO_BISHOP) {
                
            strcat(str, "b");
        }   
        else if (pt == PROMOTE_TO_KNIGHT) {
                
            strcat(str, "n");
        } 
        else {
                
            strcat(str, "q");
        }
    }

    return std::string(str);
}

void reportBestMove() {

	u32 bestMove = Threads.main()->pvLine[Threads.main()->completedDepth].line[0];

	std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;
}

void reportCurrentMove(int side, int depth, int currentMoveNumber, u32 move) {

	std::cout << "info depth " << depth << " currmove ";

	std::cout << getMoveNotation(move) << " currmovenumber " << currentMoveNumber << std::endl;
}

void display(u8 sideToMove, int depth, int selDepth, int score, std::vector<u32> pvLine) {

	std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

	std::cout 	<< "info depth " << depth << " seldepth " << selDepth 
				<< " time " << timeSpent << " nodes " << Threads.getTotalNodes() 
				/*<< " hashfull " << hashfull()*/ << " tbhits " << Threads.getTotalTTHits() 
				<< " score cp " << score << " pv";

	for (std::vector<u32>::iterator i = pvLine.begin(); i != pvLine.end(); ++i) {
		
		std::cout << " " << getMoveNotation(*i);
	}

	std::cout << "\n";
}
