#ifndef thread_h
#define thread_h

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "globals.h"
#include "cerebrum.h"

class Thread {
	
	public:

		u8 side;

		uint16_t moves_history_counter;
		
		int16_t capture_history_score[8][64][8]; // [piece][to][c_piece]
		int16_t historyScore[2][64][64];
		u32 counterMove[2][64][64];
		
		std::vector<PV> pvLine;
		std::vector<MOVE_STACK> moveStack;
		std::vector<UNDO_MOVE_STACK> undoMoveStack;
		std::vector<MOVES_HISTORY> movesHistory;
		std::vector<PAWNS_HASH> pawnHashTable;
		std::vector<EVAL_HASH> evalHashTable;

		u64 whitePieceBB[MAX_PIECES];
		u64 blackPieceBB[MAX_PIECES];
		u64 occupied, empty;

		u64 hashKey, pawnsHashKey;

		NN_Network nn;
 		NN_Board board, save;
 		
		explicit Thread();
		void clear();
};

class SearchThread : public Thread {

	public:

		bool exit = false, searching = true; // Set before starting std::thread
		bool canReportCurrMove;

		int idx, depth, selDepth, lastStableDepth;

		u32 pvMove;

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

	std::atomic_bool stop, increaseDepth;

	void set(size_t);
	void clear();

	void start_thinking();
	void start_searching();
	void wait_for_search_finished() const;


	SearchThread* main()        const { return front(); }
	SearchThread* get_best_thread() const;


	uint64_t getTotalNodes() const;
	uint64_t getTotalTTHits() const;
};


extern Thread initThread;
extern SearchThreadPool Threads;

#endif 
