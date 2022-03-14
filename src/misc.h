#ifndef MISC_H
#define MISC_H

#include "constants.h"
    
namespace misc {

    inline void init_inbetween_bb() {
        
        for (int i = 0; i < U8_MAX_SQUARES; i++) {
            for(int j = 0; j < U8_MAX_SQUARES; j++) {

                arrInBetween[i][j] = inBetweenOnTheFly(i, j);
            }
        }
    }

    inline void initCastleMaskAndFlags() {

        for (int i = 0; i < U8_MAX_SQUARES; i++) {
            
            rookCastleFlagMask[i] = 15;
        }

        rookCastleFlagMask[0] ^= CASTLE_FLAG_WHITE_QUEEN;
        rookCastleFlagMask[7] ^= CASTLE_FLAG_WHITE_KING;
        rookCastleFlagMask[56] ^= CASTLE_FLAG_BLACK_QUEEN;
        rookCastleFlagMask[63] ^= CASTLE_FLAG_BLACK_KING;
    }

    
    inline bool isRepetition(int ply, Thread *th) {
        
        bool flag = false;
        for (int i = th->moves_history_counter + ply; i >= 0; i--) {
            
            if (th->movesHistory[i].hashKey == th->hashKey) {
                
                flag = true;
                break;
            }
        }
        
        return flag;
    }
    
    template<Side side>
    inline bool isKingInCheck(Thread *th) {
        
        const auto opp = side == WHITE ? BLACK : WHITE;      
        const auto kingSq = GET_POSITION(side ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);
        
        // Staggered check to return early saving time
        
        U64 attacks = 0ULL;
        
        attacks = get_knight_attacks(kingSq);        
        
        if (attacks & (opp == WHITE ? th->whitePieceBB[KNIGHTS] : th->blackPieceBB[KNIGHTS]))
            return true;
        
        
        attacks = Bmagic(kingSq, th->occupied);
        
        if (attacks & (opp == WHITE ? th->whitePieceBB[BISHOPS] : th->blackPieceBB[BISHOPS]))
            return true;
        if (attacks & (opp == WHITE ? th->whitePieceBB[QUEEN] : th->blackPieceBB[QUEEN]))
            return true;
        
        
        attacks = Rmagic(kingSq, th->occupied);
        
        if (attacks & (opp == WHITE ? th->whitePieceBB[ROOKS] : th->blackPieceBB[ROOKS]))
            return true;
        if (attacks & (opp == WHITE ? th->whitePieceBB[QUEEN] : th->blackPieceBB[QUEEN]))
            return true;
        
        
        attacks = get_king_attacks(kingSq);
        
        if (attacks & (opp == WHITE ? th->whitePieceBB[KING] : th->blackPieceBB[KING])) 
            return true;        
        
        
        attacks = opp == WHITE ? 
        wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]) :
        bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
        
        if (attacks & (1ULL << kingSq))
            return true;
        
        return false;
    }
    
}


#endif
