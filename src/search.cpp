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
#include <cstdint>

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
std::mutex mtx;

int timePerMove;
int stableMoveCount = 0;

int MAX_DEPTH = 100;

int LMR[64][64];

void initLMR() {

    float a = 0.1, b = 2;
    for (int depth = 1; depth < 64; depth++) {
        for (int moves = 1; moves < 64; moves++) {

            LMR[depth][moves] = a + log(depth) * log(moves) / b;
        }
    }
}

bool SearchThread::abortSearch = false;
bool SearchThread::stop = false;

//TODO refactor logic
void startSearch(Side stm, SearchThread *th) {

    if (th == Threads.getMainSearchThread()) {

        SearchThread::abortSearch = false;

        Threads.start_searching(); // start non-main threads
        
        if (stm == WHITE)
            iterativeDeepeningSearch<WHITE>(th); // main thread start searching
        else
            iterativeDeepeningSearch<BLACK>(th); // main thread start searching
            
        SearchThread::abortSearch = true;

        // When we reach the maximum depth, we can arrive here without a raise of
        // SearchThread::stop. However, if we are pondering or in an infinite search,
        // the UCI protocol states that we shouldn't print the best move before the
        // GUI sends a "stop" or "ponderhit" command. We therefore simply wait here
        // until the GUI sends one of those commands.

        while (!SearchThread::stop)
        {} // Busy wait for a stop or a ponder reset

        // Stop the threads if not already stopped (also raise the stop if
        // "ponderhit" just reset Threads.ponder).
        SearchThread::stop = true;

        // Wait until all threads have finished
        Threads.wait_for_search_finished();

        reportBestMove();

        timeSet = false;
        stopped = false;
    } else {

        if (stm == WHITE)
            iterativeDeepeningSearch<WHITE>(th); // main thread start searching
        else
            iterativeDeepeningSearch<BLACK>(th); // main thread start searching
    }
}

template<Side stm>
void iterativeDeepeningSearch(SearchThread *th) {
    
    th->nodes = 0;
    th->ttHits = 0;	

    th->depth = I16_NO_DEPTH;
    th->completedDepth = I16_NO_DEPTH;

    stableMoveCount = 0;

    for (int depth = 1; depth < MAX_DEPTH; depth++) {

        if (th != Threads.getMainSearchThread()) { 

            // Mutex will automatically be unlocked when lck goes out of scope
            std::lock_guard<std::mutex> lck {mtx}; 

            U16 count = 0;

            for (SearchThread *thread : Threads.getSearchThreads()) {

                if (th != thread && depth == thread->depth) {
                
                    count++;	
                }
            }

            if (count >= (option_thread_count / 2)) {

                continue;
            }
        } 


        th->depth = depth;

        if (stm == WHITE)
            aspirationWindowSearch<WHITE>(th);
        else
            aspirationWindowSearch<BLACK>(th);
        

        if (SearchThread::stop)
            break;


        if (th != Threads.getMainSearchThread())
            continue;


        if (timeSet && th->completedDepth >= 4) {


            // score change
            int prevScore = th->pvLine.at(th->completedDepth-3).score;
            int currentScore = th->pvLine.at(th->completedDepth).score;

            const auto scoreChangeFactor = prevScore > currentScore ? 
                fmax(0.5, fmin(1.5, ((prevScore - currentScore) * 0.05))) : 0.5;
            

            // best move change 			
            assert(th->pvLine.at(th->completedDepth).line[0] != NO_MOVE 
                && th->pvLine.at(th->completedDepth-1).line[0] != NO_MOVE);

            U32 previousMove = th->pvLine.at(th->completedDepth-1).line[0];
            U32 currentMove = th->pvLine.at(th->completedDepth).line[0];
            
            stableMoveCount = previousMove == currentMove ? stableMoveCount + 1 : 0;
            stableMoveCount = std::min(10, stableMoveCount);

            const auto stableMoveFactor =  1.25 - stableMoveCount * 0.05;

            
            // win factor
            const auto winFactor = currentScore >= U16_WIN_SCORE ? 0.5 : 1;
            
            // Check for time 
            if (	vgen::time_elapsed_milliseconds(startTime) 
                >	(timePerMove * scoreChangeFactor * stableMoveFactor * winFactor)) {

                SearchThread::stop = true;
                break;	
            }
        }
    } 

    SearchThread::stop = true;
}

