//
//  uci.h
//  Vgen
//
//  Created by Amar Thapa on 25/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#ifndef uci_h
#define uci_h

#include <stdio.h>

#include "globals.h"
#include "thread.h"

void UciLoop(void);
u8 addFileNumber(char ch);
u8 parsePosition (std::string str, Thread *th);

#endif /* uci_h */
