#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <stack>
#include <vector>
#include <memory>
#include <string>

#include "Constants.h"
#include "ChessPiece.h"

class ChessAI;

class GameManager {
public:
    GameManager();
    ~GameManager();

    ChessPiece pieces[32];

    void createPieces();

    GamePoint possibleMoves[32];
    int possibleMoveCount;

    std::stack<GamePoint> positionStack;
    std::stack<int> pieceIndexStack;

    struct GameStateSnapshot {
        GamePoint enPassantTargetSquare;
        int enPassantPawnIndex;
        bool whiteKingMoved;
        bool blackKingMoved;
        bool whiteRookKingsideMoved;
        bool whiteRookQueensideMoved;
        bool blackRookKingsideMoved;
        bool blackRookQueensideMoved;
        int capturedPieceIdx;
        GamePoint capturedPieceOldPos;
        int promotedPawnIdx;
        int originalPawnIndexValue;
        int castlingRookIdx;
        GamePoint castlingRookOldPos;
        GamePoint castlingRookNewPos;
    };
    std::stack<GameStateSnapshot> gameStateSnapshots;

    void movePiece(int pieceIdx, GamePoint oldPos, GamePoint newPos);

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

    bool isKingInCheck(int kingColor) const;

    void setBoardFromFEN(const std::string& fen);
    std::string convertMoveToAlgebraic(int pieceIdx, GamePoint oldPos, GamePoint newPos) const;
    void applyUCIStringMove(const std::string& moveStr, PlayerColor currentPlayer);

private:
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;

    GamePoint enPassantTargetSquare;
    int enPassantPawnIndex;

    GamePoint getKingPosition(int kingColor) const;

    GamePoint getRookPosition(int rookColor, bool isKingside) const;

    bool checkAndAddMove(int pieceIdx, int newX, int newY, int pieceColor);

    char toFile(int col) const;
    int toRank(int row) const;
    GamePoint algebraicToSquare(const std::string& algebraic) const;
    int getPieceIndexAt(GamePoint pixelPos) const;
    int getPieceIndexAtGrid(int row, int col) const;
};

#endif // GAME_MANAGER_H
