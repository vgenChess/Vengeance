#ifndef NAMESPACES_H_INCLUDED
#define NAMESPACES_H_INCLUDED

#include <vector>
#include <cstring>
#include <thread>

#include "TimeManagement.h"
#include "zobrist.h"
#include "structs.h"
#include "classes.h"
#include "enums.h"


namespace tmg {

    inline TimeManager timeManager;
};

namespace tt {

    inline U16 age;
    inline U64 size;
    inline HashEntry* hashTable;
};

namespace zobrist {

    inline Zobrist zobrist;
};


#include "HashManagement.h"

namespace game {

    class GameInfo
    {

    public:

        Side stm;

        int mIndex;
        int depth;
        int selDepth;
        int completedDepth;
        int stableMoveCount;

        U16 moves_history_counter;

        U64 nodes;
        U64 ttHits;
        U64 hashKey;
        U64 pawnsHashKey;
        U64 occupied;
        U64 empty;

        // TODO check alignment and datatype and test
        int historyScore[2][64][64];
        int captureHistoryScore[8][64][8]; // [piece][to][c_piece]
        U32 counterMove[2][64][64];

        alignas(64) int16_t accumulator[2][NN_SIZE];
        alignas(64) U64 whitePieceBB[MAX_PIECES];
        alignas(64)	U64 blackPieceBB[MAX_PIECES];

        std::vector<MOVE_LIST> moveList;
        std::vector<PV> pvLine;
        std::vector<MOVE_STACK> moveStack;
        std::vector<UNDO_MOVE_STACK> undoMoveStack;
        std::vector<MOVES_HISTORY> movesHistory;
        std::vector<PawnsHashEntry> pawnsHashTable;
        std::vector<EvalHashEntry> evalHashTable;

        EvalInfo evalInfo;
        HashManager hashManager;

        GameInfo() {

            init();
        }

        ~GameInfo() {

            moveList.clear();
            pvLine.clear();
            moveStack.clear();
            undoMoveStack.clear();
            movesHistory.clear();
            pawnsHashTable.clear();
            evalHashTable.clear();
        }

        void init() {

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

        void clear() {

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

    inline GameInfo *initInfo = new GameInfo();

    inline bool abortSearch;
    inline bool searching;

    inline std::vector<GameInfo*> infos;
    inline std::vector<std::thread> threads;

    template<Stats stats>
    inline U64 getStats() {

        U64 sum = 0;
        for (GameInfo *gi : game::infos) {

            sum += stats == NODES ? gi->nodes : (stats == TTHITS ? gi->ttHits : 0);
        }

        return sum;
    }
};

#endif // NAMESPACES_H_INCLUDED
