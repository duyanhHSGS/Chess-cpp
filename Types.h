#ifndef TYPES_H
#define TYPES_H

#include <cstdint> // For uint64_t and uint8_t if needed elsewhere, and for common types.

// Represents the color of a player.
enum class PlayerColor {
    White = 0, // Changed from 1 to 0 for 0-based array indexing consistency
    Black = 1  // Changed from -1 to 1 for 0-based array indexing consistency
};

// Represents the type of a chess piece (regardless of color).
// Values are ordered from 0-5 to facilitate indexing into Zobrist hash arrays
// and other piece-related lookups.
enum class PieceTypeIndex {
    PAWN = 0,    // White PAWN maps to index 0, Black PAWN maps to index 6
    KNIGHT = 1,  // White KNIGHT maps to index 1, Black KNIGHT maps to index 7
    BISHOP = 2,  // White BISHOP maps to index 2, Black BISHOP maps to index 8
    ROOK = 3,    // White ROOK maps to index 3, Black ROOK maps to index 9
    QUEEN = 4,   // White QUEEN maps to index 4, Black QUEEN maps to index 10
    KING = 5,    // White KING maps to index 5, Black KING maps to index 11
    NONE = -1    // Used to indicate no piece or an invalid piece type.
};

// Represents the current status of the game.
enum class GameStatus {
    ONGOING,                // Game is still in progress.
    CHECKMATE_WHITE_WINS,   // White has checkmated Black.
    CHECKMATE_BLACK_WINS,   // Black has checkmated White.
    STALEMATE,              // Game is a draw due to stalemate (no legal moves, but not in check).
    DRAW_FIFTY_MOVE,        // Game is a draw due to the 50-move rule (100 halfmoves without pawn move or capture).
    DRAW_THREEFOLD_REPETITION // Game is a draw due to threefold repetition of position.
};

// Represents a point on the chessboard using file (x) and rank (y) coordinates.
// x: file (0-7, where 0='a', 7='h')
// y: rank (0-7, where 0='1', 7='8')
struct GamePoint {
    uint8_t x; // File (column)
    uint8_t y; // Rank (row)
};

#endif // TYPES_H
