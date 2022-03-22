
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
#include "search.h"
#include "evaluate.h"
#include "make_unmake.h"
#include "utility.h"
#include "fen.h"
#include "functions.h"
#include "weights.h"

#define MAXEPOCHS		1000000000
#define NPARTITIONS		4 
#define BATCHSIZE		16 
#define NPOSITIONS		7150000 
#define DISPLAY_TIME	60				
#define LRDROPRATE		1

struct Score 
{	
	double mg;
	double eg;
};

class LoadCoeff 
{
public:
	int8_t type[NTERMS];
	int8_t coeffs[2][NTERMS];
};	

int seePieceVal[8] = { 0, 100, 300, 300, 500, 900, 2000, 0 };

std::vector<Data> dataList;

double sigmoid(double K, double E) 
{
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

void loadCoefficients(TraceCoefficients *T, LoadCoeff *loadCoeff) 
{
	int i = 0;

	// Material

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->weight_val_pawn[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->weight_val_pawn[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->weight_val_knight[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->weight_val_knight[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->weight_val_bishop[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->weight_val_bishop[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->weight_val_rook[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->weight_val_rook[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->weight_val_queen[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->weight_val_queen[BLACK];   

                        

    // Pawns
	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->pawnIsland[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->pawnIsland[BLACK]; 

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->isolatedPawns[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->isolatedPawns[BLACK];  

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->backwardPawns[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->backwardPawns[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->doublePawns[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->doublePawns[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->pawnHoles[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->pawnHoles[BLACK];                         
    
    for (int k = 0; k < 8; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->pawnChain[k][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->pawnChain[k][BLACK];                         
	}

	for (int k = 0; k < 8; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->phalanxPawn[k][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->phalanxPawn[k][BLACK];                         
	}

	for (int k = 0; k < 8; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->defendedPhalanxPawn[k][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->defendedPhalanxPawn[k][BLACK];                         
	}
    
    for (int k = 0; k < 8; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->passedPawn[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->passedPawn[BLACK][k];                         
	}
    
    for (int k = 0; k < 8; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->defendedPassedPawn[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->defendedPassedPawn[BLACK][k];                         
	}


	// Knights

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->knightAllPawnsCount[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->knightAllPawnsCount[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->knightOutpost[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->knightOutpost[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->undefendedKnight[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->undefendedKnight[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->knightDefendedByPawn[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->knightDefendedByPawn[BLACK];                         


	// Bishops

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->bishopPair[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->bishopPair[BLACK];                       

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->undefendedBishop[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->undefendedBishop[BLACK];                         
                             

    // Rooks                     

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookBehindStmPassedPawn[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookBehindStmPassedPawn[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookBehindOppPassedPawn[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookBehindOppPassedPawn[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookFlankOutpost[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookFlankOutpost[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->halfOpenFile[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->halfOpenFile[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->openFile[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->openFile[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookEnemyQueenSameFile[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookEnemyQueenSameFile[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookSupportingFriendlyRook[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookSupportingFriendlyRook[BLACK];                         

	loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookOnSeventhRank[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookOnSeventhRank[BLACK];                         
    
    loadCoeff->type[i] = NORMAL;
	loadCoeff->coeffs[WHITE][i] = T->rookOnEightRank[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookOnEightRank[BLACK];                         
    

    // Queen

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->queenUnderdevelopedPieces[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->queenUnderdevelopedPieces[BLACK];                         
  	
  	// King

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->kingPawnShield[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->kingPawnShield[BLACK];                         

	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->kingEnemyPawnStorm[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->kingEnemyPawnStorm[BLACK]; 


    // Pieces Mobility
    for (int k = 0; k < 16; k++) {
	
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->knightMobility[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->knightMobility[BLACK][k];                         
	}

    for (int k = 0; k < 16; k++) {
		
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->bishopMobility[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->bishopMobility[BLACK][k];                         
	}

    for (int k = 0; k < 16; k++) {
		
		loadCoeff->type[i] = NORMAL;
	    loadCoeff->coeffs[WHITE][i] = T->rookMobility[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->rookMobility[BLACK][k];                         
	}
	
    for (int k = 0; k < 32; k++) {
	
		loadCoeff->type[i] = NORMAL;	
	    loadCoeff->coeffs[WHITE][i] = T->queenMobility[WHITE][k];                         
	    loadCoeff->coeffs[BLACK][i++] = T->queenMobility[BLACK][k];                         
	}


    loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->centerControl[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->centerControl[BLACK];                         


	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->knightAttack[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->knightAttack[BLACK];                         

	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->bishopAttack[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->bishopAttack[BLACK];                         

	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->rookAttack[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookAttack[BLACK];                         

	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->queenAttack[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->queenAttack[BLACK];                         


	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->rookSafeContactCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookSafeContactCheck[BLACK];                         

	loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->queenSafeContactCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->queenSafeContactCheck[BLACK];                         


    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->knightCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->knightCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->bishopCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->bishopCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->rookCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->rookCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->queenCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->queenCheck[BLACK];                         


    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->safetyAdjustment[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safetyAdjustment[BLACK];                         


    //PSQT

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->kingPSQT[kingSq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->kingPSQT[kingSq][BLACK];                         
    }

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			loadCoeff->type[i] = NORMAL;
		    
		    loadCoeff->coeffs[WHITE][i] = T->pawnPSQT[kingSq][pieceSq][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->pawnPSQT[kingSq][pieceSq][BLACK];                         
    	}
    }

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			loadCoeff->type[i] = NORMAL;
		    loadCoeff->coeffs[WHITE][i] = T->knightPSQT[kingSq][pieceSq][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->knightPSQT[kingSq][pieceSq][BLACK];                         
    	}
    }

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			loadCoeff->type[i] = NORMAL;
		    loadCoeff->coeffs[WHITE][i] = T->bishopPSQT[kingSq][pieceSq][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->bishopPSQT[kingSq][pieceSq][BLACK];                         
    	}
    }

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			loadCoeff->type[i] = NORMAL;
		    loadCoeff->coeffs[WHITE][i] = T->rookPSQT[kingSq][pieceSq][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->rookPSQT[kingSq][pieceSq][BLACK];                         
    	}
    }

    for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			loadCoeff->type[i] = NORMAL;
		    loadCoeff->coeffs[WHITE][i] = T->queenPSQT[kingSq][pieceSq][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->queenPSQT[kingSq][pieceSq][BLACK];                         
    	}
    }


    assert(i == NTERMS);
}

void startTuner() {

	int count = 0;

	TVector params = {}, cparams = {};


	cparams[MG][count] = ScoreMG(weight_val_pawn);
	cparams[EG][count++] = ScoreEG(weight_val_pawn);
	
	cparams[MG][count] = ScoreMG(weight_val_knight);
	cparams[EG][count++] = ScoreEG(weight_val_knight);

	cparams[MG][count] = ScoreMG(weight_val_bishop);
	cparams[EG][count++] = ScoreEG(weight_val_bishop);
	
	cparams[MG][count] = ScoreMG(weight_val_rook);
	cparams[EG][count++] = ScoreEG(weight_val_rook);

	cparams[MG][count] = ScoreMG(weight_val_queen);
	cparams[EG][count++] = ScoreEG(weight_val_queen);


	cparams[MG][count] = ScoreMG(weight_pawn_island);
	cparams[EG][count++] = ScoreEG(weight_pawn_island);

	cparams[MG][count] = ScoreMG(weight_isolated_pawn);
	cparams[EG][count++] = ScoreEG(weight_isolated_pawn);
	
	cparams[MG][count] = ScoreMG(weight_backward_pawn);
	cparams[EG][count++] = ScoreEG(weight_backward_pawn);

	cparams[MG][count] = ScoreMG(weight_double_pawn);
	cparams[EG][count++] = ScoreEG(weight_double_pawn);

	cparams[MG][count] = ScoreMG(weight_pawn_hole);
	cparams[EG][count++] = ScoreEG(weight_pawn_hole);

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_pawn_chain[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_pawn_chain[i]);
	}

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_phalanx_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_phalanx_pawn[i]);
	}

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_defended_phalanx_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_defended_phalanx_pawn[i]);
	}

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_passed_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_passed_pawn[i]);
	}

	for (int i = 0; i < 8; i++) {
 		
		cparams[MG][count] = ScoreMG(arr_weight_defended_passed_pawn[i]);
		cparams[EG][count++] = ScoreEG(arr_weight_defended_passed_pawn[i]);
	}



	cparams[MG][count] = ScoreMG(weight_knight_all_pawns_count);
	cparams[EG][count++] = ScoreEG(weight_knight_all_pawns_count);

	cparams[MG][count] = ScoreMG(weight_knight_outpost);
	cparams[EG][count++] = ScoreEG(weight_knight_outpost);

	cparams[MG][count] = ScoreMG(weight_undefended_knight);
	cparams[EG][count++] = ScoreEG(weight_undefended_knight);
	
	cparams[MG][count] = ScoreMG(weight_knight_defended_by_pawn);
	cparams[EG][count++] = ScoreEG(weight_knight_defended_by_pawn);



	cparams[MG][count] = ScoreMG(weight_bishop_pair);
	cparams[EG][count++] = ScoreEG(weight_bishop_pair);

	cparams[MG][count] = ScoreMG(weight_undefended_bishop);
	cparams[EG][count++] = ScoreEG(weight_undefended_bishop);


	cparams[MG][count] = ScoreMG(weight_rook_behind_stm_passed_pawn);
	cparams[EG][count++] = ScoreEG(weight_rook_behind_stm_passed_pawn);

	cparams[MG][count] = ScoreMG(weight_rook_behind_opp_passed_pawn);
	cparams[EG][count++] = ScoreEG(weight_rook_behind_opp_passed_pawn);

	cparams[MG][count] = ScoreMG(weight_rook_flank_outpost);
	cparams[EG][count++] = ScoreEG(weight_rook_flank_outpost);

	cparams[MG][count] = ScoreMG(weight_rook_half_open_file);
	cparams[EG][count++] = ScoreEG(weight_rook_half_open_file);

	cparams[MG][count] = ScoreMG(weight_rook_open_file);
	cparams[EG][count++] = ScoreEG(weight_rook_open_file);

	cparams[MG][count] = ScoreMG(weight_rook_enemy_queen_same_file);
	cparams[EG][count++] = ScoreEG(weight_rook_enemy_queen_same_file);

	cparams[MG][count] = ScoreMG(weight_rook_supporting_friendly_rook);
	cparams[EG][count++] = ScoreEG(weight_rook_supporting_friendly_rook);

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

	
	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
	
		cparams[MG][count] = ScoreMG(kingPSQT[kingSq]);
		cparams[EG][count++] = ScoreEG(kingPSQT[kingSq]);
	}

	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			cparams[MG][count] = ScoreMG(pawnPSQT[kingSq][pieceSq]);
			cparams[EG][count++] = ScoreEG(pawnPSQT[kingSq][pieceSq]);
		}
	}

	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			cparams[MG][count] = ScoreMG(knightPSQT[kingSq][pieceSq]);
			cparams[EG][count++] = ScoreEG(knightPSQT[kingSq][pieceSq]);
		}
	}

	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			cparams[MG][count] = ScoreMG(bishopPSQT[kingSq][pieceSq]);
			cparams[EG][count++] = ScoreEG(bishopPSQT[kingSq][pieceSq]);
		}
	}

	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			cparams[MG][count] = ScoreMG(rookPSQT[kingSq][pieceSq]);
			cparams[EG][count++] = ScoreEG(rookPSQT[kingSq][pieceSq]);
		}
	}

	for (int kingSq = 0; kingSq < U8_MAX_SQUARES; kingSq++) {
		for (int pieceSq = 0; pieceSq < U8_MAX_SQUARES; pieceSq++) {

			cparams[MG][count] = ScoreMG(queenPSQT[kingSq][pieceSq]);
			cparams[EG][count++] = ScoreEG(queenPSQT[kingSq][pieceSq]);
		}
	}


	assert(count == NTERMS);

	std::fstream newfile;
   	newfile.open ("lichess-big3-resolved.book", std::ios::in);	// NPOSITIONS = 7150000 roughly
	// newfile.open ("quiet-labeled.epd", std::ios::in); 			// NPOSITIONS = 725000 exact
	
	count = 0;

	TraceCoefficients *T = new TraceCoefficients();

	// read data from file to a vector
	if (newfile.is_open()) {   
		
		Side side;
		std::string tp;
		double result;
		std::string fen;
		
		while (getline(newfile, tp)) {

			
			fen = tp.substr(0, tp.find("\""));
			
			// std::string fen = tp.substr(0, tp.find(";"));

			if (fen.length() <= 0) 
				continue;


			// unsigned first = fen.find('{') + 1;
			// unsigned last = fen.find('}');
			// std::string strNew = fen.substr (first, last-first);
			
			// int stored_eval = std::stoi(strNew);


			// for file "quiet-labeled.epd"

		/*	if (tp.find("1-0") != std::string::npos)	
				result = 1.0;
			else if (tp.find("1/2-1/2") != std::string::npos) 
				result = 0.5;
			else if (tp.find("0-1") != std::string::npos)	
				result = 0.0;
			else 
				continue;
*/

			// for file "4818922_positions_gm2600.txt"

			if (tp.find("1.0") != std::string::npos)	
				result = 1.0;
			else if (tp.find("0.5") != std::string::npos) 
				result = 0.5;
			else if (tp.find("0.0") != std::string::npos)	
				result = 0.0;
			else 
				continue;

			
			assert(result == 1.0 || result == 0.5 || result == 0.0);


			T->clear();
			
			LoadCoeff *loadCoeff = new LoadCoeff();

			Thread *th = new Thread();
			th->moves_history_counter = 1;
		
			Data data = {};	
			data.result = result;

	
			side = parseFen(fen, th);
			
			data.sEval = traceFullEval(side, T, th);
				
			delete th; // free resources after use

			if (side == BLACK) {

				data.sEval = -data.sEval;
			}
					
			data.eval = T->eval;

			data.pfactors[MG] = 0 + T->phase / 24.0;
		    data.pfactors[EG] = 1 - T->phase / 24.0;
    		
			data.phase = T->phase;

		    data.safety[WHITE] = T->safety[WHITE];
		    data.safety[BLACK] = T->safety[BLACK];

			loadCoefficients(T, loadCoeff);

			std::vector<CoefficientsInfo> infoList;
			for (uint16_t i = 0; i < NTERMS; i++) {

				// std::cout<<coeffs[WHITE][i]<<","<<coeffs[BLACK][i]<<"\n";

				if (	loadCoeff->type[i] == NORMAL 
					&&	loadCoeff->coeffs[WHITE][i] - loadCoeff->coeffs[BLACK][i] != 0.0) {

					CoefficientsInfo coefficientsInfo = {
						i, 
						loadCoeff->coeffs[WHITE][i], 
						loadCoeff->coeffs[BLACK][i],
						loadCoeff->type[i]
					};

					infoList.push_back(coefficientsInfo);
				}

				if (	loadCoeff->type[i] != NORMAL 
					&&	(loadCoeff->coeffs[WHITE][i] != 0.0 || loadCoeff->coeffs[BLACK][i] != 0.0)) {

					CoefficientsInfo coefficientsInfo = {
						i, 
						loadCoeff->coeffs[WHITE][i], 
						loadCoeff->coeffs[BLACK][i],
						loadCoeff->type[i]
					};

					infoList.push_back(coefficientsInfo);	
				}
			}
			
			delete loadCoeff; // free resources after use


			std::copy(infoList.begin(), infoList.end(), std::back_inserter(data.coefficientsInfoList));

			dataList.push_back(data);

			count++;

			if (count % 10000 == 0)	{

				std::cout << (count * 100) / NPOSITIONS << "% loaded\n";
			}

			if (count >= NPOSITIONS) {

				break;			
			}	
		}
	}
      
    newfile.close();

    assert(count == NPOSITIONS);

	std::cout<<"Data list size -> " << dataList.size() << "\n";

	optimise(params, cparams);
}


void optimise(TVector params, TVector cparams) {

	const double K = computeOptimalK();

	double bestMae = 100, mae = 0, prevMae = 100;

// 	TVector adagrad = {}, cache = {};
	TVector M = {}, R = {};

	// For Adam optimiser
	double beta1 = 0.9, beta2 = 0.999;
	double alpha1 = 0.001;
	
	auto tunerStartTime = std::chrono::steady_clock::now();
	
	int counter = 0;
	uint64_t index = 0;

	std::vector<Data> data_batch;
	
	for (uint64_t epoch = 1; epoch < MAXEPOCHS; epoch++) {

		if (index >= dataList.size()) {

			index = 0;
			std::random_shuffle(dataList.begin(), dataList.end());	
		}

		data_batch.clear();
    	std::copy(dataList.begin() + index, (dataList.begin() + index) + BATCHSIZE, back_inserter(data_batch));
    	
    	index += BATCHSIZE;


        TVector gradient = {};

		computeGradient(gradient, params, data_batch, K);


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

		#pragma omp parallel
		{
			// Parallel region with default number of threads
			#pragma omp for schedule(auto)
			for (int i = 0; i < NTERMS; ++i) {
				
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
		} // End of the Parallel region 
		
		
		auto tunerTimeNow = std::chrono::steady_clock::now();

		if (std::chrono::duration_cast<std::chrono::seconds>(tunerTimeNow - tunerStartTime).count() > DISPLAY_TIME) {

			tunerStartTime = std::chrono::steady_clock::now();

			mae = tunedEvaluationErrors(params, K);

			std::cout << "Epoch = [" << epoch * BATCHSIZE / NPOSITIONS << "], ";
			std::cout << "Error = [" << std::fixed << std::setprecision(8) << mae << "], ";
			std::cout << "Rate = [" << std::fixed << std::setprecision(5) << alpha1 << "], ";
			
			if (prevMae > mae)
				std::cout << "Error loss = [" << std::fixed << std::setprecision(8) << prevMae - mae << "]" << std::endl;
			else
				std::cout << "Error gain = [" << std::fixed << std::setprecision(8) << mae - prevMae << "]" << std::endl;

			counter++;
			if (counter > 4) {

				counter = 0;

				if (mae < bestMae) {

					bestMae = mae;

					std::async(saveWeights, params, cparams);			
				}
			} 

			prevMae = mae;
		}
	}
} 

double staticEvaluationErrors(double K) {

    double total = 0.0;

    #pragma omp parallel shared(total) 
    {
    	#pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
    	for (int i = 0; i < NPOSITIONS; i++)
            total += pow(dataList[i].result - sigmoid(K, dataList[i].sEval), 2);
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
    }

    return total / (double) NPOSITIONS;
}


double linearEvaluation(TVector weights, Data data, TGradientData *gradientData) {

    double midgame, endgame, wsafety[2], bsafety[2];
    double normal[2], safety[2];
    double mg[2][2] = {}, eg[2][2] = {};

    // Save any modifications for MG or EG for each evaluation type
	for (auto info : data.coefficientsInfoList) {
		
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
    normal[MG] -= MIN(0, -ScoreMG(data.safety[WHITE]) * std::abs(ScoreMG(data.safety[WHITE])) / 720.0)
                - MIN(0, -ScoreMG(data.safety[BLACK]) * std::abs(ScoreMG(data.safety[BLACK])) / 720.0);
    normal[EG] -= MIN(0, -ScoreEG(data.safety[WHITE]) / 20.0) - MIN(0, -ScoreEG(data.safety[BLACK]) / 20.0);

    // Compute the new, true "safety" evaluations for each side
    safety[MG] = MIN(0, -wsafety[MG] * fabs(-wsafety[MG]) / 720.0)
               - MIN(0, -bsafety[MG] * fabs(-bsafety[MG]) / 720.0);
    safety[EG] = MIN(0, -wsafety[EG] / 20.0) - MIN(0, -bsafety[EG] / 20.0);


    // Save this information since we need it to compute the gradients
    if (gradientData != NULL) {
    	
    	TGradientData tGradientData;
    	
    	tGradientData.wsafetymg = wsafety[MG]; 
    	tGradientData.bsafetymg = bsafety[MG];
    	tGradientData.wsafetyeg = wsafety[EG];
    	tGradientData.bsafetyeg = bsafety[EG];

    	gradientData = &tGradientData;
    }


    midgame = normal[MG] + safety[MG];
    endgame = normal[EG] + safety[EG];


    return (midgame * data.phase + endgame * (24.0 - data.phase)) / 24.0;
} 

void updateSingleGradient(Data data, TVector gradient, TVector weights, double K) {

	TGradientData gradientData;

    double E = linearEvaluation(weights, data, &gradientData);
    double S = sigmoid(K, E);
    double A = (data.result - S) * S * (1 - S);
	
    double mgBase = A * data.pfactors[MG];
    double egBase = A * data.pfactors[EG];

	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;

	for (auto info : list) {

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


void computeGradient(TVector gradient, TVector weights, std::vector<Data> data_batch,  double K) {

	#pragma omp parallel shared(gradient) 
    {
        TVector local = {};

		#pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)   
        for (uint32_t i = 0; i < data_batch.size(); i++) 
        	updateSingleGradient(data_batch[i], local, weights, K);
        
        for (uint32_t i = 0; i < NTERMS; i++) {

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

void saveWeights(TVector params, TVector cparams) {
	
	TVector weights = {};

    // Combine updated and current parameters
    for (int j = 0; j < NTERMS; j++) {
        weights[MG][j] = round(params[MG][j] + cparams[MG][j]);
        weights[EG][j] = round(params[EG][j] + cparams[EG][j]);
    }

	std::ofstream myfile;
	myfile.open ("scratch.cpp");
		
	int count = 0;

	myfile << "\n";

	myfile << "constexpr int" << "\n\n";

	myfile  << "weight_val_pawn = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_knight = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_bishop = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_rook = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;
	myfile	<< ",\nweight_val_queen = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;
	
	myfile  <<", \n";

	myfile  << "\nweight_pawn_island = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_isolated_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_backward_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_double_pawn = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_pawn_hole = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;
	
	myfile <<", \n" << "arr_weight_pawn_chain[8] = { "; 
	for(int i = 0; i < 8; i++) {

		myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++; 
	}
	
	myfile <<"}, \n" << "arr_weight_phalanx_pawn[8] = { "; 
	for(int i = 0; i < 8; i++) {

		myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++; 
	}

	myfile <<"}, \n" << "arr_weight_defended_phalanx_pawn[8] = { "; 
	for(int i = 0; i < 8; i++) {

		myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++; 
	}

	myfile <<"}, \n" << "arr_weight_passed_pawn[8] = { "; 
	for(int i = 0; i < 8; i++) {

		myfile << "S(" <<(int)weights[MG][count] << ", "<< (int)weights[EG][count]<<")" << ", "; count++; 
	}
	
	myfile << "}, \n" << "arr_weight_defended_passed_pawn[8] = { "; 
	for(int i = 0; i < 8; i++) {

		myfile << "S(" <<(int)weights[MG][count] << ", "<< (int)weights[EG][count]<<")" << ", "; count++;
	}
   	
   	myfile << "}, \n\n";
	
	
	myfile << "weight_knight_all_pawns_count = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_knight_outpost = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_undefended_knight = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_knight_defended_by_pawn = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
	myfile << "\n\n";


 	myfile << "weight_bishop_pair = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ","; count++;
 	myfile << "weight_undefended_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ","; count++;

   	myfile << "\n\n";


 	myfile << "weight_rook_behind_stm_passed_pawn = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++; 
	myfile << "\nweight_rook_behind_opp_passed_pawn = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_flank_outpost = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_half_open_file = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_open_file = " 					<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_enemy_queen_same_file = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_on_seventh_rank = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_on_eight_rank = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_supporting_friendly_rook = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n\n"; count++;

  
   	myfile << "weight_queen_underdeveloped_pieces = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n\n"; count++;


	myfile << "weight_king_pawn_shield = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;   	
	myfile << "weight_king_enemy_pawn_storm = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "\n\n";


	myfile << "arr_weight_knight_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
		
		if(((i + 1) % 8) == 0) 
			myfile<<"\n";
	}
   	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_bishop_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
		
		if(((i + 1) % 8) == 0) 
			myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_rook_mobility[16] = { \n\n";
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
		
		if(((i + 1) % 8) == 0) 
			myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, "<<"\n";

	myfile << "arr_weight_queen_mobility[32] = { \n\n";
    for (int i = 0; i < 32; i++) {
    
    	myfile << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
		
		if(((i + 1) % 8) == 0) 
			myfile<<"\n";
	}
	myfile << "\n"; 
	myfile <<"}, ";


	myfile << "\n\n";

	myfile << "weight_center_control = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
   
	myfile << "\n\n";


 	myfile << "weight_knight_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_bishop_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_queen_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_safe_contact_check = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_queen_safe_contact_check = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_knight_check = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_bishop_check = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_check = " 					<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_queen_check = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_safety_adjustment = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;

	myfile << "\n\n";


	// PSQT weights
	
	myfile << "\n" << "kingPSQT[U8_MAX_SQUARES] = {" 	<<	"\n\n";	
	for (int i = 0; i < U8_MAX_SQUARES; i++) { 

		myfile << "S(" 
				<< std::setw(4) << (int)weights[MG][count] << "," 
				<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if (((i + 1) % 8) == 0) 
			myfile << "\n";
	}	
	myfile << "}," << "\n"; 	
	
	for (int piece = PAWNS; piece < KING; piece++) {

		if (piece == PAWNS)
			myfile << "\n" << "pawnPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES] = {" 	<<	"\n\n";
		if (piece == KNIGHTS)
			myfile << "\n" << "knightPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES] = {" <<	"\n\n";
		if (piece == BISHOPS)
			myfile << "\n" << "bishopPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES] = {" <<	"\n\n";
		if (piece == ROOKS)
			myfile << "\n" << "rookPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES] = {" 	<<	"\n\n";
		if (piece == QUEEN)
			myfile << "\n" << "queenPSQT[U8_MAX_SQUARES][U8_MAX_SQUARES] = {" 	<<	"\n\n";

		for (int j = 0; j < U8_MAX_SQUARES; j++) { 
			for (int i = 0; i < U8_MAX_SQUARES; i++) { 

				myfile << "S(" 
						<< std::setw(4) << (int)weights[MG][count] << "," 
						<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
					
				if (((i + 1) % 16) == 0) 
					myfile << "\n";
			}  
		}	

		if (piece == QUEEN)
			myfile << "};" << "\n"; 	
		else 
			myfile << "}," << "\n"; 	
	}

	myfile.close();

	assert(count == NTERMS);
}
