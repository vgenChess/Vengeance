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
#include "movegen.h"
#include "make_unmake.h"
#include "utility.h"
#include "nonslidingmoves.h"
#include "magicmoves.h"
#include "see.h"
#include "constants.h"
#include "history.h"
#include "ucireport.h"
#include "misc.h"
#include "TimeManagement.h"
#include "HashManagement.h"
#include "nnue.h"
#include "enums.h"
#include "namespaces.h"

using namespace game;

int option_thread_count, LMR[64][64], LMP[2][LMP_DEPTH];

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
    for (int depth = 0; depth < LMP_DEPTH; depth++)
    {       
        LMP[1][depth] = 3 + depth * depth;
        LMP[0][depth] = LMP[1][depth] / 2;
    }
}


void startSearch()
{
    // increment the age which will be used in hash entry to phase out old entries
    tt::age++;

    searching = true;
    abortSearch = false;

    previousInfoTime = 0;

    for (GameInfo *g: infos) {

        // do not delete the info for initialising data for other threads
        if (g == initInfo)
            continue;

        // free resources allocated if any for the previous search
        delete g;
    }

    // clear the previous search infos and threads
    infos.clear();
    threads.clear();


    for (int i = 0; i < option_thread_count; i++) {

        GameInfo* lGi = new GameInfo();

        lGi->clone(initInfo);

        infos.push_back(lGi);
    }


    // Spawn more threads if requested via the uci interface

    for (int i = 1; i < option_thread_count; i++) {

        if (initInfo->stm == WHITE) {

            threads.emplace_back(iterativeDeepening<WHITE>, i, infos[i]);
        } else {

            threads.emplace_back(iterativeDeepening<BLACK>, i, infos[i]);
        }
    }


    // start the search for the main thread
    if (initInfo->stm == WHITE) {

        iterativeDeepening<WHITE>(0, infos[0]);
    }
    else {

        iterativeDeepening<BLACK>(0, infos[0]);
    }


    // after the main thread return from the search
    // signal abort for other threads if any
    abortSearch = true;


    // wait for other threads before proceeding to display the best move
    for (std::thread& th: threads)
        th.join();


    // Display the best move from the search

    auto bestIndex = 0;
    auto bestInfo = infos[0];

    int bestDepth = bestInfo->completedDepth;
    int bestScore = bestInfo->pvLine[bestDepth].score;

    int currentDepth, currentScore;

    for (uint16_t i = 1; i < infos.size(); i++)
    {
        GameInfo* g = infos[i];

        currentDepth = g->completedDepth;
        currentScore = g->pvLine[currentDepth].score;

        if (currentScore > bestScore && currentDepth > bestDepth) {

            bestInfo = g;
            bestIndex = i;
        }
    }


   const auto completedDepth = bestInfo->completedDepth;

    if (bestIndex > 0) {

        const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
            tmg::timeManager.getStartTime());

        const auto score = bestInfo->pvLine[completedDepth].score;
        const auto pline = bestInfo->pvLine[completedDepth].line;

        std::cout << reportPV(completedDepth, bestInfo->selDepth, score, timeElapsedMs,
                              pline, getStats<NODES>(), getStats<TTHITS>()) << std::endl;
    }


    const auto bestMove = bestInfo->pvLine[completedDepth].line[0];

    std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;


    // finally finish the search

    tmg::timeManager.updateTimeSet(false);
    tmg::timeManager.updateStopped(false);

    searching = false;
}


