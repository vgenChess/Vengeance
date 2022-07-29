//
//  movegen.c
//  Vgen
//
//  Created by Amar Thapa on 2/05/17.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <assert.h>
#include <iostream>

#include "types.h"
#include "movegen.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "utility.h"
#include "search.h"
#include "see.h"
#include "constants.h"
#include "functions.h"
#include "utility.h"

/*
 *    Move Type
 *
 *    0 -> quiet moves
 *    1 -> captures
 *    2 -> pawn double moves
 *    3 -> en_passant
 *    4 -> castling
 *    5 -> promotions
 *    6 -> king moves
 */

/*
 *    Promotion Type
 *
 *    0 - Queen
 *    1 - Rook
 *    2 - Knight
 *    3 - Bishop
 **/

uint16_t val_piece[8] = { 

    0, 
    VALUE_PAWN, 
    VALUE_KNIGHT, 
    VALUE_BISHOP, 
    VALUE_ROOK,
    VALUE_QUEEN, 
    VALUE_KING, 
    0
};

inline U32 createMove(Side stm, U32 promotion_type, U32 castleDir, U32 move_type, U32 c_piece, U32 piece, U32 from, U32 to) {
    
    return (0ULL | promotion_type << 24 | castleDir << 22 | move_type << 19 
        | stm << 18 | c_piece << 15 | piece << 12 | from << 6 | to);
}

inline Move getNoMove() {

    Move noMove;
    noMove.move = NO_MOVE;

    return noMove;
}

void generatePushes(Side stm, std::vector<Move> &moves, GameInfo *th) {
    
    U8 from, to; 
    U64 bitboard, pushes, empty = th->empty; 
    Move move;

    for (U8 piece = PAWNS; piece <= KING; piece++) {

        bitboard = stm ? th->blackPieceBB[piece] : th->whitePieceBB[piece];

        if (piece == PAWNS) {

            U64 pawns = bitboard & (stm ? NOT_RANK_2 : NOT_RANK_7);

            while (pawns) {

                from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;

                // single push
                to = stm ? from - 8 : from + 8;

                if ((1ULL << to) & empty) {

                    move.move = createMove(stm, 0, 0, MOVE_NORMAL, DUMMY, PAWNS, from, to);
                    moves.push_back(move);
                }
            }

            pawns = bitboard & (stm ? RANK_7 : RANK_2);

            while (pawns) {
                
                from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;

                to = stm ? from - 8 : from + 8;
                // check for inbetween square is empty before double push
                if ((1ULL << to) & empty) {

                    // get the double push target square
                    to = stm ? from - 16 : from + 16;

                    // check if target square is empty for the double push
                    if ((1ULL << to) & empty) {

                        move.move = createMove(stm, 0, 0, MOVE_DOUBLE_PUSH, DUMMY, PAWNS, from, to);
                        moves.push_back(move);
                    }
                }
            }

            continue;
        }

        while (bitboard) {

            from = __builtin_ctzll(bitboard);
            bitboard &= bitboard - 1;

                if (    piece == KNIGHTS)  pushes = get_knight_attacks(from);
            else if (   piece == BISHOPS)  pushes = Bmagic(from, th->occupied);
            else if (   piece == ROOKS)    pushes = Rmagic(from, th->occupied);
            else if (   piece == QUEEN)    pushes = Qmagic(from, th->occupied);
            else if (   piece == KING)     pushes = get_king_attacks(from);
            
            pushes &= empty;

            while (pushes) {
            
                to = __builtin_ctzll(pushes);
                pushes &= pushes - 1;

                move.move = createMove(stm, 0, 0, MOVE_NORMAL, DUMMY, piece, from, to);
                moves.push_back(move);
            }
        }
    }
}

