// ChessAI.cpp
#include "ChessAI.h"
#include "GameManager.h" // Include GameManager to access its public methods
#include "Constants.h"   // Include Constants for AI_SEARCH_DEPTH etc.
#include <tuple>         // For std::tuple
#include <SDL_log.h>     // For SDL_Log

ChessAI::ChessAI() {
    // Constructor implementation (if any initialization is needed)
}

// Evaluates the current board state.
// A positive score favors White, a negative score favors Black.
int ChessAI::getEvaluation(GameManager* gameManager) const {
    int evaluation = 0;
    const ChessPiece* pieces = gameManager->getPieces(); // Access pieces from GameManager

    for (int i = 0; i < 32; ++i) {
        // Only consider pieces that are on the board
        if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
            evaluation += pieces[i].cost; // pieces[i].cost already stores value with sign for color
        }
    }
    return evaluation;
}

// Main function to get the best move using Alpha-Beta Pruning
SDL_Point ChessAI::getBestMove(GameManager* gameManager, bool isMaximizingPlayer) {
    int bestValue = isMaximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    SDL_Point bestNewPos = {-1, -1};
    int bestPieceIdx = -1;
    SDL_Point bestOldPos = {-1, -1};

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;

    // Generate all possible moves for the current player
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> moves = generatePseudoLegalMoves(gameManager, currentPlayer);

    for (const auto& move : moves) {
        int pieceIdx = std::get<0>(move);
        SDL_Point oldPos = std::get<1>(move);
        SDL_Point newPos = std::get<2>(move);

        // Apply the move (temporarily)
        applyMove(gameManager, pieceIdx, oldPos, newPos);

        // Call minimax for the opponent's turn
        int value = minimax(gameManager, AI_SEARCH_DEPTH - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), !isMaximizingPlayer);

        // Undo the move
        undoMove(gameManager);

        if (isMaximizingPlayer) {
            if (value > bestValue) {
                bestValue = value;
                bestPieceIdx = pieceIdx;
                bestOldPos = oldPos;
                bestNewPos = newPos;
            }
        } else { // Minimizing player
            if (value < bestValue) {
                bestValue = value;
                bestPieceIdx = pieceIdx;
                bestOldPos = oldPos;
                bestNewPos = newPos;
            }
        }
    }

    // Push the chosen move's piece index and old position onto the stack
    // so GameManager::playGame can retrieve them.
    // This is a specific requirement due to how GameManager::playGame processes AI moves.
    gameManager->positionStack.push(bestOldPos);
    gameManager->pieceIndexStack.push(bestPieceIdx);

    return bestNewPos;
}

// Alpha-Beta Pruning algorithm
int ChessAI::minimax(GameManager* gameManager, int depth, int alpha, int beta, bool isMaximizingPlayer) {
    if (depth == 0) {
        return getEvaluation(gameManager);
    }

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> moves = generatePseudoLegalMoves(gameManager, currentPlayer);

    if (moves.empty()) {
        // No legal moves: check for checkmate or stalemate
        if (gameManager->isKingInCheck(static_cast<int>(currentPlayer))) {
            // Checkmate: current player's king is in check and has no legal moves
            return isMaximizingPlayer ? std::numeric_limits<int>::min() + 1000 : std::numeric_limits<int>::max() - 1000;
        } else {
            // Stalemate: current player's king is not in check but has no legal moves
            return 0; // Draw
        }
    }


    if (isMaximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto& move : moves) {
            int pieceIdx = std::get<0>(move);
            SDL_Point oldPos = std::get<1>(move);
            SDL_Point newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = minimax(gameManager, depth - 1, alpha, beta, false);
            undoMove(gameManager);

            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break; // Beta cut-off
            }
        }
        return maxEval;
    } else { // Minimizing player
        int minEval = std::numeric_limits<int>::max();
        for (const auto& move : moves) {
            int pieceIdx = std::get<0>(move);
            SDL_Point oldPos = std::get<1>(move);
            SDL_Point newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = minimax(gameManager, depth - 1, alpha, beta, true);
            undoMove(gameManager);

            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                break; // Alpha cut-off
            }
        }
        return minEval;
    }
}

// Helper to generate all pseudo-legal moves for the current player
std::vector<std::tuple<int, SDL_Point, SDL_Point>> ChessAI::generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer) {
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> pseudoLegalMoves;
    const ChessPiece* pieces = gameManager->getPieces();

    int startIdx = (currentPlayer == PlayerColor::White) ? 16 : 0;
    int endIdx = (currentPlayer == PlayerColor::White) ? 32 : 16;

    for (int i = startIdx; i < endIdx; ++i) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue; // Skip captured pieces

        // Temporarily calculate possible moves for this piece
        gameManager->calculatePossibleMoves(i);

        // Add calculated moves to our list
        SDL_Point currentPiecePos = {pieces[i].rect.x, pieces[i].rect.y};
        for (int j = 0; j < gameManager->possibleMoveCount; ++j) {
            pseudoLegalMoves.emplace_back(i, currentPiecePos, gameManager->possibleMoves[j]);
        }
    }
    return pseudoLegalMoves;
}

// Helper to apply a move (for simulation)
void ChessAI::applyMove(GameManager* gameManager, int pieceIdx, SDL_Point oldPos, SDL_Point newPos) {
    gameManager->movePiece(pieceIdx, oldPos, newPos);
}

// Helper to undo a move (for backtracking in minimax)
void ChessAI::undoMove(GameManager* gameManager) {
    gameManager->undoLastMove();
}
