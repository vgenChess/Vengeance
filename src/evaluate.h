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

class EvalInfo {
	
	public:

		u64 openFilesBB;
		u64 halfOpenFilesBB[2]; 

		u64 pawnsAttacks[2];
		u64 knightsAttacks[2];
		u64 bishopsAttacks[2];
		u64 rooksAttacks[2];
		u64 queensAttacks[2];
		u64 kingAttacks[2];

		u64 attacks[2];

		u64 kingZoneBB[2];

		int kingSq[2];

		int kingAttackersCount[2];
    int kingAttackersWeight[2];

    int kingAdjacentZoneAttacksCount[2];

    void clear() {

			this->openFilesBB = 0ULL;

			for (int i = 0; i < 2; i++) {

				this->halfOpenFilesBB[i] = 0ULL;

				this->pawnsAttacks[i] = 0ULL;
				this->knightsAttacks[i] = 0ULL;
				this->bishopsAttacks[i] = 0ULL;
				this->rooksAttacks[i] = 0ULL;
				this->queensAttacks[i] = 0ULL;
				this->kingAttacks[i] = 0ULL;

				this->attacks[i] = 0ULL;

				this->kingZoneBB[i] = 0ULL;

				this->kingSq[i] = 0;

				this->kingAttackersCount[i] = 0;
   			this->kingAttackersWeight[i] = 0;

    		this->kingAdjacentZoneAttacksCount[i] = 0;
			} 
		}
};

class TraceCoefficients {

	public:

		int nPawns[2];
		int nKnights[2];
		int nBishops[2];
		int nRooks[2];
		int nQueen[2];
		
		int isolatedPawns[2];
		int backwardPawns[2];
		int doublePawns[2];
		int defendedPawns[2];
		int pawnHoles[2];
		int passedPawn[2][8];
		int defendedPassedPawn[2][8];
		
		int undefendedKnight[2];
		int knightDefendedByPawn[2];
		
		int bishopPair[2];
		int undefendedBishop[2];
		int badBishop[2];

		int halfOpenFile[2];
		int openFile[2];
		int rookEnemyQueenSameFile[2];
		int rookSupportingFriendlyRook[2];
		int rookBlockedByKing[2];
		int rookOnSeventhRank[2];
		int rookOnEightRank[2];

		int queenUnderdevelopedPieces[2];

		int kingPawnShield[2];
		int kingEnemyPawnStorm[2];


		// Pieces Mobility

		int knightMobility[2][16];
		int bishopMobility[2][16];
		int rookMobility[2][16];
		int queenMobility[2][32];

		int pawnPSQT[2][64];
		int knightPSQT[2][64];
		int bishopPSQT[2][64];
		int rookPSQT[2][64];
		int queenPSQT[2][64];
		int kingPSQT[2][64];

    int knightAttack[2];
		int bishopAttack[2];
		int rookAttack[2];
		int queenAttack[2];

		int rookSafeContactCheck[2];
		int queenSafeContactCheck[2];

		int knightCheck[2];
		int bishopCheck[2];
		int rookCheck[2];
		int queenCheck[2];

		int safetyAdjustment[2];

		int phase;
		int eval;

		int safety[2];

		int centerControl[2];

