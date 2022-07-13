//
//  make_unmake.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <stdbool.h>
#include <iostream>

#include "make_unmake.h"
#include "utility.h"
#include "thread.h"
#include "functions.h"
#include "constants.h"
#include "zobrist.h"
#include "nnue.h"
#include <string.h>

U64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;
U8 rookCastleFlagMask[64];

void make_move(int ply, U32 move, GameInfo *gi) {
    
    
    const U8 mtype = move_type(move);
    
    const U8 stm = colorType(move);
    const U8 opp = stm ^ 1; 
    
    const U8 fromSq = from_sq(move);
    const U8 toSq = to_sq(move);
    
    const U8 piece = pieceType(move);
    const U8 target = cPieceType(move);
    
    
    U64 from_bb = 1ULL << fromSq;
    U64 to_bb = 1ULL << toSq; 
    
    U64 from_to_bb = from_bb ^ to_bb;
    
    
    gi->undoMoveStack[ply].castleFlags = gi->moveStack[ply].castleFlags;
    gi->undoMoveStack[ply].epFlag = gi->moveStack[ply].epFlag;
    gi->undoMoveStack[ply].epSquare = gi->moveStack[ply].epSquare;
    gi->undoMoveStack[ply].hashKey = gi->hashKey;
    gi->undoMoveStack[ply].pawnsHashKey = gi->pawnsHashKey;
    
    
    const int mhCounter = gi->moves_history_counter + ply; // Needs investigation
    
    gi->undoMoveStack[ply].fiftyMovesCounter = gi->movesHistory[mhCounter].fiftyMovesCounter;
    
    
    if ( gi->moveStack[ply].epFlag != 0) {
        
        const auto epSqBitboard = 1ULL << gi->moveStack[ply].epSquare;
        
        if (	 epSqBitboard & A_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_A_FILE;
        else if (epSqBitboard & B_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_B_FILE;
        else if (epSqBitboard & C_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_C_FILE;
        else if (epSqBitboard & D_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_D_FILE;
        else if (epSqBitboard & E_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_E_FILE;
        else if (epSqBitboard & F_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_F_FILE;
        else if (epSqBitboard & G_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_G_FILE;
        else if (epSqBitboard & H_FILE)	gi->hashKey ^= Zobrist::objZobrist.KEY_EP_H_FILE;
    }
    
    
    // making any move will make the ep move invalid
    gi->moveStack[ply].epFlag = 0;
    
    
    
    switch (mtype) {
        
        
        case MOVE_NORMAL: {
            
            
            gi->movesHistory[mhCounter].fiftyMovesCounter++;
            
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[piece][stm][fromSq]
            ^ Zobrist::objZobrist.zobristKey[piece][stm][toSq];
            
            
            if (piece == PAWNS) {
                
                gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[fromSq]
                                ^ Zobrist::objZobrist.pawnZobristKey[toSq];
                
                gi->movesHistory[mhCounter].fiftyMovesCounter = 0;
            } else if (piece == KING) { // TODO check logic
                
                if (stm == WHITE) {
                    
                    if ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_QUEEN;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                    }
                    
                    if ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_KING;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                    }
                } else {
                    
                    if ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_QUEEN;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                    }
                    
                    if ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_KING;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                    }
                }
            } else if (piece == ROOKS) { // TODO check logic
                
                gi->moveStack[ply].castleFlags &= rookCastleFlagMask[fromSq];
                
                if (fromSq == 0 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                } else if (fromSq == 56 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                } else if (fromSq == 7 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                } else if (fromSq == 63 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                }
            }
            
            break;
        }
        
        
        
        
        
        case MOVE_CAPTURE: {
            
            gi->movesHistory[mhCounter].fiftyMovesCounter = 0;
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
                
                gi->whitePieceBB[target] ^= to_bb;
                gi->whitePieceBB[PIECES] ^= to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
                
                gi->blackPieceBB[target] ^= to_bb;
                gi->blackPieceBB[PIECES] ^= to_bb;
            }
            
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[piece][stm][fromSq]
                        ^ Zobrist::objZobrist.zobristKey[piece][stm][toSq];
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[target][opp][toSq];
            
            
            if (piece == PAWNS)	
                gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[fromSq]
                                ^ Zobrist::objZobrist.pawnZobristKey[toSq];
            
            if (target == PAWNS)	
                gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[toSq];
            
            
            // update castle flags
            if (piece == KING) {
                if (stm == WHITE) {
                    
                    int castleQueenSide = gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN;
                    if (castleQueenSide) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_QUEEN;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                    }
                    
                    int castleKingSide = gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING;
                    if (castleKingSide) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_KING;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                    }
                } else {
                    
                    int castleQueenSide = gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN;
                    if (castleQueenSide) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_QUEEN;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                    }
                    
                    int castleKingSide = gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING;
                    if (castleKingSide) {
                        
                        gi->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_KING;
                        gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                    }
                }
            } else if (piece == ROOKS) {
                
                gi->moveStack[ply].castleFlags &= rookCastleFlagMask[fromSq];
                
                if (fromSq == 0 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                } else if (fromSq == 56 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                } else if (fromSq == 7 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                } else if (fromSq == 63 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                }
            }
            
            
            if (target == ROOKS) {
                
                gi->moveStack[ply].castleFlags &= rookCastleFlagMask[toSq];
                
                if (toSq == 0 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                } else if (toSq == 56 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                } else if (toSq == 7 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                } else if (toSq == 63 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                }
            }
            
            break;
        }
        
        
        
        
        case MOVE_DOUBLE_PUSH: {
            
            
            gi->movesHistory[mhCounter].fiftyMovesCounter++;
            gi->moveStack[ply].epFlag = 1;
            gi->moveStack[ply].epSquare = stm ? toSq + 8 : toSq - 8;
            
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][stm][fromSq]
                        ^ Zobrist::objZobrist.zobristKey[PAWNS][stm][toSq];
            
            gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[fromSq]
                            ^ Zobrist::objZobrist.pawnZobristKey[toSq];
            
            break;
        }
        
        
        
        
        case MOVE_ENPASSANT: {
            
            
            
            gi->movesHistory[mhCounter].fiftyMovesCounter = 0;
            
            
            //  en_passant capture
            if (stm == WHITE) {
                
                gi->whitePieceBB[PAWNS] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
                
                gi->blackPieceBB[PAWNS] ^= (to_bb >> 8);
                gi->blackPieceBB[PIECES] ^= (to_bb >> 8);
            } else {
                
                
                gi->blackPieceBB[PAWNS] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
                
                gi->whitePieceBB[PAWNS] ^= (to_bb << 8);
                gi->whitePieceBB[PIECES] ^= (to_bb << 8);
            }
            
            
            U8 sqOfCapturedPawn = stm ? toSq + 8 : toSq - 8;	
            
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][stm][fromSq]
                        ^ Zobrist::objZobrist.zobristKey[PAWNS][stm][toSq];
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][opp][sqOfCapturedPawn];
            
            gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[fromSq]
                            ^ Zobrist::objZobrist.pawnZobristKey[toSq];
            gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[sqOfCapturedPawn];
            
            
            break;
        }
        
        
        
        
        case MOVE_CASTLE: {
            
            
            gi->movesHistory[mhCounter].fiftyMovesCounter++;
            
            U8 castleDirection = castleDir(move);
            
            if (stm == WHITE) {
                
                if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {
                    
                    //clear out king and rook
                    gi->whitePieceBB[piece] ^= 0x0000000000000010U;
                    gi->whitePieceBB[target] ^= 0x0000000000000001U;
                    
                    // set king and rook
                    gi->whitePieceBB[piece] ^= 0x0000000000000004U;
                    gi->whitePieceBB[target] ^= 0x0000000000000008U;
                    
                    // update pieces
                    gi->whitePieceBB[PIECES] ^= 0x0000000000000011U;
                    gi->whitePieceBB[PIECES] ^= 0x000000000000000CU;
                    
                    
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[KING][WHITE][4]
                                ^ Zobrist::objZobrist.zobristKey[KING][WHITE][2];
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][WHITE][0]
                                ^ Zobrist::objZobrist.zobristKey[ROOKS][WHITE][3];
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                    
                } else if (castleDirection == WHITE_CASTLE_KING_SIDE) {
                    
                    //clear out king and rook
                    gi->whitePieceBB[piece] ^= 0x0000000000000010U;
                    gi->whitePieceBB[target] ^= 0x0000000000000080U;
                    
                    // set king and rook
                    gi->whitePieceBB[piece] ^= 0x0000000000000040U;
                    gi->whitePieceBB[target] ^= 0x0000000000000020U;
                    
                    // update pieces
                    gi->whitePieceBB[PIECES] ^= 0x0000000000000090U;
                    gi->whitePieceBB[PIECES] ^= 0x0000000000000060U;
                    
                    
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[KING][WHITE][4]
                                ^ Zobrist::objZobrist.zobristKey[KING][WHITE][6];
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][WHITE][7]
                                ^ Zobrist::objZobrist.zobristKey[ROOKS][WHITE][5];
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                    
                }
                
                gi->moveStack[ply].castleFlags &= ~(CASTLE_FLAG_WHITE_KING | CASTLE_FLAG_WHITE_QUEEN);
                
            } else {
                
                if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {
                    
                    //clear out king and rook
                    gi->blackPieceBB[piece] ^= 0x1000000000000000U;
                    gi->blackPieceBB[target] ^= 0x0100000000000000U;
                    
                    // set king and rook
                    gi->blackPieceBB[piece] ^= 0x0400000000000000U;
                    gi->blackPieceBB[target] ^= 0x0800000000000000U;
                    
                    // update pieces
                    gi->blackPieceBB[PIECES] ^= 0x1100000000000000U;
                    gi->blackPieceBB[PIECES] ^= 0x0C00000000000000U;
                    
                    
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[KING][BLACK][60]
                                ^ Zobrist::objZobrist.zobristKey[KING][BLACK][58];
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][BLACK][56]
                                ^ Zobrist::objZobrist.zobristKey[ROOKS][BLACK][59];
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                    
                    
                } else if (castleDirection == BLACK_CASTLE_KING_SIDE) {
                    
                    //clear out king and rook
                    gi->blackPieceBB[piece] ^= 0x1000000000000000U;
                    gi->blackPieceBB[target] ^= 0x8000000000000000U;
                    
                    // set king and rook
                    gi->blackPieceBB[piece] ^= 0x4000000000000000U;
                    gi->blackPieceBB[target] ^= 0x2000000000000000U;
                    
                    // update pieces
                    gi->blackPieceBB[PIECES] ^= 0x9000000000000000U;
                    gi->blackPieceBB[PIECES] ^= 0x6000000000000000U;
                    
                    
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[KING][BLACK][60]
                                ^ Zobrist::objZobrist.zobristKey[KING][BLACK][62];
                    gi->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][BLACK][63]
                                ^ Zobrist::objZobrist.zobristKey[ROOKS][BLACK][61];
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                    
                }
                
                gi->moveStack[ply].castleFlags &= ~(CASTLE_FLAG_BLACK_KING | CASTLE_FLAG_BLACK_QUEEN);
            }
            
            break;
        }
        
        
        
        
        
        case MOVE_PROMOTION: {
            
            
            // promotion involves a pawn move
            gi->movesHistory[mhCounter].fiftyMovesCounter = 0;
            
            U8 promoteTo = DUMMY;
            U8 pType = promType(move);
            
            if      (pType == PROMOTE_TO_QUEEN)		promoteTo = QUEEN;
            else if (pType == PROMOTE_TO_ROOK)		promoteTo = ROOKS;
            else if (pType == PROMOTE_TO_BISHOP)	promoteTo = BISHOPS;
            else if (pType == PROMOTE_TO_KNIGHT)	promoteTo = KNIGHTS;
            
            
            if (stm) {
                
                gi->blackPieceBB[PAWNS] ^= from_bb;
                gi->blackPieceBB[promoteTo] ^= to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else {
                
                gi->whitePieceBB[PAWNS] ^= from_bb;
                gi->whitePieceBB[promoteTo] ^= to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            
            if (target != DUMMY) {
                
                if (stm) {
                    
                    gi->whitePieceBB[target] ^= to_bb;
                    gi->whitePieceBB[PIECES] ^= to_bb;
                } else {
                    
                    gi->blackPieceBB[target] ^= to_bb;
                    gi->blackPieceBB[PIECES] ^= to_bb;
                }
                
                gi->hashKey ^= Zobrist::objZobrist.zobristKey[target][opp][toSq];
                
            }
            
            
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][stm][fromSq];
            gi->hashKey ^= Zobrist::objZobrist.zobristKey[promoteTo][stm][toSq];
            
            
            if (target == ROOKS) {
                
                
                if (toSq == 0 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
                } else if (toSq == 56 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
                } else if (toSq == 7 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
                } else if (toSq == 63 && ( gi->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {
                    
                    gi->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE;
                }
                
                
                gi->moveStack[ply].castleFlags &= rookCastleFlagMask[toSq];
            }
            
            
            gi->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[fromSq];
            
            break;
        }
        
        
        default: {
            
            break;
        }
    }
    
    
    gi->occupied = gi->whitePieceBB[PIECES] | gi->blackPieceBB[PIECES];
    gi->empty = ~( gi->occupied);
    
    
    gi->hashKey ^= Zobrist::objZobrist.KEY_SIDE_TO_MOVE;


    // update accumulator after each move

    if (!gi->isInit && piece != DUMMY) {


        memcpy( gi->undoMoveStack[ply].accumulator[WHITE],
                 gi->accumulator[WHITE], sizeof(int16_t) * NN_SIZE);
        memcpy( gi->undoMoveStack[ply].accumulator[BLACK],
                 gi->accumulator[BLACK], sizeof(int16_t) * NN_SIZE);


        if (piece == KING || mtype == MOVE_CASTLE) {

            refresh_accumulator( gi, WHITE);
            refresh_accumulator( gi, BLACK);
        } else {

            // TODO check logic

            std::vector<int> white_added_features;
            std::vector<int> white_removed_features;
            std::vector<int> black_added_features;
            std::vector<int> black_removed_features;


            auto p = piece - 1;
            auto cp = target - 1;

            auto wKingSq = GET_POSITION( gi->whitePieceBB[KING]);
            auto bKingSq = GET_POSITION( gi->blackPieceBB[KING]) ^ 63;

            auto fromW = fromSq;
            auto toW = toSq;

            auto fromB = fromSq ^ 63;
            auto toB = toSq ^ 63;

            auto indexW = (p << 1) + stm;
            auto indexB = (p << 1) + (1 - stm);


            auto featureW = (640 * wKingSq) + (64 * indexW) + (fromW);
            auto featureB = (640 * bKingSq) + (64 * indexB) + (fromB);

            // remove the feature index from the initial position
            white_removed_features.push_back(featureW);
            black_removed_features.push_back(featureB);


            if (mtype == MOVE_PROMOTION) {

                int pType = promType(move);

                if (pType == PROMOTE_TO_QUEEN)  p = QUEEN - 1;
                if (pType == PROMOTE_TO_ROOK)   p = ROOKS - 1;
                if (pType == PROMOTE_TO_BISHOP) p = BISHOPS - 1;
                if (pType == PROMOTE_TO_KNIGHT) p = KNIGHTS - 1;

                indexW = (p << 1) + stm;
                indexB = (p << 1) + (1 - stm);
            }

            featureW = (640 * wKingSq) + (64 * indexW) + (toW);
            featureB = (640 * bKingSq) + (64 * indexB) + (toB);


            // add the feature index to the destination position
            white_added_features.push_back(featureW);
            black_added_features.push_back(featureB);


            auto target_piece_side = 1 - stm;

            if ((  mtype == MOVE_CAPTURE
                || mtype == MOVE_PROMOTION
                || mtype == MOVE_ENPASSANT)
                && target != DUMMY) {

                indexW = (cp << 1) + target_piece_side;
                indexB = (cp << 1) + 1 - target_piece_side;

                const auto toSquare =
                    mtype == MOVE_ENPASSANT ? (stm ? toSq + 8 : toSq - 8) : toSq;

                toW = toSquare;
                toB = toSquare ^ 63;

                featureW = (640 * wKingSq) + (64 * indexW) + (toW);
                featureB = (640 * bKingSq) + (64 * indexB) + (toB);

                // remove any feature index of the captured piece
                white_removed_features.push_back(featureW);
                black_removed_features.push_back(featureB);
            }

            update_accumulator( gi, white_removed_features, white_added_features, WHITE);
            update_accumulator( gi, black_removed_features, black_added_features, BLACK);
        }
    }
}



void unmake_move(int ply, U32 move, GameInfo *gi ) {
    
    U8 castleDirection = castleDir(move);
    
    const U8 stm = colorType(move);
    
    const U8 fromSq = from_sq(move);
    const U8 toSq = to_sq(move);
    
    const U8 piece = pieceType(move);
    const U8 target = cPieceType(move);
    
    U64 from_bb = 1ULL << fromSq;
    U64 to_bb = 1ULL << toSq; 
    
    U64 from_to_bb = from_bb ^ to_bb;
    
    
    
    gi->moveStack[ply].castleFlags = gi->undoMoveStack[ply].castleFlags;
    gi->moveStack[ply].epFlag = gi->undoMoveStack[ply].epFlag;
    gi->moveStack[ply].epSquare = gi->undoMoveStack[ply].epSquare;
    
    gi->hashKey = gi->undoMoveStack[ply].hashKey;
    gi->pawnsHashKey = gi->undoMoveStack[ply].pawnsHashKey;
    
    gi->movesHistory[gi->moves_history_counter + ply].fiftyMovesCounter =
        gi->undoMoveStack[ply].fiftyMovesCounter;
    
    
    
    switch (move_type(move)) {
        
        
        case MOVE_NORMAL: {
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            break;
        }
        
        
        case MOVE_CAPTURE: {
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
                
                gi->whitePieceBB[target] ^= to_bb;
                gi->whitePieceBB[PIECES] ^= to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
                
                gi->blackPieceBB[target] ^= to_bb;
                gi->blackPieceBB[PIECES] ^= to_bb;
            }
            
            break;
        }
        
        
        case MOVE_DOUBLE_PUSH: {
            
            if (stm) {  
                
                gi->blackPieceBB[piece] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else { 
                
                gi->whitePieceBB[piece] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            break;
        }
        
        
        case MOVE_ENPASSANT: {
            
            if (stm == WHITE) {
                
                gi->whitePieceBB[PAWNS] ^= from_to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
                
                gi->blackPieceBB[PAWNS] ^= (to_bb >> 8);
                gi->blackPieceBB[PIECES] ^= (to_bb >> 8);
                
            } else {
                
                gi->blackPieceBB[PAWNS] ^= from_to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
                
                gi->whitePieceBB[PAWNS] ^= (to_bb << 8);
                gi->whitePieceBB[PIECES] ^= (to_bb << 8);
            }
            
            break;
        }
        
        
        case MOVE_CASTLE: {
            
            if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {
                
                // clear king and rook
                gi->whitePieceBB[piece] ^= 0x0000000000000004U;
                gi->whitePieceBB[target] ^= 0x0000000000000008U;
                
                //set king and rook
                gi->whitePieceBB[piece] ^= 0x0000000000000010U;
                gi->whitePieceBB[target] ^= 0x0000000000000001U;
                
                // update pieces
                gi->whitePieceBB[PIECES] ^= 0x000000000000000CU;
                gi->whitePieceBB[PIECES] ^= 0x0000000000000011U;
            } else if (castleDirection == WHITE_CASTLE_KING_SIDE) {
                
                gi->whitePieceBB[piece] ^= 0x0000000000000040U;
                gi->whitePieceBB[target] ^= 0x0000000000000020U;
                
                gi->whitePieceBB[piece] ^= 0x0000000000000010U;
                gi->whitePieceBB[target] ^= 0x0000000000000080U;
                
                gi->whitePieceBB[PIECES] ^= 0x0000000000000060U;
                gi->whitePieceBB[PIECES] ^= 0x0000000000000090U;
            } else if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {
                
                gi->blackPieceBB[piece] ^= 0x0400000000000000U;
                gi->blackPieceBB[target] ^= 0x0800000000000000U;
                
                gi->blackPieceBB[piece] ^= 0x1000000000000000U;
                gi->blackPieceBB[target] ^= 0x0100000000000000U;
                
                gi->blackPieceBB[PIECES] ^= 0x0C00000000000000U;
                gi->blackPieceBB[PIECES] ^= 0x1100000000000000U;
            } else if (castleDirection == BLACK_CASTLE_KING_SIDE) {
                
                gi->blackPieceBB[piece] ^= 0x4000000000000000U;
                gi->blackPieceBB[target] ^= 0x2000000000000000U;
                
                gi->blackPieceBB[piece] ^= 0x1000000000000000U;
                gi->blackPieceBB[target] ^= 0x8000000000000000U;
                
                gi->blackPieceBB[PIECES] ^= 0x6000000000000000U;
                gi->blackPieceBB[PIECES] ^= 0x9000000000000000U;
            }
            
            break;
        }
        
        
        case MOVE_PROMOTION: {
            
            U8 p = -1;
            U8 pType = promType(move);
            
            if  	(	pType == PROMOTE_TO_QUEEN)	p = QUEEN;
            else if (	pType == PROMOTE_TO_ROOK)	p = ROOKS;
            else if (	pType == PROMOTE_TO_BISHOP) p = BISHOPS;
            else if (	pType == PROMOTE_TO_KNIGHT) p = KNIGHTS;
            
            if (stm) {
                
                gi->blackPieceBB[PAWNS] ^= from_bb;
                gi->blackPieceBB[p] ^= to_bb;
                gi->blackPieceBB[PIECES] ^= from_to_bb;
            } else {
                
                gi->whitePieceBB[PAWNS] ^= from_bb;
                gi->whitePieceBB[p] ^= to_bb;
                gi->whitePieceBB[PIECES] ^= from_to_bb;
            }
            
            if (target != DUMMY) {
                
                if (stm) {
                    
                    gi->whitePieceBB[target] ^= to_bb;
                    gi->whitePieceBB[PIECES] ^= to_bb;
                } else {
                    
                    gi->blackPieceBB[target] ^= to_bb;
                    gi->blackPieceBB[PIECES] ^= to_bb;
                }
            }
            
            break;
        }
    }


    gi->occupied = gi->whitePieceBB[PIECES] | gi->blackPieceBB[PIECES];
    gi->empty = ~( gi->occupied);


    if (!gi->isInit) {


        memcpy( gi->accumulator[WHITE],
                 gi->undoMoveStack[ply].accumulator[WHITE],
               sizeof(int16_t) * NN_SIZE);

        memcpy( gi->accumulator[BLACK],
                 gi->undoMoveStack[ply].accumulator[BLACK],
               sizeof(int16_t) * NN_SIZE);
    }
}



void makeNullMove(int ply, GameInfo *gi ) { // Needs investigation
    
    const auto mhCounter = gi->moves_history_counter + ply; // Needs investigation
    
    gi->undoMoveStack[ply].fiftyMovesCounter = gi->movesHistory[mhCounter].fiftyMovesCounter;
    
    gi->undoMoveStack[ply].castleFlags = gi->moveStack[ply].castleFlags;
    gi->undoMoveStack[ply].epFlag = gi->moveStack[ply].epFlag;
    gi->undoMoveStack[ply].epSquare = gi->moveStack[ply].epSquare;
    gi->undoMoveStack[ply].hashKey = gi->hashKey;
    gi->undoMoveStack[ply].pawnsHashKey = gi->pawnsHashKey;
    
    // making any move will make the ep move invalid
    gi->moveStack[ply].epFlag = 0;
    
    
    gi->hashKey ^= Zobrist::objZobrist.KEY_SIDE_TO_MOVE;


    if (!gi->isInit) {

        memcpy( gi->undoMoveStack[ply].accumulator[WHITE],
                 gi->accumulator[WHITE], sizeof(int16_t) * NN_SIZE);
        memcpy( gi->undoMoveStack[ply].accumulator[BLACK],
                 gi->accumulator[BLACK], sizeof(int16_t) * NN_SIZE);
    }
}


void unmakeNullMove(int ply, GameInfo *gi ) {
    
    gi->moveStack[ply].castleFlags = gi->undoMoveStack[ply].castleFlags;
    gi->moveStack[ply].epFlag = gi->undoMoveStack[ply].epFlag;
    gi->moveStack[ply].epSquare = gi->undoMoveStack[ply].epSquare;
    
    gi->hashKey = gi->undoMoveStack[ply].hashKey;
    gi->pawnsHashKey = gi->undoMoveStack[ply].pawnsHashKey;
    
    const auto mhCounter = gi->moves_history_counter + ply; // Needs investigation
    
    gi->movesHistory[mhCounter].fiftyMovesCounter = gi->undoMoveStack[ply].fiftyMovesCounter;


    if (!gi->isInit) {


        memcpy( gi->accumulator[WHITE],
                 gi->undoMoveStack[ply].accumulator[WHITE],
               sizeof(int16_t) * NN_SIZE);

        memcpy( gi->accumulator[BLACK],
                 gi->undoMoveStack[ply].accumulator[BLACK],
               sizeof(int16_t) * NN_SIZE);
    }
}



