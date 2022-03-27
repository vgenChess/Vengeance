
#ifndef WEIGHTS_H
#define WEIGHTS_H

#include "functions.h"


constexpr int

weight_val_pawn = S(87, 93),
weight_val_knight = S(360, 226),
weight_val_bishop = S(420, 262),
weight_val_rook = S(578, 466),
weight_val_queen = S(1209, 878), 

weight_isolated_pawn = S(-14, -2),
weight_backward_pawn = S(-9, -6),
weight_double_pawn = S(-3, -20),
weight_pawn_hole = S(-8, 1), 

arr_weight_pawn_chain[8] = { 

S(   0,   0), S(   0,   0), S(   5,  13), S(  23,   7), 
S(  25,  29), S(  36,  25), S(   0,   0), S(   0,   0), 

}, 
arr_weight_phalanx_pawn[8] = { 

S(   0,   0), S(   3,  -5), S(  -4,  -3), S(   8,   2), 
S(  14,  17), S(  75,  51), S(  68, 133), S(   0,   0), 

}, 
arr_weight_defended_phalanx_pawn[8] = { 

S(   0,   0), S(   0,   0), S(   4,   6), S(  15,   9), 
S(  32,  43), S(  49,  31), S(  11,  14), S(   0,   0), 

}, 
arr_weight_passed_pawn[8] = { 

S(   0,   0), S(  -8,   4), S( -15,   7), S( -17,  25), 
S(  -9,  49), S(  -8, 117), S(  21, 128), S(   0,   0), 

}, 
arr_weight_defended_passed_pawn[8] = { 

S(   0,   0), S(   0,   0), S(   0,  17), S(  -2,  33), 
S(  28,  65), S(  75, 142), S( 205, 151), S(   0,   0), 

}, 
weight_minor_has_pawn_shield = S(10, 5), 

weight_knight_all_pawns_count = S(4, 6), 
weight_knight_outpost = S(11, 4), 
weight_undefended_knight = S(-13, 0), 
weight_knight_defended_by_pawn = S(-5, 0), 

weight_bishop_pair = S(35, 52),
weight_undefended_bishop = S(-4, -1),

weight_rook_behind_stm_passed_pawn = S(17, -8), 
weight_rook_behind_opp_passed_pawn = S(-17, 19), 
weight_rook_flank_outpost = S(-5, 3), 
weight_rook_half_open_file = S(18, 6), 
weight_rook_open_file = S(39, -8), 
weight_rook_enemy_queen_same_file = S(12, -13), 
weight_rook_on_seventh_rank = S(2, 8), 
weight_rook_on_eight_rank = S(9, 4), 
weight_rook_supporting_friendly_rook = S(34, 0), 

weight_queen_underdeveloped_pieces = S(-8, 8), 

