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
#include "thread.h"
#include "see.h"
#include "constants.h"
#include "history.h"
#include "ucireport.h"
#include "misc.h"
#include "TimeManagement.h"
#include "HashManagement.h"

bool SearchThread::abortSearch = false, SearchThread::stopSearch = false;
int option_thread_count, stableMoveCount = 0, MAX_DEPTH = 100, LMR[64][64], LMP[2][U8_LMP_DEPTH];

std::mutex mutex;

int fmargin[4] = { 0, 200, 300, 500 };

void initLMR() 
{
    const float a = 0.1, b = 2;
    for (int depth = 1; depth < 64; depth++) 
    {
        for (int moves = 1; moves < 64; moves++) 
        {
            LMR[depth][moves] = a + log(depth) * log(moves) / b;
        }
    }
}

void initLMP()
{
    for (int depth = 0; depth < U8_LMP_DEPTH; depth++)
    {       
        LMP[1][depth] = 3 + depth * depth;
        LMP[0][depth] = LMP[1][depth] / 2;
    }
}

void SearchThread::startSearch(Side stm) 
{
    if (this == searchThreads.getMainSearchThread()) 
    {

        HashManager::age += 1;

        SearchThread::abortSearch = false;
        SearchThread::stopSearch = false;
        
        searchThreads.search<false>(); 

        if (stm == WHITE)
            iterativeDeepening<WHITE>(this); 
        else
            iterativeDeepening<BLACK>(this); 
        
        SearchThread::stopSearch = true;    
        SearchThread::abortSearch = true;

        searchThreads.waitForAll();

        reportBestMove();

        TimeManager::sTimeManager.updateTimeSet(false);
        TimeManager::sTimeManager.updateStopped(false);
    } 
    else 
    {
        if (stm == WHITE)
            iterativeDeepening<WHITE>(this); 
        else
            iterativeDeepening<BLACK>(this); 
    }
}

template<Side stm>
void iterativeDeepening(SearchThread *th) 
{    
    th->nodes = 0;
    th->ttHits = 0;

    th->depth = I16_NO_DEPTH;
    th->completedDepth = I16_NO_DEPTH;

    stableMoveCount = 0;

    for (int depth = 1; depth < MAX_DEPTH; depth++) 
    {
        th->depth = depth;

        aspirationWindow<stm>(th);

        if (SearchThread::stopSearch)
        {
            break;
        }

        if (th != searchThreads.getMainSearchThread())
        {
            continue;
        }
        
        const auto completedDepth = th->completedDepth;
        if (TimeManager::sTimeManager.isTimeSet() && completedDepth >= 4) 
        {
            // score change
            int prevScore = th->pvLine[completedDepth-3].score;
            int currentScore = th->pvLine[completedDepth].score;

            const auto scoreChangeFactor = prevScore > currentScore ? 
                fmax(0.5, fmin(1.5, ((prevScore - currentScore) * 0.05))) : 0.5;
            
            // best move change
            assert(th->pvLine[completedDepth].line[0] != NO_MOVE 
                && th->pvLine[completedDepth-1].line[0] != NO_MOVE);

            const auto previousMove = th->pvLine[completedDepth-1].line[0];
            const auto currentMove = th->pvLine[completedDepth].line[0];
            
            stableMoveCount = previousMove == currentMove ? stableMoveCount + 1 : 0;
            stableMoveCount = std::min(10, stableMoveCount);

            const auto stableMoveFactor =  1.25 - stableMoveCount * 0.05;

            
            // win factor
            const auto winFactor = currentScore >= U16_WIN_SCORE ? 0.5 : 1;
            
            
            const auto totalFactor = scoreChangeFactor * stableMoveFactor * winFactor;
            
            // Check for time 
            if (TimeManager::sTimeManager.time_elapsed_milliseconds(TimeManager::sTimeManager.getStartTime()) 
                    > (TimeManager::sTimeManager.getTimePerMove() * totalFactor)) 
            {
                SearchThread::stopSearch = true;
                break;
            }
        }
    } 

    SearchThread::stopSearch = true;
}