void generateCaptures(Side stm, std::vector<Move> &moves, GameInfo *th) {
    
    int from, to; 
    U64 cPieceBB, fromBB;

    U64 oppPieces = stm ^ 1 ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

    U64 b, attacks;
    Move move;
    for (int p = PAWNS; p <= KING; p++) {

        b = stm ? th->blackPieceBB[p] : th->whitePieceBB[p];
        attacks = 0ULL;

        if (p == PAWNS) b &= stm ? NOT_RANK_2 : NOT_RANK_7;
        
        while (b) {

            from = __builtin_ctzll(b);
            b &= b - 1;

            assert(from >= 0 && from <= 63);

            if (p == PAWNS) {
                
                fromBB = 1ULL << from;
                attacks = stm ? 
                    ((fromBB >> 7) & NOT_A_FILE) | ((fromBB >> 9) & NOT_H_FILE) :
                    ((fromBB << 7) & NOT_H_FILE) | ((fromBB << 9) & NOT_A_FILE);
            }
            else if (p == KNIGHTS) attacks |= get_knight_attacks(from);            
            else if (p == BISHOPS) attacks |= Bmagic(from, th->occupied);
            else if (p == ROOKS)   attacks |= Rmagic(from, th->occupied);
            else if (p == QUEEN)   attacks |= Qmagic(from, th->occupied);
            else if (p == KING)    attacks |= get_king_attacks(from);

            attacks &= oppPieces;

            while (attacks) {

                to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;

                for (U8 cPieceType = PAWNS; cPieceType < KING; cPieceType++) {

                    cPieceBB = (stm ^ 1) ? th->blackPieceBB[cPieceType] : th->whitePieceBB[cPieceType];

                    if (cPieceBB & (1ULL << to)) {

                        move.move = createMove(stm, 0, 0, MOVE_CAPTURE, cPieceType, p, from, to);
                        moves.push_back(move);
                    }
                }
            }
        }   
    }
}

void genCastlingMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th) {

    Move move;

    U8 castleFlags = th->moveStack[ply].castleFlags;

    if (stm == WHITE) {
        
        if (castleFlags & CASTLE_FLAG_WHITE_QUEEN) {
           
            U64 wq_sqs = th->empty & WQ_SIDE_SQS;
            if (wq_sqs == WQ_SIDE_SQS 

                &&  !(      isSqAttacked(2, WHITE, th) 
                        ||  isSqAttacked(3, WHITE, th) 
                        ||  isSqAttacked(4, WHITE, th))) {
                    
                move.move = createMove(stm, 0, 0, MOVE_CASTLE, ROOKS, KING, 4, 2);
                moves.push_back(move);
            }
        }
        
        if (castleFlags & CASTLE_FLAG_WHITE_KING) {
            
            U64 wk_sqs = th->empty & WK_SIDE_SQS;
            
            if (wk_sqs == WK_SIDE_SQS 

                &&  !(      isSqAttacked(4, WHITE, th) 
                        ||  isSqAttacked(5, WHITE, th)
                        ||  isSqAttacked(6, WHITE, th))) {

                move.move = createMove(stm, 0, 1, MOVE_CASTLE, ROOKS, KING, 4, 6);
                moves.push_back(move);
            }
        }
    } else {
        
        if (castleFlags & CASTLE_FLAG_BLACK_QUEEN) {

            U64 bq_sqs = th->empty & BQ_SIDE_SQS;
            
            if (bq_sqs == BQ_SIDE_SQS 

                &&  !(      isSqAttacked(58, BLACK, th) 
                        ||  isSqAttacked(59, BLACK, th)
                        ||  isSqAttacked(60, BLACK, th))) {
                    
                move.move = createMove(stm, 0, 2, MOVE_CASTLE, ROOKS, KING, 60, 58);
                moves.push_back(move);
            }
        }
        
        if (castleFlags & CASTLE_FLAG_BLACK_KING) {     
            // Shouldn't keep this in an else check since the function should generate all castling moves 
            
            U64 bk_sqs = th->empty & BK_SIDE_SQS; 
            
            if (bk_sqs == BK_SIDE_SQS 
                
                && !(       isSqAttacked(60, BLACK, th) 
                        ||  isSqAttacked(61, BLACK, th) 
                        ||  isSqAttacked(62, BLACK, th))) {
            
                move.move = createMove(stm, 0, 3, MOVE_CASTLE, ROOKS, KING, 60, 62);
                moves.push_back(move);
            }
        }
    }
}

void genEnpassantMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th) {
    
    if (th->moveStack[ply].epFlag) {
        
        U8 from;
        U8 to = th->moveStack[ply].epSquare;
        U64 target_sqs;
        U64 target_pawns;
        U64 pawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
        
        U64 epSquareBB = 1ULL << to;

        if (stm == WHITE) {

            target_sqs = ((epSquareBB >> 7) & NOT_A_FILE) | ((epSquareBB >> 9) & NOT_H_FILE);
        } else {
            
            target_sqs = ((epSquareBB << 7) & NOT_H_FILE) | ((epSquareBB << 9) & NOT_A_FILE);
        }
        
        target_pawns = target_sqs & pawns;
        
        Move move;
        while (target_pawns) {
            
            from = __builtin_ctzll(target_pawns);
            
            target_pawns &= target_pawns - 1;
            
            move.move = createMove(stm, 0, 0, MOVE_ENPASSANT, PAWNS, PAWNS, from, to);

            moves.push_back(move);
        }
    }
}

void genPromotionsNormal(Side stm, std::vector<Move> &moves, GameInfo *th) {
    
    U8 from;
    U8 to;
	U64 pawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]; 
    U64 pawnsToPromote;
    U64 empty = th->empty;
    
    if (stm == WHITE) {
        
        pawnsToPromote = pawns & RANK_7;
    } else {
        
        pawnsToPromote = pawns & RANK_2;
    }

    Move move;
	while (pawnsToPromote) {
        
        from = __builtin_ctzll(pawnsToPromote);
        pawnsToPromote &= pawnsToPromote - 1;

        to = stm ? from - 8 : from + 8;

        if ((1ULL << to) & empty) {

            move.move = createMove(stm, PROMOTE_TO_QUEEN, 0, MOVE_PROMOTION, DUMMY, PAWNS, from, to);
            moves.push_back(move);

            move.move = createMove(stm, PROMOTE_TO_ROOK, 0, MOVE_PROMOTION, DUMMY, PAWNS, from, to);
            moves.push_back(move);
            
            move.move = createMove(stm, PROMOTE_TO_BISHOP, 0, MOVE_PROMOTION, DUMMY, PAWNS, from, to);
            moves.push_back(move);
            
            move.move = createMove(stm, PROMOTE_TO_KNIGHT, 0, MOVE_PROMOTION, DUMMY, PAWNS, from, to);
            moves.push_back(move);
        } 
    }
}

