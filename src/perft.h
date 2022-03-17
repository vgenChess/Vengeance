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

U64 perft(int perft, U8 depth, U8 color, Thread *th);
void startPerft(U8 side, U8 depth, Thread *th);

#endif /* perft_h */
