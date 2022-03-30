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
#include "functions.h"

#if defined(__TEST_TUNER) || defined (__TUNE)
	#include "zerodEvals.h"
#else
	#include "tunedEvals.h"
#endif

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
int PSQT[U8_MAX_PIECES][U8_MAX_SQUARES];
U64 kingZoneBB[U8_MAX_SIDES][U8_MAX_SQUARES];
U64 forwardRanksBB[U8_MAX_SIDES][8]; 

template<Side side>
void initEvalInfo(Thread *th) 
{	
	int sq = -1;
	U64 bb = 0ULL;
	for (int piece = KNIGHTS; piece <= KING; piece++) 
	{
		bb = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
		
		while (bb) 
		{
			sq = GET_POSITION(bb);
			POP_POSITION(bb);

			assert(sq >= 0 && sq <= 63);

			if (piece == KNIGHTS) 
			{
				th->evalInfo.knightAttacks[side][sq] = get_knight_attacks(sq);
				th->evalInfo.allKnightAttacks[side] |= th->evalInfo.knightAttacks[side][sq];
				th->evalInfo.attacks[side] |= th->evalInfo.knightAttacks[side][sq];		
			}	
			else if (piece == BISHOPS) 
			{
				th->evalInfo.bishopAttacks[side][sq] = Bmagic(sq, th->occupied);
				th->evalInfo.allBishopAttacks[side] |= th->evalInfo.bishopAttacks[side][sq];		
				th->evalInfo.attacks[side] |= th->evalInfo.bishopAttacks[side][sq];		
			}
			else if (piece == ROOKS) 
			{	
				th->evalInfo.rookAttacks[side][sq] = Rmagic(sq, th->occupied);
				th->evalInfo.allRookAttacks[side] |= th->evalInfo.rookAttacks[side][sq];						
				th->evalInfo.attacks[side] |= th->evalInfo.rookAttacks[side][sq];				
			}	
			else if (piece == QUEEN) 
			{
				th->evalInfo.queenAttacks[side][sq] = Qmagic(sq, th->occupied);
				th->evalInfo.allQueenAttacks[side] |= th->evalInfo.queenAttacks[side][sq];
				th->evalInfo.attacks[side] |= th->evalInfo.queenAttacks[side][sq];		
			}
			else if (piece == KING)	
			{	
				th->evalInfo.kingAttacks[side] = get_king_attacks(sq);			
				th->evalInfo.attacks[side] |= th->evalInfo.kingAttacks[side];
			}	
		}	
	}

	th->evalInfo.kingSq[side] = side == WHITE ? 
		GET_POSITION(th->whitePieceBB[KING]) : GET_POSITION(th->blackPieceBB[KING]);	

	// King Safety 

	th->evalInfo.kingAttackersCount[side] = 0;
    th->evalInfo.kingAttackersWeight[side] = 0;
  	th->evalInfo.kingAdjacentZoneAttacksCount[side] = 0;
}

int traceFullEval(Side side, TraceCoefficients *traceCoefficients, Thread *th) 
{
	T = traceCoefficients;

	return side == WHITE ? fullEval(WHITE, th) : fullEval(BLACK, th);
}

