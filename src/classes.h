#ifndef CLASSES_H
#define CLASSES_H

#include <string>

#include "types.h"
#include "constants.h"
#include "functions.h"
#include "enums.h"

class Notation 
{    
public:
    static std::string algebricSq[64];
};

class EvalInfo 
{	

public:
	
	U64 openFilesBB;
	U64 halfOpenFilesBB[MAX_SIDES]; 

	U64 knightAttacks[MAX_SIDES][MAX_SQUARES];
	U64 bishopAttacks[MAX_SIDES][MAX_SQUARES];
	U64 rookAttacks[MAX_SIDES][MAX_SQUARES];
	U64 queenAttacks[MAX_SIDES][MAX_SQUARES];

	U64 allPawnAttacks[MAX_SIDES];
	U64 allKnightAttacks[MAX_SIDES];
	U64 allBishopAttacks[MAX_SIDES];
	U64 allRookAttacks[MAX_SIDES];
	U64 allQueenAttacks[MAX_SIDES];
	U64 kingAttacks[MAX_SIDES];
	U64 attacks[MAX_SIDES];

	U64 kingZoneBB[MAX_SIDES];
	
	int kingSq[MAX_SIDES];
	int kingAttackersCount[MAX_SIDES];
	int kingAttackersWeight[MAX_SIDES];
	int kingAdjacentZoneAttacksCount[MAX_SIDES];

	U64 passedPawns[MAX_SIDES];

    void clear() {

		openFilesBB = 0ULL;

		for (int i = 0; i < MAX_SIDES; i++) {

			passedPawns[i] = 0ULL;
	
			halfOpenFilesBB[i] = 0ULL;

			allPawnAttacks[i] = 0ULL;
			allKnightAttacks[i] = 0ULL;
			allBishopAttacks[i] = 0ULL;
			allRookAttacks[i] = 0ULL;
			allQueenAttacks[i] = 0ULL;
			kingAttacks[i] = 0ULL;

			for (int j = 0; j < MAX_SQUARES; j++) {

				knightAttacks[i][j] = 0ULL;
				bishopAttacks[i][j] = 0ULL;
				rookAttacks[i][j] = 0ULL;
				queenAttacks[i][j] = 0ULL;
			}

			attacks[i] = 0ULL;

			kingZoneBB[i] = 0ULL;

			kingSq[i] = 0;

			kingAttackersCount[i] = 0;
			kingAttackersWeight[i] = 0;

			kingAdjacentZoneAttacksCount[i] = 0;
		} 
	}
};


class SearchInfo 
{

public:

	bool rootNode;
	bool mainThread;
	bool singularSearch;
	bool nullMove;

	int treePos;
	U32 skipMove;
    
    U32 line[MAX_PLY];
    
	SearchInfo() 
	{
		rootNode = false;
		mainThread = false;
		singularSearch = false;
		nullMove = false;

		      treePos = 0;

		skipMove = 0UL;
        
        line[0] = NO_MOVE;
	}
};


class TraceCoefficients 
{

public:

	int weight_val_pawn[MAX_SIDES];
	int weight_val_knight[MAX_SIDES];
	int weight_val_bishop[MAX_SIDES];
	int weight_val_rook[MAX_SIDES];
	int weight_val_queen[MAX_SIDES];
	

	// Pawns

	int pawnIsland[MAX_SIDES];
	int isolatedPawns[MAX_SIDES];
	int backwardPawns[MAX_SIDES];
	int doublePawns[MAX_SIDES];
	int pawnHoles[MAX_SIDES];
	int pawnChain[8][MAX_SIDES];
	int phalanxPawn[8][MAX_SIDES];
	int defendedPhalanxPawn[8][MAX_SIDES];
	int passedPawn[MAX_SIDES][8];
	int defendedPassedPawn[MAX_SIDES][8];
	
	// Knights

	int knightAllPawnsCount[MAX_SIDES];
	int knightOutpost[MAX_SIDES];
	int undefendedKnight[MAX_SIDES];
	int knightDefendedByPawn[MAX_SIDES];
	
	
	// Bishops

	int bishopPair[MAX_SIDES];
	int undefendedBishop[MAX_SIDES];

	// Rooks

	int rookBehindStmPassedPawn[MAX_SIDES];
	int rookBehindOppPassedPawn[MAX_SIDES];
	int rookFlankOutpost[MAX_SIDES];
	int halfOpenFile[MAX_SIDES];
	int openFile[MAX_SIDES];
	int rookEnemyQueenSameFile[MAX_SIDES];
	int rookSupportingFriendlyRook[MAX_SIDES];
	int rookOnSeventhRank[MAX_SIDES];
	int rookOnEightRank[MAX_SIDES];
	
	// Queen

