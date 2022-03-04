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
#include "hash.h"
#include "thread.h"
#include "functions.h"
#include "constants.h"
#include "globals.h"


U64 KEY_SIDE_TO_MOVE;

U64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
U64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
U64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
U64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;

U64 KEY_EP_A_FILE;
U64 KEY_EP_B_FILE;
U64 KEY_EP_C_FILE;
U64 KEY_EP_D_FILE;
U64 KEY_EP_E_FILE;
U64 KEY_EP_F_FILE;
U64 KEY_EP_G_FILE;
U64 KEY_EP_H_FILE;

U64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;

U64 zobrist[U8_MAX_PIECES][U8_MAX_SIDES][U8_MAX_SQUARES];
U64 pawnZobristKey[U8_MAX_SQUARES];

U64 index_bb[U8_MAX_SQUARES];
U8 rookCastleFlagMask[64];


void make_move(int ply, U32 move, Thread *th) {


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
	
	
	th->undoMoveStack[ply].castleFlags = th->moveStack[ply].castleFlags;
	th->undoMoveStack[ply].epFlag = th->moveStack[ply].epFlag;
	th->undoMoveStack[ply].epSquare = th->moveStack[ply].epSquare;
	th->undoMoveStack[ply].hashKey = th->hashKey;
	th->undoMoveStack[ply].pawnsHashKey = th->pawnsHashKey;


	const int mhCounter = th->moves_history_counter + ply; // Needs investigation 

	th->undoMoveStack[ply].fiftyMovesCounter = th->movesHistory[mhCounter].fiftyMovesCounter;


	if (th->moveStack[ply].epFlag != 0) {

		U64 epSqBitboard = 1ULL << th->moveStack[ply].epSquare;

		if (	 epSqBitboard & A_FILE)	th->hashKey ^= KEY_EP_A_FILE;
		else if (epSqBitboard & B_FILE)	th->hashKey ^= KEY_EP_B_FILE;
		else if (epSqBitboard & C_FILE)	th->hashKey ^= KEY_EP_C_FILE;
		else if (epSqBitboard & D_FILE)	th->hashKey ^= KEY_EP_D_FILE;
		else if (epSqBitboard & E_FILE)	th->hashKey ^= KEY_EP_E_FILE;
		else if (epSqBitboard & F_FILE)	th->hashKey ^= KEY_EP_F_FILE;
		else if (epSqBitboard & G_FILE)	th->hashKey ^= KEY_EP_G_FILE;
		else if (epSqBitboard & H_FILE)	th->hashKey ^= KEY_EP_H_FILE;
	}


	// making any move will make the ep move invalid
	th->moveStack[ply].epFlag = 0;



	switch (mtype) {


		case MOVE_NORMAL: {

			
			quiet++;


    		th->movesHistory[mhCounter].fiftyMovesCounter++;	


			if (stm) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 

				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			}


			th->hashKey ^= zobrist[piece][stm][fromSq] ^ zobrist[piece][stm][toSq];
			

			if (piece == PAWNS) {

				th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

    			th->movesHistory[mhCounter].fiftyMovesCounter = 0;	
			} else if (piece == KING) { // TODO check logic
				
				if (stm == WHITE) {

					if (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN) {

						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_QUEEN;
						th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
					}

					if (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING) {

						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_KING;
						th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
					}
				} else {

					if (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN) {

						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_QUEEN;
						th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
					}

					if (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING) {
						
						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_KING;
						th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
					}
				}
			} else if (piece == ROOKS) { // TODO check logic

				th->moveStack[ply].castleFlags &= rookCastleFlagMask[fromSq];

				if (fromSq == 0 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
				} else if (fromSq == 56 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
				} else if (fromSq == 7 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
				} else if (fromSq == 63 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
				}
			}

			break;
		}





		case MOVE_CAPTURE: {
			

			cap++;

		
			th->movesHistory[mhCounter].fiftyMovesCounter = 0;	

			if (stm) {  
	
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;

				th->whitePieceBB[target] ^= to_bb;
				th->whitePieceBB[PIECES] ^= to_bb;
			} else { 

				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			
				th->blackPieceBB[target] ^= to_bb;
				th->blackPieceBB[PIECES] ^= to_bb;
			}
			
			th->hashKey ^= zobrist[piece][stm][fromSq] ^ zobrist[piece][stm][toSq];
			th->hashKey ^= zobrist[target][opp][toSq];


			if (piece == PAWNS)	
				th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

			if (target == PAWNS)	
				th->pawnsHashKey ^= pawnZobristKey[toSq];
	

			// update castle flags
			if (piece == KING) {
				if (stm == WHITE) {

					int castleQueenSide = th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN;
					if (castleQueenSide) {
					
						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_QUEEN;
						th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
					}

					int castleKingSide = th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING;
					if (castleKingSide) {
					
						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_WHITE_KING;
						th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
					}
				} else {

					int castleQueenSide = th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN;
					if (castleQueenSide) {
					
						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_QUEEN;
						th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
					}
					
					int castleKingSide = th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING;
					if (castleKingSide) {
					
						th->moveStack[ply].castleFlags &= ~CASTLE_FLAG_BLACK_KING;
						th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
					}
				}
			} else if (piece == ROOKS) {

				th->moveStack[ply].castleFlags &= rookCastleFlagMask[fromSq];

				if (fromSq == 0 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
				} else if (fromSq == 56 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
				} else if (fromSq == 7 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
				} else if (fromSq == 63 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
				}
			}


			if (target == ROOKS) {

				th->moveStack[ply].castleFlags &= rookCastleFlagMask[toSq];

				if (toSq == 0 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
				} else if (toSq == 56 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
				} else if (toSq == 7 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
				} else if (toSq == 63 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
				}
			}

			break;
		}




		case MOVE_DOUBLE_PUSH: {


			quiet++;


			th->movesHistory[mhCounter].fiftyMovesCounter++;	
			th->moveStack[ply].epFlag = 1;			
			th->moveStack[ply].epSquare = stm ? toSq + 8 : toSq - 8;


			if (stm) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;		
			} else { 
				
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;	
			}
			

			th->hashKey ^= zobrist[PAWNS][stm][fromSq] ^ zobrist[PAWNS][stm][toSq];

			th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

			break;
		}




		case MOVE_ENPASSANT: {


			ep++;
			
			
			th->movesHistory[mhCounter].fiftyMovesCounter = 0;


			//  en_passant capture
			if (stm == WHITE) {

				th->whitePieceBB[PAWNS] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb; 

				th->blackPieceBB[PAWNS] ^= (to_bb >> 8);
				th->blackPieceBB[PIECES] ^= (to_bb >> 8);
			} else {


				th->blackPieceBB[PAWNS] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb; 

				th->whitePieceBB[PAWNS] ^= (to_bb << 8);
				th->whitePieceBB[PIECES] ^= (to_bb << 8);
			}


			U8 sqOfCapturedPawn = stm ? toSq + 8 : toSq - 8;	

			th->hashKey ^= zobrist[PAWNS][stm][fromSq] ^ zobrist[PAWNS][stm][toSq];
			th->hashKey ^= zobrist[PAWNS][opp][sqOfCapturedPawn];

			th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];
			th->pawnsHashKey ^= pawnZobristKey[sqOfCapturedPawn]; 


			break;
		}




		case MOVE_CASTLE: {

			
			cas++;


			th->movesHistory[mhCounter].fiftyMovesCounter++;	
			
			U8 castleDirection = castleDir(move);

			if (stm == WHITE) {
				
				if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {

					//clear out king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000010U;
					th->whitePieceBB[target] ^= 0x0000000000000001U;

					// set king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000004U;
					th->whitePieceBB[target] ^= 0x0000000000000008U;

					// update pieces
					th->whitePieceBB[PIECES] ^= 0x0000000000000011U;
					th->whitePieceBB[PIECES] ^= 0x000000000000000CU;


					th->hashKey ^= zobrist[KING][WHITE][4] ^ zobrist[KING][WHITE][2];
					th->hashKey ^= zobrist[ROOKS][WHITE][0] ^ zobrist[ROOKS][WHITE][3];
					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;

				} else if (castleDirection == WHITE_CASTLE_KING_SIDE) {

					//clear out king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000010U;
					th->whitePieceBB[target] ^= 0x0000000000000080U;

					// set king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000040U;
					th->whitePieceBB[target] ^= 0x0000000000000020U;

					// update pieces
					th->whitePieceBB[PIECES] ^= 0x0000000000000090U;
					th->whitePieceBB[PIECES] ^= 0x0000000000000060U;


					th->hashKey ^= zobrist[KING][WHITE][4] ^ zobrist[KING][WHITE][6];
					th->hashKey ^= zobrist[ROOKS][WHITE][7] ^ zobrist[ROOKS][WHITE][5];
					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;

				}

				th->moveStack[ply].castleFlags &= ~(CASTLE_FLAG_WHITE_KING | CASTLE_FLAG_WHITE_QUEEN);

			} else {
				
				if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {

					//clear out king and rook
					th->blackPieceBB[piece] ^= 0x1000000000000000U;
					th->blackPieceBB[target] ^= 0x0100000000000000U;

					// set king and rook
					th->blackPieceBB[piece] ^= 0x0400000000000000U;
					th->blackPieceBB[target] ^= 0x0800000000000000U;

					// update pieces
					th->blackPieceBB[PIECES] ^= 0x1100000000000000U;
					th->blackPieceBB[PIECES] ^= 0x0C00000000000000U;


					th->hashKey ^= zobrist[KING][BLACK][60] ^ zobrist[KING][BLACK][58];
					th->hashKey ^= zobrist[ROOKS][BLACK][56] ^ zobrist[ROOKS][BLACK][59];
					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;


				} else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

					//clear out king and rook
					th->blackPieceBB[piece] ^= 0x1000000000000000U;
					th->blackPieceBB[target] ^= 0x8000000000000000U;

					// set king and rook
					th->blackPieceBB[piece] ^= 0x4000000000000000U;
					th->blackPieceBB[target] ^= 0x2000000000000000U;

					// update pieces
					th->blackPieceBB[PIECES] ^= 0x9000000000000000U;
					th->blackPieceBB[PIECES] ^= 0x6000000000000000U;


					th->hashKey ^= zobrist[KING][BLACK][60] ^ zobrist[KING][BLACK][62];
					th->hashKey ^= zobrist[ROOKS][BLACK][63] ^ zobrist[ROOKS][BLACK][61];
					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;

				}

				th->moveStack[ply].castleFlags &= ~(CASTLE_FLAG_BLACK_KING | CASTLE_FLAG_BLACK_QUEEN);	
			}
		
			break;
		}





		case MOVE_PROMOTION: {


			prom++;

			// promotion involves a pawn move
			th->movesHistory[mhCounter].fiftyMovesCounter = 0;	
		
			U8 promoteTo = DUMMY;
			U8 pType = promType(move);

				 if (pType == PROMOTE_TO_QUEEN)		promoteTo = QUEEN;
			else if (pType == PROMOTE_TO_ROOK)		promoteTo = ROOKS;
			else if (pType == PROMOTE_TO_BISHOP)	promoteTo = BISHOPS;
			else if (pType == PROMOTE_TO_KNIGHT)	promoteTo = KNIGHTS;
			

			if (stm) {

				th->blackPieceBB[PAWNS] ^= from_bb;
				th->blackPieceBB[promoteTo] ^= to_bb;
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else {
			
				th->whitePieceBB[PAWNS] ^= from_bb;
				th->whitePieceBB[promoteTo] ^= to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}


			if (target != DUMMY) {

				if (stm) {

					th->whitePieceBB[target] ^= to_bb;
					th->whitePieceBB[PIECES] ^= to_bb;
				} else {

					th->blackPieceBB[target] ^= to_bb;
					th->blackPieceBB[PIECES] ^= to_bb;
				}
		
				th->hashKey ^= zobrist[target][opp][toSq]; 

			}

					
			th->hashKey ^= zobrist[PAWNS][stm][fromSq];
			th->hashKey ^= zobrist[promoteTo][stm][toSq];


			if (target == ROOKS) {


				if (toSq == 0 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_QUEEN)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
				} else if (toSq == 56 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_QUEEN)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
				} else if (toSq == 7 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_WHITE_KING)) {

					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_KING_SIDE;
				} else if (toSq == 63 && (th->moveStack[ply].castleFlags & CASTLE_FLAG_BLACK_KING)) {

					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_KING_SIDE;
				}


				th->moveStack[ply].castleFlags &= rookCastleFlagMask[toSq];
			}


			th->pawnsHashKey ^= pawnZobristKey[fromSq];

			break;
		}


		default: {

			break;
		}
	}


	th->occupied = th->whitePieceBB[PIECES] | th->blackPieceBB[PIECES];	
	th->empty = ~(th->occupied);		


	th->hashKey ^= KEY_SIDE_TO_MOVE;
	th->pawnsHashKey ^= KEY_SIDE_TO_MOVE;
}




