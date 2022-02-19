//
//  search.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <future>
#include <atomic>
#include <ostream>

#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "make_unmake.h"
#include "utility.h"
#include "vtime.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "hash.h"
#include "thread.h"
#include "see.h"
#include "constants.h"
#include "cerebrum.h"
#include "history.h"
#include "ucireport.h"

bool timeSet, stopped;

int option_thread_count;

std::chrono::steady_clock::time_point startTime, stopTime;

int timePerMove;
int MAX_TIME_PER_SEARCH = 0;
int MIN_TIME_PER_SEARCH = 0;

int seeVal[8] = { 0, VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP, VALUE_ROOK, VALUE_QUEEN, VALUE_KING, 0};

std::mutex mtx;

int MAX_DEPTH = 100;
bool ABORT_SEARCH;

void startSearch(u8 sideToMove) {


	ABORT_SEARCH = false;

	Threads.start_searching(); // start non-main threads
	searchMain(sideToMove, Threads.main());          // main thread start searching

	ABORT_SEARCH = true;


	// When we reach the maximum depth, we can arrive here without a raise of
	// Threads.stop. However, if we are pondering or in an infinite search,
	// the UCI protocol states that we shouldn't print the best move before the
	// GUI sends a "stop" or "ponderhit" command. We therefore simply wait here
	// until the GUI sends one of those commands.

	while (!Threads.stop)
	{} // Busy wait for a stop or a ponder reset

	// Stop the threads if not already stopped (also raise the stop if
	// "ponderhit" just reset Threads.ponder).
	Threads.stop = true;

	// Wait until all threads have finished
	Threads.wait_for_search_finished();


	reportBestMove();

	
	timeSet = false;
	stopped = false;
}


int maxTimePerMove = 0, minTimePerMove = 0; 
int stableMoveCount = 0;

void searchMain(int sideToMove, SearchThread *th) {
	
	th->nodes = 0;
	th->ttHits = 0;	

	th->depth = VALI16_NO_DEPTH;
	th->completedDepth = VALI16_NO_DEPTH;

	stableMoveCount = 0;

	maxTimePerMove = 4 * timePerMove;
	minTimePerMove = timePerMove / 4;

	for (int depth = 1; depth < MAX_DEPTH; depth++) {

		if (th != Threads.main()) { 

			// Mutex will automatically be unlocked when lck goes out of scope
			std::lock_guard<std::mutex> lck {mtx}; 

			uint8_t count = 0;

			for (SearchThread *thread : Threads) {

				if (th != thread && depth == thread->depth) {
				
					count++;	
	            }
			}

			if (count >= (Threads.size() / 2)) {

				continue;
			}
		} 


		th->depth = depth;


		aspirationWindowSearch(sideToMove, th);


		if (Threads.stop)
			break;


		if (th != Threads.main())
			continue;


 		if (timeSet && th->completedDepth >= 4) {


		    int scoreDiff = th->pvLine.at(th->completedDepth-3).score - th->pvLine.at(th->completedDepth).score;
		    
		    float scoreChangeFactor = fmax(0.5, fmin(1.5, 0.1 + scoreDiff * 0.05));


		    assert(th->pvLine.at(th->completedDepth).line.size() > 0 && th->pvLine.at(th->completedDepth-1).line.size() > 0);

		    if (th->pvLine.at(th->completedDepth).line.at(0) == th->pvLine.at(th->completedDepth-1).line.at(0)) {
				
				stableMoveCount = std::min(10, stableMoveCount + 1);
		    }
			else {
				
				stableMoveCount = 0;
			}

		    float stableMoveFactor = 1.25 - stableMoveCount * 0.05;


		    // Check for time 

		    std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
		    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

	    	if (timeSpent > timePerMove * scoreChangeFactor * stableMoveFactor) {

				Threads.stop = true;
				break;	
			}
		}
	} 
}


