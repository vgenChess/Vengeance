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

#define MIDGAME 1
#define ENDGAME 2

// for tuning
int TUNE = 0;

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

	A_FILE,
	B_FILE,
	C_FILE,
	D_FILE,
	E_FILE,
	F_FILE,
	G_FILE,
	H_FILE
};


int WHITE_PSQT[8][64];
int BLACK_PSQT[8][64];

EvalInfo *evalInfo = new EvalInfo();

void initEvalInfo(EvalInfo *info, Thread *th) {


	info->openFilesBB = openFiles(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);


	info->halfOpenFilesBB[WHITE] 
		= halfOpenOrOpenFile(th->whitePieceBB[PAWNS]) ^ info->openFilesBB;	
	info->halfOpenFilesBB[BLACK] 
		= halfOpenOrOpenFile(th->blackPieceBB[PAWNS]) ^ info->openFilesBB;


	info->pawnsAttacks[WHITE] = 
		wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);
	info->pawnsAttacks[BLACK] = 
		bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]);
	
	info->attacks[WHITE] |= info->pawnsAttacks[WHITE];
	info->attacks[BLACK] |= info->pawnsAttacks[BLACK];
	
	int sq = -1;
	u64 b = 0ULL, attacks = 0ULL;
	for (int p = KNIGHTS; p <= KING; p++) {

		for (int side = WHITE; side <= BLACK; side++) {

			b = side ? th->blackPieceBB[p] : th->whitePieceBB[p];
			attacks = 0ULL;
			while (b) {

				sq = __builtin_ctzll(b);
				b &= b - 1;

				assert(sq >= 0 && sq <= 63);

				if (p == KNIGHTS) attacks |= get_knight_attacks(sq);
				else if (p == BISHOPS) attacks |= Bmagic(sq, th->occupied);
				else if (p == ROOKS)   attacks |= Rmagic(sq, th->occupied);
				else if (p == QUEEN)   attacks |= Qmagic(sq, th->occupied);
				else if (p == KING)    attacks |= get_king_attacks(sq);
			}	

			if (p == KNIGHTS) info->knightsAttacks[side] = attacks;
			else if (p == BISHOPS) info->bishopsAttacks[side] = attacks;
			else if (p == ROOKS)   info->rooksAttacks[side] = attacks;
			else if (p == QUEEN)   info->queensAttacks[side] = attacks;
			else if (p == KING)    info->kingAttacks[side] = attacks;
		
			info->attacks[side] |= attacks;
		}
	}

	info->kingSq[WHITE] = __builtin_ctzll(th->whitePieceBB[KING]);	
	info->kingSq[BLACK] = __builtin_ctzll(th->blackPieceBB[KING]);	

	
	info->kingZoneBB[WHITE] = info->kingAttacks[WHITE] | (info->kingAttacks[WHITE] >> 8);
	info->kingZoneBB[BLACK] = info->kingAttacks[BLACK] | (info->kingAttacks[BLACK] << 8);


	info->kingAttackersCount[WHITE] = 0;
    info->kingAttackersCount[BLACK] = 0;
    
    info->kingAttackersWeight[WHITE] = 0;
	info->kingAttackersWeight[BLACK] = 0;

  	info->kingAdjacentZoneAttacksCount[WHITE] = 0;
  	info->kingAdjacentZoneAttacksCount[BLACK] = 0;	
}

int traceFullEval(TraceCoefficients *traceCoefficients, u8 sideToMove, Thread *th) {

	T = traceCoefficients;

	return fullEval(sideToMove, th);
}

