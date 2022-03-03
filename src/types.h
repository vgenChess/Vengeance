#ifndef TYPES_H
#define TYPES_H

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
(byte & 0x80 ? '1' : '0'), \
(byte & 0x40 ? '1' : '0'), \
(byte & 0x20 ? '1' : '0'), \
(byte & 0x10 ? '1' : '0'), \
(byte & 0x08 ? '1' : '0'), \
(byte & 0x04 ? '1' : '0'), \
(byte & 0x02 ? '1' : '0'), \
(byte & 0x01 ? '1' : '0')

typedef uint8_t 	u8;
typedef uint16_t 	u16;
typedef uint32_t 	u32;
typedef uint64_t 	u64;
typedef uint64_t Bitboard;

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

enum Stage {
	PLAY_HASH_MOVE, GEN_CAPTURES, PLAY_CAPTURES, PLAY_KILLER_MOVE_1,
	PLAY_KILLER_MOVE_2, PLAY_COUNTER_MOVE, GEN_PROMOTIONS,
	PLAY_PROMOTIONS, PLAY_BAD_CAPTURES, GEN_QUIETS, PLAY_QUIETS, STAGE_DONE
};

enum {
	hashfEXACT, hashfALPHA, hashfBETA
};


typedef struct {
    
    uint64_t key; 
    int score;
} PAWNS_HASH;

typedef struct {
    
    uint64_t key; 
    int score;
} EVAL_HASH;


typedef struct {
	
	u32 move;
	int32_t score;
} Move;

typedef struct {
	
    u8 castleFlags;
	u8 epFlag;
	u8 epSquare;

	u32 move;
	u32 ttMove;
    u32 killerMoves[2];

    int sEval;

    float extension; 
} MOVE_STACK;

typedef struct {

	u8 castleFlags;
	u8 epFlag;
	u8 epSquare;
	int fiftyMovesCounter;	
	uint64_t hashKey;
	uint64_t pawnsHashKey;
} UNDO_MOVE_STACK;

typedef struct {

	int fiftyMovesCounter;
	u64 hashKey;
} MOVES_HISTORY;

typedef struct {

	u8 flags; 
	int depth;
	int value;
	int sEval;
    u32 bestMove;    
    uint64_t key;
} HASHE;

typedef struct {

	bool skipQuiets;
	int stage;
	u32 ttMove, counterMove;

	std::vector<Move> moves;
	std::vector<Move> badCaptures;
} MOVE_LIST;

class PV {

public:
	int score;
	std::vector<u32> line;
};

class EvalInfo {
	
public:
	
	Bitboard openFilesBB;
	Bitboard halfOpenFilesBB[2]; 

	Bitboard knightAttacks[2][64];
	Bitboard bishopAttacks[2][64];
	Bitboard rookAttacks[2][64];
	Bitboard queenAttacks[2][64];

	Bitboard allPawnAttacks[2];
	Bitboard allKnightAttacks[2];
	Bitboard allBishopAttacks[2];
	Bitboard allRookAttacks[2];
	Bitboard allQueenAttacks[2];
	Bitboard kingAttacks[2];
	Bitboard attacks[2];

	Bitboard kingZoneBB[2];
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