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
#include "psqt.h"
#include "hash.h"
#include "magicmoves.h"
#include "nonslidingmoves.h"
#include "tuner.h"
#include "weights.h"
#include "functions.h"

#define MIDGAME 1
#define ENDGAME 2

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

u64 arrFiles[8] = {

	A_FILE, B_FILE, C_FILE, D_FILE,
	E_FILE, F_FILE, G_FILE, H_FILE
};


int WHITE_PSQT[8][64];
int BLACK_PSQT[8][64];


void initEvalInfo(Thread *th) {

	th->evalInfo.clear();


	th->evalInfo.openFilesBB = openFiles(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);


	th->evalInfo.halfOpenFilesBB[WHITE] 
		= halfOpenOrOpenFile(th->whitePieceBB[PAWNS]) ^ th->evalInfo.openFilesBB;	
	th->evalInfo.halfOpenFilesBB[BLACK] 
		= halfOpenOrOpenFile(th->blackPieceBB[PAWNS]) ^ th->evalInfo.openFilesBB;


	th->evalInfo.pawnsAttacks[WHITE] = 
		wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);
	th->evalInfo.pawnsAttacks[BLACK] = 
		bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
	
	th->evalInfo.attacks[WHITE] |= th->evalInfo.pawnsAttacks[WHITE];
	th->evalInfo.attacks[BLACK] |= th->evalInfo.pawnsAttacks[BLACK];
	
	int sq = -1;
	u64 bb = 0ULL, attacks = 0ULL;
	for (int p = KNIGHTS; p <= KING; p++) {

		for (int side = WHITE; side <= BLACK; side++) {

			bb = side ? th->blackPieceBB[p] : th->whitePieceBB[p];
			attacks = 0ULL;
			while (bb) {

				sq = GET_POSITION(bb);
				POP_POSITION(bb);

				assert(sq >= 0 && sq <= 63);

					 if (p == KNIGHTS)	attacks |= get_knight_attacks(sq);
				else if (p == BISHOPS)	attacks |= Bmagic(sq, th->occupied);
				else if (p == ROOKS)	attacks |= Rmagic(sq, th->occupied);
				else if (p == QUEEN)	attacks |= Qmagic(sq, th->occupied);
				else if (p == KING)		attacks |= get_king_attacks(sq);
			}	

				 if (p == KNIGHTS) 	th->evalInfo.knightsAttacks[side] = attacks;
			else if (p == BISHOPS)	th->evalInfo.bishopsAttacks[side] = attacks;
			else if (p == ROOKS)	th->evalInfo.rooksAttacks[side] = attacks;
			else if (p == QUEEN)	th->evalInfo.queensAttacks[side] = attacks;
			else if (p == KING)		th->evalInfo.kingAttacks[side] = attacks;
		
			th->evalInfo.attacks[side] |= attacks;
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

int32_t traceFullEval(TraceCoefficients *traceCoefficients, u8 sideToMove, Thread *th) {

	T = traceCoefficients;

	return fullEval(sideToMove, th);
}

int32_t fullEval(u8 sideToMove, Thread *th) {
	
	#if defined(TUNE)
	#else

		int hashedEval;
		if (probeEval(&hashedEval, th)) {

			return sideToMove == WHITE ? hashedEval : -hashedEval;
		}
	#endif
	

	int nWhitePawns = POPCOUNT(th->whitePieceBB[PAWNS]);
	int nWhiteKnights = POPCOUNT(th->whitePieceBB[KNIGHTS]);
	int nWhiteBishops = POPCOUNT(th->whitePieceBB[BISHOPS]);
	int nWhiteRooks = POPCOUNT(th->whitePieceBB[ROOKS]);
	int nWhiteQueen = POPCOUNT(th->whitePieceBB[QUEEN]);

	int nBlackPawns = POPCOUNT(th->blackPieceBB[PAWNS]);
	int nBlackKnights = POPCOUNT(th->blackPieceBB[KNIGHTS]);
	int nBlackBishops = POPCOUNT(th->blackPieceBB[BISHOPS]);
	int nBlackRooks = POPCOUNT(th->blackPieceBB[ROOKS]);
	int nBlackQueen = POPCOUNT(th->blackPieceBB[QUEEN]);


	#if defined(TUNE)
		
		T->nPawns[BLACK] =  nBlackPawns;
		T->nKnights[BLACK] = nBlackKnights;
		T->nBishops[BLACK] = nBlackBishops;
		T->nRooks[BLACK] = nBlackRooks;
		T->nQueen[BLACK] = nBlackQueen;

		T->nPawns[WHITE] =  nWhitePawns;
		T->nKnights[WHITE] = nWhiteKnights;
		T->nBishops[WHITE] = nWhiteBishops;
		T->nRooks[WHITE] = nWhiteRooks;
		T->nQueen[WHITE] = nWhiteQueen;
	#endif


	initEvalInfo(th);


	int eval = evaluateSide(WHITE, th) - evaluateSide(BLACK, th);

	#if defined(TUNE)	

		T->eval = eval;
	#endif


	// Tapered evaluation 
	int phase = 4 * (nWhiteQueen + nBlackQueen) 
		+ 2 * (nWhiteRooks + nBlackRooks)
      	+ 1 * (nWhiteKnights + nBlackKnights)
      	+ 1 * (nWhiteBishops + nBlackBishops);


	#if defined(TUNE)	

		T->phase = phase;
	#endif


    int32_t score = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;

    #if defined(TUNE)	
	#else

		recordEval(score, th);
	#endif


	return sideToMove == WHITE ? score : -score;
}

int32_t evaluateSide(int side, Thread *th) {

	int32_t score = 0;

	#if defined(TUNE)

		score += pawnsEval(side, th);
	#else
	
		int pawnsScore;
		if (!probePawnHash(&pawnsScore, th)) { // TODO check pawn hash logic

			pawnsScore = pawnsEval(side, th);
			recordPawnHash(pawnsScore, th);
		}

		score += pawnsScore;
	#endif

	score += PSQTScore(side, th) 
		+ knightsEval(side, th) 
		+ bishopsEval(side, th) 
		+ rooksEval(side, th) 
		+ queenEval(side, th) 
		+ kingEval(side, th) 
		+ evalBoard(side, th); 
	
	return score;
}


int32_t pawnsEval(u8 sideToMove, Thread *th) {

	int32_t score = 0;
	int sq = -1, rank = -1;

	#if defined(TUNE)
		
		u64 bb = sideToMove ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
		while (bb) {

			int sq = GET_POSITION(bb);
			POP_POSITION(bb);

			T->pawnPSQT[sideToMove][sideToMove ? Mirror64[sq] : sq] = 1; 
		}
	#endif


	const int nIsolatedPawns = isolatedPawns(sideToMove, th);
	score += nIsolatedPawns * weight_isolated_pawn;

	#if defined(TUNE)	
		
		T->isolatedPawns[sideToMove] = nIsolatedPawns;
	#endif

	const int nDoublePawns = numOfDoublePawns(sideToMove, th);
	score += nDoublePawns * weight_double_pawn;

	#if defined(TUNE)	
		
		T->doublePawns[sideToMove] = nDoublePawns;
	#endif

	const int nBackwardPawns = countBackWardPawns(sideToMove, th);
	score += nBackwardPawns * weight_backward_pawn;

	#if defined(TUNE)	
		
		T->backwardPawns[sideToMove] = nBackwardPawns;
	#endif

	u64 defendedPawns = sideToMove ? 
		th->blackPieceBB[PAWNS] & th->evalInfo.pawnsAttacks[BLACK]
		: th->whitePieceBB[PAWNS] & th->evalInfo.pawnsAttacks[WHITE];

	int nDefendedPawns = POPCOUNT(defendedPawns);
	score += nDefendedPawns * weight_defended_pawn;

	#if defined(TUNE)	
		
		T->defendedPawns[sideToMove] = nDefendedPawns;
	#endif

	const int nPawnHoles = numOfPawnHoles(sideToMove, th);
	score += nPawnHoles * weight_pawn_hole;

	#if defined(TUNE)	
		
		T->pawnHoles[sideToMove] = nPawnHoles;
	#endif

	u64 passedPawns = sideToMove ? bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])
		: wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
	while (passedPawns) {
		
		sq = GET_POSITION(passedPawns);
		POP_POSITION(passedPawns);

		assert(sq >= 0 && sq <= 63);

		rank = sideToMove ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		assert(rank+1 >= 1 && rank+1 <= 8);
	

		if ((1ULL << sq) & th->evalInfo.pawnsAttacks[sideToMove]) {

			score += arr_weight_defended_passed_pawn[rank];
			
			#if defined(TUNE)	
			
				T->defendedPassedPawn[sideToMove][rank]++;
			#endif
		} else {
			
			score += arr_weight_passed_pawn[rank];

			#if defined(TUNE)	
			
				T->passedPawn[sideToMove][rank]++;
			#endif
		}		
	}

	return score;
}