void unmake_move(int ply, U32 move, Thread *th) {
 
	U8 castleDirection = castleDir(move);

	const U8 stm = colorType(move);
	
	const U8 fromSq = from_sq(move);
	const U8 toSq = to_sq(move);

	const U8 piece = pieceType(move);
	const U8 target = cPieceType(move);

	U64 from_bb = 1ULL << fromSq;
	U64 to_bb = 1ULL << toSq; 

	U64 from_to_bb = from_bb ^ to_bb;



	th->moveStack[ply].castleFlags = th->undoMoveStack[ply].castleFlags;
	th->moveStack[ply].epFlag = th->undoMoveStack[ply].epFlag;
	th->moveStack[ply].epSquare = th->undoMoveStack[ply].epSquare;
	
	th->hashKey = th->undoMoveStack[ply].hashKey;
	th->pawnsHashKey = th->undoMoveStack[ply].pawnsHashKey;

	th->movesHistory[th->moves_history_counter + ply].fiftyMovesCounter = th->undoMoveStack[ply].fiftyMovesCounter;



	switch (move_type(move)) {


		case MOVE_NORMAL: {
			
			if (stm) {  
			
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 
			
				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			}

			break;
		}


		case MOVE_CAPTURE: {

			if (stm) {  
			
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;

				th->whitePieceBB[target] ^= to_bb;
				th->whitePieceBB[PIECES] ^= to_bb;
			} else { 
			
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;

				th->blackPieceBB[target] ^= to_bb;
				th->blackPieceBB[PIECES] ^= to_bb;
			}

			break;
		}


		case MOVE_DOUBLE_PUSH: {

			if (stm) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 
				
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}

			break;
		}


		case MOVE_ENPASSANT: {

			if (stm == WHITE) {

				th->whitePieceBB[PAWNS] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb; 

				th->blackPieceBB[PAWNS] ^= (to_bb >> 8);
				th->blackPieceBB[PIECES] ^= (to_bb >> 8);

			} else {

				th->blackPieceBB[PAWNS] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;

				th->whitePieceBB[PAWNS] ^= (to_bb << 8);
				th->whitePieceBB[PIECES] ^= (to_bb << 8);
			}

			break;
		}


		case MOVE_CASTLE: {

			if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {

				// clear king and rook
				th->whitePieceBB[piece] ^= 0x0000000000000004U;
				th->whitePieceBB[target] ^= 0x0000000000000008U;

				//set king and rook
				th->whitePieceBB[piece] ^= 0x0000000000000010U;
				th->whitePieceBB[target] ^= 0x0000000000000001U;

				// update pieces
				th->whitePieceBB[PIECES] ^= 0x000000000000000CU;
				th->whitePieceBB[PIECES] ^= 0x0000000000000011U;
			} else if (castleDirection == WHITE_CASTLE_KING_SIDE) {

				th->whitePieceBB[piece] ^= 0x0000000000000040U;
				th->whitePieceBB[target] ^= 0x0000000000000020U;

				th->whitePieceBB[piece] ^= 0x0000000000000010U;
				th->whitePieceBB[target] ^= 0x0000000000000080U;

				th->whitePieceBB[PIECES] ^= 0x0000000000000060U;
				th->whitePieceBB[PIECES] ^= 0x0000000000000090U;
			} else if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {

				th->blackPieceBB[piece] ^= 0x0400000000000000U;
				th->blackPieceBB[target] ^= 0x0800000000000000U;

				th->blackPieceBB[piece] ^= 0x1000000000000000U;
				th->blackPieceBB[target] ^= 0x0100000000000000U;

				th->blackPieceBB[PIECES] ^= 0x0C00000000000000U;
				th->blackPieceBB[PIECES] ^= 0x1100000000000000U;
			} else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

				th->blackPieceBB[piece] ^= 0x4000000000000000U;
				th->blackPieceBB[target] ^= 0x2000000000000000U;

				th->blackPieceBB[piece] ^= 0x1000000000000000U;
				th->blackPieceBB[target] ^= 0x8000000000000000U;

				th->blackPieceBB[PIECES] ^= 0x6000000000000000U;
				th->blackPieceBB[PIECES] ^= 0x9000000000000000U;
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

				th->blackPieceBB[PAWNS] ^= from_bb;
				th->blackPieceBB[p] ^= to_bb;
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else {
			
				th->whitePieceBB[PAWNS] ^= from_bb;
				th->whitePieceBB[p] ^= to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}

			if (target != DUMMY) {

				if (stm) {

					th->whitePieceBB[target] ^= to_bb;
					th->whitePieceBB[PIECES] ^= to_bb;
				} else {

					th->blackPieceBB[target] ^= to_bb;
					th->blackPieceBB[PIECES] ^= to_bb;
				}
			}

			break;
		}
	}


	th->occupied = th->whitePieceBB[PIECES] | th->blackPieceBB[PIECES];
	th->empty = ~(th->occupied);
}



