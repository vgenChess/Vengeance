//
//  perft.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "perft.h"
#include "movegen.h"
#include "make_unmake.h"
#include "utility.h"
#include "misc.h"

// TODO fix perft 

void startPerft(Side side, U8 depth, GameInfo *th) {
    
    U64 nodes;
    
    clock_t start, end;
    double cpu_time_used;
    double nps;
    
    for (int i = 1; i <= depth; i++) {
        
        nodes = 0;
        int ply = 0;
        
        start = clock();
        
        nodes = perft(ply, i, side, th);
        
        end = clock();
        
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        nps = (double) (nodes / (cpu_time_used * 1000000));
        printf("Depth(%d)=   ", i);
        printf("%10llu (%8.3f sec), color - %s, %7.3f MN/s\n",
               nodes, cpu_time_used, ((side == 0) ? "WHITE" : "BLACK"), nps);
    }
}

U64 perft(int ply, U8 depth, Side side, GameInfo *th) {

    U64 nodes = 0;
    
    if (depth == 0) 
        return 1;
    
    if (ply != 0) {
    
        th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
        th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
        th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
    }

    MOVE_LIST moveList;
    
    moveList.skipQuiets = false;
    moveList.stage = PLAY_HASH_MOVE;
    moveList.ttMove = NO_MOVE;
    moveList.counterMove = NO_MOVE;
    moveList.moves.clear();
    moveList.badCaptures.clear();
    
    Move currentMove;
    
    while (true) 
    {
        // fetch next psuedo-legal move
        currentMove = getNextMove(side, ply, th, &moveList);

        if (moveList.stage == STAGE_DONE)
        {
            break;
        }
        
        // check if move generator returns a valid psuedo-legal move
        assert(currentMove.move != NO_MOVE);

        // make the move
        make_move(ply, currentMove.move, th);
        
        // check if psuedo-legal move is valid
        if (side == WHITE)
        {
            if (isKingInCheck<WHITE>(th)) 
            {
                unmake_move(ply, currentMove.move, th);
                continue;
            }
        }
        else 
        {
            if (isKingInCheck<BLACK>(th)) 
            {
                unmake_move(ply, currentMove.move, th);
                continue;
            }
        }
        
        nodes += perft(ply + 1, depth - 1, side == WHITE ? BLACK : WHITE, th);
    }

    return nodes;
}
