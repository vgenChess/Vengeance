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

	static bool abortSearch, stopSearch;
    
    bool mTerminate, mSearching;

	int mIndex, depth, completedDepth, selDepth;

	U64 nodes, ttHits;
    
    std::thread mThread;

	std::mutex mMutex;
	std::condition_variable mCv;
	
	SearchThread(int index);
	~SearchThread();

	int getIndex() { return mIndex; }
	
	void initialise();
	void search();
	void startSearch(Side stm);
	void waitIfSearching();
	void searchThreadLifeCycle();
};

class SearchThreadPool 
{

std::vector<SearchThread*> threads;

public:

	void createThreadPool(int n);
	void clear();
	void waitForAll();

	U64 totalNodes();
	U64 totalTTHits();

	SearchThread* getMainSearchThread();
	SearchThread* getBestThread();

    std::vector<SearchThread*> getSearchThreads();
    
    template <bool isInit>
    void search()
    {
        auto mainThread = threads[0];

        if (isInit) {
            
            mainThread->waitIfSearching();
            
            SearchThread::stopSearch = false;
            
            for (SearchThread* th : threads) th->initialise();
            
            mainThread->search();
        } 
        else 
        {
            for (SearchThread* th : threads)
            {
                if (th != mainThread) th->search();
            }
        }
    }
};

extern Thread initThread;
extern SearchThreadPool searchThreads;

#endif 
