//
//  evaluate.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <assert.h>

#include "evaluate.h"
#include "movegen.h"
#include "make_unmake.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "tuner.h"
#include "weights.h"
#include "functions.h"

TraceCoefficients *T;

int Mirror64[U8_MAX_SQUARES] = {

	56,	57,	58,	59,	60,	61,	62,	63,
	48,	49,	50,	51,	52,	53,	54,	55,
	40,	41,	42,	43,	44,	45,	46,	47,
	32,	33,	34,	35,	36,	37,	38,	39,
	24,	25,	26,	27,	28,	29,	30,	31,
	16,	17,	18,	19,	20,	21,	22,	23,
	8,	9,	10,	11,	12,	13,	14,	15,
	0,	1,	2,	3,	4,	5,	6,	7
};

U64 arrFiles[8] = {

	A_FILE, B_FILE, C_FILE, D_FILE,
	E_FILE, F_FILE, G_FILE, H_FILE
};

U64 arrRanks[8] = {

	RANK_1, RANK_2, RANK_3, RANK_4,
	RANK_5, RANK_6, RANK_7, RANK_8
};

// TODO check datatype
int PSQT[U8_MAX_SQUARES][U8_MAX_PIECES][U8_MAX_SQUARES];

template<Side stm>
void initEvalInfo(Thread *th) 
{	
	int sq = -1;
	U64 bb = 0ULL;
	for (int piece = KNIGHTS; piece <= KING; piece++) 
	{
		bb = stm ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
		
		while (bb) 
		{
			sq = GET_POSITION(bb);
			POP_POSITION(bb);

			assert(sq >= 0 && sq <= 63);

			if (piece == KNIGHTS) 
			{
				th->evalInfo.knightAttacks[stm][sq] = get_knight_attacks(sq);
				th->evalInfo.allKnightAttacks[stm] |= th->evalInfo.knightAttacks[stm][sq];
				th->evalInfo.attacks[stm] |= th->evalInfo.knightAttacks[stm][sq];		
			}	
			else if (piece == BISHOPS) 
			{
				th->evalInfo.bishopAttacks[stm][sq] = Bmagic(sq, th->occupied);
				th->evalInfo.allBishopAttacks[stm] |= th->evalInfo.bishopAttacks[stm][sq];		
				th->evalInfo.attacks[stm] |= th->evalInfo.bishopAttacks[stm][sq];		
			}
			else if (piece == ROOKS) 
			{	
				th->evalInfo.rookAttacks[stm][sq] = Rmagic(sq, th->occupied);
				th->evalInfo.allRookAttacks[stm] |= th->evalInfo.rookAttacks[stm][sq];						
				th->evalInfo.attacks[stm] |= th->evalInfo.rookAttacks[stm][sq];				
			}	
			else if (piece == QUEEN) 
			{
				th->evalInfo.queenAttacks[stm][sq] = Qmagic(sq, th->occupied);
				th->evalInfo.allQueenAttacks[stm] |= th->evalInfo.queenAttacks[stm][sq];
				th->evalInfo.attacks[stm] |= th->evalInfo.queenAttacks[stm][sq];		
			}
			else if (piece == KING)	
			{	
				th->evalInfo.kingAttacks[stm] = get_king_attacks(sq);			
				th->evalInfo.attacks[stm] |= th->evalInfo.kingAttacks[stm];
			}	
		}	
	}

	th->evalInfo.kingSq[stm] = stm == WHITE ? 
		GET_POSITION(th->whitePieceBB[KING]) : GET_POSITION(th->blackPieceBB[KING]);	

	// King Safety 
		
	th->evalInfo.kingZoneBB[stm] = stm == WHITE ? 
								th->evalInfo.kingAttacks[WHITE] | (th->evalInfo.kingAttacks[WHITE] >> 8) :
								th->evalInfo.kingAttacks[BLACK] | (th->evalInfo.kingAttacks[BLACK] << 8);

	th->evalInfo.kingAttackersCount[stm] = 0;
    th->evalInfo.kingAttackersWeight[stm] = 0;
  	th->evalInfo.kingAdjacentZoneAttacksCount[stm] = 0;
}

int traceFullEval(Side stm, TraceCoefficients *traceCoefficients, Thread *th) 
{
	T = traceCoefficients;

	return stm ? fullEval(BLACK, th) : fullEval(WHITE, th);
}

