#ifndef TYPES_H
#define TYPES_H

// Enum for player colors (White or Black).
enum class PlayerColor {
    Black = -1,
    White = 1
};

// Represents a square on the board using 0-7 for x and y (grid coordinates).
struct GamePoint {
    int x;
    int y;
};

// Enum class for piece types (Pawn, Knight, etc.).
// Using enum class for stronger type safety and to prevent name collisions.
enum class PieceTypeIndex {
    NONE = -1, // Used to indicate no piece or an invalid piece type.
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
};

#endif // TYPES_H
