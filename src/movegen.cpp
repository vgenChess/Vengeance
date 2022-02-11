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


bool checkIfValidMove(int ply, u8 side, u32 move, Thread* th) {

    if (move != NO_MOVE) {    

        u8 p = pieceType(move);
        // u8 cp = cPieceType(move);
        // u8 mType = move_type(move);
        
        u64 pbb = side ? th->blackPieceBB[p] : th->whitePieceBB[p];
        // u64 cpbb = (side ^ 1) ? th->blackPieceBB[cp] : th->whitePieceBB[cp];

        int fromSQ = from_sq(move);     
        // int toSQ = to_sq(move);

        if ((1ULL << fromSQ) & pbb) return true;


        // if ((1ULL << fromSQ) & pbb) {

        //     if (mType == MOVE_NORMAL) {

        //         if (~th->occupied & (1ULL << toSQ)) return true;
        //     } else if (mType == MOVE_CASTLE) {
                
        //         Move moveList[MAX_MOVES]; 
        //         int numberOfMoves = genCastlingMoves(ply, moveList, 0, side, th);
        //         for (int i = 0; i < numberOfMoves; i++) {

        //             if (moveList[i].move == move)   return true;
        //         }
        //     } else if (mType == MOVE_ENPASSANT) {

        //         if (th->moveStack[ply].epFlag && (~th->occupied & (1ULL << toSQ))) return true;
        //     } else if (mType == MOVE_CAPTURE) {

        //         if ((1ULL << toSQ) & cpbb) return true;
        //     } else if (mType == MOVE_PROMOTION) {

        //         if (cp == DUMMY && ~th->occupied & (1ULL << toSQ)) return true;
        //         if (cp != DUMMY && (1ULL << toSQ) & cpbb) return true;
        //     }
        // }
    }

    return false;
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



void getMoves(const int ply, const int side, std::vector<Move> &moves, const int stage,
    const bool isQuiescense, Thread *th) {    
    

    const bool IS_ROOT_NODE = ply == 0;
        
    moves.clear(); 
 
    switch (stage) {


        case STAGE_HASH_MOVE : {
            

            u32 ttMove = th->moveStack[ply].ttMove;
            
            if (ttMove != NO_MOVE) {
            
                if (checkIfValidMove(ply, side, ttMove, th)) {
                    
                    Move m;
                    m.move = ttMove;

                    moves.push_back(m);            
                }
            }

            break; 
        }


        case STAGE_PROMOTIONS : {


            genPromotionsNormal(moves, side, th);
            
            genPromotionsAttacks(moves, side, th);

            break;
        }


        case STAGE_CAPTURES : {


            genAttacks(ply, moves, side, th);
            
            u8 atk_piece, cap_piece, mt;

            int to;

            int score;
            u32 move;

            for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i) {

                move = (*i).move;

                atk_piece = pieceType(move);
                to = to_sq(move);
                cap_piece = cPieceType(move);

                mt = move_type(move);

                if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) cap_piece = PAWNS;

                (*i).score = th->capture_history_score[atk_piece][to][cap_piece];
            }

            break;
        }


        case STAGE_KILLER_MOVES : {

    
            genCastlingMoves(ply, moves, side, th);
    
            generatePushes(side, moves, th);
   
    
            u32 previousMove = IS_ROOT_NODE ? NO_MOVE : th->moveStack[ply - 1].move;

            int score;
            u32 move;
            for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i)
            {
        
                move = (*i).move;
                (*i).score = th->historyScore[side][from_sq(move)][to_sq(move)];

                if (    previousMove != NO_MOVE    
                    &&  move == th->counterMove[side][from_sq(previousMove)][to_sq(previousMove)]) {

                    (*i).score += BONUS_COUNTER_MOVE;
                }
            }

            break;
        }
    }
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
    
    return (    0ULL 
            |   promotion_type << 24 
            |   castleDir << 22
            |   move_type << 19 
            |   side << 18 
            |   c_piece << 15
            |   piece << 12 
            |   from << 6 
            |   to);
}

/* Extract data from a move structure */
#define promType(move)           (move & 0x3000000) >> 24
#define castleDir(move)        (move & 0xC00000) >> 22
#define move_type(move)         (move & 0x380000) >> 19
#define colorType(move)        (move & 0x40000) >> 18
#define cPieceType(move)      (move & 0x38000) >> 15
#define pieceType(move)        (move & 0x7000) >> 12
#define from_sq(move)              (move & 0xFC0) >> 6
#define to_sq(move)                move & 0x3F


