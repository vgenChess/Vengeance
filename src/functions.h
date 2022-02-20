#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#define POPCOUNT(pieces) __builtin_popcountll(pieces)
#define GET_POSITION(pieces) __builtin_ctzll(pieces)
#define POP_POSITION(pieces) pieces &= pieces - 1

/* Extract data from a move structure */
#define promType(move)		(move & 0x3000000) >> 24
#define castleDir(move)		(move & 0xC00000) >> 22
#define move_type(move)     (move & 0x380000) >> 19
#define colorType(move)     (move & 0x40000) >> 18
#define cPieceType(move)    (move & 0x38000) >> 15
#define pieceType(move)     (move & 0x7000) >> 12
#define from_sq(move)       (move & 0xFC0) >> 6
#define to_sq(move)          move & 0x3F

#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define ScoreMG(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define ScoreEG(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

#define S(mg, eg) (MakeScore((mg), (eg)))


#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))


#endif