int fullEval(u8 sideToMove, Thread *th) {

	int hashedEval;
	if (!TUNE && probeEval(&hashedEval, th)) {

		return sideToMove == WHITE ? hashedEval : -hashedEval;
	}	
	

	int nWhitePawns = __builtin_popcountll(th->whitePieceBB[PAWNS]);
	int nWhiteKnights = __builtin_popcountll(th->whitePieceBB[KNIGHTS]);
	int nWhiteBishops = __builtin_popcountll(th->whitePieceBB[BISHOPS]);
	int nWhiteRooks = __builtin_popcountll(th->whitePieceBB[ROOKS]);
	int nWhiteQueen = __builtin_popcountll(th->whitePieceBB[QUEEN]);

	int nBlackPawns = __builtin_popcountll(th->blackPieceBB[PAWNS]);
	int nBlackKnights = __builtin_popcountll(th->blackPieceBB[KNIGHTS]);
	int nBlackBishops = __builtin_popcountll(th->blackPieceBB[BISHOPS]);
	int nBlackRooks = __builtin_popcountll(th->blackPieceBB[ROOKS]);
	int nBlackQueen = __builtin_popcountll(th->blackPieceBB[QUEEN]);

	if (TUNE) {	
		
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
	}

	
	evalInfo->clear();
	initEvalInfo(evalInfo, th);

	int eval = evaluateSide(WHITE, evalInfo, th) - evaluateSide(BLACK, evalInfo, th);

	if (TUNE)	T->eval = eval;
	

	// Tapered evaluation 

	int phase = 4 * (nWhiteQueen + nBlackQueen) 
		+ 2 * (nWhiteRooks + nBlackRooks)
      	+ 1 * (nWhiteKnights + nBlackKnights)
      	+ 1 * (nWhiteBishops + nBlackBishops);

	if (TUNE)	T->phase = phase;
	
	
    int score = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;

    if (!TUNE)	recordEval(score, th);

	return sideToMove == WHITE ? score : -score;
}

int evaluateSide(int side, EvalInfo *evalInfo, Thread *th) {

	int score = 0;

	if (TUNE) {

		score += pawnsEval(side, evalInfo, th);
	} else {

		int pawnsScore;
		if (!probePawnHash(&pawnsScore, th)) { // TODO check pawn hash logic

			pawnsScore = pawnsEval(side, evalInfo, th);
			recordPawnHash(pawnsScore, th);
		}

		score += pawnsScore; 
	}

	score += PSQTScore(side, th) 
		+ knightsEval(side, evalInfo, th) 
		+ bishopsEval(side, evalInfo, th) 
		+ rooksEval(side, evalInfo, th) 
		+ queenEval(side, evalInfo, th) 
		+ kingEval(side, evalInfo, th); 
		// + evalBoard(side, th, evalInfo); 
	
	return score;
}


int pawnsEval(u8 sideToMove, EvalInfo *evalInfo, Thread *th) {

	int score = 0;
	int sq = -1, rank = -1;

	if (TUNE) {
		
		u64 bitboard = sideToMove ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
		while (bitboard) {

			int sq = __builtin_ctzll(bitboard);
			bitboard &= bitboard - 1; 

			T->pawnPSQT[sideToMove][sideToMove ? Mirror64[sq] : sq] = 1; 
		}
	}


	const int nIsolatedPawns = isolatedPawns(sideToMove, th);
	score += nIsolatedPawns * weight_isolated_pawn;

	if (TUNE)	T->isolatedPawns[sideToMove] = nIsolatedPawns;


	const int nDoublePawns = numOfDoublePawns(sideToMove, th);
	score += nDoublePawns * weight_double_pawn;

	if (TUNE)	T->doublePawns[sideToMove] = nDoublePawns;
			

	const int nBackwardPawns = countBackWardPawns(sideToMove, th);
	score += nBackwardPawns * weight_backward_pawn;

	if (TUNE)	T->backwardPawns[sideToMove] = nBackwardPawns;


	u64 defendedPawns = sideToMove ? 
		th->blackPieceBB[PAWNS] & evalInfo->pawnsAttacks[BLACK]
		: th->whitePieceBB[PAWNS] & evalInfo->pawnsAttacks[WHITE];

	int nDefendedPawns = __builtin_popcountll(defendedPawns);
	score += nDefendedPawns * weight_defended_pawn;

	if (TUNE)	T->defendedPawns[sideToMove] = nDefendedPawns;


	const int nPawnHoles = numOfPawnHoles(sideToMove, th);
	score += nPawnHoles * weight_pawn_hole;

	if (TUNE)	T->pawnHoles[sideToMove] = nPawnHoles;



	u64 passedPawns = sideToMove ? bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])
		: wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]);
	while (passedPawns) {
		
		sq = __builtin_ctzll(passedPawns);
		passedPawns &= passedPawns - 1;

		assert(sq >= 0 && sq <= 63);

		rank = sideToMove ? Mirror64[sq] >> 3 : sq >> 3; // = sq / 8

		assert(rank+1 >= 1 && rank+1 <= 8);
	

		if ((1ULL << sq) & evalInfo->pawnsAttacks[sideToMove]) {

			score += arr_weight_defended_passed_pawn[rank];
			
			if (TUNE)	T->defendedPassedPawn[sideToMove][rank]++;
		} else {
			
			score += arr_weight_passed_pawn[rank];

			if (TUNE)	T->passedPawn[sideToMove][rank]++;
		}		
	}

	return score;
}



