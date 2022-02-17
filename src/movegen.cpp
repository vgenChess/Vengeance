//
//  movegen.c
//  Vgen
//
//  Created by Amar Thapa on 2/05/17.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <assert.h>
#include <iostream>

#include "globals.h"
#include "movegen.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "utility.h"
#include "search.h"
#include "see.h"
#include "constants.h"

typedef unsigned char u8;

void genMoves(int ply, std::vector<Move> &moves, u8 side, Thread *th) {
    
    genSpecialMoves(ply, moves, side, th);
    genAttacks(ply, moves, side, th);
	generatePushes(side, moves, th);
}

void genAttacks(int ply, std::vector<Move> &moves, u8 side, Thread *th) {
    
    genEnpassantMoves(ply, moves, side, th);
    generateCaptures(side, moves, th);
}

void genSpecialMoves(int ply, std::vector<Move> &moves, u8 side, Thread *th) {
	 
    genPromotionsAttacks(moves, side, th);
	genPromotionsNormal(moves, side, th);
    genCastlingMoves(ply, moves, side, th);	
}

void generatePushes(u8 side, std::vector<Move> &moves, Thread *th) {
    
    u8 from, to; 
    u64 bitboard, pushes, empty = th->empty; 
    Move move;

    for (u8 piece = PAWNS; piece <= KING; piece++) {

        bitboard = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];

        if (piece == PAWNS) {

            u64 pawns = bitboard & (side ? NOT_RANK_2 : NOT_RANK_7);

            while (pawns) {

                from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;

                // single push
                to = side ? from - 8 : from + 8;

                if ((1ULL << to) & empty) {

                    move.move = createMove(0, 0, MOVE_NORMAL, side, DUMMY, PAWNS, from, to);
                    moves.push_back(move);
                }
            }

            pawns = bitboard & (side ? RANK_7 : RANK_2);

            while (pawns) {
                
                from = __builtin_ctzll(pawns);
                pawns &= pawns - 1;

                to = side ? from - 8 : from + 8;
                // check for inbetween square is empty before double push
                if ((1ULL << to) & empty) {

                    // get the double push target square
                    to = side ? from - 16 : from + 16;

                    // check if target square is empty for the double push
                    if ((1ULL << to) & empty) {

                        move.move = createMove(0, 0, MOVE_DOUBLE_PUSH, side, DUMMY, PAWNS, from, to);
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

                move.move = createMove(0, 0, MOVE_NORMAL, side, DUMMY, piece, from, to);
                moves.push_back(move);
            }
        }
    }
}

void generateCaptures(u8 side, std::vector<Move> &moves, Thread *th) {
    
    u8 from, to; 
    u64 bitboard, cPieceBB;

    u64 oppPieces = side ^ 1 ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

    u64 b, attacks;
    Move move;
    for (int p = PAWNS; p <= KING; p++) {

        b = side ? th->blackPieceBB[p] : th->whitePieceBB[p];
        attacks = 0ULL;

        if (p == PAWNS) b &= side ? NOT_RANK_2 : NOT_RANK_7;
        
        while (b) {

            from = __builtin_ctzll(b);
            b &= b - 1;

            assert(from >= 0 && from <= 63);

            if (p == PAWNS) attacks = side ? 
                    ((index_bb[from] >> 7) & NOT_A_FILE) | ((index_bb[from] >> 9) & NOT_H_FILE) :
                    ((index_bb[from] << 7) & NOT_H_FILE) | ((index_bb[from] << 9) & NOT_A_FILE);
            else if (p == KNIGHTS) attacks |= get_knight_attacks(from);            
            else if (p == BISHOPS) attacks |= Bmagic(from, th->occupied);
            else if (p == ROOKS)   attacks |= Rmagic(from, th->occupied);
            else if (p == QUEEN)   attacks |= Qmagic(from, th->occupied);
            else if (p == KING)    attacks |= get_king_attacks(from);

            attacks &= oppPieces;

            while (attacks) {

                to = __builtin_ctzll(attacks);
                attacks &= attacks - 1;

                for (u8 cPieceType = PAWNS; cPieceType < KING; cPieceType++) {

                    cPieceBB = (side ^ 1) ? th->blackPieceBB[cPieceType] : th->whitePieceBB[cPieceType];

                    if (cPieceBB & (1ULL << to)) {

                        move.move = createMove(0, 0, MOVE_CAPTURE, side, cPieceType, p, from, to);
                        moves.push_back(move);
                    }
                }
            }
        }   
    }
}


void genCastlingMoves(int ply, std::vector<Move> &moves, u8 side, Thread *th) {

    Move move;

    u8 castleFlags = th->moveStack[ply].castleFlags;

    if (side == WHITE) {
        
        if (castleFlags & CASTLE_FLAG_WHITE_QUEEN) {
           
            u64 wq_sqs = th->empty & WQ_SIDE_SQS;
            if (wq_sqs == WQ_SIDE_SQS 

                &&  !(      isSqAttacked(2, WHITE, th) 
                        ||  isSqAttacked(3, WHITE, th) 
                        ||  isSqAttacked(4, WHITE, th))) {
                    
                move.move = createMove(0, 0, MOVE_CASTLE, WHITE, ROOKS, KING, 4, 2);
                moves.push_back(move);
            }
        }
        
        if (castleFlags & CASTLE_FLAG_WHITE_KING) {
            
            u64 wk_sqs = th->empty & WK_SIDE_SQS;
            
            if (wk_sqs == WK_SIDE_SQS 

                &&  !(      isSqAttacked(4, WHITE, th) 
                        ||  isSqAttacked(5, WHITE, th)
                        ||  isSqAttacked(6, WHITE, th))) {

                move.move = createMove(0, 1, MOVE_CASTLE, WHITE, ROOKS, KING, 4, 6);
                moves.push_back(move);
            }
        }
    } else {
        
        if (castleFlags & CASTLE_FLAG_BLACK_QUEEN) {

            u64 bq_sqs = th->empty & BQ_SIDE_SQS;
            
            if (bq_sqs == BQ_SIDE_SQS 

                &&  !(      isSqAttacked(58, BLACK, th) 
                        ||  isSqAttacked(59, BLACK, th)
                        ||  isSqAttacked(60, BLACK, th))) {
                    
                move.move = createMove(0, 2, MOVE_CASTLE, BLACK, ROOKS, KING, 60, 58);
                moves.push_back(move);
            }
        }
        
        if (castleFlags & CASTLE_FLAG_BLACK_KING) {     
            // Shouldn't keep this in an else check since the function should generate all castling moves 
            
            u64 bk_sqs = th->empty & BK_SIDE_SQS; 
            
            if (bk_sqs == BK_SIDE_SQS 
                
                && !(       isSqAttacked(60, BLACK, th) 
                        ||  isSqAttacked(61, BLACK, th) 
                        ||  isSqAttacked(62, BLACK, th))) {
            
                move.move = createMove(0, 3, MOVE_CASTLE, BLACK, ROOKS, KING, 60, 62);
                moves.push_back(move);
            }
        }
    }
}

void genEnpassantMoves(int ply, std::vector<Move> &moves, u8 side, Thread *th) {
    
    if (th->moveStack[ply].epFlag) {
        
        u8 from;
        u8 to = th->moveStack[ply].epSquare;
        u64 target_sqs;
        u64 target_pawns;
        u64 pawns = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
        
        u64 epSquareBB = 1ULL << to;

        if (side == WHITE) {

            target_sqs = ((epSquareBB >> 7) & NOT_A_FILE) | ((epSquareBB >> 9) & NOT_H_FILE);
        } else {
            
            target_sqs = ((epSquareBB << 7) & NOT_H_FILE) | ((epSquareBB << 9) & NOT_A_FILE);
        }
        
        target_pawns = target_sqs & pawns;
        
        Move move;
        while (target_pawns) {
            
            from = __builtin_ctzll(target_pawns);
            
            target_pawns &= target_pawns - 1;
            
            move.move = createMove(0, 0, MOVE_ENPASSANT, side, PAWNS, PAWNS, from, to);

            moves.push_back(move);
        }
    }
}

void genPromotionsNormal(std::vector<Move> &moves, u8 side, Thread *th) {
    
	u8 sq;
    u8 from;
    u8 to;
	u64 pawns = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]; 
    u64 pawnsToPromote;
    u64 empty = th->empty;
    
    if (side == WHITE) {
        
        pawnsToPromote = pawns & RANK_7;
    } else {
        
        pawnsToPromote = pawns & RANK_2;
    }

    Move move;
	while (pawnsToPromote) {
        
        from = __builtin_ctzll(pawnsToPromote);
        pawnsToPromote &= pawnsToPromote - 1;

        to = side ? from - 8 : from + 8;

        if ((1ULL << to) & empty) {

            move.move = createMove(PROMOTE_TO_QUEEN, 0, MOVE_PROMOTION, side, DUMMY, PAWNS, from, to);
            moves.push_back(move);

            move.move = createMove(PROMOTE_TO_ROOK, 0, MOVE_PROMOTION, side, DUMMY, PAWNS, from, to);
            moves.push_back(move);
            
            move.move = createMove(PROMOTE_TO_BISHOP, 0, MOVE_PROMOTION, side, DUMMY, PAWNS, from, to);
            moves.push_back(move);
            
            move.move = createMove(PROMOTE_TO_KNIGHT, 0, MOVE_PROMOTION, side, DUMMY, PAWNS, from, to);
            moves.push_back(move);
        } 
    }
}

