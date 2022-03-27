#ifndef CLASSES_H
#define CLASSES_H

#include <cstring>

#include "types.h"
#include "constants.h"
#include "functions.h"

class Notation 
{    
public:
    static std::string algebricSq[64];
};

class EvalInfo 
{	

public:

	U64 openFilesBB;
	U64 halfOpenFilesBB[U8_MAX_SIDES]; 

	U64 knightAttacks[U8_MAX_SIDES][U8_MAX_SQUARES];
	U64 bishopAttacks[U8_MAX_SIDES][U8_MAX_SQUARES];
	U64 rookAttacks[U8_MAX_SIDES][U8_MAX_SQUARES];
	U64 queenAttacks[U8_MAX_SIDES][U8_MAX_SQUARES];

	U64 allPawnAttacks[U8_MAX_SIDES];
	U64 allKnightAttacks[U8_MAX_SIDES];
	U64 allBishopAttacks[U8_MAX_SIDES];
	U64 allRookAttacks[U8_MAX_SIDES];
	U64 allQueenAttacks[U8_MAX_SIDES];
	U64 kingAttacks[U8_MAX_SIDES];
	U64 attacks[U8_MAX_SIDES];

	int kingSq[U8_MAX_SIDES];
	int kingAttackersCount[U8_MAX_SIDES];
	int kingAttackersWeight[U8_MAX_SIDES];
	int kingAdjacentZoneAttacksCount[U8_MAX_SIDES];

	U64 passedPawns[U8_MAX_SIDES];

