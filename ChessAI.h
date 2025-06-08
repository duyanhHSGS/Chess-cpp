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
     * @brief Implements the recursive Minimax search algorithm.
     *
     * Minimax is a decision-making algorithm used to choose the optimal move for a player,
     * assuming the opponent also plays optimally. It works by building a game tree
     * and evaluating positions at a certain depth.
     *
     * The function always returns a score from the perspective of the *current active player*
     * at the node being evaluated. For example, if it's White's turn at `current_board`,
     * `minimax` returns a score White wants to maximize. If it's Black's turn, it returns
     * a score Black wants to maximize (which is negative from White's absolute perspective).
     *
     * @param board The current state of the chessboard (non-const reference).
     * @param depth The remaining depth to search. When depth reaches 0, the `evaluate`
     * function is called to get a static score.
     * @return The best score found for the `board.active_player` at the current node,
     * assuming optimal play from both sides down to the specified depth.
     */
    int minimax(ChessBoard& board, int depth);
};

#endif // CHESS_AI_H
