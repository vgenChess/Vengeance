
#include <assert.h> 

#include <cmath>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono> 
#include <algorithm>
#include <random>
#include <vector>
#include <future> 

#include "tuner.h"
#include "hash.h"
#include "search.h"
#include "evaluate.h"
#include "make_unmake.h"
#include "utility.h"
#include "fen.h"


#define MAXEPOCHS      1000000000
#define NPARTITIONS    			4 
#define BATCHSIZE      		   16 
#define NPOSITIONS     	   725000 
#define DISPLAY_TIME 		   60				
#define LRDROPRATE			    1


struct Score {
	
	double mg;
	double eg;
};

int seePieceVal[8] = { 0, 100, 300, 300, 500, 900, 2000, 0 };

int scaling_constant = 575;

double K = 1.13;

std::vector<bool> paramHistoryList;
std::vector<std::thread> threads;
std::vector<Thread> tunerThreads;
std::vector<Data> dataList;


double sigmoid(double K, double E) {
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}


void loadWeights(TVector params, TVector cparams) {
	
	TVector weights = {0};

    // Combine updated and current parameters
    for (int j = 0; j < NTERMS; j++) {

        weights[MG][j] = round(params[MG][j] + cparams[MG][j]);
        weights[EG][j] = round(params[EG][j] + cparams[EG][j]);
    }

	int count = 0;

	weight_pawn = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_knight = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_bishop = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_rook = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_queen = MakeScore(weights[MG][count], weights[EG][count++]);
	
	weight_isolated_pawn = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_backward_pawn = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_double_pawn = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_defended_pawn = MakeScore(weights[MG][count], weights[EG][count++]);

    for(int i = 0; i < 8; i++) {

    	arr_weight_passed_pawn[i] = MakeScore(weights[MG][count], weights[EG][count++]);
    }

    for(int i = 0; i < 8; i++) {
    	
    	arr_weight_defended_passed_pawn[i] = MakeScore(weights[MG][count], weights[EG][count++]);
    }


    weight_bishop_pair = MakeScore(weights[MG][count], weights[EG][count++]);

	
	weight_rook_half_open_file = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_rook_open_file = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_rook_enemy_queen_same_file = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_rook_supporting_friendly_rook = MakeScore(weights[MG][count], weights[EG][count++]);

	
	weight_queen_underdeveloped_pieces = MakeScore(weights[MG][count], weights[EG][count++]);


	weight_king_pawn_shield = MakeScore(weights[MG][count], weights[EG][count++]);
	weight_king_enemy_pawn_storm = MakeScore(weights[MG][count], weights[EG][count++]);

    for (int i = 0; i < 16; i++) {
    	
    	arr_weight_knight_mobility[i] = MakeScore(weights[MG][count], weights[EG][count++]);    
    }

    for (int i = 0; i < 16; i++) {
    	
    	arr_weight_bishop_mobility[i] = MakeScore(weights[MG][count], weights[EG][count++]);    
    }

    for (int i = 0; i < 16; i++) {
    	
    	arr_weight_rook_mobility[i] = MakeScore(weights[MG][count], weights[EG][count++]);    
    }

    for (int i = 0; i < 32; i++) {
    	
    	arr_weight_queen_mobility[i] = MakeScore(weights[MG][count], weights[EG][count++]);    
    }

    for(int i = 8; i <= 55; i++) pawnPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]); 
    for(int i = 0; i <= 63; i++) knightPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]); 
    for(int i = 0; i <= 63; i++) bishopPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]); 
    for(int i = 0; i <= 63; i++) rookPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]); 
    for(int i = 0; i <= 63; i++) queenPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]);
    for(int i = 0; i <= 63; i++) kingPSQT[i] = MakeScore(weights[MG][count], weights[EG][count++]); 

    assert(count == NTERMS);
}

