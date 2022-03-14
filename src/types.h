#ifndef TYPES_H
#define TYPES_H

#include <vector>

/*

The primitive data types available in C++ are as follows:


Type 				Description 						Bytes *  			Range *
======================================================================================================================================================

char 				character or small integer 			1 	 				signed: -128 to 127
																			unsigned: 0 to 255
______________________________________________________________________________________________________________________________________________________

int 				integer 							short: 2 			signed short: -32,768 to 32,767
																			unsigned short: 0 to 65,535

														normal: 4           signed: -2,147,483,648 to 2,147,483,647
																			unsigned: 0 to 4,294,967,295

														long: 4 			signed long: -2,147,483,648 to 2,147,483,647
																			unsigned long: 0 to 4,294,967,295

														long long: 8 		signed long long: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
																			unsigned long long: 0 to 18,446,744,073,709,551,615
______________________________________________________________________________________________________________________________________________________
																					
bool 				boolean value  						1 					true or false
______________________________________________________________________________________________________________________________________________________

float 				floating-point number 				4 					1.17549*10-38 to 3.40282*1038
______________________________________________________________________________________________________________________________________________________

double 				double-precision 					8 					2.22507*10-308 to 1.79769*10308
					floatingpoint number
______________________________________________________________________________________________________________________________________________________

long double 		extended-precision  				12 					3.36210*10-4932 to 1.18973*104932
					floatingpoint number
______________________________________________________________________________________________________________________________________________________

wchar_t 			wide character or short 			2 					1 wide character
					integer
______________________________________________________________________________________________________________________________________________________
*/


typedef unsigned char 		U8;
typedef unsigned short 		U16;
typedef unsigned long 		U32;
typedef unsigned long long	U64;

typedef signed char 		S8;
typedef signed short 		S16;
typedef signed long 		S32;
typedef signed long long	S64;

enum { NORMAL, SAFETY };
enum { MG, EG };

enum Side {

	WHITE = 0, BLACK = 1
};

enum {

	NUL = true, NO_NULL = false,
	SING = true, NON_SING = false
};

enum Piece {
	DUMMY = 0, PAWNS = 1, KNIGHTS = 2, BISHOPS = 3, 
	ROOKS = 4, QUEEN = 5, KING = 6, PIECES = 7
};

enum {
	VALUE_DUMMY = 0,
	VALUE_PAWN = 100, VALUE_KNIGHT = 300, VALUE_BISHOP = 300,
	VALUE_ROOK = 500, VALUE_QUEEN = 900, VALUE_KING = 2000
};

enum {
	MOVE_NORMAL = 0, MOVE_CAPTURE = 1, MOVE_DOUBLE_PUSH = 2,
	MOVE_ENPASSANT = 3, MOVE_CASTLE = 4, MOVE_PROMOTION = 5
};

enum {
	CASTLE_FLAG_WHITE_QUEEN = 1, CASTLE_FLAG_WHITE_KING = 2,
	CASTLE_FLAG_BLACK_QUEEN = 4, CASTLE_FLAG_BLACK_KING = 8
};

enum {
	WHITE_CASTLE_QUEEN_SIDE = 0, WHITE_CASTLE_KING_SIDE = 1,
	BLACK_CASTLE_QUEEN_SIDE = 2, BLACK_CASTLE_KING_SIDE = 3
};

enum {
	PROMOTE_TO_QUEEN = 0, PROMOTE_TO_ROOK = 1,
	PROMOTE_TO_BISHOP = 2, PROMOTE_TO_KNIGHT = 3
};

enum Stage {
	
	PLAY_HASH_MOVE = 		0, 
	GEN_CAPTURES = 			10, 
	PLAY_CAPTURES = 		20, 
	PLAY_KILLER_MOVE_1 = 	30,
	PLAY_KILLER_MOVE_2 = 	40, 
	PLAY_COUNTER_MOVE = 	50, 
	GEN_PROMOTIONS = 		60,
	PLAY_PROMOTIONS = 		70, 
	PLAY_BAD_CAPTURES = 	80, 
	GEN_QUIETS = 			90, 
	PLAY_QUIETS = 			100, 
	STAGE_DONE = 			1000
};

enum {
	
	hashfEXACT = 5, 
	hashfALPHA = 10, 
	hashfBETA = 15
};

enum ThreadState {

	SLEEP, SEARCH, EXIT
};

#endif
