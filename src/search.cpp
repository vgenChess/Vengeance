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

    memset(moveNodeCount, 0, sizeof(moveNodeCount[0][0]) * MAX_SQUARES * MAX_SQUARES);

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
    auto bestThread = infos[0];

    int bestDepth, currentDepth;
    int bestScore, currentScore;

    for (uint16_t i = 1; i < infos.size(); i++)
    {
        GameInfo* g = infos[i];

        bestDepth = bestThread->completedDepth;
        currentDepth = g->completedDepth;

        bestScore = bestThread->pvLine[bestDepth].score;
        currentScore = g->pvLine[currentDepth].score;

        if (    currentScore > bestScore
            &&  currentDepth > bestDepth)
        {
            bestThread = g;
            bestIndex = i;
        }
    }

    if (bestIndex > 0) {

        const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
            tmg::timeManager.getStartTime());

        const auto score = bestThread->pvLine[bestThread->completedDepth].score;
        const auto pline = bestThread->pvLine[bestThread->completedDepth].line;

        std::cout << reportPV(bestThread->depth, bestThread->selDepth, score, timeElapsedMs,
                              pline, getStats<NODES>(), getStats<TTHITS>()) << std::endl;
    }


    const auto bestMove = bestThread->pvLine[bestThread->completedDepth].line[0];

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

            const auto completedDepth = gi->completedDepth;

            const auto prevScore = gi->pvLine[completedDepth-3].score;
            const auto currentScore = gi->pvLine[completedDepth].score;

            float scoreFactor = 1, stableFactor = 1, nodesFactor = 1;


            // Score difference
            //===================================================================

            if (std::abs(currentScore) > WIN_SCORE 
                || (currentScore - prevScore >= 0)) scoreFactor = 0.5;

            if (prevScore > currentScore + 10) scoreFactor += 0.05;
            if (prevScore > currentScore + 20) scoreFactor += 0.05;
            if (prevScore > currentScore + 40) scoreFactor += 0.05;


            // Stable Move
            //===================================================================

            assert( gi->pvLine[completedDepth].line[0] != NO_MOVE
                && gi->pvLine[completedDepth-1].line[0] != NO_MOVE);

            const auto previousMove = gi->pvLine[completedDepth-1].line[0];
            const auto currentMove = gi->pvLine[completedDepth].line[0];

            gi->stableMoveCount = previousMove == currentMove ?
                std::min(10, gi->stableMoveCount + 1) : 0;

            if (gi->stableMoveCount > 3)
                stableFactor = pow(1 - gi->stableMoveCount / 100, 3);


            // Ratio of the tree size under best move to the whole tree
            //======================================================================================

            const auto bestMoveTotalNodes = moveNodeCount[from_sq(currentMove)][to_sq(currentMove)];

            const auto x = bestMoveTotalNodes / gi->nodes;

            if (x > 0.75)
                nodesFactor = pow(1 - x / 8, 3);

            if (tmg::timeManager.timeElapsed<MILLISECONDS>(
                tmg::timeManager.getStartTime()) >= timePerMove * scoreFactor * stableFactor * nodesFactor)
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

    si.ply = 0;
    si.rootNode = true;
    si.nullMove = false;
    si.singularSearch = false;
    si.skipMove = NO_MOVE;
    si.mainThread = index == 0;

    int failHighCounter = 0;

    while (true)
    {
        si.line[0] = NO_MOVE;
        
        gi->selDepth = NO_DEPTH;
        
        score = alphabeta<stm>(alpha, beta, MATE, std::max(PLY, (gi->depth - failHighCounter) * PLY), gi, &si);

        if (abortSearch)
        {
            return;
        }

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
            {
                failHighCounter++;
            }
        }
        else 
        {
            const auto currentDepth = gi->depth;
            
            gi->completedDepth = currentDepth;
            
            PV pv;
            pv.score = score;
            
            for (int i = 0; i < MAX_PLY; i++)
            {
                pv.line[i] = si.line[i];
            
                if (pv.line[i] == NO_MOVE)
                {
                    break;
                }
            }
            
            gi->pvLine[currentDepth] = pv;
            
            break;
        }

        window += window / 4; 
    }

    assert (score > alpha && score < beta);
}