int fullEval(U8 side, Thread *th) 
{	
	bool pawnsHashHit = false;

	#if defined(__TUNE) || defined(__TEST_TUNER)
	
		U8 sq;
		U64 bb;
		for (U8 s = WHITE; s <= BLACK; s++) 
		{
			for (U8 piece = PAWNS; piece <= KING; piece++) 
			{	
				bb = s ? th->blackPieceBB[piece] : th->whitePieceBB[piece];

				while (bb) 
				{
					sq = GET_POSITION(bb);
					POP_POSITION(bb);

					T->weight_val_pawn[s] 	+= 	piece == PAWNS 		? 1 : 0;
					T->weight_val_knight[s]	+= 	piece == KNIGHTS 	? 1 : 0;
					T->weight_val_bishop[s]	+= 	piece == BISHOPS 	? 1 : 0;
					T->weight_val_rook[s] 	+= 	piece == ROOKS 		? 1 : 0;
					T->weight_val_queen[s] 	+= 	piece == QUEEN 		? 1 : 0;
					
					sq = s ? Mirror64[sq] : sq;
	
					T->kingPSQT 	[sq][s] = piece == KING  	? 1 : 0;
					T->pawnPSQT	 	[sq][s] = piece == PAWNS 	? 1 : 0; 			
					T->knightPSQT	[sq][s] = piece == KNIGHTS 	? 1 : 0; 			
					T->bishopPSQT	[sq][s] = piece == BISHOPS 	? 1 : 0; 			
					T->rookPSQT		[sq][s] = piece == ROOKS 	? 1 : 0; 			
					T->queenPSQT	[sq][s] = piece == QUEEN 	? 1 : 0; 			
				}
			}
		}

		auto pawnsHashEntry = &th->pawnsHashTable[th->pawnsHashKey % U16_PAWN_HASH_TABLE_RECORDS];
   		pawnsHashHit = false;
	#else

		//@TODO check eval hash implementation
	    auto evalHashEntry = &th->evalHashTable[th->hashKey % U16_EVAL_HASH_TABLE_RECORDS];
	    if (evalHashEntry->key == th->hashKey)
	    {
	    	return side == WHITE ? evalHashEntry->score : -evalHashEntry->score;
	    }

        auto pawnsHashEntry = &th->pawnsHashTable[th->pawnsHashKey % U16_PAWN_HASH_TABLE_RECORDS];
	   	pawnsHashHit = pawnsHashEntry->key == th->pawnsHashKey;
	#endif


	th->evalInfo.clear();

	th->evalInfo.pawnsHashHit = pawnsHashHit;

	const auto whitePawns = th->whitePieceBB[PAWNS];
	const auto blackPawns = th->blackPieceBB[PAWNS];

	th->evalInfo.openFilesBB = pawnsHashHit ? pawnsHashEntry->openFilesBB 
											: openFiles(whitePawns, blackPawns); 

	th->evalInfo.halfOpenFilesBB[WHITE] = pawnsHashHit ? pawnsHashEntry->halfOpenFilesBB[WHITE] 
					 								   : halfOpenOrOpenFile(whitePawns) ^ th->evalInfo.openFilesBB;

	th->evalInfo.halfOpenFilesBB[BLACK] = pawnsHashHit ? pawnsHashEntry->halfOpenFilesBB[BLACK] 
					 								   : halfOpenOrOpenFile(blackPawns) ^ th->evalInfo.openFilesBB;

	th->evalInfo.allPawnAttacks[WHITE] = pawnsHashHit ? pawnsHashEntry->allPawnAttacks[WHITE] 
					 								  : wPawnWestAttacks(whitePawns) | wPawnEastAttacks(whitePawns);

	th->evalInfo.allPawnAttacks[BLACK] = pawnsHashHit ? pawnsHashEntry->allPawnAttacks[BLACK] 
					 								  : bPawnWestAttacks(blackPawns) | bPawnEastAttacks(blackPawns);


	th->evalInfo.attacks[WHITE] |= th->evalInfo.allPawnAttacks[WHITE];
	th->evalInfo.attacks[BLACK] |= th->evalInfo.allPawnAttacks[BLACK];


	initEvalInfo<WHITE>(th);
	initEvalInfo<BLACK>(th);


	int eval = 0;

	#if defined(__TUNE) || defined(__TEST_TUNER)

		th->evalInfo.passedPawns[WHITE] = wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
		th->evalInfo.passedPawns[BLACK] = bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS]);

		eval += evaluatePawns<WHITE>(th) - evaluatePawns<BLACK>(th);
	#else
	
	    if (!pawnsHashHit)
	    {
			th->evalInfo.passedPawns[WHITE] = wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
			th->evalInfo.passedPawns[BLACK] = bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS]);

	    	pawnsHashEntry->key = th->pawnsHashKey;
	    
			pawnsHashEntry->openFilesBB = th->evalInfo.openFilesBB;

			pawnsHashEntry->halfOpenFilesBB[WHITE] = th->evalInfo.halfOpenFilesBB[WHITE];
			pawnsHashEntry->halfOpenFilesBB[BLACK] = th->evalInfo.halfOpenFilesBB[BLACK]; 

			pawnsHashEntry->allPawnAttacks[WHITE] = th->evalInfo.allPawnAttacks[WHITE];
			pawnsHashEntry->allPawnAttacks[BLACK] = th->evalInfo.allPawnAttacks[BLACK];

			pawnsHashEntry->pawnsEval = evaluatePawns<WHITE>(th) - evaluatePawns<BLACK>(th);
	    }
	    else 
	    {
	    	// fetch the PawnKing eval from the Pawn Hash table required for evaluateKing() later
			th->evalInfo.pkEval[WHITE] = pawnsHashEntry->pkEval[WHITE];
			th->evalInfo.pkEval[BLACK] = pawnsHashEntry->pkEval[BLACK];	
	    }

     	eval += pawnsHashEntry->pawnsEval;
   
	#endif
	
	eval += evaluateKnights<WHITE>(th) 	- 	evaluateKnights<BLACK>(th);
	eval += evaluateBishops<WHITE>(th) 	-	evaluateBishops<BLACK>(th);
	eval += evaluateRooks<WHITE>(th) 	- 	evaluateRooks<BLACK>(th);
	eval += evaluateQueen<WHITE>(th) 	- 	evaluateQueen<BLACK>(th);

	// evaluation of other pieces other than king needs to be done first
	// because of values required for king safety calculation
	eval += evaluateKing<WHITE>(th)		- 	evaluateKing<BLACK>(th);
	

	// save Pawn and King related information to the Pawn Hash Table
	if (!pawnsHashHit)
	{
		pawnsHashEntry->pkEval[WHITE] = th->evalInfo.pkEval[WHITE];
		pawnsHashEntry->pkEval[BLACK] = th->evalInfo.pkEval[BLACK];
	}

	// Tapered evaluation 
	int phase =   4 * POPCOUNT(th->whitePieceBB[QUEEN]   | th->blackPieceBB[QUEEN]) 
				+ 2 * POPCOUNT(th->whitePieceBB[ROOKS]   | th->blackPieceBB[ROOKS])
      			+ 1 * POPCOUNT(th->whitePieceBB[BISHOPS] | th->blackPieceBB[BISHOPS])
      			+ 1 * POPCOUNT(th->whitePieceBB[KNIGHTS] | th->blackPieceBB[KNIGHTS]);


    int score = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;


    #if defined(__TUNE) || defined(__TEST_TUNER)

		T->eval = eval;
		T->phase = phase;
	#else

		evalHashEntry->key = th->hashKey;
		evalHashEntry->score = score;
	#endif

	return side == WHITE ? score : -score;
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