void genPromotionsAttacks(Side stm, std::vector<Move> &moves, GameInfo *th) {
    
    U8 from;
    U8 to;
    U64 toAttack;
    U64 fromBB;
    U64 pawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
    U64 pawnsToPromote;
    U64 cPieceBB;
    
    pawnsToPromote = stm ? (pawns & RANK_2) : (pawns & RANK_7);

    Move move;
    while (pawnsToPromote) {
        
        from = __builtin_ctzll(pawnsToPromote);
        pawnsToPromote &= pawnsToPromote - 1;

        fromBB = 1ULL << from;
        
        if (stm == WHITE) {
            
            toAttack = (((fromBB << 7) & NOT_H_FILE) | ((fromBB << 9) & NOT_A_FILE));
        } else {
            
            toAttack = (((fromBB >> 7) & NOT_A_FILE) | ((fromBB >> 9) & NOT_H_FILE));
        }

        while (toAttack) {
            
            to = __builtin_ctzll(toAttack);
            toAttack &= toAttack - 1;
             
            for (int cPiece = KNIGHTS; cPiece <= QUEEN; cPiece++) {

                cPieceBB = stm ^ 1 ? th->blackPieceBB[cPiece] : th->whitePieceBB[cPiece];

                if (cPieceBB & (1ULL << to)) {

                    move.move = createMove(stm, PROMOTE_TO_QUEEN, 0, MOVE_PROMOTION, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                    
                    move.move = createMove(stm, PROMOTE_TO_ROOK, 0, MOVE_PROMOTION, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                    
                    move.move = createMove(stm, PROMOTE_TO_BISHOP, 0, MOVE_PROMOTION, cPiece, PAWNS, from, to);
                    moves.push_back(move);

                    move.move = createMove(stm, PROMOTE_TO_KNIGHT, 0, MOVE_PROMOTION, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                }
            }
        }
    }
}

bool isValidMove(Side stm, const int ply, const U32 move, GameInfo *th) {
    
    if (move == NO_MOVE) 
        return false;
    
    const auto opp = stm == WHITE ? BLACK : WHITE;
    const auto fromSq = from_sq(move);
    const auto toSq = to_sq(move);
    const auto fromSqBB = 1ULL << fromSq;
    const auto toSqBB = 1ULL << toSq;
    const auto moveType = move_type(move);
    const auto piece = pieceType(move);
    const auto capturePiece = cPieceType(move);
    const auto pieceBB = stm ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
    const auto capturePieceBB = opp ? th->blackPieceBB[capturePiece] : th->whitePieceBB[capturePiece];

    
    if (moveType == MOVE_NORMAL || moveType == MOVE_DOUBLE_PUSH || moveType == MOVE_CAPTURE) {

        if (moveType == MOVE_DOUBLE_PUSH && inBetween(fromSq, toSq) & th->occupied) 
            return false; // pawn's path is blocked       

        if (piece == ROOKS && !(Rmagic(fromSq, th->occupied) & toSqBB)) 
            return false; // rook's path is blocked

        if (piece == BISHOPS && !(Bmagic(fromSq , th->occupied) & toSqBB)) 
            return false; // bishop's path is blocked

        if (piece == QUEEN && !(Qmagic(fromSq, th->occupied) & toSqBB)) 
            return false; // queen's path is blocked

        // finally check for pieces on their squares
        return moveType == MOVE_CAPTURE ? 
            (fromSqBB & pieceBB) && (toSqBB & capturePieceBB) :
            (fromSqBB & pieceBB) && (toSqBB & th->empty);
    }

    
    if (moveType == MOVE_CASTLE) { 

        const auto castleFlags = th->moveStack[ply].castleFlags;

        const auto castleDirection = castleDir(move);
        
        const auto oppAttacks = getAttacks(opp, th);
        
        if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) { 
      
            if (castleFlags & CASTLE_FLAG_WHITE_QUEEN) {
                
                U64 wq_sqs = th->empty & WQ_SIDE_SQS;
                if (    wq_sqs == WQ_SIDE_SQS 
                    &&  !(      oppAttacks & (1ULL << 2) 
                            ||  oppAttacks & (1ULL << 3)
                            ||  oppAttacks & (1ULL << 4))) { 
                    
                    return true;
                }
            }
        } 
        else if (castleDirection == WHITE_CASTLE_KING_SIDE) {
            
            if (castleFlags & CASTLE_FLAG_WHITE_KING) {
                
                U64 wk_sqs = th->empty & WK_SIDE_SQS;
                if (    wk_sqs == WK_SIDE_SQS 
                    &&  !(      oppAttacks & (1ULL << 4) 
                            ||  oppAttacks & (1ULL << 5)
                            ||  oppAttacks & (1ULL << 6))) {

                    return true;
                }
            }        
        }
        else if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {
            
            if (castleFlags & CASTLE_FLAG_BLACK_QUEEN) {
    
                U64 bq_sqs = th->empty & BQ_SIDE_SQS;
                if (    bq_sqs == BQ_SIDE_SQS 
                    &&  !(      oppAttacks & (1ULL << 58) 
                            ||  oppAttacks & (1ULL << 59)
                            ||  oppAttacks & (1ULL << 60))) {
                    
                    return true;
                }
            }
        }
        else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

            if (castleFlags & CASTLE_FLAG_BLACK_KING) {
    
                U64 bk_sqs = th->empty & BK_SIDE_SQS; 
                if (    bk_sqs == BK_SIDE_SQS 
                    && !(       oppAttacks & (1ULL << 60) 
                            ||  oppAttacks & (1ULL << 61)
                            ||  oppAttacks & (1ULL << 62))) {
                    
                    return true;
                }
            }
        }
    }

    if (moveType == MOVE_ENPASSANT) {

        return th->moveStack[ply].epFlag && th->moveStack[ply].epSquare == toSq;
    }
    
    if (moveType == MOVE_PROMOTION) {

        return capturePiece == DUMMY ? 
            (fromSqBB & pieceBB) && (toSqBB & th->empty) :
            (fromSqBB & pieceBB) && (toSqBB & capturePieceBB);
    }

    return false;
}


void genSpecialMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th) {
     
    genPromotionsAttacks(stm, moves, th);
    genPromotionsNormal(stm, moves, th);
    genCastlingMoves(stm, ply, moves, th);  
}

