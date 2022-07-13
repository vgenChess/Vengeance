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
#include "thread.h"
#include "see.h"
#include "constants.h"
#include "history.h"
#include "ucireport.h"
#include "misc.h"
#include "TimeManagement.h"
#include "HashManagement.h"
#include "nnue.h"

bool GameInfo::abortSearch = false;
bool GameInfo::searching = false;

int option_thread_count, LMR[64][64], LMP[2][LMP_DEPTH];

std::mutex mutex;

int fmargin[4] = { 0, 200, 300, 500 };

std::vector<std::thread> threads;
std::vector<GameInfo*> infos;

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


void spawnThreads(GameInfo *gameInfo ) {


    for (int count = 1; count < option_thread_count; count++) {

        GameInfo *lGi = new GameInfo();

        lGi->clone( gameInfo );

        refresh_accumulator( lGi, WHITE);
        refresh_accumulator( lGi, BLACK);

        infos.push_back( lGi );

        if ( lGi->stm == WHITE)
            threads.emplace_back(search<WHITE>, count, lGi );
        else
            threads.emplace_back(search<BLACK>, count, lGi );
    }
}


// FIXME during last move all time is used up and it loses on time
void startSearch(int index, GameInfo *gi)
{
    // only the main thread should be able to call this function
    assert(index == 0);

    HashManager::age += 1;

    GameInfo::searching = true;
    GameInfo::abortSearch = false;

    threads.clear();
    infos.clear();

    refresh_accumulator(gi, WHITE);
    refresh_accumulator(gi, BLACK);

    infos.push_back(gi);


    spawnThreads(gi);


    if (gi->stm == WHITE) {

        search<WHITE>(index, gi);
    } else {

        search<BLACK>(index, gi);
    }


    assert(index == 0);


    GameInfo::abortSearch = true;

    for (std::thread& th: threads)
        th.join();


    // Display the best move

    auto bestThread = infos[0];

    int bestThreadDepth, currentThreadDepth;
    int bestThreadScore, currentThreadScore;

    GameInfo* th;
    for (uint16_t i = 1; i < infos.size(); i++)
    {
        th = infos[i];

        bestThreadDepth = bestThread->completedDepth;
        currentThreadDepth = th->completedDepth;

        bestThreadScore = bestThread->pvLine[bestThreadDepth].score;
        currentThreadScore = th->pvLine[currentThreadDepth].score;

        if (    currentThreadScore > bestThreadScore
            &&  currentThreadDepth > bestThreadDepth)
        {
            bestThread = th;
        }
    }


    U32 bestMove = bestThread->pvLine[infos[0]->completedDepth].line[0];

    std::cout << "bestmove " << getMoveNotation(bestMove) << std::endl;

    TimeManager::sTm.updateTimeSet(false);
    TimeManager::sTm.updateStopped(false);

    GameInfo::searching = false;
}


template<Side stm>
void search(int index, GameInfo *gi) {

    iterativeDeepening<stm>(index, gi);
}


template<Side stm>
void iterativeDeepening(int index, GameInfo *gi)
{    
    gi->nodes = 0;
    gi->ttHits = 0;

    gi->depth = NO_DEPTH;
    gi->completedDepth = NO_DEPTH;

    gi->stableMoveCount = 0;

    for (int depth = 1; depth < 100; depth++)
    {
        gi->depth = depth;

        aspirationWindow<stm>(index, gi );

        if (GameInfo::abortSearch)
            return;

        if ( index != 0)
            continue;

        if (TimeManager::sTm.isTimeSet() && gi->completedDepth >= 4)
        {
            float multiplier = 1;
            const auto timePerMove = TimeManager::sTm.getTimePerMove();

            const auto completedDepth = gi->completedDepth;

            const auto prevScore = gi->pvLine[completedDepth-3].score;
            const auto currentScore = gi->pvLine[completedDepth].score;

            // reduce time for winning scores
            if (std::abs(currentScore) >= 10000) {

                multiplier = 0.5;
            } else {

                auto scoreDiff = prevScore - currentScore;

                     if (scoreDiff < 10) multiplier = 0.5;
                else if (scoreDiff > 40) multiplier = 1.5;
                else if (scoreDiff > 25) multiplier = 1.3;
                else if (scoreDiff > 15) multiplier = 1.15;
            }


            // Stable Move

            assert( gi->pvLine[completedDepth].line[0] != NO_MOVE
                && gi->pvLine[completedDepth-1].line[0] != NO_MOVE);

            const auto previousMove = gi->pvLine[completedDepth-1].line[0];
            const auto currentMove = gi->pvLine[completedDepth].line[0];

            gi->stableMoveCount = previousMove == currentMove ? gi->stableMoveCount + 1 : 0;

            if ( gi->stableMoveCount > 7)
                multiplier = 0.5;


            if (TimeManager::sTm.time_elapsed_milliseconds(
                TimeManager::sTm.getStartTime()) >= timePerMove * multiplier) {

                return;
            }
        }
    } 
}



