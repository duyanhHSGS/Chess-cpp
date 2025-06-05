#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <vector>
#include <limits>
#include <algorithm>
#include <tuple>
#include <cstdint> // For uint64_t

#include "Constants.h"
#include "ChessPiece.h" // Includes GamePoint and GameRect

class GameManager;

// Transposition Table Entry Type
enum NodeType {
    EXACT,      // Exact evaluation for the position
    ALPHA,      // Alpha cutoff (value is a lower bound)
    BETA        // Beta cutoff (value is an upper bound)
};

// Transposition Table Entry Structure
struct TTEntry {
    uint64_t hash;      // Zobrist hash of the position
    int value;          // Stored evaluation score
    int depth;          // Search depth at which this entry was stored
    NodeType type;      // Type of node (Exact, Alpha, Beta)
    // Add a 'move' field if you want to store the best move found for this position
};

class ChessAI {
public:
    ChessAI();

    GamePoint getBestMove(GameManager* gameManager, bool isMaximizingPlayer);

    int getEvaluation(GameManager* gameManager) const;

    // New function to check if a hash is in the transposition table
    bool hasPositionInTable(uint64_t hash) const;

private:
    // Transposition Table
    std::vector<TTEntry> transpositionTable;

    // Transposition Table operations
    TTEntry* probeHash(uint64_t hash, int depth, int alpha, int beta);
    void storeHash(uint64_t hash, int value, int depth, NodeType type);

    int minimax(GameManager* gameManager, int depth, int alpha, int beta, bool isMaximizingPlayer);

    int quiescenceSearch(GameManager* gameManager, int alpha, int beta, bool isMaximizingPlayer);

    std::vector<std::tuple<int, GamePoint, GamePoint>> generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer);

    void applyMove(GameManager* gameManager, int pieceIdx, GamePoint oldPos, GamePoint newPos);

    void undoMove(GameManager* gameManager);

    bool isEndgame(GameManager* gameManager) const;

    static const int pawnPST[8][8];
    static const int knightPST[8][8];
    static const int bishopPST[8][8];
    static const int rookPST[8][8];
    static const int queenPST[8][8];
    static const int kingPST_middlegame[8][8];
    static const int kingPST_endgame[8][8];
};

#endif // CHESS_AI_H
