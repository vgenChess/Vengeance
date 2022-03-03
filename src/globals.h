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

#include "types.h"

#define INPUT_BUFFER 800 * 6

#define C64(constantU64) constantU64

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

#define FLIP_TB(sq) ((sq)^0x38) // Flip top-to-bottom (A8==A1, A7==A2 etc.)
#define NUMBER_OF_TBLS  12

#define MAX_PLY 128
#define MAX_MOVES 256

/* Extract data from a move structure */


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

extern int MAX_DEPTH;

// for perft
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

extern int pawnPSQT[64];
extern int knightPSQT[64];
extern int bishopPSQT[64];
extern int rookPSQT[64];
extern int queenPSQT[64];
extern int kingPSQT[64];

#endif /* globals_h */