    void clear() {

		openFilesBB = 0ULL;

		for (int i = 0; i < U8_MAX_SIDES; i++) {

			passedPawns[i] = 0ULL;
	
			halfOpenFilesBB[i] = 0ULL;

			allPawnAttacks[i] = 0ULL;
			allKnightAttacks[i] = 0ULL;
			allBishopAttacks[i] = 0ULL;
			allRookAttacks[i] = 0ULL;
			allQueenAttacks[i] = 0ULL;
			kingAttacks[i] = 0ULL;

			for (int j = 0; j < U8_MAX_SQUARES; j++) {

				knightAttacks[i][j] = 0ULL;
				bishopAttacks[i][j] = 0ULL;
				rookAttacks[i][j] = 0ULL;
				queenAttacks[i][j] = 0ULL;
			}

			attacks[i] = 0ULL;

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
		
	int ply;
	int depth;
	int realDepth;
	
	U32 skipMove;
    
    U32 line[U16_MAX_PLY];
    
	SearchInfo() 
	{
	
		ply = 0;
		depth = 0;
	
		skipMove = 0UL;
        
        line[0] = NO_MOVE;
	}
};


class TraceCoefficients 
{

public:

	int weight_val_pawn[U8_MAX_SIDES];
	int weight_val_knight[U8_MAX_SIDES];
	int weight_val_bishop[U8_MAX_SIDES];
	int weight_val_rook[U8_MAX_SIDES];
	int weight_val_queen[U8_MAX_SIDES];
	

	// Pawns

	int isolatedPawns[U8_MAX_SIDES];
	int backwardPawns[U8_MAX_SIDES];
	int doublePawns[U8_MAX_SIDES];
	int pawnHoles[U8_MAX_SIDES];
	int pawnChain[8][U8_MAX_SIDES];
	int phalanxPawn[8][U8_MAX_SIDES];
	int defendedPhalanxPawn[8][U8_MAX_SIDES];
	int passedPawn[U8_MAX_SIDES][8];
	int defendedPassedPawn[U8_MAX_SIDES][8];
	
	int	minorPawnShield[U8_MAX_SIDES];

	// Knights

	int knightAllPawnsCount[U8_MAX_SIDES];
	int knightOutpost[U8_MAX_SIDES];
	int undefendedKnight[U8_MAX_SIDES];
	int knightDefendedByPawn[U8_MAX_SIDES];
	
	
	// Bishops

	int bishopPair[U8_MAX_SIDES];
	int undefendedBishop[U8_MAX_SIDES];

	// Rooks

	int rookBehindStmPassedPawn[U8_MAX_SIDES];
	int rookBehindOppPassedPawn[U8_MAX_SIDES];
	int rookFlankOutpost[U8_MAX_SIDES];
	int halfOpenFile[U8_MAX_SIDES];
	int openFile[U8_MAX_SIDES];
	int rookEnemyQueenSameFile[U8_MAX_SIDES];
	int rookSupportingFriendlyRook[U8_MAX_SIDES];
	int rookOnSeventhRank[U8_MAX_SIDES];
	int rookOnEightRank[U8_MAX_SIDES];
	
	// Queen

	int queenUnderdevelopedPieces[U8_MAX_SIDES];

	// Pieces Mobility

	int knightMobility[U8_MAX_SIDES][16];
	int bishopMobility[U8_MAX_SIDES][16];
	int rookMobility[U8_MAX_SIDES][16];
	int queenMobility[U8_MAX_SIDES][32];

	
	// Piece Square Tables

	int kingPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int pawnPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int knightPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int bishopPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int rookPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	int queenPSQT[U8_MAX_SQUARES][U8_MAX_SIDES];
	

	// King Safety

	int pawnShield[8][8][U8_MAX_SIDES];
	int blockedStorm[8][8][U8_MAX_SIDES];
	int unblockedStorm[8][8][U8_MAX_SIDES];

	int knightAttack[U8_MAX_SIDES];
	int bishopAttack[U8_MAX_SIDES];
	int rookAttack[U8_MAX_SIDES];
	int queenAttack[U8_MAX_SIDES];

	int safeKnightCheck[U8_MAX_SIDES];
	int safeBishopCheck[U8_MAX_SIDES];
	int safeRookCheck[U8_MAX_SIDES];
	int safeQueenCheck[U8_MAX_SIDES];

	int unsafeKnightCheck[U8_MAX_SIDES];
	int unsafeBishopCheck[U8_MAX_SIDES];
	int unsafeRookCheck[U8_MAX_SIDES];
	int unsafeQueenCheck[U8_MAX_SIDES];

	int safetyAdjustment[U8_MAX_SIDES];

	int phase;
	int eval;

	int safety[U8_MAX_SIDES];


	void clear() 
	{
		phase = 0;
		eval = 0;

		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{		
			weight_val_pawn[i] = 0;
			weight_val_knight[i] = 0;
			weight_val_bishop[i] = 0;
			weight_val_rook[i] = 0;
			weight_val_queen[i] = 0;
		}
		
		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{
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

		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{
			minorPawnShield[i] = 0;

			knightAllPawnsCount[i] = 0;
			knightOutpost[i] = 0;
			undefendedKnight[i] = 0;
			knightDefendedByPawn[i] = 0;
		}

		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{
			bishopPair[i] = 0;
			undefendedBishop[i] = 0;
		}

		for (int i = 0; i < U8_MAX_SIDES; i++) 
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
	
		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{
			queenUnderdevelopedPieces[i] = 0;
		}
			
		for (int i = 0; i < U8_MAX_SIDES; i++)	
		{
			for (int j = 0; j < 16; j++) 
			{
				knightMobility[i][j] = 0;
				bishopMobility[i][j] = 0;
				rookMobility[i][j] = 0;			
			}
		}

		for (int i = 0; i < U8_MAX_SIDES; i++)	
		{
			for (int j = 0; j < 32; j++)
				queenMobility[i][j] = 0;
		}

		for (U8 side = WHITE; side <= BLACK; side++) 
		{
			for (int i = 0; i < 64; i++) 
			{
				kingPSQT[i][side] = 0;
				pawnPSQT[i][side] = 0;
				knightPSQT[i][side] = 0;
				bishopPSQT[i][side] = 0;
				rookPSQT[i][side] = 0;
				queenPSQT[i][side] = 0;
			}
		}
	

    	memset(pawnShield, 0, sizeof(int) * 8 * 8 * 2);
    	memset(blockedStorm, 0, sizeof(int) * 8 * 8 * 2);
    	memset(unblockedStorm, 0, sizeof(int) * 8 * 8 * 2);
		

		for (int i = 0; i < U8_MAX_SIDES; i++) 
		{				
	    	knightAttack[i] = 0;
			bishopAttack[i] = 0;
			rookAttack[i] = 0;
			queenAttack[i] = 0;

			safeKnightCheck[i] = 0;
			safeBishopCheck[i] = 0;
			safeRookCheck[i] = 0;
			safeQueenCheck[i] = 0;

			unsafeKnightCheck[i] = 0;
			unsafeBishopCheck[i] = 0;
			unsafeRookCheck[i] = 0;
			unsafeQueenCheck[i] = 0;

			safetyAdjustment[i] = 0;

			safety[i] = 0;
		}
	}
};

#endif