void genAttacks(Side stm, int ply, std::vector<Move> &moves, GameInfo *th) {
    
    genEnpassantMoves(stm, ply, moves, th);
    generateCaptures(stm, moves, th);
}

void genMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th) {
   
   if (stm) {
    
        genSpecialMoves(BLACK, ply, moves, th);
        genAttacks(BLACK, ply, moves, th);
        generatePushes(BLACK, moves, th);
   } else {

        genSpecialMoves(WHITE, ply, moves, th);
        genAttacks(WHITE, ply, moves, th);
        generatePushes(WHITE, moves, th);
   }
}

inline int getBestMoveIndex(std::vector<Move> &moves) {
  
    int m = 0;
    int n = moves.size();
    for (int i = m + 1; i < n; i++) {

        if (moves[i].score > moves[m].score) {

            m = i;
        }
    }

    return m;
}


void scoreCaptureMoves( GameInfo *th, MOVE_LIST *moveList) {

    int piece, to, target, mt;
   
    for (auto &m : moveList->moves) {

        piece = pieceType(m.move);
        to = to_sq(m.move);
        target = cPieceType(m.move);

        mt = move_type(m.move);

        if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) 
            target = PAWNS;

        m.score = th->captureHistoryScore[piece][to][target];
    }
}

void scoreNormalMoves(Side stm, int ply, GameInfo *th, MOVE_LIST *moveList) {

    U32 previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

    for (auto &m : moveList->moves) {

        m.score = th->historyScore[stm][from_sq(m.move)][to_sq(m.move)];

        if (    previousMove != NO_MOVE    
            &&  m.move == th->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)]) {

            m.score += U16_COUNTER_MOVE_BONUS; // TODO check logic
        }
    }
}