int fullEval(U8 stm, Thread *th) 
{	
	bool pawnsHashHit = false;

	#if defined(TUNE)
	
		U8 sq, kingSq;
		U64 bb;
		for (U8 side = WHITE; side <= BLACK; side++) 
		{
			for (U8 piece = PAWNS; piece <= KING; piece++) 
			{	
				bb = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];

				while (bb) 
				{
					sq = GET_POSITION(bb);
					POP_POSITION(bb);

					T->weight_val_pawn[side] 	+= 	piece == PAWNS 		? 1 : 0;
					T->weight_val_knight[side] 	+= 	piece == KNIGHTS 	? 1 : 0;
					T->weight_val_bishop[side] 	+= 	piece == BISHOPS 	? 1 : 0;
					T->weight_val_rook[side] 	+= 	piece == ROOKS 		? 1 : 0;
					T->weight_val_queen[side] 	+= 	piece == QUEEN 		? 1 : 0;
					
					kingSq = GET_POSITION(side ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);

					sq = side ? Mirror64[sq] : sq;
					kingSq = side ? Mirror64[kingSq] : kingSq;

					T->kingPSQT 	[kingSq]	[side] = piece == KING  	? 1 : 0;
					T->pawnPSQT	 	[kingSq][sq][side] = piece == PAWNS 	? 1 : 0; 			
					T->knightPSQT	[kingSq][sq][side] = piece == KNIGHTS 	? 1 : 0; 			
					T->bishopPSQT	[kingSq][sq][side] = piece == BISHOPS 	? 1 : 0; 			
					T->rookPSQT		[kingSq][sq][side] = piece == ROOKS 	? 1 : 0; 			
					T->queenPSQT	[kingSq][sq][side] = piece == QUEEN 	? 1 : 0; 			
				}
			}
		}

	#else

		//@TODO check eval hash implementation
	    auto evalHashEntry = &th->evalHashTable[th->hashKey % U16_EVAL_HASH_TABLE_RECORDS];
	    if (evalHashEntry->key == th->hashKey)
	    {
	    	return stm == WHITE ? evalHashEntry->score : -evalHashEntry->score;
	    }

	    auto pawnsHashEntry = &th->pawnsHashTable[th->pawnsHashKey % U16_PAWN_HASH_TABLE_RECORDS];
	    
	   	pawnsHashHit = pawnsHashEntry->key == th->pawnsHashKey;
	#endif


	th->evalInfo.clear();

	const auto whitePawns = th->whitePieceBB[PAWNS];
	const auto blackPawns = th->blackPieceBB[PAWNS];

	th->evalInfo.openFilesBB = 
		pawnsHashHit ? pawnsHashEntry->openFilesBB : openFiles(whitePawns, blackPawns); // @TODO check logic

	th->evalInfo.halfOpenFilesBB[WHITE] = 
		pawnsHashHit ? pawnsHashEntry->halfOpenFilesBB[WHITE] : halfOpenOrOpenFile(whitePawns) ^ th->evalInfo.openFilesBB; // @TODO check logic
	th->evalInfo.halfOpenFilesBB[BLACK] =
		pawnsHashHit ? pawnsHashEntry->halfOpenFilesBB[BLACK] :	halfOpenOrOpenFile(blackPawns) ^ th->evalInfo.openFilesBB; // @TODO check logic

	th->evalInfo.allPawnAttacks[WHITE] =
		pawnsHashHit ? pawnsHashEntry->allPawnAttacks[WHITE] : wPawnWestAttacks(whitePawns) | wPawnEastAttacks(whitePawns);
	th->evalInfo.allPawnAttacks[BLACK] = 
		pawnsHashHit ? pawnsHashEntry->allPawnAttacks[BLACK] : bPawnWestAttacks(blackPawns) | bPawnEastAttacks(blackPawns);

	th->evalInfo.attacks[WHITE] |= th->evalInfo.allPawnAttacks[WHITE];
	th->evalInfo.attacks[BLACK] |= th->evalInfo.allPawnAttacks[BLACK];


	initEvalInfo<WHITE>(th);
	initEvalInfo<BLACK>(th);


	int eval = 0;

	#if defined(TUNE)

		th->evalInfo.passedPawns[WHITE] = wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
		th->evalInfo.passedPawns[BLACK] = bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS]);

		eval += pawnsEval<WHITE>(th) 	- pawnsEval<BLACK>(th);
		eval += pawnKingEval<WHITE>(th) - pawnKingEval<BLACK>(th);
	#else
	
	    if (!pawnsHashHit)
	    {
		    // only calculate passed pawns if there is no cached evaluation for pawns	
			th->evalInfo.passedPawns[WHITE] = wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
			th->evalInfo.passedPawns[BLACK] = bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS]);

	    	pawnsHashEntry->key = th->pawnsHashKey;
	    
	    	pawnsHashEntry->pawnsEval = pawnsEval<WHITE>(th) - pawnsEval<BLACK>(th);
	    	pawnsHashEntry->pawnKingEval = pawnKingEval<WHITE>(th) - pawnKingEval<BLACK>(th);

			pawnsHashEntry->openFilesBB = th->evalInfo.openFilesBB;

			pawnsHashEntry->halfOpenFilesBB[WHITE] = th->evalInfo.halfOpenFilesBB[WHITE];
			pawnsHashEntry->halfOpenFilesBB[BLACK] = th->evalInfo.halfOpenFilesBB[BLACK]; 

			pawnsHashEntry->allPawnAttacks[WHITE] = th->evalInfo.allPawnAttacks[WHITE];
			pawnsHashEntry->allPawnAttacks[BLACK] = th->evalInfo.allPawnAttacks[BLACK];
	    }

     	eval += pawnsHashEntry->pawnsEval;
    	eval += pawnsHashEntry->pawnKingEval;
   
	#endif
	
	eval += knightsEval<WHITE>(th) 	- 	knightsEval<BLACK>(th);
	eval += bishopsEval<WHITE>(th) 	-	bishopsEval<BLACK>(th);
	eval += rooksEval<WHITE>(th) 	- 	rooksEval<BLACK>(th);
	eval += queenEval<WHITE>(th) 	- 	queenEval<BLACK>(th);

	// evaluation of other pieces other than king needs to be done first
	// because of values required for king safety calculation
	eval += kingSafety<WHITE>(th)	- 	kingSafety<BLACK>(th);
	eval += evalBoard<WHITE>(th)	- 	evalBoard<BLACK>(th);
	
	// Tapered evaluation 
	int phase = 	4 * POPCOUNT(th->whitePieceBB[QUEEN] 	| 	th->blackPieceBB[QUEEN]) 
				+ 	2 * POPCOUNT(th->whitePieceBB[ROOKS] 	| 	th->blackPieceBB[ROOKS])
      			+ 	1 * POPCOUNT(th->whitePieceBB[BISHOPS] 	| 	th->blackPieceBB[BISHOPS])
      			+ 	1 * POPCOUNT(th->whitePieceBB[KNIGHTS]	| 	th->blackPieceBB[KNIGHTS]);

    int score = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;

    #if defined(TUNE)

		T->eval = eval;
		T->phase = phase;
	#else

		evalHashEntry->key = th->hashKey;
		evalHashEntry->score = score;
	#endif

	return stm == WHITE ? score : -score;
}