void aspirationWindowSearch(u8 sideToMove, SearchThread *th) {

	int32_t window = VALI32_MATE;

	int32_t score = -VALI32_MATE;
	int32_t alpha = -VALI32_MATE, beta = VALI32_MATE;
	
	if (th->depth > 4 && th->completedDepth > 0) {

		window = VALUI8_AP_WINDOW;

		int scoreKnown = th->pvLine.at(th->completedDepth).score;

  		alpha = std::max(-VALI32_MATE, scoreKnown - window);
	  	beta  = std::min( VALI32_MATE, scoreKnown + window);	
	}
 	

	std::vector<u32> pline;


	SearchInfo searchInfo;

	searchInfo.side = sideToMove;
	searchInfo.ply = 0;
	searchInfo.depth = th->depth;
	searchInfo.realDepth = th->depth;
	searchInfo.isNullMoveAllowed = false;


	while (true) {

		pline.clear();
		th->selDepth = VALI16_NO_DEPTH;
		
		score = alphabetaSearch(alpha, beta, th, &pline, &searchInfo, VALI32_MATE);

		if (Threads.stop)
        	break;
		
		if (score <= alpha)	{

			beta = (alpha + beta) / 2;
			alpha = std::max(alpha - window, -VALI32_MATE);
		}
		else if (score >= beta)	{

			beta = std::min(beta + window, VALI32_MATE);
		}	
		else {

			th->completedDepth = th->depth;

			th->pvLine.at(th->completedDepth).score = score;

			th->pvLine.at(th->completedDepth).line.clear();
			std::copy(pline.begin(), pline.end(), back_inserter(th->pvLine.at(th->completedDepth).line));

			break;
		}

		window = window + window / 2; 
	}

	
	if (Threads.stop)
    	return;


	assert (score > alpha && score < beta);


	if (th != Threads.main()) 
		return;


	display(sideToMove, th->completedDepth, th->selDepth,
		th->pvLine[th->completedDepth].score, th->pvLine[th->completedDepth].line);

	if (!th->canReportCurrMove) {

		int interval = std::chrono::duration_cast<std::chrono::seconds>(
		    std::chrono::steady_clock::now() - startTime).count();    	

		if (interval > 3) 
			th->canReportCurrMove = true;
	}
}


void checkTime() {

	std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();

    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

    if (timeNow.time_since_epoch() >= stopTime.time_since_epoch()) {

		Threads.stop = true;
	}

	// readInput();	
}


