#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "ChessPiece.h"

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

#endif // CONSTANTS_H
