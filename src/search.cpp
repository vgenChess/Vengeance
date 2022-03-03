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
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "hash.h"
#include "thread.h"
#include "see.h"
#include "constants.h"
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

int LMR[64][64];

void initLMR() {

	float a = 0.1, b = 2;
    for (int depth = 1; depth < 64; depth++) {
    	for (int moves = 1; moves < 64; moves++) {

        	LMR[depth][moves] = a + log(depth) * log(moves) / b;
    	}
    }
}

//TODO refactor logic
void startSearch(U8 side) {


	ABORT_SEARCH = false;

	Threads.start_searching(); // start non-main threads
	iterativeDeepeningSearch(side, Threads.main()); // main thread start searching

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
	
 
int stableMoveCount = 0;

void iterativeDeepeningSearch(int sideToMove, SearchThread *th) {
	
	th->nodes = 0;
	th->ttHits = 0;	

	th->depth = I16_NO_DEPTH;
	th->completedDepth = I16_NO_DEPTH;

	stableMoveCount = 0;

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


			// score change
			int32_t prevScore = th->pvLine.at(th->completedDepth-3).score;
 			int32_t currentScore = th->pvLine.at(th->completedDepth).score;

			float scoreChangeFactor = prevScore > currentScore ? 
 				fmax(0.5, fmin(1.5, ((prevScore - currentScore) * 0.05))) : 0.5;
			

			// best move change 			
 			assert(th->pvLine.at(th->completedDepth).line.size() > 0 
		    	&& th->pvLine.at(th->completedDepth-1).line.size() > 0);

 			U32 previousMove = th->pvLine.at(th->completedDepth-1).line.at(0);
 			U32 currentMove = th->pvLine.at(th->completedDepth).line.at(0);
		    
 			stableMoveCount = previousMove == currentMove ? stableMoveCount + 1 : 0;
 			stableMoveCount = std::min(10, stableMoveCount);

 			float stableMoveFactor =  1.25 - stableMoveCount * 0.05;

			
			// win factor
			float winFactor = currentScore >= U16_WIN_SCORE ? 0.5 : 1;
			
		    
		    // Check for time 
		    std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
		    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

	    	if (timeSpent > (timePerMove * scoreChangeFactor * stableMoveFactor * winFactor)) {

				Threads.stop = true;
				break;	
			}
		}
	} 

	Threads.stop = true;
}


void aspirationWindowSearch(U8 side, SearchThread *th) {

	int32_t window = I32_MATE;

	int32_t score = -I32_MATE;
	int32_t alpha = -I32_MATE, beta = I32_MATE;
	
	if (th->depth > 4 && th->completedDepth > 0) {

		window = U8_AP_WINDOW;

		int scoreKnown = th->pvLine.at(th->completedDepth).score;

  		alpha = std::max(-I32_MATE, scoreKnown - window);
	  	beta  = std::min( I32_MATE, scoreKnown + window);	
	}
 	

	std::vector<U32> line;


	SearchInfo searchInfo;

	searchInfo.side = side;
	searchInfo.ply = 0;
	searchInfo.realDepth = th->depth;
	searchInfo.isNullMoveAllowed = false;
	searchInfo.pline = line;
	searchInfo.skipMove = NO_MOVE;		

	int failHighCounter = 0;

	while (true) {
		
		searchInfo.depth = std::max(1, th->depth - failHighCounter);
		th->selDepth = I16_NO_DEPTH;	
		searchInfo.pline.clear();
		
		score = alphabetaSearch(alpha, beta, I32_MATE, th, &searchInfo);

		if (Threads.stop)
        	break;

		if (score <= alpha)	{

			beta = (alpha + beta) / 2;
			alpha = std::max(score - window, -I32_MATE);

			failHighCounter = 0;
		}
		else if (score >= beta)	{

			beta = std::min(score + window, I32_MATE);
			
			if (std::abs(score) < U16_WIN_SCORE)
				failHighCounter++;
		}	
		else {

			th->completedDepth = th->depth;

			th->pvLine.at(th->completedDepth).score = score;

			th->pvLine.at(th->completedDepth).line.clear();
			std::copy(searchInfo.pline.begin(), searchInfo.pline.end(), back_inserter(
				th->pvLine.at(th->completedDepth).line));

			break;
		}

		window += window / 4; 
	}

	
	if (Threads.stop)
    	return;


	assert (score > alpha && score < beta);


	if (th != Threads.main()) 
		return;

	reportPV(th);

	if (!th->canReportCurrMove) {

		int interval = std::chrono::duration_cast<std::chrono::milliseconds>(
		    std::chrono::steady_clock::now() - startTime).count();    	

		if (interval > 3000) 
			th->canReportCurrMove = true;
	}
}


