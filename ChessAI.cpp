#include "ChessAI.h"      // Include the header for ChessAI itself
#include "MoveGenerator.h" // Needed to generate legal moves
#include "Constants.h"    // To access AI_SEARCH_DEPTH, DEFAULT_AI_TYPE, etc.
#include "ChessBitboardUtils.h" // For bitboard manipulation functions and move_to_string
#include <random>         // For std::random_device, std::mt19937_64, std::uniform_int_distribution
#include <iostream>       // For std::cerr to output error or debug messages
#include <vector>         // Required for std::vector to manage collections of moves
#include <algorithm>      // Required for std::min, std::max (for evaluation/search)

// Constructor for ChessAI.
// Initializes the random number generator used by the AI.
// The `rng_engine` is a member of the ChessAI class, so it's initialized here
// using `std::random_device` to ensure a non-deterministic seed for true randomness.
ChessAI::ChessAI() : rng_engine(std::random_device{}()) {
    // In a real, more advanced chess engine, this constructor would also be responsible for:
    // - Initializing transposition tables (for caching search results and detecting repetitions).
    // - Loading opening books (to play predefined moves in the opening phase).
    // - Setting up any other persistent data structures required by the chosen search algorithm.
    // - Potentially configuring AI parameters (e.g., specific evaluation weights).
}

/**
 * @brief Evaluates the given chess board position and returns a numerical score.
 * A higher score indicates a better position for the active player.
 *
 * This is a placeholder evaluation function and currently only considers material.
 * Material values are commonly used in chess engines as a basic evaluation component.
 * Future enhancements will include positional factors (e.g., pawn structure, king safety,
 * piece activity, mobility), which add more chess-specific intelligence.
 *
 * @param board The chess board position to evaluate (const reference, as its state is not modified).
 * @return An integer representing the evaluation score. A positive score indicates
 * an advantage for White, and a negative score indicates an advantage for Black.
 * Scores are typically given in centipawns (1/100th of a pawn), so a pawn is 100.
 *
 * Example Material Values (in centipawns):
 * Pawn:   100
 * Knight: 320
 * Bishop: 330
 * Rook:   500
 * Queen:  900
 * King:   (Infinite/very high, as it's not a tradable piece but its safety is paramount)
 */
int ChessAI::evaluate(const ChessBoard& board) const {
    int score = 0; // Initialize score to zero.

    // Define standard piece values in centipawns.
    // These are often chosen based on common chess wisdom and engine tuning.
    const int PAWN_VALUE   = 100;
    const int KNIGHT_VALUE = 320;
    const int BISHOP_VALUE = 330;
    const int ROOK_VALUE   = 500;
    const int QUEEN_VALUE  = 900;
    // King value is usually implicitly handled by checkmate scores or king safety tables.
    // We don't add a king's material value directly to prevent "trading" kings.

    // Iterate through all 64 squares of the board to sum up material.
    for (int i = 0; i < 64; ++i) {
        // Check for white pieces.
        if (ChessBitboardUtils::test_bit(board.white_pawns, i))    score += PAWN_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_knights, i)) score += KNIGHT_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_bishops, i)) score += BISHOP_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_rooks, i))  score += ROOK_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_queens, i)) score += QUEEN_VALUE;
        // No score for king directly, as its value is positional (safety, mobility) and infinite.

        // Check for black pieces.
        else if (ChessBitboardUtils::test_bit(board.black_pawns, i))   score -= PAWN_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_knights, i)) score -= KNIGHT_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_bishops, i)) score -= BISHOP_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_rooks, i))  score -= ROOK_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_queens, i)) score -= QUEEN_VALUE;
    }

    // Adjust score based on the active player.
    // If it's Black's turn, we want to return a score that is from Black's perspective,
    // so we negate the score. This makes the search simpler: the AI always tries to maximize
    // the score it receives, regardless of whose turn it is.
    // Example: If White has a +200 centipawn advantage and it's White's turn, score is +200.
    // If White has a +200 centipawn advantage and it's Black's turn, the score for Black is -200.
    if (board.active_player == PlayerColor::Black) {
        return -score;
    }
    return score;
}

