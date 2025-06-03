// ChessAI.cpp
#include "ChessAI.h"
#include "GameManager.h" // Needed to access GameManager's public methods and state
#include "Constants.h"    // Needed for AI_SEARCH_DEPTH, PlayerColor
#include "ChessPiece.h"   // Needed for ChessPiece struct

#include <algorithm> // For std::min, std::max
#include <limits>    // For std::numeric_limits

// --- Piece-Square Tables (PSTs) for positional evaluation ---
// These tables represent the value of a piece on each square.
// Values are relative to the piece's base material value.
// Higher values mean better position.
// For White pieces, 0,0 is bottom-left (a1). For Black, it's mirrored.

// Pawn PST (for White pawns)
const int ChessAI::PAWN_PST[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5, 5, 10, 25, 25, 10, 5, 5},
    {0, 0, 0, 20, 20, 0, 0, 0},
    {5, -5, -10, 0, 0, -10, -5, 5},
    {5, 10, 10, -20, -20, 10, 10, 5},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

// Knight PST (for White knights) - Knights are stronger in the center
const int ChessAI::KNIGHT_PST[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0, 0, 0, 0, -20, -40},
    {-30, 0, 10, 15, 15, 10, 0, -30},
    {-30, 5, 15, 20, 20, 15, 5, -30},
    {-30, 0, 15, 20, 20, 15, 0, -30},
    {-30, 5, 10, 15, 15, 10, 5, -30},
    {-40, -20, 0, 5, 5, 0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}
};

// Bishop PST (for White bishops)
const int ChessAI::BISHOP_PST[8][8] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 10, 10, 5, 0, -10},
    {-10, 5, 5, 10, 10, 5, 5, -10},
    {-10, 0, 10, 10, 10, 10, 0, -10},
    {-10, 10, 10, 10, 10, 10, 10, -10},
    {-10, 5, 0, 0, 0, 0, 5, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20}
};

// Rook PST (for White rooks)
const int ChessAI::ROOK_PST[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {5, 10, 10, 10, 10, 10, 10, 5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {0, 0, 0, 5, 5, 0, 0, 0}
};

// Queen PST (for White queens)
const int ChessAI::QUEEN_PST[8][8] = {
    {-20, -10, -10, -5, -5, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 5, 5, 5, 0, -10},
    {-5, 0, 5, 5, 5, 5, 0, -5},
    {0, 0, 5, 5, 5, 5, 0, -5},
    {-10, 5, 5, 5, 5, 5, 0, -10},
    {-10, 0, 5, 0, 0, 0, 0, -10},
    {-20, -10, -10, -5, -5, -10, -10, -20}
};

// King PST (for White king, different values for endgame vs. opening/middlegame)
// This is a simplified version for opening/middlegame (king safety)
const int ChessAI::KING_PST[8][8] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    {20, 20, 0, 0, 0, 0, 20, 20},
    {20, 30, 10, 0, 0, 10, 30, 20}
};


// Helper function to get the mirrored PST value for black pieces
int ChessAI::getMirroredPSTValue(const int pst[8][8], int x, int y) {
    // Mirror vertically for black pieces
    return pst[7 - y][x];
}

// Evaluates the current state of the board.
// Positive score favors White, negative favors Black.
int ChessAI::evaluateBoard(GameManager* gameManager) {
    int totalCost = 0;
    const ChessPiece* pieces = gameManager->getPieces(); // Get access to the pieces array

    for (int i = 0; i < 32; ++i) {
        // Only evaluate pieces that are on the board
        if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
            totalCost += pieces[i].cost; // Add material value

            // Add positional value using Piece-Square Tables (PSTs)
            int x = (pieces[i].rect.x - OFFSET.x) / TILE_SIZE;
            int y = (pieces[i].rect.y - OFFSET.y) / TILE_SIZE;

            int pieceType = abs(pieces[i].index);
            int positionalValue = 0;

            if (pieces[i].index > 0) { // White piece
                switch (pieceType) {
                    case 6: positionalValue = PAWN_PST[y][x]; break;   // Pawn
                    case 2: positionalValue = KNIGHT_PST[y][x]; break; // Knight
                    case 3: positionalValue = BISHOP_PST[y][x]; break; // Bishop
                    case 1: positionalValue = ROOK_PST[y][x]; break;   // Rook
                    case 4: positionalValue = QUEEN_PST[y][x]; break;  // Queen
                    case 5: positionalValue = KING_PST[y][x]; break;   // King
                }
            } else { // Black piece
                switch (pieceType) {
                    case 6: positionalValue = getMirroredPSTValue(PAWN_PST, x, y); break;
                    case 2: positionalValue = getMirroredPSTValue(KNIGHT_PST, x, y); break;
                    case 3: positionalValue = getMirroredPSTValue(BISHOP_PST, x, y); break;
                    case 1: positionalValue = getMirroredPSTValue(ROOK_PST, x, y); break;
                    case 4: positionalValue = getMirroredPSTValue(QUEEN_PST, x, y); break;
                    case 5: positionalValue = getMirroredPSTValue(KING_PST, x, y); break;
                }
                positionalValue = -positionalValue; // Invert value for black pieces
            }
            totalCost += positionalValue;
        }
    }
    return totalCost;
}

