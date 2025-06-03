// Constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL.h> // Required for SDL_Point

// --- Game Board and Display Constants ---
const int TILE_SIZE = 56;
const SDL_Point OFFSET = {28, 28}; // Offset for piece rendering on the board

// Initial board setup
// Positive numbers for White pieces, Negative for Black pieces
// -1: Rook, -2: Knight, -3: Bishop, -4: Queen, -5: King, -6: Pawn
// 1: Rook, 2: Knight, 3: Bishop, 4: Queen, 5: King, 6: Pawn
inline int initial_board[8][8] = {
    {-1, -2, -3, -4, -5, -3, -2, -1}, // Black back rank
    {-6, -6, -6, -6, -6, -6, -6, -6}, // Black pawns
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {6, 6, 6, 6, 6, 6, 6, 6},     // White pawns
    {1, 2, 3, 4, 5, 3, 2, 1}      // White back rank
};

// --- Player and AI Constants ---
enum class PlayerColor {
    White,
    Black
};

// Define FUTILITY_MARGIN for Futility Pruning
// A small value, e.g., representing a pawn's value, to allow for small tactical gains
const int FUTILITY_MARGIN = 10; // Represents 1 pawn unit, fo

// Define which color the human player controls
const PlayerColor HUMAN_PLAYER_COLOR = PlayerColor::White; // Human plays as White

// Define the search depth for the Alpha-Beta pruning algorithm
// Higher depth means stronger AI but more computation time
const int AI_SEARCH_DEPTH = 5;
const int AI_ENDGAME_SEARCH_DEPTH = 7;

#endif // CONSTANTS_H