template <Side side>
int evaluatePawns(Thread *th) 
{
	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	int sq = -1, rank = -1;
	
	int score = 0;
	

	auto ourPawns = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	while (ourPawns) 
	{
		sq = GET_POSITION(ourPawns);
		POP_POSITION(ourPawns);

		// Pawn PSQT Score
		score += side ? PSQT[PAWNS][Mirror64[sq]] : PSQT[PAWNS][sq];
	}



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
	ourPawns = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	auto phalanxPawns = pawnsWithEastNeighbors(ourPawns) | pawnsWithWestNeighbors(ourPawns);
	while (phalanxPawns) 
	{
		sq = GET_POSITION(phalanxPawns);
		POP_POSITION(phalanxPawns);

		rank = side ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		// Check if the phalanx pawn is defended by another pawn
		if (th->evalInfo.allPawnAttacks[side] & (1ULL << sq)) 
		{	
			// Defended phalanx pawn
		
			score += arr_weight_defended_phalanx_pawn[rank];
			
			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->defendedPhalanxPawn[rank][side]++;
			#endif
		} 
		else 
		{	
			// Normal phalanx pawn

			score += arr_weight_phalanx_pawn[rank];

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->phalanxPawn[rank][side]++;
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
	auto p = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	// get the member (center) of at least triple chains
	// http://www.talkchess.com/forum3/viewtopic.php?t=55477
	auto b = defendedDefenders1(p) | defendedDefenders2(p);

	while (b) 
	{
		sq = GET_POSITION(b);
		POP_POSITION(b);
		
		rank = side ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		// add the score based on the rank of the member
		score += arr_weight_pawn_chain[rank];
		
		#if defined(__TUNE) || defined(__TEST_TUNER)	
		
			T->pawnChain[rank][side]++;
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

	score += POPCOUNT(isolatedPawns<side>(th)) * weight_isolated_pawn;

	#if defined(__TUNE) || defined(__TEST_TUNER)	
		
		T->isolatedPawns[side] = POPCOUNT(isolatedPawns<side>(th));
	#endif


	// Double pawns
	
	score += POPCOUNT(doublePawns<side>(th)) * weight_double_pawn;

	#if defined(__TUNE) || defined(__TEST_TUNER)	
		
		T->doublePawns[side] = POPCOUNT(doublePawns<side>(th));
	#endif


	// Backward pawns	
	/*
		A backward pawn is a pawn that is behind the pawns next to him and cannot move forward without being captured. 
		At the same time this pawn is on a semi-open file that makes it vulnerable to being attacked, particularly by rooks. 
		Backward pawns are often a significant weakness in your position.
	*/
	
	score += POPCOUNT(backwardPawns<side>(th)) * weight_backward_pawn;

	#if defined(__TUNE) || defined(__TEST_TUNER)	
		
		T->backwardPawns[side] = POPCOUNT(backwardPawns<side>(th));
	#endif


	// Pawn holes
	/*
		Weak squares generally refer to squares on the 5th or 6th rank (inside enemy territory) that cannot be defended by pawns. 
		A Square that cannot be defended by a pawn can more easily be occupied by a piece. 
		Therefore, weak squares are often an opportunity to further improve the development of your pieces.
		Naturally it should also be noted that, in most cases, 
		weak squares near or in the center are more useful than weak squares on the sides of the board
	*/
	
	score += POPCOUNT(pawnHoles<side>(th)) * weight_pawn_hole;

	#if defined(__TUNE) || defined(__TEST_TUNER)	
		
		T->pawnHoles[side] = POPCOUNT(pawnHoles<side>(th));
	#endif



	// Passed pawns
	/*
		A passed pawn refers to a pawn that cannot be stopped by enemy pawns from reaching the other side. 
		This often means your opponent will have to use a piece to stop the passed pawn. 
		This can give you an advantage since your opponent will have a piece that is tied down in a defensive task.
	*/

	auto stmPassedPawns = th->evalInfo.passedPawns[side];
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
		if (arrFiles[file - 1] & th->evalInfo.allRookAttacks[side] & stmPassedPawns) 
		{	
			score += weight_rook_behind_stm_passed_pawn;
			
			#if defined(__TUNE) || defined(__TEST_TUNER) 

				T->rookBehindStmPassedPawn[side]++;
			#endif
		}
		
		if (arrFiles[file - 1] & th->evalInfo.allRookAttacks[side] & oppPassedPawns) 
		{	
			score += weight_rook_behind_opp_passed_pawn;
			
			#if defined(__TUNE) || defined(__TEST_TUNER) 
			
				T->rookBehindOppPassedPawn[side]++;
			#endif
		}
	}

	while (stmPassedPawns) 
	{	
		sq = GET_POSITION(stmPassedPawns);
		POP_POSITION(stmPassedPawns);

		assert(sq >= 0 && sq <= 63);

		rank = side ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		assert(rank+1 >= 1 && rank+1 <= 8);
		
		// Check if the passed pawn is defended by another pawn
		if (th->evalInfo.allPawnAttacks[side] & (1ULL << sq)) {
			
			// Defended passed pawn
		
			score += arr_weight_defended_passed_pawn[rank];
			
			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->defendedPassedPawn[side][rank]++;
			#endif
		} 
		else 
		{	
			// Normal passed pawn

			score += arr_weight_passed_pawn[rank];

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->passedPawn[side][rank]++;
			#endif
		}	
	}

	return score;
}


template <Side side>
int evaluateKnights(Thread *th) 
{
	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];
	const auto allPawnsCount = POPCOUNT(th->blackPieceBB[PAWNS] | th->whitePieceBB[PAWNS]);
	const auto oppPawnHoles = ~th->evalInfo.allPawnAttacks[opp] & EXTENDED_CENTER;
	
	auto knightsBB = side ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];
	
	bool hasPawnShield;

	int mobilityCount = 0, sq = -1;

	int score = 0;
	
	U64 attacksBB, squareBB;
	
	while (knightsBB) 
	{
		sq = GET_POSITION(knightsBB);
		POP_POSITION(knightsBB);

		assert(sq >= 0 && sq <= 63);
			
		squareBB = (1ULL << sq);	

		// Knight PSQT Score
		score += side ? PSQT[KNIGHTS][Mirror64[sq]] : PSQT[KNIGHTS][sq];


		attacksBB = th->evalInfo.knightAttacks[side][sq];


		// Decreasing value as pawns disappear

		score += weight_knight_all_pawns_count * allPawnsCount;

		#if defined (__TUNE) || defined(__TEST_TUNER)

			T->knightAllPawnsCount[side] += allPawnsCount; 	
		#endif

		
		// 	Outpost
		//	An outpost is a square on the fourth, fifth, sixth, 
		//	or seventh rank which is protected by a pawn and which cannot be attacked by an opponent's pawn.
		if (oppPawnHoles & squareBB & th->evalInfo.allPawnAttacks[side]) 
		{
			score += weight_knight_outpost;

			#if defined(__TUNE) || defined(__TEST_TUNER) 

				T->knightOutpost[side]++;
			#endif
		}

		
		// Penalty for an undefended minor piece
		
		bool isDefended = squareBB & th->evalInfo.attacks[side]; 

		if (!isDefended) 
		{
			score += weight_undefended_knight;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->undefendedKnight[side]++;			
			#endif
		}

		
		// Marginal bonus for a knight defended by a pawn (TODO check weight and consider removing this)
		
		bool isDefendedByAPawn = squareBB & th->evalInfo.allPawnAttacks[side];
		
		if (isDefendedByAPawn) 
		{
			score += weight_knight_defended_by_pawn;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->knightDefendedByPawn[side]++;
			#endif
		}

		// Bonus if it is shielded by a pawn
		hasPawnShield = (side == WHITE) ? (squareBB << 8) & th->whitePieceBB[PAWNS]
						     			: (squareBB >> 8) & th->blackPieceBB[PAWNS];
		if (hasPawnShield)
		{
			score += weight_minor_has_pawn_shield;
				
			#if defined(__TUNE) || defined(__TEST_TUNER) 

				T->minorPawnShield[side]++;
			#endif
		}

		// Knight mobility
		
		U64 mobilityBB = attacksBB & th->empty & ~th->evalInfo.allPawnAttacks[opp];

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_knight_mobility[mobilityCount];

		#if defined(__TUNE) || defined(__TEST_TUNER)

			T->knightMobility[side][mobilityCount]++;
		#endif


		// Update values required for King safety 
		
		if (attacksBB & kingZoneBB[opp][kingSq]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_knight_attack;

            #if defined(__TUNE) || defined(__TEST_TUNER)	
            
            	T->knightAttack[opp]++; 
            #endif
		}
	}

	return score;
}