// Pawn eval terms implemented
/*
->	Pawn PSQT
->	Pawn Islands
->	Isolated Pawns
->	Double Pawns
->	Backward Pawns
->	Pawn Holes
->	Pawn Chain
->	Pawn Phalanx
->	Passed Pawns
->	Defended Passed Pawns
->	Tarrasch rule
*/

template <Side stm>
int pawnsEval(Thread *th) 
{
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];

	int sq = -1, rank = -1;
	
	int score = 0;
	

	// Piece Square Tables
	
	auto ourPawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	while (ourPawns) 
	{
		sq = GET_POSITION(ourPawns);
		POP_POSITION(ourPawns);

		score += stm ? PSQT[Mirror64[kingSq]][PAWNS][Mirror64[sq]] : PSQT[kingSq][PAWNS][sq];
	}


	// Pawn islands
	/*
		At the start of a new game all the pawns are connected, but as the game continue and some exchanges are made, 
		the pawns may become disconnected. When a group of pawns gets disconnected from the rest of the pawn-structure 
		they become a pawn-island. Generally, the more pawn-islands you have, the harder it is to defend them all. 
		Therefore, more pawn-island usually implies a weaker pawn-structure.
	*/

	ourPawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	
	// https://www.chessprogramming.org/Pawn_Islands_(Bitboards)
	score += POPCOUNT(islandsEastFiles(ourPawns)) * weight_pawn_island;

	#if defined(TUNE)

		T->pawnIsland[stm] = POPCOUNT(islandsEastFiles(ourPawns));
	#endif


	/*
			Pawn Structures
          X   X   X   
		X  	   __________________A phalanx
		  X	 _|_
		X  	|  	|
		  X	P P P --------->	Defended phalanx pawn 
		X P X   X P ------->	Member of a pawn chain 
		P X   X   X P ----->	Base of a pawn chain
		X   X   X   X  
	*/

	// Pawn phalanx
	/*
		A pawn-phalanx occurs when 2 or more pawns are placed alongside each other. 
		They are usually quite useful in attack since together they control a lot of 
		squares in front of them and the one pawn will support the advance of the other. 
		However, a pawn-phalanx can also become vulnerable since they’re not defending each another 
		and you will need to use your pieces to defend them. 
		In such a case they are sometimes referred to as “hanging pawns.”
	*/
	
	// TODO check logic
	ourPawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	auto phalanxPawns = pawnsWithEastNeighbors(ourPawns) | pawnsWithWestNeighbors(ourPawns);
	while (phalanxPawns) 
	{
		sq = GET_POSITION(phalanxPawns);
		POP_POSITION(phalanxPawns);

		rank = stm ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		// Check if the phalanx pawn is defended by another pawn
		if (th->evalInfo.allPawnAttacks[stm] & (1ULL << sq)) 
		{	
			// Defended phalanx pawn
		
			score += arr_weight_defended_phalanx_pawn[rank];
			
			#if defined(TUNE)	
			
				T->defendedPhalanxPawn[rank][stm]++;
			#endif
		} 
		else 
		{	
			// Normal phalanx pawn

			score += arr_weight_phalanx_pawn[rank];

			#if defined(TUNE)	
			
				T->phalanxPawn[rank][stm]++;
			#endif
		}	
	}


	// Pawn chain
	/*
		A pawn chain refers to pawns that protect one another on a diagonal. 
		A pawn chain is often a fairly strong defensive setup. 
		One downside of a pawn-chain is that the pawns cover either light squares or dark squares, not both. 
		This implies you will need your pieces to protect the squares not covered by the pawns. 
		A pawn-chain could also be problematic to a bishop that moves on the same color squares.
	*/

	// TODO check logic
	auto p = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	// get the member (center) of at least triple chains
	// http://www.talkchess.com/forum3/viewtopic.php?t=55477
	auto b = defendedDefenders1(p) | defendedDefenders2(p);

	while (b) 
	{
		sq = GET_POSITION(b);
		POP_POSITION(b);
		
		rank = stm ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		// add the score based on the rank of the member
		score += arr_weight_pawn_chain[rank];
		
		#if defined(TUNE)	
		
			T->pawnChain[rank][stm]++;
		#endif
	}

	// Base pawns of a chain(not defended by another pawns) are not scored.
	// Since unprotected pawns are likely to be captured which results in a pawn loss for the side,
	// they are most likely to be protected by the side to avoid it.


	// Isolated pawns
	/*
		An isolated pawn is a pawn that does not have a pawn on either side of it. 
		Generally an isolated pawn can be a weakness in your position but on the other hand it also means 
		that your pieces will have better mobility around an isolated pawn since there movements aren’t restricted as much. 
		For this reason an isolated pawn is not at weak in the middle-game as it is in the endgame stage.
	*/

	score += POPCOUNT(isolatedPawns<stm>(th)) * weight_isolated_pawn;

	#if defined(TUNE)	
		
		T->isolatedPawns[stm] = POPCOUNT(isolatedPawns<stm>(th));
	#endif


	// Double pawns
	
	score += POPCOUNT(doublePawns<stm>(th)) * weight_double_pawn;

	#if defined(TUNE)	
		
		T->doublePawns[stm] = POPCOUNT(doublePawns<stm>(th));
	#endif


	// Backward pawns	
	/*
		A backward pawn is a pawn that is behind the pawns next to him and cannot move forward without being captured. 
		At the same time this pawn is on a semi-open file that makes it vulnerable to being attacked, particularly by rooks. 
		Backward pawns are often a significant weakness in your position.
	*/
	
	score += POPCOUNT(backwardPawns<stm>(th)) * weight_backward_pawn;

	#if defined(TUNE)	
		
		T->backwardPawns[stm] = POPCOUNT(backwardPawns<stm>(th));
	#endif


	// Pawn holes
	/*
		Weak squares generally refer to squares on the 5th or 6th rank (inside enemy territory) that cannot be defended by pawns. 
		A Square that cannot be defended by a pawn can more easily be occupied by a piece. 
		Therefore, weak squares are often an opportunity to further improve the development of your pieces.
		Naturally it should also be noted that, in most cases, 
		weak squares near or in the center are more useful than weak squares on the sides of the board
	*/
	
	score += POPCOUNT(pawnHoles<stm>(th)) * weight_pawn_hole;

	#if defined(TUNE)	
		
		T->pawnHoles[stm] = POPCOUNT(pawnHoles<stm>(th));
	#endif



	// Passed pawns
	/*
		A passed pawn refers to a pawn that cannot be stopped by enemy pawns from reaching the other side. 
		This often means your opponent will have to use a piece to stop the passed pawn. 
		This can give you an advantage since your opponent will have a piece that is tied down in a defensive task.
	*/

	auto stmPassedPawns = th->evalInfo.passedPawns[stm];
	auto oppPassedPawns = th->evalInfo.passedPawns[opp];

	// Tarrasch rule
	/*	
		Rooks should be placed behind passed pawns – either the player's or the opponent's
		The idea behind the guideline is that (1) if a player's rook is behind his passed pawn,
		the rook protects it as it advances, and (2) if it is behind an opponent's passed pawn, 
		the pawn cannot advance unless it is protected along its way.
	*/

	for (int file = 1; file <= 8; file++) 
	{
		if (arrFiles[file - 1] & th->evalInfo.allRookAttacks[stm] & stmPassedPawns) 
		{	
			score += weight_rook_behind_stm_passed_pawn;
			
			#if defined(TUNE) 

				T->rookBehindStmPassedPawn[stm]++;
			#endif
		}
		
		if (arrFiles[file - 1] & th->evalInfo.allRookAttacks[stm] & oppPassedPawns) 
		{	
			score += weight_rook_behind_opp_passed_pawn;
			
			#if defined(TUNE) 
			
				T->rookBehindOppPassedPawn[stm]++;
			#endif
		}
	}

	while (stmPassedPawns) 
	{	
		sq = GET_POSITION(stmPassedPawns);
		POP_POSITION(stmPassedPawns);

		assert(sq >= 0 && sq <= 63);

		rank = stm ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		assert(rank+1 >= 1 && rank+1 <= 8);
		
		// Check if the passed pawn is defended by another pawn
		if (th->evalInfo.allPawnAttacks[stm] & (1ULL << sq)) {
			
			// Defended passed pawn
		
			score += arr_weight_defended_passed_pawn[rank];
			
			#if defined(TUNE)	
			
				T->defendedPassedPawn[stm][rank]++;
			#endif
		} 
		else 
		{	
			// Normal passed pawn

			score += arr_weight_passed_pawn[rank];

			#if defined(TUNE)	
			
				T->passedPawn[stm][rank]++;
			#endif
		}	
	}

	return score;
}