void checkTime() {

	std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();

    if (timeNow.time_since_epoch() >= stopTime.time_since_epoch()) {

		Threads.stop = true;
	}	
}


int alphabetaSearch(int alpha, int beta, int mate, SearchThread *th, SearchInfo *si) {


	// if (alpha >= beta) {
	// 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", ply =" << si->ply << "\n";
	// 	std::cout<<alpha<<","<<beta<<std::endl;
	// }


	assert(alpha < beta); 


	const U8 SIDE = si->side;
	const U8 OPP = SIDE ^ 1;

	const int PLY = si->ply;
	
	const bool IS_ROOT_NODE = PLY == 0;
  	const bool IS_MAIN_THREAD = th == Threads.main();
  	const bool IS_SINGULAR_SEARCH = si->skipMove != NO_MOVE;
	const bool CAN_NULL_MOVE = si->isNullMoveAllowed;


	int depth = si->depth;


	// Quiescense Search(under observation)

	if (depth <= 0 || PLY >= U16_MAX_PLY) {

		si->pline.clear();
		return quiescenseSearch(PLY, SIDE, alpha, beta, th, &si->pline);
	}
	


	// Check time spent		

	if (	timeSet 
		&&  IS_MAIN_THREAD 
		&&  th->nodes % U16_CHECK_NODES == 0) checkTime();
	
	if (IS_MAIN_THREAD && Threads.stop) return 0;

	if (ABORT_SEARCH) return 0; 




	th->selDepth = IS_ROOT_NODE ? 0 : std::max(th->selDepth, PLY);
   
	th->nodes++;



	
	//http://www.talkchess.com/forum3/viewtopic.php?t=63090
	th->moveStack[PLY + 1].killerMoves[0] = NO_MOVE;
	th->moveStack[PLY + 1].killerMoves[1] = NO_MOVE;


	const bool IS_PV_NODE = alpha != beta - 1;

	const int16_t I16_ALL_PIECES = POPCOUNT(th->occupied);


	if (!IS_ROOT_NODE) {

		// Repetition detection
		if (isRepetition(PLY, th)) {

			if (I16_ALL_PIECES > 22)	return -50;
			if (I16_ALL_PIECES > 12)	return -25;	// middlegame
									 	return   0;	// endgame
		}

		th->movesHistory[th->moves_history_counter + PLY + 1].hashKey = th->hashKey;

		// Check for drawish endgame
		if (isPositionDraw(th)) return 0;
	}


	// Transposition Table lookup
	
	HASHE *tt = IS_SINGULAR_SEARCH ? NULL : &hashTable[th->hashKey % HASH_TABLE_SIZE];
	
	bool ttMatch = probeHash(tt, th);
	int32_t ttScore = ttMatch ? tt->value : I32_UNKNOWN;
	U32 ttMove =  ttMatch ? tt->bestMove : NO_MOVE;

	if (ttMatch) th->ttHits++;

	if (!IS_ROOT_NODE && !IS_PV_NODE && ttMatch && tt->depth >= depth && ttScore != I32_UNKNOWN) {
		
		if (	tt->flags == hashfEXACT 
			||	(tt->flags == hashfBETA && ttScore >= beta)
			||	(tt->flags == hashfALPHA && ttScore <= alpha)) {

			return ttScore;
		}
	}



	// Alternative to IID
	if (depth >= 4 && !ttMove && !IS_SINGULAR_SEARCH) 
		depth--;	




	const bool IS_IN_CHECK = isKingInCheck(SIDE, th);
    bool improving = false;

	int32_t sEval = I32_UNKNOWN;

	if (IS_SINGULAR_SEARCH) {

		sEval = th->moveStack[PLY].sEval;
	} else if (IS_IN_CHECK) {
        
        sEval = I32_UNKNOWN;
        improving = false;
    } else if (ttMatch) {
        
        sEval = tt->sEval;
        if (sEval == I32_UNKNOWN)
            sEval = fullEval(SIDE, th); // Do not save sEval to the TT since it can overwrite the previous hash entry
    } else {

        sEval = fullEval(SIDE, th);

        recordHash(NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval, th);		
    }



	if (!IS_IN_CHECK) {

		improving = PLY >= 2 ? sEval > th->moveStack[PLY - 2].sEval : true;
	}



	const int16_t I16_OPP_PIECES = POPCOUNT(
		OPP ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);	

	// Reverse Futility Pruning
	if (	!IS_ROOT_NODE 
		&&	!IS_PV_NODE 
		&&	!IS_IN_CHECK 
		&&	!IS_SINGULAR_SEARCH
		&&	std::abs(alpha) < U16_WIN_SCORE
		&&	std::abs(beta) < U16_WIN_SCORE 
		&&	I16_OPP_PIECES > 3) { 

		assert(sEval != I32_UNKNOWN);
		
		if (depth == 1 && sEval - U16_RVRFPRUNE >= beta) 
			return beta; 	
	
		if (depth == 2 && sEval - U16_EXT_RVRFPRUNE >= beta) 
			return beta;		
	
		if (depth == 3 && sEval - U16_LTD_RVRRAZOR >= beta) 
			depth--;
	}


	if (!IS_ROOT_NODE) {

		th->moveStack[PLY].epFlag = th->moveStack[PLY - 1].epFlag;
		th->moveStack[PLY].epSquare = th->moveStack[PLY - 1].epSquare;
		th->moveStack[PLY].castleFlags = th->moveStack[PLY - 1].castleFlags;
	}

	th->moveStack[PLY].ttMove = ttMove;
	th->moveStack[PLY].sEval = sEval;


	std::vector<U32> line;


	SearchInfo searchInfo;	
	searchInfo.pline = line;

	bool mateThreat = false;	 
	// Null Move pruning 
	if (	!IS_ROOT_NODE 
		&&	!IS_PV_NODE 
		&&	!IS_IN_CHECK 
		&&	!IS_SINGULAR_SEARCH
		&&	I16_OPP_PIECES > 3 // Check the logic for endgame
		&&	CAN_NULL_MOVE 
		&&	depth > 2 
		&&	sEval >= beta) { 


		makeNullMove(PLY, th);


		int r = depth > 9 ? 3 : 2;	

	
		searchInfo.side = OPP;
		searchInfo.ply = PLY + 1;
		searchInfo.depth = depth - r;
		searchInfo.isNullMoveAllowed = false;
		
		searchInfo.pline.clear();
		int score = -alphabetaSearch(-beta, -beta + 1, mate - 1, th, &searchInfo);


		unmakeNullMove(PLY, th);


		if (score >= beta)
			return beta;
	
		if (std::abs(score) >= U16_WIN_SCORE) // Mate threat 	
			mateThreat = true;
	}


	bool fPrune = false;
	if (	!IS_ROOT_NODE 
		&&	!IS_PV_NODE 
		&&	!IS_IN_CHECK 
		&&	!IS_SINGULAR_SEARCH
		&&	std::abs(alpha) < U16_WIN_SCORE
		&&	std::abs(beta) < U16_WIN_SCORE 
		&&	I16_OPP_PIECES > 3)	{ 
			
		assert(sEval != I32_UNKNOWN);

		if (depth == 1 && sEval + U16_FPRUNE <= alpha) // Futility Pruning
			fPrune = true;  	
		
		else if (depth == 2 && sEval + U16_EXT_FPRUNE <= alpha) // Extended Futility Pruning
			fPrune = true;			
		
		else if (depth == 3 && sEval + U16_LTD_RAZOR <= alpha) // Limited Razoring
			depth--; 					
	}
	

	bool ttMoveIsSingular = false;

	// Singular search
	if (	!IS_ROOT_NODE
		&&	!IS_SINGULAR_SEARCH
		&&  depth >= 7
		&&  ttMatch
		&&  ttMove != NO_MOVE 
		&&  std::abs(ttScore) < U16_WIN_SCORE
		&&  tt->flags == hashfBETA
		&&  tt->depth >= depth - 3) {


		int sBeta = ttScore - 4 * depth;
		int sDepth = depth / 2 - 1;

		searchInfo.skipMove = ttMove;
		searchInfo.side = SIDE;
		searchInfo.ply = PLY;
		searchInfo.depth = sDepth;
		searchInfo.pline.clear();

		int32_t score = alphabetaSearch(sBeta - 1, sBeta, mate, th, &searchInfo);

		searchInfo.skipMove = NO_MOVE;

		if (score < sBeta) {
			
			ttMoveIsSingular = true;
		} else if (sBeta >= beta) { // TODO check logic

			return sBeta;
		}
	}


	bool isQuietMove = false;
	
	int8_t hashf = hashfALPHA;
	int16_t currentMoveType, currentMoveFromSq, currentMoveToSq;
	int32_t reduce = 0, extend = 0, movesPlayed = 0, newDepth = 0;
	int32_t score = -I32_MATE, bestScore = -I32_MATE;

	U32 bestMove = NO_MOVE, previousMove = IS_ROOT_NODE ? NO_MOVE : th->moveStack[PLY - 1].move;

	const U32 KILLER_MOVE_1 = th->moveStack[PLY].killerMoves[0];
	const U32 KILLER_MOVE_2 = th->moveStack[PLY].killerMoves[1];

	Move currentMove;

	std::vector<U32> quietMovesPlayed, captureMovesPlayed;
	
	th->moveList[PLY].skipQuiets = false;
	th->moveList[PLY].stage = PLAY_HASH_MOVE;
	th->moveList[PLY].ttMove = ttMove;
	th->moveList[PLY].counterMove = previousMove == NO_MOVE ? 
		NO_MOVE : th->counterMove[SIDE][from_sq(previousMove)][to_sq(previousMove)];
	th->moveList[PLY].moves.clear();
	th->moveList[PLY].badCaptures.clear();

	while (true) {

		// fetch next psuedo-legal move
		currentMove = getNextMove(PLY, SIDE, th, &th->moveList[PLY]);

		if (th->moveList[PLY].stage == STAGE_DONE) 
			break;

		// check if move generator returns a valid psuedo-legal move
		assert(currentMove.move != NO_MOVE);

		// skip the move if its in a singular search and the current move is singular
		if (IS_SINGULAR_SEARCH && currentMove.move == si->skipMove) 
			continue;

		// make the move
		make_move(PLY, currentMove.move, th);

		// check if psuedo-legal move is valid
		if (isKingInCheck(SIDE, th)) {
			
			unmake_move(PLY, currentMove.move, th);
			continue;
		}

		// psuedo legal move is valid, increment number of moves played
		movesPlayed++;


		currentMoveType = move_type(currentMove.move);
		currentMoveToSq = to_sq(currentMove.move);
		currentMoveFromSq = from_sq(currentMove.move);


		isQuietMove = currentMoveType == MOVE_NORMAL || currentMoveType == MOVE_CASTLE 
			|| currentMoveType == MOVE_DOUBLE_PUSH;

		if (isQuietMove) {

			quietMovesPlayed.push_back(currentMove.move);
		} else if (currentMoveType != MOVE_PROMOTION) { 
			// all promotions are scored equal and ordered differently from capture moves
			
			captureMovesPlayed.push_back(currentMove.move);
		}


		// Prune moves based on conditions met
		if (	movesPlayed > 1
			&&	!IS_ROOT_NODE 
			&&	!IS_PV_NODE
			&&	!IS_IN_CHECK 
			&&	move_type(currentMove.move) != MOVE_PROMOTION) {

			if (isQuietMove) {
				
				// Futility pruning
				if (fPrune) {
					
					th->moveList[PLY].skipQuiets = true;

					unmake_move(PLY, currentMove.move, th);
					continue;
				}

				// Late move pruning
				if (	depth <= U8_LMP_DEPTH 
					&&	movesPlayed >= U8_LMP_BASE * depth) {

					th->moveList[PLY].skipQuiets = true;

					unmake_move(PLY, currentMove.move, th);
					continue;
				}

				// History pruning
				if (	depth <= U8_HISTORY_PRUNING_DEPTH 
					&&	currentMove.score < I16_HISTORY_PRUNING) {

					unmake_move(PLY, currentMove.move, th);
					continue;
				}
			}	
		}



        // report current move being searched
        if (	IS_ROOT_NODE 
        	&&	IS_MAIN_THREAD
        	&&	th->canReportCurrMove) {	

        	reportCurrentMove(si->realDepth, movesPlayed, currentMove.move);
        }


        
        th->moveStack[PLY].move = currentMove.move;

        extend = 0;
		float extension = IS_ROOT_NODE ? 0 : th->moveStack[PLY - 1].extension;

		//	Fractional Extensions
		if (!IS_ROOT_NODE) { // TODO check extensions logic	

			int16_t pieceCurrMove = pieceType(currentMove.move);

			if (currentMove.move == ttMove && ttMoveIsSingular) { // Singular extension

				extension += F_SINGULAR_EXT;
			}

			if (IS_IN_CHECK) {

				extension += F_CHECK_EXT;	// Check extension
			}

			if (mateThreat) {

				extension += F_MATE_THREAT_EXT; // Mate threat extension
			}
		
			if (currentMoveType == MOVE_PROMOTION) {

				extension += F_PROMOTION_EXT;	// Promotion extension
			}
			
			bool isPrank = SIDE ? 
				currentMoveToSq >= 8 && currentMoveToSq <= 15 : 
				currentMoveToSq >= 48 && currentMoveToSq <= 55;
			
			if (pieceCurrMove == PAWNS && isPrank) {

				extension += F_PRANK_EXT;  // Pawn push extension
			}

			int16_t prevMoveType = move_type(previousMove);
			if (previousMove != NO_MOVE && prevMoveType == MOVE_CAPTURE) {

				int16_t prevMoveToSq = to_sq(previousMove);
				if (currentMoveToSq == prevMoveToSq) {

					extension += F_RECAPTURE_EXT; // Recapture extension
				}
			}

			if (extension >= F_ONE_PLY) {

				extend = 1;
				extension -= F_ONE_PLY;
				if (extension >= F_ONE_PLY) {

					extension = 3 * F_ONE_PLY / 4;
				}
			}
		} 

		th->moveStack[PLY].extension = extension;		
	 

		newDepth = (depth - 1) + extend;


		reduce = 0;


		searchInfo.side = OPP;
		searchInfo.ply = PLY + 1;
		searchInfo.isNullMoveAllowed = true;


		if (movesPlayed <= 1) {	// Principal Variation Search

			searchInfo.depth = newDepth;
			searchInfo.pline.clear();

			score = -alphabetaSearch(-beta, -alpha, mate - 1, th, &searchInfo);
		} else {
			
			// Late Move Reductions (Under observation)

			if (	depth > 2
				&&	movesPlayed > 1
				&&	isQuietMove) {
				
		        reduce = LMR[std::min(depth, 63)][std::min(movesPlayed, 63)];

				if (!IS_PV_NODE) reduce++;
				if (!improving && !IS_IN_CHECK) reduce++; // IS_IN_CHECK sets improving to false
				if (IS_IN_CHECK && pieceType(currentMove.move) == KING) reduce++;

				if (th->moveList[PLY].stage < GEN_QUIETS) reduce--; // reduce less for killer and counter moves
	            
	            reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO	rewrite logic				


	        	reduce = std::min(depth - 1, std::max(reduce, 1));

	        	searchInfo.depth = newDepth - reduce;	
				searchInfo.pline.clear();
				
				score = -alphabetaSearch(-alpha - 1, -alpha, mate - 1, th, &searchInfo);			

			} else {

				score = alpha + 1;
			}

			if (score > alpha) {	// Research 

				searchInfo.depth = newDepth;
				searchInfo.pline.clear();

				score = -alphabetaSearch(-alpha - 1, -alpha, mate - 1, th, &searchInfo);					
		
				if (score > alpha && score < beta) {

					score = -alphabetaSearch(-beta, -alpha, mate - 1, th, &searchInfo);					
				}
			}
		}


		unmake_move(PLY, currentMove.move, th);


		if (score > bestScore) {

			bestScore = score;
			bestMove = currentMove.move;

			if (score > alpha) {

				alpha = score;
				hashf = hashfEXACT;
				
				// record the moves for the PV			
				si->pline.clear();
				si->pline.push_back(bestMove);
				std::copy(searchInfo.pline.begin(), searchInfo.pline.end(), back_inserter(si->pline));
				searchInfo.pline.clear();

				if (score >= beta) {

					hashf = hashfBETA;
					
					// Fail high
					// No further moves need to be searched, since one refutation is already sufficient 
					// to avoid the move that led to this node or position. 
					break;
				}
			}
		}
	}


	if (hashf == hashfBETA) {

		if (isQuietMove) {

			// update killers
		
			if (bestMove != KILLER_MOVE_1 && bestMove != KILLER_MOVE_2) {

				th->moveStack[PLY].killerMoves[1] = KILLER_MOVE_1;
				th->moveStack[PLY].killerMoves[0] = bestMove;
			}

			updateHistory(PLY, SIDE, depth, bestMove, quietMovesPlayed, th);				
		} 

		updateCaptureHistory(depth, bestMove, captureMovesPlayed, th);
	}


	if (movesPlayed == 0) { // Mate and stalemate check

		return IS_IN_CHECK ? -mate : 0;
	}


	if (!IS_SINGULAR_SEARCH)
		recordHash(bestMove, depth, bestScore, hashf, sEval, th);


	return bestScore;
}	


