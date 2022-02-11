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

int age = 0;

bool timeSet, stopped;

int option_thread_count;

std::chrono::steady_clock::time_point startTime;
std::chrono::steady_clock::time_point stopTime;

int timePerMove;
int MAX_TIME_PER_SEARCH = 0;
int MIN_TIME_PER_SEARCH = 0;

int seeVal[8] = { 
    
    0, 
    VALUE_PAWN, 
    VALUE_KNIGHT, 
    VALUE_BISHOP, 
    VALUE_ROOK,	
    VALUE_QUEEN, 
    VALUE_KING, 
    0
};

std::vector<Stage> STAGES = {

	STAGE_HASH_MOVE,
	STAGE_PROMOTIONS,
	STAGE_CAPTURES,
	STAGE_KILLER_MOVES,
	STAGE_BAD_CAPTURES,
	STAGE_NORMAL_MOVES
};


int ABORT_SIGNAL;

std::vector<Thread> sThreads;
std::mutex mtx;

int MAX_DEPTH = 100;

std::string getMoveNotation(const u32 move) {

	char str[10];
	str[0] = '\0';

	strcat(str, algebricPos(from_sq(move)));
	strcat(str, algebricPos(to_sq(move)));

    if (move_type(move) == MOVE_PROMOTION) {

    	u8 pt = promType(move);

        if  (pt == PROMOTE_TO_ROOK) {
                
            strcat(str, "r");
        }
        else if (pt == PROMOTE_TO_BISHOP) {
                
            strcat(str, "b");
        }   
        else if (pt == PROMOTE_TO_KNIGHT) {
                
            strcat(str, "n");
        } 
        else {
                
            strcat(str, "q");
        }
    }

    return std::string(str);
}

void reportBestMove() {

	u32 bestMove = Threads.main()->pvLine[Threads.main()->depth].line[0];

	std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;
}

void reportCurrentMove(int side, int depth, int currentMoveNumber, u32 move) {

	std::cout << "info depth " << depth << " currmove ";

	std::cout << getMoveNotation(move) << " currmovenumber " << currentMoveNumber << std::endl;
}

void display(u8 sideToMove, int depth, int selDepth, int score, std::vector<u32> pvLine) {

	std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
    int timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - startTime).count();

	std::cout 	<< "info depth " << depth << " seldepth " << selDepth 
				<< " time " << timeSpent << " nodes " << Threads.getTotalNodes() 
				/*<< " hashfull " << hashfull()*/ << " tbhits " << Threads.getTotalTTHits() 
				<< " score cp " << score << " pv";

	for (std::vector<u32>::iterator i = pvLine.begin(); i != pvLine.end(); ++i) {
		
		std::cout << " " << getMoveNotation(*i);
	}

	std::cout << "\n";
}


