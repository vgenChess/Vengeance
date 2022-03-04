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

#include "globals.h"
#include "evaluate.h"
#include "utility.h"
#include "movegen.h"
#include "make_unmake.h"
#include "hash.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "tuner.h"
#include "weights.h"
#include "functions.h"

TraceCoefficients *T;

int Mirror64[64] = {

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


int WHITE_PSQT[U8_MAX_PIECES][U8_MAX_SQUARES];
int BLACK_PSQT[U8_MAX_PIECES][U8_MAX_SQUARES];


void initEvalInfo(Thread *th) {

	th->evalInfo.clear();


	th->evalInfo.openFilesBB = openFiles(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);


	th->evalInfo.halfOpenFilesBB[WHITE] 
		= halfOpenOrOpenFile(th->whitePieceBB[PAWNS]) ^ th->evalInfo.openFilesBB;	
	th->evalInfo.halfOpenFilesBB[BLACK] 
		= halfOpenOrOpenFile(th->blackPieceBB[PAWNS]) ^ th->evalInfo.openFilesBB;


	th->evalInfo.allPawnAttacks[WHITE] = 
		wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);
	th->evalInfo.allPawnAttacks[BLACK] = 
		bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
	
	th->evalInfo.attacks[WHITE] |= th->evalInfo.allPawnAttacks[WHITE];
	th->evalInfo.attacks[BLACK] |= th->evalInfo.allPawnAttacks[BLACK];
	
	int sq = -1;
	U64 bb = 0ULL;
	for (int p = KNIGHTS; p <= KING; p++) {

		for (U8 stm = WHITE; stm <= BLACK; stm++) {

			bb = stm ? th->blackPieceBB[p] : th->whitePieceBB[p];
			
			while (bb) {

				sq = GET_POSITION(bb);
				POP_POSITION(bb);

				assert(sq >= 0 && sq <= 63);

				if (p == KNIGHTS) {

					th->evalInfo.knightAttacks[stm][sq] = get_knight_attacks(sq);
					th->evalInfo.allKnightAttacks[stm] |= th->evalInfo.knightAttacks[stm][sq];
					th->evalInfo.attacks[stm] |= th->evalInfo.knightAttacks[stm][sq];		
				}	
				else if (p == BISHOPS) {

					th->evalInfo.bishopAttacks[stm][sq] = Bmagic(sq, th->occupied);
					th->evalInfo.allBishopAttacks[stm] |= th->evalInfo.bishopAttacks[stm][sq];		
					th->evalInfo.attacks[stm] |= th->evalInfo.bishopAttacks[stm][sq];		
				}
				else if (p == ROOKS) {
					
					th->evalInfo.rookAttacks[stm][sq] = Rmagic(sq, th->occupied);
					th->evalInfo.allRookAttacks[stm] |= th->evalInfo.rookAttacks[stm][sq];						
					th->evalInfo.attacks[stm] |= th->evalInfo.rookAttacks[stm][sq];				
				}	
				else if (p == QUEEN) {

					th->evalInfo.queenAttacks[stm][sq] = Qmagic(sq, th->occupied);
					th->evalInfo.allQueenAttacks[stm] |= th->evalInfo.queenAttacks[stm][sq];
					th->evalInfo.attacks[stm] |= th->evalInfo.queenAttacks[stm][sq];		
				}
				else if (p == KING)	{
					
					th->evalInfo.kingAttacks[stm] = get_king_attacks(sq);			
					th->evalInfo.attacks[stm] |= th->evalInfo.kingAttacks[stm];
				}	
			}	
		}
	}


	th->evalInfo.kingSq[WHITE] = GET_POSITION(th->whitePieceBB[KING]);	
	th->evalInfo.kingSq[BLACK] = GET_POSITION(th->blackPieceBB[KING]);	

	
	th->evalInfo.kingZoneBB[WHITE] = th->evalInfo.kingAttacks[WHITE] | (th->evalInfo.kingAttacks[WHITE] >> 8);
	th->evalInfo.kingZoneBB[BLACK] = th->evalInfo.kingAttacks[BLACK] | (th->evalInfo.kingAttacks[BLACK] << 8);


	th->evalInfo.kingAttackersCount[WHITE] = 0;
    th->evalInfo.kingAttackersCount[BLACK] = 0;
    
    th->evalInfo.kingAttackersWeight[WHITE] = 0;
	th->evalInfo.kingAttackersWeight[BLACK] = 0;

  	th->evalInfo.kingAdjacentZoneAttacksCount[WHITE] = 0;
  	th->evalInfo.kingAdjacentZoneAttacksCount[BLACK] = 0;	
}

