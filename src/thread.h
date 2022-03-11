#ifndef thread_h
#define thread_h

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "globals.h"
#include "constants.h"
#include "classes.h"
#include "structs.h"

class Thread {
	
public:

	bool isInit = false;

	Side side;

	U16 moves_history_counter;


	int historyScore[2][64][64];
	int captureHistoryScore[8][64][8]; // [piece][to][c_piece]

	U32 counterMove[2][64][64];
	
	std::vector<MOVE_LIST> moveList;
	std::vector<PV> pvLine;
	std::vector<MOVE_STACK> moveStack;
	std::vector<UNDO_MOVE_STACK> undoMoveStack;
	std::vector<MOVES_HISTORY> movesHistory;
	std::vector<PAWNS_HASH> pawnHashTable;
	std::vector<EVAL_HASH> evalHashTable;

	U64 whitePieceBB[U8_MAX_PIECES];
	U64 blackPieceBB[U8_MAX_PIECES];
	U64 occupied, empty;

	U64 hashKey, pawnsHashKey;

	EvalInfo evalInfo;
	
	explicit Thread();

	void init();
	void clear();
};

class SearchThread : public Thread {

public:

	static bool abortSearch;
	static bool stop;

	ThreadState state;
	
	bool terminate;

	int idx, depth, completedDepth, selDepth;

	std::atomic<U64> nodes, ttHits;
    
    std::thread stdThread;

	std::mutex mutex;
	std::condition_variable cv;

	explicit SearchThread(int);
	virtual ~SearchThread();

	int getIndex() { return idx; }
	
	void init();
	void loop();
	void start_searching();
	void wait_for_search_finished();
};


class SearchThreadPool {

std::vector<SearchThread*> searchThreads;

public:

	void createThreadPool(U16 nThreads);
	void clear();

	void start_thinking();
	void start_searching();
	void wait_for_search_finished();

	U64 totalNodes();
	U64 totalTTHits();

	SearchThread* getMainSearchThread();
	
	std::vector<SearchThread*> getSearchThreads();
};


extern Thread initThread;
extern SearchThreadPool Threads;

#endif 