// Implements the Alpha-Beta pruning algorithm.
int ChessAI::alphaBeta(GameManager* gameManager, int depth, bool isMaximizingPlayer, int alpha, int beta) {
    // Base case: If depth is 0 or game is over (e.g., checkmate/stalemate, not implemented here)
    if (depth == 0) {
        return evaluateBoard(gameManager);
    }

    // Temporary storage for possible moves to avoid re-calculating on undo
    SDL_Point possibleMoveTemp[32];
    int currentPossibleMoveCount;

    if (isMaximizingPlayer) { // AI is White (maximizing player)
        int bestMoveValue = std::numeric_limits<int>::min(); // Initialize with negative infinity

        // Iterate through all White pieces (indices 16 to 31)
        for (int pieceIdx = 16; pieceIdx < 32; ++pieceIdx) {
            // Skip captured pieces
            if (gameManager->pieces[pieceIdx].rect.x == -100 && gameManager->pieces[pieceIdx].rect.y == -100) {
                continue;
            }

            // Calculate possible moves for the current piece
            gameManager->calculatePossibleMoves(pieceIdx);
            currentPossibleMoveCount = gameManager->possibleMoveCount;
            gameManager->possibleMoveCount = 0; // Reset for next calculation

            // Copy possible moves to temporary array
            for (int i = 0; i < currentPossibleMoveCount; ++i) {
                possibleMoveTemp[i] = gameManager->possibleMoves[i];
            }

            // Try each possible move
            for (int i = 0; i < currentPossibleMoveCount; ++i) {
                SDL_Point oldPos = {gameManager->pieces[pieceIdx].rect.x, gameManager->pieces[pieceIdx].rect.y};
                SDL_Point newPos = possibleMoveTemp[i];

                gameManager->movePiece(pieceIdx, oldPos, newPos); // Make the move
                int eval = alphaBeta(gameManager, depth - 1, false, alpha, beta); // Recurse for opponent
                gameManager->undoLastMove(); // Undo the move

                bestMoveValue = std::max(bestMoveValue, eval);
                alpha = std::max(alpha, bestMoveValue);

                if (beta <= alpha) { // Alpha-Beta Cutoff
                    return bestMoveValue;
                }
            }
        }
        return bestMoveValue;

    } else { // AI is Black (minimizing player)
        int bestMoveValue = std::numeric_limits<int>::max(); // Initialize with positive infinity

        // Iterate through all Black pieces (indices 0 to 15)
        for (int pieceIdx = 0; pieceIdx < 16; ++pieceIdx) {
            // Skip captured pieces
            if (gameManager->pieces[pieceIdx].rect.x == -100 && gameManager->pieces[pieceIdx].rect.y == -100) {
                continue;
            }

            // Calculate possible moves for the current piece
            gameManager->calculatePossibleMoves(pieceIdx);
            currentPossibleMoveCount = gameManager->possibleMoveCount;
            gameManager->possibleMoveCount = 0; // Reset for next calculation

            // Copy possible moves to temporary array
            for (int i = 0; i < currentPossibleMoveCount; ++i) {
                possibleMoveTemp[i] = gameManager->possibleMoves[i];
            }

            // Try each possible move
            for (int i = 0; i < currentPossibleMoveCount; ++i) {
                SDL_Point oldPos = {gameManager->pieces[pieceIdx].rect.x, gameManager->pieces[pieceIdx].rect.y};
                SDL_Point newPos = possibleMoveTemp[i];

                gameManager->movePiece(pieceIdx, oldPos, newPos); // Make the move
                int eval = alphaBeta(gameManager, depth - 1, true, alpha, beta); // Recurse for opponent
                gameManager->undoLastMove(); // Undo the move

                bestMoveValue = std::min(bestMoveValue, eval);
                beta = std::min(beta, bestMoveValue);

                if (beta <= alpha) { // Alpha-Beta Cutoff
                    return bestMoveValue;
                }
            }
        }
        return bestMoveValue;
    }
}

