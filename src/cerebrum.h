/*
 * The Cerebrum library
 * Copyright (c) 2020, by David Carteau. All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/****************************************************************************/
/** NAME : cerebrum.h                                                      **/
/** AUTHOR : David Carteau, France, December 2020                          **/
/** LICENSE: MIT (see above and "license.txt" file content)                **/
/****************************************************************************/

/* 
 * The library is composed of two parts :
 * - cerebrum.py : Python program which aims at training neural networks
 *   used in the Orion UCI chess engine
 * - cerebrum.h and cerebrum.c : inference code which can be embedded in a
 *   chess engine
 */

#ifndef CEREBRUM_H_INCLUDED
#define CEREBRUM_H_INCLUDED

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include "immintrin.h"
#include "thread.h"
#include "globals.h"


/****************************************************************************/
/** EXTERNAL FUNCTIONS                                                     **/
/****************************************************************************/

int nn_load(NN_Network* nn, char* filename);

void nn_inputs_upd_all(NN_Network* nn, Thread* th);
void nn_inputs_add_piece(NN_Network* nn, Thread* th, int piece_type, int piece_color, int piece_position);
void nn_inputs_del_piece(NN_Network* nn, Thread* th, int piece_type, int piece_color, int piece_position);
void nn_inputs_mov_piece(NN_Network* nn, Thread* th, int piece_type, int piece_color, int from, int to);

int nn_eval(NN_Network* nn, Thread* th, int color);

void refresh_accumulator(
    NnueAccumulator&        new_acc,          // storage for the result
    const std::vector<int>& active_features,  // the indices of features that are active for this position
    const int                   perspective       // the perspective to refresh
) 

#endif // CEREBRUM_H_INCLUDED
