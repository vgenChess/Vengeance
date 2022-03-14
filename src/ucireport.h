#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>

#include "globals.h"
#include "thread.h"
#include "time.h"

inline std::string algebricSq[64] = {

    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
}; 

inline std::string getMoveNotation(const U32 move) {

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

inline void reportBestMove() {

    U32 bestMove = Threads.getMainSearchThread()->pvLine[Threads.getMainSearchThread()->completedDepth].line[0];

    std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;
}
    
inline void reportPV(SearchThread *th) {

    const auto depth = th->completedDepth;
    const auto selDepth = th->selDepth;
    const auto pvLine = th->pvLine[th->completedDepth].line;

    int score = th->pvLine[th->completedDepth].score;
    
    std::cout << "info depth " << depth << " seldepth " << selDepth; 
    std::cout << " time " << TimeManager::time_elapsed_milliseconds(TimeManager::timeManager.startTime); 
    std::cout << " nodes " << Threads.totalNodes();
    std::cout/*<< " hashfull " << hashfull()*/ << " tbhits " << Threads.totalTTHits();
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

#endif
