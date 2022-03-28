
#ifndef TUNED_EVALS_H
#define TUNED_EVALS_H

#include "functions.h"


constexpr int

weight_val_pawn = S(87, 90),
weight_val_knight = S(381, 203),
weight_val_bishop = S(429, 261),
weight_val_rook = S(599, 450),
weight_val_queen = S(1323, 820), 

weight_isolated_pawn = S(-13, -2),
weight_backward_pawn = S(-9, -5),
weight_double_pawn = S(-3, -19),
weight_pawn_hole = S(-10, 1), 

arr_weight_pawn_chain[8] = { 

S(   0,   0), S(   0,   0), S(   5,  14), S(  24,   8), 
S(  29,  33), S(  22,  40), S(   0,   0), S(   0,   0), 

}, 
arr_weight_phalanx_pawn[8] = { 

S(   0,   0), S(   3,  -5), S(  -5,  -3), S(   8,   2), 
S(  15,  17), S( 131,  46), S( 132, 199), S(   0,   0), 

}, 
arr_weight_defended_phalanx_pawn[8] = { 

S(   0,   0), S(   0,   0), S(   5,   7), S(  14,  10), 
S(  38,  41), S(  45,  56), S(   9,  11), S(   0,   0), 

}, 
arr_weight_passed_pawn[8] = { 

S(   0,   0), S(  -8,   5), S( -14,   8), S( -17,  25), 
S(  -8,  49), S( -11, 122), S(  11, 147), S(   0,   0), 

}, 
arr_weight_defended_passed_pawn[8] = { 

S(   0,   0), S(   0,   0), S(   1,  17), S(  -2,  33), 
S(  31,  64), S(  83, 148), S( 257, 164), S(   0,   0), 

}, 

weight_minor_has_pawn_shield = S(10, 6), 

weight_knight_all_pawns_count = S(3, 6), 
weight_knight_outpost = S(11, 3), 
weight_undefended_knight = S(-14, 1), 
weight_knight_defended_by_pawn = S(-5, 2), 

weight_bishop_pair = S(34, 53),
weight_undefended_bishop = S(-5, -1),

weight_rook_behind_stm_passed_pawn = S(17, -9), 
weight_rook_behind_opp_passed_pawn = S(-18, 19), 
weight_rook_flank_outpost = S(-6, 3), 
weight_rook_half_open_file = S(18, 7), 
weight_rook_open_file = S(39, -7), 
weight_rook_enemy_queen_same_file = S(12, -13), 
weight_rook_on_seventh_rank = S(1, 9), 
weight_rook_on_eight_rank = S(-17, 2), 
weight_rook_supporting_friendly_rook = S(50, 13), 

weight_queen_underdeveloped_pieces = S(-7, 3), 

