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

#include "perft.h"
#include "movegen.h"
#include "make_unmake.h"
#include "utility.h"

void startPerft(U8 side, U8 depth, Thread *th) {
    
    U64 nodes;
    
    clock_t start, end;
    double cpu_time_used;
    double nps;
    
    prevCap = 0;
    prevEp = 0;
    prevCas = 0;
    
    for (int i = 1; i <= depth; i++) {
        
        nodes = 0;
        quiet = 0;
        cap = 0;
        ep = 0;
        cas = 0;
        check = 0;
        int ply = 0;
        prom = 0;
        
        start = clock();
        
        nodes = perft(ply, i, side, th);
        
        end = clock();
        
        cap = cap - prevCap;
        ep = ep - prevEp;
        cas = cas - prevCas;
        
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        nps = (double) (nodes / (cpu_time_used * 1000000));
        printf("Depth(%d)=   ", i);
        printf("%10llu (%8.3f sec), color - %s, captures - %8llu, en - %6llu, cas - %6llu, prom - %8llu, %7.3f MN/s\n",
               nodes, cpu_time_used, ((side == 0) ? "WHITE" : "BLACK"), cap, ep, cas, prom, nps);
        
        prevCap = cap;
        prevEp = ep;
        prevCas = cas;
    }
}

U64 perft(int ply, U8 depth, U8 side, Thread *th) {
    
    Move move_list[U16_MAX_MOVES];
    U64 nodes = 0;
    U8 i;
    
    if (depth == 0) 
        return 1;
    
    if (ply != 0) {
    
        th->moveStack[ply].epFlag = th->moveStack[ply - 1].epFlag;
        th->moveStack[ply].epSquare = th->moveStack[ply - 1].epSquare;
        th->moveStack[ply].castleFlags = th->moveStack[ply - 1].castleFlags;
    }

    std::vector<Move> moves;
    Move m;
    // for (int stage = STAGE_PROMOTIONS; stage <= STAGE_NORMAL_MOVES; stage++) {

    //     moves.clear();
    //     getMoves(ply, side, moves, stage, false, th);       

    //     for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); i++) {
            
    //         m = *i;

    //         make_move(ply, m.move, th);

    //         if (!isKingInCheck(side, th))
    //             nodes += perft(ply + 1, depth - 1, side ^ 1, th);
    //         else {

    //             switch (move_type(m.move)) {

    //                 case MOVE_CAPTURE: cap--; break;
    //                 case MOVE_PROMOTION: prom--; break;
    //                 case MOVE_CASTLE: cas--; break;
    //                 case MOVE_ENPASSANT: ep--; break;
    //             }
    //         }

    //         unmake_move(ply, m.move, th);
    //     }
    // }
    
    return nodes;
}
