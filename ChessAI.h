// ChessAI.h
#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <SDL.h>
#include <vector>
#include <limits> // For std::numeric_limits
#include <algorithm> // For std::max, std::min
#include <tuple> // For std::tuple

#include "Constants.h" // Include Constants to get PlayerColor definition

// Forward declaration to avoid circular dependency
class GameManager;

class ChessAI {
public:
    ChessAI(); // Constructor

    // Main function to get the best move using Alpha-Beta Pruning
    SDL_Point getBestMove(GameManager* gameManager, bool isMaximizingPlayer);

    // Evaluates the current board state.
    // A positive score favors White, a negative score favors Black.
    int getEvaluation(GameManager* gameManager) const;

private:
    // Alpha-Beta Pruning algorithm
    int minimax(GameManager* gameManager, int depth, int alpha, int beta, bool isMaximizingPlayer);

    // Quiescence Search to extend search in "noisy" positions (captures, checks, promotions)
    int quiescenceSearch(GameManager* gameManager, int alpha, int beta, bool isMaximizingPlayer);

    // Helper to generate all pseudo-legal moves for the current player
    // (doesn't check for king safety yet)
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer);

    // Helper to apply a move (for simulation)
    void applyMove(GameManager* gameManager, int pieceIdx, SDL_Point oldPos, SDL_Point newPos);

    // Helper to undo a move (for backtracking in minimax)
    void undoMove(GameManager* gameManager);

    // Determines if the current game state is an endgame based on material
    bool isEndgame(GameManager* gameManager) const;

    // Piece-Square Tables (PSTs) for positional evaluation
    // Values are from White's perspective; Black's values will be mirrored.
    static const int pawnPST[8][8];
    static const int knightPST[8][8];
    static const int bishopPST[8][8];
    static const int rookPST[8][8];
    static const int queenPST[8][8];
    static const int kingPST_middlegame[8][8]; // King PST for middlegame
    static const int kingPST_endgame[8][8];    // King PST for endgame
};

#endif // CHESS_AI_H