template <Side side>
int evaluateBishops(Thread *th) 
{
	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];

	bool isDefended, hasPawnShield;

	int mobilityCount = 0, sq = -1;
	
	int score = 0;
	
	auto bishopBB = side ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];

	U64 attacksBB, squareBB;
	
	// Bishop pair
	if (POPCOUNT(bishopBB) == 2) 
	{	
		score += weight_bishop_pair;

		#if defined(__TUNE) || defined(__TEST_TUNER)	
			
			T->bishopPair[side] = 1;
		#endif
	}

	while (bishopBB) 
	{
		sq = GET_POSITION(bishopBB);
		POP_POSITION(bishopBB);

		assert(sq >= 0 && sq <= 63);
		
		squareBB = (1ULL << sq);	

		attacksBB = th->evalInfo.bishopAttacks[side][sq];		


		// Bishop PSQT Score
		score += side ? PSQT[BISHOPS][Mirror64[sq]] : PSQT[BISHOPS][sq];


		// Penalty for an undefended minor piece
		
		isDefended = squareBB & th->evalInfo.attacks[side]; 

		if (!isDefended) 
		{
			score += weight_undefended_bishop;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->undefendedBishop[side]++;			
			#endif
		}


		// Bonus if it is shielded by a pawn
		hasPawnShield = (side == WHITE) ? (squareBB << 8) & th->whitePieceBB[PAWNS]
						     			: (squareBB >> 8) & th->blackPieceBB[PAWNS];
		if (hasPawnShield)
		{
			score += weight_minor_has_pawn_shield;
				
			#if defined(__TUNE) || defined(__TEST_TUNER) 

				T->minorPawnShield[side]++;
			#endif
		}


		// Bishop mobility

		mobilityCount = POPCOUNT(attacksBB & th->empty);

		score += arr_weight_bishop_mobility[mobilityCount];

		#if defined(__TUNE) || defined(__TEST_TUNER) 

			T->bishopMobility[side][mobilityCount]++;
		#endif



		// Update values required for King safety 

		if (attacksBB & kingZoneBB[opp][kingSq]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_bishop_attack;

            #if defined(__TUNE) || defined(__TEST_TUNER)	
            
            	T->bishopAttack[opp]++; 
            #endif
		}
	}
	
	return score;
}