enum Stats {

    NODES, TTHITS
};

template<Stats stats>
inline U64 getStats() {

    U64 sum = 0;
    for (GameInfo *gi : infos) {

        sum += stats == NODES ? gi->nodes : (stats == TTHITS ? gi->ttHits : 0);
    }

    return sum;
}


template<Side stm>
void aspirationWindow(int index, GameInfo *gi)
{
    int window = MATE;

    int score = -MATE;
    int alpha = -MATE, beta = MATE;
    
    if (gi->depth > 4 && gi->completedDepth > 0)
    {
        window = U8_AP_WINDOW;

        int scoreKnown = gi->pvLine.at(gi->completedDepth).score;

        alpha = std::max(-MATE, scoreKnown - window);
        beta  = std::min( MATE, scoreKnown + window);
    }
    
    SearchInfo searchInfo;

    searchInfo.ply = 0;
    searchInfo.rootNode = true;
    searchInfo.nullMove = false;
    searchInfo.singularSearch = false;
    searchInfo.realDepth = gi->depth;
    searchInfo.skipMove = NO_MOVE;
    searchInfo.mainThread = index == 0;

    int failHighCounter = 0;

    while (true)
    {
        searchInfo.depth = std::max(1, gi->depth - failHighCounter);
        searchInfo.line[0] = NO_MOVE;
        
        gi->selDepth = NO_DEPTH;
        
        score = alphabeta<stm>(alpha, beta, MATE, gi, &searchInfo);

        if (GameInfo::abortSearch)
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
                pv.line[i] = searchInfo.line[i];
            
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


    if (index == 0) {

        auto time_elapsed_milliseconds = TimeManager::sTm.time_elapsed_milliseconds(
            TimeManager::sTm.getStartTime()
        );

        if (gi->depth > 1 && time_elapsed_milliseconds >= 3000) {

            reportPV(infos[0], getStats<NODES>(), getStats<TTHITS>());
        }
    }
}


template<Side stm>
int alphabeta(int alpha, int beta, const int mate, GameInfo *gi, SearchInfo *si )
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
    

    auto depth = si->depth;


    // Quiescense Search(under observation)

    if (depth <= 0 || ply >= MAX_PLY )
    {
        si->line[0] = NO_MOVE;
        
        return quiescenseSearch<stm>(alpha, beta, gi, si );
    }
    

    // Check time spent
    if (    mainThread
        &&  TimeManager::sTm.isTimeSet()
        &&  gi->nodes % CHECK_NODES == 0)
    {
        GameInfo::abortSearch = TimeManager::time_now().time_since_epoch()
            >= TimeManager::sTm.getStopTime().time_since_epoch();
    }
    

    if (GameInfo::abortSearch)
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
    
    int ttScore = I32_UNKNOWN;
    U32 ttMove = NO_MOVE;
    
    if (hashHit) 
    {
        gi->ttHits++;
        
        ttScore = hashEntry->value;
        ttMove =  hashEntry->bestMove;
    
        if (!rootNode && !pvNode && hashEntry->depth >= depth && ttScore != I32_UNKNOWN)
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
    if (depth >= 4 && !ttMove && !singularSearch )
        depth--;


    const auto isInCheck = isKingInCheck<stm>(gi);

    const auto sEval =
        singularSearch
            ? gi->moveStack[ply].sEval : isInCheck
            ? I32_UNKNOWN : hashHit
            ? ((hashEntry->sEval == I32_UNKNOWN)
            ? predict(stm, gi) : hashEntry->sEval) : predict(stm, gi);

    const auto improving = isInCheck ? false : ply >= 2 ? sEval > gi->moveStack[ply - 2].sEval : true;

    if (!singularSearch && !isInCheck && !hashHit)
    {
        gi->hashManager.recordHash(gi->hashKey, NO_MOVE, NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval);
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
        &&  depth <= 3
        &&	oppPiecesCount > 3) 
    { 

        assert(sEval != I32_UNKNOWN);
        

        if (sEval - fmargin[depth] >= beta)       // Reverse Futility Pruning
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

        if (sEval + fmargin[depth] <= alpha)      // Futility Pruning
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
        &&	depth > 2
        &&	sEval >= beta)
    {
        makeNullMove(ply, gi);

        const auto R = depth > 6 ? 2 : 1;
    
        lSi.nullMove = true;

        lSi.ply = ply + 1;
        lSi.depth = depth - R - 1;
        lSi.line[0] = NO_MOVE;
        
        const auto score = -alphabeta<opp>(-beta, -beta + 1, mate - 1, gi, &lSi );

        unmakeNullMove(ply, gi);

        lSi.nullMove = false;

        if (score >= beta)
            return beta;
    
        if (std::abs(score) >= WIN_SCORE ) // Mate threat
            mateThreat = true;
    }
    


    // Singular search

    bool singularMove = false;

    if (    !rootNode
        &&  !singularSearch
        &&  depth >= 7
        &&  hashHit
        &&  ttMove != NO_MOVE 
        &&  std::abs(ttScore) < WIN_SCORE
        &&  hashEntry->flags == hashfBETA
        &&  hashEntry->depth >= depth - 3)
    {
        const auto sBeta = ttScore - 4 * depth;
        const auto sDepth = depth / 2 - 1;

        lSi.singularSearch = true;
        lSi.nullMove = false;
        lSi.skipMove = ttMove;
        lSi.ply = ply;
        lSi.depth = sDepth;
        lSi.line[0] = NO_MOVE;

        const auto score = alphabeta<stm>(sBeta - 1, sBeta, mate, gi, &lSi );

        lSi.skipMove = NO_MOVE;
        lSi.singularSearch = false;

        if (score < sBeta) 
        {
            singularMove = true;
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
    int score = -MATE, bestScore = -MATE;

    U32 bestMove = NO_MOVE, previousMove = rootNode ? NO_MOVE : gi->moveStack[ply - 1].move;

    const U32 KILLER_MOVE_1 = gi->moveStack[ply].killerMoves[0];
    const U32 KILLER_MOVE_2 = gi->moveStack[ply].killerMoves[1];

    Move currentMove;

    std::vector<U32> quietsPlayed, capturesPlayed;
    
    gi->moveList[ply].skipQuiets = false;
    gi->moveList[ply].stage = PLAY_HASH_MOVE;
    gi->moveList[ply].ttMove = ttMove;
    gi->moveList[ply].counterMove = previousMove == NO_MOVE ?
        NO_MOVE : gi->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)];
    gi->moveList[ply].moves.clear();
    gi->moveList[ply].badCaptures.clear();


    while (true) 
    {
        // fetch next psuedo-legal move
        currentMove = getNextMove(stm, ply, gi, &gi->moveList[ply]);

        if (gi->moveList[ply].stage == STAGE_DONE)
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
                    gi->moveList[ply].skipQuiets = true;
                    continue;
                }

                // Late move pruning
                if (    depth < LMP_DEPTH
                    &&  movesPlayed >= LMP[improving][depth])
                {
                    gi->moveList[ply].skipQuiets = true;
                    continue;
                }

                // History pruning
                if (    depth <= HISTORY_PRUNING_DEPTH
                    &&  currentMove.score < HISTORY_PRUNING * depth)
                {
                    continue;
                }
            } 
            else
            {   // SEE pruning
                if (    depth <= SEE_PRUNING_DEPTH
                    &&  currentMove.seeScore < SEE_PRUNING * depth)
                {
                    continue;
                }
            }
        }

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
            if (TimeManager::sTm.time_elapsed_milliseconds(
                TimeManager::sTm.getStartTime()) > CURRMOVE_INTERVAL )
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

        
        gi->moveStack[ply].move = currentMove.move;


        extend = 0;
        float extension = rootNode ? 0 : gi->moveStack[ply - 1].extension;

        //	Fractional Extensions
        if (!rootNode ) // TODO check extensions logic
        { 
            int16_t pieceCurrMove = pieceType(currentMove.move);

            if (currentMove.move == ttMove && singularMove ) // Singular extension
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

        gi->moveStack[ply].extension = extension;
        
        newDepth = (depth - 1) + extend;

        reduce = 0;


        lSi.ply = ply + 1;

        
        if (movesPlayed <= 1) 
        { // Principal Variation Search

            lSi.depth = newDepth;
            lSi.line[0] = NO_MOVE;

            score = -alphabeta<opp>(-beta, -alpha, mate - 1, gi, &lSi );
        } 
        else 
        { // Late Move Reductions (Under observation)
        
            if (	depth > 2
                &&	movesPlayed > 1
                &&	isQuietMove) 
            {
                reduce = LMR[std::min(depth, 63)][std::min(movesPlayed, 63)];

                if (!pvNode )
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
                
                if (gi->moveList[ply].stage < GEN_QUIETS)
                {
                    reduce--; // reduce less for killer and counter moves
                }
                
                reduce -= std::max(-2, std::min(2, currentMove.score / 5000));	// TODO rewrite logic				

                reduce = std::min(depth - 1, std::max(reduce, 1));

                lSi.depth = newDepth - reduce;
                lSi.line[0] = NO_MOVE;
                
                score = -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, gi, &lSi );
            } else
            {
                score = alpha + 1;
            }

            if (score > alpha) 
            {   // Research 
                
                lSi.depth = newDepth;
                lSi.line[0] = NO_MOVE;

                score = -alphabeta<opp>(-alpha - 1, -alpha, mate - 1, gi, &lSi );
                
                if (score > alpha && score < beta) 
                {
                    score = -alphabeta<opp>(-beta, -alpha, mate - 1, gi, &lSi );
                }
            }
        }

        
        unmake_move(ply, currentMove.move, gi);

        
        if (score > bestScore) 
        {
            bestScore = score;
            bestMove = currentMove.move;

            if (score > alpha) 
            {
                alpha = score;
                hashf = hashfEXACT;
                
                // record the moves for the PV
                if ( pvNode )
                {
                    auto pline = &si->line[0];
                    auto line = &lSi.line[0];
                    
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
        
                gi->moveStack[ply].killerMoves[1] = KILLER_MOVE_1;
                gi->moveStack[ply].killerMoves[0] = bestMove;
            }

            updateHistory(stm, ply, depth, bestMove, quietsPlayed, gi);
        } 

        updateCaptureHistory(depth, bestMove, capturesPlayed, gi);
    }


    if (movesPlayed == 0)
    {    // Mate and stalemate check
        return isInCheck ? -mate : 0;
    }


    if (!singularSearch )
    {
        gi->hashManager.recordHash(gi->hashKey, bestMove, depth, bestScore, hashf, sEval);
    }


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
    if (    TimeManager::sTm.isTimeSet()
        &&  mainThread && gi->nodes % CHECK_NODES == 0)
    {
        GameInfo::abortSearch = TimeManager::time_now().time_since_epoch()
            >= TimeManager::sTm.getStopTime().time_since_epoch();
    }
    
    if (GameInfo::abortSearch)
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
        return predict(stm, gi);
    }
    
    
    auto hashEntry = gi->hashManager.getHashEntry(gi->hashKey);
    
    const auto hashHit = gi->hashManager.probeHash(hashEntry, gi->hashKey);
    
    int ttScore = I32_UNKNOWN;

    if (hashHit) 
    { // no depth check required since its 0 in quiescense search
        
        gi->ttHits++;

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
    int bestScore = -MATE;

    auto sEval = hashHit ? ((hashEntry->sEval == I32_UNKNOWN) ? 
                                    predict(stm, gi) : hashEntry->sEval) : predict(stm, gi);
    if (!hashHit)
    {
        gi->hashManager.recordHash(gi->hashKey, NO_MOVE, NO_DEPTH, I32_UNKNOWN, U8_NO_BOUND, sEval);
    }
    

    bestScore = sEval;
    alpha = std::max(alpha, sEval);

    if (alpha >= beta)
    {
        return sEval;
    }
    
    gi->moveStack[ply].epFlag = gi->moveStack[ply - 1].epFlag;
    gi->moveStack[ply].epSquare = gi->moveStack[ply - 1].epSquare;
    gi->moveStack[ply].castleFlags = gi->moveStack[ply - 1].castleFlags;


    gi->moveList[ply].skipQuiets = true;
    gi->moveList[ply].stage = GEN_CAPTURES;
    gi->moveList[ply].ttMove = NO_MOVE;
    gi->moveList[ply].counterMove = NO_MOVE;
    gi->moveList[ply].moves.clear();
    gi->moveList[ply].badCaptures.clear();

    const auto oppPiecesCount = POPCOUNT(opp ? gi->blackPieceBB[PIECES] : gi->whitePieceBB[PIECES]);
    const auto qFutilityBase = sEval + U16_Q_DELTA; 

    U8 hashf = hashfALPHA, capPiece = DUMMY;
    int movesPlayed = 0; 
    int score = -MATE;

    Move currentMove;
    
    SearchInfo lSi;
    lSi.ply = ply + 1;
    
    while (true) 
    {
        currentMove = getNextMove(stm, ply, gi, &gi->moveList[ply]);

        if (gi->moveList[ply].stage >= PLAY_BAD_CAPTURES)
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


        make_move(ply, currentMove.move, gi);

        if (isKingInCheck<stm>(gi))
        {
            unmake_move(ply, currentMove.move, gi);

            continue;
        }

        movesPlayed++;

        capPiece = cPieceType(currentMove.move);


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


