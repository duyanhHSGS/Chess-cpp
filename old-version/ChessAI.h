#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <vector>
#include <random>
#include <future>
#include <atomic>

#include "ChessBoard.h"
#include "Move.h"
#include "Types.h"

class ChessAI {
public:
    ChessAI();
    Move findBestMove(ChessBoard& board);

private:
    std::mt19937_64 rng_engine; 

    std::atomic<long long> nodes_evaluated_count;
    std::atomic<long long> branches_explored_count;
    int current_search_depth_set;

    int evaluate(const ChessBoard& board) const;
    int minimax(ChessBoard& board, int depth);
    int alphaBeta(ChessBoard& board, int depth, int alpha, int beta);
};

#endif // CHESS_AI_H