int32_t alphabetaSearch(int32_t alpha, int32_t beta, SearchThread *th, std::vector<u32> *pline, SearchInfo *si, int32_t mate) {


	// if (alpha >= beta) {

	// 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", ply =" << si->ply << "\n";
	// 	std::cout<<alpha<<","<<beta<<std::endl;
	// }


	assert(alpha < beta); 


	const u8 side = si->side;
	const u8 OPP = side ^ 1;

	const int ply = si->ply;
	
	const bool IS_ROOT_NODE = ply == 0;
  	const bool IS_MAIN_THREAD = th == Threads.main();
	const bool can_null_move = si->isNullMoveAllowed;


	int depth = si->depth;


	// Quiescense Search(under observation)

	if (depth <= 0 || ply >= MAX_PLY) {

		return quiescenseSearch(ply, side, alpha, beta, th, pline);
	}
	


	// Check time spent		

	if (	timeSet 
		&&  IS_MAIN_THREAD 
		&&  th->nodes % VALUI16_CHECK_NODES == 0) checkTime();
	
	if (IS_MAIN_THREAD && Threads.stop) return 0;

	if (ABORT_SEARCH) return 0; 




	th->selDepth = IS_ROOT_NODE ? 0 : std::max(th->selDepth, ply);
   
	th->nodes++;



	
	//http://www.talkchess.com/forum3/viewtopic.php?t=63090
	th->moveStack[ply + 1].killerMoves[0] = NO_MOVE;
	th->moveStack[ply + 1].killerMoves[1] = NO_MOVE;



	const bool IS_PV_NODE = alpha != beta - 1;

	const int16_t VALI16_ALL_PIECES = __builtin_popcountll(th->occupied);


	// Repetition detection
	if (!IS_ROOT_NODE) {

		for (int i = th->moves_history_counter + ply; i >= 0; i--) {
			
			if (th->movesHistory[i].hashKey == th->hashKey) {

				if (VALI16_ALL_PIECES > 22) return -50;
				
				if (VALI16_ALL_PIECES > 12) return -25;	// middlegame
					     					
											return   0;	// endgame
			}
		}

		th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;
	}



	// Transposition Table lookup
	
	HASHE *tt = &hashTable[th->hashKey % HASH_TABLE_SIZE];
	
	bool ttMatch = probeHash(tt, th);
	int32_t ttScore = ttMatch ? tt->value : VALI32_UNKNOWN;
	u32 ttMove =  ttMatch ? tt->bestMove : NO_MOVE;

	if (ttMatch) th->ttHits++;

	if (!IS_ROOT_NODE && !IS_PV_NODE && ttMatch && tt->depth >= depth && ttScore != VALI32_UNKNOWN) {
		
		if (	tt->flags == hashfEXACT 
			||	(tt->flags == hashfBETA && ttScore >= beta)
			||	(tt->flags == hashfALPHA && ttScore <= alpha)) return ttScore;
	}




	const bool IS_IN_CHECK = isKingInCheck(side, th);

    int32_t sEval = IS_IN_CHECK ? VALI32_UNKNOWN : (ttMatch ? tt->sEval : nn_eval(&nnue, th, (side == WHITE ? 0 : 1)));
    // int32_t sEval = IS_IN_CHECK ? VALI32_UNKNOWN : (ttMatch ? tt->sEval : fullEval(side, th));
	
	if (!ttMatch) recordHash(NO_MOVE, VALI16_NO_DEPTH, VALI32_UNKNOWN, VALUI8_NO_BOUND, sEval, th);		
	
	bool improving = !IS_IN_CHECK && ply >= 2 && sEval > th->moveStack[ply - 2].sEval;




	assert (!(!IS_IN_CHECK && sEval == VALI32_UNKNOWN));


	const int16_t VALI16_OPP_PIECES = 
		__builtin_popcountll(OPP ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
	
	bool canPruneOrReduce = false;

	if (	!IS_ROOT_NODE 
		&&  !IS_PV_NODE 
		&&  !IS_IN_CHECK
		&&	std::abs(alpha) < VALUI16_WIN_SCORE
		&&	std::abs(beta) < VALUI16_WIN_SCORE
		&&	VALI16_OPP_PIECES > 3) {

		canPruneOrReduce = true;	
	}	
	

	// Reverse Futility Pruning (Under observation)

	if (canPruneOrReduce) { 

		assert(sEval != VALI32_UNKNOWN);
		
		if (depth == 1 && sEval - VALUI16_RVRFPRUNE >= beta)
			return beta; 	
	
		if (depth == 2 && sEval - VALUI16_EXT_RVRFPRUNE >= beta) 
			return beta;		
	
		if (depth == 3 && sEval - VALUI16_LTD_RVRRAZOR >= beta)
			depth--;
	}


	if (!IS_ROOT_NODE) {

		th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
		th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
		th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
	}

	th->moveStack[ply].ttMove = ttMove;
	th->moveStack[ply].sEval = sEval;


	std::vector<u32> line;


	SearchInfo searchInfo;	


	bool mateThreat = false;
	// Null Move pruning 
	// Check the logic for endgame 

	if (	!IS_ROOT_NODE	
		&&	!IS_PV_NODE 
		&&	!IS_IN_CHECK 
		&&	can_null_move 
		&&	VALI16_OPP_PIECES > 3
		&&	depth > 2 
		&&	sEval >= beta) { 


		makeNullMove(ply, th);


		int r = depth > 9 ? 3 : 2;	

	
		searchInfo.side = OPP;
		searchInfo.ply = ply + 1;
		searchInfo.depth = depth - r;
		searchInfo.isNullMoveAllowed = false;
		

		int score = -alphabetaSearch(-beta, -beta + 1, th, &line, &searchInfo, mate - 1);


		unmakeNullMove(ply, th);


		if (score >= beta)
			return beta;
	
		if (std::abs(score) >= VALUI16_WIN_SCORE)	// Mate threat extension
			mateThreat = true;
	}




	bool f_prune = false;
	if (canPruneOrReduce)	{ 
			
		if (depth == 1 && sEval + VALUI16_FPRUNE <= alpha)
			f_prune = true;  	// Futility Pruning
		
		else if (depth == 2 && sEval + VALUI16_EXT_FPRUNE <= alpha)
			f_prune = true;		// Extended Futility Pruning	
		
		else if (depth == 3 && sEval + VALUI16_LTD_RAZOR <= alpha)	
			depth--; 			// Limited Razoring		
	}
	



	// Alternative to IID
	if (depth >= 4 && !ttMove) 
		depth--;	
	



	bool isQuietMove = false;

	int currentMoveType;
	int hashf = hashfALPHA;
	int reduce = 0, extend = 0, movesPlayed = 0, newDepth = 0;
	int score = -VALI32_MATE, bestScore = -VALI32_MATE;

	u32 bestMove = NO_MOVE, previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

	const u32 KILLER_MOVE_1 = th->moveStack[ply].killerMoves[0];
	const u32 KILLER_MOVE_2 = th->moveStack[ply].killerMoves[1];
	
	Move currentMove;

	std::vector<u32> quietMovesPlayed, captureMovesPlayed;
	
	th->moveList[ply].skipQuiets = false;
	th->moveList[ply].stage = PLAY_HASH_MOVE;
	th->moveList[ply].ttMove = ttMove;
	th->moveList[ply].counterMove = previousMove == NO_MOVE ? NO_MOVE : th->counterMove[side][from_sq(previousMove)][to_sq(previousMove)];
	th->moveList[ply].moves.clear();
	th->moveList[ply].badCaptures.clear();

	while (true) {


		currentMove = getNextMove(ply, side, th, &th->moveList[ply]);


		if (th->moveList[ply].stage == STAGE_DONE) break;


		assert(currentMove.move != NO_MOVE);


		make_move(ply, currentMove.move, th);


		if (isKingInCheck(side, th)) {
			
			unmake_move(ply, currentMove.move, th);
			continue;
		}


		movesPlayed++;


		currentMoveType = move_type(currentMove.move);

		isQuietMove = currentMoveType == MOVE_NORMAL ||	currentMoveType == MOVE_CASTLE 
			|| currentMoveType == MOVE_DOUBLE_PUSH;

		if (isQuietMove) {

			quietMovesPlayed.push_back(currentMove.move);
		} else {
			
			// all promotions are scored equal with a default value during move ordering
			if (currentMoveType != MOVE_PROMOTION)	
				captureMovesPlayed.push_back(currentMove.move);
		}


		// Pruning 
		if (	movesPlayed > 1
			&&	!IS_ROOT_NODE 
			&&	!IS_PV_NODE
			&&	!IS_IN_CHECK 
			&&	move_type(currentMove.move) != MOVE_PROMOTION) {

			if (isQuietMove) {
				
				// Futility pruning
				if (f_prune) {
					
					th->moveList[ply].skipQuiets = true;

					unmake_move(ply, currentMove.move, th);
					continue;
				}

				// Late move pruning
				if (	depth <= VALUI8_LMP_DEPTH 
					&&	movesPlayed >= VALUI8_LMP_BASE * depth) {

					th->moveList[ply].skipQuiets = true;

					unmake_move(ply, currentMove.move, th);
					continue;
				}

				// History pruning
				if (	depth <= VALUI8_HISTORY_PRUNING_DEPTH 
					&&	currentMove.score < VALI16_HISTORY_PRUNING) {

					unmake_move(ply, currentMove.move, th);
					continue;
				}
			}	
		}



        // report current move
        if (	IS_ROOT_NODE 
        	&&	IS_MAIN_THREAD
        	&&	th->canReportCurrMove) {	

        	reportCurrentMove(side, si->realDepth, movesPlayed, currentMove.move);
        }



        th->moveStack[ply].move = currentMove.move;

        extend = 0;
		float extension = IS_ROOT_NODE ? 0 : th->moveStack[ply - 1].extension;

		//	Extensions
		if (!IS_ROOT_NODE) { // TODO check extensions logic	

			int16_t toSqCurrentMove = to_sq(currentMove.move);
			int16_t pieceCurrMove = pieceType(currentMove.move);

			if (IS_IN_CHECK) 
				extension += VALF_EXT_CHECK;	// Check extension

			if (mateThreat)
				extension += VALF_EXT_MATE_THREAT; // Mate threat extension
		
			if (currentMoveType == MOVE_PROMOTION) 
				extension += VALF_EXT_PROMOTION;	// Promotion extension
			
			bool isPrank = side ? 
				toSqCurrentMove >= 8 && toSqCurrentMove <= 15 : 
				toSqCurrentMove >= 48 && toSqCurrentMove <= 55;
			if (pieceCurrMove == PAWNS && isPrank) 
				extension += VALF_EXT_PRANK;  // Pawn push extension

			int16_t prevMoveType = move_type(previousMove);
			if (previousMove != NO_MOVE && (prevMoveType == MOVE_CAPTURE || prevMoveType == MOVE_ENPASSANT)) {

				int16_t toSqPreviousMove = to_sq(previousMove);

				if (toSqCurrentMove == toSqPreviousMove)
					extension += VALF_EXT_RECAPTURE; // Recapture extension
			}

			if (extension >= VALF_ONE_PLY) {

				extend = 1;
				extension -= VALF_ONE_PLY;
				if (extension >= VALF_ONE_PLY) 
					extension = 3 * VALF_ONE_PLY / 4;
			}
		} 

		th->moveStack[ply].extension = extension;		
	 

		newDepth = (depth - 1) + extend;


		reduce = 0;


		searchInfo.side = OPP;
		searchInfo.ply = ply + 1;
		searchInfo.isNullMoveAllowed = true;


		if (movesPlayed <= 1) {	// Principal Variation Search

			searchInfo.depth = newDepth;

			score = -alphabetaSearch(-beta, -alpha, th, &line, &searchInfo, mate - 1);
		} else {
			
			// Late Move Reductions (Under observation)

			if (	depth > 2
				&&	movesPlayed > 3
				&&	isQuietMove) {
				
		        
				reduce = depth > 6 ? depth / 3 : 1;	


				if (!IS_PV_NODE) reduce++;
				if (!improving && !IS_IN_CHECK) reduce++; // IS_IN_CHECK sets improving to false
				if (IS_IN_CHECK && pieceType(currentMove.move) == KING) reduce++;

				if (currentMove.move == KILLER_MOVE_1 || currentMove.move == KILLER_MOVE_2) reduce--;
	            
	            reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO	rewrite logic				

	        	int r = std::min(depth - 1, std::max(reduce, 1));	// TODO rewrite logic


	        	searchInfo.depth = newDepth - r;	

				score = -alphabetaSearch(-alpha - 1, -alpha, th, &line, &searchInfo, mate - 1);			

			} else {

				score = alpha + 1;
			}

			if (score > alpha) {	// Research 

				searchInfo.depth = newDepth;
				
				score = -alphabetaSearch(-alpha - 1, -alpha, th, &line, &searchInfo, mate - 1);					
		
				if (score > alpha && score < beta) {

					score = -alphabetaSearch(-beta, -alpha, th, &line, &searchInfo, mate - 1);					
				}
			}
		}


		unmake_move(ply, currentMove.move, th);


		if (score > bestScore) {

			bestScore = score;
			bestMove = currentMove.move;
			
			if (score > alpha) {

				alpha = score;
				hashf = hashfEXACT;

				pline->clear();
				pline->push_back(bestMove);
				
				std::copy(line.begin(), line.end(), back_inserter(*pline));
				line.clear();

				if (score >= beta) {

					hashf = hashfBETA;
					
					break;
				} 					
			} 
		}
	}


	if (hashf == hashfBETA) { // failed high

		if (isQuietMove) {

			if (	bestMove != KILLER_MOVE_1 
				&&  bestMove != KILLER_MOVE_2) {

				th->moveStack[ply].killerMoves[1] = KILLER_MOVE_1;
				th->moveStack[ply].killerMoves[0] = bestMove;
			}							

			updateHistory(ply, side, depth, bestMove, quietMovesPlayed, th);			
		} else {

			updateCaptureHistory(ply, side, depth, bestMove, captureMovesPlayed, th);
		}
	}


	if (movesPlayed == 0) { // Mate and stalemate check

		return IS_IN_CHECK ? -mate : 0;
	}


	recordHash(bestMove, depth, bestScore, hashf, sEval, th);


	return bestScore;
}	


// TODO should limit Quiescense search explosion
int32_t quiescenseSearch(int32_t ply, int8_t side, int32_t alpha, int32_t beta, SearchThread *th, std::vector<u32> *pline) {


	assert (alpha < beta);
	assert (ply > 0);


	const int8_t OPP = side ^ 1;

	const bool IS_MAIN_THREAD = th == Threads.main();


	// Check if time limit has been reached
	if (timeSet && IS_MAIN_THREAD && th->nodes % VALUI16_CHECK_NODES == 0) checkTime();	
	
	if (IS_MAIN_THREAD && Threads.stop) return 0;

	if (ABORT_SEARCH) return 0; 



	th->selDepth = std::max(th->selDepth, ply);
	th->nodes++;



	const int16_t VALI16_ALL_PIECES = __builtin_popcountll(th->occupied);


	// Repetition detection
	for (int i = th->moves_history_counter + ply; i >= 0; i--) {

		if (th->movesHistory[i].hashKey == th->hashKey) {

			if (	VALI16_ALL_PIECES > 22)	return -50;
			if (	VALI16_ALL_PIECES > 12)	return -25;	// middlegame
				     				 		return   0;	// endgame
		}
	}

	th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;



	if (ply >= MAX_PLY - 1) {
		// return fullEval(side, th);
		return nn_eval(&nnue, th, (side == WHITE ? 0 : 1));
	}



	HASHE *tt = &hashTable[th->hashKey % HASH_TABLE_SIZE];

	bool ttMatch = probeHash(tt, th);  
	int32_t ttScore = VALI32_UNKNOWN;

	if (ttMatch) { // no depth check required since its 0 in quiescense search

		th->ttHits++;

		ttScore = tt->value;

	  	if (	ttScore != VALI32_UNKNOWN 
	  		&&	(tt->flags == hashfEXACT 
			||	(tt->flags == hashfBETA && ttScore >= beta)
			||	(tt->flags == hashfALPHA && ttScore <= alpha))) {

	  		return ttScore;	
	  	}
	}


	u32 bestMove = NO_MOVE;
	int bestScore = -VALI32_MATE;
	int sEval;

	// pull cached eval if it exists
	int eval = sEval = (ttMatch && tt->sEval != VALI32_UNKNOWN) ? tt->sEval : nn_eval(&nnue, th, (side == WHITE ? 0 : 1));
	// int eval = sEval = (ttMatch && tt->sEval != VALI32_UNKNOWN) ? tt->sEval : fullEval(side, th);
	
	if (!ttMatch) recordHash(NO_MOVE, VALI16_NO_DEPTH, VALI32_UNKNOWN, VALUI8_NO_BOUND, eval, th);		
	

	bestScore = eval;
	alpha = std::max(alpha, eval);

	if (alpha >= beta) return eval;	


	
	th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
	th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
	th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;


	const int32_t Q_FUTILITY_BASE = sEval + VALUI16_Q_DELTA; 

	const int16_t VALI16_OPP_PIECES = __builtin_popcountll(
		OPP ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	
	int32_t hashf = hashfALPHA;
	
	Move currentMove;

	std::vector<u32> line;

	th->moveList[ply].skipQuiets = true;
	th->moveList[ply].stage = GEN_CAPTURES;
	th->moveList[ply].ttMove = NO_MOVE;
	th->moveList[ply].counterMove = NO_MOVE;
	th->moveList[ply].moves.clear();
	th->moveList[ply].badCaptures.clear();

	int16_t capPiece, movesPlayed = 0; 
	int32_t  score = -VALI32_MATE;

	while (true) {

		currentMove = getNextMove(ply, side, th, &th->moveList[ply]);

		if (th->moveList[ply].stage >= PLAY_BAD_CAPTURES) break;

		assert(currentMove.move != NO_MOVE);

		make_move(ply, currentMove.move, th);

		if (isKingInCheck(side, th)) {

			unmake_move(ply, currentMove.move, th);

 			continue;
		}


		movesPlayed++;

		capPiece = cPieceType(currentMove.move);

 		// Pruning
 		if (	movesPlayed > 1
 			&&	capPiece != DUMMY
 			&&	move_type(currentMove.move) != MOVE_PROMOTION) {

			// Delta pruning
	 		if (VALI16_OPP_PIECES > 3 && Q_FUTILITY_BASE + seeVal[capPiece] <= alpha) {

				unmake_move(ply, currentMove.move, th);

	 			continue;
	 		}
 		}


		score = -quiescenseSearch(ply + 1, OPP, -beta, -alpha, th, &line);


		unmake_move(ply, currentMove.move, th);


		if (score > bestScore) {

			bestScore = score;
			bestMove = currentMove.move;
			
			if (score > alpha) {	
				
				alpha = score;
				hashf = hashfEXACT;

				pline->clear();
				pline->push_back(bestMove);
				std::copy(line.begin(), line.end(), back_inserter(*pline));
				line.clear();

				if (score >= beta) {

					hashf = hashfBETA;
					
					break;
				} 					
			} 
		}
	}


	recordHash(bestMove, 0, bestScore, hashf, sEval, th);


	return bestScore;
}

































// For Tuning
/*int QuiescenseForTuning(int ply, int side, int alpha, int beta, int depth, Thread *th, TraceCoefficients *T) {

	
	const u8 opponent = side ^ 1;

	
	int staticEval = traceFullEval(T, side, th);


	if (ply > MAX_PLY)	return staticEval;

	if (staticEval >= beta)	return staticEval;
	
	if (staticEval > alpha)	alpha = staticEval;


	if (ply != 0) {
	
		th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
		th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
		th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
	}

	std::vector<Move> moveList;	

	int val = -VALUI32_MATE;

	Move currentMove, tmp;

	for (int stage = STAGE_PROMOTIONS; stage <= STAGE_CAPTURES; stage++) {

		moveList.clear();
		getMoves(ply, side, moveList, stage, true, th);

		int n = moveList.size();

		for (int i = 0; i < n; i++) {   // loop n times - 1 per element
        	for (int j = 0; j < n - i - 1; j++) { // last i elements are sorted already
    
            	if (moveList[j].score < moveList[j + 1].score) {
            	
                	tmp = moveList[j];
                	moveList[j] = moveList[j + 1];
                	moveList[j + 1] = tmp;
           		}
        	}
    	}


		for (std::vector<Move>::iterator i = moveList.begin(); i != moveList.end(); ++i) {
	
   	 		currentMove = *i;

			make_move(ply, currentMove.move, th);

			if (!isKingInCheck(side, th)) {

				val = -QuiescenseForTuning(ply + 1, opponent, -beta, -alpha, depth - 1, th, T);

				unmake_move(ply, currentMove.move, th);

				if (val > alpha) {

					if (val >= beta)	return beta;
					
					alpha = val;
				}
			} else {

				unmake_move(ply, currentMove.move, th);
			}
		}
	}

	return alpha;
}*/
