
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "thread.h"
#include "evaluate.h"

void initLMR();

void startSearch(Side stm);
void iterativeDeepeningSearch(Side stm, SearchThread *thread);

template<Side stm> 
void aspirationWindowSearch(SearchThread *th);

template<Side stm, bool isNullMoveAllowed, bool isSingularSearch>
int alphabetaSearch(int alpha, int beta, const int mate, SearchThread *th, SearchInfo *si);

template<Side stm> 
int quiescenseSearch(int ply, int alpha, int beta, SearchThread *th, std::vector<U32> *pline);

#endif /* search_h */
