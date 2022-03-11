#include <assert.h>

#include "thread.h"
#include "search.h"

Thread initThread;
SearchThreadPool Threads;

Thread::Thread() {

	this->moveList = std::vector<MOVE_LIST> (U16_MAX_MOVES);
	this->pvLine = std::vector<PV> (U16_MAX_PLY);
	this->moveStack = std::vector<MOVE_STACK> (U16_MAX_PLY + 4);
	this->undoMoveStack = std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY + 4);
	this->movesHistory = std::vector<MOVES_HISTORY> (8192);
	this->pawnHashTable = std::vector<PAWNS_HASH> (U32_PAWN_HASH_TABLE_SIZE);
	this->evalHashTable = std::vector<EVAL_HASH> (U32_EVAL_HASH_TABLE_SIZE);

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 64; ++k) {

				this->historyScore[i][j][k] = 0;
				this->counterMove[i][j][k] = 0;
			}
		}
	}

	for (int i = 0; i < U8_MAX_PIECES; ++i) {
		
		this->whitePieceBB[i] = 0;
		this->blackPieceBB[i] = 0;
	}

	this->occupied = 0;
	this->empty = 0;
}

void Thread::initMembers() {

	this->moveList = std::vector<MOVE_LIST> (U16_MAX_MOVES);
	this->pvLine = std::vector<PV> (U16_MAX_PLY);
	this->moveStack = std::vector<MOVE_STACK> (U16_MAX_PLY + 4);
	this->undoMoveStack = std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY + 4);
	this->movesHistory = std::vector<MOVES_HISTORY> (8192);
	this->pawnHashTable = std::vector<PAWNS_HASH> (U32_PAWN_HASH_TABLE_SIZE);
	this->evalHashTable = std::vector<EVAL_HASH> (U32_EVAL_HASH_TABLE_SIZE);

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 64; ++k) {

				this->historyScore[i][j][k] = 0;
				this->counterMove[i][j][k] = 0;
			}
		}
	}

	for (int i = 0; i < U8_MAX_PIECES; ++i) {
		
		this->whitePieceBB[i] = 0;
		this->blackPieceBB[i] = 0;
	}

	this->occupied = 0;
	this->empty = 0;	
}

void Thread::clear() {

	this->moveList.clear();
	this->pvLine.clear();
	this->moveStack.clear();
	this->undoMoveStack.clear();
	this->movesHistory.clear();
	this->pawnHashTable.clear();
	this->evalHashTable.clear();


	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 8; ++k) {

				this->captureHistoryScore[i][j][k] = 0;
			}
		}
	}


	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 64; ++k) {

				this->historyScore[i][j][k] = 0;
				this->counterMove[i][j][k] = 0;
			}
		}
	}

	for (int i = 0; i < U8_MAX_PIECES; ++i) {

		this->whitePieceBB[i] = 0;
		this->blackPieceBB[i] = 0;
	}

	this->occupied = 0;
	this->empty = 0;	
}

SearchThread::SearchThread(int index) {

	idx = index;
	side = WHITE;
	state = SLEEP;

	stdThread = std::thread([this]() {

		while (true) {

			std::unique_lock<std::mutex> lk(mutex);

			state = SLEEP;

			cv.notify_one(); // Wake up anyone waiting for search finished
			cv.wait(lk, [&]{ return state != SLEEP; });

			if (terminate) 
				break;

			state = SEARCH;

			lk.unlock();

			startSearch(side, this);
		}
	});

	wait_for_search_finished();
}



/// Thread destructor wakes up the thread in idle_loop() and waits
/// for its termination. Thread should be already waiting.

SearchThread::~SearchThread() {

	assert(state != SEARCH);

	terminate = true;

	start_searching();
	stdThread.join();

	this->pvLine.clear();
	this->moveStack.clear();
	this->undoMoveStack.clear();
	this->movesHistory.clear();
	this->pawnHashTable.clear();
	this->evalHashTable.clear();
}



/// Thread::start_searching() wakes up the thread that will start the search

void SearchThread::start_searching() {

	std::lock_guard<std::mutex> lk(mutex);
	state = SEARCH;

	cv.notify_one(); // Wake up the thread in idle_loop()
}


/// Thread::wait_for_search_finished() blocks on the condition variable
/// until the thread has finished searching.

void SearchThread::wait_for_search_finished() {

	std::unique_lock<std::mutex> lk(mutex);
	cv.wait(lk, [&]{ return state != SEARCH; });
}


void SearchThread::init() {

	side = initThread.side;

	pvLine.clear();
	moveStack.clear();	
	undoMoveStack.clear();
	movesHistory.clear();

	pvLine = std::vector<PV> (U16_MAX_PLY);
	moveStack = std::vector<MOVE_STACK> (U16_MAX_PLY + 4);
	undoMoveStack = std::vector<UNDO_MOVE_STACK> (U16_MAX_PLY + 4);
	movesHistory = std::vector<MOVES_HISTORY> (8192);

	//TODO use one line code to copy all the init struct to other struct, like memcopy etc
	moveStack[0].castleFlags = initThread.moveStack[0].castleFlags;
	moveStack[0].epFlag = initThread.moveStack[0].epFlag;
	moveStack[0].epSquare = initThread.moveStack[0].epSquare;

	for (int i = 0; i < 8192; i++) {

		movesHistory[i].hashKey = initThread.movesHistory[i].hashKey;
		movesHistory[i].fiftyMovesCounter = initThread.movesHistory[i].fiftyMovesCounter;
	}

	moves_history_counter = initThread.moves_history_counter;

	for (int piece = DUMMY; piece <= PIECES; piece++) {

		whitePieceBB[piece] = initThread.whitePieceBB[piece];
		blackPieceBB[piece] = initThread.blackPieceBB[piece];
	}

	occupied = initThread.occupied;
	empty = initThread.empty;

	hashKey = initThread.hashKey;
	pawnsHashKey = initThread.pawnsHashKey;
}


void SearchThreadPool::createThreadPool(U16 nThreads) {

	if (searchThreads.size() > 0) {

		getMainSearchThread()->wait_for_search_finished();
		
		for (U16 i = 0; i < searchThreads.size(); i++) {

			delete searchThreads.at(i);
		}
	
		searchThreads.clear();
    }

    if (nThreads > 0) {

		searchThreads.push_back(new SearchThread(0));
		
		for (U16 i = 1; i < nThreads; i++)
			searchThreads.push_back(new SearchThread(i));
    }		
}


void SearchThreadPool::clear() {

	for (SearchThread* th : searchThreads)
		th->clear();
}


void SearchThreadPool::start_thinking() {

	getMainSearchThread()->wait_for_search_finished();

	SearchThreadPool::stop = false;

	for (SearchThread* th : searchThreads)
		th->init();

	getMainSearchThread()->start_searching();
}

void SearchThreadPool::start_searching() {

    for (SearchThread* th : searchThreads) {

    	if (th != getMainSearchThread())
            th->start_searching();    	
    }
}

void SearchThreadPool::wait_for_search_finished() {

    for (SearchThread* th : searchThreads) {

        if (th != getMainSearchThread())
            th->wait_for_search_finished();
    }
}

U64 SearchThreadPool::totalNodes() {

	U64 total = 0;
	for (SearchThread *thread : searchThreads)
		total += thread->nodes;

	return total;
}

U64 SearchThreadPool::totalTTHits() {

	U64 total = 0;

	for (SearchThread *thread : searchThreads) 
		total += thread->ttHits;

	return total;
}