void startSearch(u8 sideToMove) {

	age++; // For storing hash age information

	Threads.start_searching(); // start non-main threads
	searchMain(sideToMove, Threads.main());          // main thread start searching

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

#define GET_MIN(A, B) ((A) < (B) ? (A) : (B)) //TODO change to standard lib for float comparison
#define GET_MAX(A, B) ((A) > (B) ? (A) : (B))

void searchMain(int sideToMove, SearchThread *th) {
	

	const bool is_main_thread = th == Threads.main();

	th->nodes = 0;
	th->ttHits = 0;	

	th->depth = NO_DEPTH;
	th->completedDepth = NO_DEPTH;

	stableMoveCount = 0;

	maxTimePerMove = 5 * timePerMove;
	minTimePerMove = timePerMove / 4;

	for (int depth = 1; depth < MAX_DEPTH; depth++) {
		

		if (ABORT_SIGNAL)	
			break;
	

		if (!is_main_thread) { 

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
		    
		    float scoreChangeFactor = GET_MAX(0.5, GET_MIN(1.5, 0.1 + scoreDiff * 0.05));


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

		    if (timeSpent > minTimePerMove) {

		    	if (timeSpent > maxTimePerMove || 
					timeSpent > timePerMove * scoreChangeFactor * stableMoveFactor) {

					Threads.stop = true;
					break;	
				}
		    }
		}
	} 
}


void aspirationWindowSearch(u8 sideToMove, SearchThread *th) {

	int window = AP_WINDOW;

	int score = -INF;
	int alpha = -INF, beta = INF;

	int failedHighCounter = 0;
	
	if (th->depth > 4 && th->completedDepth > 0) {

		int scoreKnown = th->pvLine.at(th->completedDepth).score;

  		alpha = std::max(-INF, scoreKnown - window);
	  	beta  = std::min( INF, scoreKnown + window);	
	}
 	

	std::vector<u32> pline;


	SearchInfo searchInfo;

	searchInfo.side = sideToMove;
	searchInfo.ply = 0;
	searchInfo.realDepth = th->depth;
	searchInfo.isNullMoveAllowed = false;


	while (true) {

		pline.clear();
		th->selDepth = NO_DEPTH;
		
		searchInfo.depth = std::max(1, th->depth - failedHighCounter);

		score = alphabetaSearch(alpha, beta, th, &pline, &searchInfo, MATE);

		if (score <= alpha)	{

			beta = (alpha + beta) / 2;
			alpha = std::max(alpha - window, -INF);

			failedHighCounter = 0;
		}
		else if (score >= beta)	{

			beta = std::min(beta + window, INF);

			failedHighCounter++;
		}	
		else {

			th->completedDepth = th->depth;

			th->pvLine.at(th->completedDepth).score = score;

			th->pvLine.at(th->completedDepth).line.clear();
			std::copy(pline.begin(), pline.end(), back_inserter(th->pvLine.at(th->completedDepth).line));

			break;
		}

		window += window / 4; 

		if (Threads.stop) 
			break;
	}

	assert (score > alpha && score < beta);


	if (th != Threads.main()) 
		return;


	display(sideToMove, th->completedDepth, th->selDepth,
		th->pvLine[th->completedDepth].score, th->pvLine[th->completedDepth].line);

	if (!th->canReportCurrMove) {

		int interval = std::chrono::duration_cast<std::chrono::seconds>(
		    std::chrono::steady_clock::now() - startTime).count();    	

		if (interval > 3) th->canReportCurrMove = true;
	}
}

void updateHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &quietMovesPlayed, Thread *th) {


	int bonus = std::min(400, depth * depth), delta = 0;

	int16_t hScore;

	u32 move, previousMove;

	// History Heuristics		
	for (std::vector<u32>::iterator i = quietMovesPlayed.begin(); i != quietMovesPlayed.end(); ++i) {
		
		move = *i;

		delta = (move == bestMove) ? bonus : -bonus;
		hScore = th->historyScore[side][from_sq(move)][to_sq(move)];

		th->historyScore[side][from_sq(move)][to_sq(move)] += 32 * delta - hScore * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->historyScore[side][from_sq(move)][to_sq(move)] > 10000)
		// 	std::cout << th->historyScore[side][from_sq(move)][to_sq(move)] << ", ";
		// mtx.unlock();
	}


	// Counter move heuristics
	previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

	if (previousMove != NO_MOVE) {

		th->counterMove[side][from_sq(previousMove)][to_sq(previousMove)] = bestMove;
	}
}


void updateCaptureHistory(int ply, int side, int depth, u32 bestMove,std::vector<u32>&captureMovesPlayed, Thread *th) {

	int bonus = std::min(400, depth * depth), delta = 0;

	int16_t chScore;

	uint16_t atk_piece, to, cap_piece;
	u32 move, previousMove;

	u8 mt;

	// Capture History Heuristics	
	for (std::vector<u32>::iterator i = captureMovesPlayed.begin(); i != captureMovesPlayed.end(); ++i) {
		
		move = *i;

		delta = (move == bestMove) ? bonus : -bonus;

		atk_piece = pieceType(move);
		to = to_sq(move);
		cap_piece = cPieceType(move);

		mt = move_type(move);

		if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) 
			cap_piece = PAWNS;


		chScore = th->capture_history_score[atk_piece][to][cap_piece];

		th->capture_history_score[atk_piece][to][cap_piece] += 32 * delta - chScore * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->capture_history_score[atk_piece][to][cap_piece] > 1000)
		// 	std::cout << th->capture_history_score[atk_piece][to][cap_piece] << ", ";
		// mtx.unlock();
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