void loadCoefficients(TraceCoefficients *T, TVector coeffs, TArray type) {

	int i = 0;

	// Material

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->nPawns[WHITE];                         
    coeffs[BLACK][i++] = T->nPawns[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->nKnights[WHITE];                         
    coeffs[BLACK][i++] = T->nKnights[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->nBishops[WHITE];                         
    coeffs[BLACK][i++] = T->nBishops[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->nRooks[WHITE];                         
    coeffs[BLACK][i++] = T->nRooks[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->nQueen[WHITE];                         
    coeffs[BLACK][i++] = T->nQueen[BLACK];   

                        

    // Pawns

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->isolatedPawns[WHITE];                         
    coeffs[BLACK][i++] = T->isolatedPawns[BLACK];  

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->backwardPawns[WHITE];                         
    coeffs[BLACK][i++] = T->backwardPawns[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->doublePawns[WHITE];                         
    coeffs[BLACK][i++] = T->doublePawns[BLACK];                         

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->defendedPawns[WHITE];                         
    coeffs[BLACK][i++] = T->defendedPawns[BLACK];     

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->pawnHoles[WHITE];                         
    coeffs[BLACK][i++] = T->pawnHoles[BLACK];                         
	
    for (int k = 0; k < 8; k++) {
	
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->passedPawn[WHITE][k];                         
	    coeffs[BLACK][i++] = T->passedPawn[BLACK][k];                         
	}
    
    for (int k = 0; k < 8; k++) {
	
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->defendedPassedPawn[WHITE][k];                         
	    coeffs[BLACK][i++] = T->defendedPassedPawn[BLACK][k];                         
	}

	// Knights

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->undefendedKnight[WHITE];                         
    coeffs[BLACK][i++] = T->undefendedKnight[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->knightDefendedByPawn[WHITE];                         
    coeffs[BLACK][i++] = T->knightDefendedByPawn[BLACK];                         

	// Bishops

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->bishopPair[WHITE];                         
    coeffs[BLACK][i++] = T->bishopPair[BLACK]; 

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->undefendedBishop[WHITE];                         
    coeffs[BLACK][i++] = T->undefendedBishop[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->badBishop[WHITE];                         
    coeffs[BLACK][i++] = T->badBishop[BLACK];                         

                             
    // Rooks                     

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->halfOpenFile[WHITE];                         
    coeffs[BLACK][i++] = T->halfOpenFile[BLACK];                         

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->openFile[WHITE];                         
    coeffs[BLACK][i++] = T->openFile[BLACK];                         

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->rookEnemyQueenSameFile[WHITE];                         
    coeffs[BLACK][i++] = T->rookEnemyQueenSameFile[BLACK];                         

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->rookSupportingFriendlyRook[WHITE];                         
    coeffs[BLACK][i++] = T->rookSupportingFriendlyRook[BLACK];                         

	// type[i] = NORMAL;
	// coeffs[WHITE][i] = T->rookBlockedByKing[WHITE];                         
 //    coeffs[BLACK][i++] = T->rookBlockedByKing[BLACK];                         

	type[i] = NORMAL;
	coeffs[WHITE][i] = T->rookOnSeventhRank[WHITE];                         
    coeffs[BLACK][i++] = T->rookOnSeventhRank[BLACK];                         
    
    type[i] = NORMAL;
	coeffs[WHITE][i] = T->rookOnEightRank[WHITE];                         
    coeffs[BLACK][i++] = T->rookOnEightRank[BLACK];                         
    

    // Queen

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->queenUnderdevelopedPieces[WHITE];                         
    coeffs[BLACK][i++] = T->queenUnderdevelopedPieces[BLACK];                         
  	
  	// King

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->kingPawnShield[WHITE];                         
    coeffs[BLACK][i++] = T->kingPawnShield[BLACK];                         

	type[i] = NORMAL;
    coeffs[WHITE][i] = T->kingEnemyPawnStorm[WHITE];                         
    coeffs[BLACK][i++] = T->kingEnemyPawnStorm[BLACK]; 


    // Pieces Mobility
    for (int k = 0; k < 16; k++) {
	
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->knightMobility[WHITE][k];                         
	    coeffs[BLACK][i++] = T->knightMobility[BLACK][k];                         
	}

    for (int k = 0; k < 16; k++) {
		
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->bishopMobility[WHITE][k];                         
	    coeffs[BLACK][i++] = T->bishopMobility[BLACK][k];                         
	}

    for (int k = 0; k < 16; k++) {
		
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->rookMobility[WHITE][k];                         
	    coeffs[BLACK][i++] = T->rookMobility[BLACK][k];                         
	}
	
    for (int k = 0; k < 32; k++) {
	
		type[i] = NORMAL;	
	    coeffs[WHITE][i] = T->queenMobility[WHITE][k];                         
	    coeffs[BLACK][i++] = T->queenMobility[BLACK][k];                         
	}



    //PSQT

    for (int k = 8; k < 56; k++) {
	
		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->pawnPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->pawnPSQT[BLACK][k];                         
    }

    for (int k = 0; k < 64; k++) {

		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->knightPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->kingPSQT[BLACK][k];                         
    }

    for (int k = 0; k < 64; k++) {

		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->bishopPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->bishopPSQT[BLACK][k];                         
    }

    for (int k = 0; k < 64; k++) {

		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->rookPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->rookPSQT[BLACK][k];                         
    }

    for (int k = 0; k < 64; k++) {

		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->queenPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->queenPSQT[BLACK][k];                         
    }

    for (int k = 0; k < 64; k++) {

		type[i] = NORMAL;
	    coeffs[WHITE][i] = T->kingPSQT[WHITE][k];                         
	    coeffs[BLACK][i++] = T->kingPSQT[BLACK][k];                         
    }
    

    type[i] = NORMAL;
    coeffs[WHITE][i] = T->centerControl[WHITE];                         
    coeffs[BLACK][i++] = T->centerControl[BLACK];                         



	type[i] = SAFETY;
    coeffs[WHITE][i] = T->knightAttack[WHITE];                         
    coeffs[BLACK][i++] = T->knightAttack[BLACK];                         

	type[i] = SAFETY;
    coeffs[WHITE][i] = T->bishopAttack[WHITE];                         
    coeffs[BLACK][i++] = T->bishopAttack[BLACK];                         

	type[i] = SAFETY;
    coeffs[WHITE][i] = T->rookAttack[WHITE];                         
    coeffs[BLACK][i++] = T->rookAttack[BLACK];                         

	type[i] = SAFETY;
    coeffs[WHITE][i] = T->queenAttack[WHITE];                         
    coeffs[BLACK][i++] = T->queenAttack[BLACK];                         





	type[i] = SAFETY;
    coeffs[WHITE][i] = T->rookSafeContactCheck[WHITE];                         
    coeffs[BLACK][i++] = T->rookSafeContactCheck[BLACK];                         

	type[i] = SAFETY;
    coeffs[WHITE][i] = T->queenSafeContactCheck[WHITE];                         
    coeffs[BLACK][i++] = T->queenSafeContactCheck[BLACK];                         






    type[i] = SAFETY;
    coeffs[WHITE][i] = T->knightCheck[WHITE];                         
    coeffs[BLACK][i++] = T->knightCheck[BLACK];                         

    type[i] = SAFETY;
    coeffs[WHITE][i] = T->bishopCheck[WHITE];                         
    coeffs[BLACK][i++] = T->bishopCheck[BLACK];                         

    type[i] = SAFETY;
    coeffs[WHITE][i] = T->rookCheck[WHITE];                         
    coeffs[BLACK][i++] = T->rookCheck[BLACK];                         

    type[i] = SAFETY;
    coeffs[WHITE][i] = T->queenCheck[WHITE];                         
    coeffs[BLACK][i++] = T->queenCheck[BLACK];                         



    type[i] = SAFETY;
    coeffs[WHITE][i] = T->safetyAdjustment[WHITE];                         
    coeffs[BLACK][i++] = T->safetyAdjustment[BLACK];                         



    assert(i == NTERMS);
}

void startTuner() {

	int count = 0;

	TVector params = {0}, cparams = {0};

	cparams[MG][count] = ScoreMG(weight_pawn);
	cparams[EG][count++] = ScoreEG(weight_pawn);
	
	cparams[MG][count] = ScoreMG(weight_knight);
	cparams[EG][count++] = ScoreEG(weight_knight);

	cparams[MG][count] = ScoreMG(weight_bishop);
	cparams[EG][count++] = ScoreEG(weight_bishop);
	
	cparams[MG][count] = ScoreMG(weight_rook);
	cparams[EG][count++] = ScoreEG(weight_rook);

	cparams[MG][count] = ScoreMG(weight_queen);
	cparams[EG][count++] = ScoreEG(weight_queen);


	cparams[MG][count] = ScoreMG(weight_isolated_pawn);
	cparams[EG][count++] = ScoreEG(weight_isolated_pawn);
	
	cparams[MG][count] = ScoreMG(weight_backward_pawn);
	cparams[EG][count++] = ScoreEG(weight_backward_pawn);

	cparams[MG][count] = ScoreMG(weight_double_pawn);
	cparams[EG][count++] = ScoreEG(weight_double_pawn);
	
	cparams[MG][count] = ScoreMG(weight_defended_pawn);
	cparams[EG][count++] = ScoreEG(weight_defended_pawn);

	cparams[MG][count] = ScoreMG(weight_pawn_hole);
	cparams[EG][count++] = ScoreEG(weight_pawn_hole);

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_passed_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_passed_pawn[i]);
	}

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_defended_passed_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_defended_passed_pawn[i]);
	}


	cparams[MG][count] = ScoreMG(weight_undefended_knight);
	cparams[EG][count++] = ScoreEG(weight_undefended_knight);
	
	cparams[MG][count] = ScoreMG(weight_knight_defended_by_pawn);
	cparams[EG][count++] = ScoreEG(weight_knight_defended_by_pawn);



	cparams[MG][count] = ScoreMG(weight_undefended_bishop);
	cparams[EG][count++] = ScoreEG(weight_undefended_bishop);

	cparams[MG][count] = ScoreMG(weight_bishop_pair);
	cparams[EG][count++] = ScoreEG(weight_bishop_pair);

	cparams[MG][count] = ScoreMG(weight_bad_bishop);
	cparams[EG][count++] = ScoreEG(weight_bad_bishop);




	cparams[MG][count] = ScoreMG(weight_rook_half_open_file);
	cparams[EG][count++] = ScoreEG(weight_rook_half_open_file);

	cparams[MG][count] = ScoreMG(weight_rook_open_file);
	cparams[EG][count++] = ScoreEG(weight_rook_open_file);

	cparams[MG][count] = ScoreMG(weight_rook_enemy_queen_same_file);
	cparams[EG][count++] = ScoreEG(weight_rook_enemy_queen_same_file);

	cparams[MG][count] = ScoreMG(weight_rook_supporting_friendly_rook);
	cparams[EG][count++] = ScoreEG(weight_rook_supporting_friendly_rook);

	// cparams[MG][count] = ScoreMG(weight_rook_blocked_by_king);
	// cparams[EG][count++] = ScoreEG(weight_rook_blocked_by_king);

	cparams[MG][count] = ScoreMG(weight_rook_on_seventh_rank);
	cparams[EG][count++] = ScoreEG(weight_rook_on_seventh_rank);

	cparams[MG][count] = ScoreMG(weight_rook_on_eight_rank);
	cparams[EG][count++] = ScoreEG(weight_rook_on_eight_rank);





	cparams[MG][count] = ScoreMG(weight_queen_underdeveloped_pieces);
	cparams[EG][count++] = ScoreEG(weight_queen_underdeveloped_pieces);


	cparams[MG][count] = ScoreMG(weight_king_pawn_shield);
	cparams[EG][count++] = ScoreEG(weight_king_pawn_shield);

	cparams[MG][count] = ScoreMG(weight_king_enemy_pawn_storm);
	cparams[EG][count++] = ScoreEG(weight_king_enemy_pawn_storm);

	
	for (int i = 0; i < 16; i++) {

		cparams[MG][count] = ScoreMG(arr_weight_knight_mobility[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_knight_mobility[i]);
	}

	for (int i = 0; i < 16; i++) {

		cparams[MG][count] = ScoreMG(arr_weight_bishop_mobility[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_bishop_mobility[i]);
	}

	for (int i = 0; i < 16; i++) {

		cparams[MG][count] = ScoreMG(arr_weight_rook_mobility[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_rook_mobility[i]);
	}

	for (int i = 0; i < 32; i++) {

		cparams[MG][count] = ScoreMG(arr_weight_queen_mobility[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_queen_mobility[i]);
	}

	for (int i = 8; i <= 55; i++) {

		cparams[MG][count] = ScoreMG(pawnPSQT[i]);
		cparams[EG][count++] = ScoreEG(pawnPSQT[i]);
	}

	for (int i = 0; i <= 63; i++) {
	
		cparams[MG][count] = ScoreMG(knightPSQT[i]);
		cparams[EG][count++] = ScoreEG(knightPSQT[i]);
	}

	for (int i = 0; i <= 63; i++) {
	
		cparams[MG][count] = ScoreMG(bishopPSQT[i]);
		cparams[EG][count++] = ScoreEG(bishopPSQT[i]);
	}

	for (int i = 0; i <= 63; i++) {
	
		cparams[MG][count] = ScoreMG(rookPSQT[i]);
		cparams[EG][count++] = ScoreEG(rookPSQT[i]);
	}

	for (int i = 0; i <= 63; i++) {
	
		cparams[MG][count] = ScoreMG(queenPSQT[i]);
		cparams[EG][count++] = ScoreEG(queenPSQT[i]);
	}
	
	for (int i = 0; i <= 63; i++) {

		cparams[MG][count] = ScoreMG(kingPSQT[i]);
		cparams[EG][count++] = ScoreEG(kingPSQT[i]);
	}


	cparams[MG][count] = ScoreMG(weight_center_control);
	cparams[EG][count++] = ScoreEG(weight_center_control);


	cparams[MG][count] = ScoreMG(weight_knight_attack);
	cparams[EG][count++] = ScoreEG(weight_knight_attack);

	cparams[MG][count] = ScoreMG(weight_bishop_attack);
	cparams[EG][count++] = ScoreEG(weight_bishop_attack);

	cparams[MG][count] = ScoreMG(weight_rook_attack);
	cparams[EG][count++] = ScoreEG(weight_rook_attack);

	cparams[MG][count] = ScoreMG(weight_queen_attack);
	cparams[EG][count++] = ScoreEG(weight_queen_attack);

	cparams[MG][count] = ScoreMG(weight_rook_safe_contact_check);
	cparams[EG][count++] = ScoreEG(weight_rook_safe_contact_check);

	cparams[MG][count] = ScoreMG(weight_queen_safe_contact_check);
	cparams[EG][count++] = ScoreEG(weight_queen_safe_contact_check);

	cparams[MG][count] = ScoreMG(weight_knight_check);
	cparams[EG][count++] = ScoreEG(weight_knight_check);

	cparams[MG][count] = ScoreMG(weight_bishop_check);
	cparams[EG][count++] = ScoreEG(weight_bishop_check);

	cparams[MG][count] = ScoreMG(weight_rook_check);
	cparams[EG][count++] = ScoreEG(weight_rook_check);

	cparams[MG][count] = ScoreMG(weight_queen_check);
	cparams[EG][count++] = ScoreEG(weight_queen_check);



	cparams[MG][count] = ScoreMG(weight_safety_adjustment);
	cparams[EG][count++] = ScoreEG(weight_safety_adjustment);



	// displayWeights(params, cparams);

	// std::cout<<count<<"\n";


	assert(count == NTERMS);

	std::fstream newfile;
   	newfile.open ("quiet-labeled.epd", std::ios::in); 
	
	count = 0;

	TraceCoefficients *T = new TraceCoefficients();

	// read data from file to a vector
	if (newfile.is_open()) {   

		std::string tp;

		Thread th;	
	    th.moves_history_counter = 1;
		


		while (getline(newfile, tp)) {


			double result;



			std::string fen = tp.substr(0, tp.find("\""));
			//std::string fen = tp.substr(0, tp.find(";"));


			if (fen.length() <= 0) continue;



			// unsigned first = fen.find('{') + 1;
			// unsigned last = fen.find('}');
			// std::string strNew = fen.substr (first, last-first);
			
			// int stored_eval = std::stoi(strNew);


			// for file "quiet-labeled.epd"

			if (tp.find("1-0") != std::string::npos)	
				result = 1.0;
			else if (tp.find("1/2-1/2") != std::string::npos) 
				result = 0.5;
			else if (tp.find("0-1") != std::string::npos)	
				result = 0.0;
			else 
				continue;


			// for file "4818922_positions_gm2600.txt"

			// if (tp.find("1.0") != std::string::npos)	
			// 	result = 1.0;
			// else if (tp.find("0.5") != std::string::npos) 
			// 	result = 0.5;
			// else if (tp.find("0.0") != std::string::npos)	
			// 	result = 0.0;
			// else 
			// 	continue;

			
			assert(result == 1.0 || result == 0.5 || result == 0.0);



			Data data = {};	

			data.result = result;

			TVector coeffs;  

			T->clear();

			u8 side = parseFen(fen, &th);
			
			data.sEval = traceFullEval(T, side, &th);
			//data.sEval = QuiescenseForTuning(0, side, -VAL_INFINITY, VAL_INFINITY, 12, &th, T);  

			// data.result = stored_eval;
			
			if (side == BLACK) {

				data.sEval = -data.sEval;
				// data.result = -data.result;
			}
						

			data.eval = T->eval;

			data.pfactors[MG] = 0 + T->phase / 24.0;
		    data.pfactors[EG] = 1 - T->phase / 24.0;
    		
			data.phase = T->phase;

		    data.safety[WHITE] = T->safety[WHITE];
		    data.safety[BLACK] = T->safety[BLACK];


			TArray type;		
			loadCoefficients(T, coeffs, type);

			std::vector<CoefficientsInfo> infoList;
			for (int i = 0; i < NTERMS; i++) {

				if (type[i] == NORMAL && coeffs[WHITE][i] - coeffs[BLACK][i] != 0.0) {

					CoefficientsInfo coefficientsInfo = {
						i, 
						coeffs[WHITE][i], 
						coeffs[BLACK][i],
						type[i]
					};

					infoList.push_back(coefficientsInfo);
				}

				if (type[i] != NORMAL && (coeffs[WHITE][i] != 0.0 || coeffs[BLACK][i] != 0.0)) {

					CoefficientsInfo coefficientsInfo = {
						i, 
						coeffs[WHITE][i], 
						coeffs[BLACK][i],
						type[i]
					};

					infoList.push_back(coefficientsInfo);	
				}
			}
			

			std::copy(infoList.begin(), infoList.end(), std::back_inserter(data.coefficientsInfoList));

			dataList.push_back(data);

			count++;

			if (count % 10000 == 0)	std::cout << count << " positions loaded \n";

			if (count >= NPOSITIONS)	break;
		}
	}
      
    newfile.close();

    assert(count == NPOSITIONS);

    // std::vector<int> weights;

    // for (int i = 0; i < initialGuess.size(); i++) {

    // 	weights.push_back(initialGuess[i]);
    // 	paramHistoryListraceCoefficients.push_back(true);
    // }

    // calculateParamHistoryList(historyParams);

	// bool parameterImproved = false;
	// scaling_constant = optimiseConstant(scaling_constant, 0, averageEvaluationError(weights),
	// 			500, 1, weights, true, parameterImproved);

	// if (parameterImproved) {

	// 	std::cout << "\n" << "New Scaling constant = " << scaling_constant << "\n";
	// }
	
	// localOptimize(initialGuess);
	optimise(params, cparams);
}




void writeEvalToFile() {

	std::fstream newfile;
   	newfile.open ("ccrl-40-15-elo-3000_7_5_m.epd", std::ios::in); 
		
	std::ofstream foutput; 
	foutput.open ("data.txt", std::ios::app); 

	int count = 1;

	// read data from file to a vector
	if (newfile.is_open() && foutput.is_open()) {   

		std::string tp;

		Thread th;	
	    th.moves_history_counter = 1;
		
		while (getline(newfile, tp)) {


			std::string fen = tp.substr(0, tp.find(";"));



			if (fen.length() <= 0) continue;

			int score = evaluateFENNNUE(&fen[0]);

			// u8 side = parseFen(const_cast<char *>(fen.c_str()), &th);

			// int score = evaluateNNUE(side, &th);

			foutput<<fen << " " << "{" << score << "}" << "\n"; 
			
			count++;

			if (count % 10000 == 0) std::cout << count << " eval for entries written" << std::endl;	
		}
	}

	newfile.close();
	foutput.close(); 

	std::cout << count - 1 << " eval for entries written" << std::endl;
}

void getEval() {

	std::fstream newfile;
   	newfile.open ("data.txt", std::ios::in); 
	
	if (newfile.is_open()) {   

		std::string tp;

		int count = 0;
		while (getline(newfile, tp)) {

			count++;
			std::string fen = tp.substr(0, tp.find("\""));
	

			unsigned first = fen.find('{') + 1;
			unsigned last = fen.find('}');
			std::string strNew = fen.substr (first, last-first);
			int stored_eval = std::stoi( strNew);

			std::cout<<stored_eval<<std::endl;

			if (count > 10) break;
		}
	}	

}




void optimise(TVector params, TVector cparams) {

	const double K = computeOptimalK();


	std::cout<<"Data list size -> " << dataList.size() << "\n";


	double bestMae = 0.0, mae = 0.0;

	TVector adagrad = {0}, cache = {0}, M = {0}, R = {0};

	// For Adam optimiser
	double beta1 = 0.9, beta2 = 0.999;
	double alpha1 = 0.001;
	double eps = 0.00000001;
	
	auto startTime = std::chrono::steady_clock::now();
	auto skipTime = std::chrono::steady_clock::now();
	auto lrStepTime = std::chrono::steady_clock::now();

	int counter = 0;
	int index = 0;

	bool skip = false;
	std::vector<Data> data_batch(BATCHSIZE);
	
	for (u64 epoch = 1; epoch < MAXEPOCHS; epoch++) {

		if (skip) { 

			skip = false;

			index += BATCHSIZE;
			continue;
		}


		if (index >= dataList.size()) {

			index = 0;
			std::random_shuffle(dataList.begin(), dataList.end());	
		}


    	std::copy(dataList.begin() + index, (dataList.begin() + index) + BATCHSIZE, data_batch.begin());
    	index += BATCHSIZE;


        TVector gradient = {0};

		computeGradient(gradient, params, data_batch, K);
		
		for (int i = 0; i < NTERMS; i++) {
			
			// Adagrad

            // adagrad[MG][i] += pow((K / 200.0) * gradient[MG][i] / BATCHSIZE, 2.0);
            // adagrad[EG][i] += pow((K / 200.0) * gradient[EG][i] / BATCHSIZE, 2.0);
            // params[MG][i] += (K / 200.0) * (gradient[MG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + adagrad[MG][i]));
            // params[EG][i] += (K / 200.0) * (gradient[EG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + adagrad[EG][i]));

			// RMSProp

            // cache[MG][i] = beta1 * cache[MG][i] + (1.0 - beta1) * pow((K / 200.0) * gradient[MG][i] / BATCHSIZE, 2.0);
            // cache[EG][i] = beta1 * cache[EG][i] + (1.0 - beta1) * pow((K / 200.0) * gradient[EG][i] / BATCHSIZE, 2.0);
            // params[MG][i] += (K / 200.0) * (gradient[MG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + cache[MG][i]));
            // params[EG][i] += (K / 200.0) * (gradient[EG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + cache[EG][i]));

			// Adam
		    M[MG][i] = beta1 * M[MG][i] + (1.0 - beta1) * (K / 200.0) * gradient[MG][i] / BATCHSIZE;
		    M[EG][i] = beta1 * M[EG][i] + (1.0 - beta1) * (K / 200.0) * gradient[EG][i] / BATCHSIZE;

        	R[MG][i] = beta2 * R[MG][i] + (1.0 - beta2) * pow((K / 200.0) * gradient[MG][i] / BATCHSIZE, 2.0);
        	R[EG][i] = beta2 * R[EG][i] + (1.0 - beta2) * pow((K / 200.0) * gradient[EG][i] / BATCHSIZE, 2.0);

        	double mg_m_k_hat = M[MG][i] / (1.0 - pow(beta1, epoch));
        	double eg_m_k_hat = M[EG][i] / (1.0 - pow(beta1, epoch));

        	double mg_r_k_hat = R[MG][i] / (1.0 - pow(beta2, epoch));
        	double eg_r_k_hat = R[EG][i] / (1.0 - pow(beta2, epoch));

        	params[MG][i] += alpha1 * mg_m_k_hat / (sqrt(mg_r_k_hat) + 1e-8);
        	params[EG][i] += alpha1 * eg_m_k_hat / (sqrt(eg_r_k_hat) + 1e-8);
		}

		auto endTime = std::chrono::steady_clock::now();

		if (std::chrono::duration_cast<std::chrono::seconds>(endTime - skipTime).count() > 20) {
			
			skipTime = std::chrono::steady_clock::now();
			skip = true;
		}

		if (std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() > DISPLAY_TIME) {

			startTime = std::chrono::steady_clock::now();

			// new weights gets loaded here	
			mae = tunedEvaluationErrors(params, K);

			// loadWeights(params, cparams);				
			// double mae = averageEvaluationError();
			
			std::cout << std::setprecision(10);
			std::cout << "Iteration -> " << epoch << ", ";
			std::cout << "MAE -> " << mae << ", ";
			std::cout << "LR -> " << alpha1 << std::endl;

			counter++;
			if (counter > 4) {

				counter = 0;

				displayWeights(params, cparams);

				std::async(writeToFile, params, cparams);
			} 
		}


		// if (std::chrono::duration_cast<std::chrono::seconds>(endTime - lrStepTime).count() > 60 * 5) {
			
		// 	lrStepTime = std::chrono::steady_clock::now();

		// 	if (alpha1 > 0.001)
		// 		alpha1 = alpha1 / LRDROPRATE;
		// }
	}
} 

double staticEvaluationErrors(double K) {

    double total = 0.0;

    #pragma omp parallel shared(total) 
    {
    	#pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
    	for (int i = 0; i < NPOSITIONS; i++)
            total += pow(dataList[i].result - sigmoid(K, dataList[i].sEval), 2);
  
    	// for (int i = 0; i < NPOSITIONS; i++)
     //        total += pow(dataList[i].result - dataList[i].sEval, 2);
    
    }

    return total / (double) NPOSITIONS;
}

double tunedEvaluationErrors(TVector weights, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
  	    #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
  	      for (int i = 0; i < NPOSITIONS; i++)
          	total += pow(dataList[i].result - sigmoid(K, linearEvaluation(weights, dataList[i], NULL)), 2);
  
        // for (int i = 0; i < NPOSITIONS; i++)
        //     total += pow(dataList[i].result - linearEvaluation(weights, dataList[i], NULL), 2);
    
    }

    return total / (double) NPOSITIONS;
}


double linearEvaluation(TVector weights, Data data, TGradientData *gradientData) {

    double midgame, endgame, wsafety[2], bsafety[2];
    double normal[2], safety[2];
    double mg[2][2] = {0}, eg[2][2] = {0};

	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;

    // Save any modifications for MG or EG for each evaluation type
	for (std::vector<CoefficientsInfo>::iterator i = list.begin(); i != list.end(); ++i) {
		
		CoefficientsInfo info = *i;

		mg[info.type][WHITE] += (double) info.wcoeff * weights[MG][info.index];
        mg[info.type][BLACK] += (double) info.bcoeff * weights[MG][info.index];
        eg[info.type][WHITE] += (double) info.wcoeff * weights[EG][info.index];
        eg[info.type][BLACK] += (double) info.bcoeff * weights[EG][info.index];		
	}

    // Grab the original "normal" evaluations and add the modified parameters
    normal[MG] = (double) ScoreMG(data.eval) + mg[NORMAL][WHITE] - mg[NORMAL][BLACK];
    normal[EG] = (double) ScoreEG(data.eval) + eg[NORMAL][WHITE] - eg[NORMAL][BLACK];

    // Grab the original "safety" evaluations and add the modified parameters
    wsafety[MG] = (double) ScoreMG(data.safety[WHITE]) + mg[SAFETY][WHITE];
    wsafety[EG] = (double) ScoreEG(data.safety[WHITE]) + eg[SAFETY][WHITE];
    bsafety[MG] = (double) ScoreMG(data.safety[BLACK]) + mg[SAFETY][BLACK];
    bsafety[EG] = (double) ScoreEG(data.safety[BLACK]) + eg[SAFETY][BLACK];

    // Remove the original "safety" evaluation that was double counted into the "normal" evaluation
    normal[MG] -= MIN(0, -ScoreMG(data.safety[WHITE]) * fabs(ScoreMG(data.safety[WHITE])) / 720.0)
                - MIN(0, -ScoreMG(data.safety[BLACK]) * fabs(ScoreMG(data.safety[BLACK])) / 720.0);
    normal[EG] -= MIN(0, -ScoreEG(data.safety[WHITE]) / 20.0) - MIN(0, -ScoreEG(data.safety[BLACK]) / 20.0);

    // Compute the new, true "safety" evaluations for each side
    safety[MG] = MIN(0, -wsafety[MG] * fabs(-wsafety[MG]) / 720.0)
               - MIN(0, -bsafety[MG] * fabs(-bsafety[MG]) / 720.0);
    safety[EG] = MIN(0, -wsafety[EG] / 20.0) - MIN(0, -bsafety[EG] / 20.0);


    // Save this information since we need it to compute the gradients
    if(gradientData != NULL) 
		*gradientData = (TGradientData) {
	        wsafety[MG], bsafety[MG], wsafety[EG], bsafety[EG]
	    };


    midgame = normal[MG] + safety[MG];
    endgame = normal[EG] + safety[EG];


    return (midgame * data.phase + endgame * (24.0 - data.phase)) / 24.0;
} 



void updateSingleGradient(Data data, TVector gradient, TVector weights, double K) {

	TGradientData gradientData;

    double E = linearEvaluation(weights, data, &gradientData);
    double S = sigmoid(K, E);
    double A = (data.result - S) * S * (1 - S);
	
	// double A = data.result - E;

    double mgBase = A * data.pfactors[MG];
    double egBase = A * data.pfactors[EG];

	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;

	for (auto &info : list) {

        if (info.type == NORMAL) {
    
            gradient[MG][info.index] += mgBase * (info.wcoeff - info.bcoeff);
		    gradient[EG][info.index] += egBase * (info.wcoeff - info.bcoeff);
        }

		if (info.type == SAFETY) {

	    	gradient[MG][info.index] += (mgBase / 360.0) 
	    		* (fmax(gradientData.bsafetymg, 0) * info.bcoeff - (fmax(gradientData.wsafetymg, 0) * info.wcoeff));

		    gradient[EG][info.index] += (egBase /  20.0) 
		    	* ((gradientData.bsafetyeg > 0.0) * info.bcoeff - (gradientData.wsafetyeg > 0.0) * info.wcoeff);
		}
	}
}











// void updateSingleGradient(Data data, TVector gradient, TVector weights) {

//     double E = linearEvaluation(weights, data);
//     double S = sigmoid(K, E);
//     double A = (data.result - S) * S * (1 - S);

//     double mgBase = A * data.pfactors[MG];
//     double egBase = A * data.pfactors[EG];

// 	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;

// 	for (int j = 0; j < lisT->size(); j++) {

// 		uint16_t index = list[j].index;
// 	    int8_t coeffsDiff = list[j].wcoeff - list[j].bcoeff;
	
// 		gradient[MG][index] += mgBase * coeffsDiff;
// 		gradient[EG][index] += egBase * coeffsDiff;
// 	}
// }





// void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch) {

//     #pragma omp parallel shared(gradient)
//     {
//         TVector local = {0};
//         #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS)
//         for (int i = 0; i < NPOSITIONS; i++)
//             updateSingleGradient(data_batch[i], local, weights);

//         for (int i = 0; i < NTERMS; i++) {
            
//             gradient[MG][i] += local[MG][i];
//             gradient[EG][i] += local[EG][i];
//         }
//     }
// }


// void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K, int batch) {

//     #pragma omp parallel shared(gradient)
//     {
//         TVector local = {0};

//        #pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)   
//         for (int i = batch * BATCHSIZE; i < (batch + 1) * BATCHSIZE; i++)
//         	updateSingleGradient(data_batch[i], local, weights, K);

//         for (int i = 0; i < NTERMS; i++) {

//             gradient[MG][i] += local[MG][i];
//             gradient[EG][i] += local[EG][i];
//         }
//     }
// }


void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K) {

	#pragma omp parallel shared(gradient) 
    {
        TVector local = {0};

		#pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)   
        for (int i = 0; i < data_batch.size(); i++) {
        	updateSingleGradient(data_batch[i], local, weights, K);
        }

        for (int i = 0; i < NTERMS; i++) {

            gradient[MG][i] += local[MG][i];
            gradient[EG][i] += local[EG][i];
        }
    }
}

double computeOptimalK() {

    double start = -10, end = 10, step = 1;
    double curr = start, error, best = staticEvaluationErrors(start);

    printf("\n\nComputing optimal K\n");

    for (int i = 0; i < KPRECISION; i++) {

        curr = start - step;
        while (curr < end) {
            curr = curr + step;
            error = staticEvaluationErrors(curr);
            if (error <= best)
                best = error, start = curr;
        }

        printf("Epoch [%d] K = [%.9f] E = [%.9f]\n", i, start, best);

        end   = start + step;
        start = start - step;
        step  = step  / 10.0;
    }

    printf("\n");

    return start;
}

void calculateParamHistoryList(std::vector<double> historyParams) {

	const int nParams = historyParams.size();

	//TODO load weights
	double bestE = averageEvaluationError();

	std::vector<double> bestParValues = historyParams;
   
	for (int pi = 0; pi < nParams; pi++) {

		paramHistoryList[pi] = true;
		
		std::vector<double> newParValues = bestParValues;
		newParValues[pi] += 1;
		
		//TODO load weights
		double newE = averageEvaluationError();
		
		if (newE < bestE) {

			bestE = newE;
			bestParValues = newParValues;

			paramHistoryList[pi] = true;

			std::cout <<"\n" << "Progress = "  << pi + 1 << "/" << nParams << "\n";
 
			// displayWeights(bestParValues);
		} else {

			newParValues[pi] -= 2;

			//TODO load weights
			newE = averageEvaluationError();
			if (newE < bestE) {

				bestE = newE;
				bestParValues = newParValues;

				paramHistoryList[pi] = false;
	
				std::cout <<"\n" << "Progress = "  << pi + 1 << "/" << nParams << "\n";
 
				// displayWeights(bestParValues);
			}
		}
	}
}

double averageEvaluationError() {

	// auto startTime = std::chrono::steady_clock::now();

	double error = 0.0;
	u64 positionsChecked = 0;
	
	// for (int i = 0; i < num_positions; i++) {

	// 	Thread tunerThread;
	// 	// Bad access error fix
	//     tunerThread.movesHistoryCounter = 1;

	// 	u8 side = parseFen(const_cast<char *>(dataList[i].fen.c_str()), &tunerThread);

	// 	// int eval = side ? -quiescense(0, BLACK, -VAL_INFINITY, VAL_INFINITY, &tunerThread) 
	// 	// 	: quiescense(0, WHITE, -VAL_INFINITY, VAL_INFINITY, &tunerThread);

	// 	double eval = fullEval(WHITE, &tunerThread);

	// 	error += pow(dataList[i].result - sigmoid(K, eval), 2);

	// 	positionsChecked++;
	// }

	double mae = error / (double) positionsChecked;

	// auto endTime = std::chrono::steady_clock::now();

	// std::cout << "Positions -> " << positionsChecked << ", MAE -> " << std::setprecision(10) << mae  
	// 	<< ", Time -> " << std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() << "s" << "\n";

	return mae;
}

int optimiseConstant(int constant, int pos, double current_mea, int max_iter, int step, 
	std::vector<double> weightList, bool isOptimisingScalingConstant, bool &improved) {

	improved = false;

    int original = constant;

    double adjusted_mea;
    constant = original + step;

    if (!isOptimisingScalingConstant) weightList[pos] = constant;

    //TODO load new weights
    //loadWeights(weightList);

    if ((adjusted_mea = averageEvaluationError()) < current_mea) {

        std::cout << "optimising constant (increasing): " << constant << std::endl;

        while (adjusted_mea < current_mea && abs(constant - original) <= max_iter) {
            
            current_mea = adjusted_mea;
            constant += step;
            
            if (!isOptimisingScalingConstant) weightList[pos] = constant;

            adjusted_mea = averageEvaluationError();

            std::cout << " : constant: " << constant << " -> " << adjusted_mea << std::endl;
        }

        constant -= step;

        improved = true;
    } else {

        std::cout << "optimising constant (decreasing): " << constant << std::endl;

        constant = original - step;
        
        if (!isOptimisingScalingConstant) weightList[pos] = constant;

        adjusted_mea = averageEvaluationError();
        while (adjusted_mea < current_mea && abs(constant - original) <= max_iter) {
        
            current_mea = adjusted_mea;
            constant -= step;

		    if (!isOptimisingScalingConstant) weightList[pos] = constant;

            adjusted_mea = averageEvaluationError();

            std::cout << " : constant: " << constant << " -> " << adjusted_mea << std::endl;

            improved = true;
        }

        constant += step;
    }

    if (improved) {

    	std::cout << "constant optimised: " << constant << " -> " << current_mea << std::endl;
    }

    return constant;
}

std::vector<double> localOptimize(std::vector<double>& initialGuess) {

	int cycle_count = 0;

	const int nParams = initialGuess.size();

	// double bestE = averageEvaluationError(initialGuess);

	std::vector<double> bestParValues = initialGuess;

	bool improved = true;
   
	while ( improved ) {

		cycle_count++;

		improved = false;

		for (int pi = 0; pi < nParams; pi++) {

			std::cout <<"\n" << "Cycle count = " << cycle_count << ", Current progress = "  << pi + 1 << "/" << nParams << "\n";		

  			std::vector<double> newParValues = bestParValues;
			
			bool parameterImproved1 = false;
			
			//TODO load weights before calling averageEvaluationError()

			int constant = optimiseConstant(newParValues[pi], pi, averageEvaluationError(),
				10, 5, newParValues, false, parameterImproved1);
 			
 			if (parameterImproved1) {

				newParValues[pi] = constant;

           		bestParValues = newParValues;

				std::cout << "\n" << "First pass parameter improved" << "\n";			
			}

			bool parameterImproved2 = false;
			
			constant = optimiseConstant(newParValues[pi], pi, averageEvaluationError(),
					5, 1, newParValues, false, parameterImproved2);
			
			if (parameterImproved2) {

				newParValues[pi] = constant;

           		bestParValues = newParValues;

				std::cout << "\n" << "Second pass parameter improved" << "\n";			
			}

			if (parameterImproved1 || parameterImproved2) {

				// displayWeights(bestParValues);

				improved = true;
			}

			// if (paramHistoryList[pi]) {

			// 	paramHistoryList[pi] = true;

			// 	newParValues[pi] += 1;
			// 	double newE = averageEvaluationError(newParValues);
			// 	if (newE < bestE) {

			// 		bestE = newE;
			// 		bestParValues = newParValues;
			// 		improved = true;

			// 		paramHistoryList[pi] = true;
		
			// 		displayWeights();
					
			// 		std::cout <<"\n" << "Cycle count = " << cycle_count << ", Current progress = "  << pi + 1 << "/" << nParams << "\n";
			// 	} else {

			// 		newParValues[pi] -= 2;
			// 		newE = averageEvaluationError(newParValues);
			// 		if (newE < bestE) {

			// 			bestE = newE;
			// 			bestParValues = newParValues;
			// 			improved = true;
		
			// 			paramHistoryList[pi] = false;

			// 			displayWeights();
			
			// 			std::cout <<"\n" << "Cycle count = " << cycle_count << ", Current progress = "  << pi + 1 << "/" << nParams << "\n";
			// 		}
			// 	}
			// } else {

			// 	paramHistoryList[pi] = false;

			// 	newParValues[pi] -= 1;
			// 	double newE = averageEvaluationError(newParValues);
			// 	if (newE < bestE) {

			// 		bestE = newE;
			// 		bestParValues = newParValues;
			// 		improved = true;

			// 		paramHistoryList[pi] = false;
		 
			// 		displayWeights();

			// 		std::cout <<"\n" << "Cycle count = " << cycle_count << ", Current progress = "  << pi + 1 << "/" << nParams << "\n";
			// 	} else {

			// 		newParValues[pi] += 2;
			// 		newE = averageEvaluationError(newParValues);
			// 		if (newE < bestE) {

			// 			bestE = newE;
			// 			bestParValues = newParValues;
			// 			improved = true;
		
			// 			paramHistoryList[pi] = true;
 
			// 			displayWeights();

			// 			std::cout <<"\n" << "Cycle count = " << cycle_count << ", Current progress = "  << pi + 1 << "/" << nParams << "\n";
			// 		}
			// 	}
			// }
		}
	}

   return bestParValues;
}

void displayWeights(TVector params, TVector cparams) {

	TVector weights = {0};

    // Combine updated and current parameters
    for (int j = 0; j < NTERMS; j++) {
        weights[MG][j] = round(params[MG][j] + cparams[MG][j]);
        weights[EG][j] = round(params[EG][j] + cparams[EG][j]);
    }

	int count = 0;

	std::cout << "\n";

	std::cout << "Weights" << "\n\n";

	std::cout 
		<< "weight_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_knight = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_rook = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")"
		<< ",\nweight_queen = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")";
	
	std::cout<<", \n\n";


	std::cout 
		<< "\nweight_isolated_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_backward_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_double_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_defended_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")"
		<< ",\nweight_pawn_hole = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")";

	std::cout <<", \n" << "arr_weight_passed_pawn[8] = { "; 
    for(int i = 0; i < 8; i++) std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", "; 
	
	std::cout << "}, \n" << "arr_weight_defended_passed_pawn[8] = { "; 
    for(int i = 0; i < 8; i++) std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";

   	std::cout << "}, \n\n";



   	std::cout << "weight_undefended_knight = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
	std::cout << "weight_knight_defended_by_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
	std::cout<<"\n\n";


 	std::cout << "weight_undefended_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
 	std::cout << "weight_bad_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
   	std::cout << "weight_bishop_pair = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
	std::cout<<"\n\n";


 	std::cout << "weight_rook_half_open_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_open_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_enemy_queen_same_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		// "\nweight_rook_blocked_by_king = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_on_seventh_rank = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_rook_on_eight_rank = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_supporting_friendly_rook = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";

  
   	std::cout << "weight_queen_underdeveloped_pieces = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";


	std::cout << "weight_king_pawn_shield = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";   	
	std::cout << "weight_king_enemy_pawn_storm = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";   	

	std::cout << "arr_weight_knight_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	}
   	std::cout << "\n"; 
	std::cout <<"}, "<<"\n";

	std::cout << "arr_weight_bishop_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	}
	std::cout << "\n"; 
	std::cout <<"}, "<<"\n";

	std::cout << "arr_weight_rook_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	}
	std::cout << "\n"; 
	std::cout <<"}, "<<"\n";

	std::cout << "arr_weight_queen_mobility[32] = { \n\n";
    for (int i = 0; i < 32; i++) {
    
    	std::cout << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	}
	std::cout << "\n"; 
	std::cout <<"}, "<<"\n";


	// PSQT weights

	std::cout <<"\n"<< "int pawnPSQT[64] = {" <<"\n\n";
	
	for (int i = 0; i < 8; i++) { 
	
		std::cout << "S(" << std::setw(4) << 0<<"," << 
			std::setw(4) <<0<<")" << ", "; 
	} std::cout << "\n";

	for (int i = 0; i < 48; i++) { 
	
		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	}

	for (int i = 0; i < 8; i++) { 
	
		std::cout << "S(" << std::setw(4) << 0<<"," << 
			std::setw(4) <<0<<")" << ", "; 
	} std::cout << "\n";
	std::cout <<"};"<<"\n";
	

	std::cout <<"\n"<< "int knightPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	} std::cout << "\n"; 
	std::cout <<"};"<<"\n";


	std::cout <<"\n"<< "int bishopPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	} std::cout << "\n";
	std::cout <<"};"<<"\n"; 


	std::cout <<"\n"<< "int rookPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	} std::cout << "\n"; 
	std::cout <<"};"<<"\n"; 


	std::cout <<"\n"<< "int queenPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	} std::cout << "\n"; 	
	std::cout <<"};"<<"\n"; 


	std::cout <<"\n"<< "int kingPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		std::cout << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) std::cout<<"\n";
	} std::cout << "\n";
	std::cout <<"};"<<"\n\n"; 


	std::cout << "weight_center_control = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";
   

 	std::cout << "weight_knight_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_bishop_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_queen_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_safe_contact_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_queen_safe_contact_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_knight_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_bishop_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_queen_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
  		"\nweight_safety_adjustment = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";

	assert (count == NTERMS); 
}

