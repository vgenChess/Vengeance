//
//  globals.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef globals_h
#define globals_h

#include <stdbool.h>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <ratio>
#include <chrono>

#define INPUT_BUFFER 800 * 6

typedef uint8_t 		u8;
typedef uint16_t 	u16;
typedef uint32_t 	u32;
typedef uint64_t 	u64;

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

#define C64(constantU64) constantU64

#define MAX_MOVES 256

#define WHITE 0
#define BLACK 1

#define PROMOTE_TO_QUEEN 0
#define PROMOTE_TO_ROOK 1
#define PROMOTE_TO_BISHOP 2
#define PROMOTE_TO_KNIGHT 3

#define MAX_SIDES 2
#define MAX_PIECES 8
#define MAX_SQUARES 64

#define INDEX_BB_SIZE 64

#define PAWN_HASH_TABLE_SIZE 2048
#define EVAL_HASH_TABLE_SIZE 16384

#define hashfEXACT 0
#define hashfALPHA 1
#define hashfBETA 2

#define FLIP_TB(sq) ((sq)^0x38) // Flip top-to-bottom (A8==A1, A7==A2 etc.)
#define NUMBER_OF_TBLS  12

#define MAX_PLY 256

#define CASTLE_FLAG_WHITE_KING		2
#define CASTLE_FLAG_WHITE_QUEEN		1
#define CASTLE_FLAG_BLACK_KING		8
#define CASTLE_FLAG_BLACK_QUEEN		4


#define WHITE_CASTLE_QUEEN_SIDE    	0
#define WHITE_CASTLE_KING_SIDE  	1
#define BLACK_CASTLE_QUEEN_SIDE 	2
#define BLACK_CASTLE_KING_SIDE 		3


/* Extract data from a move structure */

#define MOVE_NORMAL			0
#define MOVE_CAPTURE		1
#define MOVE_DOUBLE_PUSH 	2
#define MOVE_ENPASSANT 		3
#define MOVE_CASTLE 		4
#define MOVE_PROMOTION 		5


#define promType(move)		(move & 0x3000000) >> 24
#define castleDir(move)		(move & 0xC00000) >> 22
#define move_type(move)     (move & 0x380000) >> 19
#define colorType(move)     (move & 0x40000) >> 18
#define cPieceType(move)    (move & 0x38000) >> 15
#define pieceType(move)     (move & 0x7000) >> 12
#define from_sq(move)       (move & 0xFC0) >> 6
#define to_sq(move)          move & 0x3F


#define RANK_1 0x00000000000000FFU
#define RANK_2 0x000000000000FF00U
#define RANK_3 0x0000000000FF0000U
#define RANK_4 0x00000000FF000000U
#define RANK_5 0x000000FF00000000U
#define RANK_6 0x0000FF0000000000U
#define RANK_7 0x00FF000000000000U
#define RANK_8 0xFF00000000000000U

#define NOT_RANK_2 0xFFFFFFFFFFFF00FFU
#define NOT_RANK_7 0xFF00FFFFFFFFFFFFU

#define NOT_RANK_1 0xFFFFFFFFFFFFFF00U
#define NOT_RANK_8 0x00FFFFFFFFFFFFFFU

#define WQ_SIDE_SQS 0x000000000000000EU
#define WK_SIDE_SQS 0x0000000000000060U
#define BQ_SIDE_SQS 0x0E00000000000000U
#define BK_SIDE_SQS 0x6000000000000000U

#define NOT_A_FILE 0XFEFEFEFEFEFEFEFEU
#define NOT_H_FILE 0X7F7F7F7F7F7F7F7FU

#define A_FILE 0x101010101010101U
#define B_FILE 0x202020202020202U
#define C_FILE 0x404040404040404U
#define D_FILE 0x808080808080808U
#define E_FILE 0x1010101010101010U
#define F_FILE 0x2020202020202020U
#define G_FILE 0x4040404040404040U
#define H_FILE 0x8080808080808080U 

