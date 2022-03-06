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

	int weight_val_pawn[U8_MAX_SIDES];
	int weight_val_knight[U8_MAX_SIDES];
	int weight_val_bishop[U8_MAX_SIDES];
	int weight_val_rook[U8_MAX_SIDES];
	int weight_val_queen[U8_MAX_SIDES];
	

	// Pawns

	int pawnIsland[U8_MAX_SIDES];
	int pawnChain[U8_MAX_SIDES];
	int isolatedPawns[U8_MAX_SIDES];
	int backwardPawns[U8_MAX_SIDES];
	int doublePawns[U8_MAX_SIDES];
	int pawnHoles[U8_MAX_SIDES];
	int pawnPhalanx[8][U8_MAX_SIDES];
	int passedPawn[U8_MAX_SIDES][8];
	int defendedPassedPawn[U8_MAX_SIDES][8];
	
	// Knights

	int knightAllPawnsCount[U8_MAX_SIDES];
	int knightOutpost[U8_MAX_SIDES];
	int undefendedKnight[U8_MAX_SIDES];
	int knightDefendedByPawn[U8_MAX_SIDES];
	
	
	// Bishops

	int bishopPair[U8_MAX_SIDES];


	// Rooks

	int rookBehindPassedPawn[U8_MAX_SIDES];
	int rookFlankOutpost[U8_MAX_SIDES];
	int halfOpenFile[U8_MAX_SIDES];
	int openFile[U8_MAX_SIDES];
	int rookEnemyQueenSameFile[U8_MAX_SIDES];
	int rookSupportingFriendlyRook[U8_MAX_SIDES];
	int rookOnSeventhRank[U8_MAX_SIDES];
	int rookOnEightRank[U8_MAX_SIDES];
	

	// Queen

	int queenUnderdevelopedPieces[U8_MAX_SIDES];


	// King 

	int kingPawnShield[U8_MAX_SIDES];
	int kingEnemyPawnStorm[U8_MAX_SIDES];


	// Pieces Mobility

	int knightMobility[U8_MAX_SIDES][16];
	int bishopMobility[U8_MAX_SIDES][16];
	int rookMobility[U8_MAX_SIDES][16];
	int queenMobility[U8_MAX_SIDES][32];

	
	// Piece Square Tables

	int kingPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int pawnPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES][U8_MAX_SIDES];
	int knightPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES][U8_MAX_SIDES];
	int bishopPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES][U8_MAX_SIDES];
	int rookPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES][U8_MAX_SIDES];
	int queenPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES][U8_MAX_SIDES];
	

	// King Safety

	int knightAttack[U8_MAX_SIDES];
	int bishopAttack[U8_MAX_SIDES];
	int rookAttack[U8_MAX_SIDES];
	int queenAttack[U8_MAX_SIDES];

	int rookSafeContactCheck[U8_MAX_SIDES];
	int queenSafeContactCheck[U8_MAX_SIDES];

	int knightCheck[U8_MAX_SIDES];
	int bishopCheck[U8_MAX_SIDES];
	int rookCheck[U8_MAX_SIDES];
	int queenCheck[U8_MAX_SIDES];

	int safetyAdjustment[U8_MAX_SIDES];

	int phase;
	int eval;

	int safety[U8_MAX_SIDES];


	// General Board features

	int centerControl[U8_MAX_SIDES];

	void clear() {

		phase = 0;
		eval = 0;

		for (int i = 0; i < U8_MAX_SIDES; i++) {
			
			weight_val_pawn[i] = 0;
			weight_val_knight[i] = 0;
			weight_val_bishop[i] = 0;
			weight_val_rook[i] = 0;
			weight_val_queen[i] = 0;
		}
		
		for (int i = 0; i < U8_MAX_SIDES; i++) {

			pawnIsland[i] = 0;
			pawnChain[i] = 0;
			isolatedPawns[i] = 0;
			backwardPawns[i] = 0;
			doublePawns[i] = 0;
			pawnHoles[i] = 0;

			for (int j = 0; j < 8; j++) {

				pawnPhalanx[j][i] = 0;
				passedPawn[i][j] = 0;	
				defendedPassedPawn[i][j] = 0;			
			}
		}

		for (int i = 0; i < U8_MAX_SIDES; i++) {

			knightAllPawnsCount[i] = 0;
			knightOutpost[i] = 0;
			undefendedKnight[i] = 0;
			knightDefendedByPawn[i] = 0;
		}

		for (int i = 0; i < U8_MAX_SIDES; i++) {

			bishopPair[i] = 0;
		}

		for (int i = 0; i < U8_MAX_SIDES; i++) {
			
			rookBehindPassedPawn[i] = 0;
			rookFlankOutpost[i] = 0;
			halfOpenFile[i] = 0;
			openFile[i] = 0;
			rookEnemyQueenSameFile[i] = 0;
			rookSupportingFriendlyRook[i] = 0;
			rookOnSeventhRank[i] = 0;
			rookOnEightRank[i] = 0;		
		}
	
		for (int i = 0; i < U8_MAX_SIDES; i++) {

			queenUnderdevelopedPieces[i] = 0;
		}
			
		for (int i = 0; i < U8_MAX_SIDES; i++)	{
			for (int j = 0; j < 16; j++) {

				knightMobility[i][j] = 0;
				bishopMobility[i][j] = 0;
				rookMobility[i][j] = 0;			
			}
		}

		for (int i = 0; i < U8_MAX_SIDES; i++)	{
			for (int j = 0; j < 32; j++)
				queenMobility[i][j] = 0;
		}

		for (U8 side = WHITE; side <= BLACK; side++) {

			for (int i = 0; i < 64; i++) {

				kingPSQT[i][side] = 0;

				for (int j = 0; j < 64; j++) {
					
					pawnPSQT[i][j][side] = 0;
					knightPSQT[i][j][side] = 0;
					bishopPSQT[i][j][side] = 0;
					rookPSQT[i][j][side] = 0;
					queenPSQT[i][j][side] = 0;
				}
			}
		}
	
		for (int i = 0; i < U8_MAX_SIDES; i++) {

			kingPawnShield[i] = 0;
			kingEnemyPawnStorm[i] = 0;
	
	    	knightAttack[i] = 0;
			bishopAttack[i] = 0;
			rookAttack[i] = 0;
			queenAttack[i] = 0;

			rookSafeContactCheck[i] = 0;
			queenSafeContactCheck[i] = 0;

			knightCheck[i] = 0;
			bishopCheck[i] = 0;
			rookCheck[i] = 0;
			queenCheck[i] = 0;

			safetyAdjustment[i] = 0;

			safety[i] = 0;
		}
	}
};

void setDist();

void initTableDoublePawns();

int traceFullEval(TraceCoefficients *traceCoefficients, U8 stm, Thread *th);

int fullEval(U8 stm, Thread *th);

int pawnsEval(U8 stm, Thread *th);
int knightsEval(U8 stm, Thread *th);
int bishopsEval(U8 stm, Thread *th);
int rooksEval(U8 stm, Thread *th);
int queenEval(U8 stm, Thread *th);
int kingEval(U8 stm, Thread *th);

int evalBoard(U8 stm, Thread *th); 

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






























