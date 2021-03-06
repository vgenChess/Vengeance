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

void genMoves(Side stm, int ply, std::vector<Move> &moves, Thread *th);
void genPushes(Side stm, std::vector<Move> &moves, Thread *th);
void genAttacks(Side stm, int ply, std::vector<Move> &moves, Thread *th);
void generateCaptures(Side stm, std::vector<Move> &moves, Thread *th);
void generatePushes(Side stm, std::vector<Move> &moves, Thread *th);
void genSpecialMoves(Side stm, int ply, std::vector<Move> &moves, Thread *th);

void genCastlingMoves(Side stm, int ply, std::vector<Move> &moves, Thread *th);
void genEnpassantMoves(Side stm, int ply, std::vector<Move> &moves, Thread *th);
void genPromotionsNormal(Side stm, std::vector<Move> &moves, Thread *th);
void genPromotionsAttacks(Side stm, std::vector<Move> &moves, Thread *th);

Move getNextMove(Side stm, int ply, Thread *th, MOVE_LIST *moveList);

#endif /* movegen_h */
