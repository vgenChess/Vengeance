
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "thread.h"
#include "evaluate.h"

extern std::vector<Thread> sThreads;	

class SearchInfo {

public:
		
	U8 side;
	int ply;
	int depth;
	int realDepth;
	
	bool isNullMoveAllowed;	 

	U32 skipMove;

	std::vector<U32> pline;

	SearchInfo() {
		
		side = WHITE;
		ply = 0;
		depth = 0;
	
		isNullMoveAllowed = false;

		skipMove = NO_MOVE;

		pline.clear();
	}
};

void initLMR();

void startSearch(U8 side);
void iterativeDeepeningSearch(int side, SearchThread *thread);
void aspirationWindowSearch(U8 side, SearchThread *th);

int alphabetaSearch(int alpha, int beta, int mate, SearchThread *th, SearchInfo *si);
int quiescenseSearch(int ply, int side, int alpha, int beta, SearchThread *th, std::vector<U32> *pline);

void updateHistory(int ply, int side, int depth, U32 bestMove, std::vector<U32> &quietMovesPlayed, Thread *th);
void updateCaptureHistory(int ply, int side, int depth, U32 bestMove, std::vector<U32> &captureMovesPlayed, Thread *th);

void getMoveList(int ply, int side, std::vector<Move> &moves, U8 stage, Thread *th);

U64 attacksTo(U64 occupied, U8 square, U8 side, Thread *th);

// int QuiescenseForTuning(int ply, int side, int alpha, int beta, int depth, Thread *th, TraceCoefficients *T);

#endif /* search_h */
