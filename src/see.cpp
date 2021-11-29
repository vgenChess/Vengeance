#include <assert.h>
#include <iostream>

#include "see.h"
#include "movegen.h"

int pieceValue[8] = {0, 100, 300, 300, 500, 900, 2000, 0}; // TODO use tuned piece values

u64 attacksTo(u64 occ, u8 square, u8 sideToMove, Thread *th) {

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

// SEE - Static Exchange Evaluation
int see(u32 move, u8 sideToMove, Thread *th) {

	if (!th->whitePieceBB[KING] || !th->blackPieceBB[KING]) {
		
		return -1000;
	} 

	int frSq = from_sq(move);
	int toSq = to_sq(move);

	u8 aPiece = pieceType(move);
	u8 cPiece = cPieceType(move);

	int whiteKingSq = __builtin_ctzll(th->whitePieceBB[KING]);
	int blackKingSq = __builtin_ctzll(th->blackPieceBB[KING]);

	u64 occ = th->occupied;

	int swapList[32];
	int index = 1;

	u8 capture;

	bool isEnpassant = (move_type(move) == MOVE_ENPASSANT);

	// Handle en-passant
	if (isEnpassant) {

		occ ^= (1ULL << (sideToMove ? toSq + 8 : toSq - 8));
		capture = PAWNS;
	} else {

		capture = cPiece;
	}

	assert(capture != KING);

	swapList[0] = pieceValue[capture];

	u64 fromSet = (1ULL << frSq);

	occ ^= fromSet;

	u64 attackers = attacksTo(occ, toSq, WHITE, th) | attacksTo(occ, toSq, BLACK, th);

	const bool is_prank = (1ULL << toSq) & (sideToMove ? RANK_2 : RANK_7);
	
	// Handle Promotion
	if (aPiece == PAWNS && is_prank) {

		swapList[0] += pieceValue[QUEEN] - pieceValue[PAWNS];
		capture = QUEEN;
	} else {

		capture = aPiece;
	}

	sideToMove ^= 1;

	u64 sideToMoveAttackers = attackers & (sideToMove ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	u64 kingPinners = 0ULL;

	// remove pinned pieces
	kingPinners = pinners(sideToMove ? blackKingSq : whiteKingSq, sideToMove, th);
	if (kingPinners & occ) {

		sideToMoveAttackers &= ~pinned(kingPinners, sideToMove ? blackKingSq : whiteKingSq, sideToMove, th);
	}

	if (!sideToMoveAttackers) {

		return swapList[0];
	}

	assert(swapList[0] > 0);

	u64 rooks = th->whitePieceBB[ROOKS] | th->blackPieceBB[ROOKS];
	u64 bishops = th->whitePieceBB[BISHOPS] | th->blackPieceBB[BISHOPS];
	u64 queens = th->whitePieceBB[QUEEN] | th->blackPieceBB[QUEEN];
	
	do {

		for (aPiece = PAWNS; !(sideToMoveAttackers & (sideToMove ? th->blackPieceBB[aPiece] : th->whitePieceBB[aPiece])); aPiece++) { 

			assert(aPiece < KING);
		}		

		occ ^= (sideToMoveAttackers & (sideToMove ? th->blackPieceBB[aPiece] : th->whitePieceBB[aPiece]));

		attackers |= ((Bmagic(toSq, occ) & (bishops | queens)) | (Rmagic(toSq, occ) & (rooks | queens))) ;
		
		attackers &= occ;

		swapList[index] = -swapList[index - 1] + pieceValue[capture];

		// Handle Promotion
		if (aPiece == PAWNS && is_prank) {

			swapList[index] += pieceValue[QUEEN] - pieceValue[PAWNS];
			capture = QUEEN;
		} else {

			capture = aPiece;
		}

		index++;

		sideToMove ^= 1;

		sideToMoveAttackers = attackers & (sideToMove ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]); 

 		// remove pinned pieces
		kingPinners = pinners(sideToMove ? blackKingSq : whiteKingSq, sideToMove, th);
		if (kingPinners & occ) {

			sideToMoveAttackers &= ~pinned(kingPinners, sideToMove ? blackKingSq : whiteKingSq, sideToMove, th);
		}

		// Stop after a king capture
		if (aPiece == KING && sideToMoveAttackers) {

			swapList[index++] = pieceValue[KING];

			break;
		}
	} while (sideToMoveAttackers);

	while (--index) {

		swapList[index-1] = std::min(-swapList[index], swapList[index-1]);
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

	int value = see(move, side, &initThread);

	std::cout << "See score = " << value << std::endl;
}