int32_t knightsEval(u8 side, Thread *th) {

	const u8 opp = side ^ 1;

	int32_t score = 0;
	int mobilityCount = 0;

	int sq = -1, rank = -1; 

	u64 knightsBB = side ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];

	while (knightsBB) {

		sq = GET_POSITION(knightsBB);
		POP_POSITION(knightsBB);

		assert(sq >= 0 && sq <= 63);

		u64 attacksBB = get_knight_attacks(sq);

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_knight_attack;

            #if defined(TUNE)	
            
            	T->knightAttack[opp]++; 
            #endif

            u64 bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb) {

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}

		// undefended
		if (!((1ULL << sq) & th->evalInfo.attacks[side])) {

			score += weight_undefended_knight;

			#if defined(TUNE)	
			
				T->undefendedKnight[side]++;			
			#endif
		}

		// defended by pawn
		if ((1ULL << sq) & th->evalInfo.pawnsAttacks[side]) {

			score += weight_knight_defended_by_pawn;

			#if defined(TUNE)	
			
				T->knightDefendedByPawn[side]++;
			#endif
		}


		u64 mobilityBB = attacksBB 
			& ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]) 
			& ~(th->evalInfo.pawnsAttacks[opp]);

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_knight_mobility[mobilityCount];

		#if defined(TUNE)

			T->knightPSQT[side][side ? Mirror64[sq] : sq] = 1; 
			T->knightMobility[side][mobilityCount]++;
		#endif
	}

	return score;
}


