#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "ChessPiece.h"
#include <cstdint> // For uint64_t

const int TILE_SIZE = 56;
const GamePoint OFFSET = {28, 28};

inline int initial_board[8][8] = {
    {-1, -2, -3, -4, -5, -3, -2, -1},
    {-6, -6, -6, -6, -6, -6, -6, -6},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {6, 6, 6, 6, 6, 6, 6, 6},
    {1, 2, 3, 4, 5, 3, 2, 1}
};

const int AI_SEARCH_DEPTH = 5;
const int AI_ENDGAME_SEARCH_DEPTH = 7;

enum class PlayerColor {
    White = 1,
    Black = -1
};

const PlayerColor HUMAN_PLAYER_COLOR = PlayerColor::White;

// Zobrist Hashing Constants
// 6 piece types * 2 colors * 64 squares = 12 * 64 = 768
// + 1 for black to move
// + 16 for castling rights (2^4 possibilities)
// + 8 for en passant file (a-h)
// Total unique random keys needed
const int ZOBRIST_PIECE_SQUARE_KEYS_COUNT = 12 * 64; // 6 piece types, 2 colors, 64 squares
const int ZOBRIST_CASTLING_KEYS_COUNT = 16;         // 2^4 = 16 combinations of castling rights
const int ZOBRIST_EN_PASSANT_KEYS_COUNT = 8;        // 8 files for en passant target square

// Transposition Table Size (power of 2 for efficient modulo)
const uint64_t TT_SIZE = 1024 * 1024; // 1 million entries

#endif // CONSTANTS_H