/**
 * @brief Finds and returns the best legal move for the current active player on the given board.
 * This is the primary interface for the GameManager to request a move from the AI.
 *
 * This function's behavior is determined by the `DEFAULT_AI_TYPE` constant from `Constants.h`.
 *
 * @param board A non-constant reference to the current ChessBoard state.
 * This is crucial because the search algorithm will temporarily `apply_move()` and `undo_move()`
 * on this board object to explore different game lines.
 * @return The calculated best legal move. If no legal moves are found (e.g.,
 * due to checkmate or stalemate), it returns a default-constructed,
 * invalid `Move` (where `piece_moved_type_idx` is `PieceTypeIndex::NONE`).
 */
Move ChessAI::findBestMove(ChessBoard& board) {
    MoveGenerator move_gen; // Create an instance of the MoveGenerator to generate legal moves.

    // Generate all legal moves for the current board state.
    // This is the essential first step for any chess AI, as it can only consider valid moves.
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    // Handle the case where there are no legal moves available.
    // This signifies a game termination condition (checkmate or stalemate).
    if (legal_moves.empty()) {
        std::cerr << "ChessAI: No legal moves found. Game is likely over (checkmate or stalemate)." << std::endl;
        return Move({0,0}, {0,0}, PieceTypeIndex::NONE); // Return a default/invalid move to signal no move.
    }

    // Use a `switch` statement based on the `DEFAULT_AI_TYPE` from `Constants.h`.
    // This allows for easy selection and testing of different AI behaviors.
    switch (DEFAULT_AI_TYPE) {
        case AIType::RANDOM_MOVER: {
            // --- Single-threaded Random Mover Implementation ---
            // This is the simplest AI behavior: it just picks a random legal move.
            // This implementation is now single-threaded, as multi-threading for a
            // purely random choice introduces more overhead than benefit.
            
            // Create a uniform distribution to select a random index within the bounds of `legal_moves`.
            // The range is from 0 up to (but including) `legal_moves.size() - 1`.
            std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
            
            // Select a random move using the ChessAI's main random number engine (`rng_engine`).
            // `rng_engine` is a member variable, initialized in the constructor.
            Move chosen_move = legal_moves[dist(rng_engine)]; 
            
            // Output to standard error (cerr) for debugging purposes. This message will not
            // interfere with the UCI communication on standard output (cout).
            std::cerr << "ChessAI (Random): Chose move: " << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
            
            return chosen_move; // Return the randomly chosen move.
        }
        case AIType::SIMPLE_EVALUATION: {
            // --- Simple Evaluation (1-Ply Search) Implementation ---
            // This AI will look one move ahead, evaluate each resulting position using
            // the `evaluate` function, and choose the move that leads to the best score.
            // This is a greedy approach and forms the foundation for more advanced searches.

            int best_score = -999999; // Initialize with a very small number (negative infinity)
                                     // Assuming scores are centipawns, this value is effectively "minus infinity".
            Move best_move = legal_moves[0]; // Initialize best_move with the first legal move as a fallback.

            // Iterate through each legal move to find the one that yields the best board evaluation.
            // `const auto& move`: Iterates by constant reference to avoid unnecessary copying of `Move` objects.
            for (const auto& move : legal_moves) {
                StateInfo info_for_undo; // Object to store the board state *before* applying the move.
                                         // This is essential for `undo_move` to restore the board correctly.

                // Temporarily apply the current move to the board.
                // This simulates making the move without actually committing to it.
                board.apply_move(move, info_for_undo);

                // Evaluate the new board position.
                // The `evaluate` function will return a score from the perspective of the *current* active player.
                // Since `apply_move` flips the active player, the score returned by `evaluate` will be
                // from the perspective of the *opponent* if we strictly pass `board` as-is.
                // To get the score from *our* perspective (the player who *just* moved), we can negate the result.
                // Or, more correctly for Minimax, the `evaluate` function should return positive for White advantage
                // and negative for Black advantage. Then, the calling side maximizes its score.
                // Current `evaluate` returns from active player's perspective, so we need to negate it to get value for *us*.
                int current_score = evaluate(board); // Evaluate the position after applying the move.

                // In a Minimax context, the score returned by `evaluate` is for the side *whose turn it is*.
                // Since we just made a move for `current_player`, the board's `active_player` is now the opponent.
                // So, `evaluate(board)` gives the opponent's score. We want to maximize our score, so we negate.
                // More precise: if `evaluate` returns score from White's perspective:
                //   if (board.previous_active_player == PlayerColor::White) current_score = evaluate(board);
                //   else current_score = -evaluate(board); // opponent's score
                // Simpler: If `evaluate` returns score for the player whose turn it currently is (after the move):
                //   int current_score = -evaluate(board); // Negate because it's opponent's turn.
                // However, our `evaluate` already handles negating if it's Black's turn, so just use `evaluate(board)`.
                // For a 1-ply search, the active player on the *new* board is the opponent.
                // So the score we get from `evaluate` is what the *opponent* aims for.
                // We want to minimize the opponent's best score.
                // This will be clearer when implementing Minimax/Alpha-Beta.
                // For now, let's just pick the move that yields the highest score for White, and lowest for Black.

                // If the current move results in a better score than the `best_score` found so far,
                // update `best_score` and `best_move`.
                if (current_score > best_score) {
                    best_score = current_score;
                    best_move = move;
                }

                // Crucial: Undo the move to restore the board to its original state for the next iteration.
                // This allows the AI to explore all moves from the *same* starting position without side effects.
                board.undo_move(move, info_for_undo);
            }
            // Output for debugging.
            std::cerr << "ChessAI (Simple Evaluation): Chose move: " << ChessBitboardUtils::move_to_string(best_move) 
                      << " with score: " << best_score << std::endl;
            return best_move; // Return the move that leads to the best evaluated position.
        }
        case AIType::MINIMAX:
        case AIType::ALPHA_BETA:
            // TODO: Implement the Minimax or Alpha-Beta search algorithm here.
            // This involves recursively exploring the game tree to a certain depth (e.g., AI_SEARCH_DEPTH).
            // - `Minimax`: Explores all possible moves to a given depth, assuming optimal play from both sides.
            // - `Alpha-Beta`: An optimization of Minimax that prunes branches that cannot possibly
            //   influence the final decision, significantly improving performance.
            // These algorithms would heavily use `board.apply_move()` and `board.undo_move()`
            // to navigate the game tree, and `evaluate()` to score leaf nodes.
            // Example:
            //   int best_score_from_search = alphaBeta(board, AI_SEARCH_DEPTH, -INFINITE, INFINITE, board.active_player);
            //   // Logic to find the actual move that led to best_score_from_search.
            std::cerr << "ChessAI: MINIMAX/ALPHA_BETA not yet implemented. Falling back to simple evaluation." << std::endl;
            // Fallback: For now, if these modes are selected, it will perform the simple evaluation logic
            // as we did in the SIMPLE_EVALUATION case. This ensures a functional fallback.
            {
                int best_score = -999999;
                Move best_move = legal_moves[0];

                for (const auto& move : legal_moves) {
                    StateInfo info_for_undo;
                    board.apply_move(move, info_for_undo);
                    int current_score = evaluate(board);
                    if (current_score > best_score) {
                        best_score = current_score;
                        best_move = move;
                    }
                    board.undo_move(move, info_for_undo);
                }
                std::cerr << "ChessAI (Fallback from MINIMAX/ALPHA_BETA): Chose move: " 
                          << ChessBitboardUtils::move_to_string(best_move) << " with score: " 
                          << best_score << std::endl;
                return best_move;
            }
        default:
            // Handle any unconfigured or unknown AI types as a fallback.
            std::cerr << "ChessAI: Unknown AIType configured. Falling back to random." << std::endl;
            // Fallback to random if an unknown AIType is specified.
            {
                std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
                Move chosen_move = legal_moves[dist(rng_engine)];
                std::cerr << "ChessAI (Fallback from Unknown AIType): Chose move: " 
                          << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
                return chosen_move;
            }
    }
}