int32_t bishopsEval(u8 side, Thread *th) {

	const u8 opp = side ^ 1;

	int32_t score = 0;
	int mobilityCount = 0;
	int sq = -1;

	u64 bishopBB = side ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
	
	const int nBishops = POPCOUNT(bishopBB);
	
	if (nBishops == 2) {	// This should be equal to 2 and not greater or equal to 
							// since we want to capture the weight for exactly 2 bishops
		score += weight_bishop_pair;

		#if defined(TUNE)	
			
			T->bishopPair[side]++;
		#endif
	}

	while (bishopBB) {
	
		sq = GET_POSITION(bishopBB);
		POP_POSITION(bishopBB);

		assert(sq >= 0 && sq <= 63);

		u64 attacksBB = Bmagic(sq, th->occupied);

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_bishop_attack;

            #if defined(TUNE)	
            
            	T->bishopAttack[opp]++; 
            #endif
            	
            u64 bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	

		if (!((1ULL << sq) & th->evalInfo.attacks[side])) {

			score += weight_undefended_bishop;

			#if defined(TUNE)	
			
				T->undefendedBishop[side]++;			
			#endif
		}

		
		int count = POPCOUNT(th->evalInfo.bishopsAttacks[side] 
			& (side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]));
		
		if (count > 0) {

			score += count * weight_bad_bishop;

			#if defined(TUNE)	

				T->badBishop[side] = count;
			#endif
		}

		u64 mobilityBB = attacksBB & ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_bishop_mobility[mobilityCount];

		#if defined(TUNE) 

			T->bishopPSQT[side][side ? Mirror64[sq] : sq] = 1; 	
			T->bishopMobility[side][mobilityCount]++;
		#endif
	}
	
	return score;
}