// TODO should limit Quiescense search explosion
int quiescenseSearch(int ply, int side, int alpha, int beta, SearchThread *th, std::vector<U32> *pline) {


	assert (alpha < beta);
	assert (ply > 0);


	const U8 OPP = side ^ 1;

	const bool IS_MAIN_THREAD = th == Threads.main();


	// Check if time limit has been reached
	if (timeSet && IS_MAIN_THREAD && th->nodes % U16_CHECK_NODES == 0) checkTime();	
	
	if (IS_MAIN_THREAD && Threads.stop) return 0;

	if (ABORT_SEARCH) return 0; 


	th->selDepth = std::max(th->selDepth, ply);
	th->nodes++;


	const char VAL_ALL_PIECES = POPCOUNT(th->occupied);

	// Repetition detection
	if (isRepetition(ply, th)) {

		if (VAL_ALL_PIECES > 22)	return -50;
		if (VAL_ALL_PIECES > 12)	return -25;	// middlegame
								 	return   0;	// endgame
	}

	th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;

	// Check for drawish endgame
	if (isPositionDraw(th)) return 0;

	if (ply >= U16_MAX_PLY - 1) return fullEval(side, th);



	HASHE *tt = &hashTable[th->hashKey % HASH_TABLE_SIZE];

	bool ttMatch = probeHash(tt, th);  
	int32_t ttScore = I32_UNKNOWN;

	if (ttMatch) { // no depth check required since its 0 in quiescense search

		th->ttHits++;

		ttScore = tt->value;

	  	if (	ttScore != I32_UNKNOWN 
	  		&&	(tt->flags == hashfEXACT 
			||	(tt->flags == hashfBETA && ttScore >= beta)
			||	(tt->flags == hashfALPHA && ttScore <= alpha))) {

	  		return ttScore;	
	  	}
	}


	U32 bestMove = NO_MOVE;
	int32_t sEval, bestScore = -I32_MATE;
	
	if (ttMatch) {
        
        sEval = tt->sEval;
        if (sEval == I32_UNKNOWN)
            sEval = fullEval(side, th); // Do not save sEval to the TT since it can overwrite the previous hash entry
    } else {

        sEval = fullEval(side, th);

        recordHash(NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval, th);		
    }


	bestScore = sEval;
	alpha = std::max(alpha, sEval);

	if (alpha >= beta) return sEval;	


	
	th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
	th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
	th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;


	const int32_t Q_FUTILITY_BASE = sEval + U16_Q_DELTA; 

	const int16_t I16_OPP_PIECES = 
		POPCOUNT(OPP ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	
	int32_t hashf = hashfALPHA;
	
	Move currentMove;

	std::vector<U32> line;

	th->moveList[ply].skipQuiets = true;
	th->moveList[ply].stage = GEN_CAPTURES;
	th->moveList[ply].ttMove = NO_MOVE;
	th->moveList[ply].counterMove = NO_MOVE;
	th->moveList[ply].moves.clear();
	th->moveList[ply].badCaptures.clear();

	int16_t capPiece, movesPlayed = 0; 
	int32_t score = -I32_MATE;

	while (true) {

		currentMove = getNextMove(ply, side, th, &th->moveList[ply]);

		if (th->moveList[ply].stage >= PLAY_BAD_CAPTURES) 
			break;

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
	 		if (I16_OPP_PIECES > 3 && Q_FUTILITY_BASE + seeVal[capPiece] <= alpha) {

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

	
	const U8 opponent = side ^ 1;

	
	int staticEval = traceFullEval(T, side, th);


	if (ply > U16_MAX_PLY)	return staticEval;

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
