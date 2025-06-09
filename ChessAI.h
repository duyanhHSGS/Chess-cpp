#ifndef CHESS_AI_H
#define CHESS_AI_H

#include "ChessBoard.h" 
#include "Move.h"       
#include "Types.h"    
#include <vector>

class ChessAI {
public:
    ChessAI();
    Move findBestMove(ChessBoard& board);

private:
    unsigned long long nodes_evaluated_count;
    unsigned long long branches_explored_count;
    unsigned int current_search_depth_set;

    int evaluate(const ChessBoard& board) const;
    int alphaBeta(ChessBoard& board, int depth, int alpha, int beta, std::vector<Move>& variation);
};

#endif // CHESS_AI_H
