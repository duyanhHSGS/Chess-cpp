#ifndef CHESS_PIECE_H
#define CHESS_PIECE_H

struct GamePoint {
    int x;
    int y;
};

struct GameRect {
    int x;
    int y;
    int w;
    int h;
};

struct ChessPiece {
    GameRect rect;
    int index;
    int cost;
};

#endif // CHESS_PIECE_H