int knightsEval(u8 side, EvalInfo *evalInfo, Thread *th) {

	const u8 opponent = side ^ 1;

	int score = 0;
	int mobilityCount = 0;

	int sq = -1, rank = -1; 

	u64 knightsBB = side ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];

	while (knightsBB) {

		sq = __builtin_ctzll(knightsBB);
		knightsBB &= knightsBB - 1;

		assert(sq >= 0 && sq <= 63);

		u64 attacksBB = get_knight_attacks(sq);

		if (attacksBB & evalInfo->kingZoneBB[opponent]) {
 			
 			evalInfo->kingAttackersCount[opponent]++;
            evalInfo->kingAttackersWeight[opponent] += weight_knight_attack;

            if (TUNE)	T->knightAttack[opponent]++; 

            u64 bb = (attacksBB & evalInfo->kingAttacks[opponent]);
            if (bb)	
           		evalInfo->kingAdjacentZoneAttacksCount[opponent] += __builtin_popcountll(bb);
		}

		// undefended
		if (!((1ULL << sq) & evalInfo->attacks[side])) {

			score += weight_undefended_knight;

			if (TUNE)	T->undefendedKnight[side]++;			
		}

		// defended by pawn
		if ((1ULL << sq) & evalInfo->pawnsAttacks[side]) {

			score += weight_knight_defended_by_pawn;

			if (TUNE)	T->knightDefendedByPawn[side]++;
		}


		u64 mobilityBB = attacksBB 
			& ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]) 
			& ~(evalInfo->pawnsAttacks[opponent]);

		mobilityCount = __builtin_popcountll(mobilityBB);

		score += arr_weight_knight_mobility[mobilityCount];

		if (TUNE) {

			T->knightPSQT[side][side ? Mirror64[sq] : sq] = 1; 
			T->knightMobility[side][mobilityCount]++;
		}
	}

	return score;
}






int bishopsEval(u8 side, EvalInfo *evalInfo, Thread *th) {

	const u8 opponent = side ^ 1;

	int score = 0;
	int mobilityCount = 0;
	int sq = -1;

	u64 bishopBB = side ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
	
	const int nBishops = __builtin_popcountll(bishopBB);
	
	if (nBishops == 2) {	// This should be equal to 2 and not greater or equal to 
							// since we want to capture the weight for exactly 2 bishops

		score += weight_bishop_pair;

		if (TUNE)	T->bishopPair[side]++;
	}


	while (bishopBB) {
	
		sq = __builtin_ctzll(bishopBB);

		bishopBB &= bishopBB - 1;


		assert(sq >= 0 && sq <= 63);


		u64 attacksBB = Bmagic(sq, th->occupied);

		if (attacksBB & evalInfo->kingZoneBB[opponent]) {
 			
 			evalInfo->kingAttackersCount[opponent]++;
            evalInfo->kingAttackersWeight[opponent] += weight_bishop_attack;

            if (TUNE)	T->bishopAttack[opponent]++; 

            u64 bb = (attacksBB & evalInfo->kingAttacks[opponent]);
            if (bb)	
           		evalInfo->kingAdjacentZoneAttacksCount[opponent] += __builtin_popcountll(bb);
		}
	

		if (!((1ULL << sq) & evalInfo->attacks[side])) {

			score += weight_undefended_bishop;

			if (TUNE)	T->undefendedBishop[side]++;			
		}

		

		int count = __builtin_popcountll(evalInfo->bishopsAttacks[side] 
			& (side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]));
		
		if (count > 0) {

			score += count * weight_bad_bishop;

			if (TUNE)	T->badBishop[side] = count;
		}



		u64 mobilityBB = attacksBB & ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = __builtin_popcountll(mobilityBB);


		score += arr_weight_bishop_mobility[mobilityCount];


		if (TUNE) {
			
			T->bishopPSQT[side][side ? Mirror64[sq] : sq] = 1; 	

			T->bishopMobility[side][mobilityCount]++;
		}
	}
	
	return score;
}