void writeToFile(TVector params, TVector cparams) {
	
	TVector weights = {0};

    // Combine updated and current parameters
    for (int j = 0; j < NTERMS; j++) {
        weights[MG][j] = round(params[MG][j] + cparams[MG][j]);
        weights[EG][j] = round(params[EG][j] + cparams[EG][j]);
    }

	std::ofstream myfile;
	myfile.open ("scratch.cpp");
		
	int count = 0;

	myfile << "\n";

	myfile << "Weights" << "\n\n";

	myfile 
		<< "weight_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_knight = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_rook = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")"
		<< ",\nweight_queen = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")";
	
	myfile<<", \n";

	myfile 
		<< "\nweight_isolated_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_backward_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_double_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" 
		<< ",\nweight_defended_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")"
		<< ",\nweight_pawn_hole = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")";

	myfile <<", \n" << "arr_weight_passed_pawn[8] = { "; 
    for(int i = 0; i < 8; i++) 
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", "; 
	
	myfile << "}, \n" << "arr_weight_defended_passed_pawn[8] = { "; 
    for(int i = 0; i < 8; i++) 
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";

   	myfile << "}, \n\n";
	


	myfile << "weight_undefended_knight = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
	myfile << "weight_knight_defended_by_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
	myfile << "\n\n";


 	myfile << "weight_undefended_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
    myfile << "weight_bad_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";
   	myfile << "weight_bishop_pair = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
	myfile << "\n\n";


 	myfile << "weight_rook_half_open_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_open_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_enemy_queen_same_file = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		// "\nweight_rook_blocked_by_king = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_on_seventh_rank = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_rook_on_eight_rank = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_supporting_friendly_rook = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";

  
   	myfile << "weight_queen_underdeveloped_pieces = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";


	myfile << "weight_king_pawn_shield = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n";   	
	myfile << "weight_king_enemy_pawn_storm = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";


	myfile << "arr_weight_knight_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) myfile<<"\n";
	}
   	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_bishop_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_rook_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_queen_mobility[32] = { \n\n";
    for (int i = 0; i < 32; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", ";
		
		if(((i + 1) % 8) == 0) myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, "<<"\n";


	// PSQT weights

	myfile <<"\n"<< "int pawnPSQT[64] = {" <<"\n\n";
	
	for (int i = 0; i < 8; i++) { 
	
		myfile << "S(" << std::setw(4) << 0<<"," << 
			std::setw(4) <<0<<")" << ", "; 
	} myfile << "\n";

	for (int i = 0; i < 48; i++) { 
	
		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	}

	for (int i = 0; i < 8; i++) { 
	
		myfile << "S(" << std::setw(4) << 0<<"," << 
			std::setw(4) <<0<<")" << ", "; 
	} myfile << "\n";
	myfile <<"};"<<"\n";
	

	myfile <<"\n"<< "int knightPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	} myfile << "\n"; 
	myfile <<"};"<<"\n";


	myfile <<"\n"<< "int bishopPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	} myfile << "\n";
	myfile <<"};"<<"\n"; 


	myfile <<"\n"<< "int rookPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	} myfile << "\n"; 
	myfile <<"};"<<"\n"; 


	myfile <<"\n"<< "int queenPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	} myfile << "\n"; 	
	myfile <<"};"<<"\n"; 


	myfile <<"\n"<< "int kingPSQT[64] = {" <<"\n\n";
	for (int i = 0; i < 64; i++) { 

		myfile << "S(" << std::setw(4) << (int)weights[MG][count]<<"," << 
			std::setw(4) <<(int)weights[EG][count++]<<")" << ", "; 
			
		if(((i + 1) % 8) == 0) myfile<<"\n";
	} myfile << "\n";
	myfile <<"};"<<"\n\n";


	myfile << "weight_center_control = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";
   

 	myfile << "weight_knight_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_bishop_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_queen_attack = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_safe_contact_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_queen_safe_contact_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
		"\nweight_knight_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_bishop_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_rook_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
   		"\nweight_queen_check = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", " <<
  		"\nweight_safety_adjustment = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count++]<<")" << ", \n\n";


	myfile.close();

	assert(count == NTERMS);
}