int traceFullEval(TraceCoefficients *traceCoefficients, U8 stm, Thread *th) {

	T = traceCoefficients;

	return fullEval(stm, th);
}

int fullEval(U8 stm, Thread *th) {
	
	#if defined(TUNE)
	
		U8 sq;
		U64 bb;
		for (U8 side = WHITE; side <= BLACK; side++) {

			for (U8 piece = PAWNS; piece <= KING; piece++) {
				
				bb = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];

				while (bb) {

					sq = GET_POSITION(bb);
					POP_POSITION(bb);

					T->weight_val_pawn[side] 	+= 	piece == PAWNS 		? 	1 	: 	0;
					T->weight_val_knight[side] 	+= 	piece == KNIGHTS 	? 	1 	: 	0;
					T->weight_val_bishop[side] 	+= 	piece == BISHOPS 	? 	1 	: 	0;
					T->weight_val_rook[side] 	+= 	piece == ROOKS 		? 	1 	: 	0;
					T->weight_val_queen[side] 	+= 	piece == QUEEN 		? 	1 	:	0;
					
					T->blackPawnPSQT[sq] 		=	piece == PAWNS 		?	(side ? 1 : 0) 	: 	0; 			
					T->blackKnightPSQT[sq] 		=	piece == KNIGHTS 	?	(side ? 1 : 0) 	: 	0;
					T->blackBishopPSQT[sq] 		=	piece == BISHOPS 	?	(side ? 1 : 0) 	: 	0;
					T->blackRookPSQT[sq] 		= 	piece == ROOKS 		?	(side ? 1 : 0) 	: 	0; 	
					T->blackQueenPSQT[sq] 		= 	piece == QUEEN 		? 	(side ? 1 : 0) 	: 	0;
					T->blackKingPSQT[sq] 		= 	piece == KING 		?	(side ? 1 : 0) 	: 	0; 	 			
		
					T->whitePawnPSQT[sq] 		= 	piece == PAWNS 		? 	(side ? 0 : 1) 	: 	0;
					T->whiteKnightPSQT[sq] 		= 	piece == KNIGHTS 	? 	(side ? 0 : 1) 	: 	0; 			
					T->whiteBishopPSQT[sq] 		= 	piece == BISHOPS 	?	(side ? 0 : 1) 	: 	0;		
					T->whiteRookPSQT[sq] 		= 	piece == ROOKS 		? 	(side ? 0 : 1) 	: 	0; 			
					T->whiteQueenPSQT[sq] 		= 	piece == QUEEN 		? 	(side ? 0 : 1) 	: 	0;		
					T->whiteKingPSQT[sq] 		= 	piece == KING 		? 	(side ? 0 : 1) 	: 	0;					
				}
			}
		}

	#else
	
		int hashedEval;
		if (probeEval(&hashedEval, th)) {

			return stm == WHITE ? hashedEval : -hashedEval;
		}

	#endif


	initEvalInfo(th);


	int eval = 0;

	// TODO Pawn eval cache
/*
	#if defined(TUNE)

		eval += pawnsEval(WHITE, th) - pawnsEval(BLACK, th);
	#else
	
		int pawnsScore;
		if (!probePawnHash(&pawnsScore, th)) { // TODO check pawn hash logic

			pawnsScore = pawnsEval(stm, th);
			recordPawnHash(pawnsScore, th);
		}

		score += pawnsScore;
	#endif
*/

	eval += 	pawnsEval(WHITE, th) 	- 	pawnsEval(BLACK, th);
	eval += 	knightsEval(WHITE, th) 	- 	knightsEval(BLACK, th);
	eval += 	bishopsEval(WHITE, th) 	-	bishopsEval(BLACK, th);
	eval += 	rooksEval(WHITE, th) 	- 	rooksEval(BLACK, th);
	eval += 	queenEval(WHITE, th) 	- 	queenEval(BLACK, th);

	// evaluation of other pieces other than king needs to be done first
	// before king eval because of values needed for king safety calculation
	eval += 	kingEval(WHITE, th) 	- 	kingEval(BLACK, th);
	eval += 	evalBoard(WHITE, th) 	- 	evalBoard(BLACK, th);

	eval += 	th->material;


	// Tapered evaluation 
	int phase = 4 * POPCOUNT(th->whitePieceBB[QUEEN] | th->blackPieceBB[QUEEN]) 
			+ 	2 * POPCOUNT(th->whitePieceBB[ROOKS] | th->blackPieceBB[ROOKS])
      		+ 	1 * POPCOUNT(th->whitePieceBB[BISHOPS] | th->blackPieceBB[BISHOPS])
      		+ 	1 * POPCOUNT(th->whitePieceBB[KNIGHTS] | th->blackPieceBB[KNIGHTS]);

    int score = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;

    #if defined(TUNE)	

		T->eval = eval;
		T->phase = phase;
	#else

		recordEval(score, th);
	#endif

	return stm == WHITE ? score : -score;
}