		void clear() {

			for (int i = 0; i < 2; i++)
				this->nPawns[i] = 0;
			for (int i = 0; i < 2; i++)
				this->nKnights[i] = 0;
			for (int i = 0; i < 2; i++)
				this->nBishops[i] = 0;
			for (int i = 0; i < 2; i++)
				this->nRooks[i] = 0;
			for (int i = 0; i < 2; i++)
				this->nQueen[i] = 0;

			for (int i = 0; i < 2; i++)
				this->isolatedPawns[i] = 0;
			for (int i = 0; i < 2; i++)
				this->backwardPawns[i] = 0;
			for (int i = 0; i < 2; i++)
				this->doublePawns[i] = 0;
			for (int i = 0; i < 2; i++)
				this->defendedPawns[i] = 0;
			for (int i = 0; i < 2; i++)
				this->pawnHoles[i] = 0;

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 8; j++)
					this->passedPawn[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 8; j++)
					this->defendedPassedPawn[i][j] = 0;
			}

			for (int i = 0; i < 2; i++) 
				this->undefendedKnight[i] = 0;
			for (int i = 0; i < 2; i++)
				this->knightDefendedByPawn[i] = 0;
			

			for (int i = 0; i < 2; i++)
				this->bishopPair[i] = 0;
			for (int i = 0; i < 2; i++)
				this->undefendedBishop[i] = 0;
			for (int i = 0; i < 2; i++) 
				this->badBishop[i] = 0;


			for (int i = 0; i < 2; i++)
				this->halfOpenFile[i] = 0;
			for (int i = 0; i < 2; i++)
				this->openFile[i] = 0;
			for (int i = 0; i < 2; i++)
				this->rookEnemyQueenSameFile[i] = 0;
			for (int i = 0; i < 2; i++)
				this->rookSupportingFriendlyRook[i] = 0;
			for (int i = 0; i < 2; i++)
				this->rookBlockedByKing[i] = 0;
			for (int i = 0; i < 2; i++)
				this->rookOnSeventhRank[i] = 0;
			for (int i = 0; i < 2; i++)
				this->rookOnEightRank[i] = 0;

			for (int i = 0; i < 2; i++)
				this->queenUnderdevelopedPieces[i] = 0;
				
			for (int i = 0; i < 2; i++)
				this->kingPawnShield[i] = 0;
			for (int i = 0; i < 2; i++)
				this->kingEnemyPawnStorm[i] = 0;


			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 16; j++)
					this->knightMobility[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 16; j++)
					this->bishopMobility[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 16; j++)
					this->rookMobility[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 32; j++)
					this->queenMobility[i][j] = 0;
			}


			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 64; j++)
					this->pawnPSQT[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 64; j++)
					this->knightPSQT[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 64; j++)
					this->bishopPSQT[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 64; j++)
					this->rookPSQT[i][j] = 0;
			}

			for (int i = 0; i < 2; i++)	{
				for (int j = 0; j < 64; j++)
					this->queenPSQT[i][j] = 0;
			}

			for (int i = 0; i < 2; i++) { 	
				for (int j = 0; j < 64; j++)
					this->kingPSQT[i][j] = 0;
			}

			this->phase = 0;
			this->eval = 0;

			for (int i = 0; i < 2; i++) {
				
				this->safety[i] = 0;

		    this->knightAttack[i] = 0;
				this->bishopAttack[i] = 0;
				this->rookAttack[i] = 0;
				this->queenAttack[i] = 0;

				this->rookSafeContactCheck[i] = 0;
				this->queenSafeContactCheck[i] = 0;

				this->knightCheck[i] = 0;
				this->bishopCheck[i] = 0;
				this->rookCheck[i] = 0;
				this->queenCheck[i] = 0;

				this->safetyAdjustment[i] = 0;
			}
		}
};

void setDist();

void initTableDoublePawns();

int traceFullEval(TraceCoefficients *traceCoefficients, u8 sideToMove, Thread *th);
int fullEval(u8 sideToMove, Thread *th);
int evaluateSide(int side, EvalInfo *evalInfo, Thread *th);

int PSQTScore(u8 sideToMove, Thread *th);
int pawnsEval(u8 sideToMove, EvalInfo *evalInfo, Thread *th);
int knightsEval(u8 side, EvalInfo *evalInfo, Thread *th);
int bishopsEval(u8 side, EvalInfo *evalInfo, Thread *th);
int rooksEval(u8 side, EvalInfo *evalInfo, Thread *th);
int queenEval(u8 side, EvalInfo *evalInfo, Thread *th);
int kingEval(u8 side, EvalInfo *evalInfo, Thread *th);

int evalBoard(u8 side, Thread *th, EvalInfo *evalInfo); 

void initPSQT();

int numOfPawnHoles(u8 side, Thread *th);
int isolatedPawns(u8 side, Thread *th);
int numOfDoublePawns(u8 side, Thread *th);
int countBackWardPawns(u8 side, Thread *th);
int countPassedPawns(u8 side, Thread *th);
int countDefendedPawns(u8 side, Thread *th);

u64 wPawnsBehindOwn(u64 wpawns);
u64 bPawnsBehindOwn(u64 bpawns); 
u64 wPawnsInfrontOwn (u64 wpawns);
u64 bPawnsInfrontOwn (u64 bpawns);

u64 wBackward(u64 wpawns, u64 bpawns);
u64 bBackward(u64 bpawns, u64 wpawns);

u64 wPassedPawns(u64 wpawns, u64 bpawns);
u64 bPassedPawns(u64 bpawns, u64 wpawns);

u64 wPawnDefendedFromWest(u64 wpawns);
u64 wPawnDefendedFromEast(u64 wpawns);
u64 bPawnDefendedFromWest(u64 bpawns);
u64 bPawnDefendedFromEast(u64 bpawns);

u64 noNeighborOnEastFile (u64 pawns);
u64 noNeighborOnWestFile (u64 pawns);
u64 isolanis(u64 pawns);

u64 openFiles(u64 wpanws, u64 bpawns);

u64 halfOpenOrOpenFile(u64 gen);


inline int count_1s_max_15(u64 b) {
  unsigned w = unsigned(b >> 32), v = unsigned(b);
  v = v - ((v >> 1) & 0x55555555);
  w = w - ((w >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
  v = ((v+w) * 0x11111111) >> 28;
  return int(v);
}

int evaluateNNUE(u8 side, Thread *th);


#endif /* evaluate_h */






