void makeNullMove(int ply, Thread *th) { // Needs investigation
	
	const int mhCounter = th->moves_history_counter + ply; // Needs investigation 

	th->undoMoveStack[ply].fiftyMovesCounter = th->movesHistory[mhCounter].fiftyMovesCounter;

	th->undoMoveStack[ply].castleFlags = th->moveStack[ply].castleFlags;
	th->undoMoveStack[ply].epFlag = th->moveStack[ply].epFlag;
	th->undoMoveStack[ply].epSquare = th->moveStack[ply].epSquare;
	th->undoMoveStack[ply].hashKey = th->hashKey;
	th->undoMoveStack[ply].pawnsHashKey = th->pawnsHashKey;

	// making any move will make the ep move invalid
	th->moveStack[ply].epFlag = 0;


	th->hashKey ^= KEY_SIDE_TO_MOVE;
	th->pawnsHashKey ^= KEY_SIDE_TO_MOVE;
}


void unmakeNullMove(int ply, Thread *th) {
	
	th->moveStack[ply].castleFlags = th->undoMoveStack[ply].castleFlags;
	th->moveStack[ply].epFlag = th->undoMoveStack[ply].epFlag;
	th->moveStack[ply].epSquare = th->undoMoveStack[ply].epSquare;
	
	th->hashKey = th->undoMoveStack[ply].hashKey;
	th->pawnsHashKey = th->undoMoveStack[ply].pawnsHashKey;
	
	int mhCounter = th->moves_history_counter + ply; // Needs investigation

	th->movesHistory[mhCounter].fiftyMovesCounter = th->undoMoveStack[ply].fiftyMovesCounter;
}



