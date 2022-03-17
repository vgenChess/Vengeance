//
//  utility.c
//  Vgen
//
//  Created by Amar Thapa on 23/12/18.
//  Copyright © 2018 Amar Thapa. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <cstring>
#include <assert.h>

#include "nonslidingmoves.h"
#include "utility.h"
#include "magicmoves.h"
#include "make_unmake.h"
#include "movegen.h"
#include "perft.h"
#include "functions.h"
#include "zobrist.h"
#include "misc.h"

U64 arrInBetween[64][64];

std::string Notation::algebricSq[64] = {
    
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
}; 

void print_bb(U64 board) {
    
    printf(" \n ---------- ");
    int row;
    int col;
    for (row = 7; row >= 0; row--) {
        printf("\n|%d ", row + 1);
        for (col = 0; col < 8; col++) {
            if ((((U64) 1) << ((row * 8 + col)) & board)
                == (U64) 1 << (row * 8 + col)) {
                putchar('x');
            } else
                putchar('.');
        }
        printf("|");
    }
    printf("\n|  abcdefgh|\n");
    printf(" ---------- \n");
}

void print_board(U64 board, Thread *th) {

    printf("\n -------------------- \n");
    printf("|                    |");
 
    int row;
    int col;
    for (row = 7; row >= 0; row--) {
        printf("\n|%d  ", row + 1);
        for (col = 0; col < 8; col++) {
            if ((((U64) 1) << ((row * 8 + col)) & board)
                == (U64) 1 << (row * 8 + col)) {
                
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[KING])
                    putchar('K');
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[QUEEN])
                    putchar('Q');
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[BISHOPS])
                    putchar('B');
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[KNIGHTS])
                    putchar('N');
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[ROOKS])
                    putchar('R');
                if ((U64) 1 << (row * 8 + col) & th->whitePieceBB[PAWNS])
                    putchar('P');
                
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[KING])
                    putchar('k');
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[QUEEN])
                    putchar('q');
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[BISHOPS])
                    putchar('b');
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[KNIGHTS])
                    putchar('n');
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[ROOKS])
                    putchar('r');
                if ((U64) 1 << (row * 8 + col) & th->blackPieceBB[PAWNS])
                    putchar('p');
            } else {

                if (row % 2 == 0) {

                    if (col % 2 == 0)
                        putchar('.');
                    else 
                        putchar('.');   
                } else {

                    if (col % 2 == 0)
                        putchar('.');
                    else 
                        putchar('.');
                }
            }
            
            putchar(' ');
        }
        printf(" |");
    }

    printf("\n|                    |\n");
    printf("|   a b c d e f g h  |\n");
    printf(" -------------------- \n");
}

/**
 * Flip a bitboard vertically about the centre ranks.
 * Rank 1 is mapped to rank 8 and vice versa.
 * @param x any bitboard
 * @return bitboard x flipped vertically
 */
U64 flipVertical(U64 x) {
    return  ( (x << 56)                           ) |
            ( (x << 40) & C64(0x00ff000000000000) ) |
            ( (x << 24) & C64(0x0000ff0000000000) ) |
            ( (x <<  8) & C64(0x000000ff00000000) ) |
            ( (x >>  8) & C64(0x00000000ff000000) ) |
            ( (x >> 24) & C64(0x0000000000ff0000) ) |
            ( (x >> 40) & C64(0x000000000000ff00) ) |
            ( (x >> 56) );
}

