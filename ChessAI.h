// ChessAI.h
#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <SDL.h>
#include <vector>
#include <limits> // For std::numeric_limits
#include <algorithm> // For std::max, std::min

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

    // Helper to generate all pseudo-legal moves for the current player
    // (doesn't check for king safety yet)
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer);

    // Helper to apply a move (for simulation)
    void applyMove(GameManager* gameManager, int pieceIdx, SDL_Point oldPos, SDL_Point newPos);

    // Helper to undo a move (for backtracking in minimax)
    void undoMove(GameManager* gameManager);
};

#endif // CHESS_AI_H
