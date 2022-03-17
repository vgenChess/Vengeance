#ifndef thread_h
#define thread_h

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "constants.h"
#include "classes.h"
#include "structs.h"
#include "HashManagement.h"

class Thread 
{

public:

	Side side;
    
	U16 moves_history_counter;

	int historyScore[2][64][64];
	int captureHistoryScore[8][64][8]; // [piece][to][c_piece]

	U32 counterMove[2][64][64];
    
    U64 hashKey, pawnsHashKey;
	U64 occupied, empty;

    U64 whitePieceBB[U8_MAX_PIECES];
	U64 blackPieceBB[U8_MAX_PIECES];

	std::vector<MOVE_LIST> moveList;
	std::vector<PV> pvLine;
	std::vector<MOVE_STACK> moveStack;
	std::vector<UNDO_MOVE_STACK> undoMoveStack;
	std::vector<MOVES_HISTORY> movesHistory;
    std::vector<PawnsHashEntry> pawnsHashTable;
    std::vector<EvalHashEntry> evalHashTable;
	
    EvalInfo evalInfo;
    HashManager hashManager;

	Thread();
    ~Thread();

    void init();
	void clear();
};

class SearchThread : public Thread 
{

public:

	static bool abortSearch;
	static bool stopSearch;
    
    bool terminate;

	int idx, depth, completedDepth, selDepth;

	std::atomic<U64> nodes, ttHits;
    
    std::thread thread;

	std::mutex mutex;
	std::condition_variable cv;
    
    ThreadState mState;
	
	SearchThread(int);
	~SearchThread();

	int getIndex() { return idx; }
	
	void initialise();
	void search();
    
    template<ThreadState state>
    void blockThreadIfState();
};


class SearchThreadPool 
{

std::vector<SearchThread*> searchThreads;

public:

	void createThreadPool(int nThreads);
	void clear();
    
	void wait_for_search_finished();

	U64 totalNodes();
	U64 totalTTHits();

	SearchThread* getMainSearchThread();
	
    std::vector<SearchThread*> getSearchThreads();
    
    template <bool isFromUci>
    void search()
    {
        if (isFromUci) {
            
            getMainSearchThread()->blockThreadIfState<SEARCH>();
            
            SearchThread::stopSearch = false;
            
            for (SearchThread* th : searchThreads)
            {
                th->initialise();
            }
            
            getMainSearchThread()->search();
        } 
        else 
        {
            for (SearchThread* th : searchThreads)
            {
                if (th != getMainSearchThread())
                {
                    th->search();
                }
            }
        }
    }
};

extern Thread initThread;
extern SearchThreadPool Threads;

#endif 