U64 getAttacks(const U8 stm, Thread *th) {

    U64 attacks = 0ULL, b;

    attacks |= stm ?
        bPawnWestAttacks(th->blackPieceBB[PAWNS]) | bPawnEastAttacks(th->blackPieceBB[PAWNS]): 
        wPawnWestAttacks(th->whitePieceBB[PAWNS]) | wPawnEastAttacks(th->whitePieceBB[PAWNS]);

    b = stm ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS];
    while(b) {
        
        attacks |= get_knight_attacks(GET_POSITION(b));
        POP_POSITION(b);
    }

    b = stm ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS];
    while(b) {

        attacks |= Bmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }

    b = stm ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS];
    while(b) {
        
        attacks |= Rmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }
    
    b = stm ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN];
    while(b) {
        
        attacks |= Qmagic(GET_POSITION(b), th->occupied);
        POP_POSITION(b);
    }  

    int kingSq = GET_POSITION(stm ? th->blackPieceBB[KING] : th->whitePieceBB[KING]);

    attacks |= get_king_attacks(kingSq);

    return attacks;
} 

/**
 * Function to check if a kingSq is attacked.
 * @return true or false
 */
bool isSqAttacked(U8 sq, U8 side, Thread *th) {
    
    U64 attacks;
    
    /* check if a king is attacking a sq */
    
    attacks = get_king_attacks(sq);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[KING] : th->whitePieceBB[KING])) {

        return true;
    }
    
    /* check if a queen is attacking a sq */
    
    attacks = Qmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[QUEEN] : th->whitePieceBB[QUEEN])) {

        return true;
    }
    
    /* check if a bishop is attacking a kingSq */
    
    attacks = Bmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[BISHOPS] : th->whitePieceBB[BISHOPS])) {

        return true;
    }
    
    /* check if a knight is attacking a kingSq */
    
    attacks = get_knight_attacks(sq);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[KNIGHTS] : th->whitePieceBB[KNIGHTS])) {

        return true;
    }
    
    /* check if a rook is attacking a kingSq */
    
    attacks = Rmagic(sq, th->occupied);
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[ROOKS] : th->whitePieceBB[ROOKS])) {

        return true;
    }
    
    /* check if a pawn is attacking a kingSq */
    
    if (side == WHITE) {
        
        attacks = (((1ULL << sq) << 7) & NOT_H_FILE) | (((1ULL << sq) << 9) & NOT_A_FILE);
    } else {

        attacks = (((1ULL << sq) >> 7) & NOT_A_FILE) | (((1ULL << sq) >> 9) & NOT_H_FILE);
    }
    
    if (attacks & ((side ^ 1) ? th->blackPieceBB[PAWNS] : th->whitePieceBB[PAWNS])) {

        return true;
    }
    
    return false;
}

