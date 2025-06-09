#ifndef CHESS_AI_H
#define CHESS_AI_H

#include "ChessBoard.h"     // To interact with the board state.
#include "Move.h"           // To define and return moves.
#include "MoveGenerator.h"  // To generate legal moves.
#include <vector>           // For std::vector to store moves and variations.
#include <string>           // For debug output, if needed.

// Value used to represent a checkmate score. Should be higher than any possible material score.
// Ensures that checkmate is always preferred over any material gain.
const int MATE_VALUE = 200000000;

// ============================================================================
// Piece-Square Tables (PSTs) - New Addition for Positional Evaluation
// Values are for White's pieces. Black's values will be mirrored.
// These are simple, common values; fine-tuning can improve performance.
// ============================================================================

const int PAWN_PST[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10, -20, -20,  10,  10,   5,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,   5,  10,  25,  25,  10,   5,   5,
    10,  10,  20,  30,  30,  20,  10,  10,
    50,  50,  50,  50,  50,  50,  50,  50,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int KNIGHT_PST[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50
};

const int BISHOP_PST[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

const int ROOK_PST[64] = {
    0,   0,   0,   5,   5,   0,   0,   0,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    5,  10,  10,  10,  10,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};

const int QUEEN_PST[64] = {
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -5,    0,   5,   5,   5,   5,   0,  -5,
    0,     0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

const int KING_PST[64] = {
    // Opening/Middlegame King Safety
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,   0,   0,   0,   0,  20,  20,
    20,  30,  10,   0,   0,  10,  30,  20
    // Note: For endgame, King PST would typically encourage centralization.
    // This table assumes an early/mid-game king safety bias towards corners.
};

// ============================================================================
// End Piece-Square Tables
// ============================================================================

// The ChessAI class encapsulates the logic for the chess engine's "brain",
// including evaluation functions and search algorithms.
class ChessAI {
public:
    // Constructor.
    ChessAI();

    // Main function to find the best move for the current board state.
    // This is the entry point for the AI to make a move decision.
    Move findBestMove(ChessBoard& board);

private:
    // Internal counters for debugging and performance analysis.
    long long nodes_evaluated_count;
    long long branches_explored_count;
    int current_search_depth_set;

    // A single instance of MoveGenerator to avoid repeated construction.
    MoveGenerator move_gen;

    // Evaluates the current board position from the perspective of the active player.
    // A positive score indicates a better position for the active player.
    // This is a basic material-only evaluation for now.
    int evaluate(const ChessBoard& board) const;

    // The Alpha-Beta pruning search algorithm.
    // Recursively explores the game tree to find the best possible score.
    // @param board The current chess board state (modified temporarily).
    // @param depth The remaining search depth.
    // @param alpha The alpha bound (best score found for maximizing player so far).
    // @param beta The beta bound (best score found for minimizing player so far).
    // @param variation A vector to store the principal variation (sequence of best moves).
    // @return The evaluated score for the current node.
    int alphaBeta(ChessBoard& board, int depth, int alpha, int beta, std::vector<Move>& variation);
};

#endif // CHESS_AI_H