/** TODO
Increasing value as pawns disappear 
Tarrasch Rule
*/


int rooksEval(u8 side, EvalInfo *evalInfo, Thread *th) {
	
	const u8 opponent = side ^ 1;

	int score = 0;
	int sq = -1;

	int mobilityCount = 0;

	const int kingSq = evalInfo->kingSq[side];


	u64 rooksBB = side ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];

	while (rooksBB) {

		sq = __builtin_ctzll(rooksBB);
		rooksBB &= rooksBB - 1;

		assert(sq >= 0 && sq <= 63);


		if (TUNE)	T->rookPSQT[side][side ? Mirror64[sq] : sq] = 1; 


		if ((1ULL << sq) & evalInfo->halfOpenFilesBB[side]) {

			score += weight_rook_half_open_file;
		
			if (TUNE)	T->halfOpenFile[side]++;
		}


		if ((1ULL << sq) & evalInfo->openFilesBB) {

			score += weight_rook_open_file;

			if (TUNE)	T->openFile[side]++;
		}


		if (arrFiles[sq & 7] & 
			(opponent ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

			score += weight_rook_enemy_queen_same_file;

			if (TUNE)	T->rookEnemyQueenSameFile[side]++;
		}


		u64 attacksBB = Rmagic(sq, th->occupied);

		if (attacksBB & evalInfo->kingZoneBB[opponent]) {
 			
 			evalInfo->kingAttackersCount[opponent]++;
            evalInfo->kingAttackersWeight[opponent] += weight_rook_attack;

            if (TUNE)	T->rookAttack[opponent]++; 

            u64 bb = (attacksBB & evalInfo->kingAttacks[opponent]);
            if (bb)	
           		evalInfo->kingAdjacentZoneAttacksCount[opponent] += __builtin_popcountll(bb);
		}
	

		u64 mobilityBB = attacksBB & 
			~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = __builtin_popcountll(mobilityBB);

		score += arr_weight_rook_mobility[mobilityCount];

		if (TUNE)	T->rookMobility[side][mobilityCount]++;
		

		if (attacksBB & rooksBB) {

			score += weight_rook_supporting_friendly_rook;

			if (TUNE)	T->rookSupportingFriendlyRook[side] = 1;
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
			
				if (TUNE)	T->rookOnSeventhRank[BLACK]++;					
			} 	

			// Rook on Eight rank (Rank 1 for black)
			if ((1ULL << sq) & RANK_1) {

				score += weight_rook_on_eight_rank;

				if (TUNE)	T->rookOnEightRank[BLACK]++;					
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
			
				if (TUNE)	T->rookOnSeventhRank[WHITE]++;					
			} 	


			// Rook on Eight rank (Rank 1 for black)
			if ((1ULL << sq) & RANK_8) {

				score += weight_rook_on_eight_rank;

				if (TUNE)	T->rookOnEightRank[WHITE]++;					
			} 
		}
	}

	return score;
}




