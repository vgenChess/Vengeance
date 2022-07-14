
#ifndef search_h
#define search_h

#include <vector>
#include <memory>

#include "types.h"
#include "functions.h"
#include "namespaces.h"
#include "classes.h"

using namespace game;

void initLMR();
void initLMP();

void startSearch(int index, GameInfo *gi);

template<Side stm>
void iterativeDeepening(int index, GameInfo* gi);

template<Side stm>
void aspirationWindow(int index, GameInfo* gi);

template<Side stm>
int alphabeta(int alpha, int beta, const int mate, GameInfo *gi, SearchInfo* si);

template<Side stm> 
int quiescenseSearch(int alpha, int beta, GameInfo* gi, SearchInfo* si);

#endif /* search_h */
