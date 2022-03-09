#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <cstdint>

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

#define INPUT_BUFFER 800 * 6

#define NUMBER_OF_TBLS  12

#define NO_MOVE 0UL

#define FLIP_TB(sq) ((sq)^0x38) // Flip top-to-bottom (A8==A1, A7==A2 etc.)

#define POPCOUNT(pieces) __builtin_popcountll(pieces)
#define GET_POSITION(pieces) __builtin_ctzll(pieces)
#define POP_POSITION(pieces) pieces &= pieces - 1

/* Extract data from a move structure */
#define promType(move)		(move & 0x3000000)	>> 24
#define castleDir(move)		(move & 0xC00000) 	>> 22
#define move_type(move)     (move & 0x380000)	>> 19
#define colorType(move)     (move & 0x40000) 	>> 18
#define cPieceType(move)    (move & 0x38000) 	>> 15
#define pieceType(move)     (move & 0x7000) 	>> 12
#define from_sq(move)       (move & 0xFC0) 		>> 6
#define to_sq(move)          move & 0x3F

#define ScoreMG(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define ScoreEG(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

inline int MakeScore(int mg, int eg) {

	return (int)((unsigned int)eg << 16) + mg;
}

constexpr int S(const int mg, const int eg) {

	return (int)((unsigned int)eg << 16) + mg;
}

#endif