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
     * The ChessAI operates on a const reference to the ChessBoard, ensuring it does
     * not modify the actual game state directly. It performs its search by creating
     * temporary board states (via make/unmake calls).
     *
     * @param board The current state of the chessboard (non-const reference needed for apply/undo).
     * @return The calculated best legal move. If no legal moves are found (e.g.,
     * checkmate or stalemate), this method would typically return a default-constructed
     * or special 'null' move. For now, it will pick a random legal move.
     */
    Move findBestMove(ChessBoard& board);

    // --- Future Search and Evaluation Methods ---
    // These would typically be private helpers that implement the actual AI logic.
    // int minimax(ChessBoard& board, int depth);
    // int alphaBeta(ChessBoard& board, int depth, int alpha, int beta, PlayerColor player_to_maximize);
    // int evaluate(const ChessBoard& board) const;
    // void orderMoves(std::vector<Move>& moves) const; // For move ordering during search

private:
    // A random number generator for simple move selection in the initial phase.
    // Will be replaced by actual search logic.
    std::mt19937_64 rng_engine; 
};

#endif // CHESS_AI_H