template <Side side>
int evaluateRooks(Thread *th) 
{	
	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];

	const auto oppFlankPawnHoles = ~th->evalInfo.allPawnAttacks[opp] 
								& ~EXTENDED_CENTER & ~(RANK_1 | RANK_2 | RANK_7 | RANK_8);
	
	auto rooksBB = side ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];
	
	U64 attacksBB, squareBB;

	int score = 0;
	int sq = -1, mobilityCount = 0;

	while (rooksBB) 
	{
		sq = GET_POSITION(rooksBB);
		POP_POSITION(rooksBB);

		assert(sq >= 0 && sq <= 63);

		squareBB = (1ULL << sq);	

		attacksBB = th->evalInfo.rookAttacks[side][sq];		


		// Rook PSQT Score
		score += side ? PSQT[ROOKS][Mirror64[sq]] : PSQT[ROOKS][sq];



		// Nimzowitsch argued when the outpost is in one of the flank (a-, b-, g- and h-) files 
		// the ideal piece to make use of the outpost is a rook. 
		// This is because the rook can put pressure on all the squares along the rank
		
		if (oppFlankPawnHoles & squareBB & th->evalInfo.allPawnAttacks[side]) 
		{
			score += weight_rook_flank_outpost;
			
			#if defined(__TUNE) || defined(__TEST_TUNER)

				T->rookFlankOutpost[side]++;
			#endif
		}


		// Rook on half open file
		
		if (squareBB & th->evalInfo.halfOpenFilesBB[side]) 
		{
			score += weight_rook_half_open_file;
		
			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->halfOpenFile[side]++;
			#endif
		}

		// Rook on open file
		
		if (squareBB & th->evalInfo.openFilesBB) 
		{
			score += weight_rook_open_file;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->openFile[side]++;
			#endif
		}

		// Rook on same file as enemy Queen
		
		if (	arrFiles[sq % 8] 
			&	(opp ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) 
		{
			score += weight_rook_enemy_queen_same_file;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
			
				T->rookEnemyQueenSameFile[side]++;
			#endif
		}

		// Rook on Seventh rank
		
		if (squareBB & (side ? RANK_2 : RANK_7)) 
		{
			score += weight_rook_on_seventh_rank;
		
			#if defined(__TUNE) || defined(__TEST_TUNER)	
				
				T->rookOnSeventhRank[side]++;
			#endif
		}

		// Rook on Eight rank
		
		if (squareBB & (side ? RANK_1 : RANK_8)) 
		{
			score += weight_rook_on_eight_rank;

			#if defined(__TUNE) || defined(__TEST_TUNER)	

				T->rookOnEightRank[side]++;							
			#endif
		} 	


		// Connected Rooks

		if (attacksBB & rooksBB) 
		{
			score += weight_rook_supporting_friendly_rook;

			#if defined(__TUNE) || defined(__TEST_TUNER)	
				
				T->rookSupportingFriendlyRook[side] = 1;
			#endif
		}


		// Rook mobility
		
		mobilityCount = POPCOUNT(attacksBB & th->empty);

		score += arr_weight_rook_mobility[mobilityCount]; 

		#if defined(__TUNE) || defined(__TEST_TUNER)	
			
			T->rookMobility[side][mobilityCount]++; 
		#endif


		// Update values required for King safety later in evaluateKing
		
		if (attacksBB & kingZoneBB[opp][kingSq]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_rook_attack;

            #if defined(__TUNE) || defined(__TEST_TUNER)	
            
            	T->rookAttack[opp]++; 
            #endif
		}
	}

	return score;
}