template<Side stm>
void iterativeDeepening(int index, GameInfo *gi)
{    
    gi->nodes = 0;
    gi->ttHits = 0;

    gi->depth = NO_DEPTH;
    gi->completedDepth = NO_DEPTH;

    gi->stableMoveCount = 0;

    for (gi->depth = 1; gi->depth < 100; gi->depth++) {

        if (index == 0 &&
            tmg::timeManager.timeElapsed<MILLISECONDS>(tmg::timeManager.getStartTime()) > 1000) {

            std::cout << "info depth " << gi->depth << std::endl;
        }

        aspirationWindow<stm>(index, gi );

        if (abortSearch)
            return;

        if ( index > 0)
            continue;

        const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
            tmg::timeManager.getStartTime());

        if (timeElapsedMs >= 4000) {

            const auto score = gi->pvLine[gi->completedDepth].score;
            const auto pline = gi->pvLine[gi->completedDepth].line;

            std::cout << reportPV(gi->depth, gi->selDepth, score, timeElapsedMs,
                                  pline, getStats<NODES>(), getStats<TTHITS>()) << std::endl;
        }

        if (tmg::timeManager.isTimeSet() && gi->completedDepth >= 4) {

            float multiplier = 1;
            const auto timePerMove = tmg::timeManager.getTimePerMove();

            const auto completedDepth = gi->completedDepth;

            const auto prevScore = gi->pvLine[completedDepth-3].score;
            const auto currentScore = gi->pvLine[completedDepth].score;

            // reduce time for winning scores
            if (std::abs(currentScore) >= 10000) {

                multiplier = 0.5;
            } else {

                const auto scoreDiff = prevScore - currentScore;

                if (scoreDiff <= 10) multiplier = 0.5;

                if (scoreDiff > 15) multiplier += 0.125;
                if (scoreDiff > 25) multiplier += 0.125;
                if (scoreDiff > 35) multiplier += 0.125;
            }


            // Stable Move

            assert( gi->pvLine[completedDepth].line[0] != NO_MOVE
                && gi->pvLine[completedDepth-1].line[0] != NO_MOVE);

            const auto previousMove = gi->pvLine[completedDepth-1].line[0];
            const auto currentMove = gi->pvLine[completedDepth].line[0];

            gi->stableMoveCount = previousMove == currentMove ? gi->stableMoveCount + 1 : 0;

            if ( gi->stableMoveCount >= 10)
                multiplier = 0.5;

            if (tmg::timeManager.timeElapsed<MILLISECONDS>(
                tmg::timeManager.getStartTime()) >= timePerMove * multiplier)
                return;
        }
    }
}

template<Side stm>
void aspirationWindow(int index, GameInfo *gi)
{
    int window = MATE;

    int score = -MATE;
    int alpha = -MATE, beta = MATE;
    
    if (gi->depth > 4 && gi->completedDepth > 0)
    {
        window = AP_WINDOW;

        int scoreKnown = gi->pvLine.at(gi->completedDepth).score;

        alpha = std::max(-MATE, scoreKnown - window);
        beta  = std::min( MATE, scoreKnown + window);
    }
    
    SearchInfo si;

    si.treePos = 0;
    si.rootNode = true;
    si.nullMove = false;
    si.singularSearch = false;
    si.skipMove = NO_MOVE;
    si.mainThread = index == 0;

    int failHighCounter = 0;

    int depth = NO_DEPTH;

    while (true)
    {
        si.line[0] = NO_MOVE;
        
        gi->selDepth = NO_DEPTH;
        
        depth = std::max(PLY, (gi->depth - failHighCounter) * PLY);

        score = alphabeta<stm>(alpha, beta, MATE, depth, gi, &si);

        if (abortSearch)
            return;

        if (score <= alpha) 
        {
            beta = (alpha + beta) / 2;
            alpha = std::max(score - window, -MATE );

            failHighCounter = 0;
        }
        else if (score >= beta) 
        {
            beta = std::min(score + window, MATE );
            
            if (std::abs(score) < WIN_SCORE )
                failHighCounter++;
        }
        else 
        {
            gi->completedDepth = gi->depth;
            
            gi->pvLine[gi->depth].score = score;

            memcpy(gi->pvLine[gi->depth].line, si.line, sizeof(U32) * MAX_PLY);
            
            break;
        }

        window += window / 4; 
    }

    assert (score > alpha && score < beta);
}






void checkTime()
{
    const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
        tmg::timeManager.getStartTime());

    if (timeElapsedMs - previousInfoTime >= 1000)
    {
        previousInfoTime = (timeElapsedMs + 100) / 1000 * 1000;

        uint64_t nodes = getStats<NODES>();
        uint64_t nps = timeElapsedMs ? nodes / timeElapsedMs * 1000 : 0;
        uint64_t ttHits = getStats<TTHITS>();

        std::cout << "info time " << timeElapsedMs << " nodes " << nodes << " nps " << nps
            << " tbhits " << ttHits << std::endl;
    }

    abortSearch = tmg::timeManager.isTimeSet() && tmg::timeManager.time_now().time_since_epoch()
            >= tmg::timeManager.getStopTime().time_since_epoch();
}




