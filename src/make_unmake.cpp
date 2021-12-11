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
#include "cerebrum.h"


u64 KEY_SIDE_TO_MOVE;

u64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
u64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
u64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
u64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;

u64 KEY_EP_A_FILE;
u64 KEY_EP_B_FILE;
u64 KEY_EP_C_FILE;
u64 KEY_EP_D_FILE;
u64 KEY_EP_E_FILE;
u64 KEY_EP_F_FILE;
u64 KEY_EP_G_FILE;
u64 KEY_EP_H_FILE;

u64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;

u64 zobrist[8][2][64];
u64 pawnZobristKey[64];

u64 index_bb[INDEX_BB_SIZE];
u8 rookCastleFlagMask[64];


void make_move(int ply, u32 move, Thread *th) {



	const u8 mtype = move_type(move);

	const u8 sideToMove = colorType(move);
	const u8 opponent = sideToMove ^ 1; 

	const u8 fromSq = from_sq(move);
	const u8 toSq = to_sq(move);

	const u8 piece = pieceType(move);
	const u8 c_piece = cPieceType(move);


	u64 from_bb = 1ULL << fromSq;
	u64 to_bb = 1ULL << toSq; 

	u64 from_to_bb = from_bb ^ to_bb;
	
	
	th->undoMoveStack[ply].castleFlags = th->moveStack[ply].castleFlags;
	th->undoMoveStack[ply].epFlag = th->moveStack[ply].epFlag;
	th->undoMoveStack[ply].epSquare = th->moveStack[ply].epSquare;
	th->undoMoveStack[ply].hashKey = th->hashKey;
	th->undoMoveStack[ply].pawnsHashKey = th->pawnsHashKey;



	const int mhCounter = th->moves_history_counter + ply; // Needs investigation 

	th->undoMoveStack[ply].fiftyMovesCounter = th->movesHistory[mhCounter].fiftyMovesCounter;


	if (th->moveStack[ply].epFlag != 0) {

		u64 epSqBitboard = 1ULL << th->moveStack[ply].epSquare;

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


	memcpy(&th->save, &th->board, sizeof(th->board));
 
 if (piece == KING || mtype == MOVE_PROMOTION || mtype == MOVE_ENPASSANT) {
 
    th->board.pieces[0] = &th->whitePieceBB[PAWNS];
    th->board.pieces[1] = &th->blackPieceBB[PAWNS];
     nn_inputs_upd_all(&nn, &th->board);
  } else {
 
     nn_inputs_mov_piece(&nn, &th->board, piece-1, (sideToMove== WHITE ? 0 : 1), fromSq, toSq);
      
      if (mtype==MOVE_CAPTURE) {
          nn_inputs_del_piece(&nn, &th->board, c_piece-1, (sideToMove == WHITE ? 1 : 0), toSq);
     }
 }
 





	switch (mtype) {


		case MOVE_NORMAL: {

			
			quiet++;


    		th->movesHistory[mhCounter].fiftyMovesCounter++;	


			if (sideToMove) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 

				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			}



			th->hashKey ^= zobrist[piece][sideToMove][fromSq] ^ zobrist[piece][sideToMove][toSq];
			


			if (piece == PAWNS) {

				th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

    			th->movesHistory[mhCounter].fiftyMovesCounter = 0;	
			} else if (piece == KING) { // TODO check logic
				
				if (sideToMove == WHITE) {

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

			if (sideToMove) {  
	
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;

				th->whitePieceBB[c_piece] ^= to_bb;
				th->whitePieceBB[PIECES] ^= to_bb;
			} else { 

				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			
				th->blackPieceBB[c_piece] ^= to_bb;
				th->blackPieceBB[PIECES] ^= to_bb;
			}
			
			th->hashKey ^= zobrist[piece][sideToMove][fromSq] ^ zobrist[piece][sideToMove][toSq];
			th->hashKey ^= zobrist[c_piece][opponent][toSq];

			if (piece == PAWNS)	
				th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

			if (c_piece == PAWNS)	
				th->pawnsHashKey ^= pawnZobristKey[toSq];
	

			// update castle flags
			if (piece == KING) {
				if (sideToMove == WHITE) {

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


			if (c_piece == ROOKS) {

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
			th->moveStack[ply].epSquare = sideToMove ? toSq + 8 : toSq - 8;


			if (sideToMove) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;		
			} else { 
				
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;	
			}
			

			th->hashKey ^= zobrist[PAWNS][sideToMove][fromSq] ^ zobrist[PAWNS][sideToMove][toSq];

			th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];

			break;
		}




		case MOVE_ENPASSANT: {


			ep++;
			
			
			th->movesHistory[mhCounter].fiftyMovesCounter = 0;


			//  en_passant capture
			if (sideToMove == WHITE) {

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


			const int sq = sideToMove ? toSq + 8 : toSq - 8;	// sq visibility is within the case block

			th->hashKey ^= zobrist[PAWNS][sideToMove][fromSq] ^ zobrist[PAWNS][sideToMove][toSq];
			th->hashKey ^= zobrist[PAWNS][opponent][sq];

			th->pawnsHashKey ^= pawnZobristKey[fromSq] ^ pawnZobristKey[toSq];
			th->pawnsHashKey ^= pawnZobristKey[sq]; 

			break;
		}




		case MOVE_CASTLE: {

			
			cas++;


			th->movesHistory[mhCounter].fiftyMovesCounter++;	
			
			u8 castleDirection = castleDir(move);

			if (sideToMove == WHITE) {
				
				if (castleDirection == WHITE_CASTLE_QUEEN_SIDE) {

					//clear out king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000010U;
					th->whitePieceBB[c_piece] ^= 0x0000000000000001U;

					// set king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000004U;
					th->whitePieceBB[c_piece] ^= 0x0000000000000008U;

					// update pieces
					th->whitePieceBB[PIECES] ^= 0x0000000000000011U;
					th->whitePieceBB[PIECES] ^= 0x000000000000000CU;


					th->hashKey ^= zobrist[KING][WHITE][4] ^ zobrist[KING][WHITE][2];
					th->hashKey ^= zobrist[ROOKS][WHITE][0] ^ zobrist[ROOKS][WHITE][3];
					th->hashKey ^= KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
				} else if (castleDirection == WHITE_CASTLE_KING_SIDE) {

					//clear out king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000010U;
					th->whitePieceBB[c_piece] ^= 0x0000000000000080U;

					// set king and rook
					th->whitePieceBB[piece] ^= 0x0000000000000040U;
					th->whitePieceBB[c_piece] ^= 0x0000000000000020U;

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
					th->blackPieceBB[c_piece] ^= 0x0100000000000000U;

					// set king and rook
					th->blackPieceBB[piece] ^= 0x0400000000000000U;
					th->blackPieceBB[c_piece] ^= 0x0800000000000000U;

					// update pieces
					th->blackPieceBB[PIECES] ^= 0x1100000000000000U;
					th->blackPieceBB[PIECES] ^= 0x0C00000000000000U;


					th->hashKey ^= zobrist[KING][BLACK][60] ^ zobrist[KING][BLACK][58];
					th->hashKey ^= zobrist[ROOKS][BLACK][56] ^ zobrist[ROOKS][BLACK][59];
					th->hashKey ^= KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
				} else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

					//clear out king and rook
					th->blackPieceBB[piece] ^= 0x1000000000000000U;
					th->blackPieceBB[c_piece] ^= 0x8000000000000000U;

					// set king and rook
					th->blackPieceBB[piece] ^= 0x4000000000000000U;
					th->blackPieceBB[c_piece] ^= 0x2000000000000000U;

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
		
			uint8_t p = -1;
			uint8_t pType = promType(move);

			if (pType == PROMOTE_TO_QUEEN) p = QUEEN;
			else if (pType == PROMOTE_TO_ROOK) p = ROOKS;
			else if (pType == PROMOTE_TO_BISHOP) p = BISHOPS;
			else if (pType == PROMOTE_TO_KNIGHT) p = KNIGHTS;
			

			if (sideToMove) {

				th->blackPieceBB[PAWNS] ^= from_bb;
				th->blackPieceBB[p] ^= to_bb;
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else {
			
				th->whitePieceBB[PAWNS] ^= from_bb;
				th->whitePieceBB[p] ^= to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}


			if (c_piece != DUMMY) {

				if (sideToMove) {

					th->whitePieceBB[c_piece] ^= to_bb;
					th->whitePieceBB[PIECES] ^= to_bb;
				} else {

					th->blackPieceBB[c_piece] ^= to_bb;
					th->blackPieceBB[PIECES] ^= to_bb;
				}
		
				th->hashKey ^= zobrist[c_piece][opponent][toSq]; 
			}

					
			th->hashKey ^= zobrist[PAWNS][sideToMove][fromSq];
			th->hashKey ^= zobrist[p][sideToMove][toSq];


			if (c_piece == ROOKS) {


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




void unmake_move(int ply, u32 move, Thread *th) {

  memcpy(&th->board, &th->save, sizeof(th->save));
 
	u8 castleDirection = castleDir(move);

	const u8 sideToMove = colorType(move);
	const u8 opponent = sideToMove ^ 1; 

	const u8 fromSq = from_sq(move);
	const u8 toSq = to_sq(move);

	const u8 piece = pieceType(move);
	const u8 c_piece = cPieceType(move);

	u64 from_bb = 1ULL << fromSq;
	u64 to_bb = 1ULL << toSq; 

	u64 from_to_bb = from_bb ^ to_bb;



	th->moveStack[ply].castleFlags = th->undoMoveStack[ply].castleFlags;
	th->moveStack[ply].epFlag = th->undoMoveStack[ply].epFlag;
	th->moveStack[ply].epSquare = th->undoMoveStack[ply].epSquare;
	
	th->hashKey = th->undoMoveStack[ply].hashKey;
	th->pawnsHashKey = th->undoMoveStack[ply].pawnsHashKey;


	th->movesHistory[th->moves_history_counter + ply].fiftyMovesCounter = th->undoMoveStack[ply].fiftyMovesCounter;



	switch (move_type(move)) {


		case MOVE_NORMAL: {
			
			if (sideToMove) {  
			
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 
			
				th->whitePieceBB[piece] ^= from_to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb; 
			}

			break;
		}


		case MOVE_CAPTURE: {

			if (sideToMove) {  
			
				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;

				th->whitePieceBB[c_piece] ^= to_bb;
				th->whitePieceBB[PIECES] ^= to_bb;
			} else { 
			
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;

				th->blackPieceBB[c_piece] ^= to_bb;
				th->blackPieceBB[PIECES] ^= to_bb;
			}

			break;
		}


		case MOVE_DOUBLE_PUSH: {

			if (sideToMove) {  

				th->blackPieceBB[piece] ^= from_to_bb; 
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else { 
				
				th->whitePieceBB[piece] ^= from_to_bb; 
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}

			break;
		}


		case MOVE_ENPASSANT: {

			if (sideToMove == WHITE) {

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
				th->whitePieceBB[c_piece] ^= 0x0000000000000008U;

				//set king and rook
				th->whitePieceBB[piece] ^= 0x0000000000000010U;
				th->whitePieceBB[c_piece] ^= 0x0000000000000001U;

				// update pieces
				th->whitePieceBB[PIECES] ^= 0x000000000000000CU;
				th->whitePieceBB[PIECES] ^= 0x0000000000000011U;
			} else if (castleDirection == WHITE_CASTLE_KING_SIDE) {

				th->whitePieceBB[piece] ^= 0x0000000000000040U;
				th->whitePieceBB[c_piece] ^= 0x0000000000000020U;

				th->whitePieceBB[piece] ^= 0x0000000000000010U;
				th->whitePieceBB[c_piece] ^= 0x0000000000000080U;

				th->whitePieceBB[PIECES] ^= 0x0000000000000060U;
				th->whitePieceBB[PIECES] ^= 0x0000000000000090U;
			} else if (castleDirection == BLACK_CASTLE_QUEEN_SIDE) {

				th->blackPieceBB[piece] ^= 0x0400000000000000U;
				th->blackPieceBB[c_piece] ^= 0x0800000000000000U;

				th->blackPieceBB[piece] ^= 0x1000000000000000U;
				th->blackPieceBB[c_piece] ^= 0x0100000000000000U;

				th->blackPieceBB[PIECES] ^= 0x0C00000000000000U;
				th->blackPieceBB[PIECES] ^= 0x1100000000000000U;
			} else if (castleDirection == BLACK_CASTLE_KING_SIDE) {

				th->blackPieceBB[piece] ^= 0x4000000000000000U;
				th->blackPieceBB[c_piece] ^= 0x2000000000000000U;

				th->blackPieceBB[piece] ^= 0x1000000000000000U;
				th->blackPieceBB[c_piece] ^= 0x8000000000000000U;

				th->blackPieceBB[PIECES] ^= 0x6000000000000000U;
				th->blackPieceBB[PIECES] ^= 0x9000000000000000U;
			}

			break;
		}


		case MOVE_PROMOTION: {

			u8 p = -1;
			u8 pType = promType(move);

			if  	(	pType == PROMOTE_TO_QUEEN)	p = QUEEN;
			else if (	pType == PROMOTE_TO_ROOK)	p = ROOKS;
			else if (	pType == PROMOTE_TO_BISHOP) p = BISHOPS;
			else if (	pType == PROMOTE_TO_KNIGHT) p = KNIGHTS;
			
			if (sideToMove) {

				th->blackPieceBB[PAWNS] ^= from_bb;
				th->blackPieceBB[p] ^= to_bb;
				th->blackPieceBB[PIECES] ^= from_to_bb;
			} else {
			
				th->whitePieceBB[PAWNS] ^= from_bb;
				th->whitePieceBB[p] ^= to_bb;
				th->whitePieceBB[PIECES] ^= from_to_bb;
			}

			if (c_piece != DUMMY) {

				if (sideToMove) {

					th->whitePieceBB[c_piece] ^= to_bb;
					th->whitePieceBB[PIECES] ^= to_bb;
				} else {

					th->blackPieceBB[c_piece] ^= to_bb;
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

    memcpy(&th->save, &th->board, sizeof(th->board));
 

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

    memcpy(&th->board, &th->save, sizeof(th->save));

	th->moveStack[ply].castleFlags = th->undoMoveStack[ply].castleFlags;
	th->moveStack[ply].epFlag = th->undoMoveStack[ply].epFlag;
	th->moveStack[ply].epSquare = th->undoMoveStack[ply].epSquare;
	
	th->hashKey = th->undoMoveStack[ply].hashKey;
	th->pawnsHashKey = th->undoMoveStack[ply].pawnsHashKey;


	int mhCounter = th->moves_history_counter + ply; // Needs investigation

	th->movesHistory[mhCounter].fiftyMovesCounter = th->undoMoveStack[ply].fiftyMovesCounter;
}