int queenEval(u8 side, EvalInfo *evalInfo, Thread *th) {

	const u8 opponent = side ^ 1;

	int score = 0;

	int sq = -1;
	int mobilityCount = 0;

	const int pieceCount = 
		__builtin_popcountll(th->whitePieceBB[PIECES] | th->blackPieceBB[PIECES]);

	u64 attacksBB = 0ULL, mobilityBB = 0ULL; 	
	u64 queenBB = side ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];

	while (queenBB) {

		sq = __builtin_ctzll(queenBB);
		queenBB &= queenBB - 1;

		assert(sq >= 0 && sq <= 63);

		if (TUNE)	T->queenPSQT[side][side ? Mirror64[sq] : sq] = 1; 		
	
	
		if (pieceCount > 20 && side ? sq <= 55 : sq >= 8) { // recheck logic

			u64 minorPiecesBB = side ? 
				th->blackPieceBB[ROOKS] | th->blackPieceBB[KNIGHTS] | th->blackPieceBB[BISHOPS] :
				th->whitePieceBB[ROOKS] | th->whitePieceBB[KNIGHTS] | th->whitePieceBB[BISHOPS];

			u64 underdevelopedPiecesBB = (side ? RANK_8 : RANK_1) & minorPiecesBB; 

			int count = __builtin_popcountll(underdevelopedPiecesBB);
			
			score += count * weight_queen_underdeveloped_pieces;
			
			if (TUNE)	T->queenUnderdevelopedPieces[side] += count;
		}


		attacksBB = Qmagic(sq, th->occupied);

		if (attacksBB & evalInfo->kingZoneBB[opponent]) {
 			
 			evalInfo->kingAttackersCount[opponent]++;
            evalInfo->kingAttackersWeight[opponent] += weight_queen_attack;

            if (TUNE)	T->queenAttack[opponent]++; 

            u64 bb = (attacksBB & evalInfo->kingAttacks[opponent]);
            if (bb)	
           		evalInfo->kingAdjacentZoneAttacksCount[opponent] += __builtin_popcountll(bb);
		}


		mobilityBB = attacksBB & ~(side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

		mobilityCount = __builtin_popcountll(mobilityBB);


		score += arr_weight_queen_mobility[mobilityCount];


		if (TUNE)	T->queenMobility[side][mobilityCount]++;
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

// 	const u8 opponent = us ^ 1;
// 	u64 enemyPieceBB = 0ULL, attacks = 0ULL;

// 	u64 kingBB = us ? th->blackPieceBB[KING] : th->whitePieceBB[KING];
// 	u64 friendlyPawns = us ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
// 	u64 enemyPawns = opponent ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];


// 	const int kingSq = evalInfo->kingSq[us];
// 	const u64 shieldBB = evalInfo->shieldBB[us];

// 	if (TUNE)	T->kingPSQT[us][us ? Mirror64[kingSq] : kingSq] = 1; 			

// 	// TODO logic needs rechecking (many pawns or just three pawns)
// 	const int nPawnsShield = __builtin_popcountll(shieldBB & friendlyPawns);
// 	score += nPawnsShield * weight_king_pawn_shield;

// 	// check for enemy pawns storm
// 	// add another rank resulting in 3x3 area above king for enemy pawns storm
// 	u64 enemyPawnStormArea = shieldBB | (us ? shieldBB >> 8 : shieldBB << 8);
	
// 	const int nEnemyPawnsStorm = __builtin_popcountll(enemyPawnStormArea & enemyPawns);
// 	score += nEnemyPawnsStorm * weight_king_enemy_pawn_storm;

// 	if (TUNE) {

// 		T->kingPawnShield[us] = nPawnsShield;
// 		T->kingEnemyPawnStorm[us] = nEnemyPawnsStorm;
// 	}


// 	if (evalInfo->kingAttackersCount[opponent] >= 2
// 		&& evalInfo->kingAdjacentZoneAttacksCount[opponent]) {

// 		u64 kingUndefendedSquares  =
// 	        evalInfo->attacks[opponent] & ~evalInfo->pawnsAttacks[us]
// 	        & ~evalInfo->knightsAttacks[us] & ~evalInfo->bishopsAttacks[us]
// 	        & ~evalInfo->rooksAttacks[us] & ~evalInfo->queensAttacks[us]
// 	        & evalInfo->kingAttacks[us];

// 	   	attackCounter =  std::min(25, (evalInfo->kingAttackersCount[opponent] * evalInfo->kingAttackersWeight[opponent]) / 2)
//                  + 3 * (evalInfo->kingAdjacentZoneAttacksCount[opponent] + count_1s_max_15(kingUndefendedSquares));
               

// 		u64 enemyPieces = opponent ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

// 		u64 b, b2;

// 		b = kingUndefendedSquares & evalInfo->queensAttacks[opponent] & ~enemyPieces;
	     
// 		if(b) {
		
// 		    u64 attackedByOthers =
// 				evalInfo->pawnsAttacks[opponent] |
// 				evalInfo->knightsAttacks[opponent] |
// 				evalInfo->bishopsAttacks[opponent] |
// 				evalInfo->rooksAttacks[opponent];

// 			b &= attackedByOthers;

// 			int count = count_1s_max_15(b);

// 	        attackCounter += weight_queen_safe_contact_check_counter * count;
// 		}


// 		b = kingUndefendedSquares & evalInfo->rooksAttacks[opponent] & ~enemyPieces;
	     
// 		if(b) {
		
// 		    u64 attackedByOthers =
// 				evalInfo->pawnsAttacks[opponent] |
// 				evalInfo->knightsAttacks[opponent] |
// 				evalInfo->bishopsAttacks[opponent] |
// 				evalInfo->rooksAttacks[opponent];

// 			b &= attackedByOthers;

// 			int count = count_1s_max_15(b);

// 	        attackCounter += weight_rook_safe_contact_check_counter * count;
// 		}



// 		b = Rmagic(kingSq, th->occupied) & ~enemyPieces & ~evalInfo->attacks[us];
// 		// Queen checks
// 		b2 = b & evalInfo->queensAttacks[opponent];
// 		if(b2) attackCounter += weight_queen_check_counter * count_1s_max_15(b2);

// 		// Rook checks
// 		b2 = b & evalInfo->rooksAttacks[opponent];
// 		if(b2) attackCounter += weight_rook_check_counter * count_1s_max_15(b2);


// 	    b = Bmagic(kingSq, th->occupied) & ~enemyPieces & ~evalInfo->attacks[us];
// 	    // Queen checks
// 		b2 = b & evalInfo->queensAttacks[opponent];
// 	    if(b2) attackCounter += weight_queen_check_counter * count_1s_max_15(b2);

// 	    // Bishop checks
// 	    b2 = b & evalInfo->bishopsAttacks[opponent];
// 	    if(b2) attackCounter += weight_bishop_check_counter * count_1s_max_15(b2);



// 	    b = get_knight_attacks(kingSq) & ~enemyPieces & ~evalInfo->attacks[us];
// 	    // Knight checks
// 	    b2 = b & evalInfo->knightsAttacks[opponent];
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


int kingEval(u8 us, EvalInfo *evalInfo, Thread *th) {
	
	int sq = -1;												    	
	int score = 0;
	int attackCounter = 0;		
	int nAttacks = 0;
	int attackers = 0;

	const u8 opponent = us ^ 1;
	u64 enemyPieceBB = 0ULL, attacks = 0ULL;

	u64 friendlyPawns = us ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];
	u64 enemyPawns = opponent ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];


	const int kingSq = evalInfo->kingSq[us];
	
	if (TUNE)	T->kingPSQT[us][us ? Mirror64[kingSq] : kingSq] = 1; 			


	const int nPawnsShield = __builtin_popcountll(evalInfo->kingZoneBB[us] & friendlyPawns);
	score += nPawnsShield * weight_king_pawn_shield;

	if (TUNE) 	T->kingPawnShield[us] = nPawnsShield;
	

	const u64 enemyPawnStormArea = evalInfo->kingZoneBB[us] | 
		(us ? evalInfo->kingZoneBB[us] >> 8 : evalInfo->kingZoneBB[us] << 8);

	const int nEnemyPawnsStorm = __builtin_popcountll(enemyPawnStormArea & enemyPawns);
	score += nEnemyPawnsStorm * weight_king_enemy_pawn_storm;

	if (TUNE)	T->kingEnemyPawnStorm[us] = nEnemyPawnsStorm;
	

	const int queenCount = __builtin_popcountll(opponent ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN]);

	if (queenCount >= 1 
		&& evalInfo->kingAttackersCount[us] >= 2
		&& evalInfo->kingAdjacentZoneAttacksCount[us]) { // TODO recheck logic for if check

		u64 kingUndefendedSquares  =
	        evalInfo->attacks[opponent] & ~evalInfo->pawnsAttacks[us]
	        & ~evalInfo->knightsAttacks[us] & ~evalInfo->bishopsAttacks[us]
	        & ~evalInfo->rooksAttacks[us] & ~evalInfo->queensAttacks[us]
	        & evalInfo->kingAttacks[us];

		u64 enemyPieces = opponent ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES];

		int safetyScore = evalInfo->kingAttackersWeight[us];

		int count;
		u64 b, b2;

		b = kingUndefendedSquares & evalInfo->queensAttacks[opponent] & ~enemyPieces;
	     
		if(b) {
		
		    u64 attackedByOthers =
				evalInfo->pawnsAttacks[opponent] |
				evalInfo->knightsAttacks[opponent] |
				evalInfo->bishopsAttacks[opponent] |
				evalInfo->rooksAttacks[opponent];

			b &= attackedByOthers;

			count = __builtin_popcountll(b);

	        safetyScore += weight_queen_safe_contact_check * count;

	        if (TUNE)	T->queenSafeContactCheck[us] = count;
		}


		b = kingUndefendedSquares & evalInfo->rooksAttacks[opponent] & ~enemyPieces;
	     
		if(b) {
		
		    u64 attackedByOthers =
				evalInfo->pawnsAttacks[opponent] |
				evalInfo->knightsAttacks[opponent] |
				evalInfo->bishopsAttacks[opponent] |
				evalInfo->rooksAttacks[opponent];

			b &= attackedByOthers;

			count = __builtin_popcountll(b);

	        safetyScore += weight_rook_safe_contact_check * count;
			
	        if (TUNE)	T->rookSafeContactCheck[us] = count;
		}
		

		b = Qmagic(kingSq, th->occupied) & ~enemyPieces & ~evalInfo->attacks[us];

		b2 = b & evalInfo->queensAttacks[opponent];
		if(b2) {
		
			count = __builtin_popcountll(b);
			
			safetyScore += weight_queen_check * count;
			
			if (TUNE)	T->queenCheck[us] = count;
		}


		b = Rmagic(kingSq, th->occupied) & ~enemyPieces & ~evalInfo->attacks[us];
		
		// Rook checks
		b2 = b & evalInfo->rooksAttacks[opponent];
		if(b2) {
			
			count = __builtin_popcountll(b);
	
			safetyScore += weight_rook_check * count;

			if (TUNE)	T->rookCheck[us] = count;
		}


	    b = Bmagic(kingSq, th->occupied) & ~enemyPieces & ~evalInfo->attacks[us];

	    // Bishop checks
	    b2 = b & evalInfo->bishopsAttacks[opponent];
	    if(b2) {

			count = __builtin_popcountll(b);

		  	safetyScore += weight_bishop_check * count;

		  	if (TUNE)	T->bishopCheck[us] = count;
	    } 

	  
	    b = get_knight_attacks(kingSq) & ~enemyPieces & ~evalInfo->attacks[us];
	    // Knight checks
	    b2 = b & evalInfo->knightsAttacks[opponent];
	    if(b2) {

			count = __builtin_popcountll(b);

	    	safetyScore += weight_knight_check * count;
	  		
	  		if (TUNE)	T->knightCheck[us] = count;
	    } 


	    safetyScore += weight_safety_adjustment;

	    if (TUNE)	T->safetyAdjustment[us] = 1;


	
		int mg = ScoreMG(safetyScore);
		int eg = ScoreEG(safetyScore);
		
	    score += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20);  
	

		if (TUNE)	T->safety[us] = safetyScore;
	} 
    else if (TUNE) {

    	T->knightAttack[us] = 0;
		T->bishopAttack[us] = 0;
		T->rookAttack[us] = 0;
		T->queenAttack[us] = 0;
    }



	return score;
}




