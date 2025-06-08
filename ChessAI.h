#ifndef CHESS_AI_H
#define CHESS_AI_H

// Include necessary standard library headers
#include <vector> // For std::vector to work with moves
#include <random> // REQUIRED: For std::mt19937_64, std::random_device

// Include our custom chess engine component headers
#include "ChessBoard.h" // To interact with the ChessBoard state
#include "Move.h"       // To return a best move
#include "Types.h"      // For PlayerColor, PieceTypeIndex etc.

/**
 * @class ChessAI
 * @brief Encapsulates the artificial intelligence for finding the best chess moves.
 *
 * This class will contain the search algorithm (e.g., Minimax, Alpha-Beta pruning)
 * and the evaluation function responsible for scoring chess positions.
 *
 * Primary Responsibilities:
 * - **Search Algorithm:** Implement algorithms to explore the game tree.
 * - **Evaluation Function:** Assign a numerical score to a given board position,
 * representing its favorability for the active player.
 * - **Move Selection:** Determine the "best" move based on the search results.
 */
class ChessAI {
public:
    /**
     * @brief Constructs a new ChessAI object.
     * Any initial setup for the AI (e.g., loading opening books, setting up hash tables)
     * would go here.
     */
    ChessAI();

    /**
     * @brief Finds and returns the best legal move for the current active player
     * on the given board.
     *
     * This is the main interface for the GameManager to request a move from the AI.
     * The ChessAI operates on a non-const reference to the ChessBoard, as it will
     * temporarily apply and undo moves during its search.
     *
     * @param board The current state of the chessboard (non-const reference).
     * @return The calculated best legal move. If no legal moves are found (e.g.,
     * checkmate or stalemate), this method returns a default-constructed
     * or special 'null' move.
     */
    Move findBestMove(ChessBoard& board);

private:
    // A random number generator used by the AI (e.g., for random move selection or tie-breaking).
    std::mt19937_64 rng_engine; 

    /**
     * @brief Evaluates the given chess board position and returns a numerical score.
     * A higher score indicates a better position for the active player.
     *
     * This is a placeholder evaluation function and currently only considers material.
     * Future enhancements will include positional factors, king safety, pawn structure, etc.
     *
     * @param board The chess board position to evaluate (const reference, as it's not modified).
     * @return An integer representing the evaluation score. Positive for white advantage,
     * negative for black advantage.
     *
     * Example:
     * - A board where White is up a Queen might return +900.
     * - A board where Black is up a Rook might return -500.
     */
    int evaluate(const ChessBoard& board) const;
};

#endif // CHESS_AI_H
