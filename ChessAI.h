// ChessAI.h
#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <SDL.h> // Required for SDL_Point
#include <vector> // For std::vector if used for moves

// Forward declaration to avoid circular dependency with GameManager
class GameManager;

class ChessAI {
public:
    // Calculates the best move for the AI using Alpha-Beta pruning.
    // gameManager: Pointer to the GameManager instance to access game state and simulate moves.
    // isMaximizingPlayer: True if the AI is the maximizing player (White), false if minimizing (Black).
    SDL_Point getBestMove(GameManager* gameManager, bool isMaximizingPlayer);

    // Implements the Alpha-Beta pruning algorithm.
    // depth: Current depth of the search.
    // isMaximizingPlayer: True if the current node is a maximizing node, false if minimizing.
    // alpha: The best score found so far for the maximizing player.
    // beta: The best score found so far for the minimizing player.
    // Returns the evaluated score for the current node.
    int alphaBeta(GameManager* gameManager, int depth, bool isMaximizingPlayer, int alpha, int beta);

    // Evaluates the current state of the board.
    // This function will incorporate material value and positional value (e.g., Piece-Square Tables).
    // gameManager: Pointer to the GameManager instance to access piece positions and types.
    // Returns an integer representing the board's evaluation score.
    int evaluateBoard(GameManager* gameManager);

private:
    // Piece-Square Tables (PSTs) for positional evaluation.
    // These tables assign a value to each square for a specific piece type.
    // The values are typically adjusted for White pieces, and mirrored for Black.
    // Example: Pawn PST (values for white pawns, indexed [rank][file])
    // The values are relative to the piece's base material value.
    // For simplicity, these are defined here. In a larger engine, they might be in a separate config.
    static const int PAWN_PST[8][8];
    static const int KNIGHT_PST[8][8];
    static const int BISHOP_PST[8][8];
    static const int ROOK_PST[8][8];
    static const int QUEEN_PST[8][8];
    static const int KING_PST[8][8];

    // Helper function to get the mirrored PST value for black pieces
    int getMirroredPSTValue(const int pst[8][8], int x, int y);
};

#endif // CHESS_AI_H