/** TODO
Increasing value as pawns disappear 
Tarrasch Rule
*/
int32_t rooksEval(u8 side, Thread *th) {
	
	const u8 opp = side ^ 1;

	int32_t score = 0;
	int sq = -1;

	int mobilityCount = 0;

	const int kingSq = th->evalInfo.kingSq[side];

	u64 rooksBB = side ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];

	while (rooksBB) {

		sq = GET_POSITION(rooksBB);
		POP_POSITION(rooksBB);

		assert(sq >= 0 && sq <= 63);

		#if defined(TUNE)	
			
			T->rookPSQT[side][side ? Mirror64[sq] : sq] = 1; 
		#endif

		if ((1ULL << sq) & th->evalInfo.halfOpenFilesBB[side]) {

			score += weight_rook_half_open_file;
		
			#if defined(TUNE)	
			
				T->halfOpenFile[side]++;
			#endif
		}


		if ((1ULL << sq) & th->evalInfo.openFilesBB) {

			score += weight_rook_open_file;

			#if defined(TUNE)	
			
				T->openFile[side]++;
			#endif
		}


		if (arrFiles[sq & 7] & 
			(opp ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

			score += weight_rook_enemy_queen_same_file;

			#if defined(TUNE)	
			
				T->rookEnemyQueenSameFile[side]++;
			#endif
		}


		u64 attacksBB = Rmagic(sq, th->occupied);

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_rook_attack;

            #if defined(TUNE)	
            
            	T->rookAttack[opp]++; 
            #endif

            u64 bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb)	{
            	
           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}
	

		u64 mobilityBB = attacksBB & 
			~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_rook_mobility[mobilityCount];

		#if defined(TUNE)	
			
			T->rookMobility[side][mobilityCount]++;
		#endif

		if (attacksBB & rooksBB) {

			score += weight_rook_supporting_friendly_rook;

			#if defined(TUNE)	
				
				T->rookSupportingFriendlyRook[side] = 1;
			#endif
		}

			
		if (side) {

			// for black

			// TODO king can be in other squares too and still block the rook
			// Penalty for a Rook blocked by an uncastled King
			// if (kingSq == 60 && sq >= 56 && sq < 64) {
	
			// 	score += weight_rook_blocked_by_king;

			// 	if (TUNE) {

			// 		T->rookBlockedByKing[BLACK]++;
			// 	}
			// }


			// Rook on Seventh rank (Rank 2 for black)
			if ((1ULL << sq) & RANK_2) {

				score += weight_rook_on_seventh_rank;
			
				#if defined(TUNE)	
				
					T->rookOnSeventhRank[BLACK]++;					
				#endif
			}

			// Rook on Eight rank (Rank 1 for black)
			if ((1ULL << sq) & RANK_1) {

				score += weight_rook_on_eight_rank;

				#if defined(TUNE)	

					T->rookOnEightRank[BLACK]++;					
				#endif
			} 	
		} else {
			
			// for white 	


			// Penalty for a Rook blocked by an uncastled King
			// if (kingSq == 4 && (sq >= 0 && sq < 8)) {

			// 	score += weight_rook_blocked_by_king;

			// 	if (TUNE) {

			// 		T->rookBlockedByKing[WHITE]++;
			// 	}
			// } 				
		


			// Rook on Seventh rank (Rank 2 for black)
			if ((1ULL << sq) & RANK_7) {

				score += weight_rook_on_seventh_rank;
			
				#if defined(TUNE)	
	
					T->rookOnSeventhRank[WHITE]++;					
				#endif
			} 	


			// Rook on Eight rank (Rank 1 for black)
			if ((1ULL << sq) & RANK_8) {

				score += weight_rook_on_eight_rank;

				#if defined(TUNE)	
	
					T->rookOnEightRank[WHITE]++;					
				#endif
			} 
		}
	}

	return score;
}




int32_t queenEval(u8 side, Thread *th) {

	u8 opp = side ^ 1;

	int32_t score = 0;

	int sq = -1;
	int mobilityCount = 0;

	u64 attacksBB = 0ULL, mobilityBB = 0ULL; 	
	u64 queenBB = side ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];

	while (queenBB) {

		sq = GET_POSITION(queenBB);
		POP_POSITION(queenBB);

		assert(sq >= 0 && sq <= 63);

		#if defined(TUNE)
	
			T->queenPSQT[side][side ? Mirror64[sq] : sq] = 1; 		
		#endif

			// early game
		if (POPCOUNT(th->occupied) > 20 && side ? sq <= 55 : sq >= 8) { // recheck logic

			u64 minorPiecesBB = side ? 
				th->blackPieceBB[KNIGHTS] | th->blackPieceBB[BISHOPS] :
				th->whitePieceBB[KNIGHTS] | th->whitePieceBB[BISHOPS];

			u64 underdevelopedPiecesBB = (side ? RANK_8 : RANK_1) & minorPiecesBB; 

			int count = POPCOUNT(underdevelopedPiecesBB);
			
			score += count * weight_queen_underdeveloped_pieces;
			
			#if defined(TUNE)	
	
				T->queenUnderdevelopedPieces[side] += count;
			#endif
		}


		attacksBB = Qmagic(sq, th->occupied);

		if (attacksBB & th->evalInfo.kingZoneBB[opp]) {
 			
 			th->evalInfo.kingAttackersCount[opp]++;
            th->evalInfo.kingAttackersWeight[opp] += weight_queen_attack;

            #if defined(TUNE)	
    
            	T->queenAttack[opp]++; 
            #endif

            u64 bb = (attacksBB & th->evalInfo.kingAttacks[opp]);
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[opp] += POPCOUNT(bb);
            }
		}


		mobilityBB = attacksBB & ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = POPCOUNT(mobilityBB);


		score += arr_weight_queen_mobility[mobilityCount];


		#if defined(TUNE)	
	
			T->queenMobility[side][mobilityCount]++;
		#endif
	}	

	return score;
}



// TODO check logic, cleanup
// int kingEval(u8 us, EvalInfo *evalInfo, Thread *th) {
	
// 	int sq = -1;												    	
// 	int score = 0;
// 	int attackCounter = 0;		
// 	int nAttacks = 0;
// 	int attackers = 0;

