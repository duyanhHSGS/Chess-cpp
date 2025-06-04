#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <vector>
#include <limits>
#include <algorithm>
#include <tuple>

#include "Constants.h"
#include "ChessPiece.h" 

class GameManager;

class ChessAI {
public:
    ChessAI();

    GamePoint getBestMove(GameManager* gameManager, bool isMaximizingPlayer);

    int getEvaluation(GameManager* gameManager) const;

private:
    int minimax(GameManager* gameManager, int depth, int alpha, int beta, bool isMaximizingPlayer);

    int quiescenceSearch(GameManager* gameManager, int alpha, int beta, bool isMaximizingPlayer);

    std::vector<std::tuple<int, GamePoint, GamePoint>> generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer);

    void applyMove(GameManager* gameManager, int pieceIdx, GamePoint oldPos, GamePoint newPos);

    void undoMove(GameManager* gameManager);

    bool isEndgame(GameManager* gameManager) const;

    static const int pawnPST[8][8];
    static const int knightPST[8][8];
    static const int bishopPST[8][8];
    static const int rookPST[8][8];
    static const int queenPST[8][8];
    static const int kingPST_middlegame[8][8];
    static const int kingPST_endgame[8][8];
};

#endif // CHESS_AI_H
