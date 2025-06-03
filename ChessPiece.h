// ChessPiece.h
#ifndef CHESS_PIECE_H
#define CHESS_PIECE_H

#include <SDL.h> // Required for SDL_Texture and SDL_Rect

// Structure to represent a single chess piece
struct ChessPiece {
    SDL_Texture* texture; // Pointer to the SDL texture for the piece sprite sheet
    SDL_Rect rect;        // Source rectangle on the sprite sheet (x, y, w, h)
                          // Also used as destination rectangle for rendering on board (x, y, w, h)
    int index;            // Type of piece: -1 to -6 for Black, 1 to 6 for White
                          // (1: Rook, 2: Knight, 3: Bishop, 4: Queen, 5: King, 6: Pawn)
    int cost;             // Material value of the piece (e.g., 10 for pawn, 30 for knight/bishop)
};

#endif // CHESS_PIECE_H