int pawnsEval(U8 stm, Thread *th) {

	int score = 0;
	int sq = -1, rank = -1;

	const int nIsolatedPawns = isolatedPawns(stm, th);
	score += nIsolatedPawns * weight_isolated_pawn;

	#if defined(TUNE)	
		
		T->isolatedPawns[stm] = nIsolatedPawns;
	#endif

	const int nDoublePawns = numOfDoublePawns(stm, th);
	score += nDoublePawns * weight_double_pawn;

	#if defined(TUNE)	
		
		T->doublePawns[stm] = nDoublePawns;
	#endif

	const int nBackwardPawns = countBackWardPawns(stm, th);
	score += nBackwardPawns * weight_backward_pawn;

	#if defined(TUNE)	
		
		T->backwardPawns[stm] = nBackwardPawns;
	#endif

	U64 defendedPawns = stm ? 
		th->blackPieceBB[PAWNS] & th->evalInfo.allPawnAttacks[BLACK]
		: th->whitePieceBB[PAWNS] & th->evalInfo.allPawnAttacks[WHITE];

	int nDefendedPawns = POPCOUNT(defendedPawns);
	score += nDefendedPawns * weight_defended_pawn;

	#if defined(TUNE)	
		
		T->defendedPawns[stm] = nDefendedPawns;
	#endif

	const int nPawnHoles = numOfPawnHoles(stm, th);
	score += nPawnHoles * weight_pawn_hole;

	#if defined(TUNE)	
		
		T->pawnHoles[stm] = nPawnHoles;
	#endif

	U64 passedPawns = stm ? bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])
		: wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
	while (passedPawns) {
		
		sq = GET_POSITION(passedPawns);
		POP_POSITION(passedPawns);

		assert(sq >= 0 && sq <= 63);

		rank = stm ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		assert(rank+1 >= 1 && rank+1 <= 8);
	

		if ((1ULL << sq) & th->evalInfo.allPawnAttacks[stm]) {

			score += arr_weight_defended_passed_pawn[rank];
			
			#if defined(TUNE)	
			
				T->defendedPassedPawn[stm][rank]++;
			#endif
		} else {
			
			score += arr_weight_passed_pawn[rank];

			#if defined(TUNE)	
			
				T->passedPawn[stm][rank]++;
			#endif
		}		
	}

	return score;
}



int knightsEval(U8 stm, Thread *th) {

	U8 opp = stm ^ 1;

	int score = 0;
	int mobilityCount = 0;

	int sq = -1; 

	U64 knightsBB = stm ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];

	while (knightsBB) {

		sq = GET_POSITION(knightsBB);
		POP_POSITION(knightsBB);

		assert(sq >= 0 && sq <= 63);

		U64 attacksBB = th->evalInfo.knightAttacks[stm][sq];

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_knight_attack;

            #if defined(TUNE)	
            
            	T->knightAttack[opp]++; 
            #endif

            U64 bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb) {

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}

		// undefended
		if (!((1ULL << sq) & th->evalInfo.attacks[stm])) {

			score += weight_undefended_knight;

			#if defined(TUNE)	
			
				T->undefendedKnight[stm]++;			
			#endif
		}

		// defended by pawn
		if ((1ULL << sq) & th->evalInfo.allPawnAttacks[stm]) {

			score += weight_knight_defended_by_pawn;

			#if defined(TUNE)	
			
				T->knightDefendedByPawn[stm]++;
			#endif
		}

		//TODO check logic
		U64 mobilityBB = attacksBB & th->empty & ~th->evalInfo.allPawnAttacks[opp];

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_knight_mobility[mobilityCount];

		#if defined(TUNE)

			T->knightMobility[stm][mobilityCount]++;
		#endif
	}

	return score;
}


