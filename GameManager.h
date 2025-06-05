#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <stack>
#include <vector>
#include <memory>
#include <string>
#include <cstdint> // For uint64_t

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
        uint64_t prevZobristHash; // Store previous hash for undo
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

    // Zobrist Hashing
    uint64_t currentZobristHash; // Current Zobrist hash of the board

private:
    // Zobrist Keys (initialized once)
    static uint64_t zobristPieceKeys[12][64]; // [piece_type (0-11)][square (0-63)]
    static uint64_t zobristBlackToMoveKey;
    static uint64_t zobristCastlingKeys[16];  // 0-15 for 4 bits of castling rights
    static uint64_t zobristEnPassantKeys[8];  // 0-7 for file of en passant square

    static bool zobristKeysInitialized;
    void initializeZobristKeys();
    void toggleZobristPiece(int pieceIndex, GamePoint pos); // Helper to XOR piece key
    uint64_t calculateZobristHash() const; // Calculates hash from scratch

    // These flags represent if the king/rook *have moved* from their starting squares,
    // which affects castling legality. They are updated by moves and initialized by FEN.
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;

    // This represents the square a pawn landed on if it moved two squares.
    // Its x-coordinate is the file, its y-coordinate is the rank.
    // (-1, -1) if no en passant target.
    GamePoint enPassantTargetSquare;
    // This is the index of the pawn that created the en passant target square.
    // Only valid immediately after a double pawn push. -1 otherwise.
    // NOT directly set by FEN as FEN does not carry this specific info.
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
