//
//  make_unmake.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef make_unmake_h
#define make_unmake_h

#include "globals.h"
#include "thread.h"

void make_move(int ply, u32 move, Thread *th);
void unmake_move(int ply, u32 move, Thread *th);

void makeNullMove(int ply, Thread *th);
void unmakeNullMove(int ply, Thread *th); 

#endif /* make_unmake_h */