Move getNextMove(Side stm, int ply, GameInfo *th, MOVE_LIST *moveList) {

    switch (moveList->stage) {

        case PLAY_HASH_MOVE : {

            moveList->stage = GEN_CAPTURES;

            if (isValidMove(stm, ply, moveList->ttMove, th)) {
                    
                Move m;

                m.move = moveList->ttMove;
                
                return m;           
            }
        }
        
        
        //fallthrough

        case GEN_CAPTURES : {

            moveList->moves.clear();
            genAttacks(stm, ply, moveList->moves, th);

            scoreCaptureMoves(th, moveList);
            
            moveList->stage = PLAY_GOOD_CAPTURES;
        }
        
        // fallthrough
        
        case PLAY_GOOD_CAPTURES : {

            if (moveList->moves.size() > 0) {

                int index = getBestMoveIndex(moveList->moves);
                
                Move m = moveList->moves[index];

                moveList->moves.erase(moveList->moves.begin() + index);
                
                if (m.move == moveList->ttMove)
                    return getNextMove(stm, ply, th, moveList);

                m.seeScore = SEE(stm, m.move, th);

                if (m.seeScore < 0) { // bad capture

                    moveList->badCaptures.push_back(m);
                    
                    return getNextMove(stm, ply, th, moveList);
                } 

                return m;
            }   

            moveList->stage = PLAY_KILLER_MOVE_1;
        }
        
        //fallthrough

        case PLAY_KILLER_MOVE_1 : {

            moveList->stage = PLAY_KILLER_MOVE_2;

            if (    !moveList->skipQuiets
                &&  moveList->killerMove1 != moveList->ttMove
                &&  isValidMove(stm, ply, moveList->killerMove1, th)) {
                    
                Move m;

                m.move = moveList->killerMove1;
               
                return m;           
            }
        }
        
        //fallthrough

        case PLAY_KILLER_MOVE_2 : {

            moveList->stage = PLAY_COUNTER_MOVE;

            if (    !moveList->skipQuiets    
                &&  moveList->killerMove2 != moveList->ttMove
                &&  isValidMove(stm, ply, moveList->killerMove2, th)) {
                    
                Move m;

                m.move = moveList->killerMove2;
               
                return m;           
            }
        }

        // fallthrough

        case PLAY_COUNTER_MOVE : {

            moveList->stage = GEN_PROMOTIONS;

            if (    !moveList->skipQuiets
                &&  moveList->counterMove != moveList->ttMove
                &&  moveList->counterMove != moveList->killerMove1
                &&  moveList->counterMove != moveList->killerMove2
                &&  isValidMove(stm, ply, moveList->counterMove, th)) {

                Move m;

                m.move = moveList->counterMove;

                return m;
            }           
        }


         // fallthrough
        // ignore skipQuiets for promotions 
        case GEN_PROMOTIONS : {

            moveList->moves.clear();

            genPromotionsAttacks(stm, moveList->moves, th);
            genPromotionsNormal(stm, moveList->moves, th);

            for (auto &m : moveList->moves) {

                m.score = 1000; // give equal score for promotions
            }
            
            moveList->stage = PLAY_PROMOTIONS;              
        }
        
        //fallthrough 

        case PLAY_PROMOTIONS : {

            if (moveList->moves.size() > 0) {

                int index = getBestMoveIndex(moveList->moves);
                
                Move m = moveList->moves[index];
                
                moveList->moves.erase(moveList->moves.begin() + index);
                    
                if (m.move == moveList->ttMove) {
    
                    return getNextMove(stm, ply, th, moveList);
                }

                return m;
            }

            moveList->stage = PLAY_BAD_CAPTURES;
        }


        //fallthrough

        case PLAY_BAD_CAPTURES : {

            if (moveList->badCaptures.size() > 0) {

                int index = getBestMoveIndex(moveList->badCaptures);
                    
                Move m = moveList->badCaptures[index];

                moveList->badCaptures.erase(moveList->badCaptures.begin() + index);

                return m.move == moveList->ttMove ? getNextMove(stm, ply, th, moveList) : m;
            }

            moveList->stage = GEN_QUIETS;
        }


        //fallthrough

        case GEN_QUIETS : { // generate all non-capture moves excluding promotions

            if (!moveList->skipQuiets) {

                moveList->moves.clear();

                genCastlingMoves(stm, ply, moveList->moves, th);
                generatePushes(stm, moveList->moves, th);
                
                scoreNormalMoves(stm, ply, th, moveList);                 
            }

            moveList->stage = PLAY_QUIETS;
        }

        //fallthrough

        case PLAY_QUIETS : {

            if (    !moveList->skipQuiets
                &&  moveList->moves.size() > 0) {

                int index = getBestMoveIndex(moveList->moves);
                    
                Move m = moveList->moves[index];

                moveList->moves.erase(moveList->moves.begin() + index);

                if (    m.move == moveList->ttMove 
                    ||  m.move == moveList->counterMove
                    ||  m.move == moveList->killerMove1
                    ||  m.move == moveList->killerMove2) {
                  
                    return getNextMove(stm, ply, th, moveList);
                }

                return m;
            }

            moveList->stage = STAGE_DONE;

            return getNoMove();
        }

        case STAGE_DONE:

            return getNoMove();
    }

    return getNoMove();
}