int alphabetaSearch(int alpha, int beta, SearchThread *th, std::vector<u32> *pline, SearchInfo *si, int mate) {


	assert(alpha < beta); 


	const u8 side = si->side;
	const u8 opp = side ^ 1;

	const int ply = si->ply;
	
	const bool is_root_node = ply == 0;
  	const bool is_main_thread = th == Threads.main();
	const bool can_null_move = si->isNullMoveAllowed;


	int depth = si->depth;


	// Quiescense Search ----------------------------------------------------------------------------------- 
	// under observation

	if (depth <= 0 || ply >= MAX_PLY) {

		return quiescenseSearch(ply, depth, side, alpha, beta, th, pline);
	}  



	// Check time spent ------------------------------------------------------------------------------------		

	if (	timeSet 
		&&  is_main_thread 
		&&  th->nodes % X_NODES == 0) {

		checkTime();
	}

	// return if need to stop 
	if (	is_main_thread 
		&&  Threads.stop 
		&&  !is_root_node) {

		return 0;
	}

	// return from helper threads 
	if (	!is_root_node 
		&&  ABORT_SIGNAL)	{

		Threads.stop = true;
		
		return 0;
	}

	//---------------------------------------------------------------------------------------------------




	th->selDepth = is_root_node ? 0 : std::max(th->selDepth, ply);
   
	th->nodes++;



	
	//http://www.talkchess.com/forum3/viewtopic.php?t=63090
	th->moveStack[ply + 1].killerMoves[0] = NO_MOVE;
	th->moveStack[ply + 1].killerMoves[1] = NO_MOVE;



	const bool is_pv_node = alpha != beta - 1;

	const int num_pieces = __builtin_popcountll(th->occupied);


	// Repetition detection
	
	if (!is_root_node) {

		for (int i = th->moves_history_counter + ply; i >= 0; i--) {
			
			if (th->movesHistory[i].hashKey == th->hashKey) {

				if (	num_pieces > 22)	return -50;
				if (	num_pieces > 12)	return -25;	// middlegame
					     					return   0;	// endgame
			}
		}

		th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;
	}


	// Transposition Table lookup

	bool ttMatch = false;

	int ttDepth = NO_DEPTH;
	int ttBound = NO_BOUND;
	int ttValue = -INF;
	int sEval = -INF;
	
	u32 ttMove = NO_MOVE;
	
	if (probeHash(&sEval, &ttDepth, &ttValue, &ttBound, &ttMove, th)) {

		ttMatch = true;

		th->ttHits++;

		if (!is_pv_node) {

			if (ttDepth >= depth) {
	            
		        if (ttBound == hashfEXACT)	return ttValue;
		        
		        if ((ttBound == hashfALPHA) && (ttValue <= alpha))	return alpha;
		        
		        if ((ttBound == hashfBETA) && (ttValue >= beta))	return beta;
			}
		}
	}


	const auto num_opp_pieces = __builtin_popcountll(
		opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	const auto in_check = isKingInCheck(side, th);


	if (in_check) {

		sEval = -INF;
	} else if (ttMatch) {

		if (sEval == -INF) {
			
			sEval = nn_eval(&nn, th, (side == WHITE ? 0 : 1));		
		}
	} else {
		
		sEval = nn_eval(&nn, th, (side == WHITE ? 0 : 1));			
		
		recordHash(age, NO_MOVE, NO_DEPTH, -INF, NO_BOUND, sEval, th);		
	}



	bool improving = false;
	if (is_root_node || in_check || ply < 2) {

		improving = false;
	} else {
		
		improving = sEval > th->moveStack[ply-2].sEval;
	}
	


	assert (!(!in_check && sEval == -INF));


	
	bool canPruneOrReduce = false;

	if (	!is_root_node 
		&&  !is_pv_node 
		&&  !in_check
		&&	std::abs(alpha) < WIN_SCORE_THRESHOLD
		&&	std::abs(beta) < WIN_SCORE_THRESHOLD
		&&	num_opp_pieces > 3) {

		canPruneOrReduce = true;	
	}	
	

	// Reverse Futility Pruning (Under observation)

	if (canPruneOrReduce)	{ 

		assert(sEval != -INF);
		
		if (	depth == 1 
			&&	sEval - R_F_PRUNE_THRESHOLD >= beta) {

			return beta; 	
		}		 

		if (	depth == 2 
			&&	sEval - R_EXT_F_PRUNE_THRESHOLD >= beta) {

			return beta;		
		}	
	
		if (	depth == 3 
			&&	sEval - R_LTD_RZR_THRESHOLD >= beta) {

			depth--;
		}		
	}


	if (!is_root_node) {

		th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
		th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
		th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
	}

	th->moveStack[ply].ttMove = ttMove;
	th->moveStack[ply].sEval = sEval;
	th->moveStack[ply].extend = 0;


	std::vector<u32> line;


	SearchInfo searchInfo;	


	// Null Move pruning 
	// Check the logic for endgame 

	if (	!is_root_node	
		&&	!is_pv_node 
		&&	!in_check 
		&&	can_null_move 
		&&	num_opp_pieces > 3
		&&	depth > 2 
		&&	sEval >= beta) { 


		makeNullMove(ply, th);


		auto r = depth > 9 ? 3 : 2;	

	
		searchInfo.side = opp;
		searchInfo.ply = ply + 1;
		searchInfo.depth = depth - r;
		searchInfo.isNullMoveAllowed = false;
		

		int score = -alphabetaSearch(alpha, beta, th, &line, &searchInfo, mate - 1);


		unmakeNullMove(ply, th);


		if (score >= beta) {

			return beta;
		}	

		else if (std::abs(score) >= WIN_SCORE_THRESHOLD) {	// Mate Threat extension

			depth++;
		}
	}



	//(Under observation)

	bool f_prune = false;
	if (canPruneOrReduce)	{ 
			
		if (	depth == 1 
			&&	sEval + F_PRUNE_THRESHOLD <= alpha) {

			f_prune = true;  	// Futility Pruning
		}
		
		else if (depth == 2 
			&&	sEval + EXT_F_PRUNE_THRESHOLD <= alpha) {

			f_prune = true;		// Extended Futility Pruning	
		}
		
		else if (depth == 3 
			&&	sEval + LTD_RZR_THRESHOLD <= alpha)	{

			depth--; 			// Limited Razoring		
		}
	}
	



	// Alternative to IID ----------------------------------------------------------------------------------- under observation
	// (Reduce instead of finding the best move for low depths)
	
/*	if (	is_pv_node 
		&&	depth >= 6 
		&&	!ttMove) {

		depth -= 2;	
	}
*/
	if (	depth >= 4 
		&&  !ttMove) {

		depth--;	
	}

	//---------------------------------------------------------------------------------------------------------------------



	u8 currentMoveType;

	bool isQuietMove = false, failedHigh = false, isValidMove = false;

	int hashf = hashfALPHA;
	int reduce = 0, extend = 0, legalMoves = 0, newDepth = 0, see_score = 0;
	int score = -INF, bestScore = -INF;

	u32 bestMove = NO_MOVE;
	const u32 KILLER_MOVE_1 = th->moveStack[ply].killerMoves[0];
	const u32 KILLER_MOVE_2 = th->moveStack[ply].killerMoves[1];
	
	Move currentMove, tmpMove;

	std::vector<u32> quietMovesPlayed, captureMovesPlayed;
	std::vector<Move> moves, badCaptures, killerMoves, normalMoves;


	// Seach Loop for the available moves ----------------------------------------------------------------
	
	for (auto stage : STAGES) {	

		// get the moves according to current stage -------------------------------------------------------

		moves.clear();

		if (stage == STAGE_NORMAL_MOVES) {

			moves = normalMoves;	
		}	else if (stage == STAGE_BAD_CAPTURES) {

			moves = badCaptures; // TODO check logic
		}	else {

			getMoves(ply, side, moves, stage, false, th);		
		} 
	

		if (stage == STAGE_KILLER_MOVES) {

			// Logic can be improved
			for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i) {
				
				tmpMove = *i;		

				if (	tmpMove.move == KILLER_MOVE_1	
					||	tmpMove.move == KILLER_MOVE_2) {

					killerMoves.push_back(tmpMove);
				}	else {

					normalMoves.push_back(tmpMove);		
				} 				
			}

			moves = killerMoves;
		}

		//------------------------------------------------------------------------------------




		// Sort the moves--------------------------------------------------------------------- 
			
		int n = moves.size();
		for (int i = 0; i < n; i++) {   // loop n times - 1 per element
	        for (int j = 0; j < n - i - 1; j++) { // last i elements are sorted already
	    
	            if (moves[j].score < moves[j + 1].score) {

	                tmpMove = moves[j];
	                moves[j] = moves[j + 1];
	                moves[j + 1] = tmpMove;
	            }
	        }
    	}		
	
		//------------------------------------------------------------------------------------





    	// Search the moves-------------------------------------------------------------------------


		for (std::vector<Move>::iterator m = moves.begin(); m != moves.end(); ++m) {
				

	 		currentMove = *m;


	 		if (	stage > STAGE_HASH_MOVE 
	 			&&	currentMove.move == ttMove) {

	 			continue;	// dont search the hash move twice
	 		}	
			

			if (stage == STAGE_CAPTURES) {

				see_score = see(currentMove.move, side, th);
				if (see_score < 0)	{

					currentMove.score = see_score;
					badCaptures.push_back(currentMove);		

					continue;
				}
			}



			assert (currentMove.move != NO_MOVE);


			make_move(ply, currentMove.move, th);


			isValidMove = !isKingInCheck(side, th);


			if (isValidMove) {

				currentMoveType = move_type(currentMove.move);

				isQuietMove = false;
				if (	currentMoveType == MOVE_NORMAL 
					||	currentMoveType == MOVE_CASTLE 
					||	currentMoveType == MOVE_DOUBLE_PUSH) {

					isQuietMove = true;

					quietMovesPlayed.push_back(currentMove.move);
				} else {

					isQuietMove = false;

					captureMovesPlayed.push_back(currentMove.move);
				}


				// Pruning ----------------------------------------------------------------------
				
				if (	legalMoves > 0
					&&	!is_root_node 
					&&	!is_pv_node
					&&	!in_check 
					&&	stage != STAGE_PROMOTIONS) {


					if (isQuietMove) {
						
						// Futility pruning
						if (	f_prune) {
			
							unmake_move(ply, currentMove.move, th);
							continue;
						}

						// Late move pruning
						if (	depth <= LMP_MAX_DEPTH 
							&&  legalMoves >= LMP_BASE * depth) {

							unmake_move(ply, currentMove.move, th);
							continue;
						}

						// History pruning
						if (	depth <= HISTORY_PRUNING_MAX_DEPTH
							&&	currentMove.score < HISTORY_PRUNING_THRESHOLD) {

							unmake_move(ply, currentMove.move, th);
							continue;
						}
					}	
		
					else {

						// Capture History Pruning
						/*if (	depth <= CAPTURE_HISTORY_PRUNING_MAX_DEPTH
							&&	currentMove.score < CAPTURE_PRUNING_THRESHOLD) {

							unmake_move(ply, currentMove.move, th);
							continue;
						}*/

						// Negative SEE pruning 
						if (	stage == STAGE_BAD_CAPTURES 
							&&	currentMove.score < SEE_PRUNING_THRESHOLD) {
							
							unmake_move(ply, currentMove.move, th);
							continue;
						}	
					}		
				}


		        // report current move via uci protocol
		        if (	is_root_node 
		        	&&	is_main_thread
		        	&&	th->canReportCurrMove) {	

		        	reportCurrentMove(side, si->realDepth, legalMoves + 1, currentMove.move);
		        }



		        th->moveStack[ply].move = currentMove.move;



				extend = 0;



				//	Extensions---------------------------------------------------------------------------------

				if (	!is_root_node 
					&&	th->moveStack[ply - 1].extend <= MAX_EXTENSION) {	


					u8 sqCurrMove = to_sq(currentMove.move);
					u8 pieceCurrMove = pieceType(currentMove.move);

					bool is_prank = side ? 
						sqCurrMove >= 8 && sqCurrMove <= 15 : 
						sqCurrMove >= 48 && sqCurrMove <= 55;

					if (	in_check 
						||	stage == STAGE_PROMOTIONS 
						||	(pieceCurrMove == PAWNS && is_prank)) {	// Check and promotions 

						extend = 1;
					}
				} 



				th->moveStack[ply].extend = is_root_node ? 0 : th->moveStack[ply - 1].extend + extend;



  			    newDepth = (depth - 1) + extend;



				reduce = 0;



				searchInfo.side = opp;
				searchInfo.ply = ply + 1;
				searchInfo.isNullMoveAllowed = true;


				if (legalMoves == 0) {

					searchInfo.depth = newDepth;

					score = -alphabetaSearch(-beta, -alpha, th, &line, &searchInfo, mate - 1);
				} else {
					
					// Late Move Reductions (Under observation)

					if (	depth > 2
						&&	legalMoves > 3
						&&	!in_check
						&&	isQuietMove
						&&	pieceType(currentMove.move) != PAWNS) {
							

						reduce = depth > 6 ? 2 : 1;	


						if (!is_pv_node) reduce++;
						if (!improving) reduce++; // in_check sets improving to false


						if (stage == STAGE_KILLER_MOVES) reduce--;
			            reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO rewrite logic				


			        	int r = std::min(depth - 1, std::max(reduce, 1));	// TODO rewrite logic

			        	searchInfo.depth = newDepth - r;	


						score = -alphabetaSearch(-alpha - 1, -alpha, th, &line, &searchInfo, mate - 1);			

					} else {

						score = alpha + 1;
					}

					if (score > alpha) {

						searchInfo.depth = newDepth;
						
						score = -alphabetaSearch(-alpha - 1, -alpha, th, &line, &searchInfo, mate - 1);					
				
						if (score > alpha && score < beta) {

							score = -alphabetaSearch(-beta, -alpha, th, &line, &searchInfo, mate - 1);					
						}
					}
				}




				unmake_move(ply, currentMove.move, th);

				legalMoves++;




				if (score > bestScore) {

					
					bestScore = score;
				
					if (score > alpha) {
					

						bestMove = currentMove.move;
							
	
						alpha = score;
						hashf = hashfEXACT;


						pline->clear();
						pline->push_back(bestMove);
						std::copy(line.begin(), line.end(), back_inserter(*pline));
						line.clear();


						if (score >= beta) {

							hashf = hashfBETA;
							
							failedHigh = true;

							break;
						} 					
					} 
				}
			} else {

				unmake_move(ply, currentMove.move, th);
			}
		}

		if (failedHigh) {
		    
		    break;
		}
	}


	if (failedHigh) {

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



	if (legalMoves == 0) return in_check ? -mate : 0;



	recordHash(age, bestMove, depth, bestScore, hashf, sEval, th);



	return bestScore;
}





// should limit Quiescense search explosion
int quiescenseSearch(const int ply, const int depth, const int side, int alpha, int beta, 
	SearchThread *th, std::vector<u32> *pline) {


	assert (ply != 0);


	const int opp = side ^ 1;

	const bool is_main_thread = th == Threads.main();

	

	// Check if time limit has been reached-----------------------------------------------------------------------		

	if (	timeSet 
		&&  is_main_thread 
		&&  th->nodes % X_NODES == 0) {

		checkTime();	
	}

	

	// return if need to stop
	if (is_main_thread && Threads.stop) {

		return 0;
	}

	// return from helper threads 
	if (ABORT_SIGNAL)	{

		Threads.stop = true;
		
		return 0;
	}

	//-----------------------------------------------------------------------------------------------------------------



	th->selDepth = std::max(th->selDepth, ply);
	th->nodes++;



	const int num_pieces = __builtin_popcountll(th->occupied);


	// Repetition detection
	for (int i = th->moves_history_counter + ply; i >= 0; i--) {
		
		if (th->movesHistory[i].hashKey == th->hashKey) {

			if (	num_pieces > 22)	return -50;
			if (	num_pieces > 12)	return -25;	// middlegame
				     					return   0;	// endgame
		}
	}

	th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;


	int ttDepth = NO_DEPTH;
	int ttBound = NO_BOUND;
	int ttValue = -INF;
	int sEval = -INF;

	u32 ttMove = NO_MOVE;

	
	if (probeHash(&sEval, &ttDepth, &ttValue, &ttBound, &ttMove, th)) {

		th->ttHits++;
	
		if (ttDepth >= depth) {
            
	        if (	ttBound == hashfEXACT) 	{
	            return ttValue;
	        }
	        
	        if (	ttBound == hashfALPHA && ttValue <= alpha)  {
	            return alpha;
	        }
	        
	        if (	ttBound == hashfBETA && ttValue >= beta)    {
	            return beta;
	        }
		}
	}


	if (sEval == -INF) { 
	
		sEval = nn_eval(&nn, th, (side == WHITE ? 0 : 1));			
	}


	assert (sEval != -INF);


	if (ply >= MAX_PLY)	{

		return sEval;
	}

	if (sEval >= beta)	{

		recordHash(age, NO_MOVE, NO_DEPTH, sEval, hashfBETA, sEval, th);		

		return sEval;
	}
	
	if (sEval > alpha)	{

		alpha = sEval;
	}

	
	th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
	th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
	th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;


	const auto Q_FUTILITY_BASE = sEval + Q_DELTA;

	const auto num_opp_pieces = __builtin_popcountll(opp ? 
		th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

	
	int hashf = hashfALPHA;
	int score, bestScore = sEval, n, see_score;

	u32 bestMove = NO_MOVE;
	Move currentMove, tmpMove;

	std::vector<u32> line;
	std::vector<Move> moveList;	

	bool failedHigh = false;

	for (int stage = STAGE_PROMOTIONS; stage <= STAGE_CAPTURES; stage++) {


		moveList.clear();
		getMoves(ply, side, moveList, stage, true, th);


		n = moveList.size();

		for (int i = 0; i < n; i++) {   // loop n times - 1 per element
        	for (int j = 0; j < n - i - 1; j++) { // last i elements are sorted already
    
            	if (moveList[j].score < moveList[j + 1].score) {
            	
                	tmpMove = moveList[j];
                	moveList[j] = moveList[j + 1];
                	moveList[j + 1] = tmpMove;
           		}
        	}
    	}



		for (std::vector<Move>::iterator m = moveList.begin(); m != moveList.end(); ++m) {
	

   	 		currentMove = *m;


   	 		if (stage != STAGE_PROMOTIONS) {

				// Delta pruning
	   	 		if (	num_opp_pieces > 3
	   	 			&&	Q_FUTILITY_BASE + seeVal[cPieceType(currentMove.move)] <= alpha) {

	   	 			continue;
	   	 		} 

				see_score = see(currentMove.move, side, th);

   	 			// Don't search moves with negative SEE 
				if (	see_score < Q_SEE_PRUNING_THRESHOLD) {

					continue;
				}	
   	 		}

				

			assert (currentMove.move != NO_MOVE);


			make_move(ply, currentMove.move, th);


			if (!isKingInCheck(side, th)) {


				score = -quiescenseSearch(ply + 1, depth - 1, opp, -beta, -alpha, th, &line);


				unmake_move(ply, currentMove.move, th);


				if (score > bestScore) {


					bestScore = score;
					
					if (score > alpha) {	

						
						bestMove = currentMove.move;

						
						alpha = score;
						hashf = hashfEXACT;


						pline->clear();
						pline->push_back(bestMove);
						std::copy(line.begin(), line.end(), back_inserter(*pline));
						line.clear();


						if (score >= beta) {

							hashf = hashfBETA;
							
							failedHigh = true;

							break;
						} 					
					} 
				}
			} else {

				unmake_move(ply, currentMove.move, th);				
			}
		}

		if (failedHigh) 
			break;
	}


	recordHash(age, bestMove, depth, bestScore, hashf, sEval, th);


	return bestScore;
}

































// For Tuning
int QuiescenseForTuning(int ply, int side, int alpha, int beta, int depth, Thread *th, TraceCoefficients *T) {

	
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

	int val = -INF;

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
}
