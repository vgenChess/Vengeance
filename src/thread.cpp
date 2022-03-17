#include <assert.h>
#include <cstring>

#include "thread.h"
#include "search.h"

Thread initThread;
SearchThreadPool Threads;

Thread::Thread()
{
    clear();
    init();
}

void Thread::clear()
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
    memset(whitePieceBB, 0, sizeof(U64) * 8);
    memset(blackPieceBB, 0, sizeof(U64) * 8);

    occupied = 0;
    empty = 0;
}

void Thread::init()
{
    moveList = 		std::vector<MOVE_LIST> (U16_MAX_MOVES);
    pvLine = 		std::vector<PV> (U16_MAX_PLY);
    moveStack = 	std::vector<MOVE_STACK> (U16_MAX_PLY);
    undoMoveStack = std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY);
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


Thread::~Thread()
{
    moveList.clear();
    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();
}


SearchThread::SearchThread(int index)
{
    idx = index;
    side = WHITE;
    mState = SLEEP;

    thread = std::thread([this]()
    {
        while (!terminate)
        {
            cv.notify_one(); // Wake up anyone waiting for search finished
            
            blockThreadIfState<SLEEP>();
            
            if (!terminate)
            {
                startSearch(side, this);
                mState = SLEEP;
            }
        }
    });

    blockThreadIfState<SEARCH>();
}


SearchThread::~SearchThread()
{
    assert(mState != SEARCH);

    terminate = true;

    search();

    thread.join();
}



/// Thread::start_searching() wakes up the thread that will start the search

void SearchThread::search()
{
    std::lock_guard<std::mutex> lk(mutex);
    
    mState = SEARCH;

    cv.notify_one(); // Wake up the thread in idle_loop()
}


void SearchThread::initialise()
{
    side = initThread.side;

    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();

    pvLine =           std::vector<PV> (U16_MAX_PLY);
    moveStack =        std::vector<MOVE_STACK> (U16_MAX_PLY + 4);
    undoMoveStack =    std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY + 4);
    movesHistory =     std::vector<MOVES_HISTORY> (8192);
    pawnsHashTable =   std::vector<PawnsHashEntry>(U16_PAWN_HASH_TABLE_RECORDS);
    evalHashTable =    std::vector<EvalHashEntry>(U16_EVAL_HASH_TABLE_RECORDS);


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
}

void SearchThreadPool::createThreadPool(int nThreads)
{
    if (searchThreads.size() > 0)
    {
        getMainSearchThread()->blockThreadIfState<SEARCH>();

        for (SearchThread *thread: searchThreads)
        {
            delete thread;
        }
        
        searchThreads.clear();
    }

    if (nThreads > 0)
    {
        for (int i = 0; i < nThreads; i++)
        {
            searchThreads.push_back(new SearchThread(i));
        }
    }
}


void SearchThreadPool::clear()
{
    for (SearchThread* th : searchThreads)
    {
        th->clear();
    }
}


void SearchThreadPool::wait_for_search_finished()
{
    for (SearchThread* th : searchThreads)
    {
        if (th != getMainSearchThread())
        {
            th->blockThreadIfState<SEARCH>();
        }
    }
}

template <ThreadState state>
void SearchThread::blockThreadIfState()
{
    std::unique_lock<std::mutex> lk(mutex);
    
    while (mState == state) 
    {
        cv.wait(lk);
    }
    
    lk.unlock();
}

U64 SearchThreadPool::totalNodes()
{
    U64 total = 0;
    for (SearchThread *thread : searchThreads)
    {
        total += thread->nodes;
    }
    
    return total;
}

U64 SearchThreadPool::totalTTHits()
{
    U64 total = 0;

    for (SearchThread *thread : searchThreads)
    {
        total += thread->ttHits;
    }
    
    return total;
}

SearchThread* SearchThreadPool::getMainSearchThread()
{
    return searchThreads[0];
}

std::vector<SearchThread*> SearchThreadPool::getSearchThreads()
{
    return searchThreads;
}
