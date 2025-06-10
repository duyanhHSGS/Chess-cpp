#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint> // For uint8_t, uint64_t
#include <array>   // For std::array for FILE_MASKS_ARRAY

// ============================================================================
// --- Customizable AI & Evaluation Constants (Tuning Parameters) ---
// ============================================================================

// Define the search depth for the Alpha-Beta pruning algorithm.
// Higher depth means stronger AI but more computation time.
constexpr uint64_t AI_SEARCH_DEPTH = 5; 

// --- Piece Values (can be adjusted to emphasize/de-emphasize pieces) ---
// These values represent the relative strength of each piece.
// Adjusting these will influence how Carolyna values material.
constexpr int PAWN_VALUE   = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE   = 500;
constexpr int QUEEN_VALUE  = 900;
constexpr int KING_VALUE   = 20000; // King value is typically symbolic and very high (mate value)

// --- Mobility Bonus ---
// Bonus points per square a piece controls.
// Higher values encourage pieces to be more active and control more squares.
constexpr int MOBILITY_BONUS_PER_SQUARE = 5;

// --- King Safety Bonuses/Penalties ---
// Bonus awarded for completing castling (usually once per side).
// Significant bonus to encourage king safety in the opening/middlegame.
constexpr int CASTLING_BONUS_KINGSIDE   = 100; // Bonus for kingside castling
constexpr int CASTLING_BONUS_QUEENSIDE  = 100; // Bonus for queenside castling

// Penalties for pawn structures around the king (pawn shield).
// Missing pawns in the shield weaken the king's defense.
constexpr int PAWN_SHIELD_MISSING_PAWN_PENALTY = 20;
// Penalty if pawns in the shield have advanced too far, potentially exposing the king.
constexpr int PAWN_SHIELD_ADVANCED_PAWN_PENALTY = 10;

// Penalties for open or semi-open files near the king.
// Open files can be dangerous as they allow rooks and queens to attack the king directly.
constexpr int OPEN_FILE_FULL_OPEN_PENALTY = 20; // File with no friendly or enemy pawns
constexpr int OPEN_FILE_SEMI_OPEN_PENALTY = 10;  // File with friendly pawns but no enemy pawns (enemy can use it)

// --- Pawn Structure Penalties/Bonuses ---
// Penalty for isolated pawns (pawns with no friendly pawns on adjacent files).
// Isolated pawns can be weak and harder to defend.
constexpr int ISOLATED_PAWN_PENALTY = 20;
// Penalty for doubled pawns (two or more pawns on the same file).
// Doubled pawns can restrict mobility and be less effective.
constexpr int DOUBLED_PAWN_PENALTY = 15;
// Base bonus for passed pawns (pawns with no enemy pawns in front or on adjacent files).
// Passed pawns are valuable as they can promote.
constexpr int PASSED_PAWN_BASE_BONUS = 30;
// Additional bonus for passed pawns based on how many ranks they have advanced.
// The closer to promotion, the more valuable they become.
constexpr int PASSED_PAWN_RANK_BONUS_FACTOR = 15;
// Bonus for connected pawns (pawns supporting each other diagonally).
// Connected pawns form strong defensive and offensive chains.
constexpr int CONNECTED_PAWN_BONUS = 15;

// ============================================================================
// --- Non-Customizable / Fixed Constants (System and Utility) ---
// ============================================================================

// Optional: Number of CPU cores/threads to use for parallel search (e.g., for root moves).
// This is a system-level configuration, not an AI tuning parameter.
const uint8_t NUMBER_OF_CORES_USED = 4;

// Define the different types of AI algorithms your engine can use.
// This is an enum for strategic choice, not a numerical tuning knob.
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
// This sets the initial AI mode.
const AIType DEFAULT_AI_TYPE = AIType::ALPHA_BETA;

// File masks as an array for convenient iteration/access in evaluation.
// These are fundamental bitboard definitions and not tuning parameters.
const std::array<uint64_t, 8> FILE_MASKS_ARRAY = {
    0x0101010101010101ULL, // Corresponds to FILE_A (file 0)
    0x0202020202020202ULL, // Corresponds to FILE_B (file 1)
    0x0404040404040404ULL, // Corresponds to FILE_C (file 2)
    0x0808080808080808ULL, // Corresponds to FILE_D (file 3)
    0x1010101010101010ULL, // Corresponds to FILE_E (file 4)
    0x2020202020202020ULL, // Corresponds to FILE_F (file 5)
    0x4040404040404040ULL, // Corresponds to FILE_G (file 6)
    0x8080808080808080ULL  // Corresponds to FILE_H (file 7)
};


#endif // CONSTANTS_H
