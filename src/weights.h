#ifndef weights_h
#define weights_h 

#include "evaluate.h"

int 

weight_pawn = S(92, 74),
weight_knight = S(391, 328),
weight_bishop = S(389, 327),
weight_rook = S(498, 532),
weight_queen = S(1189, 959), 


weight_isolated_pawn = S(-11, -5),
weight_backward_pawn = S(-6, -7),
weight_double_pawn = S(-9, -16),
weight_defended_pawn = S(9, 6),
weight_pawn_hole = S(-8, 1), 
arr_weight_passed_pawn[8] = { S(0, 0), S(0, 7), S(-5, 11), S(-15, 34), S(2, 58), S(3, 122), S(68, 169), S(0, 0), }, 
arr_weight_defended_passed_pawn[8] = { S(0, 0), S(0, 0), S(-1, 14), S(-3, 33), S(8, 65), S(64, 127), S(268, 117), S(0, 0), }, 

weight_undefended_knight = S(-12, -1), 
weight_knight_defended_by_pawn = S(3, 14), 

weight_undefended_bishop = S(28, 50), 
weight_bad_bishop = S(-6, -5), 
weight_bishop_pair = S(0, 1), 

weight_rook_half_open_file = S(16, 10), 
weight_rook_open_file = S(52, -7), 
weight_rook_enemy_queen_same_file = S(13, 16), 
weight_rook_on_seventh_rank = S(5, 15), 
weight_rook_on_eight_rank = S(0, 15), 
weight_rook_supporting_friendly_rook = S(3, 18), 

weight_queen_underdeveloped_pieces = S(-3, -14), 

weight_king_pawn_shield = S(16, 0), 
weight_king_enemy_pawn_storm = S(-37, 24), 

arr_weight_knight_mobility[16] = { 

S(-31, -142), S(-13, -92), S(-2, -71), S(3, -58), S(17, -49), S(21, -34), S(29, -34), S(31, -28), 
S(53, -43), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 

}, 
arr_weight_bishop_mobility[16] = { 

S(-17, -81), S(-11, -67), S(-2, -52), S(2, -40), S(4, -29), S(9, -22), S(14, -18), S(15, -12), 
S(18, -9), S(21, -10), S(23, -7), S(26, -9), S(18, -4), S(44, -6), S(0, 0), S(0, 0), 

}, 
arr_weight_rook_mobility[16] = { 

S(-21, -85), S(-8, -47), S(-6, -26), S(-2, -22), S(0, -14), S(4, -8), S(9, -3), S(14, -1), 
S(13, 5), S(18, 6), S(25, 9), S(27, 11), S(34, 15), S(46, 12), S(31, 16), S(0, 0), 

}, 
arr_weight_queen_mobility[32] = { 

S(4, -101), S(-2, -105), S(-3, -115), S(-6, -120), S(-2, -119), S(1, -99), S(-4, -86), S(-5, -52), 
S(-3, -61), S(-2, -46), S(1, -43), S(-1, -23), S(-5, -12), S(1, -3), S(3, 4), S(2, 10), 
S(7, 9), S(12, 17), S(20, 20), S(13, 33), S(15, 36), S(32, 30), S(33, 46), S(66, 28), 
S(81, 16), S(88, 32), S(88, 16), S(84, 27), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 

},

weight_center_control = S(5, -1), 

weight_knight_attack = S(98, 1), 
weight_bishop_attack = S(93, 1), 
weight_rook_attack = S(76, 1), 
weight_queen_attack = S(94, 174), 
weight_rook_safe_contact_check = S(53, -268), 
weight_queen_safe_contact_check = S(145, 267), 
weight_knight_check = S(29, -21), 
weight_bishop_check = S(58, 87), 
weight_rook_check = S(27, 107), 
weight_queen_check = S(14, -54), 
weight_safety_adjustment = S(-26, -175)
;

int SafetyTable[100] = {
    0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
  18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
  68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
 140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
 260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
 377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
 494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};


#endif

