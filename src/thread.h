#ifndef thread_h
#define thread_h

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <cstring>

#include "constants.h"
#include "classes.h"
#include "structs.h"
#include "HashManagement.h"


class GameInfo
{
public:

	static bool abortSearch;
	static bool searching;

	int mIndex, depth, completedDepth, selDepth;

	U64 nodes, ttHits;

	int stableMoveCount;

	Side stm;
    
	U16 moves_history_counter;

	int historyScore[2][64][64];
	int captureHistoryScore[8][64][8]; // [piece][to][c_piece]
	
	U32 counterMove[2][64][64];
    
    U64 hashKey, pawnsHashKey;
	U64 occupied, empty;

    alignas(64) U64 whitePieceBB[MAX_PIECES];
	alignas(64)	U64 blackPieceBB[MAX_PIECES];

	alignas(64) int16_t accumulator[2][NN_SIZE];

	std::vector<MOVE_LIST> moveList;
	std::vector<PV> pvLine;
	std::vector<MOVE_STACK> moveStack;
	std::vector<UNDO_MOVE_STACK> undoMoveStack;
	std::vector<MOVES_HISTORY> movesHistory;
	std::vector<PawnsHashEntry> pawnsHashTable;
	std::vector<EvalHashEntry> evalHashTable;

    EvalInfo evalInfo;
    HashManager hashManager;

	GameInfo();
    ~GameInfo();

	void init();
	void clear();

	void clone(GameInfo *gameInfo) {

		stm = gameInfo->stm;

		moveStack[0].castleFlags = gameInfo->moveStack[0].castleFlags;
		moveStack[0].epFlag = gameInfo->moveStack[0].epFlag;
		moveStack[0].epSquare = gameInfo->moveStack[0].epSquare;

		for (int i = 0; i < 8192; i++)
		{
			movesHistory[i].hashKey = gameInfo->movesHistory[i].hashKey;
			movesHistory[i].fiftyMovesCounter = gameInfo->movesHistory[i].fiftyMovesCounter;
		}

		moves_history_counter = gameInfo->moves_history_counter;

		memcpy(whitePieceBB, gameInfo->whitePieceBB, sizeof(U64) * MAX_PIECES);
		memcpy(blackPieceBB, gameInfo->blackPieceBB, sizeof(U64) * MAX_PIECES);

		occupied = gameInfo->occupied;
		empty = gameInfo->empty;

		hashKey = gameInfo->hashKey;
		pawnsHashKey = gameInfo->pawnsHashKey;
	}
};

#endif 
