#ifndef MISC_H
#define MISC_H

#include "constants.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"

extern int MAX_DEPTH;
extern U8 rookCastleFlagMask[MAX_SQUARES];

inline bool isRepetition(int ply, GameInfo *gi)
{    
    bool flag = false;
    for (int i = gi->moves_history_counter + ply; i >= 0; i--)
    {
        if (gi->movesHistory[i].hashKey == gi->hashKey)
        {
            flag = true;
            break;
        }
    }
    
    return flag;
}

template<Side side>
inline bool isKingInCheck(GameInfo *gi) {

    const auto opp = side == WHITE ? BLACK : WHITE;      
    const auto kingSq = GET_POSITION(side ? gi->blackPieceBB[KING] : gi->whitePieceBB[KING]);
    
    // Staggered check to return early saving time
    
    U64 attacks = 0ULL;
    
    attacks = get_knight_attacks(kingSq);        
    
    if (attacks & (opp == WHITE ? gi->whitePieceBB[KNIGHTS] : gi->blackPieceBB[KNIGHTS]))
    {
        return true;
    }
    
    attacks = Bmagic(kingSq, gi->occupied);
    
    if (attacks & (opp == WHITE ? gi->whitePieceBB[BISHOPS] : gi->blackPieceBB[BISHOPS]))
    {
        return true;
    }
    if (attacks & (opp == WHITE ? gi->whitePieceBB[QUEEN] : gi->blackPieceBB[QUEEN]))
    {
        return true;
    }
    
    attacks = Rmagic(kingSq, gi->occupied);
    
    if (attacks & (opp == WHITE ? gi->whitePieceBB[ROOKS] : gi->blackPieceBB[ROOKS]))
    {
        return true;
    }
    if (attacks & (opp == WHITE ? gi->whitePieceBB[QUEEN] : gi->blackPieceBB[QUEEN]))
    {
        return true;
    }
    
    attacks = get_king_attacks(kingSq);
    
    if (attacks & (opp == WHITE ? gi->whitePieceBB[KING] : gi->blackPieceBB[KING]))
    {
        return true;        
    }
    
    attacks = opp == WHITE ? 
    wPawnWestAttacks(gi->whitePieceBB[PAWNS]) | wPawnEastAttacks(gi->whitePieceBB[PAWNS]) :
    bPawnWestAttacks(gi->blackPieceBB[PAWNS]) | bPawnEastAttacks(gi->blackPieceBB[PAWNS]);
    
    if (attacks & (1ULL << kingSq))
    {
        return true;
    }
    
    return false;
}

#endif
