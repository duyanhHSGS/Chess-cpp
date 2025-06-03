#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <stack>
#include <vector>
#include <memory>

#include "Constants.h"
#include "ChessPiece.h"

class ChessAI;

class GameManager {
public:
    GameManager();
    ~GameManager();

    void playGame();

    void initializeSDL();
    void cleanUpSDL();

    ChessPiece pieces[32];
    void createPieces();

    SDL_Point possibleMoves[32];
    int possibleMoveCount;

    std::stack<SDL_Point> positionStack;
    std::stack<int> pieceIndexStack;

    void movePiece(int pieceIdx, SDL_Point oldPos, SDL_Point newPos);

    void undoLastMove();

    void addPossibleMove(int x, int y);

    void calculatePossibleMoves(int pieceIdx);

    void findRookMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findBishopMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findKnightMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findKingMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findPawnMoves(int pieceIdx, int x, int y, int grid[9][9]);

    std::unique_ptr<ChessAI> ai;

    const ChessPiece* getPieces() const { return pieces; }

    void getCurrentBoardGrid(int grid[9][9]) const;

    bool isSquareAttacked(int targetX, int targetY, int attackingColor) const;

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_Texture* boardTexture = nullptr;
    SDL_Texture* figuresTexture = nullptr;
    SDL_Texture* positiveMoveTexture = nullptr;
    SDL_Texture* redTexture = nullptr;

    void renderPieces();

    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;

    SDL_Point enPassantTargetSquare;
    int enPassantPawnIndex;

    bool isKingInCheck(int kingColor) const;

    SDL_Point getKingPosition(int kingColor) const;

    SDL_Point getRookPosition(int rookColor, bool isKingside) const;

    bool checkAndAddMove(int pieceIdx, int newX, int newY, int pieceColor);
};

#endif // GAME_MANAGER_H
