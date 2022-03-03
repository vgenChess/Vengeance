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

U32 createMove(U32 promotion_type, U32 castle_dir, U32 move_type, U32 color, U32 c_piece, U32 piece, U32 from, U32 to);

void genMoves(int ply, std::vector<Move> &moves, U8 color, Thread *th);

void genPushes(std::vector<Move> &moves, U8 color, Thread *th);
void genAttacks(int ply, std::vector<Move> &moves, U8 color, Thread *th);

void generateCaptures(U8 side, std::vector<Move> &moves, Thread *th);
void generatePushes(U8 side, std::vector<Move> &moves, Thread *th);

void genSpecialMoves(int ply, std::vector<Move> &moves, U8 sideToMove, Thread *th);

void genCastlingMoves(int ply, std::vector<Move> &moves, U8 color, Thread *th);
void genEnpassantMoves(int ply, std::vector<Move> &moves, U8 color, Thread *th);
void genPromotionsNormal(std::vector<Move> &moves, U8 sideToMove, Thread *th);
void genPromotionsAttacks(std::vector<Move> &moves, U8 sideToMove, Thread *th);

Move getNextMove(int ply, int side, Thread *th, MOVE_LIST *searchInfo);

#endif /* movegen_h */
