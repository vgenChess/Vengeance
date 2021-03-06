//
//  fen.cpp
//  Vgen
//
//  Created by Amar Thapa on 25/12/18.
//


#include <stdlib.h> 
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <assert.h>

#include "functions.h"
#include "fen.h"
#include "utility.h"
#include "structs.h"
#include "zobrist.h"

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch) {

    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}


Side parseFen(std::string str, Thread *th) {


    th->hashKey = 0ULL;
    th->pawnsHashKey = 0ULL;
            
	Side side = WHITE;

    std::vector <std::string> tokens1; 

    split(str, tokens1, ' ');


    
    // parse side to move ------------------------------------

    std::string strSide = tokens1[1];

    if (strSide[0] == 'w') side = WHITE;
    else if (strSide[0] == 'b') side = BLACK;

    assert (side == WHITE || side == BLACK);

    if (side) th->hashKey ^= Zobrist::objZobrist.KEY_SIDE_TO_MOVE;
    
    //-------------------------------------------------------




    // parse castle flags -----------------------------------


    std::string strCastleFlags = tokens1[2];

	U8 flag = 0;    

    for (auto &ch : strCastleFlags) {
        
        if (ch == 'K') {
            
            flag |= CASTLE_FLAG_WHITE_KING;

            th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_KING_SIDE;
        } else if (ch == 'k') {
            
            flag |= CASTLE_FLAG_BLACK_KING;

            th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_KING_SIDE; 
        } else if (ch == 'Q') {
            
            flag |= CASTLE_FLAG_WHITE_QUEEN;

            th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_WHITE_CASTLE_QUEEN_SIDE;
        } else if (ch == 'q') {
            
            flag |= CASTLE_FLAG_BLACK_QUEEN;

            th->hashKey ^= Zobrist::objZobrist.KEY_FLAG_BLACK_CASTLE_QUEEN_SIDE;
        }
    }


    th->moveStack[0].castleFlags = flag;

    //-----------------------------------------------------------

    


    // parse en-passant flags -----------------------------------

    std::string checkEpSquare = tokens1[3];    
    if (checkEpSquare[0] != '-') {
        
        th->moveStack[0].epFlag = 1;

        th->moveStack[0].epSquare = squareFromAlgebricPos(checkEpSquare.c_str());

        U64 epSqBitboard = 1ULL << th->moveStack[0].epSquare;

        if (     epSqBitboard & A_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_A_FILE;
        else if (epSqBitboard & B_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_B_FILE;
        else if (epSqBitboard & C_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_C_FILE;
        else if (epSqBitboard & D_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_D_FILE;
        else if (epSqBitboard & E_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_E_FILE;
        else if (epSqBitboard & F_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_F_FILE;
        else if (epSqBitboard & G_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_G_FILE;
        else if (epSqBitboard & H_FILE) th->hashKey ^= Zobrist::objZobrist.KEY_EP_H_FILE;
    } else {
		
        th->moveStack[0].epFlag = 0;
        th->moveStack[0].epSquare = 100; // 100th square does not exist in standard chess
	}


    //----------------------------------------------------------

    
    std::vector <std::string> tokens2; 

    split(tokens1[0], tokens2, '/');


    int pos = 63;
    
    for (int i = 0; i < 8; i++) {
        
        char reverse[10];
        int reversePos = 0;
        
        for (int j = tokens2[i].length(); j > 0; j--) {
            
            char ch = tokens2[i].at(j - 1);
            
            reverse[reversePos] = ch;
            reversePos++;
        }
        
        reverse[reversePos] = '\0';
        
        reversePos = 0;
        
        while (reverse[reversePos] != '\0') {

            switch (reverse[reversePos]) {
                    
                    //Black side
                case 'k':
                    
                    th->blackPieceBB[KING] |= (1ULL << pos);
                    
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[KING][BLACK][pos];
                        
                    pos--;
                    break;
                case 'q':
                    
                    th->blackPieceBB[QUEEN] |= (1ULL << pos);
                    
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[QUEEN][BLACK][pos];
                        
                    pos--;
                    break;
                case 'b':
                    th->blackPieceBB[BISHOPS] |= (1ULL << pos);
                    
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[BISHOPS][BLACK][pos];
                    
                    pos--;
                    break;
                case 'n':
                    th->blackPieceBB[KNIGHTS] |= (1ULL << pos);
                   
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[KNIGHTS][BLACK][pos];
                       
                    pos--;
                    break;
                case 'r':
                    th->blackPieceBB[ROOKS] |= (1ULL << pos);
                   
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][BLACK][pos];
                       
                    pos--;
                    break;
                case 'p':
                    th->blackPieceBB[PAWNS] |= (1ULL << pos);
                   
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][BLACK][pos];
                    th->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[pos];   

                    pos--;
                    break;
               

                    //White side
                case 'K':
                    th->whitePieceBB[KING] |= (1ULL << pos);
                    
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[KING][WHITE][pos];
                     
                    pos--;
                    break;
                case 'Q':

                    th->whitePieceBB[QUEEN] |= (1ULL << pos);
                
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[QUEEN][WHITE][pos];
                 
                    pos--;
                    break;
                case 'B':

                    th->whitePieceBB[BISHOPS] |= (1ULL << pos);
                
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[BISHOPS][WHITE][pos];
                         
                    pos--;
                    break;
                case 'N':
                
                    th->whitePieceBB[KNIGHTS] |= (1ULL << pos);
                
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[KNIGHTS][WHITE][pos];
                     
                    pos--;
                    break;
                case 'R':
                
                    th->whitePieceBB[ROOKS] |= (1ULL << pos);
                
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[ROOKS][WHITE][pos];
                    
                    pos--;
                    break;
                case 'P':
                
                    th->whitePieceBB[PAWNS] |= (1ULL << pos);
                    
                    th->hashKey ^= Zobrist::objZobrist.zobristKey[PAWNS][WHITE][pos];            
                    th->pawnsHashKey ^= Zobrist::objZobrist.pawnZobristKey[pos];

                    pos--;
                    break;
                    
                    //    No of spaces
                case '1':
                    pos = pos - 1;
                    break;
                case '2':
                    pos = pos - 2;
                    break;
                case '3':
                    pos = pos - 3;
                    break;
                case '4':
                    pos = pos - 4;
                    break;
                case '5':
                    pos = pos - 5;
                    break;
                case '6':
                    pos = pos - 6;
                    break;
                case '7':
                    pos = pos - 7;
                    break;
                case '8':
                    pos = pos - 8;
                    break;
                default: 
                    break;       
            }

            reversePos++;
        }
    }


    th->whitePieceBB[PIECES] = 
            th->whitePieceBB[KING] 
        |   th->whitePieceBB[QUEEN]
        |   th->whitePieceBB[BISHOPS]
        |   th->whitePieceBB[KNIGHTS]
        |   th->whitePieceBB[ROOKS] 
        |   th->whitePieceBB[PAWNS];


    th->blackPieceBB[PIECES] = 
            th->blackPieceBB[KING] 
        |   th->blackPieceBB[QUEEN]
        |   th->blackPieceBB[BISHOPS] 
        |   th->blackPieceBB[KNIGHTS]
        |   th->blackPieceBB[ROOKS] 
        |   th->blackPieceBB[PAWNS];
    

    th->occupied = th->whitePieceBB[PIECES] | th->blackPieceBB[PIECES];


    th->empty = ~(th->occupied);


    // print_board(th->occupied, th);

            
    // std::cout << "Side to move : " << (side == 0 ? "WHITE" : "BLACK") << "\n";
    // std::cout << "castleFlags = " <<(int) th->moveStack[0].castleFlags << "\n";
    // std::cout << "epSquare = " << (int) th->moveStack[0].epSquare << "\n";


    return side;
}