template<Side stm>
void aspirationWindow(SearchThread *th) 
{
    int window = I32_MATE;

    int score = -I32_MATE;
    int alpha = -I32_MATE, beta = I32_MATE;
    
    if (th->depth > 4 && th->completedDepth > 0) 
    {
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

    while (true) 
    {
        searchInfo.depth = std::max(1, th->depth - failHighCounter);
        searchInfo.line[0] = NO_MOVE;
        
        th->selDepth = I16_NO_DEPTH;
        
        score = alphabeta<stm, NO_NULL, NON_SING>(alpha, beta, I32_MATE, th, &searchInfo);

        if (SearchThread::stopSearch)
        {
            break;
        }
        
        if (score <= alpha) 
        {
            beta = (alpha + beta) / 2;
            alpha = std::max(score - window, -I32_MATE);

            failHighCounter = 0;
        }
        else if (score >= beta) 
        {
            beta = std::min(score + window, I32_MATE);
            
            if (std::abs(score) < U16_WIN_SCORE)
            {
                failHighCounter++;
            }
        }
        else 
        {
            const auto currentDepth = th->depth;
            
            th->completedDepth = currentDepth;
            
            PV pv;
            pv.score = score;
            
            for (int i = 0; i < U16_MAX_PLY; i++) 
            {
                pv.line[i] = searchInfo.line[i];
            
                if (pv.line[i] == NO_MOVE)
                {
                    break;
                }
            }
            
            th->pvLine[currentDepth] = pv;
            
            break;
        }

        window += window / 4; 
    }

    
    if (SearchThread::stopSearch)
    {
        return;
    }

    assert (score > alpha && score < beta);

    if (th != searchThreads.getMainSearchThread())
    {
        return;
    }
    
    reportPV(th);
}

inline void checkTime() 
{
    SearchThread::stopSearch = TimeManager::time_now().time_since_epoch() 
                    >= TimeManager::sTimeManager.getStopTime().time_since_epoch();
}

template<Side stm, bool isNullMoveAllowed, bool isSSearch>
int alphabeta(int alpha, int beta, const int mate, SearchThread *th, SearchInfo *si) 
{
    // if (alpha >= beta) {
    // 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", ply =" << si->ply << "\n";
    // 	std::cout<<alpha<<","<<beta<<std::endl;
    // }

    assert(alpha < beta); 
    
    constexpr auto opp = stm == WHITE ? BLACK : WHITE;
    
    const auto ply = si->ply;
    const auto isRootNode = ply == 0;
    const auto isMainThread = th == searchThreads.getMainSearchThread();
    const auto isSingularSearch = isSSearch;
    const auto canNullMove = isNullMoveAllowed;
    const auto isPvNode = alpha != beta - 1;
    

    int depth = si->depth;


    // Quiescense Search(under observation)

    if (depth <= 0 || ply >= U16_MAX_PLY) 
    {
        si->line[0] = NO_MOVE;
        
        return quiescenseSearch<stm>(alpha, beta, th, si);
    }
    

    // Check time spent
    if (    isMainThread
        &&  TimeManager::sTimeManager.isTimeSet()
        &&  th->nodes % U16_CHECK_NODES == 0) 
    {
        checkTime();
    }
    
    if (isMainThread && SearchThread::stopSearch) 
    {
        return 0;
    }
    
    if (SearchThread::abortSearch) 
    {
        return 0; 
    }

    
    th->selDepth = isRootNode ? 0 : std::max(th->selDepth, ply);

    th->nodes++;

    
    //http://www.talkchess.com/forum3/viewtopic.php?t=63090
    th->moveStack[ply + 1].killerMoves[0] = NO_MOVE;
    th->moveStack[ply + 1].killerMoves[1] = NO_MOVE;


    const auto allPiecesCount = POPCOUNT(th->occupied);
    
    
    if (!isRootNode) 
    {
        if (isRepetition(ply, th)) // Repetition detection
        {
            if (allPiecesCount > 22) // earlygame
            {
                return -50;
            }
            else if (allPiecesCount > 12) // middlegame
            {
                return -25;
            }
            else 
            {
                return 0; // endgame
            }
        }
        
        th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;
        
    }
    

    // Transposition Table lookup
    
    auto hashEntry = isSingularSearch ? nullptr : th->hashManager.getHashEntry(th->hashKey);
    
    const auto hashHit = th->hashManager.probeHash(hashEntry, th->hashKey);
    
    int ttScore = I32_UNKNOWN;
    U32 ttMove = NO_MOVE;
    
    if (hashHit) 
    {
        th->ttHits++;
        
        ttScore = hashEntry->value;
        ttMove =  hashEntry->bestMove;
    
        if (!isRootNode && !isPvNode && hashEntry->depth >= depth && ttScore != I32_UNKNOWN) 
        {
            if (    hashEntry->flags == hashfEXACT 
                ||  (hashEntry->flags == hashfBETA && ttScore >= beta)
                ||  (hashEntry->flags == hashfALPHA && ttScore <= alpha)) 
            {
                return ttScore;
            }
        }
    }


    // Alternative to IID
    if (depth >= 4 && !ttMove && !isSingularSearch) 
    {
        depth--;
    }


    const auto isInCheck = isKingInCheck<stm>(th);
   
    const auto sEval =    isSingularSearch  ?   th->moveStack[ply].sEval 
                        : isInCheck         ?   I32_UNKNOWN 
                        : hashHit           ?   ((hashEntry->sEval == I32_UNKNOWN) ? fullEval(stm, th) : hashEntry->sEval) 
                        : fullEval(stm, th);

    const auto improving = isInCheck ? false : ply >= 2 ? sEval > th->moveStack[ply - 2].sEval : true;

    if (!isSingularSearch && !isInCheck && !hashHit)
    {
        th->hashManager.recordHash(th->hashKey, NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval);
    }


    const auto oppPiecesCount = POPCOUNT(opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
    bool fPrune = false;
    
    if (	!isRootNode 
        &&	!isPvNode 
        &&	!isInCheck 
        &&	!isSingularSearch
        &&	std::abs(alpha) < U16_WIN_SCORE
        &&	std::abs(beta) < U16_WIN_SCORE 
        &&  depth <= 3
        &&	oppPiecesCount > 3) 
    { 

        assert(sEval != I32_UNKNOWN);
        

        if (sEval - fmargin[depth] >= beta)       // Reverse Futility Pruning
            return beta;          
    

        if (sEval + U16_RAZOR_MARGIN < beta)      // Razoring
        {
            const auto rscore = quiescenseSearch<stm>(alpha, beta, th, si);

            if (rscore < beta) 
            {
                return rscore;
            }
        }


        if (sEval + fmargin[depth] <= alpha)      // Futility Pruning
            fPrune = true;
    }



    if (!isRootNode) 
    {
        th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
        th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
        th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
    }

    th->moveStack[ply].ttMove = ttMove;
    th->moveStack[ply].sEval = sEval;

    SearchInfo searchInfo;
    searchInfo.skipMove = NO_MOVE;



    bool mateThreat = false;

    // Null Move pruning 
    if (	!isRootNode 
        &&	!isPvNode 
        &&	!isInCheck 
        &&	!isSingularSearch
        &&	oppPiecesCount > 3 // Check the logic for endgame
        &&	canNullMove 
        &&	depth > 2 
        &&	sEval >= beta) 
    { 
        makeNullMove(ply, th);

        const auto R = depth > 6 ? 2 : 1;        
    
        searchInfo.ply = ply + 1;
        searchInfo.depth = depth - R - 1;
        searchInfo.line[0] = NO_MOVE;
        
        const auto score = -alphabeta<opp, NO_NULL, NON_SING>(-beta, -beta + 1, mate - 1, th, &searchInfo);

        unmakeNullMove(ply, th);

        if (score >= beta)
            return beta;
    
        if (std::abs(score) >= U16_WIN_SCORE) // Mate threat 	
            mateThreat = true;
    }
    

    bool ttMoveIsSingular = false;

    // Singular search
    if (    !isRootNode
        &&  !isSingularSearch
        &&  depth >= 7
        &&  hashHit
        &&  ttMove != NO_MOVE 
        &&  std::abs(ttScore) < U16_WIN_SCORE
        &&  hashEntry->flags == hashfBETA
        &&  hashEntry->depth >= depth - 3) 
    {
        const auto sBeta = ttScore - 4 * depth;
        const auto sDepth = depth / 2 - 1;

        searchInfo.skipMove = ttMove;
        searchInfo.ply = ply;
        searchInfo.depth = sDepth;
        searchInfo.line[0] = NO_MOVE;

        const auto score = alphabeta<stm, NO_NULL, SING>(sBeta - 1, sBeta, mate, th, &searchInfo);

        searchInfo.skipMove = NO_MOVE;

        if (score < sBeta) 
        {
            ttMoveIsSingular = true;
        } 
        else if (sBeta >= beta)
        { 
            return sBeta;
        }
    }

    bool isQuietMove = false;
    
    U8 hashf = hashfALPHA;
    int currentMoveType, currentMoveToSq;
    int reduce = 0, extend = 0, movesPlayed = 0, newDepth = 0;
    int score = -I32_MATE, bestScore = -I32_MATE;

    U32 bestMove = NO_MOVE, previousMove = isRootNode ? NO_MOVE : th->moveStack[ply - 1].move;

    const U32 KILLER_MOVE_1 = th->moveStack[ply].killerMoves[0];
    const U32 KILLER_MOVE_2 = th->moveStack[ply].killerMoves[1];

    Move currentMove;

    std::vector<U32> quietsPlayed, capturesPlayed;
    
    th->moveList[ply].skipQuiets = false;
    th->moveList[ply].stage = PLAY_HASH_MOVE;
    th->moveList[ply].ttMove = ttMove;
    th->moveList[ply].counterMove = previousMove == NO_MOVE ? 
        NO_MOVE : th->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)];
    th->moveList[ply].moves.clear();
    th->moveList[ply].badCaptures.clear();

    while (true) 
    {
        // fetch next psuedo-legal move
        currentMove = getNextMove(stm, ply, th, &th->moveList[ply]);

        if (th->moveList[ply].stage == STAGE_DONE)
        {
            break;
        }
        
        // check if move generator returns a valid psuedo-legal move
        assert(currentMove.move != NO_MOVE);

        // skip the move if its in a singular search and the current move is singular
        if (currentMove.move == si->skipMove) 
        {
            continue;
        }
        
        // Prune moves based on conditions met
        if (    movesPlayed > 1
            &&  !isRootNode 
            &&  !isPvNode
            &&  !isInCheck 
            &&  move_type(currentMove.move) != MOVE_PROMOTION) 
        {
            if (isQuietMove) 
            {
                // Futility pruning
                if (fPrune) 
                {
                    th->moveList[ply].skipQuiets = true;
                    continue;
                }

                // Late move pruning
                if (    depth < U8_LMP_DEPTH
                    &&  movesPlayed >= LMP[improving][depth])
                {
                    th->moveList[ply].skipQuiets = true;
                    continue;
                }

                // History pruning
                if (    depth <= U8_HISTORY_PRUNING_DEPTH 
                    &&  currentMove.score < I16_HISTORY_PRUNING * depth) 
                {
                    continue;
                }
            } 
            else
            {   // SEE pruning
                if (    depth <= U8_SEE_PRUNING_DEPTH 
                    &&  currentMove.seeScore < I16_SEE_PRUNING * depth) 
                {
                    continue;
                }
            }
        }

        // make the move
        make_move(ply, currentMove.move, th);

        // check if psuedo-legal move is valid
        if (isKingInCheck<stm>(th)) 
        {
            unmake_move(ply, currentMove.move, th);
            continue;
        }

        // psuedo legal move is valid, increment number of moves played
        movesPlayed++;

        // report current depth, moves played and current move being searched
        if (isRootNode && isMainThread) 
        {
            if (TimeManager::sTimeManager.time_elapsed_milliseconds(
                TimeManager::sTimeManager.getStartTime()) > U16_CURRMOVE_INTERVAL) 
            {
                std::cout << "info depth " << si->realDepth << " currmove ";
                std::cout << getMoveNotation(currentMove.move) << " currmovenumber " << movesPlayed << "\n";
            }
        }


        currentMoveType = move_type(currentMove.move);
        currentMoveToSq = to_sq(currentMove.move);

        isQuietMove = currentMoveType == MOVE_NORMAL || currentMoveType == MOVE_CASTLE 
            || currentMoveType == MOVE_DOUBLE_PUSH;

        if (isQuietMove) 
        {
            quietsPlayed.push_back(currentMove.move);
        } 
        else if (currentMoveType != MOVE_PROMOTION) 
        { 
            // all promotions are scored equal and ordered differently from capture moves
            capturesPlayed.push_back(currentMove.move);
        }

        
        th->moveStack[ply].move = currentMove.move;


        extend = 0;
        float extension = isRootNode ? 0 : th->moveStack[ply - 1].extension;

        //	Fractional Extensions
        if (!isRootNode) // TODO check extensions logic	
        { 
            int16_t pieceCurrMove = pieceType(currentMove.move);

            if (currentMove.move == ttMove && ttMoveIsSingular) // Singular extension
            {
                extension += F_SINGULAR_EXT;
            }
            
            if (isInCheck) 
            {
                extension += F_CHECK_EXT;	// Check extension
            }
            
            if (mateThreat)
            {
                extension += F_MATE_THREAT_EXT; // Mate threat extension
            }
            
            if (currentMoveType == MOVE_PROMOTION) 
            {
                extension += F_PROMOTION_EXT;   // Promotion extension
            }
            
            bool isPrank = stm ? 
                currentMoveToSq >= 8 && currentMoveToSq <= 15 : 
                currentMoveToSq >= 48 && currentMoveToSq <= 55;
            
            if (pieceCurrMove == PAWNS && isPrank)
            {
                extension += F_PRANK_EXT;       // Pawn push extension
            }
            
            U8 prevMoveType = move_type(previousMove);
            
            if (previousMove != NO_MOVE && prevMoveType == MOVE_CAPTURE) 
            {
                U8 prevMoveToSq = to_sq(previousMove);
            
                if (currentMoveToSq == prevMoveToSq)
                    extension += F_RECAPTURE_EXT;   // Recapture extension
            }

            if (extension >= F_ONE_PLY) 
            {
                extend = 1;
                extension -= F_ONE_PLY;

                if (extension >= F_ONE_PLY)
                {
                    extension = 3 * F_ONE_PLY / 4;
                }
            }
        } 

        th->moveStack[ply].extension = extension;
        
        newDepth = (depth - 1) + extend;

        reduce = 0;


        searchInfo.ply = ply + 1;

        
        if (movesPlayed <= 1) 
        { // Principal Variation Search

            searchInfo.depth = newDepth;
            searchInfo.line[0] = NO_MOVE;

            score = -alphabeta<opp, NUL, NON_SING>(-beta, -alpha, mate - 1, th, &searchInfo);
        } 
        else 
        { // Late Move Reductions (Under observation)
        
            if (	depth > 2
                &&	movesPlayed > 1
                &&	isQuietMove) 
            {
                reduce = LMR[std::min(depth, 63)][std::min(movesPlayed, 63)];

                if (!isPvNode) 
                {
                    reduce++;
                }
                
                if (!improving && !isInCheck) 
                {
                    reduce++; // isInCheck sets improving to false
                }
                
                if (isInCheck && pieceType(currentMove.move) == KING) 
                {
                    reduce++;
                }
                
                if (th->moveList[ply].stage < GEN_QUIETS) 
                {
                    reduce--; // reduce less for killer and counter moves
                }
                
                reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO rewrite logic				

                reduce = std::min(depth - 1, std::max(reduce, 1));

                searchInfo.depth = newDepth - reduce;	
                searchInfo.line[0] = NO_MOVE;
                
                score = -alphabeta<opp, NUL, NON_SING>(-alpha - 1, -alpha, mate - 1, th, &searchInfo);
            } else
            {
                score = alpha + 1;
            }

            if (score > alpha) 
            {   // Research 
                
                searchInfo.depth = newDepth;
                searchInfo.line[0] = NO_MOVE;

                score = -alphabeta<opp, NUL, NON_SING>(-alpha - 1, -alpha, mate - 1, th, &searchInfo);
                
                if (score > alpha && score < beta) 
                {
                    score = -alphabeta<opp, NUL, NON_SING>(-beta, -alpha, mate - 1, th, &searchInfo);
                }
            }
        }

        
        unmake_move(ply, currentMove.move, th);

        
        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = currentMove.move;

            if (score > alpha) 
            {
                alpha = score;
                hashf = hashfEXACT;
                
                // record the moves for the PV
                if (isPvNode) 
                {
                    auto pline = &si->line[0];
                    auto line = &searchInfo.line[0];
                    
                    *pline++ = currentMove.move;
                    
                    while (*line != NO_MOVE)
                    {
                        *pline++ = *line++;
                    }
                    
                    *pline = NO_MOVE;
                }
                
                if (score >= beta) 
                {
                    hashf = hashfBETA;
                    
                    // Fail high
                    // No further moves need to be searched, since one refutation is already sufficient 
                    // to avoid the move that led to this node or position. 
                    break;
                }
            }
        }
    }
    
    if (hashf == hashfBETA) 
    {
        if (isQuietMove) 
        {
            if (bestMove != KILLER_MOVE_1 && bestMove != KILLER_MOVE_2) 
            {   // update killers
        
                th->moveStack[ply].killerMoves[1] = KILLER_MOVE_1;
                th->moveStack[ply].killerMoves[0] = bestMove;
            }

            updateHistory(stm, ply, depth, bestMove, quietsPlayed, th);
        } 

        updateCaptureHistory(depth, bestMove, capturesPlayed, th);
    }


    if (movesPlayed == 0)
    {    // Mate and stalemate check
        return isInCheck ? -mate : 0;
    }

    if (!isSingularSearch)
    {
        th->hashManager.recordHash(th->hashKey, bestMove, depth, bestScore, hashf, sEval);
    }

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

    const bool isMainThread = th == searchThreads.getMainSearchThread();

    const auto ply = si->ply;

    // Check if time limit has been reached
    if (    TimeManager::sTimeManager.isTimeSet() 
        &&  isMainThread && th->nodes % U16_CHECK_NODES == 0)
    {
        checkTime();
    }
    
    if (isMainThread && SearchThread::stopSearch) 
    {
        return 0;
    }
    
    if (SearchThread::abortSearch) 
    {
        return 0; 
    }

    
    th->selDepth = std::max(th->selDepth, ply);
    th->nodes++;


    const auto allPiecesCount = POPCOUNT(th->occupied);

    // Repetition detection
    if (isRepetition(ply, th)) 
    {
        // earlygame
        if (allPiecesCount > 22)
        {
            return -50;
        }
        // middlegame
        else if (allPiecesCount > 12)
        {
            return -25;
        }
        // endgame
        else
        {
            return 0;
        }
    }

    th->movesHistory[th->moves_history_counter + ply + 1].hashKey = th->hashKey;

    
    if (ply >= U16_MAX_PLY - 1) 
    {
        return fullEval(stm, th);
    }
    
    
    auto hashEntry = th->hashManager.getHashEntry(th->hashKey);
    
    const auto hashHit = th->hashManager.probeHash(hashEntry, th->hashKey);  
    
    int ttScore = I32_UNKNOWN;

    if (hashHit) 
    { // no depth check required since its 0 in quiescense search
        
        th->ttHits++;

        ttScore = hashEntry->value;

        if (    ttScore != I32_UNKNOWN 
            &&  (    hashEntry->flags == hashfEXACT 
                ||  (hashEntry->flags == hashfBETA && ttScore >= beta)
                ||  (hashEntry->flags == hashfALPHA && ttScore <= alpha))) 
        {
            return ttScore;
        }
    }


    U32 bestMove = NO_MOVE;
    int bestScore = -I32_MATE;

    auto sEval = hashHit ? ((hashEntry->sEval == I32_UNKNOWN) ? 
                                    fullEval(stm, th) : hashEntry->sEval) : fullEval(stm, th);
    if (!hashHit)
    {
        th->hashManager.recordHash(th->hashKey, NO_MOVE, I16_NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval);
    }
    

    bestScore = sEval;
    alpha = std::max(alpha, sEval);

    if (alpha >= beta)
    {
        return sEval;
    }
    
    th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
    th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
    th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;


    th->moveList[ply].skipQuiets = true;
    th->moveList[ply].stage = GEN_CAPTURES;
    th->moveList[ply].ttMove = NO_MOVE;
    th->moveList[ply].counterMove = NO_MOVE;
    th->moveList[ply].moves.clear();
    th->moveList[ply].badCaptures.clear();

    const auto oppPiecesCount = POPCOUNT(opp ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
    const auto qFutilityBase = sEval + U16_Q_DELTA; 

    U8 hashf = hashfALPHA, capPiece = DUMMY;
    int movesPlayed = 0; 
    int score = -I32_MATE;

    Move currentMove;
    
    SearchInfo searchInfo;
    searchInfo.ply = ply + 1;
    
    while (true) 
    {
        currentMove = getNextMove(stm, ply, th, &th->moveList[ply]);

        if (th->moveList[ply].stage >= PLAY_BAD_CAPTURES) 
        {
            break;
        }
        
        assert(currentMove.move != NO_MOVE);

        // Pruning
        if (    movesPlayed > 1
            &&  capPiece != DUMMY
            &&  move_type(currentMove.move) != MOVE_PROMOTION) 
        {
            // Delta pruning
            if (oppPiecesCount > 3 && qFutilityBase + seeVal[capPiece] <= alpha) 
            {
                continue;
            }
        }


        make_move(ply, currentMove.move, th);

        if (isKingInCheck<stm>(th)) 
        {
            unmake_move(ply, currentMove.move, th);

            continue;
        }

        movesPlayed++;

        capPiece = cPieceType(currentMove.move);


        searchInfo.line[0] = NO_MOVE;
        score = -quiescenseSearch<opp>(-beta, -alpha, th, &searchInfo);


        unmake_move(ply, currentMove.move, th);


        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = currentMove.move;
            
            if (score > alpha) 
            {
                alpha = score;
                hashf = hashfEXACT;
                
                auto pline = &si->line[0];
                auto line = &searchInfo.line[0]; 
                
                *pline++ = currentMove.move;
                while (*line != NO_MOVE)
                {
                    *pline++ = *line++;
                }
                *pline = NO_MOVE;
                
                if (score >= beta) 
                {
                    hashf = hashfBETA;
                    
                    break;
                }
            } 
        }
    }

    th->hashManager.recordHash(th->hashKey, bestMove, 0, bestScore, hashf, sEval);

    return bestScore;
}


