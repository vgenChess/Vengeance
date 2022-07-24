
#ifndef NN_H_INCLUDED
#define NN_H_INCLUDED

#include "structs.h"
#include "namespaces.h"

using namespace game;

bool loadNetwork();

void refresh_accumulator ( GameInfo* gi, Side side);

void update_accumulator ( GameInfo* gi,
                         const std::vector<int>& removed_features,
                         const std::vector<int>& added_features,
                         Side side);

int predict (Side stm, GameInfo *gi );

#endif // NN_H_INCLUDED