template <Side side>
int evaluateQueen(Thread *th) 
{
	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];
	auto queenBB = side ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];
	
	int score = 0;
	int sq = -1;

	U64 attacksBB;

	while (queenBB) 
	{
		sq = GET_POSITION(queenBB);
		POP_POSITION(queenBB);

		assert(sq >= 0 && sq <= 63);

		attacksBB = th->evalInfo.queenAttacks[side][sq];


		// Queen PSQT Score
		score += side ? PSQT[QUEEN][Mirror64[sq]] : PSQT[QUEEN][sq];


		if (	POPCOUNT(th->occupied) >= 22	// early game
			&&	(side ? sq <= 48 : sq >= 15)) 
		{ 
			const auto minorPiecesBB = side ? 
									th->blackPieceBB[KNIGHTS] | th->blackPieceBB[BISHOPS] :
									th->whitePieceBB[KNIGHTS] | th->whitePieceBB[BISHOPS];

			const auto underdevelopedPiecesBB = (side ? RANK_8 : RANK_1) & minorPiecesBB; 

			score += POPCOUNT(underdevelopedPiecesBB) * weight_queen_underdeveloped_pieces;
			
			#if defined(__TUNE) || defined(__TEST_TUNER)	
	
				T->queenUnderdevelopedPieces[side] += POPCOUNT(underdevelopedPiecesBB);
			#endif
		}


		// Queen mobility
		
		score += arr_weight_queen_mobility[POPCOUNT(attacksBB & th->empty)];

		#if defined(__TUNE) || defined(__TEST_TUNER)	
	
			T->queenMobility[side][POPCOUNT(attacksBB & th->empty)]++;
		#endif


		// Update values required for King safety later in evaluateKing
		if (attacksBB & kingZoneBB[opp][kingSq]) 
		{	
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_queen_attack;

            #if defined(__TUNE) || defined(__TEST_TUNER)	
    
            	T->queenAttack[opp]++; 
            #endif
		}
	}	

	return score;
}



