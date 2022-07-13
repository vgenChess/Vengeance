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

#include "types.h"
#include "thread.h"

Side parseFen(std::string str, GameInfo* th);
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch);

#endif /* fen_h */
