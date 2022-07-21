#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <chrono>

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

typedef std::chrono::steady_clock::time_point TimePoint;

#endif