// 	const u8 opp = us ^ 1;
// 	u64 enemyPieceBB = 0ULL, attacks = 0ULL;

// 	u64 kingBB = us ? th->blackPieceBB[KING] : th->whitePieceBB[KING];
// 	u64 friendlyPawns = us ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
// 	u64 enemyPawns = opp ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];


// 	const int kingSq = th->evalInfo.kingSq[us];
// 	const u64 shieldBB = th->evalInfo.shieldBB[us];

// 	if (TUNE)	T->kingPSQT[us][us ? Mirror64[kingSq] : kingSq] = 1; 			

// 	// TODO logic needs rechecking (many pawns or just three pawns)
// 	const int nPawnsShield = POPCOUNT(shieldBB & friendlyPawns);
// 	score += nPawnsShield * weight_king_pawn_shield;

// 	// check for enemy pawns storm
// 	// add another rank resulting in 3x3 area above king for enemy pawns storm
// 	u64 enemyPawnStormArea = shieldBB | (us ? shieldBB >> 8 : shieldBB << 8);
	
// 	const int nEnemyPawnsStorm = POPCOUNT(enemyPawnStormArea & enemyPawns);
// 	score += nEnemyPawnsStorm * weight_king_enemy_pawn_storm;

// 	if (TUNE) {

// 		T->kingPawnShield[us] = nPawnsShield;
// 		T->kingEnemyPawnStorm[us] = nEnemyPawnsStorm;
// 	}


// 	if (th->evalInfo.kingAttackersCount[opp] >= 2
// 		&& th->evalInfo.kingAdjacentZoneAttacksCount[opp]) {

// 		u64 kingUndefendedSquares  =
// 	        th->evalInfo.attacks[opp] & ~th->evalInfo.pawnsAttacks[us]
// 	        & ~th->evalInfo.knightsAttacks[us] & ~th->evalInfo.bishopsAttacks[us]
// 	        & ~th->evalInfo.rooksAttacks[us] & ~th->evalInfo.queensAttacks[us]
// 	        & th->evalInfo.kingAttacks[us];

// 	   	attackCounter =  std::min(25, (th->evalInfo.kingAttackersCount[opp] * th->evalInfo.kingAttackersWeight[opp]) / 2)
//                  + 3 * (th->evalInfo.kingAdjacentZoneAttacksCount[opp] + count_1s_max_15(kingUndefendedSquares));
               

// 		u64 enemyPieces = opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

// 		u64 b, b2;

// 		b = kingUndefendedSquares & th->evalInfo.queensAttacks[opp] & ~enemyPieces;
	     
// 		if(b) {
		
// 		    u64 attackedByOthers =
// 				th->evalInfo.pawnsAttacks[opp] |
// 				th->evalInfo.knightsAttacks[opp] |
// 				th->evalInfo.bishopsAttacks[opp] |
// 				th->evalInfo.rooksAttacks[opp];

// 			b &= attackedByOthers;

// 			int count = count_1s_max_15(b);

// 	        attackCounter += weight_queen_safe_contact_check_counter * count;
// 		}


// 		b = kingUndefendedSquares & th->evalInfo.rooksAttacks[opp] & ~enemyPieces;
	     
// 		if(b) {
		
// 		    u64 attackedByOthers =
// 				th->evalInfo.pawnsAttacks[opp] |
// 				th->evalInfo.knightsAttacks[opp] |
// 				th->evalInfo.bishopsAttacks[opp] |
// 				th->evalInfo.rooksAttacks[opp];

// 			b &= attackedByOthers;

// 			int count = count_1s_max_15(b);

// 	        attackCounter += weight_rook_safe_contact_check_counter * count;
// 		}



// 		b = Rmagic(kingSq, th->occupied) & ~enemyPieces & ~th->evalInfo.attacks[us];
// 		// Queen checks
// 		b2 = b & th->evalInfo.queensAttacks[opp];
// 		if(b2) attackCounter += weight_queen_check_counter * count_1s_max_15(b2);

// 		// Rook checks
// 		b2 = b & th->evalInfo.rooksAttacks[opp];
// 		if(b2) attackCounter += weight_rook_check_counter * count_1s_max_15(b2);


// 	    b = Bmagic(kingSq, th->occupied) & ~enemyPieces & ~th->evalInfo.attacks[us];
// 	    // Queen checks
// 		b2 = b & th->evalInfo.queensAttacks[opp];
// 	    if(b2) attackCounter += weight_queen_check_counter * count_1s_max_15(b2);