// Finds and returns the best move for the AI.
SDL_Point ChessAI::getBestMove(GameManager* gameManager, bool isMaximizingPlayer) {
    SDL_Point bestOldPos = {-1, -1};
    SDL_Point bestNewPos = {-1, -1};
    int bestPieceIdx = -1;

    int minMaxScore;
    if (isMaximizingPlayer) { // AI is White
        minMaxScore = std::numeric_limits<int>::min(); // Looking for highest score
    } else { // AI is Black
        minMaxScore = std::numeric_limits<int>::max(); // Looking for lowest score
    }

    // Temporary storage for possible moves
    SDL_Point possibleMoveTemp[32];
    int currentPossibleMoveCount;

    // Determine which pieces the AI controls based on isMaximizingPlayer
    int startIdx = isMaximizingPlayer ? 16 : 0; // White pieces start at 16, Black at 0
    int endIdx = isMaximizingPlayer ? 32 : 16;  // White pieces end at 31, Black at 15

    // Iterate through all AI's pieces
    for (int pieceIdx = startIdx; pieceIdx < endIdx; ++pieceIdx) {
        // Skip captured pieces
        if (gameManager->pieces[pieceIdx].rect.x == -100 && gameManager->pieces[pieceIdx].rect.y == -100) {
            continue;
        }

        // Calculate possible moves for the current piece
        gameManager->calculatePossibleMoves(pieceIdx);
        currentPossibleMoveCount = gameManager->possibleMoveCount;
        gameManager->possibleMoveCount = 0; // Reset for next calculation

        // Copy possible moves to temporary array
        for (int k = 0; k < currentPossibleMoveCount; ++k) {
            possibleMoveTemp[k] = gameManager->possibleMoves[k];
        }

        SDL_Point currentPieceOldPos = {gameManager->pieces[pieceIdx].rect.x, gameManager->pieces[pieceIdx].rect.y};

        // Evaluate each possible move for the current piece
        for (int j = 0; j < currentPossibleMoveCount; ++j) {
            SDL_Point currentMoveNewPos = possibleMoveTemp[j];

            gameManager->movePiece(pieceIdx, currentPieceOldPos, currentMoveNewPos); // Make the move
            int alpha = std::numeric_limits<int>::min();
            int beta = std::numeric_limits<int>::max();
            // Call alphaBeta for the opponent's turn (depth-1)
            int tempScore = alphaBeta(gameManager, AI_SEARCH_DEPTH - 1, !isMaximizingPlayer, alpha, beta);
            gameManager->undoLastMove(); // Undo the move

            // Update best move based on whether AI is maximizing or minimizing
            if (isMaximizingPlayer) {
                if (tempScore > minMaxScore) {
                    minMaxScore = tempScore;
                    bestOldPos = currentPieceOldPos;
                    bestNewPos = currentMoveNewPos;
                    bestPieceIdx = pieceIdx;
                }
            } else { // Minimizing player
                if (tempScore < minMaxScore) {
                    minMaxScore = tempScore;
                    bestOldPos = currentPieceOldPos;
                    bestNewPos = currentMoveNewPos;
                    bestPieceIdx = pieceIdx;
                }
            }
        }
    }

    // Push the chosen move onto the GameManager's stacks for undo functionality
    // This mimics how GameManager::movePiece pushes to the stack.
    // The GameManager::playGame loop will then call movePiece with these values.
    gameManager->positionStack.push(bestOldPos);
    gameManager->pieceIndexStack.push(bestPieceIdx);

    return bestNewPos;
}