arr_weight_knight_mobility[16] = { 

S( -40, -57), S(  -9, -41), S(   1, -23), S(   6, -20), 
S(  14, -17), S(  19, -13), S(  24, -12), S(  25,  -9), 
S(  38, -22), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
arr_weight_bishop_mobility[16] = { 

S( -10, -36), S(  -7, -26), S(   3, -23), S(   8, -15), 
S(  14, -10), S(  19,  -3), S(  20,   0), S(  24,   2), 
S(  25,   5), S(  33,  -2), S(  40,  -3), S(  45,  -8), 
S(  51,   4), S(  44,  -7), S(   0,   0), S(   0,   0), 

}, 
arr_weight_rook_mobility[16] = { 

S( -15, -43), S(  -3, -22), S(  -2, -17), S(   0, -13), 
S(  -1,  -8), S(   6,  -6), S(   8,  -4), S(   7,   0), 
S(  13,   0), S(  19,  -2), S(  17,   4), S(  16,  10), 
S(  26,  15), S(  35,   6), S(  29,  11), S(   0,   0), 

}, 
arr_weight_queen_mobility[32] = { 

S( -12,  11), S(  -6, -76), S(  -3, -66), S(  -5, -62), S(  -1, -60), S(  -1, -33), S(   2, -36), S(  -1, -17), 
S(   1, -10), S(   7,  -2), S(   6,   6), S(   7,   0), S(   7,   8), S(   8,  13), S(  12,   6), S(  12,  16), 
S(   0,  27), S(  15,  18), S(  18,  15), S(   8,  21), S(  11,  23), S(  11,  24), S(  29,  -5), S(  50, -16), 
S(  45, -16), S(  36, -10), S(  22,  -5), S(  -2, -13), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 


weight_pawn_shield[8][8] = { 

S( -25,   0), S(  15, -29), S(  19, -14), S(  -2,   0), S(   5,  13), S(  11,  24), S(  16,  24), S(   0,   0), 
S( -29,   3), S(  26, -15), S(  15,  -8), S( -14,   1), S( -19,  19), S( -42,  26), S( -33,  40), S(   0,   0), 
S( -25,   0), S(  28, -13), S(  -6,  -2), S(  -8,   2), S( -14,   5), S(  -7,  15), S(  15,  25), S(   0,   0), 
S( -18,  -5), S(   0,  -8), S(   2,  -1), S(  -3,   4), S( -17,  10), S( -35,  20), S( -37,  36), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
weight_blocked_pawn_storm[8][8] = { 

S(   0,   0), S(   0,   0), S(  -3, -44), S(  26, -20), S(  16, -31), S(  27, -50), S(   8, -52), S(   0,   0), 
S(   0,   0), S(   0,   0), S( -53, -36), S(  19, -26), S(  -1, -19), S(   6, -40), S(  26, -46), S(   0,   0), 
S(   0,   0), S(   0,   0), S( -40,  -2), S( -21,  -8), S(  -1, -24), S(  -5, -28), S( -15, -33), S(   0,   0), 
S(   0,   0), S(   0,   0), S( -12, -43), S(  -2, -17), S(   9, -24), S(  11, -21), S(  42, -42), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 
weight_unblocked_pawn_storm[8][8] = { 

S(  -1, -23), S(  31, 191), S( -24, 130), S( -32,  49), S(  -7,   3), S(   5, -14), S(   7, -10), S(   0,   0), 
S(   1, -19), S(  26, 192), S( -36, 124), S( -10,  37), S(  -7,   7), S(   6,  -8), S(   1,  -9), S(   0,   0), 
S(   1, -18), S( -19, 160), S( -93, 110), S(  -9,  31), S(   1,   2), S(   3, -12), S(  12, -15), S(   0,   0), 
S(  -7,  -1), S( -28, 151), S( -46,  98), S( -18,  23), S(  -2,   2), S(   3,   0), S(  -6,  -1), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 

}, 

weight_knight_attack = S(17, 31), 
weight_bishop_attack = S(10, 32), 
weight_rook_attack = S(9, 31), 
weight_queen_attack = S(11, 32), 
weight_safe_knight_check = S(127, -1), 
weight_safe_bishop_check = S(38, 275), 
weight_safe_rook_check = S(75, 11), 
weight_safe_queen_check = S(40, 51), 
weight_unsafe_knight_check = S(13, -3), 
weight_unsafe_bishop_check = S(25, 132), 
weight_unsafe_rook_check = S(37, -11), 
weight_unsafe_queen_check = S(21, 15), 
weight_safety_adjustment = S(82, -62), 


pawnPSQT[U8_MAX_SQUARES] = {

S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
S(  -9,   4), S( -14,   2), S( -17,   0), S( -13,  -6), S( -13,  -2), S( -10,   3), S(  13,  -4), S(  -2,  -3), 
S(  -7,  -5), S( -12,  -7), S(   6, -14), S(   5, -17), S(  12, -15), S(  -1,  -7), S(  15,  -9), S(  -9, -11), 
S( -15,   2), S( -14,   0), S(   1, -16), S(  15, -26), S(  18, -28), S(   4,  -9), S(  -6,  -2), S( -35,  -6), 
S(  -2,  15), S(   1,   6), S(  -2,  -3), S(  18, -28), S(  26, -26), S(   6,   1), S(   8,   3), S( -23,   5), 
S(  -4,  33), S(  -6,  26), S(  25,  11), S(   4, -19), S(  34, -27), S(  44,   2), S(  10,  15), S( -10,  18), 
S(  22,  73), S(   7,  72), S( -10,  60), S(  21,  25), S(  46,   9), S(  37,  45), S(  31,  72), S( -35,  68), 
S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), 
},

knightPSQT[U8_MAX_SQUARES] = {

S( -48, -27), S( -11, -14), S( -40,  -5), S( -12,  -4), S(  -8, -15), S( -18,  -2), S( -14, -22), S( -60, -11), 
S( -13, -20), S( -25, -10), S(  -2, -12), S(   8, -10), S(  14, -17), S(  -1,  -8), S(  -2, -14), S(  -6, -24), 
S( -14, -12), S(   2,  -5), S(   5, -14), S(   3,  11), S(  12,   8), S(   8, -16), S(  22, -19), S( -16, -21), 
S(   6,  -8), S(   2,   3), S(  13,  14), S(  11,  17), S(  12,  18), S(  18,   1), S(  -4,   0), S(  -3,  -5), 
S(  22,  -7), S(  20,   7), S(   8,  18), S(  36,  21), S(  23,  14), S(  18,  20), S(  22,   6), S(  30, -10), 
S( -16, -19), S(  12, -11), S(  15,  13), S(  59,   5), S(  82, -14), S( 113, -12), S(  52, -19), S(  -3,  -9), 
S( -66, -16), S( -34,  -4), S(  77, -26), S(  17,  -3), S(  87, -28), S(  73, -28), S(   6, -17), S(  -9, -28), 
S(-213, -10), S( -39, -39), S( -54, -13), S( -35,  -5), S(  10, -19), S(-108, -14), S( -73, -23), S(-105, -65), 
},

bishopPSQT[U8_MAX_SQUARES] = {

S(  14, -25), S(  -4,  -7), S(   8,  -7), S(   7,  -2), S(   1,  -6), S( -14,  -1), S(   2, -23), S(  11, -21), 
S( -16,  -2), S(  32, -18), S(  11, -10), S(   8,  -1), S(  12,  -2), S(  22, -10), S(  42, -19), S(   3, -25), 
S(  26, -20), S(  18,   1), S(  19,   4), S(  -1,  11), S(  12,   8), S(  22,  -2), S(  10, -12), S(  19, -20), 
S(  -7,  -9), S(   0,  -1), S(   2,   8), S(  23,   5), S(  21,   6), S(   6,   9), S(   6,  -9), S( -15, -12), 
S( -24,   5), S(  -4,   9), S(   3,   7), S(  10,  16), S(  23,   8), S( -18,   7), S(  16,  -7), S( -13,  -6), 
S( -22,   1), S(  -7,   3), S(  41,  -3), S(   1,  10), S(  45,  -4), S(  46,   8), S(  33,  -5), S(  23,  -9), 
S( -45,  -8), S(  -9,  -2), S( -19,  -2), S( -10, -12), S(   2,  -6), S(  55, -19), S(   1,  -2), S(  13, -32), 
S( -30,  -8), S( -51, -18), S( -19, -17), S( -70,  -3), S( -92,   0), S( -27,  -8), S(  -5, -16), S( -48,   0), 
},

rookPSQT[U8_MAX_SQUARES] = {

S(  -1,   1), S(   0,  -1), S(   5,  -1), S(   7,   0), S(  18, -12), S(  17,  -8), S( -14,  -7), S(  16, -29), 
S( -19,  -5), S( -18,  -2), S( -20,   5), S( -10,   0), S(  -1,  -7), S(   3,  -9), S(  14, -14), S( -34,  -5), 
S( -17,  -5), S( -16,  -3), S( -16,  -1), S(   3,  -5), S(   2,  -6), S(  11, -15), S(  16, -20), S(  10, -19), 
S( -24,   4), S( -34,   9), S( -35,  13), S( -22,  11), S( -18,   8), S( -12,  -1), S(   2,  -8), S(  -6, -12), 
S( -30,  12), S( -11,   4), S(  -8,   9), S(  12,   1), S( -14,   5), S(   3,   2), S(  16,  -5), S(  16,  -5), 
S(  -1,   9), S(  -8,  10), S( -18,   9), S(   7,   2), S(  29,  -5), S(  34,  -6), S(  58,  -9), S(  28,  -7), 
S( -20,   9), S( -42,  26), S(   0,  11), S(  37,  -1), S(  -1,   1), S(  58, -11), S(  70, -14), S(  48, -15), 
S( -15,  14), S(   7,   4), S(  -7,  10), S(   6,   0), S(  30,  -5), S(  45, -10), S(  40, -11), S(  -6,   2), 
},

queenPSQT[U8_MAX_SQUARES] = {

S(  13, -22), S(  11, -33), S(  14, -31), S(  18, -29), S(  16, -21), S(   0, -32), S( -18, -31), S(   8, -36), 
S(   5, -18), S(   7, -10), S(  10, -14), S(  15,  -9), S(  19, -13), S(  30, -52), S(  30, -47), S(  13, -24), 
S(   3, -42), S(  14, -39), S(  -1,  17), S(   7, -14), S(   5,   2), S(  16,  -6), S(  14,   7), S(   3, -12), 
S(   3, -23), S( -15,  20), S( -12,   4), S( -19,  36), S(  -3,  15), S(   6,  -3), S(   3,   9), S(  -1,  -4), 
S( -26,  -5), S(  -9,   1), S( -24,   9), S( -30,  37), S( -15,  40), S( -18,  45), S( -14,  30), S(   6,   9), 
S( -26, -23), S( -25,  -5), S(   7,   1), S(  -6,  12), S(  13,  49), S(  18,  15), S(  71, -12), S(  14,   4), 
S( -35,   8), S( -47,   9), S( -45,  29), S( -60,  64), S( -61,  56), S(  57,  46), S(  -9,  56), S(  95, -46), 
S( -28,  -3), S( -10,   1), S(  12,   1), S(  26,  21), S(  33,  15), S(  41,  13), S(   4, -10), S(  -8,  34), 
},

kingPSQT[U8_MAX_SQUARES] = {

S( -38, -34), S(  13, -23), S(   8, -21), S( -27, -25), S(  10, -38), S( -44, -13), S(   4, -21), S(  -5, -48), 
S(  25, -28), S(   0,  -3), S(  -4,   2), S( -28,   6), S( -38,   6), S( -28,  10), S(   2,   6), S(  -3, -16), 
S(  19, -24), S(  46,  -3), S( -16,  12), S( -38,  16), S( -35,  15), S( -17,  14), S(   8,  11), S( -39,   0), 
S(  32, -23), S(  54,   1), S(  36,   8), S( -25,  17), S( -18,  15), S( -36,  23), S( -10,  18), S( -68,  10), 
S(  19, -14), S(  33,  12), S(  60,   8), S(  24,   9), S(  12,  10), S(  21,  21), S(  34,  26), S( -23,  17), 
S(  43,  -6), S( 131,  11), S(  58,   7), S(  69,  -2), S(  17,   8), S(  90,  17), S(  94,  33), S(   8,  21), 
S(  12,  -6), S(  98,  10), S(  72,  -4), S(  49, -12), S(  42,  -8), S(  99,   8), S(  24,  33), S( -21,  19), 
S(  25, -38), S(  55,  -8), S(  72, -25), S(  50, -29), S(  72, -28), S(  63, -10), S(  29,   3), S(  -1, -18), 
};


#endif
