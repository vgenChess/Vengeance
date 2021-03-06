
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "thread.h"
#include "evaluate.h"

void initLMR();
void initLMP();

template<Side stm> void iterativeDeepening(SearchThread* th);
template<Side stm> void aspirationWindow(SearchThread* th);

template<Side stm, bool isNullMoveAllowed, bool isSSearch>
int alphabeta(int alpha, int beta, const int mate, SearchThread* th, SearchInfo* si);

template<Side stm> 
int quiescenseSearch(int alpha, int beta, SearchThread* th, SearchInfo* si);

#endif /* search_h */
