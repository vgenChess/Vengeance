#include <assert.h>

#include "thread.h"
#include "search.h"
#include "constants.h"

Thread initThread;
SearchThreadPool Threads; // Global object


void SearchThreadPool::set(size_t requested) {

	if (size() > 0)   // destroy any existing thread(s)
	{
		main()->wait_for_search_finished();

		while (size() > 0)
			delete back(), pop_back();
	}

	if (requested > 0)   // create new thread(s)
	{
		push_back(new SearchThread(0));

		while (size() < requested)
			push_back(new SearchThread(size()));		

		clear();

		// Reallocate the hash with the new threadpool size
		// TT.resize(size_t(Options["Hash"]));

		// Init thread number dependent search params.
		// Search::init();
	}
}

/// ThreadPool::clear() sets threadPool data to initial values

void SearchThreadPool::clear() {

	for (SearchThread* th : *this)
		th->clear();
}

/// ThreadPool::start_thinking() wakes up main thread waiting in idle_loop() and
/// returns immediately. Main thread will wake up other threads and start the search.

void SearchThreadPool::start_thinking() {

	main()->wait_for_search_finished();

	stop = false;

	for (SearchThread* th : *this) {

		th->init();
	}

	main()->start_searching();
}

/// Start non-main threads

void SearchThreadPool::start_searching() {

    for (SearchThread* th : *this)
        if (th != front()) 
            th->start_searching();
}

/// Wait for non-main threads

void SearchThreadPool::wait_for_search_finished() const {

    for (SearchThread* th : *this)
        if (th != front())
            th->wait_for_search_finished();
}

uint64_t SearchThreadPool::getTotalNodes() const {

	uint64_t sum = 0;
	for (SearchThread* thread : *this) {

		sum += thread->nodes;
	}

	return sum;
}

uint64_t SearchThreadPool::getTotalTTHits() const {

	uint64_t sum = 0;
	for (SearchThread* thread : *this) {

		sum += thread->ttHits;
	}

	return sum;
}










Thread::Thread() {

	this->pvLine = std::vector<PV> (MAX_PLY);
	this->moveStack = std::vector<MOVE_STACK> (MAX_PLY);
	this->undoMoveStack = std::vector<UNDO_MOVE_STACK> (MAX_PLY);
	this->movesHistory = std::vector<MOVES_HISTORY> (MAX_PLY);
	this->pawnHashTable = std::vector<PAWNS_HASH> (PAWN_HASH_TABLE_SIZE);
	this->evalHashTable = std::vector<EVAL_HASH> (EVAL_HASH_TABLE_SIZE);

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 64; ++k) {

				this->historyScore[i][j][k] = 0;
				this->counterMove[i][j][k] = 0;
			}
		}
	}

	for (int i = 0; i < MAX_PIECES; ++i) {
		
		this->whitePieceBB[i] = 0;
		this->blackPieceBB[i] = 0;
	}

	this->occupied = 0;
	this->empty = 0;
}

void Thread::clear() {

	this->pvLine.clear();
	this->moveStack.clear();
	this->undoMoveStack.clear();
	this->movesHistory.clear();
	this->pawnHashTable.clear();
	this->evalHashTable.clear();


	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 64; ++j) {
			for (int k = 0; k < 8; ++k) {

				this->capture_history_score[i][j][k] = 0;
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

	for (int i = 0; i < MAX_PIECES; ++i) {

		this->whitePieceBB[i] = 0;
		this->blackPieceBB[i] = 0;
	}

	this->occupied = 0;
	this->empty = 0;	
}









/// Thread constructor launches the thread and waits until it goes to sleep
/// in idle_loop(). Note that 'searching' and 'exit' should be already set.

SearchThread::SearchThread(int index) : stdThread(&SearchThread::idle_loop, this) {

	this->idx = index;
	this->side = WHITE;
	this->canReportCurrMove = false;

	this->wait_for_search_finished();
}

/// Thread destructor wakes up the thread in idle_loop() and waits
/// for its termination. Thread should be already waiting.

SearchThread::~SearchThread() {

	assert(!searching);

	exit = true;
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
	searching = true;
	cv.notify_one(); // Wake up the thread in idle_loop()
}

/// Thread::wait_for_search_finished() blocks on the condition variable
/// until the thread has finished searching.

void SearchThread::wait_for_search_finished() {

	std::unique_lock<std::mutex> lk(mutex);
	cv.wait(lk, [&]{ return !searching; });
}

/// Thread::idle_loop() is where the thread is parked, blocked on the
/// condition variable, when it has no work to do.

void SearchThread::idle_loop() {

	while (true) {

		std::unique_lock<std::mutex> lk(mutex);
		searching = false;

		cv.notify_one(); // Wake up anyone waiting for search finished
		cv.wait(lk, [&]{ return searching; });

		if (exit)
			return;

		lk.unlock();

		search();
	}
}

SearchThread* SearchThreadPool::get_best_thread() const {


    SearchThread* bestThread = front();
    
    int bestScore = -INF, bestDepth = NO_DEPTH;

    for (SearchThread* th : *this) {	

    	int depth = th->depth;
   		int score = th->pvLine[depth].score;
   			
  		if ((depth == bestDepth && score > bestScore) || 
  			(score > WIN_SCORE_THRESHOLD && score > bestScore)) {
			
			bestThread = th;	
  		}
        
        if (depth > bestDepth && (score > bestScore || score <= WIN_SCORE_THRESHOLD)) {

            bestThread = th;
        }
    }

    return bestThread;
}

void SearchThread::init() {


	this->side = initThread.side;

	pvLine.clear();
	moveStack.clear();	
	undoMoveStack.clear();
	movesHistory.clear();

	pvLine = std::vector<PV> (MAX_PLY);
	moveStack = std::vector<MOVE_STACK> (MAX_PLY);
	undoMoveStack = std::vector<UNDO_MOVE_STACK> (MAX_PLY);
	movesHistory = std::vector<MOVES_HISTORY> (MAX_PLY);

	//TODO use one line code to copy all the init struct to other struct, like memcopy etc
	moveStack[0].castleFlags = initThread.moveStack[0].castleFlags;
	moveStack[0].epFlag = initThread.moveStack[0].epFlag;
	moveStack[0].epSquare = initThread.moveStack[0].epSquare;

	for (int i = 0; i < MAX_PLY; i++) {

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

	board.pieces[0] = &whitePieceBB[PAWNS];
	board.pieces[1] = &blackPieceBB[PAWNS];
}

void SearchThread::search() {

	if (this == Threads.main()) {

		startSearch(this->side);
	}
	else {

		searchMain(this->side, this);
	}
}


