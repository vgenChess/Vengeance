#include <vector>

#include "history.h"
#include "thread.h"
#include "globals.h"
#include "functions.h"

void updateHistory(Side stm, int ply, int depth, U32 bestMove, std::vector<U32> &quietMovesPlayed, Thread *th) {

	int32_t hScore;
	int32_t bonus = std::min(400, depth * depth), delta = 0;

	U32 previousMove;

	// History Heuristics
	for (auto &move : quietMovesPlayed) { 

		delta = (move == bestMove) ? bonus : -bonus;
		hScore = th->historyScore[stm][from_sq(move)][to_sq(move)];

		th->historyScore[stm][from_sq(move)][to_sq(move)] += 32 * delta - hScore * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->historyScore[side][from_sq(move)][to_sq(move)] > 10000)
		//     std::cout << th->historyScore[side][from_sq(move)][to_sq(move)] << ", ";
		// mtx.unlock();
	}

	// Counter move heuristics
	previousMove = ply == 0 ? NO_MOVE : th->moveStack[ply - 1].move;

	if (previousMove != NO_MOVE) {

		th->counterMove[stm][from_sq(previousMove)][to_sq(previousMove)] = bestMove;
	}
}

void updateCaptureHistory(int depth, U32 bestMove,std::vector<U32>&captureMovesPlayed, Thread *th) {

	int32_t bonus = std::min(400, depth * depth), delta = 0;

	int32_t score;

	uint16_t atk_piece, to, cap_piece;
	
	U8 mt;

	// Capture History Heuristics	
	for (auto &move : captureMovesPlayed) {	
	
		delta = (move == bestMove) ? bonus : -bonus;

		atk_piece = pieceType(move);
		to = to_sq(move);
		cap_piece = cPieceType(move);

		mt = move_type(move);

		if (mt == MOVE_ENPASSANT || mt == MOVE_PROMOTION) 
			cap_piece = PAWNS;

		score = th->captureHistoryScore[atk_piece][to][cap_piece];

		th->captureHistoryScore[atk_piece][to][cap_piece] += 32 * delta - score * std::abs(delta) / 512;

		// mtx.lock();
		// if (th->captureHistoryScore[atk_piece][to][cap_piece] > 1000)
		//     std::cout << th->captureHistoryScore[atk_piece][to][cap_piece] << ", ";
		// mtx.unlock();
	}
}