int bishopsEval(U8 stm, Thread *th) {

	U8 opp = stm ^ 1;

	int score = 0;
	int mobilityCount = 0;
	int sq = -1;

	U64 bishopBB = stm ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
	
	// Bishop pair
	if (POPCOUNT(bishopBB) == 2) {	

		score += weight_bishop_pair;

		#if defined(TUNE)	
			
			T->bishopPair[stm] = 1;
		#endif
	}

	while (bishopBB) {
	
		sq = GET_POSITION(bishopBB);
		POP_POSITION(bishopBB);

		assert(sq >= 0 && sq <= 63);
		
		U64 attacksBB = th->evalInfo.bishopAttacks[stm][sq];


		// Bishop mobility
		U64 mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_bishop_mobility[mobilityCount];

		#if defined(TUNE) 

			T->bishopMobility[stm][mobilityCount]++;
		#endif


		// Update values for opp King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_bishop_attack;

            #if defined(TUNE)	
            
            	T->bishopAttack[opp]++; 
            #endif
            	
            U64 bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}
	
	return score;
}


/** TODO
Increasing value as pawns disappear 
Tarrasch Rule
*/
int rooksEval(U8 stm, Thread *th) {
	
	const U8 opp = stm ^ 1;

	int score = 0;
	int sq = -1;

	int mobilityCount = 0;

	const int kingSq = th->evalInfo.kingSq[stm];

	U64 rooksBB = stm ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];

	while (rooksBB) {

		sq = GET_POSITION(rooksBB);
		POP_POSITION(rooksBB);

		assert(sq >= 0 && sq <= 63);


		// Rook on half open file
		if ((1ULL << sq) & th->evalInfo.halfOpenFilesBB[stm]) {

			score += weight_rook_half_open_file;
		
			#if defined(TUNE)	
			
				T->halfOpenFile[stm]++;
			#endif
		}

		// Rook on open file
		if ((1ULL << sq) & th->evalInfo.openFilesBB) {

			score += weight_rook_open_file;

			#if defined(TUNE)	
			
				T->openFile[stm]++;
			#endif
		}

		// Rook on same file as enemy Queen
		if (arrFiles[sq & 7] & 
			(opp ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

			score += weight_rook_enemy_queen_same_file;

			#if defined(TUNE)	
			
				T->rookEnemyQueenSameFile[stm]++;
			#endif
		}

		// Rook on Seventh rank
		if ((1ULL << sq) & (stm ? RANK_2 : RANK_7)) {

			score += weight_rook_on_seventh_rank;
		
			#if defined(TUNE)	
				
				T->rookOnSeventhRank[stm]++;
			#endif
		}

		// Rook on Eight rank
		if ((1ULL << sq) & (stm ? RANK_1 : RANK_8)) {

			score += weight_rook_on_eight_rank;

			#if defined(TUNE)	

				T->rookOnEightRank[stm]++;							
			#endif
		} 	


		U64 attacksBB = th->evalInfo.rookAttacks[stm][sq];


		// Connected Rooks
		if (attacksBB & rooksBB) {

			score += weight_rook_supporting_friendly_rook;

			#if defined(TUNE)	
				
				T->rookSupportingFriendlyRook[stm] = 1;
			#endif
		}


		// Rook mobility
		U64 mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_rook_mobility[mobilityCount]; 

		#if defined(TUNE)	
			
			T->rookMobility[stm][mobilityCount]++; 
		#endif


		// Update values for opp King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_rook_attack;

            #if defined(TUNE)	
            
            	T->rookAttack[opp]++; 
            #endif

            U64 bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb)	{
            	
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}

		/*if (stm) {

			// for black

			// TODO king can be in other squares too and still block the rook
			// Penalty for a Rook blocked by an uncastled King
			// if (kingSq == 60 && sq >= 56 && sq < 64) {
	
			// 	score += weight_rook_blocked_by_king;

			// 	if (TUNE) {

			// 		T->rookBlockedByKing[BLACK]++;
			// 	}
			// }
		} else {
			
			// for white 	


			// Penalty for a Rook blocked by an uncastled King
			// if (kingSq == 4 && (sq >= 0 && sq < 8)) {

			// 	score += weight_rook_blocked_by_king;

			// 	if (TUNE) {

			// 		T->rookBlockedByKing[WHITE]++;
			// 	}
			// }
		}*/
	}

	return score;
}



