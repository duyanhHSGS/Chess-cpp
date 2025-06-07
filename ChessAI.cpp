#include "ChessAI.h"      // Include the header for ChessAI itself
#include "MoveGenerator.h" // Needed to generate legal moves
#include "Constants.h"    // To access AI_SEARCH_DEPTH, DEFAULT_AI_TYPE, etc.
#include "ChessBitboardUtils.h" // For move_to_string, useful for debugging (though not debug prints now)
#include <random>         // For std::random_device, std::mt19937_64, std::uniform_int_distribution
#include <iostream>       // For std::cerr in case of errors (e.g., no legal moves)

// Constructor for ChessAI.
// Initializes the random number generator for the placeholder `findBestMove`.
ChessAI::ChessAI() : rng_engine(std::random_device{}()) {
    // In a real engine, this is where you would:
    // - Initialize transposition tables
    // - Load opening books
    // - Set up any persistent data structures for the search algorithm
}

// Finds and returns the best legal move for the current active player on the given board.
// For now, this is a placeholder that picks a random legal move.
// In the future, this will implement the actual search algorithm (e.g., Alpha-Beta).
Move ChessAI::findBestMove(ChessBoard& board) {
    MoveGenerator move_gen; // Create an instance of the MoveGenerator.

    // Generate all legal moves for the current board state.
    // This is a crucial step as the AI can only choose from valid moves.
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    // Placeholder Logic: Select a random legal move.
    if (legal_moves.empty()) {
        // If there are no legal moves, the game is over (checkmate or stalemate).
        // Returning a default-constructed move (or a specific "null" move) is typical.
        std::cerr << "ChessAI: No legal moves found. Game is likely over." << std::endl;
        return Move({0,0}, {0,0}, PieceTypeIndex::NONE); // Return a default/invalid move
    }

    // Use the AIType from Constants.h to decide behavior.
    // In a more advanced setup, this AIType might be passed dynamically
    // to the ChessAI instance (e.g., via a constructor parameter or a setter).
    switch (DEFAULT_AI_TYPE) {
        case AIType::RANDOM_MOVER: {
            // Use a uniform distribution to pick a random index from the vector of legal moves.
            std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
            size_t random_index = dist(rng_engine); // Get a random index.
            Move chosen_move = legal_moves[random_index]; // Get the move at that index.
            std::cerr << "ChessAI (Random): Chose move: " << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
            return chosen_move;
        }
        case AIType::SIMPLE_EVALUATION:
            // TODO: Implement simple evaluation and shallow search here
            std::cerr << "ChessAI: SIMPLE_EVALUATION not yet implemented. Falling back to random." << std::endl;
            // Fallback to random if not implemented, or return an error/default.
            return legal_moves[0]; // For now, just return the first legal move.
        case AIType::MINIMAX:
        case AIType::ALPHA_BETA:
            // TODO: Implement actual search algorithms here
            std::cerr << "ChessAI: MINIMAX/ALPHA_BETA not yet implemented. Falling back to random." << std::endl;
            // Fallback to random if not implemented, or return an error/default.
            return legal_moves[0]; // For now, just return the first legal move.
        default:
            std::cerr << "ChessAI: Unknown AIType. Falling back to random." << std::endl;
            return legal_moves[0]; // For now, just return the first legal move.
    }
}