template <Side stm>
int knightsEval(Thread *th) 
{
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];
	const auto allPawnsCount = POPCOUNT(th->blackPieceBB[PAWNS] | th->whitePieceBB[PAWNS]);
	const auto oppPawnHoles = ~th->evalInfo.allPawnAttacks[opp] & EXTENDED_CENTER;
	
	auto knightsBB = stm ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];
	
	int mobilityCount = 0, sq = -1;

	int score = 0;
	
	U64 attacksBB;
	
	while (knightsBB) 
	{
		sq = GET_POSITION(knightsBB);
		POP_POSITION(knightsBB);

		assert(sq >= 0 && sq <= 63);
		
		score += stm ? PSQT[Mirror64[kingSq]][KNIGHTS][Mirror64[sq]] : PSQT[kingSq][KNIGHTS][sq];


		attacksBB = th->evalInfo.knightAttacks[stm][sq];


		// Decreasing value as pawns disappear

		score += weight_knight_all_pawns_count * allPawnsCount;

		#if defined (TUNE)

			T->knightAllPawnsCount[stm] += allPawnsCount; 	
		#endif

		
		// 	Outpost
		//	An outpost is a square on the fourth, fifth, sixth, 
		//	or seventh rank which is protected by a pawn and which cannot be attacked by an opponent's pawn.
		if (oppPawnHoles & (1ULL << sq) & th->evalInfo.allPawnAttacks[stm]) 
		{
			score += weight_knight_outpost;

			#if defined(TUNE) 

				T->knightOutpost[stm]++;
			#endif
		}

		
		// Penalty for an undefended minor piece
		
		bool isDefended = (1ULL << sq) & th->evalInfo.attacks[stm]; 

		if (!isDefended) 
		{
			score += weight_undefended_knight;

			#if defined(TUNE)	
			
				T->undefendedKnight[stm]++;			
			#endif
		}

		
		// Marginal bonus for a knight defended by a pawn
		
		bool isDefendedByAPawn = (1ULL << sq) & th->evalInfo.allPawnAttacks[stm];
		
		if (isDefendedByAPawn) 
		{
			score += weight_knight_defended_by_pawn;

			#if defined(TUNE)	
			
				T->knightDefendedByPawn[stm]++;
			#endif
		}


		// Knight mobility
		
		U64 mobilityBB = attacksBB & th->empty & ~th->evalInfo.allPawnAttacks[opp];

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_knight_mobility[mobilityCount];

		#if defined(TUNE)

			T->knightMobility[stm][mobilityCount]++;
		#endif


		// Update values required for King safety 
		
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_knight_attack;

            #if defined(TUNE)	
            
            	T->knightAttack[opp]++; 
            #endif

            const auto bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb) 
            {
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}

	return score;
}

