//
//  make_unmake.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef make_unmake_h
#define make_unmake_h

#include "types.h"
#include "thread.h"

void make_move(int ply, U32 move, GameInfo *th);
void unmake_move(int ply, U32 move, GameInfo *th);

void makeNullMove(int ply, GameInfo *th);
void unmakeNullMove(int ply, GameInfo *th); 

#endif /* make_unmake_h */
