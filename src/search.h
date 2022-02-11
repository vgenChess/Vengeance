
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "globals.h"
#include "thread.h"
#include "evaluate.h"


extern std::vector<Thread> sThreads;	


class SearchInfo {

	public:
		
		u8 side;
		int ply;
		int depth;
		int realDepth;
		
		bool isNullMoveAllowed;	 

		SearchInfo() {
			
			side = WHITE;
			ply = 0;
			depth = 0;
		
			isNullMoveAllowed = false;
		}
};


void initThreads();

void startSearch(u8 sideToMove);
void searchMain(int side, SearchThread *thread);
void aspirationWindowSearch(u8 sideToMove, SearchThread *th, const int depth);

void display(u8 sideToMove, int depth, int selDepth, int score, std::vector<u32> pvLine);

void updateHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &quietMovesPlayed, Thread *th);
void updateCaptureHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &captureMovesPlayed, Thread *th);

int alphabetaSearch(int alpha, int beta, SearchThread *th, std::vector<u32> *pline, SearchInfo *si, int mate);

int quiescenseSearch(const int ply, const int depth, const int side, int alpha, int beta, 
	SearchThread *th, std::vector<u32> *pline);

void getMoveList(int ply, int side, std::vector<Move> &moves, u8 stage, Thread *th);

u64 attacksTo(u64 occupied, u8 square, u8 sideToMove, Thread *th);

void debugSEE(char ch, int sq);

int QuiescenseForTuning(int ply, int side, int alpha, int beta, int depth, Thread *th, TraceCoefficients *T);

#endif /* search_h */