template <Side stm>
int bishopsEval(Thread *th) 
{
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];

	bool isDefended;

	int mobilityCount = 0, sq = -1;
	
	int score = 0;
	
	auto bishopBB = stm ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];

	U64 attacksBB;
	
	// Bishop pair
	if (POPCOUNT(bishopBB) == 2) 
	{	
		score += weight_bishop_pair;

		#if defined(TUNE)	
			
			T->bishopPair[stm] = 1;
		#endif
	}

	while (bishopBB) 
	{
		sq = GET_POSITION(bishopBB);
		POP_POSITION(bishopBB);

		assert(sq >= 0 && sq <= 63);
		
		attacksBB = th->evalInfo.bishopAttacks[stm][sq];		


		score += stm ? PSQT[Mirror64[kingSq]][BISHOPS][Mirror64[sq]] : PSQT[kingSq][BISHOPS][sq];


		// Penalty for an undefended minor piece
		
		isDefended = (1ULL << sq) & th->evalInfo.attacks[stm]; 

		if (!isDefended) 
		{
			score += weight_undefended_bishop;

			#if defined(TUNE)	
			
				T->undefendedBishop[stm]++;			
			#endif
		}


		// Bishop mobility

		mobilityCount = POPCOUNT(attacksBB & th->empty);

		score += arr_weight_bishop_mobility[mobilityCount];

		#if defined(TUNE) 

			T->bishopMobility[stm][mobilityCount]++;
		#endif



		// Update values required for King safety 

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_bishop_attack;

            #if defined(TUNE)	
            
            	T->bishopAttack[opp]++; 
            #endif
            	
            const auto bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb)	
            {
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}
	
	return score;
}


