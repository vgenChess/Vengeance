#ifndef ucireport_h
#define ucireport_h

#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include "namespaces.h"
#include "constants.h"

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

inline std::string reportPV(int depth, int selDepth, int score,
                     int timeElapsedMs, U32* pvLine, U64 totalNodes, U64 totalTTHits) {

  std::stringstream ss;

  ss
    << "info depth " << depth
    << " seldepth " << selDepth
    << " score " << "cp " << score
    << " time " << timeElapsedMs
    << " nodes " << totalNodes
    << " nps " << totalNodes / timeElapsedMs * 1000
    << " tbhits " << totalTTHits;

    ss << " pv";

    U32 move;
    for (int i = 0; i < MAX_PV_LENGTH ; i++) {

        move = pvLine[i];

        if (move == NO_MOVE)
            break;

        ss << " " << getMoveNotation(move);
    }

    return ss.str();
}

#endif
