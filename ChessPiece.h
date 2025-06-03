#ifndef CHESS_PIECE_H
#define CHESS_PIECE_H

#include <SDL.h>

struct ChessPiece {
    SDL_Texture* texture;
    SDL_Rect rect;
    int index;
    int cost;
};

#endif // CHESS_PIECE_H
