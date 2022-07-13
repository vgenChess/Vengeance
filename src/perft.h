//
//  perft.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef perft_h
#define perft_h

#include "thread.h"

U64 perft(int perft, U8 depth, Side color, GameInfo *th);
void startPerft(Side side, U8 depth, GameInfo *th);

#endif /* perft_h */
