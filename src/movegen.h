//
//  movegen.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef movegen_h
#define movegen_h

#include <vector>

#include "types.h"
#include "thread.h"
#include "structs.h"

void genMoves(U8 stm, int ply, std::vector<Move> &moves, Thread *th);
template<Side stm> void genPushes(std::vector<Move> &moves, Thread *th);
template<Side stm> void genAttacks(int ply, std::vector<Move> &moves, Thread *th);
template<Side stm> void generateCaptures(std::vector<Move> &moves, Thread *th);
template<Side stm> void generatePushes(std::vector<Move> &moves, Thread *th);
template<Side stm> void genSpecialMoves(int ply, std::vector<Move> &moves, Thread *th);

template<Side stm> void genCastlingMoves(int ply, std::vector<Move> &moves, Thread *th);
template<Side stm> void genEnpassantMoves(int ply, std::vector<Move> &moves, Thread *th);
template<Side stm> void genPromotionsNormal(std::vector<Move> &moves, Thread *th);
template<Side stm> void genPromotionsAttacks(std::vector<Move> &moves, Thread *th);

Move getNextMove(Side stm, int ply, Thread *th, MOVE_LIST *moveList);

template<Side stm> U32 createMove(U32 promotion_type, U32 castle_dir, U32 move_type, U32 c_piece, U32 piece, U32 from, U32 to);

#endif /* movegen_h */
