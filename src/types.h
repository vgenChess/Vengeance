#ifndef TYPES_H
#define TYPES_H

#include <vector>
/*

The primitive data types available in C++ are as follows:

----------------------------------------------------------------------------------------------------------------------------------------
Type 				Description 						Bytes *  					Range *
-----------------------------------------------------------------------------------------------------------------------------------------

char 				character or small integer 			1 	 						signed: -128 to 127
																					unsigned: 0 to 255
-----------------------------------------------------------------------------------------------------------------------------------------------

int 				integer 							short: 2 					signed short: -32,768 to 32,767
																					unsigned short: 0 to 65,535

														normal: 4                   signed: -2,147,483,648 to 2,147,483,647
																					unsigned: 0 to 4,294,967,295

														long: 4 					signed long: -2,147,483,648 to 2,147,483,647
																					unsigned long: 0 to 4,294,967,295

														long long: 8 				signed long long: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
																					unsigned long long: 0 to 18,446,744,073,709,551,615
-----------------------------------------------------------------------------------------------------------------------------------------------------																					

bool 				boolean value  						1 							true or false
----------------------------------------------------------------------------------------------------------------------------------------------

float 				floating-point number 				4 							1.17549*10-38 to 3.40282*1038
---------------------------------------------------------------------------------------------------------------------------------------------

double 				double-precision 					8 							2.22507*10-308 to 1.79769*10308
					floatingpoint number
------------------------------------------------------------------------------------------------------------------------------------------------------------

long double 		extended-precision  				12 							3.36210*10-4932 to 1.18973*104932
					floatingpoint number
--------------------------------------------------------------------------------------------------------------------------------------------------------------------					

wchar_t 			wide character or short 			2 							1 wide character
					integer
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/


typedef unsigned char 		U8;
typedef unsigned short 		U16;
typedef unsigned long 		U32;
typedef unsigned long long	U64;

enum {
	WHITE = 0, BLACK = 1
};

enum Piece {
	DUMMY = 0, PAWNS = 1, KNIGHTS = 2, BISHOPS = 3, 
	ROOKS = 4, QUEEN = 5, KING = 6, PIECES = 7
};

enum {
	VALUE_PAWN = 100, VALUE_KNIGHT = 300, VALUE_BISHOP = 300,
	VALUE_ROOK = 500, VALUE_QUEEN = 900, VALUE_KING = 2000
};

enum {
	MOVE_NORMAL, MOVE_CAPTURE, MOVE_DOUBLE_PUSH,
	MOVE_ENPASSANT, MOVE_CASTLE, MOVE_PROMOTION
};

enum {
	CASTLE_FLAG_WHITE_QUEEN = 1, CASTLE_FLAG_WHITE_KING = 2,
	CASTLE_FLAG_BLACK_QUEEN = 4, CASTLE_FLAG_BLACK_KING = 8
};

enum {
	WHITE_CASTLE_QUEEN_SIDE = 0, WHITE_CASTLE_KING_SIDE = 1,
	BLACK_CASTLE_QUEEN_SIDE = 2, BLACK_CASTLE_KING_SIDE = 3
};

enum {
	PROMOTE_TO_QUEEN = 0, PROMOTE_TO_ROOK = 1,
	PROMOTE_TO_BISHOP = 2, PROMOTE_TO_KNIGHT = 3
};

enum Stage {
	PLAY_HASH_MOVE, GEN_CAPTURES, PLAY_CAPTURES, PLAY_KILLER_MOVE_1,
	PLAY_KILLER_MOVE_2, PLAY_COUNTER_MOVE, GEN_PROMOTIONS,
	PLAY_PROMOTIONS, PLAY_BAD_CAPTURES, GEN_QUIETS, PLAY_QUIETS, STAGE_DONE
};

enum {
	hashfEXACT, hashfALPHA, hashfBETA
};


typedef struct {
    
    U64 key; 
    int score;
} PAWNS_HASH;

typedef struct {
    
    U64 key; 
    int score;
} EVAL_HASH;


typedef struct {
	
	U32 move;
	int score;
} Move;

typedef struct {
	
    U8 castleFlags;
	U8 epFlag;
	U8 epSquare;

	U32 move;
	U32 ttMove;
    U32 killerMoves[2];

    int sEval;

    float extension; 
} MOVE_STACK;

typedef struct {

	U8 castleFlags;
	U8 epFlag;
	U8 epSquare;
	int fiftyMovesCounter;	
	U64 hashKey;
	U64 pawnsHashKey;
} UNDO_MOVE_STACK;

typedef struct {

	int fiftyMovesCounter;
	U64 hashKey;
} MOVES_HISTORY;

typedef struct {

	U8 flags; 
	int depth;
	int value;
	int sEval;
    U32 bestMove;    
    U64 key;
} HASHE;

typedef struct {

	bool skipQuiets;
	int stage;
	U32 ttMove, counterMove;

	std::vector<Move> moves;
	std::vector<Move> badCaptures;
} MOVE_LIST;

class PV {

public:
	int score;
	std::vector<U32> line;
};

class EvalInfo {
	
public:
	
	U64 openFilesBB;
	U64 halfOpenFilesBB[2]; 

	U64 knightAttacks[2][64];
	U64 bishopAttacks[2][64];
	U64 rookAttacks[2][64];
	U64 queenAttacks[2][64];

	U64 allPawnAttacks[2];
	U64 allKnightAttacks[2];
	U64 allBishopAttacks[2];
	U64 allRookAttacks[2];
	U64 allQueenAttacks[2];
	U64 kingAttacks[2];
	U64 attacks[2];

	U64 kingZoneBB[2];
	int kingSq[2];
	int kingAttackersCount[2];
	int kingAttackersWeight[2];
	int kingAdjacentZoneAttacksCount[2];

    void clear() {

		this->openFilesBB = 0ULL;

		for (int i = 0; i < 2; i++) {
	
			this->halfOpenFilesBB[i] = 0ULL;

			this->allPawnAttacks[i] = 0ULL;
			this->allKnightAttacks[i] = 0ULL;
			this->allBishopAttacks[i] = 0ULL;
			this->allRookAttacks[i] = 0ULL;
			this->allQueenAttacks[i] = 0ULL;
			this->kingAttacks[i] = 0ULL;

			for (int j = 0; j < 64; j++) {

				this->knightAttacks[i][j] = 0ULL;
				this->bishopAttacks[i][j] = 0ULL;
				this->rookAttacks[i][j] = 0ULL;
				this->queenAttacks[i][j] = 0ULL;
			}

			this->attacks[i] = 0ULL;

			this->kingZoneBB[i] = 0ULL;

			this->kingSq[i] = 0;

			this->kingAttackersCount[i] = 0;
			this->kingAttackersWeight[i] = 0;

			this->kingAdjacentZoneAttacksCount[i] = 0;
		} 
	}
};

#endif