U8 squareFromAlgebricPos(char* posName) {
    
    if (strcmp(posName, "a1") == 0) {
        return 0;
    } else if (strcmp(posName, "b1") == 0) {
        return 1;
    } else if (strcmp(posName, "c1") == 0) {
        return 2;
    } else if (strcmp(posName, "d1") == 0) {
        return 3;
    } else if (strcmp(posName, "e1") == 0) {
        return 4;
    } else if (strcmp(posName, "f1") == 0) {
        return 5;
    } else if (strcmp(posName, "g1") == 0) {
        return 6;
    } else if (strcmp(posName, "h1") == 0) {
        return 7;
    }
    
    if (strcmp(posName, "a2") == 0) {
        return 8;
    } else if (strcmp(posName, "b2") == 0) {
        return 9;
    } else if (strcmp(posName, "c2") == 0) {
        return 10;
    } else if (strcmp(posName, "d2") == 0) {
        return 11;
    } else if (strcmp(posName, "e2") == 0) {
        return 12;
    } else if (strcmp(posName, "f2") == 0) {
        return 13;
    } else if (strcmp(posName, "g2") == 0) {
        return 14;
    } else if (strcmp(posName, "h2") == 0) {
        return 15;
    }
    
    if (strcmp(posName, "a3") == 0) {
        return 16;
    } else if (strcmp(posName, "b3") == 0) {
        return 17;
    } else if (strcmp(posName, "c3") == 0) {
        return 18;
    } else if (strcmp(posName, "d3") == 0) {
        return 19;
    } else if (strcmp(posName, "e3") == 0) {
        return 20;
    } else if (strcmp(posName, "f3") == 0) {
        return 21;
    } else if (strcmp(posName, "g3") == 0) {
        return 22;
    } else if (strcmp(posName, "h3") == 0) {
        return 23;
    }
    
    if (strcmp(posName, "a4") == 0) {
        return 24;
    } else if (strcmp(posName, "b4") == 0) {
        return 25;
    } else if (strcmp(posName, "c4") == 0) {
        return 26;
    } else if (strcmp(posName, "d4") == 0) {
        return 27;
    } else if (strcmp(posName, "e4") == 0) {
        return 28;
    } else if (strcmp(posName, "f4") == 0) {
        return 29;
    } else if (strcmp(posName, "g4") == 0) {
        return 30;
    } else if (strcmp(posName, "h4") == 0) {
        return 31;
    }
    
    if (strcmp(posName, "a5") == 0) {
        return 32;
    } else if (strcmp(posName, "b5") == 0) {
        return 33;
    } else if (strcmp(posName, "c5") == 0) {
        return 34;
    } else if (strcmp(posName, "d5") == 0) {
        return 35;
    } else if (strcmp(posName, "e5") == 0) {
        return 36;
    } else if (strcmp(posName, "f5") == 0) {
        return 37;
    } else if (strcmp(posName, "g5") == 0) {
        return 38;
    } else if (strcmp(posName, "h5") == 0) {
        return 39;
    }
    
    if (strcmp(posName, "a6") == 0) {
        return 40;
    } else if (strcmp(posName, "b6") == 0) {
        return 41;
    } else if (strcmp(posName, "c6") == 0) {
        return 42;
    } else if (strcmp(posName, "d6") == 0) {
        return 43;
    } else if (strcmp(posName, "e6") == 0) {
        return 44;
    } else if (strcmp(posName, "f6") == 0) {
        return 45;
    } else if (strcmp(posName, "g6") == 0) {
        return 46;
    } else if (strcmp(posName, "h6") == 0) {
        return 47;
    }
    
    if (strcmp(posName, "a7") == 0) {
        return 48;
    } else if (strcmp(posName, "b7") == 0) {
        return 49;
    } else if (strcmp(posName, "c7") == 0) {
        return 50;
    } else if (strcmp(posName, "d7") == 0) {
        return 51;
    } else if (strcmp(posName, "e7") == 0) {
        return 52;
    } else if (strcmp(posName, "f7") == 0) {
        return 53;
    } else if (strcmp(posName, "g7") == 0) {
        return 54;
    } else if (strcmp(posName, "h7") == 0) {
        return 55;
    }
    
    if (strcmp(posName, "a8") == 0) {
        return 56;
    } else if (strcmp(posName, "b8") == 0) {
        return 57;
    } else if (strcmp(posName, "c8") == 0) {
        return 58;
    } else if (strcmp(posName, "d8") == 0) {
        return 59;
    } else if (strcmp(posName, "e8") == 0) {
        return 60;
    } else if (strcmp(posName, "f8") == 0) {
        return 61;
    } else if (strcmp(posName, "g8") == 0) {
        return 62;
    } else if (strcmp(posName, "h8") == 0) {
        return 63;
    }
    
    return 0;
}

std::string algSq[64] = {

    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
}; 