void genPromotionsAttacks(std::vector<Move> &moves, u8 side, Thread *th) {
    
    u8 sq;
    u8 from;
    u8 to;
    u64 toAttack;
    u64 fromBB;
    u64 pawns = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
    u64 pawnsToPromote;
    u64 cPieceBB;
    
    pawnsToPromote = side ? (pawns & RANK_2) : (pawns & RANK_7);

    Move move;
    while (pawnsToPromote) {
        
        from = __builtin_ctzll(pawnsToPromote);
        pawnsToPromote &= pawnsToPromote - 1;

        fromBB = 1ULL << from;
        
        if (side == WHITE) {
            
            toAttack = (((fromBB << 7) & NOT_H_FILE) | ((fromBB << 9) & NOT_A_FILE));
        } else {
            
            toAttack = (((fromBB >> 7) & NOT_A_FILE) | ((fromBB >> 9) & NOT_H_FILE));
        }

        while (toAttack) {
            
            to = __builtin_ctzll(toAttack);
            toAttack &= toAttack - 1;
             
            for (int cPiece = KNIGHTS; cPiece <= QUEEN; cPiece++) {

                cPieceBB = side ^ 1 ? th->blackPieceBB[cPiece] : th->whitePieceBB[cPiece];

                if (cPieceBB & (1ULL << to)) {

                    move.move = createMove(PROMOTE_TO_QUEEN, 0, MOVE_PROMOTION, side, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                    
                    move.move = createMove(PROMOTE_TO_ROOK, 0, MOVE_PROMOTION, side, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                    
                    move.move = createMove(PROMOTE_TO_BISHOP, 0, MOVE_PROMOTION, side, cPiece, PAWNS, from, to);
                    moves.push_back(move);

                    move.move = createMove(PROMOTE_TO_KNIGHT, 0, MOVE_PROMOTION, side, cPiece, PAWNS, from, to);
                    moves.push_back(move);
                }
            }
        }
    }
}


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


bool isValidMove(const u8 side, const int ply, const u32 move, Thread *th) {

    
    if (move == NO_MOVE) return false;
    
    u8 opponent = side ^ 1;

    u8 fromSq = from_sq(move);
    u8 toSq = to_sq(move);

    u8 piece = pieceType(move);
    u8 capturePiece = cPieceType(move);
    
    u64 pieceBB = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
    u64 capturePieceBB = opponent ? th->blackPieceBB[capturePiece] : th->whitePieceBB[capturePiece];

    int moveType = move_type(move);

    if (moveType == MOVE_NORMAL || moveType == MOVE_DOUBLE_PUSH || moveType == MOVE_CAPTURE) {

        if (moveType == MOVE_DOUBLE_PUSH) {

            u64 inBetweenStraightLineBB = inBetween(fromSq, toSq);
            
            if (inBetweenStraightLineBB & th->occupied) return false;            
        }

        if (piece == ROOKS) {

            u64 rAttacks = Rmagic(fromSq, th->occupied);

            if (!(rAttacks & (1ULL << toSq))) return false;
        }

        if (piece == BISHOPS) {

            u64 bAttacks = Bmagic(fromSq , th->occupied);

            if (!(bAttacks & (1ULL << toSq))) return false;
        }

        if (piece == QUEEN) {

            u64 qAttacks = Qmagic(fromSq, th->occupied);

            if (!(qAttacks & (1ULL << toSq))) return false;
        }

        // finally check for pieces on their squares
        if (moveType == MOVE_NORMAL || moveType == MOVE_DOUBLE_PUSH) {

            return ((1ULL << fromSq) & pieceBB) && ((1ULL << toSq) & th->empty);
        } 
        else if (moveType == MOVE_CAPTURE) {

            return ((1ULL << fromSq) & pieceBB) && ((1ULL << toSq) & capturePieceBB);
        }
    }

    if (moveType == MOVE_CASTLE) { // Todo check logic


        u64 inBetweenStraightLineBB = inBetween(fromSq, toSq);
        
        if (inBetweenStraightLineBB & th->occupied) return false;            
   

        u8 castleFlags = th->moveStack[ply].castleFlags;

        u8 castleDirection = castleDir(move);
    
        if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {
            
            return castleFlags & CASTLE_FLAG_WHITE_QUEEN;
        } 
        else if (castleDirection == WHITE_CASTLE_KING_SIDE) {

            return castleFlags & CASTLE_FLAG_WHITE_KING;
        }
        else if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {

            return castleFlags & CASTLE_FLAG_BLACK_QUEEN;
        }
        else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

            return castleFlags & CASTLE_FLAG_BLACK_KING;
        }
    }

    if (moveType == MOVE_ENPASSANT) {

        return th->moveStack[ply].epFlag && th->moveStack[ply].epSquare == toSq;
    }
    
    if (moveType == MOVE_PROMOTION) {

        if (capturePiece == DUMMY) {

            return ((1ULL << fromSq) & pieceBB) && ((1ULL << toSq) & th->empty);
        } else {

            return ((1ULL << fromSq) & pieceBB) && ((1ULL << toSq) & capturePieceBB);
        }
    }

    return false;
}


int GetTopIdx(std::vector<Move> &moves) {
  
    int m = 0;
    int n = moves.size();
    for (int i = m + 1; i < n; i++) {

        if (moves[i].score > moves[m].score) {

            m = i;
        }
    }

    return m;
}


void scoreCaptureMoves(Thread *th, MOVE_LIST *moveList) {

    int atk_piece, to, cap_piece, mt;
    u32 move;

    for (auto &m : moveList->moves) {

        atk_piece = pieceType(m.move);
        to = to_sq(m.move);
        cap_piece = cPieceType(m.move);

        mt = move_type(m.move);

        if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) 
            cap_piece = PAWNS;

        m.score = th->capture_history_score[atk_piece][to][cap_piece];
    }
}