int PSQTScore(u8 sideToMove, Thread *th) {
	
	int score = 0;

	for (int piece = PAWNS; piece <= KING; piece++) {

		u64 bitboard = sideToMove ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
		while (bitboard) {

			const int sq = __builtin_ctzll(bitboard); 
			bitboard &= bitboard - 1; 

			score += sideToMove ? BLACK_PSQT[piece][sq] : WHITE_PSQT[piece][sq];
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


int evalBoard(u8 side, Thread *th, EvalInfo *evalInfo) {

	int score = 0;

	int nCenterControl = __builtin_popcountll(CENTER & evalInfo->attacks[side]);

	score += nCenterControl * weight_center_control;

	if (TUNE)	T->centerControl[side] = nCenterControl;

	return score;
}


int numOfPawnHoles(u8 side, Thread *th) {

	u64 pawnsBB = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	u64 frontAttackSpans = side ? 
		bWestAttackFrontSpans(pawnsBB) | bEastAttackFrontSpans(pawnsBB)
		: wWestAttackFrontSpans(pawnsBB) | wEastAttackFrontSpans(pawnsBB); 

	u64 holes = ~frontAttackSpans & EXTENDED_CENTER
		& (side ? (RANK_5 | RANK_6) : (RANK_3 | RANK_4)); 

	return __builtin_popcountll(holes);
}

	
int isolatedPawns(u8 side, Thread *th) {

	return __builtin_popcountll(isolanis(side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS]));
}

int numOfDoublePawns(u8 side, Thread *th) {

	u64 pawnsBB = side ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS];

	u64 doublePawnsBB = side ? bPawnsInfrontOwn(pawnsBB) : wPawnsInfrontOwn(pawnsBB); 

	return __builtin_popcountll(doublePawnsBB);
}

int countBackWardPawns(u8 side, Thread *th) {

	return side ? __builtin_popcountll(bBackward(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		__builtin_popcountll(wBackward(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countPassedPawns(u8 side, Thread *th) {

	return side ? __builtin_popcountll(bPassedPawns(th->blackPieceBB[PAWNS], th->whitePieceBB[PAWNS])) : 
		__builtin_popcountll(wPassedPawns(th->whitePieceBB[PAWNS], th->blackPieceBB[PAWNS]));
}

int countDefendedPawns(u8 side, Thread *th) {

	return side ? __builtin_popcountll(bPawnDefendedFromWest(th->blackPieceBB[PAWNS])) + __builtin_popcountll(bPawnDefendedFromEast(th->blackPieceBB[PAWNS])) : 
		__builtin_popcountll(wPawnDefendedFromWest(th->whitePieceBB[PAWNS])) + __builtin_popcountll(wPawnDefendedFromEast(th->whitePieceBB[PAWNS]));
}

// pawns with at least one pawn in front on the same file
u64 wPawnsBehindOwn(u64 wpawns) {return wpawns & wRearSpans(wpawns);}

// pawns with at least one pawn in front on the same file
u64 bPawnsBehindOwn(u64 bpawns) {return bpawns & bRearSpans(bpawns);}

// pawns with at least one pawn behind on the same file
u64 wPawnsInfrontOwn (u64 wpawns) {return wpawns & wFrontSpans(wpawns);}

// pawns with at least one pawn behind on the same file
u64 bPawnsInfrontOwn (u64 bpawns) {return bpawns & bFrontSpans(bpawns);}

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

u64 halfOpenOrOpenFile(u64 gen) {return ~fileFill(gen);}


/**
* Evaluation subroutine suitable for chess engines.
* -------------------------------------------------
* Piece codes are
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
* Squares are
*     A1=0, B1=1 ... H8=63
* Input format:
*     piece[0] is white king, square[0] is its location
*     piece[1] is black king, square[1] is its location
*     ..
*     piece[x], square[x] can be in any order
*     ..
*     piece[n+1] is set to 0 to represent end of array
* Returns
*   Score relative to side to move in approximate centi-pawns
*/


int vgenPieceCodes[16] = {0, KING, QUEEN, ROOKS, BISHOPS, KNIGHTS, PAWNS,
		KING, QUEEN, ROOKS, BISHOPS, KNIGHTS, PAWNS, 0, 0, 0};

/**
 * Evaluate current position on board according to pieces value and pieces position according to the NNUE
 * @return score of the current board
 */
int evaluateNNUE(u8 side, Thread *th) {

    u64 bitboard;

    int piece, square;

    // arrays and fields for stockfish nnue evaluation
    int pieces[33];
    int squares[33];
    int index = 2;

    for (int pieceType = 1; pieceType <= 12; pieceType++) {
        
        // init piece bitboard copy
        bitboard = pieceType <= 6 ? th->whitePieceBB[vgenPieceCodes[pieceType]]
        	: th->blackPieceBB[vgenPieceCodes[pieceType]];

        while (bitboard) {
            // getting piece and location
            piece = pieceType;
            square = __builtin_ctzll(bitboard);
    		// removing piece
            bitboard &= bitboard - 1;
        
            if (piece == 1) {
                // handeling white king nnue
                pieces[0] = piece;
                squares[0] = square;
            } else if (piece == 7) {
                // handeling black king nnue
                pieces[1] = piece;
                squares[1] = square;
            } else {
                // converting rest of the pieces to converted squares for nnue evaluation
                pieces[index] = piece;
                squares[index] = square;
                index++;
            }
        }
    }

    // set final zero on both nnue array
    pieces[index] = 0;
    squares[index] = 0;

    // evaluating with NNUE
    return evaluateNNUE(side, pieces, squares);
}
















