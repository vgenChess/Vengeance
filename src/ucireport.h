#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>
#include <iostream>

#include "thread.h"
#include "TimeManagement.h"

inline std::string getMoveNotation(const U32 move) {

    std::string str;

    str += Notation::algebricSq[from_sq(move)];
    str += Notation::algebricSq[to_sq(move)];

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

inline void reportPV(SearchThread *th) {

    const auto depth = th->completedDepth;
    const auto selDepth = th->selDepth;
    const auto pvLine = th->pvLine[th->completedDepth].line;

    int score = th->pvLine[th->completedDepth].score;
    
    std::cout << "info depth " << depth << " seldepth " << selDepth; 
    std::cout << " time " << TimeManager::time_elapsed_milliseconds(
         TimeManager::sTimeManager.getStartTime()); 
    std::cout << " nodes " << searchThreads.totalNodes();
    std::cout/*<< " hashfull " << hashfull()*/ << " tbhits " << searchThreads.totalTTHits();
    std::cout << " score cp " << score << " pv";
    
    U32 move;
    for (int i = 0; i < U16_MAX_PLY; i++) {
        
        move = pvLine[i];
        
        if (move == NO_MOVE)
        {
            break;
        }
        
        std::cout << " " << getMoveNotation(move);
    }
    
    std::cout << "\n";
}

inline void reportBestMove() {

    auto bestThread = searchThreads.getBestThread();

    if (bestThread != searchThreads.getMainSearchThread())
        reportPV(bestThread);

    U32 bestMove = bestThread->pvLine[bestThread->completedDepth].line[0];

    std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;
}

#endif
