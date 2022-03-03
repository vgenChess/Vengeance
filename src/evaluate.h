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

int traceFullEval(TraceCoefficients *traceCoefficients, U8 stm, Thread *th);

int fullEval(U8 stm, Thread *th);

int PSQTScore(U8 stm, Thread *th);
int pawnsEval(U8 stm, Thread *th);
int knightsEval(U8 stm, Thread *th);
int bishopsEval(U8 stm, Thread *th);
int rooksEval(U8 stm, Thread *th);
int queenEval(U8 stm, Thread *th);
int kingEval(U8 stm, Thread *th);

int evalBoard(U8 stm, Thread *th); 

void initPSQT();

int numOfPawnHoles(U8 stm, Thread *th);
int isolatedPawns(U8 stm, Thread *th);
int numOfDoublePawns(U8 stm, Thread *th);
int countBackWardPawns(U8 stm, Thread *th);
int countPassedPawns(U8 stm, Thread *th);
int countDefendedPawns(U8 stm, Thread *th);

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






























