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
#include "structs.h"
#include "namespaces.h"

using namespace game;

void genMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th);
void genPushes(Side stm, std::vector<Move> &moves, GameInfo *th);
void genAttacks(Side stm, int ply, std::vector<Move> &moves, GameInfo *th);
void generateCaptures(Side stm, std::vector<Move> &moves, GameInfo *th);
void generatePushes(Side stm, std::vector<Move> &moves, GameInfo *th);

void genCastlingMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th);
void genEnpassantMoves(Side stm, int ply, std::vector<Move> &moves, GameInfo *th);
void genPromotionsNormal(Side stm, std::vector<Move> &moves, GameInfo *th);
void genPromotionsAttacks(Side stm, std::vector<Move> &moves, GameInfo *th);

Move getNextMove(Side stm, int ply, GameInfo *th, MOVE_LIST *moveList);

#endif /* movegen_h */
