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

	U8 side;

	U16 moves_history_counter;
	
	int32_t historyScore[2][64][64];
	int32_t captureHistoryScore[8][64][8]; // [piece][to][c_piece]

	U32 counterMove[2][64][64];
	
	std::vector<MOVE_LIST> moveList;
	std::vector<PV> pvLine;

	std::vector<MOVE_STACK> moveStack;
	std::vector<UNDO_MOVE_STACK> undoMoveStack;
	
	// record of moves played
	std::vector<MOVES_HISTORY> movesHistory;

	// cache for evaluation
	std::vector<PAWNS_HASH> pawnHashTable;
	std::vector<EVAL_HASH> evalHashTable;

	U64 whitePieceBB[U8_MAX_PIECES];
	U64 blackPieceBB[U8_MAX_PIECES];
	U64 occupied, empty;

	U64 hashKey, pawnsHashKey;

	EvalInfo evalInfo;
	
	explicit Thread();

	void initMembers();
	void clear();
};

class SearchThread : public Thread {

public:

	bool exit = false, searching = true; // Set before starting std::thread
	bool canReportCurrMove;

	int idx, depth, completedDepth, selDepth;

	std::atomic<uint64_t> nodes, ttHits;
    
    std::thread stdThread;

	std::mutex mutex;
	std::condition_variable cv;

	explicit SearchThread(int);
	virtual ~SearchThread();

	int index() { return idx; }
	
	void init();
	void idle_loop();
	void start_searching();
	void wait_for_search_finished();
	void search();
};

struct SearchThreadPool : public std::vector<SearchThread*> {

	std::atomic_bool stop;

	void set(size_t);
	void clear();

	void start_thinking();
	void start_searching();
	void wait_for_search_finished() const;


	SearchThread* main() const { return front(); }


	U64 getTotalNodes() const;
	U64 getTotalTTHits() const;
};


extern Thread initThread;
extern SearchThreadPool Threads;

#endif 