// 	    // Bishop checks
// 	    b2 = b & th->evalInfo.bishopsAttacks[opp];
// 	    if(b2) attackCounter += weight_bishop_check_counter * count_1s_max_15(b2);



// 	    b = get_knight_attacks(kingSq) & ~enemyPieces & ~th->evalInfo.attacks[us];
// 	    // Knight checks
// 	    b2 = b & th->evalInfo.knightsAttacks[opp];
// 	    if(b2) attackCounter += weight_knight_check_counter * count_1s_max_15(b2);
	  
	  

// 		attackCounter = std::min(99, attackCounter);
		
// 		int safety = SafetyTable[attackCounter];

// 		int mg = ScoreMG(safety);
// 		int eg = ScoreEG(safety);
		
// 	    score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20);  
	
// 		if (TUNE)	T->safety[us] = safety;
// 	}

// 	return score;
// }


int32_t kingEval(u8 us, Thread *th) {
	
	int sq = -1;												    	
	int attackCounter = 0;		
	int nAttacks = 0;
	int attackers = 0;
	
	int32_t score = 0;
	
	const u8 opp = us ^ 1;
	u64 enemyPieceBB = 0ULL, attacks = 0ULL;

	u64 friendlyPawns = us ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	u64 enemyPawns = opp ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];


	const int kingSq = th->evalInfo.kingSq[us];
	
	#if defined(TUNE)	
	
		T->kingPSQT[us][us ? Mirror64[kingSq] : kingSq] = 1; 			
	#endif

	const int nPawnsShield = POPCOUNT(th->evalInfo.kingZoneBB[us] & friendlyPawns);
	score += nPawnsShield * weight_king_pawn_shield;

	#if defined(TUNE) 	
	
		T->kingPawnShield[us] = nPawnsShield;
	#endif

	const u64 enemyPawnStormArea = th->evalInfo.kingZoneBB[us] | 
		(us ? th->evalInfo.kingZoneBB[us] >> 8 : th->evalInfo.kingZoneBB[us] << 8);

	const int nEnemyPawnsStorm = POPCOUNT(enemyPawnStormArea & enemyPawns);
	score += nEnemyPawnsStorm * weight_king_enemy_pawn_storm;

	#if defined(TUNE)	

		T->kingEnemyPawnStorm[us] = nEnemyPawnsStorm;
	#endif

	const int queenCount = POPCOUNT(opp ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN]);

	if (queenCount >= 1 
		&& th->evalInfo.kingAttackersCount[us] >= 2
		&& th->evalInfo.kingAdjacentZoneAttacksCount[us]) { // TODO recheck logic for if check

		u64 kingUndefendedSquares  =
	        th->evalInfo.attacks[opp] & ~th->evalInfo.pawnsAttacks[us]
	        & ~th->evalInfo.knightsAttacks[us] & ~th->evalInfo.bishopsAttacks[us]
	        & ~th->evalInfo.rooksAttacks[us] & ~th->evalInfo.queensAttacks[us]
	        & th->evalInfo.kingAttacks[us];

		u64 enemyPieces = opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

		int32_t safetyScore = th->evalInfo.kingAttackersWeight[us];

		int count;
		u64 b, b2;

		b = kingUndefendedSquares & th->evalInfo.queensAttacks[opp] & ~enemyPieces;
	     
		if (b) {
		
		    u64 attackedByOthers =
				th->evalInfo.pawnsAttacks[opp] |
				th->evalInfo.knightsAttacks[opp] |
				th->evalInfo.bishopsAttacks[opp] |
				th->evalInfo.rooksAttacks[opp];

			b &= attackedByOthers;

			count = POPCOUNT(b);

	        safetyScore += weight_queen_safe_contact_check * count;

	        #if defined(TUNE)	
	        	T->queenSafeContactCheck[us] = count;
			#endif
		}


		b = kingUndefendedSquares & th->evalInfo.rooksAttacks[opp] & ~enemyPieces;
	     
		if (b) {
		
		    u64 attackedByOthers =
				th->evalInfo.pawnsAttacks[opp] |
				th->evalInfo.knightsAttacks[opp] |
				th->evalInfo.bishopsAttacks[opp] |
				th->evalInfo.rooksAttacks[opp];

			b &= attackedByOthers;

			count = POPCOUNT(b);

	        safetyScore += weight_rook_safe_contact_check * count;
			
	        #if defined(TUNE)	
	        	T->rookSafeContactCheck[us] = count;
			#endif
		}
		

		b = Qmagic(kingSq, th->occupied) & ~enemyPieces & ~th->evalInfo.attacks[us];

		b2 = b & th->evalInfo.queensAttacks[opp];
		if (b2) {
		
			count = POPCOUNT(b);
			
			safetyScore += weight_queen_check * count;
			
			#if defined(TUNE)	
				T->queenCheck[us] = count;
			#endif
		}


		b = Rmagic(kingSq, th->occupied) & ~enemyPieces & ~th->evalInfo.attacks[us];
		
		// Rook checks
		b2 = b & th->evalInfo.rooksAttacks[opp];
		if (b2) {
			
			count = POPCOUNT(b);
	
			safetyScore += weight_rook_check * count;

			#if defined(TUNE)	
				T->rookCheck[us] = count;
			#endif
		}


	    b = Bmagic(kingSq, th->occupied) & ~enemyPieces & ~th->evalInfo.attacks[us];

	    // Bishop checks
	    b2 = b & th->evalInfo.bishopsAttacks[opp];
	    if (b2) {

			count = POPCOUNT(b);

		  	safetyScore += weight_bishop_check * count;

		  	#if defined(TUNE)	
		  		T->bishopCheck[us] = count;
	    	#endif
	    } 

	  
	    b = get_knight_attacks(kingSq) & ~enemyPieces & ~th->evalInfo.attacks[us];
	    // Knight checks
	    b2 = b & th->evalInfo.knightsAttacks[opp];
	    if (b2) {

			count = POPCOUNT(b);

	    	safetyScore += weight_knight_check * count;
	  		
	  		#if defined(TUNE)	
	  			T->knightCheck[us] = count;
	    	#endif
	    } 


	    safetyScore += weight_safety_adjustment;

	    #if defined(TUNE)	
	    	T->safetyAdjustment[us] = 1;
	    #endif

	
		int mg = ScoreMG(safetyScore);
		int eg = ScoreEG(safetyScore);
		
	    score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20);  
	

		#if defined(TUNE)	
			T->safety[us] = safetyScore;
		#endif
	} 
    else {

    	#if defined(TUNE)
	    	
	    	T->knightAttack[us] = 0;
			T->bishopAttack[us] = 0;
			T->rookAttack[us] = 0;
			T->queenAttack[us] = 0;
    	#endif
    } 

	return score;
}


