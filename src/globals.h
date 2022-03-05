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

#include "constants.h"
#include "types.h"

extern int MAX_DEPTH;

// for perft
extern U64 quiet, prevCap, cap, prevEp, ep, prevCas, cas, check, prom;

extern U64 KEY_SIDE_TO_MOVE;

extern U64 KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
extern U64 KEY_FLAG_WHITE_CASTLE_KING_SIDE;
extern U64 KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
extern U64 KEY_FLAG_BLACK_CASTLE_KING_SIDE;

extern U64 KEY_EP_A_FILE;
extern U64 KEY_EP_B_FILE;
extern U64 KEY_EP_C_FILE;
extern U64 KEY_EP_D_FILE;
extern U64 KEY_EP_E_FILE;
extern U64 KEY_EP_F_FILE;
extern U64 KEY_EP_G_FILE;
extern U64 KEY_EP_H_FILE;

// zobrist keys

extern U64 zobrist[U8_MAX_PIECES][U8_MAX_SIDES][U8_MAX_SQUARES];
extern U64 pawnZobristKey[U8_MAX_SQUARES];

extern HASHE *hashTable;
extern U32 HASH_TABLE_SIZE;

extern U64 index_bb[U8_MAX_SQUARES];				
extern U8 rookCastleFlagMask[U8_MAX_SQUARES];

// uci

extern bool quit;
extern int option_thread_count;

// for time management

extern bool timeSet;
extern bool stopped;

extern int timePerMove;

extern std::chrono::steady_clock::time_point startTime;
extern std::chrono::steady_clock::time_point stopTime;

extern U64 arrInBetween[U8_MAX_SQUARES][U8_MAX_SQUARES];


// Eval weights for tuning and evaluation

extern int weight_val_pawn;
extern int weight_val_knight;
extern int weight_val_bishop;
extern int weight_val_rook;
extern int weight_val_queen; 

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

extern int weight_rook_half_open_file;
extern int weight_rook_open_file;
extern int weight_rook_enemy_queen_same_file;
extern int weight_rook_supporting_friendly_rook;
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

extern int kingPSQT[U8_MAX_SQUARES];
extern int pawnPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES];
extern int knightPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES];
extern int bishopPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES];
extern int rookPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES];
extern int queenPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES];

extern int PSQT[U8_MAX_SQUARES][U8_MAX_PIECES][U8_MAX_SQUARES];

#endif /* globals_h */
