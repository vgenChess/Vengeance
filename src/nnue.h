#pragma once
#include "structs.h"

#include "thread.h"

#ifndef NN_H_INCLUDED
#define NN_H_INCLUDED


bool loadNetwork();
void nnue_init(void);
bool nnue_load_net(std::string path);

void refresh_accumulator(Thread* th, Side side);
void update_accumulator(Thread *th, const std::vector<int>& removed_features, const std::vector<int>& added_features, Side side);

int predict (Side stm, Thread *th);

#endif // NN_H_INCLUDED