arr_weight_knight_mobility[16] = { 

S( -38, -63), S(  -7, -45), S(   2, -27), S(   8, -24), 
S(  15, -20), S(  20, -15), S(  26, -13), S(  26, -11), 
S(  36, -22), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
arr_weight_bishop_mobility[16] = { 

S( -10, -38), S(  -6, -31), S(   4, -28), S(   7, -19), 
S(  14, -15), S(  19,  -7), S(  20,  -3), S(  24,  -2), 
S(  25,   0), S(  33,  -6), S(  41,  -9), S(  45, -12), 
S(  41,   0), S(  40, -13), S(   0,   0), S(   0,   0), 

}, 
arr_weight_rook_mobility[16] = { 

S( -13, -42), S(  -3, -21), S(  -1, -18), S(   2, -13), 
S(   0,  -7), S(   6,  -8), S(  10,  -5), S(   7,   0), 
S(  15,  -1), S(  21,  -2), S(  19,   3), S(  18,   8), 
S(  29,  13), S(  39,   4), S(  30,  10), S(   0,   0), 

}, 
arr_weight_queen_mobility[32] = { 

S( -13,   6), S(  -4,-104), S(  -3, -80), S(  -5, -74), 
S(  -2, -72), S(   1, -44), S(   2, -48), S(   0, -24), 
S(   1, -16), S(   7,  -9), S(   7,   1), S(   7,  -5), 
S(   6,   2), S(   8,   9), S(  12,   4), S(  10,  14), 
S(  -1,  24), S(  14,  14), S(  17,  13), S(   8,  17), 
S(   9,  21), S(  16,  17), S(  37, -16), S(  69, -32), 
S(  46, -27), S(  58, -31), S(  -9,  -9), S( -12, -20), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 


weight_pawn_shield[8][8] = { 

S( -33,  -8), S(   7, -37), S(  12, -23), S(  -7, -11), S( -14,   6), S(  -1,  17), S(  14,  17), S(   0,   0), 
S( -38,  -3), S(  18, -22), S(   6, -13), S( -24,  -6), S( -28,  13), S( -58,  19), S( -54,  35), S(   0,   0), 
S( -30,  -3), S(  25, -16), S(  -9,  -3), S( -13,  -1), S( -19,   2), S( -26,  15), S(  19,  16), S(   0,   0), 
S( -11,   6), S(   9,   2), S(  10,   9), S(   3,  14), S(  -9,  20), S( -38,  33), S( -23,  47), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
weight_blocked_pawn_storm[8][8] = { 

S(   0,   0), S(   0,   0), S(  -6, -50), S(  29, -20), S(  16, -29), S(  41, -51), S(  20, -54), S(   0,   0), 
S(   0,   0), S(   0,   0), S( -88, -28), S(  20, -27), S(  -4, -19), S(   4, -39), S(  56, -49), S(   0,   0), 
S(   0,   0), S(   0,   0), S( -62,  -3), S( -20,  -9), S(   1, -26), S(  -4, -29), S(  -4, -36), S(   0,   0), 
S(   0,   0), S(   0,   0), S(  -7, -67), S(  -7, -23), S(   5, -27), S(   6, -26), S(  53, -46), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
weight_unblocked_pawn_storm[8][8] = { 

S(  -1, -22), S( -18, 212), S( -42, 137), S( -42,  52), S(  -6,   4), S(   3, -14), S(   7, -10), S(   0,   0), 
S(   0, -18), S(   7, 195), S( -75, 131), S( -11,  38), S(  -6,   6), S(   6,  -8), S(   0,  -9), S(   0,   0), 
S(   3, -20), S( -35, 161), S(-126, 115), S(  -8,  28), S(   2,  -1), S(   4, -13), S(  14, -17), S(   0,   0), 
S( -11,  -6), S( -63, 159), S( -54,  91), S( -22,  17), S(  -8,  -3), S(  -1,  -5), S(  -9,  -8), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 

weight_knight_attack = S(15, 50), 
weight_bishop_attack = S(9, 49), 
weight_rook_attack = S(9, 50), 
weight_queen_attack = S(9, 63), 
weight_safe_knight_check = S(132, -1), 
weight_safe_bishop_check = S(39, 385), 
weight_safe_rook_check = S(80, 13), 
weight_safe_queen_check = S(41, 72), 
weight_unsafe_knight_check = S(13, 0), 
weight_unsafe_bishop_check = S(27, 146), 
weight_unsafe_rook_check = S(38, -12), 
weight_unsafe_queen_check = S(22, 17), 
weight_safety_adjustment = S(76, -100), 


pawnPSQT[U8_MAX_SQUARES] = {

S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S( -10,   4), S( -16,   4), S( -19,   2), S( -14,  -5), S( -16,   0), S( -12,   4), S(  11,  -2), S(  -3,  -2), 
S(  -6,  -4), S( -12,  -6), S(   6, -13), S(   4, -16), S(  12, -12), S(  -2,  -6), S(  15,  -9), S( -11,  -9), 
S( -16,   4), S( -13,   0), S(   3, -14), S(  16, -25), S(  19, -27), S(   5,  -9), S(  -3,  -1), S( -37,  -4), 
S(  -1,  16), S(   2,   7), S(   0,  -2), S(  18, -27), S(  26, -25), S(   8,   2), S(   9,   4), S( -25,   6), 
S(  -3,  28), S(   0,  21), S(  29,   5), S(  11, -27), S(  43, -35), S(  30,  -1), S(  -1,  12), S( -16,  16), 
S(  37,  54), S(  29,  50), S(   8,  44), S(  36,   8), S(  51,  -8), S(  31,  31), S(  45,  50), S( -54,  53), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

},

knightPSQT[U8_MAX_SQUARES] = {

S( -63,  -3), S(  -7,   4), S( -37,  12), S(  -7,  14), S(  -3,   2), S( -14,  13), S(  -9,  -7), S( -72,   8), 
S( -10,  -2), S( -25,   7), S(   3,   2), S(  16,   4), S(  19,   0), S(   4,   9), S(   4,   4), S(  -3,  -5), 
S( -11,   5), S(   8,   9), S(  12,   0), S(  10,  26), S(  21,  23), S(  14,  -1), S(  28,  -5), S( -12,  -7), 
S(  10,   8), S(   6,  20), S(  19,  28), S(  17,  32), S(  20,  32), S(  26,  16), S(   2,  14), S(   1,  11), 
S(  27,  12), S(  26,  21), S(  13,  32), S(  44,  36), S(  28,  29), S(  27,  33), S(  26,  23), S(  35,   5), 
S( -18,   0), S(  18,   4), S(  24,  27), S(  70,  19), S( 101,  -2), S( 136,  -1), S(  73,  -7), S(  11,   4), 
S( -69,   3), S( -27,  13), S(  89, -14), S(  24,  11), S( 100, -12), S(  91, -16), S(  22,   0), S(   0, -11), 
S(-229,  29), S( -34, -16), S( -57,  12), S( -22,  10), S(  32,  -5), S(-124,  14), S( -72,   5), S( -99, -38), 

},

bishopPSQT[U8_MAX_SQUARES] = {

S(  19, -25), S(   4, -11), S(  17,  -8), S(  17,  -4), S(   8,  -5), S(  -6,  -5), S(   7, -24), S(  20, -22), 
S(  -7,  -7), S(  42, -17), S(  21, -11), S(  16,   0), S(  22,  -3), S(  31, -11), S(  51, -20), S(  13, -27), 
S(  37, -20), S(  29,   0), S(  28,   2), S(  10,  10), S(  22,   7), S(  33,  -2), S(  21, -13), S(  28, -19), 
S(  -1,  -9), S(   8,  -4), S(  12,   6), S(  34,   3), S(  32,   3), S(  16,   7), S(  17, -12), S(  -5, -16), 
S( -16,   4), S(   6,   8), S(  14,   5), S(  22,  14), S(  34,   7), S(  -6,   5), S(  26,  -7), S(  -2,  -8), 
S( -10,   0), S(   7,   2), S(  55,  -7), S(  16,   8), S(  67,  -9), S(  65,   5), S(  49,  -7), S(  36, -11), 
S( -34, -10), S(   3,  -4), S(  -8,  -4), S(   2, -14), S(  14,  -8), S(  76, -26), S(  17,  -6), S(  24, -34), 
S(  -9, -15), S( -48, -13), S( -15, -15), S( -56,  -5), S( -68,  -3), S( -24,  -5), S(  18, -22), S( -19,  -8), 

},

rookPSQT[U8_MAX_SQUARES] = {

S(   2,  11), S(   2,   8), S(   8,   7), S(  11,   8), S(  21,  -3), S(  21,   0), S( -15,   3), S(  15, -18), 
S( -18,   5), S( -16,   9), S( -15,  15), S(  -6,   9), S(   2,   1), S(   3,   0), S(  13,  -4), S( -40,   6), 
S( -16,   4), S( -16,   8), S( -14,   8), S(   7,   3), S(   4,   4), S(  14,  -7), S(  19, -10), S(   5,  -8), 
S( -21,  14), S( -30,  17), S( -33,  22), S( -20,  21), S( -15,  17), S( -10,   9), S(  11,   0), S( -11,   0), 
S( -27,  22), S( -13,  15), S(  -2,  18), S(  22,   9), S(  -7,  12), S(  11,   9), S(  23,   5), S(  22,   4), 
S(   5,  18), S(  -1,  18), S(  -8,  17), S(  16,   9), S(  41,   1), S(  44,   1), S(  77,  -3), S(  47,  -2), 
S(   8,  21), S( -11,  37), S(  32,  22), S(  74,   8), S(  32,  12), S(  96,  -2), S( 117,  -8), S(  91,  -6), 
S( -28,   9), S(   0,  -2), S( -12,   4), S(  -2,  -5), S(  17, -10), S(  50, -20), S(  48, -19), S( -29,  -1), 

},

queenPSQT[U8_MAX_SQUARES] = {

S(  16, -24), S(  14, -31), S(  18, -35), S(  22, -34), S(  20, -24), S(   3, -30), S( -35,  -9), S(   5, -32), 
S(   9, -19), S(  11, -10), S(  13, -15), S(  20, -12), S(  23, -17), S(  35, -53), S(  33, -52), S(  17, -19), 
S(   5, -36), S(  17, -44), S(   2,  18), S(  10, -15), S(   7,   4), S(  20,  -7), S(  17,   8), S(   5,  -6), 
S(   5, -22), S( -14,  26), S(  -8,   6), S( -15,  38), S(   2,  17), S(  11,   0), S(   7,  13), S(   1,  -5), 
S( -24,   1), S(  -7,   2), S( -22,  14), S( -26,  40), S( -10,  42), S( -18,  47), S( -13,  36), S(   9,  11), 
S( -24, -14), S( -19,  -3), S(  11,   3), S(   1,  12), S(  24,  46), S(  26,  16), S(  88, -20), S(  20,   5), 
S( -32,  15), S( -44,  11), S( -41,  34), S( -66,  75), S( -67,  71), S(  80,  30), S(   1,  60), S( 122, -59), 
S( -24,   5), S(  -8,   6), S(  14,   4), S(  31,  21), S(  43,  13), S(  63,   5), S(  18, -12), S(  13,  28), 

},

kingPSQT[U8_MAX_SQUARES] = {

S( -48, -17), S(  16,  -8), S(  -1, -19), S( -50, -35), S( -12, -48), S( -56, -10), S(   2,  -4), S(  -6, -31), 
S(  30, -14), S(   4,  11), S( -13,   3), S( -54,  -4), S( -60,  -3), S( -42,  14), S(   1,  23), S(  -4,   2), 
S(  33, -10), S(  50,  11), S( -24,  14), S( -58,   6), S( -57,   5), S( -27,  18), S(   8,  27), S( -37,  17), 
S(  63, -12), S(  63,  15), S(  32,  10), S( -50,   7), S( -35,   5), S( -41,  25), S(  -2,  33), S( -65,  26), 
S(  42,   0), S(  48,  25), S(  58,   9), S(  -5,   0), S(  -4,   0), S(  25,  23), S(  46,  39), S( -13,  30), 
S(  62,  10), S( 162,  23), S(  47,   9), S(  50, -15), S(  11,  -5), S( 109,  14), S( 130,  43), S(  24,  36), 
S(  39,   6), S( 122,  23), S(  86,  -5), S(  33, -24), S(  35, -20), S( 102,   7), S(  49,  45), S( -25,  39), 
S(  -3,  -9), S(  71,   6), S(  89, -20), S(  70, -42), S(  77, -38), S(  83,  -6), S(  63,  21), S(  14,   1), 

};


#endif