int32_t PSQTScore(u8 sideToMove, Thread *th) {
	
	int sq;
	int32_t score = 0;

	for (int piece = PAWNS; piece <= KING; piece++) {

		u64 bb = sideToMove ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
		while (bb) {

			sq = GET_POSITION(bb); 
			
			score += sideToMove ? BLACK_PSQT[piece][sq] : WHITE_PSQT[piece][sq];
	
			POP_POSITION(bb); 
		}
	}

	return score;
}


// Helper functions

void initPSQT() {

	for (int sq = 0; sq < 64; sq++) {

		WHITE_PSQT[PAWNS][sq] = pawnPSQT[sq] + weight_pawn; 
		BLACK_PSQT[PAWNS][sq] = pawnPSQT[Mirror64[sq]] + weight_pawn;

		WHITE_PSQT[KNIGHTS][sq] = knightPSQT[sq] + weight_knight; 
		BLACK_PSQT[KNIGHTS][sq] = knightPSQT[Mirror64[sq]] + weight_knight; 

		WHITE_PSQT[BISHOPS][sq] = bishopPSQT[sq] + weight_bishop; 
		BLACK_PSQT[BISHOPS][sq] = bishopPSQT[Mirror64[sq]] + weight_bishop;

		WHITE_PSQT[ROOKS][sq] = rookPSQT[sq] + weight_rook; 
		BLACK_PSQT[ROOKS][sq] = rookPSQT[Mirror64[sq]] + weight_rook;
	
		WHITE_PSQT[QUEEN][sq] = queenPSQT[sq] + weight_queen; 
		BLACK_PSQT[QUEEN][sq] = queenPSQT[Mirror64[sq]] + weight_queen;

		WHITE_PSQT[KING][sq] = kingPSQT[sq]; 
		BLACK_PSQT[KING][sq] = kingPSQT[Mirror64[sq]];
	}
}


int32_t evalBoard(u8 side, Thread *th) {

	int32_t score = 0;

	int nCenterControl = POPCOUNT(CENTER & th->evalInfo.attacks[side]);

	score += nCenterControl * weight_center_control;

	#if defined(TUNE)	
		T->centerControl[side] = nCenterControl;
	#endif

	return score;
}