int divide(U8 depth, U8 sideToMove, Thread *th) {
    
    char pieceName[2][8] = { { ' ', (char) 0, 'N', 'B', 'R', 'Q', 'K', '\0'}, { ' ', (char) 0, 'n', 'b', 'r', 'q', 'k', '\0'}};
    
    U64 total_nodes = 0;

    char pType[24];
    char* moveType;
    U64 nodes;

    std::vector<Move> moves;

    int ply = 0;
    genMoves(sideToMove, ply, moves, th);
        
    U8 count = 0;

    clock_t start, end;
    double cpu_time_used;
    double nps;

	Move move;
    for (std::vector<Move>::iterator i = moves.begin(); i != moves.end(); ++i)
    {

        start = clock();
       
		move = *i;
		
        make_move(ply, move.move, th);
        
        switch(move_type(move.move)) {

            case MOVE_NORMAL:

                moveType = "MOVE_NORMAL";
            break;
            case MOVE_CAPTURE:

                moveType = "MOVE_CAPTURE";
            break;
            case MOVE_DOUBLE_PUSH:
            
                moveType = "MOVE_DOUBLE_PUSH";
            break;
            case MOVE_ENPASSANT:
            
                moveType = "MOVE_ENPASSANT";
            break;
            case MOVE_CASTLE:
            
                moveType = "MOVE_CASTLE";
            break;
            case MOVE_PROMOTION:
    
                sprintf(pType, "MOVE_PROMOTION-%d", promType(move.move));
                moveType = pType;
            break;
        }
        
        nodes = 0;

        if (sideToMove ? !(isKingInCheck<BLACK>(th)) : !(isKingInCheck<WHITE>(th))) {

            count++;

            nodes = (sideToMove ^ 1) ? perft(ply + 1, depth - 1, BLACK, th) : perft(ply + 1, depth - 1, WHITE, th);
            
            end  = clock();
            
            cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
            nps = (double)(nodes / (cpu_time_used * 1000000));
            
            total_nodes = total_nodes + nodes;
            
            printf("%d)%c%s-%s, ", count, pieceName[colorType(move.move)][pieceType(move.move)],
                   algSq[from_sq(move.move)], algSq[to_sq(move.move)]);
            
            printf("%llu, %s, castle flags -" BYTE_TO_BINARY_PATTERN", nps  - %7.3f MN/s\n", nodes, moveType, BYTE_TO_BINARY(th->moveStack[ply].castleFlags),  nps);
        }
        
        unmake_move(ply, move.move, th);
    }
    
    printf("Total nodes -> %llu\n", total_nodes);

    return 0;
}

void initHashKey(Thread *th) {
	
    int sq;
	U64 bitboard;
	
	th->hashKey = 0ULL;
    for(int side = WHITE; side <= BLACK; side++) {
    
        for(int piece = PAWNS; piece <= KING; piece++) {
            
            bitboard = side ? th->blackPieceBB[piece] : th->whitePieceBB[piece];
            while (bitboard) {
                
                sq = GET_POSITION(bitboard);
                POP_POSITION(bitboard);
                    
                assert(sq >= 0 && sq < 64);    

				th->hashKey ^= Zobrist::objZobrist.zobristKey[piece][side][sq];
            }
        }
    }

    th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
    th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
    th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
    th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE; 
}

void initPawnHashKey(U8 side, Thread *th) {

    U64 bitboard = th->blackPieceBB[PAWNS] | th->whitePieceBB[PAWNS];
    
    th->pawnsHashKey = 0ULL;

    U8 sq;
    while(bitboard) {

        sq = GET_POSITION(bitboard);
        POP_POSITION(bitboard);

        th->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[sq];
    }

    if (side) 
        th->hashKey ^= Zobrist::objZobrist.KEY_SIDE_TO_MOVE;
}


void initMovesHistoryTable(Thread *th) {
	
	for (int i = 0; i < U16_MAX_PLY; i++) {
		
		th->movesHistory[i].hashKey = 0ULL;
	}

    th->movesHistory[0].fiftyMovesCounter = 0;
	
    th->moves_history_counter = 0;
}

void clearKillerMovesTable(Thread *th) {
	
	for (int i = 0; i < U16_MAX_PLY; i++) {

        th->moveStack[i].killerMoves[0] = NO_MOVE;
        th->moveStack[i].killerMoves[1] = NO_MOVE;
	}
}


U64 nortFill(U64 gen) {
   gen |= (gen <<  8);
   gen |= (gen << 16);
   gen |= (gen << 32);
   return gen;
}

U64 soutFill(U64 gen) {
   gen |= (gen >>  8);
   gen |= (gen >> 16);
   gen |= (gen >> 32);
   return gen;
}