template <Side stm>
int rooksEval(Thread *th) 
{	
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];

	const auto oppFlankPawnHoles = ~th->evalInfo.allPawnAttacks[opp] 
								& ~EXTENDED_CENTER & ~(RANK_1 | RANK_2 | RANK_7 | RANK_8);
	
	auto rooksBB = stm ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];
	
	U64 attacksBB;

	int score = 0;
	int sq = -1, mobilityCount = 0;

	while (rooksBB) 
	{
		sq = GET_POSITION(rooksBB);
		POP_POSITION(rooksBB);

		assert(sq >= 0 && sq <= 63);


		attacksBB = th->evalInfo.rookAttacks[stm][sq];		


		score += stm ? PSQT[Mirror64[kingSq]][ROOKS][Mirror64[sq]] : PSQT[kingSq][ROOKS][sq];


		// Nimzowitsch argued when the outpost is in one of the flank (a-, b-, g- and h-) files 
		// the ideal piece to make use of the outpost is a rook. 
		// This is because the rook can put pressure on all the squares along the rank
		
		if (oppFlankPawnHoles & (1ULL << sq) & th->evalInfo.allPawnAttacks[stm]) 
		{
			score += weight_rook_flank_outpost;
			
			#if defined(TUNE)

				T->rookFlankOutpost[stm]++;
			#endif
		}


		// Rook on half open file
		
		if ((1ULL << sq) & th->evalInfo.halfOpenFilesBB[stm]) 
		{
			score += weight_rook_half_open_file;
		
			#if defined(TUNE)	
			
				T->halfOpenFile[stm]++;
			#endif
		}

		// Rook on open file
		
		if ((1ULL << sq) & th->evalInfo.openFilesBB) 
		{
			score += weight_rook_open_file;

			#if defined(TUNE)	
			
				T->openFile[stm]++;
			#endif
		}

		// Rook on same file as enemy Queen
		
		if (arrFiles[sq & 7] & 
			(opp ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) 
		{
			score += weight_rook_enemy_queen_same_file;

			#if defined(TUNE)	
			
				T->rookEnemyQueenSameFile[stm]++;
			#endif
		}

		// Rook on Seventh rank
		
		if ((1ULL << sq) & (stm ? RANK_2 : RANK_7)) 
		{
			score += weight_rook_on_seventh_rank;
		
			#if defined(TUNE)	
				
				T->rookOnSeventhRank[stm]++;
			#endif
		}

		// Rook on Eight rank
		
		if ((1ULL << sq) & (stm ? RANK_1 : RANK_8)) 
		{
			score += weight_rook_on_eight_rank;

			#if defined(TUNE)	

				T->rookOnEightRank[stm]++;							
			#endif
		} 	


		// Connected Rooks

		if (attacksBB & rooksBB) 
		{
			score += weight_rook_supporting_friendly_rook;

			#if defined(TUNE)	
				
				T->rookSupportingFriendlyRook[stm] = 1;
			#endif
		}


		// Rook mobility
		
		mobilityCount = POPCOUNT(attacksBB & th->empty);

		score += arr_weight_rook_mobility[mobilityCount]; 

		#if defined(TUNE)	
			
			T->rookMobility[stm][mobilityCount]++; 
		#endif


		// Update values required for King safety later in kingEval
		
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_rook_attack;

            #if defined(TUNE)	
            
            	T->rookAttack[opp]++; 
            #endif

            const auto bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb)	
            {	
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}

	return score;
}


