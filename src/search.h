
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
int32_t alphabetaSearch(int32_t alpha, int32_t beta, const int32_t mate, SearchThread *th, SearchInfo *si);

template<Side stm> 
int32_t quiescenseSearch(int ply, int32_t alpha, int32_t beta, SearchThread *th, std::vector<U32> *pline);

#endif /* search_h */