	int queenUnderdevelopedPieces[MAX_SIDES];


	// King 

	int kingPawnShield[MAX_SIDES];
	int kingEnemyPawnStorm[MAX_SIDES];


	// Pieces Mobility

	int knightMobility[MAX_SIDES][16];
	int bishopMobility[MAX_SIDES][16];
	int rookMobility[MAX_SIDES][16];
	int queenMobility[MAX_SIDES][32];

	
	// Piece Square Tables

	int kingPSQT[MAX_SQUARES][MAX_SIDES];
	int pawnPSQT[MAX_SQUARES][MAX_SQUARES][MAX_SIDES];
	int knightPSQT[MAX_SQUARES][MAX_SQUARES][MAX_SIDES];
	int bishopPSQT[MAX_SQUARES][MAX_SQUARES][MAX_SIDES];
	int rookPSQT[MAX_SQUARES][MAX_SQUARES][MAX_SIDES];
	int queenPSQT[MAX_SQUARES][MAX_SQUARES][MAX_SIDES];
	

	// King Safety

	int knightAttack[MAX_SIDES];
	int bishopAttack[MAX_SIDES];
	int rookAttack[MAX_SIDES];
	int queenAttack[MAX_SIDES];

	int rookSafeContactCheck[MAX_SIDES];
	int queenSafeContactCheck[MAX_SIDES];

	int knightCheck[MAX_SIDES];
	int bishopCheck[MAX_SIDES];
	int rookCheck[MAX_SIDES];
	int queenCheck[MAX_SIDES];

	int safetyAdjustment[MAX_SIDES];

	int phase;
	int eval;

	int safety[MAX_SIDES];


	// General Board features

	int centerControl[MAX_SIDES];

	void clear() 
	{
		phase = 0;
		eval = 0;

		for (int i = 0; i < MAX_SIDES; i++) 
		{		
			weight_val_pawn[i] = 0;
			weight_val_knight[i] = 0;
			weight_val_bishop[i] = 0;
			weight_val_rook[i] = 0;
			weight_val_queen[i] = 0;
		}
		
		for (int i = 0; i < MAX_SIDES; i++) 
		{
			pawnIsland[i] = 0;
			isolatedPawns[i] = 0;
			backwardPawns[i] = 0;
			doublePawns[i] = 0;
			pawnHoles[i] = 0;

			for (int j = 0; j < 8; j++) 
			{	
				pawnChain[j][i] = 0;
				phalanxPawn[j][i] = 0;
				defendedPhalanxPawn[j][i] = 0;
				passedPawn[i][j] = 0;	
				defendedPassedPawn[i][j] = 0;			
			}
		}

		for (int i = 0; i < MAX_SIDES; i++) 
		{
			knightAllPawnsCount[i] = 0;
			knightOutpost[i] = 0;
			undefendedKnight[i] = 0;
			knightDefendedByPawn[i] = 0;
		}

		for (int i = 0; i < MAX_SIDES; i++) 
		{
			bishopPair[i] = 0;
			undefendedBishop[i] = 0;
		}

		for (int i = 0; i < MAX_SIDES; i++) 
		{	
			rookBehindStmPassedPawn[i] = 0;
			rookBehindOppPassedPawn[i] = 0;
			rookFlankOutpost[i] = 0;
			halfOpenFile[i] = 0;
			openFile[i] = 0;
			rookEnemyQueenSameFile[i] = 0;
			rookSupportingFriendlyRook[i] = 0;
			rookOnSeventhRank[i] = 0;
			rookOnEightRank[i] = 0;		
		}
	
		for (int i = 0; i < MAX_SIDES; i++) 
		{
			queenUnderdevelopedPieces[i] = 0;
		}
			
		for (int i = 0; i < MAX_SIDES; i++)	
		{
			for (int j = 0; j < 16; j++) 
			{
				knightMobility[i][j] = 0;
				bishopMobility[i][j] = 0;
				rookMobility[i][j] = 0;			
			}
		}

		for (int i = 0; i < MAX_SIDES; i++)	
		{
			for (int j = 0; j < 32; j++)
				queenMobility[i][j] = 0;
		}

		for (U8 side = WHITE; side <= BLACK; side++) 
		{
			for (int i = 0; i < 64; i++) 
			{
				kingPSQT[i][side] = 0;

				for (int j = 0; j < 64; j++) 
				{	
					pawnPSQT[i][j][side] = 0;
					knightPSQT[i][j][side] = 0;
					bishopPSQT[i][j][side] = 0;
					rookPSQT[i][j][side] = 0;
					queenPSQT[i][j][side] = 0;
				}
			}
		}
	
		for (int i = 0; i < MAX_SIDES; i++) 
		{
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

#endif
