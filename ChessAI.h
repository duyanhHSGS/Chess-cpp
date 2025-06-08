#ifndef CHESS_AI_H
#define CHESS_AI_H

// Include necessary standard library headers
#include <vector> // For std::vector to work with moves
#include <random> // REQUIRED: For std::mt19937_64, std::random_device
#include <future> // REQUIRED: For std::async and std::future for multi-threading
#include <atomic> // REQUIRED: For std::atomic to ensure thread-safe counters

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

    // --- Search Statistics ---
    // These members will be used to track search progress and report it to the user.
    // They are declared as `std::atomic` to ensure thread-safe increments
    // when multiple search threads are updating them concurrently.
    std::atomic<long long> nodes_evaluated_count;    // Total number of nodes (positions) evaluated during the search.
    std::atomic<long long> branches_explored_count;  // Total number of moves generated/explored across all nodes.
    int current_search_depth_set;                   // The maximum depth the search was configured to reach.

    /**
     * @brief Evaluates the given chess board position and returns a numerical score.
     *
     * This evaluation function now consistently returns a score from **White's perspective**.
     * A positive score indicates an advantage for White, and a negative score indicates
     * an advantage for Black.
     *
     * @param board The chess board position to evaluate (const reference, as its state is not modified).
     * @return An integer representing the evaluation score, always from White's perspective.
     * Scores are typically given in centipawns (1/100th of a pawn), so a pawn is 100.
     */
    int evaluate(const ChessBoard& board) const;

    /**
     * @brief Implements the recursive Minimax search algorithm (in Negamax form).
     *
     * This function is implemented in a **Negamax** style. This means that at *every*
     * node, the function attempts to **maximize** the score for the `board.active_player`
     * at that specific node. The negation of the recursive call handles the alternating
     * turns correctly.
     *
     * @param board The current state of the chessboard (non-const reference).
     * @param depth The remaining depth to search. When depth reaches 0, the `evaluate`
     * function is called to get a static score.
     * @return The best score found for the `board.active_player` at the current node.
     * This score is from the perspective of the player whose turn it is at this `board` state.
     */
    int minimax(ChessBoard& board, int depth);
};

#endif // CHESS_AI_H