U64 soutOne (U64 b) {return  b >> 8;}
U64 nortOne (U64 b) {return  b << 8;}
U64 eastOne (U64 b) {return (b << 1) & NOT_A_FILE;}
U64 westOne (U64 b) {return (b >> 1) & NOT_H_FILE;}
U64 noEaOne (U64 b) {return (b << 9) & NOT_A_FILE;}
U64 soEaOne (U64 b) {return (b >> 7) & NOT_A_FILE;}
U64 soWeOne (U64 b) {return (b >> 9) & NOT_H_FILE;}
U64 noWeOne (U64 b) {return (b << 7) & NOT_H_FILE;}

U64 wFrontSpans(U64 wpawns) {return nortOne (nortFill(wpawns));}
U64 bRearSpans (U64 bpawns) {return nortOne (nortFill(bpawns));}
U64 bFrontSpans(U64 bpawns) {return soutOne (soutFill(bpawns));}
U64 wRearSpans (U64 wpawns) {return soutOne (soutFill(wpawns));}

U64 wFrontFill(U64 wpawns) {return nortFill(wpawns);}
U64 wRearFill (U64 wpawns) {return soutFill(wpawns);}

U64 bFrontFill(U64 bpawns) {return soutFill(bpawns);}
U64 bRearFill (U64 bpawns) {return nortFill(bpawns);}

U64 fileFill(U64 gen) {
   return nortFill(gen) | soutFill(gen);
}

U64 wEastAttackFrontSpans (U64 wpawns) {return eastOne(wFrontSpans(wpawns));}
U64 wWestAttackFrontSpans (U64 wpawns) {return westOne(wFrontSpans(wpawns));}
U64 bEastAttackFrontSpans (U64 bpawns) {return eastOne(bFrontSpans(bpawns));}
U64 bWestAttackFrontSpans (U64 bpawns) {return westOne(bFrontSpans(bpawns));}

U64 wEastAttackRearSpans (U64 wpawns)  {return eastOne(wRearFill(wpawns));}
U64 wWestAttackRearSpans (U64 wpawns)  {return westOne(wRearFill(wpawns));}
U64 bEastAttackRearSpans (U64 bpawns)  {return eastOne(bRearFill(bpawns));}
U64 bWestAttackRearSpans (U64 bpawns)  {return westOne(bRearFill(bpawns));}

U64 eastAttackFileFill (U64 pawns) {return eastOne(fileFill(pawns));}
U64 westAttackFileFill (U64 pawns) {return westOne(fileFill(pawns));}

U64 wPawnEastAttacks(U64 wpawns) {return noEaOne(wpawns);}
U64 wPawnWestAttacks(U64 wpawns) {return noWeOne(wpawns);}

U64 bPawnEastAttacks(U64 bpawns) {return soEaOne(bpawns);}
U64 bPawnWestAttacks(U64 bpawns) {return soWeOne(bpawns);}

U64 pawnsWithEastNeighbors(U64 pawns) {
   return pawns & westOne (pawns);
}

U64 pawnsWithWestNeighbors(U64 pawns) {
   return pawnsWithEastNeighbors(pawns) << 1; // * 2
}

U64 islandsEastFiles(U64 f) {return f & (f ^ (f >> 1));}
U64 islandsWestFiles(U64 f) {return f & (f ^ (f << 1));} // ... (f+f)

U64 defendedDefenders1 (U64 b) {return b & soWeOne(b) & noEaOne(b);}
U64 defendedDefenders2 (U64 b) {return b & soEaOne(b) & noWeOne(b);}

void initCastleMaskAndFlags() 
{    
    for (int i = 0; i < U8_MAX_SQUARES; i++) 
    {
        rookCastleFlagMask[i] = 15;
    }
    
    rookCastleFlagMask[0] ^= CASTLE_FLAG_WHITE_QUEEN;
    rookCastleFlagMask[7] ^= CASTLE_FLAG_WHITE_KING;
    rookCastleFlagMask[56] ^= CASTLE_FLAG_BLACK_QUEEN;
    rookCastleFlagMask[63] ^= CASTLE_FLAG_BLACK_KING;
}

