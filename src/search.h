
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

	std::vector<u32> pline;

	SearchInfo() {
		
		side = WHITE;
		ply = 0;
		depth = 0;
	
		isNullMoveAllowed = false;

		pline.clear();
	}
};


void startSearch(u8 sideToMove);
void searchMain(int side, SearchThread *thread);
void aspirationWindowSearch(u8 sideToMove, SearchThread *th);

void display(u8 sideToMove, int depth, int selDepth, int score, std::vector<u32> pvLine);

void updateHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &quietMovesPlayed, Thread *th);
void updateCaptureHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &captureMovesPlayed, Thread *th);

int32_t alphabetaSearch(int32_t alpha, int32_t beta, SearchThread *th, SearchInfo *si);

int32_t quiescenseSearch(int32_t ply, int8_t side, int32_t alpha, int32_t beta, SearchThread *th, std::vector<u32> *pline);

void getMoveList(int ply, int side, std::vector<Move> &moves, u8 stage, Thread *th);

u64 attacksTo(u64 occupied, u8 square, u8 sideToMove, Thread *th);

void debugSEE(char ch, int sq);

// int QuiescenseForTuning(int ply, int side, int alpha, int beta, int depth, Thread *th, TraceCoefficients *T);

#endif /* search_h */
