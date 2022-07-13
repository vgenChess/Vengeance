#include <assert.h>
#include <cstring>

#include "thread.h"
#include "search.h"
#include "nnue.h"

GameInfo initThread;

GameInfo::GameInfo()
{
    init();
}

void GameInfo::clear()
{
    moveList.clear();
    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();

    memset(captureHistoryScore, 0, sizeof(int) * 8 * 64 * 8);
    memset(historyScore, 0, sizeof(int) * 2 * 64 * 64);
    memset(counterMove, 0, sizeof(U32) * 2 * 64 * 64);
    memset(whitePieceBB, 0, sizeof(U64) * 8);
    memset(blackPieceBB, 0, sizeof(U64) * 8);

    occupied = 0;
    empty = 0;
}

void GameInfo::init()
{
    moveList = 		std::vector<MOVE_LIST> ( MAX_MOVES );
    pvLine = 		std::vector<PV> ( MAX_PLY );
    moveStack = 	std::vector<MOVE_STACK> ( MAX_PLY );
    undoMoveStack = std::vector<UNDO_MOVE_STACK> ( MAX_PLY );
    movesHistory  = std::vector<MOVES_HISTORY> (8192);
    pawnsHashTable = std::vector<PawnsHashEntry>(PAWN_HASH_TABLE_RECORDS);
    evalHashTable = std::vector<EvalHashEntry>(EVAL_HASH_TABLE_RECORDS);

    memset(captureHistoryScore, 0, sizeof(int) * 8 * 64 * 8);
    memset(historyScore, 0, sizeof(int) * 2 * 64 * 64);
    memset(counterMove, 0, sizeof(U32) * 2 * 64 * 64);
    memset(whitePieceBB, 0, sizeof(U64) * 8);
    memset(blackPieceBB, 0, sizeof(U64) * 8);

    occupied = 0;
    empty = 0;
}

GameInfo::~GameInfo()
{
    moveList.clear();
    pvLine.clear();
    moveStack.clear();
    undoMoveStack.clear();
    movesHistory.clear();
    pawnsHashTable.clear();
    evalHashTable.clear();
}

