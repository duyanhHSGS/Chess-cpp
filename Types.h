#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // For uint8_t

// Enum for player colors
enum class PlayerColor {
    White,
    Black,
    NONE // Used for empty squares or undefined color
};

// Enum for piece types (used for array indexing and move generation)
enum class PieceTypeIndex {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    NONE = 6 // Represents an empty square or no piece
};

// Struct to represent a point on the board (e.g., from_square, to_square)
// x corresponds to file (0-7, 'a'-'h')
// y corresponds to rank (0-7, '1'-'8')
struct GamePoint {
    uint8_t x;
    uint8_t y;
};

// Enum for node types in the transposition table (Alpha-Beta pruning)
enum class NodeType {
    EXACT,       // The score is an exact minimax value
    LOWER_BOUND, // The score is a lower bound (beta cutoff)
    UPPER_BOUND  // The score is an upper bound (alpha cutoff)
};

#endif // TYPES_H
