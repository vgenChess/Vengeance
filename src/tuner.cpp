
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
#define NPOSITIONS		1428000
#define NPARTITIONS		4 
#define BATCHSIZE		256 
#define LR              0.00125 
#define DISPLAY_TIME	60				

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


	loadCoeff->type[i] = NORMAL;
    loadCoeff->coeffs[WHITE][i] = T->minorPawnShield[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->minorPawnShield[BLACK];                         


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


	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			loadCoeff->type[i] = NORMAL;	
		    loadCoeff->coeffs[WHITE][i] = T->pawnShield[edge_distance][rank][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->pawnShield[edge_distance][rank][BLACK];                         
		}
	}

	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			loadCoeff->type[i] = NORMAL;	
		    loadCoeff->coeffs[WHITE][i] = T->blockedStorm[edge_distance][rank][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->blockedStorm[edge_distance][rank][BLACK];                         
		}
	}

	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			loadCoeff->type[i] = NORMAL;	
		    loadCoeff->coeffs[WHITE][i] = T->unblockedStorm[edge_distance][rank][WHITE];                         
		    loadCoeff->coeffs[BLACK][i++] = T->unblockedStorm[edge_distance][rank][BLACK];                         
		}
	}


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
    loadCoeff->coeffs[WHITE][i] = T->safeKnightCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safeKnightCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->safeBishopCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safeBishopCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->safeRookCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safeRookCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->safeQueenCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safeQueenCheck[BLACK];                         


    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->unsafeKnightCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->unsafeKnightCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->unsafeBishopCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->unsafeBishopCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->unsafeRookCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->unsafeRookCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->unsafeQueenCheck[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->unsafeQueenCheck[BLACK];                         

    loadCoeff->type[i] = SAFETY;
    loadCoeff->coeffs[WHITE][i] = T->safetyAdjustment[WHITE];                         
    loadCoeff->coeffs[BLACK][i++] = T->safetyAdjustment[BLACK];                         




    //PSQT

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->pawnPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->pawnPSQT[sq][BLACK];                         
    }

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->knightPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->knightPSQT[sq][BLACK];                         
    }

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->bishopPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->bishopPSQT[sq][BLACK];                         
    }

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->rookPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->rookPSQT[sq][BLACK];                         
    }

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->queenPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->queenPSQT[sq][BLACK];                         
    }

    for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		loadCoeff->type[i] = NORMAL;

	    loadCoeff->coeffs[WHITE][i] = T->kingPSQT[sq][WHITE];                         
	    loadCoeff->coeffs[BLACK][i++] = T->kingPSQT[sq][BLACK];                         
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


	cparams[MG][count] = ScoreMG(weight_minor_has_pawn_shield);
	cparams[EG][count++] = ScoreEG(weight_minor_has_pawn_shield);


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


	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			cparams[MG][count] = ScoreMG(weight_pawn_shield[edge_distance][rank]);
			cparams[EG][count++] = ScoreEG(weight_pawn_shield[edge_distance][rank]);                       
		}
	}


	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			cparams[MG][count] = ScoreMG(weight_blocked_pawn_storm[edge_distance][rank]);
			cparams[EG][count++] = ScoreEG(weight_blocked_pawn_storm[edge_distance][rank]);                        
		}
	}


	for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			cparams[MG][count] = ScoreMG(weight_unblocked_pawn_storm[edge_distance][rank]);
			cparams[EG][count++] = ScoreEG(weight_unblocked_pawn_storm[edge_distance][rank]);                        
		}
	}


	cparams[MG][count] = ScoreMG(weight_knight_attack);
	cparams[EG][count++] = ScoreEG(weight_knight_attack);

	cparams[MG][count] = ScoreMG(weight_bishop_attack);
	cparams[EG][count++] = ScoreEG(weight_bishop_attack);

	cparams[MG][count] = ScoreMG(weight_rook_attack);
	cparams[EG][count++] = ScoreEG(weight_rook_attack);

	cparams[MG][count] = ScoreMG(weight_queen_attack);
	cparams[EG][count++] = ScoreEG(weight_queen_attack);



	cparams[MG][count] = ScoreMG(weight_safe_knight_check);
	cparams[EG][count++] = ScoreEG(weight_safe_knight_check);

	cparams[MG][count] = ScoreMG(weight_safe_bishop_check);
	cparams[EG][count++] = ScoreEG(weight_safe_bishop_check);

	cparams[MG][count] = ScoreMG(weight_safe_rook_check);
	cparams[EG][count++] = ScoreEG(weight_safe_rook_check);

	cparams[MG][count] = ScoreMG(weight_safe_queen_check);
	cparams[EG][count++] = ScoreEG(weight_safe_queen_check);


	cparams[MG][count] = ScoreMG(weight_unsafe_knight_check);
	cparams[EG][count++] = ScoreEG(weight_unsafe_knight_check);

	cparams[MG][count] = ScoreMG(weight_unsafe_bishop_check);
	cparams[EG][count++] = ScoreEG(weight_unsafe_bishop_check);

	cparams[MG][count] = ScoreMG(weight_unsafe_rook_check);
	cparams[EG][count++] = ScoreEG(weight_unsafe_rook_check);

	cparams[MG][count] = ScoreMG(weight_unsafe_queen_check);
	cparams[EG][count++] = ScoreEG(weight_unsafe_queen_check);





	cparams[MG][count] = ScoreMG(weight_safety_adjustment);
	cparams[EG][count++] = ScoreEG(weight_safety_adjustment);

	
	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(pawnPSQT[sq]);
		cparams[EG][count++] = ScoreEG(pawnPSQT[sq]);
	}

	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(knightPSQT[sq]);
		cparams[EG][count++] = ScoreEG(knightPSQT[sq]);
	}

	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(bishopPSQT[sq]);
		cparams[EG][count++] = ScoreEG(bishopPSQT[sq]);
	}

	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(rookPSQT[sq]);
		cparams[EG][count++] = ScoreEG(rookPSQT[sq]);
	}

	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(queenPSQT[sq]);
		cparams[EG][count++] = ScoreEG(queenPSQT[sq]);
	}

	for (int sq = 0; sq < U8_MAX_SQUARES; sq++) {
	
		cparams[MG][count] = ScoreMG(kingPSQT[sq]);
		cparams[EG][count++] = ScoreEG(kingPSQT[sq]);
	}


	assert(count == NTERMS);

	std::fstream newfile;
   	// newfile.open ("lichess-big3-resolved.book", std::ios::in);	// NPOSITIONS = 7150000 roughly
	newfile.open ("quiet-labeled.epd", std::ios::in); 			// v6 NPOSITIONS = 725000 exact
																// v7 NPOSITIONS = 1428000 exact
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
			
			if (fen.length() <= 0) 
				continue;


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

			/*if (tp.find("1.0") != std::string::npos)	
				result = 1.0;
			else if (tp.find("0.5") != std::string::npos) 
				result = 0.5;
			else if (tp.find("0.0") != std::string::npos)	
				result = 0.0;
			else 
				continue;
*/
			
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

	// For Adagrad
	// TVector adagrad = {};
	
	// For RMS Prop
	// TVector cache = {};
	// double alpha1 = LR, beta1 = 0.9;

	// For Adam optimiser
	double alpha1 = LR, beta1 = 0.9, beta2 = 0.999;
	TVector M = {}, R = {};
	
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


		// RMSProp (slower than Adam)
