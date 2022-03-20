#ifndef MISC_H
#define MISC_H

#include "constants.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"

extern int MAX_DEPTH;
extern U8 rookCastleFlagMask[U8_MAX_SQUARES];

inline bool isRepetition(int ply, Thread *th) 
{    
    bool flag = false;
    for (int i = th->moves_history_counter + ply; i >= 0; i--) 
    {
        if (th->movesHistory[i].hashKey == th->hashKey) 
        {
            flag = true;
            break;
        }
    }
    
    return flag;
}

template<Side side>
inline bool isKingInCheck(Thread *th) 
{    
    const auto opp = side == WHITE ? BLACK : WHITE;      
    const auto kingSq = GET_POSITION(side ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);
    
    // Staggered check to return early saving time
    
    U64 attacks = 0ULL;
    
    attacks = get_knight_attacks(kingSq);        
    
    if (attacks & (opp == WHITE ? th->whitePieceBB[KNIGHTS] : th->blackPieceBB[KNIGHTS]))
    {
        return true;
    }
    
    attacks = Bmagic(kingSq, th->occupied);
    
    if (attacks & (opp == WHITE ? th->whitePieceBB[BISHOPS] : th->blackPieceBB[BISHOPS]))
    {
        return true;
    }
    if (attacks & (opp == WHITE ? th->whitePieceBB[QUEEN] : th->blackPieceBB[QUEEN]))
    {
        return true;
    }
    
    attacks = Rmagic(kingSq, th->occupied);
    
    if (attacks & (opp == WHITE ? th->whitePieceBB[ROOKS] : th->blackPieceBB[ROOKS]))
    {
        return true;
    }
    if (attacks & (opp == WHITE ? th->whitePieceBB[QUEEN] : th->blackPieceBB[QUEEN]))
    {
        return true;
    }
    
    attacks = get_king_attacks(kingSq);
    
    if (attacks & (opp == WHITE ? th->whitePieceBB[KING] : th->blackPieceBB[KING])) 
    {
        return true;        
    }
    
    attacks = opp == WHITE ? 
    wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]) :
    bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
    
    if (attacks & (1ULL << kingSq))
    {
        return true;
    }
    
    return false;
}

//TODO check logic, under observation
inline bool isPositionDraw(Thread *th) 
{    
    if (POPCOUNT(th->occupied) > 4)
    {
        return false;
    }
    
    //Kk
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]) == th->occupied) 
    {
        return true; 
    }
    
    //KNNk or KNk
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING] 
        |   th->whitePieceBB[KNIGHTS]) == th->occupied) 
    {
        return true; 
    }
    
    //Knnk or Knk
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->blackPieceBB[KNIGHTS]) == th->occupied) 
    {
        return true; 
    }
    
    //KNkn
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING] 
        |   th->whitePieceBB[KNIGHTS] 
        |   th->blackPieceBB[KNIGHTS]) == th->occupied 
        && POPCOUNT(th->whitePieceBB[KNIGHTS]) == 1 
        && POPCOUNT(th->blackPieceBB[KNIGHTS]) == 1)
    {
        return true; 
    }
    
    //KBk
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING] 
        |   th->whitePieceBB[BISHOPS]) == th->occupied
        && POPCOUNT(th->whitePieceBB[BISHOPS]) == 1)
    {
        return true; 
    }
    
    //Kbk
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1)
    {
        return true; 
    }   
    
    //KBkn
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[BISHOPS]
        |   th->blackPieceBB[KNIGHTS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[BISHOPS]) == 1
        &&  POPCOUNT(th->blackPieceBB[KNIGHTS]) == 1) 
    {
        return true; 
    }   
    
    //KNkb
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[KNIGHTS]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[KNIGHTS]) == 1
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1)   
    {
        return true; 
    }     
    
    //KBkb
    if ((   th->whitePieceBB[KING] 
        |   th->blackPieceBB[KING]
        |   th->whitePieceBB[BISHOPS]
        |   th->blackPieceBB[BISHOPS]) == th->occupied
        &&  POPCOUNT(th->whitePieceBB[BISHOPS]) == 1
        &&  POPCOUNT(th->blackPieceBB[BISHOPS]) == 1) 
    {
        return true; 
    }
    
    
    return false;
}


#endif