template<Side side>
int evaluatePawnAndKing(Thread *th) 
{
	constexpr Side opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];
	const auto kingRank = kingSq / 8;
	const auto kingFile = kingSq % 8;

	bool isBlocked;
	int score = 0, pawnShieldRank, pawnStormRank, fileDistance;
	
	int middleFile = __max(1, __min(6, kingFile));

	U64 pawns;
    U64 friendlyPawns = (side == WHITE ? th->whitePieceBB[PAWNS] : th->blackPieceBB[PAWNS]) 
    					& ~th->evalInfo.allPawnAttacks[opp];
    U64 opponentPawns = opp == WHITE ? th->whitePieceBB[PAWNS] : th->blackPieceBB[PAWNS];
    
    // loop through files before and after the king file
	for (int file = middleFile - 1; file <= middleFile + 1; file++)
    {   
    	//*********KING PAWN SHEILD**********// 

    	// get all friendly pawns in front of the king for the file
        pawns = friendlyPawns & arrFiles[file] & forwardRanksBB[side][kingRank];

        // get the rank of the nearest friendly pawn w.r.t side for the file
        pawnShieldRank = pawns ? relativeRank(side == WHITE ? lsb(pawns) : msb(pawns), side) : 0;
		

    	//*********OPPONENT PAWN STORM**********//
	
		// get all opponent pawns in front of the king for the file
        pawns = opponentPawns & arrFiles[file] & forwardRanksBB[side][kingRank];

		// get the rank of the nearest opponent pawn w.r.t side for the file
        pawnStormRank = pawns ? relativeRank(side == WHITE ? lsb(pawns) : msb(pawns), side) : 0;



    	fileDistance = __min(file, 7 - file);


        // add the pawn shield score for this file and rank
        score += MakeScore(weight_pawn_shield[fileDistance][pawnShieldRank], 0);

        // update values for tuning
        #if defined(__TUNE) || defined(__TEST_TUNER)

			T->pawnShield[fileDistance][pawnShieldRank][side]++;
		#endif


		// check if opponent pawn is blocked by a friendly pawn
        isBlocked = (pawnShieldRank != 0) && pawnShieldRank == (pawnStormRank - 1);
        

        // add the pawn storm score for this file and rank
        // add different values for blocked opponent pawn and unblocked opponent pawn
        score += isBlocked	? weight_blocked_pawn_storm[pawnStormRank] 
        					: MakeScore(weight_unblocked_pawn_storm[fileDistance][pawnStormRank], 0);
        
        // update values for tuning
  		#if defined(__TUNE) || defined(__TEST_TUNER)
	    
	        if (isBlocked)
	        {
				T->blockedStorm[pawnStormRank][side]++;
	        }
	        else
	        {
				T->unblockedStorm[fileDistance][pawnStormRank][side]++;
	        }
        #endif
    }

	return score;
}
	