int numOfPawnHoles(u8 side, Thread *th) {

	u64 pawnsBB = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	u64 frontAttackSpans = side ? 
		bWestAttackFrontSpans(pawnsBB) | bEastAttackFrontSpans(pawnsBB)
		: wWestAttackFrontSpans(pawnsBB) | wEastAttackFrontSpans(pawnsBB); 

	u64 holes = ~frontAttackSpans & EXTENDED_CENTER
		& (side ? (RANK_5 | RANK_6) : (RANK_3 | RANK_4)); 

	return POPCOUNT(holes);
}

	
int isolatedPawns(u8 side, Thread *th) {

	return POPCOUNT(isolanis(side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]));
}

int numOfDoublePawns(u8 side, Thread *th) {

	u64 pawnsBB = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	u64 doublePawnsBB = side ? bPawnsInfrontOwn(pawnsBB) : wPawnsInfrontOwn(pawnsBB); 

	return POPCOUNT(doublePawnsBB);
}

int countBackWardPawns(u8 side, Thread *th) {

	return side ? POPCOUNT(bBackward(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		POPCOUNT(wBackward(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countPassedPawns(u8 side, Thread *th) {

	return side ? POPCOUNT(bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		POPCOUNT(wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countDefendedPawns(u8 side, Thread *th) {

	return side ? POPCOUNT(bPawnDefendedFromWest(th->blackPieceBB[PAWNS])) + POPCOUNT(bPawnDefendedFromEast(th->blackPieceBB[PAWNS])) : 
		POPCOUNT(wPawnDefendedFromWest(th->whitePieceBB[PAWNS])) + POPCOUNT(wPawnDefendedFromEast(th->whitePieceBB[PAWNS]));
}

// pawns with at least one pawn in front on the same file
u64 wPawnsBehindOwn(u64 wpawns) {

	return wpawns & wRearSpans(wpawns);
}

// pawns with at least one pawn in front on the same file
u64 bPawnsBehindOwn(u64 bpawns) {

	return bpawns & bRearSpans(bpawns);
}

// pawns with at least one pawn behind on the same file
u64 wPawnsInfrontOwn (u64 wpawns) {

	return wpawns & wFrontSpans(wpawns);
}

// pawns with at least one pawn behind on the same file
u64 bPawnsInfrontOwn (u64 bpawns) {

	return bpawns & bFrontSpans(bpawns);
}

u64 wBackward(u64 wpawns, u64 bpawns) {

   u64 stops = wpawns << 8;

   u64 wAttackSpans = wEastAttackFrontSpans(wpawns)
                    | wWestAttackFrontSpans(wpawns);

   u64 bAttacks     = bPawnEastAttacks(bpawns)
                    | bPawnWestAttacks(bpawns);

   return (stops & bAttacks & ~wAttackSpans) >> 8;
}

u64 bBackward(u64 bpawns, u64 wpawns) {
	
   u64 stops = bpawns >> 8;
   u64 bAttackSpans = bEastAttackFrontSpans(bpawns)
                    | bWestAttackFrontSpans(bpawns);

   u64 wAttacks     = wPawnEastAttacks(wpawns)
                    | wPawnWestAttacks(wpawns);

   return (stops & wAttacks & ~bAttackSpans) << 8;
}

u64 wPassedPawns(u64 wpawns, u64 bpawns) {

   u64 allFrontSpans = bFrontSpans(bpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return wpawns & ~allFrontSpans;
}

u64 bPassedPawns(u64 bpawns, u64 wpawns) {

   u64 allFrontSpans = wFrontSpans(wpawns);
   allFrontSpans |= eastOne(allFrontSpans)
                 |  westOne(allFrontSpans);

   return bpawns & ~allFrontSpans;
}

u64 wPawnDefendedFromWest(u64 wpawns) {

   return wpawns & wPawnEastAttacks(wpawns);
}

u64 wPawnDefendedFromEast(u64 wpawns) {

   return wpawns & wPawnWestAttacks(wpawns);
}

u64 bPawnDefendedFromWest(u64 bpawns) {

   return bpawns & bPawnEastAttacks(bpawns);
}

u64 bPawnDefendedFromEast(u64 bpawns) {

   return bpawns & bPawnWestAttacks(bpawns);
}

u64 noNeighborOnEastFile (u64 pawns) {

    return pawns & ~westAttackFileFill(pawns);
}

u64 noNeighborOnWestFile (u64 pawns) {

    return pawns & ~eastAttackFileFill(pawns);
}

u64 isolanis(u64 pawns) {

   return  noNeighborOnEastFile(pawns)
         & noNeighborOnWestFile(pawns);
}

u64 openFiles(u64 wpanws, u64 bpawns) {
 
   return ~fileFill(wpanws) & ~fileFill(bpawns);
}

u64 halfOpenOrOpenFile(u64 gen) {

	return ~fileFill(gen);
}















