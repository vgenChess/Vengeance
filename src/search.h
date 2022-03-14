
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "thread.h"
#include "evaluate.h"

void initLMR();

void startSearch(Side stm, SearchThread* th);
template<Side stm> void iterativeDeepeningSearch(SearchThread* th);
template<Side stm> void aspirationWindowSearch(SearchThread* th);

template<Side stm, bool isNullMoveAllowed, bool isSingularSearch>
int alphabetaSearch(int alpha, int beta, const int mate, SearchThread* th, SearchInfo* si);

template<Side stm> 
int quiescenseSearch(int alpha, int beta, SearchThread* th, SearchInfo* si);

#endif /* search_h */