void scoreNormalMoves(int side, int ply, Thread *th, MOVE_LIST *moveList) {

    int score;

    u32 previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

    for (auto &m : moveList->moves) {

        m.score = th->historyScore[side][from_sq(m.move)][to_sq(m.move)];

        if (    previousMove != NO_MOVE    
            &&  m.move == th->counterMove[side][from_sq(previousMove)][to_sq(previousMove)]) {

            m.score += BONUS_COUNTER_MOVE;
        }
    }
}



Move getNextMove(int ply, int side, Thread *th, MOVE_LIST *moveList) {



    switch (moveList->stage) {

        case PLAY_HASH_MOVE : {

            moveList->stage = GEN_CAPTURES;

            if (isValidMove(side, ply, moveList->ttMove, th)) {
                    
                Move m;

                m.move = moveList->ttMove;
                
                return m;           
            }
        }
        
        
        //fallthrough

        case GEN_CAPTURES : {

            moveList->moves.clear();
            genAttacks(ply, moveList->moves, side, th);

            scoreCaptureMoves(th, moveList);
            
            moveList->stage = PLAY_CAPTURES;
        }
        
        // fallthrough
        
        case PLAY_CAPTURES : {

            if (moveList->moves.size() > 0) {

                int index = GetTopIdx(moveList->moves);
                
                Move m = moveList->moves[index];

                moveList->moves.erase(moveList->moves.begin() + index);
                
                if (m.move == moveList->ttMove) {
    
                    return getNextMove(ply, side, th, moveList);
                }

                if (SEE(m.move, side, th) < 0) { // bad capture

                    moveList->badCaptures.push_back(m);
                    
                    return getNextMove(ply, side, th, moveList);
                } 

                return m;
            }   

            moveList->stage = PLAY_KILLER_MOVE_1;
        }
        
        //fallthrough

        case PLAY_KILLER_MOVE_1 : {

            moveList->stage = PLAY_KILLER_MOVE_2;

            u32 killerMove1 = th->moveStack[ply].killerMoves[0];

            if (    !moveList->skipQuiets
                &&  killerMove1 != moveList->ttMove
                &&  isValidMove(side, ply, killerMove1, th)) {
                    
                Move m;

                m.move = killerMove1;
               
                return m;           
            }
        }
        
        //fallthrough

        case PLAY_KILLER_MOVE_2 : {

            moveList->stage = PLAY_COUNTER_MOVE;

            u32 killerMove2 = th->moveStack[ply].killerMoves[1];

            if (    !moveList->skipQuiets    
                &&  killerMove2 != moveList->ttMove
                &&  isValidMove(side, ply, killerMove2, th)) {
                    
                Move m;

                m.move = killerMove2;
               
                return m;           
            }
        }


        // fallthrough

        case PLAY_COUNTER_MOVE : {

            moveList->stage = GEN_PROMOTIONS;

            if (    !moveList->skipQuiets
                &&  isValidMove(side, ply, moveList->counterMove, th)) {

                Move m;

                m.move = moveList->counterMove;

                return m;
            }           
        }


         // fallthrough
        // ignore skipQuiets for promotions 
        case GEN_PROMOTIONS : {

            moveList->moves.clear();

            genPromotionsAttacks(moveList->moves, side, th);
            genPromotionsNormal(moveList->moves, side, th);

            for (auto &m : moveList->moves) {

                m.score = 1000; // give equal score for promotions
            }
            
            moveList->stage = PLAY_PROMOTIONS;              
        }
        
        //fallthrough 

        case PLAY_PROMOTIONS : {

            if (moveList->moves.size() > 0) {

                int index = GetTopIdx(moveList->moves);
                
                Move m = moveList->moves[index];
                
                moveList->moves.erase(moveList->moves.begin() + index);
                    
                if (m.move == moveList->ttMove) {
    
                    return getNextMove(ply, side, th, moveList);
                }

                return m;
            }

            moveList->stage = PLAY_BAD_CAPTURES;
        }


        //fallthrough

        case PLAY_BAD_CAPTURES : {

            if (moveList->badCaptures.size() > 0) {

                int index = GetTopIdx(moveList->badCaptures);
                    
                Move m = moveList->badCaptures[index];

                moveList->badCaptures.erase(moveList->badCaptures.begin() + index);

                return m.move == moveList->ttMove ? getNextMove(ply, side, th, moveList) : m;
            }

            moveList->stage = GEN_QUIETS;
        }


        //fallthrough

        case GEN_QUIETS : { // generate all non-capture moves excluding promotions

            if (!moveList->skipQuiets) {

                moveList->moves.clear();

                genCastlingMoves(ply, moveList->moves, side, th);
                generatePushes(side, moveList->moves, th);
                
                scoreNormalMoves(side, ply, th, moveList);                 
            }

            moveList->stage = PLAY_QUIETS;
        }

        //fallthrough

        case PLAY_QUIETS : {

            if (    !moveList->skipQuiets
                &&  moveList->moves.size() > 0) {

                int index = GetTopIdx(moveList->moves);
                    
                Move m = moveList->moves[index];

                moveList->moves.erase(moveList->moves.begin() + index);

                if (    m.move == moveList->ttMove 
                    ||  m.move == moveList->counterMove
                    ||  m.move == th->moveStack[ply].killerMoves[0] 
                    ||  m.move == th->moveStack[ply].killerMoves[1]) {
                  
                    return getNextMove(ply, side, th, moveList);
                }

                return m;
            }


            moveList->stage = STAGE_DONE;


            Move noMove;
            noMove.move = NO_MOVE;

            return noMove;
        }

        case STAGE_DONE:

            Move noMove;
            noMove.move = NO_MOVE;

            return noMove;
    }


    Move noMove;
    noMove.move = NO_MOVE;

    return noMove;
}


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

u32 createMove(u32 promotion_type, u32 castleDir, u32 move_type, u32 side,
               u32 c_piece, u32 piece, u32 from, u32 to) {
    
    return (0ULL | promotion_type << 24 | castleDir << 22 | move_type << 19 
        | side << 18 | c_piece << 15 | piece << 12 | from << 6 | to);
}

