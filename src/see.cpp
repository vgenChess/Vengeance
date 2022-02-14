#include <assert.h>
#include <iostream>

#include "see.h"
#include "movegen.h"

int SEE_VALUE[8] = {0, 100, 300, 300, 500, 900, 2000, 0}; 

u64 attacksTo(u64 occ, u8 square, u8 sideToMove, Thread *th) { // TODO check logic

	u64 attacks = 0ULL;
	u64 sqBitboard = 1ULL << square;

	if (sideToMove) {

		attacks |= ((((sqBitboard << 7) & NOT_H_FILE) | ((sqBitboard << 9) & NOT_A_FILE)) & th->blackPieceBB[PAWNS]);
	} else {

		attacks |= ((((sqBitboard >> 7) & NOT_A_FILE) | ((sqBitboard >> 9) & NOT_H_FILE)) & th->whitePieceBB[PAWNS]);
	}

	/* check if a knight is attacking a square */
	attacks |= (get_knight_attacks(square) & (sideToMove ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS]));

	/* check if a bishop or queen is attacking a square */
	attacks |= (Bmagic(square, occ)
					& ((sideToMove ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS]) | (sideToMove ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])));

	/* check if a rook or queen is attacking a square */
	attacks |= (Rmagic(square, occ)
					& ((sideToMove ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS]) | (sideToMove ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])));

	/* check if a king is attacking a sq */
	attacks |= (get_king_attacks(square) & (sideToMove ? th->blackPieceBB[KING] : th->whitePieceBB[KING]));

	return attacks & occ;
}

int SEE(u32 move, u8 sideToMove, Thread *th) {

	int moveType = move_type(move);	
	if (moveType == MOVE_CASTLE || moveType == MOVE_ENPASSANT || moveType == MOVE_PROMOTION) return 100;

	int from = from_sq(move);
	int to = to_sq(move);

	u8 piece = pieceType(move);
	u8 target = cPieceType(move);

	u64 occ = (th->occupied ^ (1ULL << from)) | (1ULL << to);
	u64 attackers = attacksTo(occ, to, WHITE, th) | attacksTo(occ, to, BLACK, th); // TODO check logic

	u64 diag = 
			th->whitePieceBB[BISHOPS] |	th->blackPieceBB[BISHOPS] 
		|	th->whitePieceBB[QUEEN] | th->blackPieceBB[QUEEN];

	u64 straight = 
			th->whitePieceBB[ROOKS] | th->blackPieceBB[ROOKS] 
		|	th->whitePieceBB[QUEEN] | th->blackPieceBB[QUEEN];


	int swapList[32], idx = 1;

	swapList[0] = SEE_VALUE[target];


	u8 stm = sideToMove ^ 1;

	u64 stmAttackers = attackers & (stm ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	if (!stmAttackers)
		return swapList[0];

	u64 fromSet;
	do {
		
		for (piece = PAWNS; !(fromSet = (stmAttackers & stm ? th->blackPieceBB[piece] : th->whitePieceBB[piece])); piece++) {
			assert(piece < KING);
		}

		assert(fromSet > 0);

		fromSet = fromSet & -fromSet; // take single bit
		occ &= fromSet;


		// check for X-ray attacks through the vacated square
		
		if (piece == PAWNS || piece == BISHOPS || piece == QUEEN) {
			attackers |= Bmagic(to, occ) & diag;
		}
	    if (piece == ROOKS || piece == QUEEN) {
	    	attackers |= Rmagic(to, occ) & straight;
	    }
		
		attackers &= occ; // remove pieces that have already attacked 


		assert(idx < 32);

		swapList[idx] = -swapList[idx - 1] + SEE_VALUE[target];

		target = piece;
		
		idx++;

		stm ^= 1;

		stmAttackers = attackers & (stm ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]); 

		// Stop after a king capture
		if (piece == KING && stmAttackers) {

			assert(idx < 32);

			swapList[idx++] = SEE_VALUE[KING];

			break;
		}
	} while (stmAttackers);

	while (--idx) {

		swapList[idx-1] = std::min(-swapList[idx], swapList[idx-1]);
	}

	return swapList[0];
}


void debugSEE(char ch, int square) {

	u8 piece, side;
	switch (ch) {

		case 'k' : side = BLACK; piece = KING; break;
		case 'q' : side = BLACK; piece = QUEEN; break;
		case 'r' : side = BLACK; piece = ROOKS; break;
		case 'b' : side = BLACK; piece = BISHOPS; break;
		case 'n' : side = BLACK; piece = KNIGHTS; break;
		case 'p' : side = BLACK; piece = PAWNS; break;


		case 'K' : side = WHITE; piece = KING; break;
		case 'Q' : side = WHITE; piece = QUEEN; break;
		case 'R' : side = WHITE; piece = ROOKS; break;
		case 'B' : side = WHITE; piece = BISHOPS; break;
		case 'N' : side = WHITE; piece = KNIGHTS; break;
		case 'P' : side = WHITE; piece = PAWNS; break;
	
		default : piece = DUMMY; break;
	}

	if (piece == DUMMY) {

		std::cout << "Invalid piece" << std::endl;
		return;
	}

	if (square < 1 || square > 64) {

		std::cout << "Invalid square" << std::endl;
		return;
	}

	std::vector<Move> moves; 

   	genMoves(0, moves, side, &initThread);
	
	u8 p, to;
	u32 move;
	bool flag = false; 
	for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i)
	{
		
		move = (*i).move;
		
		p = pieceType(move);
		to = to_sq(move);

		if (p == piece && square == to + 1) {

			flag = true;
			break;
		}		
	}

	if (!flag) {

		std::cout << "Invalid move" << std::endl;
		return;
	}

	int value = SEE(move, side, &initThread);

	std::cout << "See score = " << value << std::endl;
}