int queenEval(U8 stm, Thread *th) {

	U8 opp = stm ^ 1;

	int score = 0;

	int sq = -1;
	int mobilityCount = 0;

	U64 attacksBB = 0ULL, mobilityBB = 0ULL; 	
	U64 queenBB = stm ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];

	while (queenBB) {

		sq = GET_POSITION(queenBB);
		POP_POSITION(queenBB);

		assert(sq >= 0 && sq <= 63);


		attacksBB = th->evalInfo.queenAttacks[stm][sq];
			
	
		if (	POPCOUNT(th->occupied) >= 22	// early game
			&&	(stm ? sq <= 48 : sq >= 15)) { 

			U64 minorPiecesBB = stm ? 
				th->blackPieceBB[KNIGHTS] | th->blackPieceBB[BISHOPS] :
				th->whitePieceBB[KNIGHTS] | th->whitePieceBB[BISHOPS];

			U64 underdevelopedPiecesBB = (stm ? RANK_8 : RANK_1) & minorPiecesBB; 

			int count = POPCOUNT(underdevelopedPiecesBB);
			
			score += count * weight_queen_underdeveloped_pieces;
			
			#if defined(TUNE)	
	
				T->queenUnderdevelopedPieces[stm] += count;
			#endif
		}


		// Queen mobility
		mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_queen_mobility[mobilityCount];

		#if defined(TUNE)	
	
			T->queenMobility[stm][mobilityCount]++;
		#endif


		// Update values for opp King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_queen_attack;

            #if defined(TUNE)	
    
            	T->queenAttack[opp]++; 
            #endif

            U64 bb = attacksBB & th->evalInfo.kingAttacks[opp];
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	}	

	return score;
}


int kingEval(U8 stm, Thread *th) {
	
	U8 opp = stm ^ 1;

	int kingSq = th->evalInfo.kingSq[stm];
	
	int score = 0;	
	
	assert(kingSq >= 0 && kingSq < 64);

	U64 ourPawns = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	U64 theirPawns = opp ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	U64 pawnStormZone = th->evalInfo.kingZoneBB[stm];

	pawnStormZone |= stm ? pawnStormZone >> 8 : pawnStormZone << 8;

	score += POPCOUNT(th->evalInfo.kingZoneBB[stm] & ourPawns) * weight_king_pawn_shield;
	score += POPCOUNT(pawnStormZone & theirPawns) * weight_king_enemy_pawn_storm;

	#if defined(TUNE)	

		T->kingPawnShield[stm] = POPCOUNT(th->evalInfo.kingZoneBB[stm] & ourPawns); // TODO check logic
		T->kingEnemyPawnStorm[stm] = POPCOUNT(pawnStormZone & theirPawns); // TODO check logic
	#endif


	if (	th->evalInfo.kingAttackersCount[stm] >= 2
		&&	th->evalInfo.kingAdjacentZoneAttacksCount[stm]) { 

		int safetyScore = th->evalInfo.kingAttackersWeight[stm];

        U64 kingUndefendedSquares = 
        		th->evalInfo.attacks[opp]
        		&	th->evalInfo.kingAttacks[stm]
        		&	~(	th->evalInfo.allPawnAttacks[stm] | th->evalInfo.allKnightAttacks[stm] |
        				th->evalInfo.allBishopAttacks[stm] | th->evalInfo.allRookAttacks[stm] |
        				th->evalInfo.allQueenAttacks[stm]);

		U64 enemyPieces = opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

		U64 queenSafeContactCheck = kingUndefendedSquares 
			& th->evalInfo.allQueenAttacks[opp] 
			& ~enemyPieces
			& (		th->evalInfo.allPawnAttacks[opp] 
				|	th->evalInfo.allKnightAttacks[opp] 
				|	th->evalInfo.allBishopAttacks[opp] 
				|	th->evalInfo.allRookAttacks[opp]);

		U64 rookSafeContactCheck = kingUndefendedSquares 
			& th->evalInfo.allRookAttacks[opp] 
			& ~enemyPieces
			& (		th->evalInfo.allPawnAttacks[opp] 
				|	th->evalInfo.allKnightAttacks[opp] 
				|	th->evalInfo.allBishopAttacks[opp] 
				|	th->evalInfo.allQueenAttacks[opp]);
		
		U64 safe = ~(enemyPieces | th->evalInfo.attacks[stm]);
		U64 b1 = Rmagic(kingSq, th->occupied) & safe;
		U64 b2 = Bmagic(kingSq, th->occupied) & safe;

		U64 queenSafeChecks = (b1 | b2) & th->evalInfo.allQueenAttacks[opp];
		U64 rookSafeChecks = b1 & th->evalInfo.allRookAttacks[opp];
		U64 bishopSafeChecks = b2 & th->evalInfo.allBishopAttacks[opp];
		U64 knightSafeChecks = get_knight_attacks(kingSq) 
			& th->evalInfo.allKnightAttacks[opp] & safe;
		
		safetyScore += weight_queen_safe_contact_check * POPCOUNT(queenSafeContactCheck);
		safetyScore += weight_rook_safe_contact_check * POPCOUNT(rookSafeContactCheck);
		safetyScore += weight_queen_check * POPCOUNT(queenSafeChecks);
		safetyScore += weight_rook_check * POPCOUNT(rookSafeChecks);
		safetyScore += weight_bishop_check * POPCOUNT(bishopSafeChecks);
		safetyScore += weight_knight_check * POPCOUNT(knightSafeChecks);
		safetyScore += weight_safety_adjustment;

		#if defined(TUNE)

	        T->queenSafeContactCheck[stm] = POPCOUNT(queenSafeContactCheck);
	        T->rookSafeContactCheck[stm] = POPCOUNT(rookSafeContactCheck);
			T->queenCheck[stm] = POPCOUNT(queenSafeChecks);
			T->rookCheck[stm] = POPCOUNT(rookSafeChecks);
			T->bishopCheck[stm] = POPCOUNT(bishopSafeChecks);
			T->knightCheck[stm] = POPCOUNT(knightSafeChecks);
			T->safetyAdjustment[stm] = 1;
	    	T->safety[stm] = safetyScore;
		#endif
	
 		int mg = ScoreMG(safetyScore), eg = ScoreEG(safetyScore);

        score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20); 
	} 
    else {

    	#if defined(TUNE)
	    	
	    	T->knightAttack[stm] = 0;
			T->bishopAttack[stm] = 0;
			T->rookAttack[stm] = 0;
			T->queenAttack[stm] = 0;
    	#endif
    } 

	return score;
}


