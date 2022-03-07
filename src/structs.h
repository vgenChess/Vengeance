#ifndef STRUCTS_H 
#define STRUCTS_H

#include <vector>

#include "types.h"

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
	int material;
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

struct CoefficientsInfo {
	
	uint16_t index;

    int8_t wcoeff;
    int8_t bcoeff;

    int8_t type;
};

struct Data {
	
	float result;

	std::vector<CoefficientsInfo> coefficientsInfoList;
	float pfactors[2];

	int phase;
	int eval;
	int sEval;
	int safety[2];
};

#endif