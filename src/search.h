
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "thread.h"
#include "evaluate.h"

void initLMR();

void startSearch(U8 stm);
void iterativeDeepeningSearch(U8 stm, SearchThread *thread);
template<Side stm> void aspirationWindowSearch(SearchThread *th);
template<Side stm> int alphabetaSearch(int alpha, int beta, int mate, SearchThread *th, SearchInfo *si);
template<Side stm> int quiescenseSearch(int ply, int alpha, int beta, SearchThread *th, std::vector<U32> *pline);

template<Side stm> void getMoveList(int ply, int side, std::vector<Move> &moves, U8 stage, Thread *th);

U64 attacksTo(U64 occupied, U8 square, U8 side, Thread *th);

#endif /* search_h */