template<Side stm>
void aspirationWindowSearch(SearchThread *th) {

    int window = I32_MATE;

    int score = -I32_MATE;
    int alpha = -I32_MATE, beta = I32_MATE;
    
    if (th->depth > 4 && th->completedDepth > 0) {

        window = U8_AP_WINDOW;

        int scoreKnown = th->pvLine.at(th->completedDepth).score;

        alpha = std::max(-I32_MATE, scoreKnown - window);
        beta  = std::min( I32_MATE, scoreKnown + window);	
    }
    

    SearchInfo searchInfo;

    searchInfo.ply = 0;
    searchInfo.realDepth = th->depth;
    searchInfo.skipMove = NO_MOVE;

    int failHighCounter = 0;

    while (true) {
        
        searchInfo.depth = std::max(1, th->depth - failHighCounter);
        searchInfo.line[0] = NO_MOVE;
        
        th->selDepth = I16_NO_DEPTH;
        
        score = alphabetaSearch<stm, NO_NULL, NON_SING>(alpha, beta, I32_MATE, th, &searchInfo);

        if (SearchThread::stop)
            break;

        if (score <= alpha) {

            beta = (alpha + beta) / 2;
            alpha = std::max(score - window, -I32_MATE);

            failHighCounter = 0;
        }
        else if (score >= beta) {

            beta = std::min(score + window, I32_MATE);
            
            if (std::abs(score) < U16_WIN_SCORE)
                failHighCounter++;
        }
        else {

            th->completedDepth = th->depth;
            
            PV pv;
            pv.score = score;
            
            for (int i = 0; i < U16_MAX_PLY; i++) {
                
                pv.line[i] = searchInfo.line[i];
            
                if (pv.line[i] == NO_MOVE)
                    break;
            }
            
            
            th->pvLine[th->completedDepth] = pv;
            
            break;
        }

        window += window / 4; 
    }

    
    if (SearchThread::stop)
        return;


    assert (score > alpha && score < beta);


    if (th != Threads.getMainSearchThread()) 
        return;

    reportPV(th);
}

void checkTime() {

    SearchThread::stop = vgen::time_now().time_since_epoch() >= stopTime.time_since_epoch();
}