// Helper functions

void initPSQT() {

	for (U8 sq = 0; sq < U8_MAX_SQUARES; sq++) {

		WHITE_PSQT[PAWNS][sq] = whitePawnPSQT[sq] + weight_val_pawn; 
		WHITE_PSQT[KNIGHTS][sq] = whiteKnightPSQT[sq] + weight_val_knight; 
		WHITE_PSQT[BISHOPS][sq] = whiteBishopPSQT[sq] + weight_val_bishop; 
		WHITE_PSQT[ROOKS][sq] = whiteRookPSQT[sq] + weight_val_rook; 
		WHITE_PSQT[QUEEN][sq] = whiteQueenPSQT[sq] + weight_val_queen; 
		WHITE_PSQT[KING][sq] = whiteKingPSQT[sq]; 
	
		BLACK_PSQT[PAWNS][sq] = -blackPawnPSQT[sq] - weight_val_pawn;
		BLACK_PSQT[KNIGHTS][sq] = -blackKnightPSQT[sq] - weight_val_knight; 
		BLACK_PSQT[BISHOPS][sq] = -blackBishopPSQT[sq] - weight_val_bishop;
		BLACK_PSQT[ROOKS][sq] = -blackRookPSQT[sq] - weight_val_rook;
		BLACK_PSQT[QUEEN][sq] = -blackQueenPSQT[sq] - weight_val_queen;
		BLACK_PSQT[KING][sq] = -blackKingPSQT[sq];
	}
}


int evalBoard(U8 stm, Thread *th) {

	int score = 0;

	int nCenterControl = POPCOUNT(CENTER & th->evalInfo.attacks[stm]);

	score += nCenterControl * weight_center_control;

	#if defined(TUNE)	
		T->centerControl[stm] = nCenterControl;
	#endif

	return score;
}


int numOfPawnHoles(U8 stm, Thread *th) {

	U64 pawnsBB = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	U64 frontAttackSpans = stm ? 
		bWestAttackFrontSpans(pawnsBB) | bEastAttackFrontSpans(pawnsBB)
		: wWestAttackFrontSpans(pawnsBB) | wEastAttackFrontSpans(pawnsBB); 

	U64 holes = ~frontAttackSpans & EXTENDED_CENTER
		& (stm ? (RANK_5 | RANK_6) : (RANK_3 | RANK_4)); 

	return POPCOUNT(holes);
}

	
int isolatedPawns(U8 stm, Thread *th) {

	return POPCOUNT(isolanis(stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]));
}