/*		#pragma omp parallel for schedule(auto)
		for (int i = 0; i < NTERMS; i++) {

	        cache[MG][i] = beta1 * cache[MG][i] + (1.0 - beta1) * pow((K / 200.0) * gradient[MG][i] / BATCHSIZE, 2.0);
	        cache[EG][i] = beta1 * cache[EG][i] + (1.0 - beta1) * pow((K / 200.0) * gradient[EG][i] / BATCHSIZE, 2.0);
	        params[MG][i] += (K / 200.0) * (gradient[MG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + cache[MG][i]));
	        params[EG][i] += (K / 200.0) * (gradient[EG][i] / BATCHSIZE) * (alpha1 / sqrt(1e-8 + cache[EG][i]));
		}
*/

		// Adam
		#pragma omp parallel for schedule(auto)
		for (int i = 0; i < NTERMS; ++i) {
			
		    M[MG][i] = beta1 * M[MG][i] + (1.0 - beta1) * (K / 200.0) * gradient[MG][i] / BATCHSIZE;
		    M[EG][i] = beta1 * M[EG][i] + (1.0 - beta1) * (K / 200.0) * gradient[EG][i] / BATCHSIZE;

			R[MG][i] = beta2 * R[MG][i] + (1.0 - beta2) * pow((K / 200.0) * gradient[MG][i] / BATCHSIZE, 2.0);
        	R[EG][i] = beta2 * R[EG][i] + (1.0 - beta2) * pow((K / 200.0) * gradient[EG][i] / BATCHSIZE, 2.0);

        	params[MG][i] += alpha1 * (M[MG][i] / (1.0 - pow(beta1, epoch))) / (sqrt(R[MG][i] / (1.0 - pow(beta2, epoch))) + 1e-8);
        	params[EG][i] += alpha1 * (M[EG][i] / (1.0 - pow(beta1, epoch))) / (sqrt(R[EG][i] / (1.0 - pow(beta2, epoch))) + 1e-8);
		}
	


		auto tunerTimeNow = std::chrono::steady_clock::now();

		if (std::chrono::duration_cast<std::chrono::seconds>(tunerTimeNow - tunerStartTime).count() > DISPLAY_TIME) {

			tunerStartTime = std::chrono::steady_clock::now();

			mae = tunedEvaluationErrors(params, K);

			std::cout << "Epoch = [" << epoch * BATCHSIZE / NPOSITIONS << "], ";
			std::cout << "Error = [" << std::fixed << std::setprecision(8) << mae << "], ";
			std::cout << "Rate = [" << std::fixed << std::setprecision(5) << LR << "], ";
			std::cout << "Batch Size = [" << BATCHSIZE << "], ";
			
			if (prevMae > mae)
				std::cout << "Error loss = [" << std::fixed << std::setprecision(8) << prevMae - mae << "]" << std::endl;
			else
				std::cout << "Error gain = [" << std::fixed << std::setprecision(8) << mae - prevMae << "]" << std::endl;

			counter++;
			if (counter > 4) {

				counter = 0;

				if (mae < bestMae) {

					bestMae = mae;

					std::cout << "Saving weights..\n";
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

	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;
    // Save any modifications for MG or EG for each evaluation type
	#pragma omp parallel for schedule(auto)
	for (uint64_t i = 0; i < list.size(); i++)
	{
		mg[list[i].type][WHITE] += (double) list[i].wcoeff * weights[MG][list[i].index];
        mg[list[i].type][BLACK] += (double) list[i].bcoeff * weights[MG][list[i].index];
        eg[list[i].type][WHITE] += (double) list[i].wcoeff * weights[EG][list[i].index];
        eg[list[i].type][BLACK] += (double) list[i].bcoeff * weights[EG][list[i].index];
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
	if (gradientData != NULL) 
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
	
    double mgBase = A * data.pfactors[MG];
    double egBase = A * data.pfactors[EG];

	std::vector<CoefficientsInfo> list = data.coefficientsInfoList;

	#pragma omp parallel for schedule(auto)
	for (uint64_t i = 0; i < list.size(); i++)
	{
		if (list[i].type == NORMAL)
		{
            gradient[MG][list[i].index] += mgBase * (list[i].wcoeff - list[i].bcoeff);
		    gradient[EG][list[i].index] += egBase * (list[i].wcoeff - list[i].bcoeff);
		}
		
		if (list[i].type == SAFETY)
		{
			gradient[MG][list[i].index] += (mgBase / 360.0) 
				* (fmax(gradientData.bsafetymg, 0) * list[i].bcoeff - (fmax(gradientData.wsafetymg, 0) * list[i].wcoeff));

			gradient[EG][list[i].index] += (egBase /  20.0) 
				* ((gradientData.bsafetyeg > 0.0) * list[i].bcoeff - (gradientData.wsafetyeg > 0.0) * list[i].wcoeff);
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
        
        #pragma omp for schedule(auto)
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
	myfile.open ("weights.h");
		

	int count = 0;

	myfile << "\n";

	myfile << "#ifndef WEIGHTS_H\n";
	myfile << "#define WEIGHTS_H\n\n";

	myfile << "#include \"functions.h\"\n";
	
	myfile << "\n\n";


	myfile << "constexpr int" << "\n\n";

	myfile  << "weight_val_pawn = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_knight = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_bishop = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile	<< ",\nweight_val_rook = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;
	myfile	<< ",\nweight_val_queen = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;
	
	myfile  <<", \n";

	myfile  << "\nweight_isolated_pawn = "  << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_backward_pawn = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_double_pawn = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++; 
	myfile  << ",\nweight_pawn_hole = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")"; count++;

	myfile  <<", \n\n";

	myfile  << "arr_weight_pawn_chain[8] = { \n\n";
	
	for(int i = 0; i < 8; i++) {

    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";	
	}
	
	myfile <<"\n}, \n";
		
	myfile << "arr_weight_phalanx_pawn[8] = { \n\n";
	
	for(int i = 0; i < 8; i++) {

    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";	
	}

	myfile <<"\n}, \n";
		
	myfile << "arr_weight_defended_phalanx_pawn[8] = { \n\n";
	
	for(int i = 0; i < 8; i++) {

    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}

	myfile <<"\n}, \n";
		
	myfile << "arr_weight_passed_pawn[8] = { \n\n";
	
	for(int i = 0; i < 8; i++) {

    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
			
	}

	myfile <<"\n}, \n";
			
	myfile << "arr_weight_defended_passed_pawn[8] = { \n\n";
	
	for(int i = 0; i < 8; i++) {

    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}
   	
	myfile <<"\n}, \n";
			
	
	myfile << "weight_minor_has_pawn_shield = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n\n"; count++;
	

	myfile << "weight_knight_all_pawns_count = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_knight_outpost = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_undefended_knight = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", \n"; count++;
	myfile << "weight_knight_defended_by_pawn = " 	<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;
	myfile << "\n\n";


 	myfile << "weight_bishop_pair = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ","; count++;
 	myfile << "\nweight_undefended_bishop = " << "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ","; count++;

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


	myfile << "arr_weight_knight_mobility[16] = { \n\n";
    
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
			
		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}

	myfile <<"\n}, \n";
		
	myfile << "arr_weight_bishop_mobility[16] = { \n\n";
    
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
				
		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}

	myfile <<"\n}, \n";
		
	myfile << "arr_weight_rook_mobility[16] = { \n\n";
    
    for (int i = 0; i < 16; i++) {
    
    	myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 

		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}

	myfile <<"\n}, \n";
		
	myfile << "arr_weight_queen_mobility[32] = { \n\n";
    
    for (int i = 0; i < 32; i++) {

		myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
			
		if(((i + 1) % 4) == 0) 
			myfile<<"\n";
	}
	
	myfile <<"\n}, \n";
		

	myfile << "\n\n";


	myfile << "weight_pawn_shield[8][8] = { \n\n";
    
    for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
				
			if (((rank + 1) % 8) == 0) 
				myfile << "\n";                     
		}
	}

	myfile <<"\n}, \n";
		


	myfile << "weight_blocked_pawn_storm[8][8] = { \n\n";
    
    for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
				
			if (((rank + 1) % 8) == 0) 
				myfile << "\n";                     
		}
	}

	myfile <<"\n}, \n";
		

	myfile << "weight_unblocked_pawn_storm[8][8] = { \n\n";
    
    for (int edge_distance = 0; edge_distance < 8; edge_distance++)
	{
		for (int rank = 0; rank < 8; rank++)
		{
			myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
				
			if (((rank + 1) % 8) == 0) 
				myfile << "\n";                     
		}
	}
	
	myfile <<"\n}, \n\n";
		

 	myfile << "weight_knight_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_bishop_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_rook_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_queen_attack = " 				<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;

	myfile << "\nweight_safe_knight_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_safe_bishop_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_safe_rook_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_safe_queen_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;

	myfile << "\nweight_unsafe_knight_check = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_unsafe_bishop_check = " 		<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_unsafe_rook_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;
	myfile << "\nweight_unsafe_queen_check = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", " ; count++;

	myfile << "\nweight_safety_adjustment = " 			<< "S("<<(int)weights[MG][count]<<", "<<(int)weights[EG][count]<<")" << ", "; count++;

	myfile << "\n\n";


	// PSQT weights
	
	for (int piece = PAWNS; piece <= KING; piece++) {

		if (piece == PAWNS)
			myfile << "\n" << "pawnPSQT[U8_MAX_SQUARES] = {" 	<<	"\n\n";
		if (piece == KNIGHTS)
			myfile << "\n" << "knightPSQT[U8_MAX_SQUARES] = {" <<	"\n\n";
		if (piece == BISHOPS)
			myfile << "\n" << "bishopPSQT[U8_MAX_SQUARES] = {" <<	"\n\n";
		if (piece == ROOKS)
			myfile << "\n" << "rookPSQT[U8_MAX_SQUARES] = {" 	<<	"\n\n";
		if (piece == QUEEN)
			myfile << "\n" << "queenPSQT[U8_MAX_SQUARES] = {" 	<<	"\n\n";
		if (piece == KING)
			myfile << "\n" << "kingPSQT[U8_MAX_SQUARES] = {" 	<<	"\n\n";

		for (int i = 0; i < U8_MAX_SQUARES; i++) { 

			myfile << "S(" 
					<< std::setw(4) << (int)weights[MG][count] << "," 
					<< std::setw(4) << (int)weights[EG][count] << ")" << ", "; count++; 
				
			if (((i + 1) % 8) == 0) 
				myfile << "\n";
		}  

		if (piece == KING)
			myfile << "};" << "\n"; 	
		else 
			myfile << "}," << "\n"; 	
	}

	myfile << "\n\n";

	myfile << "#endif\n";

	myfile.close();

	assert(count == NTERMS);
}
