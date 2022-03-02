#ifndef weights_h
#define weights_h 

#include "evaluate.h"
#include "functions.h" 

int 

weight_pawn = S(100, 100),
weight_knight = S(300, 300),
weight_bishop = S(300, 300),
weight_rook = S(500, 500),
weight_queen = S(900, 900), 


weight_isolated_pawn = S(0, 0),
weight_backward_pawn = S(0, 0),
weight_double_pawn = S(0, 0),
weight_defended_pawn = S(0, 0),
weight_pawn_hole = S(0, 0), 
arr_weight_passed_pawn[8] = { 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0) 
}, 
arr_weight_defended_passed_pawn[8] = { 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0) 
}, 

weight_undefended_knight = S(0, 0), weight_knight_defended_by_pawn = S(0, 0), 

weight_undefended_bishop = S(0, 0), 
weight_bishop_pair = S(0, 0), 
weight_bad_bishop = S(0, 0),

weight_rook_half_open_file = S(0, 0), 
weight_rook_open_file = S(0, 0), 
weight_rook_enemy_queen_same_file = S(0, 0), 
weight_rook_on_seventh_rank = S(0, 0), 
weight_rook_on_eight_rank = S(0, 0), 
weight_rook_supporting_friendly_rook = S(0, 0), 

weight_queen_underdeveloped_pieces = S(0, 0), 

weight_king_pawn_shield = S(0, 0), 
weight_king_enemy_pawn_storm = S(0, 0), 

arr_weight_knight_mobility[16] = { 

 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0) 

}, 
arr_weight_bishop_mobility[16] = { 
 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0) 

}, 
arr_weight_rook_mobility[16] = { 
 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0) 

}, 
arr_weight_queen_mobility[32] = { 

 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), 
 S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0)  

},

weight_center_control = S(1, 1), 

weight_knight_attack = S(1, 1), 
weight_bishop_attack = S(1, 1), 
weight_rook_attack = S(1, 1), 
weight_queen_attack = S(1, 1), 
weight_rook_safe_contact_check = S(1, 1), 
weight_queen_safe_contact_check = S(1, 1), 
weight_knight_check = S(1, 1), 
weight_bishop_check = S(1, 1), 
weight_rook_check = S(1, 1), 
weight_queen_check = S(1, 1), 
weight_safety_adjustment = S(1, 1)
;

#endif