void checkTime()
{
    const uint64_t timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
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

    abortSearch = tmg::timeManager.isTimeSet() && timeElapsedMs >= maxTime;
}

template<Side stm>
int alphabeta(int alpha, int beta, int mate, int depth, GameInfo *gi, SearchInfo *si )
{
    // if (alpha >= beta) {
    // 	std::cout<<"realDepth=" << si->realDepth << ", depth=" << si->depth << ", ply =" << si->ply << "\n";
    // 	std::cout<<alpha<<","<<beta<<std::endl;
    // }

    assert(alpha < beta); 
    
    constexpr auto opp = stm == WHITE ? BLACK : WHITE;
    
    const auto ply = si->ply;
    const auto rootNode = si->rootNode;
    const auto mainThread = si->mainThread;
    const auto singularSearch = si->singularSearch;
    const auto nullMove = si->nullMove;
    const auto pvNode = alpha != beta - 1;
    

    // Quiescense Search(under observation)

    if (depth < PLY || ply >= MAX_PLY )
    {
        si->line[0] = NO_MOVE;
        
        return quiescenseSearch<stm>(alpha, beta, gi, si );
    }
    

    // Check time spent
    if (    mainThread
        &&  gi->nodes % CHECK_NODES == 0)
    {
        checkTime();
    }



    if (abortSearch)
    {
        return 0; 
    }

    
    gi->selDepth = rootNode ? 0 : std::max(gi->selDepth, ply);

    gi->nodes++;

    
    //http://www.talkchess.com/forum3/viewtopic.php?t=63090
    gi->moveStack[ply + 1].killerMoves[0] = NO_MOVE;
    gi->moveStack[ply + 1].killerMoves[1] = NO_MOVE;



    // Repetition detection

    const auto allPiecesCount = POPCOUNT(gi->occupied);
    
    if (!rootNode )
    {
        if (isRepetition(ply, gi))
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
        
        gi->movesHistory[gi->moves_history_counter + ply + 1].hashKey = gi->hashKey;
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
            ? gi->moveStack[ply].sEval : isInCheck
            ? VAL_UNKNOWN : hashHit
            ? ((hashEntry->sEval == VAL_UNKNOWN)
            ? nnueEval(stm, gi) : hashEntry->sEval) : nnueEval(stm, gi);

    const auto improving = isInCheck ? false : ply >= 2 ? sEval > gi->moveStack[ply - 2].sEval : true;

    if (!singularSearch && !isInCheck && !hashHit)
    {
        gi->hashManager.recordHash(gi->hashKey, NO_MOVE, NO_DEPTH, VAL_UNKNOWN, NO_BOUND, sEval);
    }


    // Search tree pruning

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
        

        if (sEval - fmargin[depth / PLY] >= beta)       // Reverse Futility Pruning
            return beta;          
    
        // TODO check logic. The quiescense call seems costly
        if (sEval + U16_RAZOR_MARGIN < beta)      // Razoring
        {
            const auto rscore = quiescenseSearch<stm>(alpha, beta, gi, si );

            if (rscore < beta) 
            {
                return rscore;
            }
        }

        if (sEval + fmargin[depth / PLY] <= alpha)      // Futility Pruning
            fPrune = true;
    }



    if (!rootNode )
    {
        gi->moveStack[ply].epFlag = gi->moveStack[ply - 1].epFlag;
        gi->moveStack[ply].epSquare = gi->moveStack[ply - 1].epSquare;
        gi->moveStack[ply].castleFlags = gi->moveStack[ply - 1].castleFlags;
    }

    gi->moveStack[ply].ttMove = ttMove;
    gi->moveStack[ply].sEval = sEval;



    SearchInfo lSi;

    lSi.skipMove = NO_MOVE;
    lSi.mainThread = mainThread;
    lSi.singularSearch = false;
    lSi.nullMove = false;
    lSi.rootNode = false;



    // Null Move pruning

    bool mateThreat = false;

    if (	!rootNode
        &&	!pvNode
        &&	!isInCheck 
        &&	!singularSearch
        &&	!nullMove
        &&	oppPiecesCount > 3 // Check the logic for endgame
        &&	depth > 2 * PLY
        &&	sEval >= beta)
    {
        makeNullMove(ply, gi);

        const auto R = depth > 6 * PLY ? 2 * PLY : PLY;
    
        lSi.nullMove = true;

        lSi.ply = ply + 1;
        lSi.line[0] = NO_MOVE;
        
        const auto score = -alphabeta<opp>(-beta, -beta + 1, mate - 1, depth - R - PLY, gi, &lSi );

        unmakeNullMove(ply, gi);

        lSi.nullMove = false;

        if (score >= beta)
            return beta;
    
        if (std::abs(score) >= WIN_SCORE ) // Mate threat
            mateThreat = true;
    }
    


    bool isQuietMove = false;
    
    U8 hashf = hashfALPHA;
    int movesPlayed = 0, newDepth = 0;
    int score = -MATE, bestScore = -MATE;

    U32 bestMove = NO_MOVE, previousMove = rootNode ? NO_MOVE : gi->moveStack[ply - 1].move;

    Move currentMove;

    std::vector<U32> quietsPlayed, capturesPlayed;
    
    MOVE_LIST moveList;

    moveList.skipQuiets = false;
    moveList.stage = PLAY_HASH_MOVE;
    moveList.ttMove = ttMove;
    moveList.killerMove1 = gi->moveStack[ply].killerMoves[0];
    moveList.killerMove2 = gi->moveStack[ply].killerMoves[1];
    moveList.counterMove = previousMove == NO_MOVE ?
        NO_MOVE : gi->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)];

    while (true) 
    {
        U64 startingNodeCount = gi->nodes;

        // fetch next psuedo-legal move
        currentMove = getNextMove(stm, ply, gi, &moveList);

        if (moveList.stage == STAGE_DONE)
            break;

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
                    moveList.skipQuiets = true;
                    continue;
                }

                // Late move pruning
                if (    depth < LMP_DEPTH * PLY
                    &&  movesPlayed >= LMP[improving][depth / PLY])
                {
                    moveList.skipQuiets = true;
                    continue;
                }

                // History pruning
                if (    depth <= HISTORY_PRUNING_DEPTH * PLY
                    &&  currentMove.score < HISTORY_PRUNING * depth / PLY)
                {
                    continue;
                }
            } 
            else
            {   // SEE pruning
                if (    depth <= SEE_PRUNING_DEPTH * PLY
                    &&  currentMove.seeScore < SEE_PRUNING * depth / PLY)
                {
                    continue;
                }
            }
        }


        const auto currentMoveType = move_type(currentMove.move);
        const auto currentMoveToSq = to_sq(currentMove.move);


        // Fractional Extensions
        // ======================================================================

        int extension = 0;

        if (!rootNode )
        {
            int16_t pieceCurrMove = pieceType(currentMove.move);

            bool isPrank = stm ?
                currentMoveToSq >= 8 && currentMoveToSq <= 15 :
                currentMoveToSq >= 48 && currentMoveToSq <= 55;

            U8 prevMoveType = move_type(previousMove);

            // Check extension
            if (isInCheck)
                extension = PLY;

            // Pawn push extension
            else if (pieceCurrMove == PAWNS && isPrank)
                extension = PLY;

            // Mate threat extension
            else if (mateThreat)
                extension = PLY / 2;

            // Recapture extension
            else if (previousMove != NO_MOVE && prevMoveType == MOVE_CAPTURE)
            {
                U8 prevMoveToSq = to_sq(previousMove);

                if (currentMoveToSq == prevMoveToSq)
                    extension = PLY / 2;
            }
        }

        // Singular search
        if (    !rootNode
            &&  !singularSearch
            &&  extension < PLY
            &&  depth >= 7 * PLY
            &&  hashHit
            &&  ttMove != NO_MOVE
            &&  currentMove.move == ttMove
            &&  std::abs(ttScore) < WIN_SCORE
            &&  hashEntry->flags == hashfBETA
            &&  hashEntry->depth >= depth - 3 * PLY)
        {
            const auto sBeta = ttScore - 4 * depth / PLY;
            const auto sDepth = (depth / PLY / 2 - 1) * PLY;

            lSi.singularSearch = true;
            lSi.nullMove = false;
            lSi.skipMove = ttMove;
            lSi.ply = ply;
            lSi.line[0] = NO_MOVE;

            const auto ssScore = alphabeta<stm>(sBeta - 1, sBeta, mate, sDepth, gi, &lSi );

            lSi.skipMove = NO_MOVE;
            lSi.singularSearch = false;

            if (ssScore < sBeta)
            {
                extension = PLY;
            }
            else if (sBeta >= beta)
            {
                return sBeta;
            }
        }




        newDepth = (depth - PLY) + extension;



        // make the move
        make_move(ply, currentMove.move, gi);

        // check if psuedo-legal move is valid
        if (isKingInCheck<stm>(gi))
        {
            unmake_move(ply, currentMove.move, gi);
            continue;
        }



        // psuedo legal move is valid, increment number of moves played
        movesPlayed++;

        // report current depth, moves played and current move being searched
        if ( rootNode && mainThread )
        {
            const auto timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
                tmg::timeManager.getStartTime());

            if (timeElapsedMs > 4000)
            {
                std::cout << "info currmove " << getMoveNotation(currentMove.move);
                std::cout << " currmovenumber " << movesPlayed << std::endl;
            }
        }

        isQuietMove = currentMoveType == MOVE_NORMAL
            || currentMoveType == MOVE_CASTLE
            || currentMoveType == MOVE_DOUBLE_PUSH
            || (currentMoveType == MOVE_PROMOTION && cPieceType(currentMove.move) == DUMMY);

        if (isQuietMove) 
            quietsPlayed.push_back(currentMove.move);
        else
            capturesPlayed.push_back(currentMove.move);

        
        gi->moveStack[ply].move = currentMove.move;


        int reduce = 0;


        lSi.ply = ply + 1;

        
        if (movesPlayed <= 1) 
        { // Principal Variation Search

            lSi.line[0] = NO_MOVE;

            score = -alphabeta<opp>(-beta, -alpha, mate - 1, newDepth, gi, &lSi );
        } 
        else 
        { // Late Move Reductions (Under observation)
        
            if (	depth > 2 * PLY
                &&	movesPlayed > 1
                &&	isQuietMove) 
            {
                reduce = LMR[std::min(depth / PLY, 63)][std::min(movesPlayed, 63)];

                if (!pvNode )
                    reduce++;

                if (!improving && !isInCheck) 
                    reduce++; // isInCheck sets improving to false

                if (isInCheck && pieceType(currentMove.move) == KING) 
                    reduce++;

                if (moveList.stage >= PLAY_KILLER_MOVE_1 && moveList.stage <= PLAY_COUNTER_MOVE)
                    reduce--; // reduce less for killer and counter moves

                reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO rewrite logic


                reduce = std::min(depth - 1, std::max(reduce, 1));

                lSi.line[0] = NO_MOVE;
                
                score = -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, newDepth - reduce * PLY, gi, &lSi );
            }
            else
            {
                score = alpha + 1;
            }

            if (score > alpha) 
            {   // Research 
                
                lSi.line[0] = NO_MOVE;

                score = -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, newDepth, gi, &lSi );
                
                if (score > alpha && score < beta) 
                    score = -alphabeta<opp>(-beta, -alpha, mate - 1, newDepth, gi, &lSi );
            }
        }


        unmake_move(ply, currentMove.move, gi);


        if (rootNode)
        {
            if (mainThread)
                moveNodeCount[from_sq(currentMove.move)][to_sq(currentMove.move)] += gi->nodes - startingNodeCount;

            if (movesPlayed == 1 || score > alpha)
            {
                auto pline = &si->line[0];
                auto line = &lSi.line[0];

                *pline++ = currentMove.move;

                while (*line != NO_MOVE)
                    *pline++ = *line++;

                *pline = NO_MOVE;

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
                {
                    auto pline = &si->line[0];
                    auto line = &lSi.line[0];

                    *pline++ = currentMove.move;

                    while (*line != NO_MOVE)
                        *pline++ = *line++;

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
    
    // TODO check logic
    if (hashf == hashfBETA) 
    {
        if (isQuietMove) 
        {
            if (bestMove != moveList.killerMove1 && bestMove != moveList.killerMove2)
            {   // update killers
        
                gi->moveStack[ply].killerMoves[1] = moveList.killerMove1;
                gi->moveStack[ply].killerMoves[0] = bestMove;
            }

            updateHistory(stm, ply, depth / PLY, bestMove, quietsPlayed, gi);
        } 

        updateCaptureHistory(depth / PLY, bestMove, capturesPlayed, gi);
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
int quiescenseSearch(int alpha, int beta, GameInfo *gi, SearchInfo* si ) {

    assert (alpha < beta);
    assert ( si->ply > 0);

    constexpr auto opp = stm == WHITE ? BLACK : WHITE;

    const bool mainThread = si->mainThread;

    const auto ply = si->ply;

    // Check if time limit has been reached
    if (    tmg::timeManager.isTimeSet()
        &&  mainThread && gi->nodes % CHECK_NODES == 0)
    {
        const uint64_t timeElapsedMs = tmg::timeManager.timeElapsed<MILLISECONDS>(
            tmg::timeManager.getStartTime());

        abortSearch = timeElapsedMs >= maxTime;
    }
    
    if (abortSearch)
    {
        return 0; 
    }

    
    gi->selDepth = std::max(gi->selDepth, ply);
    gi->nodes++;


    const auto allPiecesCount = POPCOUNT(gi->occupied);

    // Repetition detection
    if (isRepetition(ply, gi))
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

    gi->movesHistory[gi->moves_history_counter + ply + 1].hashKey = gi->hashKey;

    
    if (ply >= MAX_PLY - 1)
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
    

    if (sEval >= beta) return sEval;

    if (sEval > alpha) alpha = sEval;

    bestScore = sEval;

    
    gi->moveStack[ply].epFlag = gi->moveStack[ply - 1].epFlag;
    gi->moveStack[ply].epSquare = gi->moveStack[ply - 1].epSquare;
    gi->moveStack[ply].castleFlags = gi->moveStack[ply - 1].castleFlags;


    MOVE_LIST moveList;

    moveList.skipQuiets = true;
    moveList.stage = GEN_CAPTURES;
    moveList.ttMove = NO_MOVE;
    moveList.counterMove = NO_MOVE;

    const auto oppPiecesCount = POPCOUNT(opp ? gi->blackPieceBB[PIECES] : gi->whitePieceBB[PIECES]);
    const auto qFutilityBase = sEval + VAL_Q_DELTA;

    U8 hashf = hashfALPHA;
    int movesPlayed = 0; 
    int score = -MATE;

    Move currentMove;
    
    SearchInfo lSi;
    lSi.ply = ply + 1;
    
    while (true) 
    {
        currentMove = getNextMove(stm, ply, gi, &moveList);

        if (moveList.stage > PLAY_GOOD_CAPTURES)
            break;

        assert(currentMove.move != NO_MOVE);

        const auto capPiece = cPieceType(currentMove.move);

        assert(capPiece != DUMMY);

        // Delta pruning
        if (movesPlayed > 1 && oppPiecesCount > 3 && qFutilityBase + seeVal[capPiece] <= alpha)
            continue;

        make_move(ply, currentMove.move, gi);

        if (isKingInCheck<stm>(gi))
        {
            unmake_move(ply, currentMove.move, gi);

            continue;
        }

        movesPlayed++;


        lSi.line[0] = NO_MOVE;
        score = -quiescenseSearch<opp>(-beta, -alpha, gi, &lSi );


        unmake_move(ply, currentMove.move, gi);


        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = currentMove.move;
            
            if (score > alpha) 
            {
                alpha = score;
                hashf = hashfEXACT;
                
                auto pline = &si->line[0];
                auto line = &lSi.line[0];
                
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

    gi->hashManager.recordHash(gi->hashKey, bestMove, 0, bestScore, hashf, sEval);

    return bestScore;
}


