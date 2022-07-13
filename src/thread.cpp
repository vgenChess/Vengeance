#include <assert.h>
#include <cstring>

#include "thread.h"
#include "search.h"
#include "nnue.h"

GameInfo initThread;

GameInfo::GameInfo()
{
    init();
}

void GameInfo::clear()
{
    moveList.clear();
    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();

    memset(captureHistoryScore, 0, sizeof(int) * 8 * 64 * 8);
    memset(historyScore, 0, sizeof(int) * 2 * 64 * 64);
    memset(counterMove, 0, sizeof(U32) * 2 * 64 * 64);
    memset(whitePieceBB, 0, sizeof(U64) * 8);
    memset(blackPieceBB, 0, sizeof(U64) * 8);

    occupied = 0;
    empty = 0;
}

void GameInfo::init()
{
    moveList = 		std::vector<MOVE_LIST> ( MAX_MOVES );
    pvLine = 		std::vector<PV> ( MAX_PLY );
    moveStack = 	std::vector<MOVE_STACK> ( MAX_PLY );
    undoMoveStack = std::vector<UNDO_MOVE_STACK> ( MAX_PLY );
    movesHistory  = std::vector<MOVES_HISTORY> (8192);
    pawnsHashTable = std::vector<PawnsHashEntry>(U16_PAWN_HASH_TABLE_RECORDS);
    evalHashTable = std::vector<EvalHashEntry>(U16_EVAL_HASH_TABLE_RECORDS);

    memset(captureHistoryScore, 0, sizeof(int) * 8 * 64 * 8);
    memset(historyScore, 0, sizeof(int) * 2 * 64 * 64);
    memset(counterMove, 0, sizeof(U32) * 2 * 64 * 64);
    memset(whitePieceBB, 0, sizeof(U64) * 8);
    memset(blackPieceBB, 0, sizeof(U64) * 8);

    occupied = 0;
    empty = 0;
}

GameInfo::~GameInfo()
{
    moveList.clear();
    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();
}


/*
SearchThread::SearchThread(int index)
{
    std::unique_lock<std::mutex> lk(mMutex);
    
    mIndex = index;
    mTerminate = false;
    mSearching = true;

    mThread = std::thread(&SearchThread::searchThreadLifeCycle, this);

    mCv.wait(lk, [&]{ return !mSearching; });
}

void SearchThread::searchThreadLifeCycle()
{
    while (!mTerminate)
    {
        std::unique_lock<std::mutex> lk(mMutex);

        mSearching = false;

        while (!mSearching && !mTerminate)
        {
            mCv.notify_one();
            mCv.wait(lk);
        }

        lk.unlock();

        if (!mTerminate)
        {
            startSearch( stm );
        } 
    }
}

SearchThread::~SearchThread()
{
    assert(!mSearching);

    mTerminate = true;

    search();

    mThread.join();
}

void SearchThread::waitIfSearching()
{
    std::unique_lock<std::mutex> lk(mMutex);

    while (mSearching) 
        mCv.wait(lk);

    lk.unlock();
}

void SearchThread::search()
{
    std::lock_guard<std::mutex> lk(mMutex);
    
    mSearching = true;

    mCv.notify_one(); 
}

void SearchThread::initialise()
{
    stm = initThread.stm;

    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();

    pvLine =            std::vector<PV> (U16_MAX_PLY);
    moveStack =         std::vector<MOVE_STACK> (U16_MAX_PLY + 4);
    undoMoveStack =     std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY + 4);
    movesHistory =      std::vector<MOVES_HISTORY> (8192);
    pawnsHashTable =    std::vector<PawnsHashEntry>(U16_PAWN_HASH_TABLE_RECORDS);
    evalHashTable =     std::vector<EvalHashEntry>(U16_EVAL_HASH_TABLE_RECORDS);

    moveStack[0].castleFlags = initThread.moveStack[0].castleFlags;
    moveStack[0].epFlag = initThread.moveStack[0].epFlag;
    moveStack[0].epSquare = initThread.moveStack[0].epSquare;

    for (int i = 0; i < 8192; i++)
    {
        movesHistory[i].hashKey = initThread.movesHistory[i].hashKey;
        movesHistory[i].fiftyMovesCounter = initThread.movesHistory[i].fiftyMovesCounter;
    }

    moves_history_counter = initThread.moves_history_counter;

    for (int piece = DUMMY; piece <= PIECES; piece++)
    {
        whitePieceBB[piece] = initThread.whitePieceBB[piece];
        blackPieceBB[piece] = initThread.blackPieceBB[piece];
    }

    occupied = initThread.occupied;
    empty = initThread.empty;

    hashKey = initThread.hashKey;
    pawnsHashKey = initThread.pawnsHashKey;

    refresh_accumulator(this, WHITE);
    refresh_accumulator(this, BLACK);
}

void SearchThreadPool::createThreadPool(int n)
{
    if (threads.size() > 0)
    {
        getMainSearchThread()->waitIfSearching();

        for (SearchThread *thread: threads)
            delete thread;
        
        threads.clear();
    }

    if (n > 0)
    {
        for (int i = 0; i < n; i++)
            threads.push_back(new SearchThread(i));
    }
}

void SearchThreadPool::clear()
{
    for (SearchThread* th : threads)
        th->clear();
}

void SearchThreadPool::waitForAll()
{
    auto mainThread = threads[0];
    for (SearchThread* th : threads)
    {
        if (th != mainThread)
            th->waitIfSearching();
    }
}

U64 SearchThreadPool::totalNodes()
{
    U64 total = 0;
    for (SearchThread *thread : threads)
        total += thread->nodes;
    
    return total;
}

U64 SearchThreadPool::totalTTHits()
{
    U64 total = 0;

    for (SearchThread *thread : threads)
        total += thread->ttHits;
    
    return total;
}

SearchThread* SearchThreadPool::getMainSearchThread()
{
    return threads[0];
}

// @TODO check logic
SearchThread* SearchThreadPool::getBestThread()
{
    auto bestThread = threads[0];

    int bestThreadDepth, currentThreadDepth;
    int bestThreadScore, currentThreadScore;
    
    SearchThread* th;
    for (uint16_t i = 1; i < threads.size(); i++)
    {
        th = threads[i];

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

    return bestThread;
}

std::vector<SearchThread*> SearchThreadPool::getSearchThreads()
{
    return threads;
}
*/