template<Side stm>
int alphabeta(int alpha, int beta, int mate, int depth, GameInfo *gi, SearchInfo *si )
{
    // if (alpha >= beta) {
    // 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", treePos =" << si->treePos << "\n";
    // 	std::cout<<alpha<<","<<beta<<std::endl;
    // }

    assert(alpha < beta); 
    
    constexpr auto opp = stm == WHITE ? BLACK : WHITE;
    
    const auto treePos = si->treePos;
    const auto rootNode = si->rootNode;
    const auto mainThread = si->mainThread;
    const auto singularSearch = si->singularSearch;
    const auto nullMove = si->nullMove;
    const auto pvNode = alpha != beta - 1;
    

    // Check time spent
    if (mainThread && gi->nodes % CHECK_NODES == 0)
        checkTime();

    if (abortSearch)
        return 0;


    gi->selDepth = rootNode ? 0 : std::max(gi->selDepth, treePos);

    gi->nodes++;

    
    //http://www.talkchess.com/forum3/viewtopic.php?t=63090
    gi->moveStack[treePos + 1].killerMoves[0] = NO_MOVE;
    gi->moveStack[treePos + 1].killerMoves[1] = NO_MOVE;



    // Repetition detection

    const auto allPiecesCount = POPCOUNT(gi->occupied);
    
    if (!rootNode )
    {
        if (isRepetition(treePos, gi))
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
        
        gi->movesHistory[gi->moves_history_counter + treePos + 1].hashKey = gi->hashKey;
    }
    


    // Transposition Table lookup
    
    auto hashEntry = singularSearch ? nullptr : gi->hashManager.getHashEntry(gi->hashKey);
    
    const auto hashHit = gi->hashManager.probeHash(hashEntry, gi->hashKey);
    
    int ttScore = VAL_UNKNOWN;
    U32 ttMove = NO_MOVE;
    
    if (hashHit) 
    {
        gi->ttHits++;
        
        ttScore = hashEntry->value;
        ttMove =  hashEntry->bestMove;
    
        if (!rootNode && !pvNode && hashEntry->depth >= depth && ttScore != VAL_UNKNOWN)
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
    if (depth >= 4 * PLY && !ttMove && !singularSearch )
        depth -= PLY;


    const auto isInCheck = isKingInCheck<stm>(gi);

    const auto sEval =
        singularSearch
            ? gi->moveStack[treePos].sEval : isInCheck
            ? VAL_UNKNOWN : hashHit
            ? ((hashEntry->sEval == VAL_UNKNOWN)
            ? nnueEval(stm, gi) : hashEntry->sEval) : nnueEval(stm, gi);

    const auto improving = isInCheck ? false : treePos >= 2 ? sEval > gi->moveStack[treePos - 2].sEval : true;

    if (!singularSearch && !isInCheck && !hashHit)
        gi->hashManager.recordHash(gi->hashKey, NO_MOVE, NO_DEPTH, VAL_UNKNOWN, NO_BOUND, sEval);


    // Search tree pruning
    // ===============================================================================================

    const auto oppPiecesCount = POPCOUNT(opp ? gi->blackPieceBB[PIECES] : gi->whitePieceBB[PIECES]);
    bool fPrune = false;
    
    if (	!rootNode
        &&	!pvNode
        &&	!isInCheck 
        &&	!singularSearch
        &&	std::abs(alpha) < WIN_SCORE
        &&	std::abs(beta) < WIN_SCORE
        &&  depth <= 3 * PLY
        &&	oppPiecesCount > 3) 
    { 

        assert(sEval != VAL_UNKNOWN);

        // TODO Futility pruning
        if (sEval - fmargin[depth / PLY] >= beta)       // Reverse Futility Pruning
            return beta;          
    
        // TODO check logic. The quiescense call seems costly
        if (sEval + U16_RAZOR_MARGIN < beta)      // Razoring
        {
            const auto rscore = quiescense<stm>(alpha, beta, gi, si );

            if (rscore < beta) 
                return rscore;
        }

        if (sEval + fmargin[depth / PLY] <= alpha)      // Futility Pruning
            fPrune = true;
    }



    if (!rootNode )
    {
        gi->moveStack[treePos].epFlag = gi->moveStack[treePos - 1].epFlag;
        gi->moveStack[treePos].epSquare = gi->moveStack[treePos - 1].epSquare;
        gi->moveStack[treePos].castleFlags = gi->moveStack[treePos - 1].castleFlags;
    }

    gi->moveStack[treePos].ttMove = ttMove;
    gi->moveStack[treePos].sEval = sEval;



    SearchInfo lSi;

    lSi.skipMove = NO_MOVE;
    lSi.mainThread = mainThread;
    lSi.singularSearch = false;
    lSi.nullMove = false;
    lSi.rootNode = false;



    // Null Move pruning

    if (	!rootNode
        &&	!pvNode
        &&	!isInCheck 
        &&	!singularSearch
        &&	!nullMove
        &&	oppPiecesCount > 3 // Check the logic for endgame
        &&	depth > 2 * PLY
        &&	sEval >= beta)
    {
        makeNullMove(treePos, gi);

        const auto R = depth > 6 * PLY ? 2 * PLY : PLY;
    
        lSi.nullMove = true;

        lSi.treePos = treePos + 1;

        lSi.line[0] = NO_MOVE;
        const auto score = -alphabeta<opp>(-beta, -beta + 1, mate - 1, depth - R - PLY, gi, &lSi );

        unmakeNullMove(treePos, gi);

        lSi.nullMove = false;

        if (score >= beta)
            return beta;
    }


    // Singular search

    bool singularMove = false;

    if (    !rootNode
        &&  !singularSearch
        &&  depth >= 8 * PLY
        &&  hashHit
        &&  ttMove != NO_MOVE
        &&  std::abs(ttScore) < WIN_SCORE
        &&  hashEntry->flags == hashfBETA
        &&  hashEntry->depth >= depth - 3 * PLY)
    {
        const auto sBeta = ttScore - 4 * depth / PLY;
        const auto sDepth = ((depth / PLY) / 2 - PLY) * PLY;

        lSi.singularSearch = true;
        lSi.nullMove = false;
        lSi.skipMove = ttMove;
        lSi.treePos = treePos;

        lSi.line[0] = NO_MOVE;
        const auto score = alphabeta<stm>(sBeta - 1, sBeta, mate, sDepth, gi, &lSi );

        lSi.skipMove = NO_MOVE;
        lSi.singularSearch = false;

        if (score < sBeta)
        {
            singularMove = true;
        }
        else if (sBeta >= beta) // Todo check logic // fail high fail soft framework
        {
            return sBeta;
        }
    }



    bool isQuietMove = false;
    
    U8 hashf = hashfALPHA;
    int currentMoveType, currentMoveToSq;
    int movesPlayed = 0, newDepth = 0;
    int score = -MATE, bestScore = -MATE;

    U32 bestMove = NO_MOVE, previousMove = rootNode ? NO_MOVE : gi->moveStack[treePos - 1].move;

    const U32 KILLER_MOVE_1 = gi->moveStack[treePos].killerMoves[0];
    const U32 KILLER_MOVE_2 = gi->moveStack[treePos].killerMoves[1];

    Move currentMove;

    std::vector<U32> quietsPlayed, capturesPlayed;
    
    gi->moveList[treePos].skipQuiets = false;
    gi->moveList[treePos].stage = PLAY_HASH_MOVE;
    gi->moveList[treePos].ttMove = ttMove;
    gi->moveList[treePos].counterMove = previousMove == NO_MOVE ?
        NO_MOVE : gi->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)];
    gi->moveList[treePos].moves.clear();
    gi->moveList[treePos].badCaptures.clear();


    while (gi->moveList[treePos].stage < STAGE_DONE) {

        // fetch next psuedo-legal move
        currentMove = getNextMove(stm, treePos, gi, &gi->moveList[treePos]);

        // check if move generator returns a valid psuedo-legal move
        assert(currentMove.move != NO_MOVE);

        // skip the move if its in a singular search and the current move is singular
        if (currentMove.move == si->skipMove)
            continue;

        // Prune moves based on conditions met
        if (    movesPlayed > 1
            &&  !rootNode
            &&  !pvNode
            &&  !isInCheck 
            &&  move_type(currentMove.move) != MOVE_PROMOTION) 
        {
            if (isQuietMove) 
            {
                // Futility pruning
                if (fPrune) 
                {
                    gi->moveList[treePos].skipQuiets = true;
                    continue;
                }

                // Late move pruning
                if (    depth < LMP_DEPTH * PLY
                    &&  movesPlayed >= LMP[improving][depth / PLY])
                {
                    gi->moveList[treePos].skipQuiets = true;
                    continue;
                }

                // History pruning
                if (    depth <= HISTORY_PRUNING_DEPTH * PLY
                    &&  currentMove.score < HISTORY_PRUNING * depth / PLY)
                {
                    continue;
                }
            }
        }



        // make the move
        make_move(treePos, currentMove.move, gi);

        // check if this move leaves our king in check
        // in which case its an invalid move
        if (isKingInCheck<stm>(gi))
        {
            unmake_move(treePos, currentMove.move, gi);
            continue;
        }


        // the move is valid, increment number of moves played
        movesPlayed++;


        // report current move being search and the number of moves played
        if (rootNode && mainThread )
        {
            const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
                tmg::timeManager.getStartTime());

            if (timeElapsedMs > 4000)
            {
                std::cout << "info currmove " << getMoveNotation(currentMove.move);
                std::cout << " currmovenumber " << movesPlayed << std::endl;
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

        
        gi->moveStack[treePos].move = currentMove.move;




        // Fractional Extensions
        // =====================================================================

        int extension = 0;

        if (!rootNode ) // TODO check extensions logic
        {
            const auto pieceCurrMove = pieceType(currentMove.move);
            const auto prevMoveType = move_type(previousMove);

            const bool isPRank = stm ?
                currentMoveToSq >= 8 && currentMoveToSq <= 15 :
                currentMoveToSq >= 48 && currentMoveToSq <= 55;

            // Check extension
            if (isInCheck)
                extension = PLY;

            // Singular extension
            else if (currentMove.move == ttMove && singularMove )
                extension = PLY;

            // PRank extension
            else if (pieceCurrMove == PAWNS && isPRank)
                extension = PLY;

            // Recapture extension
            else if (previousMove != NO_MOVE && prevMoveType == MOVE_CAPTURE)
            {
                U8 prevMoveToSq = to_sq(previousMove);

                if (currentMoveToSq == prevMoveToSq)
                    extension = PLY / 2;
            }
        }




        gi->moveStack[treePos].extension = extension;


        newDepth = (depth - PLY) + extension;

        lSi.treePos = treePos + 1;




        // Late Move Reductions (Under observation)
        // ===============================================================================

        int lmrDepth = 1;

        const bool lmr = depth > 2 * PLY && movesPlayed > 1 && isQuietMove;

        if (lmr) {

            // get the reduction value according to depth and moves played
            auto reduce = LMR[std::min(depth / PLY, 63)][std::min(movesPlayed, 63)] * PLY;

            // reduce more if its not a pv node
            if (!pvNode )
                reduce += PLY;

            // reduce more if the score is not improving
            if (!improving && !isInCheck) // isInCheck sets improving to false
                reduce += PLY;

            // reduce more for king evasions
            if (isInCheck && pieceType(currentMove.move) == KING)
                reduce += PLY;

            bool isKillerOrCounterMove =
                gi->moveList[treePos].currentStage == PLAY_KILLER_MOVE_1 ||
                gi->moveList[treePos].currentStage == PLAY_KILLER_MOVE_2 ||
                gi->moveList[treePos].currentStage == PLAY_COUNTER_MOVE;

            // reduce less for killer and counter moves
            if (isKillerOrCounterMove)
                reduce -= PLY;

            // reduce according to move history score
            reduce -= std::max(-2, std::min(2, currentMove.score / 5000)) * PLY; // TODO rewrite logic

            reduce = std::max(reduce, 0);

            lmrDepth = std::max(newDepth - reduce, PLY);
        }


        // PVS framework with LMR research
        // ================================================================================

        if (pvNode && movesPlayed <= 1) {

            lSi.line[0] = NO_MOVE;
            score = newDepth < PLY ?
                -quiescense<opp>(-beta, -alpha, gi, &lSi) :
                -alphabeta<opp>(-beta, -alpha, mate - 1, newDepth, gi, &lSi );
        } else {

            if (lmr) {

                lSi.line[0] = NO_MOVE;
                score = -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, lmrDepth, gi, &lSi );
            }

            if (!lmr || (lmr && score > alpha && lmrDepth != newDepth)) {

                lSi.line[0] = NO_MOVE;
                score = newDepth < PLY ?
                    -quiescense<opp>(-alpha - 1, -alpha, gi, &lSi) :
                    -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, newDepth, gi, &lSi );
            }

            if (score > alpha && (rootNode || score < beta))  {

                lSi.line[0] = NO_MOVE;
                score = newDepth < PLY ?
                    -quiescense<opp>(-beta, -alpha, gi, &lSi) :
                    -alphabeta<opp>(-beta, -alpha, mate - 1, newDepth, gi, &lSi );
            }
        }



        unmake_move(treePos, currentMove.move, gi);


        if (rootNode)
        {
            if (movesPlayed == 1 || score > alpha)
            {
                copyPv(si->line, currentMove.move, lSi.line);

                const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
                    tmg::timeManager.getStartTime());

                if (mainThread
                    && timeElapsedMs > 100
                    && ((score > alpha && score < beta) || (timeElapsedMs > 4000))) {

                    std::cout << reportPV(gi->depth, gi->selDepth, score, timeElapsedMs,
                                 si->line, getStats<NODES>(), getStats<TTHITS>()) << std::endl;
                }
            }
        }


        if (score > bestScore)
        {
            bestScore = score;
            bestMove = currentMove.move;

            if (score > alpha)
            {
                alpha = score;
                hashf = hashfEXACT;

                // record the moves for the PV
                if (pvNode && !rootNode)
                    copyPv(si->line, currentMove.move, lSi.line);

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
    

    // TODO check logic
    if (hashf == hashfBETA) 
    {
        if (isQuietMove) 
        {
            if (bestMove != KILLER_MOVE_1 && bestMove != KILLER_MOVE_2) 
            {   // update killers
        
                gi->moveStack[treePos].killerMoves[1] = KILLER_MOVE_1;
                gi->moveStack[treePos].killerMoves[0] = bestMove;
            }

            updateHistory(stm, treePos, depth, bestMove, quietsPlayed, gi);
        } 

        updateCaptureHistory(depth, bestMove, capturesPlayed, gi);
    }


    if (movesPlayed == 0)
        return isInCheck ? -mate : 0; // Mate and stalemate check

    if (!singularSearch )
        gi->hashManager.recordHash(gi->hashKey, bestMove, depth, bestScore, hashf, sEval);


    return bestScore;
}











constexpr int seeVal[8] = {	VALUE_DUMMY, VALUE_PAWN, VALUE_KNIGHT, VALUE_BISHOP,
                            VALUE_ROOK, VALUE_QUEEN, VALUE_KING, VALUE_DUMMY };
                    
// TODO should limit Quiescense search explosion
template<Side stm>
int quiescense(int alpha, int beta, GameInfo *gi, SearchInfo* si ) {

    assert (alpha < beta);
    assert ( si->treePos > 0);

    constexpr auto opp = stm == WHITE ? BLACK : WHITE;

    const bool mainThread = si->mainThread;

    const auto treePos = si->treePos;

    // Check if time limit has been reached
    if (    tmg::timeManager.isTimeSet()
        &&  mainThread && gi->nodes % CHECK_NODES == 0)
    {
        abortSearch = tmg::timeManager.time_now().time_since_epoch()
            >= tmg::timeManager.getStopTime().time_since_epoch();
    }
    
    if (abortSearch)
    {
        return 0; 
    }

    
    gi->selDepth = std::max(gi->selDepth, treePos);
    gi->nodes++;


    const auto allPiecesCount = POPCOUNT(gi->occupied);

    // Repetition detection
    if (isRepetition(treePos, gi))
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

    gi->movesHistory[gi->moves_history_counter + treePos + 1].hashKey = gi->hashKey;

    
    if (treePos >= MAX_PLY - 1)
    {
        return nnueEval(stm, gi);
    }
    
    
    auto hashEntry = gi->hashManager.getHashEntry(gi->hashKey);
    
    const auto hashHit = gi->hashManager.probeHash(hashEntry, gi->hashKey);
    
    int ttScore = VAL_UNKNOWN;

    if (hashHit) 
    { // no depth check required since its 0 in quiescense search
        
        gi->ttHits++;

        ttScore = hashEntry->value;

        if (    ttScore != VAL_UNKNOWN
            &&  (    hashEntry->flags == hashfEXACT 
                ||  (hashEntry->flags == hashfBETA && ttScore >= beta)
                ||  (hashEntry->flags == hashfALPHA && ttScore <= alpha))) 
        {
            return ttScore;
        }
    }


    U32 bestMove = NO_MOVE;
    int bestScore = -MATE;

    auto sEval = hashHit ? ((hashEntry->sEval == VAL_UNKNOWN) ?
                                    nnueEval(stm, gi) : hashEntry->sEval) : nnueEval(stm, gi);
    if (!hashHit)
    {
        gi->hashManager.recordHash(gi->hashKey, NO_MOVE, NO_DEPTH, VAL_UNKNOWN, NO_BOUND, sEval);
    }
    

    bestScore = sEval;
    alpha = std::max(alpha, sEval);

    if (alpha >= beta)
    {
        return sEval;
    }
    
    gi->moveStack[treePos].epFlag = gi->moveStack[treePos - 1].epFlag;
    gi->moveStack[treePos].epSquare = gi->moveStack[treePos - 1].epSquare;
    gi->moveStack[treePos].castleFlags = gi->moveStack[treePos - 1].castleFlags;


    gi->moveList[treePos].skipQuiets = true;
    gi->moveList[treePos].stage = GEN_CAPTURES;
    gi->moveList[treePos].ttMove = NO_MOVE;
    gi->moveList[treePos].counterMove = NO_MOVE;
    gi->moveList[treePos].moves.clear();
    gi->moveList[treePos].badCaptures.clear();

    const auto oppPiecesCount = POPCOUNT(opp ? gi->blackPieceBB[PIECES] : gi->whitePieceBB[PIECES]);
    const auto qFutilityBase = sEval + VAL_Q_DELTA;

    U8 hashf = hashfALPHA, capPiece = DUMMY;
    int movesPlayed = 0; 
    int score = -MATE;

    Move currentMove;
    
    SearchInfo lSi;
    lSi.treePos = treePos + 1;


    while (gi->moveList[treePos].stage < STAGE_DONE) {

        currentMove = getNextMove(stm, treePos, gi, &gi->moveList[treePos]);
        
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


        make_move(treePos, currentMove.move, gi);

        if (isKingInCheck<stm>(gi))
        {
            unmake_move(treePos, currentMove.move, gi);

            continue;
        }

        movesPlayed++;

        capPiece = cPieceType(currentMove.move);


        lSi.line[0] = NO_MOVE;
        score = -quiescense<opp>(-beta, -alpha, gi, &lSi );


        unmake_move(treePos, currentMove.move, gi);


        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = currentMove.move;
            
            if (score > alpha) 
            {
                alpha = score;
                hashf = hashfEXACT;
                
                copyPv(si->line, currentMove.move, lSi.line);

                if (score >= beta) 
                {
                    hashf = hashfBETA;
                    
                    break;
                }
            } 
        }
    }

    gi->hashManager.recordHash(gi->hashKey, bestMove, 0, bestScore, hashf, sEval);

    return bestScore;
}