template <Side stm>
int queenEval(Thread *th) 
{
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];
	auto queenBB = stm ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];
	
	int score = 0;
	int sq = -1;

	U64 attacksBB;

	while (queenBB) 
	{
		sq = GET_POSITION(queenBB);
		POP_POSITION(queenBB);

		assert(sq >= 0 && sq <= 63);

		attacksBB = th->evalInfo.queenAttacks[stm][sq];


		score += stm ? PSQT[Mirror64[kingSq]][QUEEN][Mirror64[sq]] : PSQT[kingSq][QUEEN][sq];


		if (	POPCOUNT(th->occupied) >= 22	// early game
			&&	(stm ? sq <= 48 : sq >= 15)) 
		{ 
			const auto minorPiecesBB = stm ? 
									th->blackPieceBB[KNIGHTS] | th->blackPieceBB[BISHOPS] :
									th->whitePieceBB[KNIGHTS] | th->whitePieceBB[BISHOPS];

			const auto underdevelopedPiecesBB = (stm ? RANK_8 : RANK_1) & minorPiecesBB; 

			score += POPCOUNT(underdevelopedPiecesBB) * weight_queen_underdeveloped_pieces;
			
			#if defined(TUNE)	
	
				T->queenUnderdevelopedPieces[stm] += POPCOUNT(underdevelopedPiecesBB);
			#endif
		}


		// Queen mobility
		
		score += arr_weight_queen_mobility[POPCOUNT(attacksBB & th->empty)];

		#if defined(TUNE)	
	
			T->queenMobility[stm][POPCOUNT(attacksBB & th->empty)]++;
		#endif


		// Update values required for King safety later in kingEval
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_queen_attack;

            #if defined(TUNE)	
    
            	T->queenAttack[opp]++; 
            #endif

            const auto bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb)	
            {
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}	

	return score;
}

template <Side stm>
int pawnKingEval(Thread *th)
{
	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];
	const auto ourPawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	const auto theirPawns = opp ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	// TODO redo logic (Often results in bad evaluation)
	auto pawnStormZone = th->evalInfo.kingZoneBB[stm];
	
	int score = 0;	
	
	assert(kingSq >= 0 && kingSq < 64);

	score += stm ? kingPSQT[Mirror64[kingSq]] : kingPSQT[kingSq];

	pawnStormZone |= stm ? pawnStormZone >> 8 : pawnStormZone << 8;

	// Anything strictly pawn related can be stored in the Pawn Hash Table, 
	// including pawn shield terms to be used dynamically for king safety.
	// TODO implement this in Pawn Hash Table

	score += POPCOUNT(th->evalInfo.kingZoneBB[stm] & ourPawns) * weight_king_pawn_shield;
	score += POPCOUNT(pawnStormZone & theirPawns) * weight_king_enemy_pawn_storm;

	#if defined(TUNE)	

		T->kingPawnShield[stm] 		= POPCOUNT(th->evalInfo.kingZoneBB[stm] & ourPawns); // TODO check logic
		T->kingEnemyPawnStorm[stm] 	= POPCOUNT(pawnStormZone & theirPawns); // TODO check logic
	#endif

	return score;
}

