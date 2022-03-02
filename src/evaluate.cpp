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


	th->evalInfo.allPawnAttacks[WHITE] = 
		wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);
	th->evalInfo.allPawnAttacks[BLACK] = 
		bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
	
	th->evalInfo.attacks[WHITE] |= th->evalInfo.allPawnAttacks[WHITE];
	th->evalInfo.attacks[BLACK] |= th->evalInfo.allPawnAttacks[BLACK];
	
	int sq = -1;
	u64 bb = 0ULL;
	for (int p = KNIGHTS; p <= KING; p++) {

		for (int side = WHITE; side <= BLACK; side++) {

			bb = side ? th->blackPieceBB[p] : th->whitePieceBB[p];
			
			while (bb) {

				sq = GET_POSITION(bb);
				POP_POSITION(bb);

				assert(sq >= 0 && sq <= 63);

				if (p == KNIGHTS) {

					th->evalInfo.knightAttacks[side][sq] = get_knight_attacks(sq);
					th->evalInfo.allKnightAttacks[side] |= th->evalInfo.knightAttacks[side][sq];
					th->evalInfo.attacks[side] |= th->evalInfo.knightAttacks[side][sq];		
				}	
				else if (p == BISHOPS) {

					th->evalInfo.bishopAttacks[side][sq] = Bmagic(sq, th->occupied);
					th->evalInfo.allBishopAttacks[side] |= th->evalInfo.bishopAttacks[side][sq];		
					th->evalInfo.attacks[side] |= th->evalInfo.bishopAttacks[side][sq];		
				}
				else if (p == ROOKS) {
					
					th->evalInfo.rookAttacks[side][sq] = Rmagic(sq, th->occupied);
					th->evalInfo.allRookAttacks[side] |= th->evalInfo.rookAttacks[side][sq];						
					th->evalInfo.attacks[side] |= th->evalInfo.rookAttacks[side][sq];				
				}	
				else if (p == QUEEN) {

					th->evalInfo.queenAttacks[side][sq] = Qmagic(sq, th->occupied);
					th->evalInfo.allQueenAttacks[side] |= th->evalInfo.queenAttacks[side][sq];
					th->evalInfo.attacks[side] |= th->evalInfo.queenAttacks[side][sq];		
				}
				else if (p == KING)	{
					
					th->evalInfo.kingAttacks[side] = get_king_attacks(sq);			
					th->evalInfo.attacks[side] |= th->evalInfo.kingAttacks[side];
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

int32_t traceFullEval(TraceCoefficients *traceCoefficients, u8 sideToMove, Thread *th) {

	T = traceCoefficients;

	return fullEval(sideToMove, th);
}

int32_t fullEval(u8 sideToMove, Thread *th) {
	

	#if defined(TUNE)
	#else

		int32_t hashedEval;
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


	int32_t eval = 0;

	/*#if defined(TUNE)

		eval += pawnsEval(WHITE, th) - pawnsEval(BLACK, th);
	#else
	
		int pawnsScore;
		if (!probePawnHash(&pawnsScore, th)) { // TODO check pawn hash logic

			pawnsScore = pawnsEval(side, th);
			recordPawnHash(pawnsScore, th);
		}

		score += pawnsScore;
	#endif
*/

	eval += PSQTScore(WHITE, th) - PSQTScore(BLACK, th);
	eval += pawnsEval(WHITE, th) - pawnsEval(BLACK, th);
	eval += knightsEval(WHITE, th) - knightsEval(BLACK, th);
	eval += bishopsEval(WHITE, th) - bishopsEval(BLACK, th);
	eval += rooksEval(WHITE, th) - rooksEval(BLACK, th);
	eval += queenEval(WHITE, th) - queenEval(BLACK, th);
	
	// evaluation of other pieces other than king needs to be done first
	// before king eval because of values needed for king safety calculation
	eval += kingEval(WHITE, th) - kingEval(BLACK, th);
	eval += evalBoard(WHITE, th) - evalBoard(BLACK, th);
	


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
		th->blackPieceBB[PAWNS] & th->evalInfo.allPawnAttacks[BLACK]
		: th->whitePieceBB[PAWNS] & th->evalInfo.allPawnAttacks[WHITE];

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
	

		if ((1ULL << sq) & th->evalInfo.allPawnAttacks[sideToMove]) {

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

	const u8 them = side ^ 1;

	int32_t score = 0;
	int mobilityCount = 0;

	int sq = -1, rank = -1; 

	u64 knightsBB = side ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];

	while (knightsBB) {

		sq = GET_POSITION(knightsBB);
		POP_POSITION(knightsBB);

		assert(sq >= 0 && sq <= 63);

		u64 attacksBB = th->evalInfo.knightAttacks[side][sq];

		if (attacksBB & th->evalInfo.kingZoneBB[them]) {
 			
 			th->evalInfo.kingAttackersCount[them]++;
            th->evalInfo.kingAttackersWeight[them] += weight_knight_attack;

            #if defined(TUNE)	
            
            	T->knightAttack[them]++; 
            #endif

            u64 bb = (attacksBB & th->evalInfo.kingAttacks[them]);
            if (bb) {

           		th->evalInfo.kingAdjacentZoneAttacksCount[them] += POPCOUNT(bb);
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
		if ((1ULL << sq) & th->evalInfo.allPawnAttacks[side]) {

			score += weight_knight_defended_by_pawn;

			#if defined(TUNE)	
			
				T->knightDefendedByPawn[side]++;
			#endif
		}


		u64 mobilityBB = attacksBB 
			& ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]) 
			& ~(th->evalInfo.allPawnAttacks[them]);

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

	const u8 them = side ^ 1;

	int32_t score = 0;
	int mobilityCount = 0;
	int sq = -1;

	u64 bishopBB = side ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
	
	const int nBishops = POPCOUNT(bishopBB);
	
	// Bishop pair
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
		
		u64 attacksBB = th->evalInfo.bishopAttacks[side][sq];


		#if defined(TUNE) 
			// Bishop piece square table
			T->bishopPSQT[side][side ? Mirror64[sq] : sq] = 1; 	
		#endif


		// Bishop blocked by own pawn
		if (attacksBB & side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]) {

			score += weight_bad_bishop;

			#if defined(TUNE)	

				T->badBishop[side] = 1;
			#endif
		}


		// Bishop mobility
		u64 mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_bishop_mobility[mobilityCount];

		#if defined(TUNE) 

			T->bishopMobility[side][mobilityCount]++;
		#endif



		// Update values for them King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[them]) {
 			
 			th->evalInfo.kingAttackersCount[them]++;
            th->evalInfo.kingAttackersWeight[them] += weight_bishop_attack;

            #if defined(TUNE)	
            
            	T->bishopAttack[them]++; 
            #endif
            	
            u64 bb = (attacksBB & th->evalInfo.kingAttacks[them]);
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[them] += POPCOUNT(bb);
            }
		}
	}
	
	return score;
}


/** TODO
Increasing value as pawns disappear 
Tarrasch Rule
*/
int32_t rooksEval(u8 side, Thread *th) {
	
	const u8 them = side ^ 1;

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


		// Rook on half open file
		if ((1ULL << sq) & th->evalInfo.halfOpenFilesBB[side]) {

			score += weight_rook_half_open_file;
		
			#if defined(TUNE)	
			
				T->halfOpenFile[side]++;
			#endif
		}

		// Rook on open file
		if ((1ULL << sq) & th->evalInfo.openFilesBB) {

			score += weight_rook_open_file;

			#if defined(TUNE)	
			
				T->openFile[side]++;
			#endif
		}

		// Rook on same file as enemy Queen
		if (arrFiles[sq & 7] & 
			(them ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

			score += weight_rook_enemy_queen_same_file;

			#if defined(TUNE)	
			
				T->rookEnemyQueenSameFile[side]++;
			#endif
		}

		// Rook on Seventh rank
		if ((1ULL << sq) & (side ? RANK_2 : RANK_7)) {

			score += weight_rook_on_seventh_rank;
		
			#if defined(TUNE)	
				
				T->rookOnSeventhRank[side]++;
			#endif
		}

		// Rook on Eight rank
		if ((1ULL << sq) & (side ? RANK_1 : RANK_8)) {

			score += weight_rook_on_eight_rank;

			#if defined(TUNE)	

				T->rookOnEightRank[side]++;							
			#endif
		} 	


		u64 attacksBB = th->evalInfo.rookAttacks[side][sq];


		// Connected Rooks
		if (attacksBB & rooksBB) {

			score += weight_rook_supporting_friendly_rook;

			#if defined(TUNE)	
				
				T->rookSupportingFriendlyRook[side] = 1;
			#endif
		}


		// Rook mobility
		u64 mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_rook_mobility[mobilityCount]; 

		#if defined(TUNE)	
			
			T->rookMobility[side][mobilityCount]++; 
		#endif


		// Update values for them King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[them]) {
 			
 			th->evalInfo.kingAttackersCount[them]++;
            th->evalInfo.kingAttackersWeight[them] += weight_rook_attack;

            #if defined(TUNE)	
            
            	T->rookAttack[them]++; 
            #endif

            u64 bb = attacksBB & th->evalInfo.kingAttacks[them];
            if (bb)	{
            	
           		th->evalInfo.kingAdjacentZoneAttacksCount[them] += POPCOUNT(bb);
            }
		}

		/*if (side) {

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



int32_t queenEval(u8 side, Thread *th) {

	u8 them = side ^ 1;

	int32_t score = 0;

	int sq = -1;
	int mobilityCount = 0;

	u64 attacksBB = 0ULL, mobilityBB = 0ULL; 	
	u64 queenBB = side ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];

	while (queenBB) {

		sq = GET_POSITION(queenBB);
		POP_POSITION(queenBB);

		assert(sq >= 0 && sq <= 63);


		attacksBB = th->evalInfo.queenAttacks[side][sq];


		#if defined(TUNE)
	
			T->queenPSQT[side][side ? Mirror64[sq] : sq] = 1; 		
		#endif

			
		if (	POPCOUNT(th->occupied) >= 22	// early game
			&&	(side ? sq <= 48 : sq >= 15)) { 

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


		// Queen mobility
		mobilityBB = attacksBB & th->empty;

		mobilityCount = POPCOUNT(mobilityBB);

		score += arr_weight_queen_mobility[mobilityCount];

		#if defined(TUNE)	
	
			T->queenMobility[side][mobilityCount]++;
		#endif


		// Update values for them King safety 
		if (attacksBB & th->evalInfo.kingZoneBB[them]) {
 			
 			th->evalInfo.kingAttackersCount[them]++;
            th->evalInfo.kingAttackersWeight[them] += weight_queen_attack;

            #if defined(TUNE)	
    
            	T->queenAttack[them]++; 
            #endif

            u64 bb = attacksBB & th->evalInfo.kingAttacks[them];
            if (bb)	{

           		th->evalInfo.kingAdjacentZoneAttacksCount[them] += POPCOUNT(bb);
            }
		}
	}	

	return score;
}


int32_t kingEval(u8 us, Thread *th) {
	
	const u8 them = us ^ 1;

	int kingSq = th->evalInfo.kingSq[us];
	
	int32_t score = 0;	
	
	assert(kingSq >= 0 && kingSq < 64);

	u64 ourPawns = us ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	u64 theirPawns = them ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	u64 pawnStormZone = th->evalInfo.kingZoneBB[us];

	pawnStormZone |= us ? pawnStormZone >> 8 : pawnStormZone << 8;

	score += POPCOUNT(th->evalInfo.kingZoneBB[us] & ourPawns) * weight_king_pawn_shield;
	score += POPCOUNT(pawnStormZone & theirPawns) * weight_king_enemy_pawn_storm;

	#if defined(TUNE)	

		T->kingPSQT[us][us ? Mirror64[kingSq] : kingSq] = 1; 			
		T->kingPawnShield[us] = POPCOUNT(th->evalInfo.kingZoneBB[us] & ourPawns);
		T->kingEnemyPawnStorm[us] = POPCOUNT(pawnStormZone & theirPawns);
	#endif


	if (	th->evalInfo.kingAttackersCount[us] >= 2
		&&	th->evalInfo.kingAdjacentZoneAttacksCount[us]) { 


		int32_t safetyScore = th->evalInfo.kingAttackersWeight[us];

        u64 kingUndefendedSquares = 
        		th->evalInfo.attacks[them]
        		&	th->evalInfo.kingAttacks[us]
        		&	~(	th->evalInfo.allPawnAttacks[us] |	
        				th->evalInfo.allKnightAttacks[us] |
        				th->evalInfo.allBishopAttacks[us] |
        				th->evalInfo.allRookAttacks[us] |
        				th->evalInfo.allQueenAttacks[us]);

		u64 enemyPieces = them ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

		
		int count;
		u64 b;

		b = kingUndefendedSquares & th->evalInfo.allQueenAttacks[them] & ~enemyPieces;
	     
		if (b) {
		
		    u64 attackedByOthers =
				th->evalInfo.allPawnAttacks[them] |
				th->evalInfo.allKnightAttacks[them] |
				th->evalInfo.allBishopAttacks[them] |
				th->evalInfo.allRookAttacks[them];

			b &= attackedByOthers;

			count = POPCOUNT(b);

	        safetyScore += weight_queen_safe_contact_check * count;

	        #if defined(TUNE)	

	        	T->queenSafeContactCheck[us] = count;
			#endif
		}


		b = kingUndefendedSquares & th->evalInfo.allRookAttacks[them] & ~enemyPieces;
	     
		if (b) {
		
		    u64 attackedByOthers =
				th->evalInfo.allPawnAttacks[them] |
				th->evalInfo.allKnightAttacks[them] |
				th->evalInfo.allBishopAttacks[them] |
				th->evalInfo.allQueenAttacks[them];

			b &= attackedByOthers;

			count = POPCOUNT(b);

	        safetyScore += weight_rook_safe_contact_check * count;
			
	        #if defined(TUNE)	

	        	T->rookSafeContactCheck[us] = count;
			#endif
		}
		

		u64 safe = ~(enemyPieces | th->evalInfo.attacks[us]);
		u64 b1 = Rmagic(kingSq, th->occupied) & safe;
		u64 b2 = Bmagic(kingSq, th->occupied) & safe;

		u64 queenSafeChecks = (b1 | b2) & th->evalInfo.allQueenAttacks[them];
		u64 rookSafeChecks = b1 & th->evalInfo.allRookAttacks[them];
		u64 bishopSafeChecks = b2 & th->evalInfo.allBishopAttacks[them];
		u64 knightSafeChecks = get_knight_attacks(kingSq) 
			& th->evalInfo.allKnightAttacks[them] & safe;
 
		safetyScore += weight_queen_check * POPCOUNT(queenSafeChecks);
		safetyScore += weight_rook_check * POPCOUNT(rookSafeChecks);
		safetyScore += weight_bishop_check * POPCOUNT(bishopSafeChecks);
		safetyScore += weight_knight_check * POPCOUNT(knightSafeChecks);
		safetyScore += weight_safety_adjustment;

		#if defined(TUNE)

			T->queenCheck[us] = POPCOUNT(queenSafeChecks);
			T->rookCheck[us] = POPCOUNT(rookSafeChecks);
			T->bishopCheck[us] = POPCOUNT(bishopSafeChecks);
			T->knightCheck[us] = POPCOUNT(knightSafeChecks);
			T->safetyAdjustment[us] = 1;
	    	T->safety[us] = safetyScore;
		#endif
	
 		int mg = ScoreMG(safetyScore), eg = ScoreEG(safetyScore);

        score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20); 
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















