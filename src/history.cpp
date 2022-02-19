#include <vector>

#include "history.h"
#include "thread.h"
#include "globals.h"
#include "functions.h"


void updateHistory(int ply, int side, int depth, u32 bestMove, std::vector<u32> &quietMovesPlayed, Thread *th) {


	int bonus = std::min(400, depth * depth), delta = 0;

	int32_t hScore;

	u32 previousMove;

	// History Heuristics
	for (auto &move : quietMovesPlayed) { 

		delta = (move == bestMove) ? bonus : -bonus;
		hScore = th->historyScore[side][from_sq(move)][to_sq(move)];

		th->historyScore[side][from_sq(move)][to_sq(move)] += 32 * delta - hScore * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->historyScore[side][from_sq(move)][to_sq(move)] > 10000)
		//     std::cout << th->historyScore[side][from_sq(move)][to_sq(move)] << ", ";
		// mtx.unlock();
	}

	// Counter move heuristics
	previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

	if (previousMove != NO_MOVE) {

		th->counterMove[side][from_sq(previousMove)][to_sq(previousMove)] = bestMove;
	}
}


void updateCaptureHistory(int ply, int side, int depth, u32 bestMove,std::vector<u32>&captureMovesPlayed, Thread *th) {


	int bonus = std::min(400, depth * depth), delta = 0;

	int32_t chScore;

	uint16_t atk_piece, to, cap_piece;
	
	u8 mt;

	// Capture History Heuristics	
	for (auto &move : captureMovesPlayed) {	
	
		delta = (move == bestMove) ? bonus : -bonus;

		atk_piece = pieceType(move);
		to = to_sq(move);
		cap_piece = cPieceType(move);

		mt = move_type(move);

		if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) 
			cap_piece = PAWNS;

		chScore = th->capture_history_score[atk_piece][to][cap_piece];

		th->capture_history_score[atk_piece][to][cap_piece] += 32 * delta - chScore * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->capture_history_score[atk_piece][to][cap_piece] > 1000)
		//     std::cout << th->capture_history_score[atk_piece][to][cap_piece] << ", ";
		// mtx.unlock();
	}
}