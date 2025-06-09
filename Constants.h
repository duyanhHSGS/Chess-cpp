#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint> // For uint8_t, uint64_t

// --- Player and AI Constants ---
// Define the search depth for the Alpha-Beta pruning algorithm.
// Higher depth means stronger AI but more computation time.
const uint64_t AI_SEARCH_DEPTH = 5;

// Optional: Number of CPU cores/threads to use for parallel search (e.g., for root moves).
const uint8_t NUMBER_OF_CORES_USED = 4;

// Define the different types of AI algorithms your engine can use.
// This allows you to switch between AI strategies easily.
enum class AIType {
    RANDOM_MOVER,      // For simple testing or extremely weak play (picks a random legal move)
    SIMPLE_EVALUATION, // A basic evaluation function, perhaps combined with a shallow search
    MINIMAX,           // Full Minimax search (without alpha-beta pruning)
    ALPHA_BETA,        // Alpha-Beta pruning search (standard for strong engines)
    // Future AI types could be added here as your engine evolves, e.g.:
    // NEURAL_NETWORK, // For Stockfish-like or AlphaZero-like AI
    // UCT,            // For Monte Carlo Tree Search
};

// Define the default AI type to use when the engine starts.
// This can later be overridden by UCI options from a chess GUI.
const AIType DEFAULT_AI_TYPE = AIType::ALPHA_BETA;


#endif // CONSTANTS_H