#define AREA_WHITE 0x00000000FFFFFFFFU

#define CENTER 0x0000001818000000U
#define EXTENDED_CENTER 0x00003C3C3C3C0000U

#define NO_MOVE 0UL

#define PV_NODE 0 
#define NON_PV_NODE 1


/****************************************************************************/
/** MACROS                                                                 **/
/****************************************************************************/

// The two following can also be added as compilation flags
// e.g. add : -D NN_DEBUG and/or -D NN_WITH_FMA to gcc / clang

// allows assertions helping to debug when embedding the library
//#define NN_DEBUG

// allows the use of intrinsics, to increase speed of dot products
#define NN_WITH_FMA

// name of the default network file
#define NN_FILE "network.nn"

// output size of the first layer, in neurons
// set it to 256 to have an architecture similar to Stockfish's one
#define NN_SIZE 128

// constants used to convert first layer weights to int16
#define THRESHOLD 3.2f
#define FACTOR 10000.0f

// boundaries for layers outputs
#define NN_RELU_MIN 0.0f
#define NN_RELU_MAX 1.0f



/****************************************************************************/
/** TYPE DEFINITIONS                                                       **/
/****************************************************************************/

typedef struct {
	alignas(64) float W0[40960*NN_SIZE];
	alignas(64) float B0[NN_SIZE];
	alignas(64) float W1[NN_SIZE*2*32];
	alignas(64) float B1[32];
	alignas(64) float W2[32*32];
	alignas(64) float B2[32];
	alignas(64) float W3[32*1];
	alignas(64) float B3[1];
} NN_Network;

typedef struct {
	int16_t W0[40960*NN_SIZE];
	float B0[NN_SIZE];
	float W1[NN_SIZE*2*32];
	float B1[32];
	float W2[32*32];
	float B2[32];
	float W3[32*1];
	float B3[1];
} NN_Storage;

/*
 * pieces must be stored by color and by type, under the bitboard format, with
 * the following convention:
 * - white = 0, black = 1
 * - pawn = index 0, knight = 1, bishop = 2, rook = 3, queen = 4, king = 5
 */

typedef struct {
	uint64_t* pieces[2];
	float accumulator[2][NN_SIZE];
} NN_Board;

struct NnueAccumulator {

	alignas(64) float v[2][NN_SIZE]; 
};

/*
struct  alignas(64)  NnueAccumulator  {

 //  Two  vectors  of  size  N.  v[0]  for  white's,  and  v[1]  for  black's  perspect
 int16_t  v[2][NN_SIZE]; 
 int16_t*  operator[](Color  perspective)  { return  v[perspective]; }
};*/

enum {

	DUMMY = 0,
	PAWNS = 1, 
	KNIGHTS = 2,
	BISHOPS = 3,
	ROOKS = 4,
	QUEEN = 5,
	KING = 6,
	PIECES = 7
};

enum {

	VALUE_PAWN = 100,
	VALUE_KNIGHT = 300,
	VALUE_BISHOP = 300,
	VALUE_ROOK = 500,
	VALUE_QUEEN = 900, 
	VALUE_KING = 2000
};


enum Stage {

	PLAY_HASH_MOVE,
	GEN_PROMOTION_CAPTURE_MOVES,
	PLAY_PROMOTION_CAPTURE_MOVES,
	GEN_PROMOTION_QUIET_MOVES,
	PLAY_PROMOTION_QUIET_MOVES,
	GEN_CAPTURE_MOVES,
	PLAY_CAPTURE_MOVES,
	PLAY_KILLER_MOVE_1,
	PLAY_KILLER_MOVE_2,
	PLAY_BAD_CAPTURES,
	GEN_QUIET_MOVES,
	PLAY_QUIET_MOVES,
	STAGE_DONE
};

typedef struct {
    
    u64 key; 
    int score;
} PAWNS_HASH;

typedef struct {
    
    u64 key; 
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

    u8 extend;
} MOVE_STACK;