template <Side side>
int evaluateKing(Thread *th) 
{	
	int score = 0;

	constexpr auto opp = side == WHITE ? BLACK : WHITE;

	const auto kingSq = th->evalInfo.kingSq[side];
	
	assert(kingSq >= 0 && kingSq < 64);
	
	// Piece Square Table Score for King	
	score += side ? kingPSQT[Mirror64[kingSq]] : kingPSQT[kingSq];
	
	if (!th->evalInfo.pawnsHashHit)
	{
		th->evalInfo.pkEval[side] = evaluatePawnAndKing<side>(th);
	}

	// Pawn King Score for Pawn Shield and Opponent Pawns Storm
	score += th->evalInfo.pkEval[side];

	// King Safety Score for Opponent attacks and checks
	if (	th->evalInfo.kingAttackersCount[side] > 
			(1 - POPCOUNT(opp == WHITE ? th->whitePieceBB[QUEEN] : th->blackPieceBB[QUEEN])))
	{ 
		int safetyScore 	= 	th->evalInfo.kingAttackersWeight[side];

		U64 enemyPieces 	= 	opp == WHITE ? th->whitePieceBB[PIECES] : th->blackPieceBB[PIECES];						
		U64 safeSquares 	= 	~(enemyPieces | th->evalInfo.attacks[side]);
		U64 unsafeSquares   = 	~safeSquares;
		U64 rookSquares 	= 	Rmagic(kingSq, th->occupied);
		U64 bishopSquares 	= 	Bmagic(kingSq, th->occupied);
		U64 knightSquares   =	get_knight_attacks(kingSq);

		U64 queenChecks 	=	(rookSquares | bishopSquares) & th->evalInfo.allQueenAttacks[opp];
		U64 rookChecks 		= 	rookSquares & th->evalInfo.allRookAttacks[opp];
		U64 bishopChecks 	= 	bishopSquares & th->evalInfo.allBishopAttacks[opp];
		U64 knightChecks 	= 	knightSquares & th->evalInfo.allKnightAttacks[opp];

		if (queenChecks)
		{
			safetyScore += 	weight_unsafe_queen_check 	* 	POPCOUNT(queenChecks & unsafeSquares);
			safetyScore += 	weight_safe_queen_check 	* 	POPCOUNT(queenChecks & safeSquares);
			
			#if defined(__TUNE) || defined(__TEST_TUNER)

				T->unsafeQueenCheck[side] 	= 	POPCOUNT(queenChecks & unsafeSquares);
				T->safeQueenCheck[side] 	= 	POPCOUNT(queenChecks & safeSquares);
			#endif
        }
		
		if (rookChecks)
		{
			safetyScore += 	weight_unsafe_rook_check 	* 	POPCOUNT(rookChecks & unsafeSquares);
			safetyScore += 	weight_safe_rook_check 		* 	POPCOUNT(rookChecks & safeSquares);
			
			#if defined(__TUNE) || defined(__TEST_TUNER)

				T->unsafeRookCheck[side] 	= 	POPCOUNT(rookChecks & unsafeSquares);
				T->safeRookCheck[side] 		= 	POPCOUNT(rookChecks & safeSquares);
			#endif
        }

		if (bishopChecks)
		{
			safetyScore += 	weight_unsafe_bishop_check 	* 	POPCOUNT(bishopChecks & unsafeSquares);
			safetyScore += 	weight_safe_bishop_check 	* 	POPCOUNT(bishopChecks & safeSquares);
			
			#if defined(__TUNE) || defined(__TEST_TUNER)

				T->unsafeBishopCheck[side] 	= 	POPCOUNT(bishopChecks & unsafeSquares);
				T->safeBishopCheck[side] 	= 	POPCOUNT(bishopChecks & safeSquares);
			#endif
        }

		if (knightChecks)
		{
			safetyScore += 	weight_unsafe_knight_check 	* 	POPCOUNT(knightChecks & unsafeSquares);
			safetyScore += 	weight_safe_knight_check 	* 	POPCOUNT(knightChecks & safeSquares);
			
			#if defined(__TUNE) || defined(__TEST_TUNER)

				T->unsafeKnightCheck[side] 	= 	POPCOUNT(knightChecks & unsafeSquares);
				T->safeKnightCheck[side] 	= 	POPCOUNT(knightChecks & safeSquares);
			#endif
        }

		safetyScore += 	weight_safety_adjustment;

		#if defined(__TUNE) || defined(__TEST_TUNER)

			T->safetyAdjustment[side]	=	1;
	    	T->safety[side] 			= 	safetyScore;
		#endif

	
 		const auto mg = ScoreMG(safetyScore), eg = ScoreEG(safetyScore);

        score += MakeScore(-mg * __max(0, mg) / 720, -__max(0, eg) / 20); 
	} 
    else 
    {
    	#if defined(__TUNE) || defined(__TEST_TUNER)
	    	
	    	T->knightAttack[side]	= 	0;
			T->bishopAttack[side] 	=	0;
			T->rookAttack[side] 	=	0;
			T->queenAttack[side]	=	0;
    	#endif
    } 

	return score;
}


void initPSQT() 
{
	for (U8 sq = 0; sq < U8_MAX_SQUARES; sq++) 
	{
		PSQT[PAWNS][sq] 	= 	pawnPSQT[sq] 	+ 	weight_val_pawn; 
		PSQT[KNIGHTS][sq] 	= 	knightPSQT[sq] 	+	weight_val_knight; 
		PSQT[BISHOPS][sq] 	= 	bishopPSQT[sq]	+ 	weight_val_bishop; 
		PSQT[ROOKS][sq] 	= 	rookPSQT[sq] 	+	weight_val_rook; 
		PSQT[QUEEN][sq] 	= 	queenPSQT[sq]	+ 	weight_val_queen; 
	}
}

void initKingZoneBB()
{
	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) 
	{
	    kingZoneBB[WHITE][sq] = get_king_attacks(sq) | (1ULL << sq) | (get_king_attacks(sq) << 8);
	    kingZoneBB[BLACK][sq] = get_king_attacks(sq) | (1ULL << sq) | (get_king_attacks(sq) >> 8);

	    kingZoneBB[WHITE][sq] |= ((sq % 8) != 0) ? 0ULL : kingZoneBB[WHITE][sq] << 1;
	    kingZoneBB[BLACK][sq] |= ((sq % 8) != 0) ? 0ULL : kingZoneBB[BLACK][sq] << 1;

	    kingZoneBB[WHITE][sq] |= ((sq % 8) != 7) ? 0ULL : kingZoneBB[WHITE][sq] >> 1;
	    kingZoneBB[BLACK][sq] |= ((sq % 8) != 7) ? 0ULL : kingZoneBB[BLACK][sq] >> 1;
	}	
}

void initForwardRanksBB() 
{
	for (int rank = 0; rank < 8; rank++) 
	{
	    for (int i = rank; i < 8; i++)
	    {
	        forwardRanksBB[WHITE][rank] |= arrRanks[i];	
	    }

	    forwardRanksBB[BLACK][rank] = ~forwardRanksBB[WHITE][rank] | arrRanks[rank];
	}	
}













