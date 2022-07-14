#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <vector>
#include <iostream>

#include "namespaces.h"

using namespace game;

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

inline void reportPV(GameInfo *gi, U64 totalNodes, U64 totalTTHits) {

    const auto depth = gi->completedDepth;
    const auto selDepth = gi->selDepth;
    const auto pvLine = gi->pvLine[gi->completedDepth].line;

    int score = gi->pvLine[gi->completedDepth].score;
    
    std::cout << "info depth " << depth << " seldepth " << selDepth; 
    std::cout << " time " << tmg::timeManager.timeElapsed<MILLISECONDS> (
                  tmg::timeManager.getStartTime());
    std::cout << " nodes " << totalNodes;
    std::cout/*<< " hashfull " << hashfull()*/ << " tbhits " << totalTTHits;
    std::cout << " score cp " << score << " pv";
    
    U32 move;
    for (int i = 0; i < MAX_PLY; i++) {
        
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