template <Side stm>
int kingSafety(Thread *th) 
{	

	int score = 0;

	constexpr auto opp = stm == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[stm];
	
	if (	th->evalInfo.kingAttackersCount[stm] >= 2
		&&	th->evalInfo.kingAdjacentZoneAttacksCount[stm]) 
	{ 
		int safetyScore = th->evalInfo.kingAttackersWeight[stm];

        U64 kingUndefendedSquares = th->evalInfo.attacks[opp]
        						&	th->evalInfo.kingAttacks[stm]
        						&	~(		th->evalInfo.allPawnAttacks[stm] 
        								| 	th->evalInfo.allKnightAttacks[stm] 
        								|	th->evalInfo.allBishopAttacks[stm] 
        								|	th->evalInfo.allRookAttacks[stm] 
        								|	th->evalInfo.allQueenAttacks[stm]);

		U64 enemyPieces = opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

		U64 queenSafeContactCheck = kingUndefendedSquares 
								&	th->evalInfo.allQueenAttacks[opp] 
								& 	~enemyPieces
								& 	(		th->evalInfo.allPawnAttacks[opp] 
										|	th->evalInfo.allKnightAttacks[opp] 
										|	th->evalInfo.allBishopAttacks[opp] 
										|	th->evalInfo.allRookAttacks[opp]);

		U64 rookSafeContactCheck = kingUndefendedSquares 
								&	th->evalInfo.allRookAttacks[opp] 
								&	~enemyPieces
								&	(		th->evalInfo.allPawnAttacks[opp] 
										|	th->evalInfo.allKnightAttacks[opp] 
										|	th->evalInfo.allBishopAttacks[opp] 
										|	th->evalInfo.allQueenAttacks[opp]);
							
		U64 safe 	= 	~(enemyPieces | th->evalInfo.attacks[stm]);
		U64 b1 		= 	Rmagic(kingSq, th->occupied) & safe;
		U64 b2 		= 	Bmagic(kingSq, th->occupied) & safe;

		U64 queenSafeChecks 	=	(b1 | b2) & th->evalInfo.allQueenAttacks[opp];
		U64 rookSafeChecks 		= 	b1 & th->evalInfo.allRookAttacks[opp];
		U64 bishopSafeChecks 	= 	b2 & th->evalInfo.allBishopAttacks[opp];
		U64 knightSafeChecks 	= 	get_knight_attacks(kingSq) & th->evalInfo.allKnightAttacks[opp] & safe;
		
		safetyScore +=	weight_queen_safe_contact_check *	POPCOUNT(queenSafeContactCheck);
		safetyScore +=	weight_rook_safe_contact_check	*	POPCOUNT(rookSafeContactCheck);
		safetyScore += 	weight_queen_check 				* 	POPCOUNT(queenSafeChecks);
		safetyScore += 	weight_rook_check 				* 	POPCOUNT(rookSafeChecks);
		safetyScore += 	weight_bishop_check				* 	POPCOUNT(bishopSafeChecks);
		safetyScore += 	weight_knight_check 			* 	POPCOUNT(knightSafeChecks);
		safetyScore += 	weight_safety_adjustment;

		#if defined(TUNE)

	        T->queenSafeContactCheck[stm] 	= 	POPCOUNT(queenSafeContactCheck);
	        T->rookSafeContactCheck[stm] 	= 	POPCOUNT(rookSafeContactCheck);
			T->queenCheck[stm] 				= 	POPCOUNT(queenSafeChecks);
			T->rookCheck[stm] 				= 	POPCOUNT(rookSafeChecks);
			T->bishopCheck[stm] 			= 	POPCOUNT(bishopSafeChecks);
			T->knightCheck[stm] 			= 	POPCOUNT(knightSafeChecks);
			T->safetyAdjustment[stm] 		=	1;
	    	T->safety[stm] 					= 	safetyScore;

		#endif
	
 		const auto mg = ScoreMG(safetyScore), eg = ScoreEG(safetyScore);

        score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20); 
	} 
    else 
    {
    	#if defined(TUNE)
	    	
	    	T->knightAttack[stm] 	= 0;
			T->bishopAttack[stm] 	= 0;
			T->rookAttack[stm] 		= 0;
			T->queenAttack[stm]	 	= 0;
    	#endif
    } 

	return score;
}

template <Side stm>
int evalBoard(Thread *th) 
{
	int score = 0;

	score += POPCOUNT(CENTER & th->evalInfo.attacks[stm]) * weight_center_control;

	#if defined(TUNE)	
		T->centerControl[stm] = POPCOUNT(CENTER & th->evalInfo.attacks[stm]);
	#endif


	return score;
}

// Helper functions

void initPSQT() 
{
	for (U8 kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) 
	{
		for (U8 sq = 0; sq < U8_MAX_SQUARES; sq++) 
		{
			PSQT[kingSq][PAWNS][sq] 	= 	pawnPSQT[kingSq][sq] 	+ 	weight_val_pawn; 
			PSQT[kingSq][KNIGHTS][sq] 	= 	knightPSQT[kingSq][sq] 	+	weight_val_knight; 
			PSQT[kingSq][BISHOPS][sq] 	= 	bishopPSQT[kingSq][sq]	+ 	weight_val_bishop; 
			PSQT[kingSq][ROOKS][sq] 	= 	rookPSQT[kingSq][sq] 	+	weight_val_rook; 
			PSQT[kingSq][QUEEN][sq] 	= 	queenPSQT[kingSq][sq]	+ 	weight_val_queen; 
		}
	}
}
















