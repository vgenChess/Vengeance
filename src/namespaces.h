#ifndef NAMESPACES_H_INCLUDED
#define NAMESPACES_H_INCLUDED

#include <vector>
#include <cstring>
#include <thread>
#include <algorithm>

#include "structs.h"
#include "classes.h"
#include "enums.h"

namespace tmg {

#include "TimeManagement.h"

    inline TimeManager timeManager;

    template<TimeFormat format>
    inline int gameElaspsedTime() {

        if (format == MILLISECONDS)
            return timeManager.timeElapsed<MILLISECONDS>(timeManager.getStartTime());
        else if (format == SECONDS)
            return timeManager.timeElapsed<SECONDS>(timeManager.getStartTime());

        return 0;
    }
};

namespace tt {

    inline U16 age;
    inline U64 size;
    inline HashEntry* hashTable;
};

namespace zobrist {

#include "zobrist.h"

    inline Zobrist zobrist;
};

namespace game {

#include "HashManagement.h"

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
        int historyScore[2][64][64] = {};
        int captureHistoryScore[8][64][8] = {}; // [piece][to][c_piece]
        U32 counterMove[2][64][64] = {};

        alignas(64) int16_t accumulator[2][NN_SIZE] = {};
        alignas(64) U64 whitePieceBB[MAX_PIECES] = {};
        alignas(64)	U64 blackPieceBB[MAX_PIECES] = {};

        std::vector<PV> pvLine          =               std::vector<PV> ( MAX_PLY );
        std::vector<MOVE_STACK> moveStack   =           std::vector<MOVE_STACK> ( MAX_PLY );
        std::vector<UNDO_MOVE_STACK> undoMoveStack =    std::vector<UNDO_MOVE_STACK> ( MAX_PLY );
        std::vector<MOVES_HISTORY> movesHistory =       std::vector<MOVES_HISTORY> (8192);
        std::vector<PawnsHashEntry> pawnsHashTable =    std::vector<PawnsHashEntry> (PAWN_HASH_TABLE_RECORDS);
        std::vector<EvalHashEntry> evalHashTable  =     std::vector<EvalHashEntry> (EVAL_HASH_TABLE_RECORDS);

        EvalInfo evalInfo;
        HashManager hashManager;

        GameInfo() {
        }

        ~GameInfo() {
        }

        void clone(GameInfo *gameInfo) {

            this->stm = gameInfo->stm;

            this->moveStack[0].castleFlags = gameInfo->moveStack[0].castleFlags;
            this->moveStack[0].epFlag = gameInfo->moveStack[0].epFlag;
            this->moveStack[0].epSquare = gameInfo->moveStack[0].epSquare;

            for (int i = 0; i < 8192; i++)
            {
                this->movesHistory[i].hashKey = gameInfo->movesHistory[i].hashKey;
                this->movesHistory[i].fiftyMovesCounter = gameInfo->movesHistory[i].fiftyMovesCounter;
            }

            this->moves_history_counter = gameInfo->moves_history_counter;

            memcpy(this->whitePieceBB, gameInfo->whitePieceBB, sizeof(U64) * MAX_PIECES);
            memcpy(this->blackPieceBB, gameInfo->blackPieceBB, sizeof(U64) * MAX_PIECES);

            this->occupied = gameInfo->occupied;
            this->empty = gameInfo->empty;

            this->hashKey = gameInfo->hashKey;
            this->pawnsHashKey = gameInfo->pawnsHashKey;

            memcpy(this->accumulator[WHITE], gameInfo->accumulator[WHITE], sizeof(int16_t) * NN_SIZE);
            memcpy(this->accumulator[BLACK], gameInfo->accumulator[BLACK], sizeof(int16_t) * NN_SIZE);
        }
    };

    inline bool abortSearch;
    inline bool searching;

    inline uint64_t previousInfoTime;

    inline GameInfo *initInfo = new GameInfo();

    inline std::vector<GameInfo*> infos;
    inline std::vector<std::thread> threads;

    template<Stats stats>
    inline uint64_t getStats() {

        uint64_t sum = 0;
        for (GameInfo *gi : game::infos)
            sum += stats == NODES ? gi->nodes : stats == TTHITS ? gi->ttHits : 0;

        return sum;
    }


    inline void copyPv(U32* pv, U32 move, U32* pvLower)
    {
        *pv++ = move;
        if (pvLower)
            while (*pvLower != NO_MOVE)
                *pv++ = *pvLower++;
        *pv = NO_MOVE;
    }
};


namespace nnue {

#include <immintrin.h>

    inline void m256_add_dpbusd_epi32(__m256i& acc, __m256i a, __m256i b)
    {
        #if defined (USE_VNNI)
        acc = _mm256_dpbusd_epi32(acc, a, b);
        #else
        __m256i product0 = _mm256_maddubs_epi16(a, b);
        product0 = _mm256_madd_epi16(product0, _mm256_set1_epi16(1));
        acc = _mm256_add_epi32(acc, product0);
        #endif
    }

    inline __m128i m256_haddx4(__m256i a, __m256i b, __m256i c, __m256i d)
    {
        a = _mm256_hadd_epi32(a, b);
        c = _mm256_hadd_epi32(c, d);
        a = _mm256_hadd_epi32(a, c);
        const __m128i sum128lo = _mm256_castsi256_si128(a);
        const __m128i sum128hi = _mm256_extracti128_si256(a, 1);
        return _mm_add_epi32(sum128lo, sum128hi);
    }

    inline __m128i m256_haddx4(__m256i sum0, __m256i sum1, __m256i sum2, __m256i sum3, __m128i bias) {

        sum0 = _mm256_hadd_epi32(sum0, sum1);
        sum2 = _mm256_hadd_epi32(sum2, sum3);

        sum0 = _mm256_hadd_epi32(sum0, sum2);

        __m128i sum128lo = _mm256_castsi256_si128(sum0);
        __m128i sum128hi = _mm256_extracti128_si256(sum0, 1);

        return _mm_add_epi32(_mm_add_epi32(sum128lo, sum128hi), bias);
    }

    inline bool exists_file (const std::string& name) {
        if (FILE *file = fopen(name.c_str(), "r")) {
            fclose(file);
            return true;
        } else {
            return false;
        }
    }

    inline bool skipNetHeader(char **data) {

        char  *iter = *data;
        iter += 4;

        *data = iter;

        return true;
    }
};




#endif // NAMESPACES_H_INCLUDED