void init_inbetween_bb() 
{    
    for (int i = 0; i < U8_MAX_SQUARES; i++) 
    {
        for(int j = 0; j < U8_MAX_SQUARES; j++) 
        {
            arrInBetween[i][j] = inBetweenOnTheFly(i, j);
        }
    }
}

U64 inBetween(int from, int to) 
{
   return arrInBetween[from][to];
}

U64 inBetweenOnTheFly(U8 sq1, U8 sq2) 
{
   const U64 m1   = C64(-1);
   const U64 a2a7 = C64(0x0001010101010100);
   const U64 b2g7 = C64(0x0040201008040200);
   const U64 h1b7 = C64(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
   U64 btwn, line, rank, file;

   btwn  = (m1 << sq1) ^ (m1 << sq2);
   file  =   (sq2 & 7) - (sq1   & 7);
   rank  =  ((sq2 | 7) -  sq1) >> 3 ;
   line  =      (   (file  &  7) - 1) & a2a7; /* a2a7 if same file */
   line += 2 * ((   (rank  &  7) - 1) >> 58); /* b1g1 if same rank */
   line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
   line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
   line *= btwn & -btwn; /* mul acts like shift by smaller kingSq */
   return line & btwn;   /* return the bits on that line in-between */
}

U64 xrayRookAttacks(U64 occ, U64 blockers, U8 rookSq) {
   U64 attacks = Rmagic(rookSq, occ);
   blockers &= attacks;
   return attacks ^ Rmagic(rookSq, occ ^ blockers);
}

U64 xrayBishopAttacks(U64 occ, U64 blockers, U8 bishopSq) {
   U64 attacks = Bmagic(bishopSq, occ);
   blockers &= attacks;
   return attacks ^ Bmagic(bishopSq, occ ^ blockers);
}

U64 pinners(U8 kingSq, U8 side, Thread *th) {

    U64 pinners1 = xrayRookAttacks(th->occupied, side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES],
                                   kingSq) & (side ^ 1 ? th->blackPieceBB[ROOKS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[ROOKS] | th->whitePieceBB[QUEEN]);
    
    U64 pinners2 = xrayBishopAttacks(th->occupied, 
                                     (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]), 
                                     kingSq) & (side ^ 1 ? th->blackPieceBB[BISHOPS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[BISHOPS] | th->whitePieceBB[QUEEN]);
    
    return pinners1 | pinners2;
}

U64 pinnedPieces(U8 kingSq, U8 side, Thread *th) {

    U64 pinned = 0ULL;
    
    U64 pinner = xrayRookAttacks(th->occupied, side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES],
                                 kingSq) & (side ^ 1 ? th->blackPieceBB[ROOKS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[ROOKS] | th->whitePieceBB[QUEEN]);
    while (pinner) {

        int sq  = GET_POSITION(pinner);
        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
        POP_POSITION(pinner);
    }
    
    pinner = xrayBishopAttacks(th->occupied, (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]),
                               kingSq) & (side ^ 1 ? th->blackPieceBB[BISHOPS] | th->blackPieceBB[QUEEN] : th->whitePieceBB[BISHOPS] | th->whitePieceBB[QUEEN]);
    while ( pinner ) 
    {
        int sq  = GET_POSITION(pinner);
        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);
        POP_POSITION(pinner);
    }

    return pinned;
}

U64 pinned(U64 pinners, U8 kingSq, U8 side, Thread *th) 
{
    U64 pinned = 0ULL;
    
    while ( pinners ) 
    {
        int sq  = GET_POSITION(pinners);

        pinned |= inBetween(sq, kingSq) & (side ? th->blackPieceBB[PIECES] : th->whitePieceBB[PIECES]);

        POP_POSITION(pinners);
    }

    return pinned;
}