typedef struct {

	u8 castleFlags;
	u8 epFlag;
	u8 epSquare;
	u64 hashKey;
	u64 pawnsHashKey;
	int fiftyMovesCounter;
	
	NnueAccumulator accumulator;
} UNDO_MOVE_STACK;

typedef struct {

	int fiftyMovesCounter;
	u64 hashKey;
} MOVES_HISTORY;

typedef struct {
    
    u64 key;
    u32 bestMove;
	int depth;
	int value;
	int eval_static;
	u8 flags; 
} HASHE;


typedef struct {

	int stage;

	std::vector<Move> moves;
	std::vector<Move> badCaptures;
} MOVE_LIST;


class PV {

	public:
		int score;
		std::vector<u32> line;
};

extern int MAX_DEPTH;


extern int TUNE;

extern u64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;

extern u64 KEY_SIDE_TO_MOVE;

extern u64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
extern u64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
extern u64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
extern u64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;

extern u64 KEY_EP_A_FILE;
extern u64 KEY_EP_B_FILE;
extern u64 KEY_EP_C_FILE;
extern u64 KEY_EP_D_FILE;
extern u64 KEY_EP_E_FILE;
extern u64 KEY_EP_F_FILE;
extern u64 KEY_EP_G_FILE;
extern u64 KEY_EP_H_FILE;

extern int VAL_UNKNOWN;

// zobrist keys

extern u64 zobrist[8][2][64];
extern u64 pawnZobristKey[64];

extern HASHE *hashTable;
extern u32 HASH_TABLE_SIZE;

extern u64 index_bb[INDEX_BB_SIZE];				
extern u8 rookCastleFlagMask[64];

// uci
extern bool quit;
extern int option_thread_count;

// for time management
extern bool timeSet;
extern bool stopped;

extern int totalTimeLeft;
extern int timePerMove;

extern std::chrono::steady_clock::time_point startTime;
extern std::chrono::steady_clock::time_point stopTime;

extern u64 arrInBetween[64][64];




// Eval weights

extern int weight_pawn;
extern int weight_knight;
extern int weight_bishop;
extern int weight_rook;
extern int weight_queen; 

extern int weight_isolated_pawn;
extern int weight_backward_pawn;
extern int weight_double_pawn;
extern int weight_defended_pawn;
extern int weight_pawn_hole;

extern int arr_weight_passed_pawn[8];
extern int arr_weight_defended_passed_pawn[8];

extern int weight_undefended_knight;
extern int weight_knight_defended_by_pawn;

extern int weight_bishop_pair;
extern int weight_undefended_bishop;
extern int weight_bad_bishop;


extern int weight_rook_half_open_file;
extern int weight_rook_open_file;
extern int weight_rook_enemy_queen_same_file;
extern int weight_rook_supporting_friendly_rook;
extern int weight_rook_blocked_by_king;
extern int weight_rook_on_seventh_rank;
extern int weight_rook_on_eight_rank;


extern int weight_queen_underdeveloped_pieces;


extern int weight_king_many_pawn_shield;
extern int weight_king_pawn_shield;
extern int weight_king_many_enemy_pawn_storm;
extern int weight_king_enemy_pawn_storm;


extern int arr_weight_knight_mobility[16];
extern int arr_weight_bishop_mobility[16];
extern int arr_weight_rook_mobility[16];
extern int arr_weight_queen_mobility[32];


extern int pawnPSQT[64];
extern int knightPSQT[64];
extern int bishopPSQT[64];
extern int rookPSQT[64];
extern int queenPSQT[64];
extern int kingPSQT[64];



extern int weight_center_control;

extern int weight_knight_attack;
extern int weight_bishop_attack;
extern int weight_rook_attack;
extern int weight_queen_attack;

extern int weight_rook_safe_contact_check;
extern int weight_queen_safe_contact_check;

extern int weight_knight_check;
extern int weight_bishop_check;
extern int weight_rook_check;
extern int weight_queen_check;

extern int weight_safety_adjustment;


//NNUE

extern NN_Network nnue;

#endif /* globals_h */
