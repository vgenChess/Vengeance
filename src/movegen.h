//
//  movegen.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef movegen_h
#define movegen_h

#include "globals.h"
#include "thread.h"
#include "search.h"

u32 createMove(u32 promotion_type, u32 castle_dir, u32 move_type, u32 color, u32 c_piece, u32 piece, u32 from, u32 to);

void genMoves(int ply, std::vector<Move> &moves, u8 color, Thread *th);

void genPushes(std::vector<Move> &moves, u8 color, Thread *th);
void genAttacks(int ply, std::vector<Move> &moves, u8 color, Thread *th);

void generateCaptures(u8 side, std::vector<Move> &moves, Thread *th);
void generatePushes(u8 side, std::vector<Move> &moves, Thread *th);

void genSpecialMoves(int ply, std::vector<Move> &moves, u8 sideToMove, Thread *th);

void genCastlingMoves(int ply, std::vector<Move> &moves, u8 color, Thread *th);
void genEnpassantMoves(int ply, std::vector<Move> &moves, u8 color, Thread *th);
void genPromotionsNormal(std::vector<Move> &moves, u8 sideToMove, Thread *th);
void genPromotionsAttacks(std::vector<Move> &moves, u8 sideToMove, Thread *th);

void getMoves(const int ply, const int side, std::vector<Move> &moves, const int stage, const bool isQuiescense, Thread *th);

Move getNextMove(int ply, int side, Thread *th, MOVE_LIST *searchInfo);

#endif /* movegen_h */
