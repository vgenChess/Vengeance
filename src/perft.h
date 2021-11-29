//
//  perft.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef perft_h
#define perft_h

#include "globals.h"
#include "thread.h"

u64 perft(int perft, u8 depth, u8 color, Thread *th);
void startPerft(u8 side, u8 depth, Thread *th);

#endif /* perft_h */