template<Side stm, bool isNullMoveAllowed, bool isSingularSearch>
int alphabetaSearch(int alpha, int beta, const int mate, SearchThread *th, SearchInfo *si) {


    // if (alpha >= beta) {
    // 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", ply =" << si->ply << "\n";
    // 	std::cout<<alpha<<","<<beta<<std::endl;
    // }

    assert(alpha < beta); 
    
    constexpr auto OPP = stm == WHITE ? BLACK : WHITE;
    
    const auto PLY = si->ply;
    const auto IS_ROOT_NODE = PLY == 0;
    const auto IS_MAIN_THREAD = th == Threads.getMainSearchThread();
    const auto IS_SINGULAR_SEARCH = isSingularSearch;
    const auto CAN_NULL_MOVE = isNullMoveAllowed;
    const auto IS_PV_NODE = alpha != beta - 1;
    

    int depth = si->depth;


    // Quiescense Search(under observation)

    if (depth <= 0 || PLY >= U16_MAX_PLY) {

        si->line[0] = NO_MOVE;
        
        return quiescenseSearch<stm>(alpha, beta, th, si);
    }
    

    // Check time spent		

    if (timeSet && IS_MAIN_THREAD && th->nodes % U16_CHECK_NODES == 0)
        checkTime();
    
    if (IS_MAIN_THREAD && SearchThread::stop) 
        return 0;

    if (SearchThread::abortSearch) 
        return 0; 




    th->selDepth = IS_ROOT_NODE ? 0 : std::max(th->selDepth, PLY);

    th->nodes++;



    
    //http://www.talkchess.com/forum3/viewtopic.php?t=63090
    th->moveStack[PLY + 1].killerMoves[0] = NO_MOVE;
    th->moveStack[PLY + 1].killerMoves[1] = NO_MOVE;


    const auto ALL_PIECES_COUNT = POPCOUNT(th->occupied);


    if (!IS_ROOT_NODE) {

        // Repetition detection
        if (isRepetition(PLY, th)) {
            
            // earlygame
            if (ALL_PIECES_COUNT > 22)  return -50;
            // middlegame
            else if (ALL_PIECES_COUNT > 12) return -25;
            // endgame
            else return 0;
        }

        th->movesHistory[th->moves_history_counter + PLY + 1].hashKey = th->hashKey;

        // Check for drawish endgame
        if (isPositionDraw(th)) 
            return 0;
    }


    // Transposition Table lookup
    
    HASHE *tt = IS_SINGULAR_SEARCH ? NULL : &hashTable[th->hashKey % HASH_TABLE_SIZE];
    
    const auto ttMatch = probeHash(tt, th);
    int ttScore = ttMatch ? tt->value : I32_UNKNOWN;
    U32 ttMove =  ttMatch ? tt->bestMove : NO_MOVE;

    if (ttMatch) 
        th->ttHits++;

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




    const bool IS_IN_CHECK = isKingInCheck<stm>(th);
    bool improving = false;

    int sEval = I32_UNKNOWN;

    if (IS_SINGULAR_SEARCH) {

        sEval = th->moveStack[PLY].sEval;
    } else if (IS_IN_CHECK) {
        
        sEval = I32_UNKNOWN;
        improving = false;
    } else if (ttMatch) {
        
        sEval = tt->sEval;
        if (sEval == I32_UNKNOWN)
            sEval = fullEval(stm, th); 
        // Do not save sEval to the TT since it can overwrite the previous hash entry
    } else {

        sEval = fullEval(stm, th);

        recordHash(NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval, th);
    }



    if (!IS_IN_CHECK) {

        improving = PLY >= 2 ? sEval > th->moveStack[PLY - 2].sEval : true;
    }



    const auto OPP_PIECES_COUNT = POPCOUNT(
        OPP ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);	

    // Reverse Futility Pruning
    if (	!IS_ROOT_NODE 
        &&	!IS_PV_NODE 
        &&	!IS_IN_CHECK 
        &&	!IS_SINGULAR_SEARCH
        &&	std::abs(alpha) < U16_WIN_SCORE
        &&	std::abs(beta) < U16_WIN_SCORE 
        &&	OPP_PIECES_COUNT > 3) { 

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


    SearchInfo searchInfo;
    searchInfo.skipMove = NO_MOVE;


    bool mateThreat = false;

    // Null Move pruning 
    if (	!IS_ROOT_NODE 
        &&	!IS_PV_NODE 
        &&	!IS_IN_CHECK 
        &&	!IS_SINGULAR_SEARCH
        &&	OPP_PIECES_COUNT > 3 // Check the logic for endgame
        &&	CAN_NULL_MOVE 
        &&	depth > 2 
        &&	sEval >= beta) { 


        makeNullMove(PLY, th);


        const auto R = depth > 9 ? 3 : 2;
        
    
        searchInfo.ply = PLY + 1;
        searchInfo.depth = depth - R - 1;
        
        searchInfo.line[0] = NO_MOVE;
        
        const auto score = -alphabetaSearch<OPP, NO_NULL, NON_SING>(-beta, -beta + 1, mate - 1, th, &searchInfo);

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
        &&	OPP_PIECES_COUNT > 3)	{ 
            
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


        const auto sBeta = ttScore - 4 * depth;
        const auto sDepth = depth / 2 - 1;

        searchInfo.skipMove = ttMove;
        searchInfo.ply = PLY;
        searchInfo.depth = sDepth;
        searchInfo.line[0] = NO_MOVE;

        const auto score = alphabetaSearch<stm, NO_NULL, SING>(sBeta - 1, sBeta, mate, th, &searchInfo);

        searchInfo.skipMove = NO_MOVE;

        if (score < sBeta) {
            
            ttMoveIsSingular = true;
        } else if (sBeta >= beta) { // TODO check logic

            return sBeta;
        }
    }


    bool isQuietMove = false;
    
    U8 hashf = hashfALPHA;
    int currentMoveType, currentMoveToSq;
    int reduce = 0, extend = 0, movesPlayed = 0, newDepth = 0;
    int score = -I32_MATE, bestScore = -I32_MATE;

    U32 bestMove = NO_MOVE, previousMove = IS_ROOT_NODE ? NO_MOVE : th->moveStack[PLY - 1].move;

    const U32 KILLER_MOVE_1 = th->moveStack[PLY].killerMoves[0];
    const U32 KILLER_MOVE_2 = th->moveStack[PLY].killerMoves[1];

    Move currentMove;

    std::vector<U32> quietsPlayed, capturesPlayed;
    
    th->moveList[PLY].skipQuiets = false;
    th->moveList[PLY].stage = PLAY_HASH_MOVE;
    th->moveList[PLY].ttMove = ttMove;
    th->moveList[PLY].counterMove = previousMove == NO_MOVE ? 
        NO_MOVE : th->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)];
    th->moveList[PLY].moves.clear();
    th->moveList[PLY].badCaptures.clear();

    while (true) {

        // fetch next psuedo-legal move
        currentMove = getNextMove(stm, PLY, th, &th->moveList[PLY]);

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
        if (isKingInCheck<stm>(th)) {
            
            unmake_move(PLY, currentMove.move, th);
            continue;
        }

        // psuedo legal move is valid, increment number of moves played
        movesPlayed++;


        // report current depth, moves played and current move being searched
        if (IS_ROOT_NODE && IS_MAIN_THREAD) {
        
            if (vgen::time_elapsed_milliseconds(startTime) > U16_CURRMOVE_INTERVAL) {

                std::cout << "info depth " << si->realDepth << " currmove ";
                std::cout << getMoveNotation(currentMove.move) << " currmovenumber " << movesPlayed << std::endl;
            }
        }


        currentMoveType = move_type(currentMove.move);
        currentMoveToSq = to_sq(currentMove.move);


        isQuietMove = currentMoveType == MOVE_NORMAL || currentMoveType == MOVE_CASTLE 
            || currentMoveType == MOVE_DOUBLE_PUSH;

        if (isQuietMove) {

            quietsPlayed.push_back(currentMove.move);
        } else if (currentMoveType != MOVE_PROMOTION) { 
            // all promotions are scored equal and ordered differently from capture moves
            
            capturesPlayed.push_back(currentMove.move);
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
        
        th->moveStack[PLY].move = currentMove.move;

        extend = 0;
        float extension = IS_ROOT_NODE ? 0 : th->moveStack[PLY - 1].extension;

        //	Fractional Extensions
        if (!IS_ROOT_NODE) { // TODO check extensions logic	

            int16_t pieceCurrMove = pieceType(currentMove.move);

            if (currentMove.move == ttMove && ttMoveIsSingular) // Singular extension
                extension += F_SINGULAR_EXT;
            
            if (IS_IN_CHECK) 
                extension += F_CHECK_EXT;	// Check extension

            if (mateThreat)
                extension += F_MATE_THREAT_EXT; // Mate threat extension
        
            if (currentMoveType == MOVE_PROMOTION) 
                extension += F_PROMOTION_EXT;	// Promotion extension
            
            bool isPrank = stm ? 
                currentMoveToSq >= 8 && currentMoveToSq <= 15 : 
                currentMoveToSq >= 48 && currentMoveToSq <= 55;
            
            if (pieceCurrMove == PAWNS && isPrank)
                extension += F_PRANK_EXT;  // Pawn push extension

            U8 prevMoveType = move_type(previousMove);
            
            if (previousMove != NO_MOVE && prevMoveType == MOVE_CAPTURE) {

                U8 prevMoveToSq = to_sq(previousMove);
            
                if (currentMoveToSq == prevMoveToSq)
                    extension += F_RECAPTURE_EXT; // Recapture extension
            }

            if (extension >= F_ONE_PLY) {

                extend = 1;
                extension -= F_ONE_PLY;

                if (extension >= F_ONE_PLY) 
                    extension = 3 * F_ONE_PLY / 4;
            }
        } 

        th->moveStack[PLY].extension = extension;
    
        
        newDepth = (depth - 1) + extend;

        reduce = 0;


        searchInfo.ply = PLY + 1;

        
        if (movesPlayed <= 1) { // Principal Variation Search

            searchInfo.depth = newDepth;
            searchInfo.line[0] = NO_MOVE;

            score = -alphabetaSearch<OPP, NUL, NON_SING>(-beta, -alpha, mate - 1, th, &searchInfo);
        } else {
            
            // Late Move Reductions (Under observation)

            if (	depth > 2
                &&	movesPlayed > 1
                &&	isQuietMove) {
                
                reduce = LMR[std::min(depth, 63)][std::min(movesPlayed, 63)];

                if (!IS_PV_NODE) 
                    reduce++;
                
                if (!improving && !IS_IN_CHECK) 
                    reduce++; // IS_IN_CHECK sets improving to false
                
                if (IS_IN_CHECK && pieceType(currentMove.move) == KING) 
                    reduce++;

                if (th->moveList[PLY].stage < GEN_QUIETS) 
                    reduce--; // reduce less for killer and counter moves
                
                reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO	rewrite logic				


                reduce = std::min(depth - 1, std::max(reduce, 1));

                searchInfo.depth = newDepth - reduce;	
                searchInfo.line[0] = NO_MOVE;
                
                score = -alphabetaSearch<OPP, NUL, NON_SING>(-alpha - 1, -alpha, mate - 1, th, &searchInfo);
            } else {

                score = alpha + 1;
            }

            if (score > alpha) {    // Research 

                searchInfo.depth = newDepth;
                searchInfo.line[0] = NO_MOVE;

                score = -alphabetaSearch<OPP, NUL, NON_SING>(-alpha - 1, -alpha, mate - 1, th, &searchInfo);
                if (score > alpha && score < beta) {

                    score = -alphabetaSearch<OPP, NUL, NON_SING>(-beta, -alpha, mate - 1, th, &searchInfo);
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
                
                if (IS_PV_NODE) {
                    
                    auto pline = &si->line[0];
                    auto line = &searchInfo.line[0];
                    
                    *pline++ = currentMove.move;
                    
                    while (*line != NO_MOVE)
                        *pline++ = *line++;
                    
                    *pline = NO_MOVE;
                }
                
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

            if (bestMove != KILLER_MOVE_1 && bestMove != KILLER_MOVE_2) {
                // update killers
        
                th->moveStack[PLY].killerMoves[1] = KILLER_MOVE_1;
                th->moveStack[PLY].killerMoves[0] = bestMove;
            }

            updateHistory(stm, PLY, depth, bestMove, quietsPlayed, th);				
        } 

        updateCaptureHistory(depth, bestMove, capturesPlayed, th);
    }


    if (movesPlayed == 0) // Mate and stalemate check
        return IS_IN_CHECK ? -mate : 0;


    if (!IS_SINGULAR_SEARCH)
        recordHash(bestMove, depth, bestScore, hashf, sEval, th);


    return bestScore;
}


constexpr int seeVal[8] = {	VALUE_DUMMY, VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP,
                            VALUE_ROOK, VALUE_QUEEN, VALUE_KING, VALUE_DUMMY };
                    
// TODO should limit Quiescense search explosion
template<Side stm>
int quiescenseSearch(int alpha, int beta, SearchThread *th, SearchInfo* si) {


    assert (alpha < beta);
    assert (si->ply > 0);

    constexpr auto opp = stm == WHITE ? BLACK : WHITE;

    const bool IS_MAIN_THREAD = th == Threads.getMainSearchThread();

    const auto ply = si->ply;

    // Check if time limit has been reached
    if (timeSet && IS_MAIN_THREAD && th->nodes % U16_CHECK_NODES == 0) 
        checkTime();	
    
    if (IS_MAIN_THREAD && SearchThread::stop) 
        return 0;

    if (SearchThread::abortSearch) 
        return 0; 


    th->selDepth = std::max(th->selDepth, ply);
    th->nodes++;


    const auto ALL_PIECES_COUNT = POPCOUNT(th->occupied);

    // Repetition detection
    if (isRepetition(ply, th)) {
        
        // earlygame
        if (ALL_PIECES_COUNT > 22)  return -50;
        // middlegame
        else if (ALL_PIECES_COUNT > 12) return -25;
        // endgame
        else return 0;
    }

    th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;

    // Check for drawish endgame
    if (isPositionDraw(th)) 
        return 0;

    if (ply >= U16_MAX_PLY - 1) 
        return fullEval(stm, th);



    HASHE *tt = &hashTable[th->hashKey % HASH_TABLE_SIZE];

    bool ttMatch = probeHash(tt, th);  
    int ttScore = I32_UNKNOWN;

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
    int sEval, bestScore = -I32_MATE;
    
    if (ttMatch) {
        
        sEval = tt->sEval;
        if (sEval == I32_UNKNOWN)
            sEval = fullEval(stm, th); 
        // Do not save sEval to the TT since it can overwrite the previous hash entry        
    } else {

        sEval = fullEval(stm, th);

        recordHash(NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval, th);
    }


    bestScore = sEval;
    alpha = std::max(alpha, sEval);

    if (alpha >= beta) 
        return sEval;

    
    th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
    th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
    th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;


    th->moveList[ply].skipQuiets = true;
    th->moveList[ply].stage = GEN_CAPTURES;
    th->moveList[ply].ttMove = NO_MOVE;
    th->moveList[ply].counterMove = NO_MOVE;
    th->moveList[ply].moves.clear();
    th->moveList[ply].badCaptures.clear();

    const auto OPP_PIECES_COUNT = POPCOUNT(opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
    const auto Q_FUTILITY_BASE = sEval + U16_Q_DELTA; 

    U8 hashf = hashfALPHA, capPiece = DUMMY;
    int movesPlayed = 0; 
    int score = -I32_MATE;

    Move currentMove;
    
    SearchInfo searchInfo;
    searchInfo.ply = ply + 1;
    
    while (true) {

        currentMove = getNextMove(stm, ply, th, &th->moveList[ply]);

        if (th->moveList[ply].stage >= PLAY_BAD_CAPTURES) 
            break;

        assert(currentMove.move != NO_MOVE);

        make_move(ply, currentMove.move, th);

        if (isKingInCheck<stm>(th)) {

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
            if (OPP_PIECES_COUNT > 3 && Q_FUTILITY_BASE + seeVal[capPiece] <= alpha) {

                unmake_move(ply, currentMove.move, th);

                continue;
            }
        }

        searchInfo.line[0] = NO_MOVE;
        score = -quiescenseSearch<opp>(-beta, -alpha, th, &searchInfo);


        unmake_move(ply, currentMove.move, th);


        if (score > bestScore) {

            bestScore = score;
            bestMove = currentMove.move;
            
            if (score > alpha) {
                
                alpha = score;
                hashf = hashfEXACT;
                
                auto pline = &si->line[0];
                auto line = &searchInfo.line[0]; 
                
                *pline++ = currentMove.move;
                while (*line != NO_MOVE)
                        *pline++ = *line++;
                *pline = NO_MOVE;
                
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


