#ifndef ENUMS_H
#define ENUMS_H

enum Stats {

    NODES, TTHITS
};

enum TimeFormat {

    MILLISECONDS,
    SECONDS
};


enum { NORMAL, SAFETY };
enum { MG, EG };

enum Side {

	WHITE = 0, BLACK = 1
};

enum {

	NUL = true, NO_NULL = false,
	SING = true, NON_SING = false
};

enum Piece {
	DUMMY = 0, PAWNS = 1, KNIGHTS = 2, BISHOPS = 3,
	ROOKS = 4, QUEEN = 5, KING = 6, PIECES = 7
};

enum {
	VALUE_DUMMY = 0,
	VALUE_PAWN = 100, VALUE_KNIGHT = 300, VALUE_BISHOP = 300,
	VALUE_ROOK = 500, VALUE_QUEEN = 900, VALUE_KING = 2000
};

enum {
	MOVE_NORMAL = 0, MOVE_CAPTURE = 1, MOVE_DOUBLE_PUSH = 2,
	MOVE_ENPASSANT = 3, MOVE_CASTLE = 4, MOVE_PROMOTION = 5
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

	PLAY_HASH_MOVE,
	GEN_CAPTURES,
	PLAY_GOOD_CAPTURES,
	PLAY_KILLER_MOVE_1,
	PLAY_KILLER_MOVE_2,
	PLAY_COUNTER_MOVE,
	PLAY_BAD_CAPTURES,
	GEN_QUIETS,
	PLAY_QUIETS,
	STAGE_DONE
};

enum {

	hashfEXACT = 5,
	hashfALPHA = 10,
	hashfBETA = 15
};


#endif