int numOfDoublePawns(U8 stm, Thread *th) {

	U64 pawnsBB = stm ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	U64 doublePawnsBB = stm ? bPawnsInfrontOwn(pawnsBB) : wPawnsInfrontOwn(pawnsBB); 

	return POPCOUNT(doublePawnsBB);
}

int countBackWardPawns(U8 stm, Thread *th) {

	return stm ? POPCOUNT(bBackward(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		POPCOUNT(wBackward(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countPassedPawns(U8 stm, Thread *th) {

	return stm ? POPCOUNT(bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		POPCOUNT(wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countDefendedPawns(U8 stm, Thread *th) {

	return stm ? POPCOUNT(bPawnDefendedFromWest(th->blackPieceBB[PAWNS])) + POPCOUNT(bPawnDefendedFromEast(th->blackPieceBB[PAWNS])) : 
		POPCOUNT(wPawnDefendedFromWest(th->whitePieceBB[PAWNS])) + POPCOUNT(wPawnDefendedFromEast(th->whitePieceBB[PAWNS]));
}

// pawns with at least one pawn in front on the same file
U64 wPawnsBehindOwn(U64 wpawns) {

	return wpawns & wRearSpans(wpawns);
}

// pawns with at least one pawn in front on the same file
U64 bPawnsBehindOwn(U64 bpawns) {

	return bpawns & bRearSpans(bpawns);
}

// pawns with at least one pawn behind on the same file
U64 wPawnsInfrontOwn (U64 wpawns) {

	return wpawns & wFrontSpans(wpawns);
}

// pawns with at least one pawn behind on the same file
U64 bPawnsInfrontOwn (U64 bpawns) {

	return bpawns & bFrontSpans(bpawns);
}

U64 wBackward(U64 wpawns, U64 bpawns) {

   U64 stops = wpawns << 8;

   U64 wAttackSpans = wEastAttackFrontSpans(wpawns)
                    | wWestAttackFrontSpans(wpawns);

   U64 bAttacks     = bPawnEastAttacks(bpawns)
                    | bPawnWestAttacks(bpawns);

   return (stops & bAttacks & ~wAttackSpans) >> 8;
}

U64 bBackward(U64 bpawns, U64 wpawns) {
	
   U64 stops = bpawns >> 8;
   U64 bAttackSpans = bEastAttackFrontSpans(bpawns)
                    | bWestAttackFrontSpans(bpawns);

   U64 wAttacks     = wPawnEastAttacks(wpawns)
                    | wPawnWestAttacks(wpawns);

   return (stops & wAttacks & ~bAttackSpans) << 8;
}

U64 wPassedPawns(U64 wpawns, U64 bpawns) {

   U64 allFrontSpans = bFrontSpans(bpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return wpawns & ~allFrontSpans;
}

U64 bPassedPawns(U64 bpawns, U64 wpawns) {

   U64 allFrontSpans = wFrontSpans(wpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return bpawns & ~allFrontSpans;
}

U64 wPawnDefendedFromWest(U64 wpawns) {

   return wpawns & wPawnEastAttacks(wpawns);
}

U64 wPawnDefendedFromEast(U64 wpawns) {

   return wpawns & wPawnWestAttacks(wpawns);
}

U64 bPawnDefendedFromWest(U64 bpawns) {

   return bpawns & bPawnEastAttacks(bpawns);
}

U64 bPawnDefendedFromEast(U64 bpawns) {

   return bpawns & bPawnWestAttacks(bpawns);
}

U64 noNeighborOnEastFile (U64 pawns) {

    return pawns & ~westAttackFileFill(pawns);
}

U64 noNeighborOnWestFile (U64 pawns) {

    return pawns & ~eastAttackFileFill(pawns);
}

U64 isolanis(U64 pawns) {

   return  noNeighborOnEastFile(pawns)
         & noNeighborOnWestFile(pawns);
}

U64 openFiles(U64 wpanws, U64 bpawns) {
 
   return ~fileFill(wpanws) & ~fileFill(bpawns);
}

U64 halfOpenOrOpenFile(U64 gen) {

	return ~fileFill(gen);
}















