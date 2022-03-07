//
//  evaluate.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef evaluate_h
#define evaluate_h

#include "globals.h"
#include "thread.h"

void setDist();

void initTableDoublePawns();

int traceFullEval(side stm, TraceCoefficients *traceCoefficients, Thread *th);

int fullEval(U8 stm, Thread *th);

template<Side stm> int pawnsEval(Thread *th);
template<Side stm> int knightsEval(Thread *th);
template<Side stm> int bishopsEval(Thread *th);
template<Side stm> int rooksEval(Thread *th);
template<Side stm> int queenEval(Thread *th);
template<Side stm> int kingEval(Thread *th);
template<Side stm> int evalBoard(Thread *th); 

void initPSQT();

int numOfDoublePawns(U8 stm, Thread *th);
int countDefendedPawns(U8 stm, Thread *th);

U64 isolatedPawns(U8 stm, Thread *th);
U64 doublePawns(U8 stm, Thread *th);
U64 backwardPawns(U8 stm, Thread *th);
U64 pawnHoles(U8 stm, Thread *th);

U64 wPawnsBehindOwn(U64 wpawns);
U64 bPawnsBehindOwn(U64 bpawns); 
U64 wPawnsInfrontOwn (U64 wpawns);
U64 bPawnsInfrontOwn (U64 bpawns);

U64 wBackward(U64 wpawns, U64 bpawns);
U64 bBackward(U64 bpawns, U64 wpawns);

U64 wPassedPawns(U64 wpawns, U64 bpawns);
U64 bPassedPawns(U64 bpawns, U64 wpawns);

U64 wPawnDefendedFromWest(U64 wpawns);
U64 wPawnDefendedFromEast(U64 wpawns);
U64 bPawnDefendedFromWest(U64 bpawns);
U64 bPawnDefendedFromEast(U64 bpawns);

U64 noNeighborOnEastFile (U64 pawns);
U64 noNeighborOnWestFile (U64 pawns);
U64 isolanis(U64 pawns);

U64 openFiles(U64 wpanws, U64 bpawns);

U64 halfOpenOrOpenFile(U64 gen);

#endif /* evaluate_h */






























