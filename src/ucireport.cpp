
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "ucireport.h"
#include "utility.h"
#include "thread.h"

std::string algebricSq[64] = {

	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
}; 


std::string getMoveNotation(const u32 move) {

	std::string str;

	str += algebricSq[from_sq(move)];
	str += algebricSq[to_sq(move)];

    if (move_type(move) == MOVE_PROMOTION) {

    	switch (promType(move)) {

    		case PROMOTE_TO_ROOK: str += 'r'; break;
    		case PROMOTE_TO_BISHOP: str += 'b'; break;
    		case PROMOTE_TO_KNIGHT: str += 'n'; break;
    		case PROMOTE_TO_QUEEN: str += 'q'; break;
    		default : str += 'q'; break;
    	}
    }

    return str;
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
