//
//  evaluate.h
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef evaluate_h
#define evaluate_h

#include "thread.h"
#include "utility.h"

void setDist();
void initForwardRankMask();
void initTableDoublePawns();

int traceFullEval(Side stm, TraceCoefficients *traceCoefficients, Thread *th);
int fullEval(U8 stm, Thread *th);

template<Side stm> int evalBoard(Thread *th);
template<Side stm> int pawnsEval(Thread *th);
template<Side stm> int knightsEval(Thread *th);
template<Side stm> int bishopsEval(Thread *th);
template<Side stm> int rooksEval(Thread *th);
template<Side stm> int queenEval(Thread *th); 
template<Side stm> int kingSafety(Thread *th);

void initPSQT();
void initKingZoneBB();

// pawns with at least one pawn in front on the same file
inline U64 wPawnsBehindOwn(U64 wpawns) 
{
	return wpawns & wRearSpans(wpawns);
}

// pawns with at least one pawn in front on the same file
inline U64 bPawnsBehindOwn(U64 bpawns) 
{
	return bpawns & bRearSpans(bpawns);
}

// pawns with at least one pawn behind on the same file
inline U64 wPawnsInfrontOwn (U64 wpawns)
{
	return wpawns & wFrontSpans(wpawns);
}

// pawns with at least one pawn behind on the same file
inline U64 bPawnsInfrontOwn (U64 bpawns) 
{
	return bpawns & bFrontSpans(bpawns);
}

inline U64 wBackward(U64 wpawns, U64 bpawns) 
{
   U64 stops = wpawns << 8;

   U64 wAttackSpans = wEastAttackFrontSpans(wpawns)
                    | wWestAttackFrontSpans(wpawns);

   U64 bAttacks     = bPawnEastAttacks(bpawns)
                    | bPawnWestAttacks(bpawns);

   return (stops & bAttacks & ~wAttackSpans) >> 8;
}

inline U64 bBackward(U64 bpawns, U64 wpawns) 
{	
   U64 stops = bpawns >> 8;
   U64 bAttackSpans = bEastAttackFrontSpans(bpawns)
                    | bWestAttackFrontSpans(bpawns);

   U64 wAttacks     = wPawnEastAttacks(wpawns)
                    | wPawnWestAttacks(wpawns);

   return (stops & wAttacks & ~bAttackSpans) << 8;
}

inline U64 wPassedPawns(U64 wpawns, U64 bpawns) 
{
   U64 allFrontSpans = bFrontSpans(bpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return wpawns & ~allFrontSpans;
}

inline U64 bPassedPawns(U64 bpawns, U64 wpawns) 
{
   U64 allFrontSpans = wFrontSpans(wpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return bpawns & ~allFrontSpans;
}

inline U64 wPawnDefendedFromWest(U64 wpawns) 
{
   return wpawns & wPawnEastAttacks(wpawns);
}

inline U64 wPawnDefendedFromEast(U64 wpawns) 
{
   return wpawns & wPawnWestAttacks(wpawns);
}

inline U64 bPawnDefendedFromWest(U64 bpawns) 
{
   return bpawns & bPawnEastAttacks(bpawns);
}

inline U64 bPawnDefendedFromEast(U64 bpawns) 
{
   return bpawns & bPawnWestAttacks(bpawns);
}

inline U64 noNeighborOnEastFile (U64 pawns) 
{
    return pawns & ~westAttackFileFill(pawns);
}

inline U64 noNeighborOnWestFile (U64 pawns) 
{
    return pawns & ~eastAttackFileFill(pawns);
}

inline U64 isolanis(U64 pawns) 
{
   return  noNeighborOnEastFile(pawns)
         & noNeighborOnWestFile(pawns);
}

inline U64 openFiles(U64 wpanws, U64 bpawns) 
{ 
   return ~fileFill(wpanws) & ~fileFill(bpawns);
}

inline U64 halfOpenOrOpenFile(U64 gen) 
{
	return ~fileFill(gen);
}

template<Side stm>
inline U64 pawnHoles(Thread *th) 
{
	U64 pawnsBB = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	U64 frontAttackSpans = stm ? 
		bWestAttackFrontSpans(pawnsBB) | bEastAttackFrontSpans(pawnsBB)
		: wWestAttackFrontSpans(pawnsBB) | wEastAttackFrontSpans(pawnsBB); 

	U64 holes = ~frontAttackSpans & EXTENDED_CENTER
		& (stm ? (RANK_5 | RANK_6) : (RANK_3 | RANK_4)); 

	return holes;
}

template<Side stm>	
inline U64 isolatedPawns(Thread *th) 
{
	return isolanis(stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]);
}

template<Side stm> 
inline U64 doublePawns(Thread *th) 
{
	U64 pawnsBB = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	U64 doublePawnsBB = stm ? bPawnsInfrontOwn(pawnsBB) : wPawnsInfrontOwn(pawnsBB); 

	return doublePawnsBB;	
}

template<Side stm>
inline U64 backwardPawns(Thread *th) 
{
	return stm == WHITE ? 
			wBackward(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]) :
			bBackward(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS]);
}

#endif /* evaluate_h */






























