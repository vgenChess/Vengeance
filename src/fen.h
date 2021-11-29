//
//  fen.h
//  Vgen
//
//  Created by Amar Thapa on 25/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef fen_h
#define fen_h

#include <stdio.h>

#include "globals.h"
#include "thread.h"

u8 parseFen(std::string str, Thread* th);
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch);

#endif /* fen_h */
