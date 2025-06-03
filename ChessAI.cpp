// ChessAI.cpp
#include "ChessAI.h"
#include "GameManager.h" // Include GameManager to access its public methods
#include "Constants.h"   // Include Constants for AI_SEARCH_DEPTH etc.
#include <tuple>         // For std::tuple
#include <SDL_log.h>     // For SDL_Log

// Piece-Square Tables (PSTs) definitions
// Values are from White's perspective; Black's values will be mirrored (row = 7 - row)
const int ChessAI::pawnPST[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    {5, 5, 10, 25, 25, 10, 5, 5},
    {0, 0, 0, 20, 20, 0, 0, 0},
    {5, -5, -10, 0, 0, -10, -5, 5},
    {5, 10, 10, -20, -20, 10, 10, 5},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

const int ChessAI::knightPST[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0, 0, 0, 0, -20, -40},
    {-30, 0, 10, 15, 15, 10, 0, -30},
    {-30, 5, 15, 20, 20, 15, 5, -30},
    {-30, 0, 15, 20, 20, 15, 0, -30},
    {-30, 5, 10, 15, 15, 10, 5, -30},
    {-40, -20, 0, 5, 5, 0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50}
};

const int ChessAI::bishopPST[8][8] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 10, 10, 5, 0, -10},
    {-10, 5, 5, 10, 10, 5, 5, -10},
    {-10, 0, 10, 10, 10, 10, 0, -10},
    {-10, 10, 10, 10, 10, 10, 10, -10},
    {-10, 5, 0, 0, 0, 0, 5, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20}
};

const int ChessAI::rookPST[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {5, 10, 10, 10, 10, 10, 10, 5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {-5, 0, 0, 0, 0, 0, 0, -5},
    {0, 0, 0, 5, 5, 0, 0, 0}
};

const int ChessAI::queenPST[8][8] = {
    {-20, -10, -10, -5, -5, -10, -10, -20},
    {-10, 0, 0, 0, 0, 0, 0, -10},
    {-10, 0, 5, 5, 5, 5, 0, -10},
    {-5, 0, 5, 5, 5, 5, 0, -5},
    {0, 0, 5, 5, 5, 5, 0, -5},
    {-10, 5, 5, 5, 5, 5, 0, -10},
    {-10, 0, 5, 0, 0, 0, 0, -10},
    {-20, -10, -10, -5, -5, -10, -10, -20}
};

// King PST for middlegame (prioritizes safety)
const int ChessAI::kingPST_middlegame[8][8] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    {20, 20, 0, 0, 0, 0, 20, 20},
    {20, 30, 10, 0, 0, 10, 30, 20}
};

// King PST for endgame (prioritizes centralization and activity)
const int ChessAI::kingPST_endgame[8][8] = {
    {-50, -40, -30, -20, -20, -30, -40, -50},
    {-30, -20, -10, 0, 0, -10, -20, -30},
    {-30, -10, 20, 30, 30, 20, -10, -30},
    {-30, -10, 30, 40, 40, 30, -10, -30},
    {-30, -10, 30, 40, 40, 30, -10, -30},
    {-30, -10, 20, 30, 30, 20, -10, -30},
    {-30, -30, 0, 0, 0, 0, -30, -30},
    {-50, -30, -30, -30, -30, -30, -30, -50}
};


ChessAI::ChessAI() {
    // Constructor implementation (if any initialization is needed)
}

// Determines if the current game state is an endgame based on material
bool ChessAI::isEndgame(GameManager* gameManager) const {
    const ChessPiece* pieces = gameManager->getPieces();
    int whiteQueens = 0;
    int blackQueens = 0;
    int whiteMinorPieces = 0; // Knights and Bishops
    int blackMinorPieces = 0; // Knights and Bishops

    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue; // Skip captured

        int pieceType = std::abs(pieces[i].index);
        int pieceColor = pieces[i].index > 0 ? 1 : -1;

        if (pieceType == 4) { // Queen
            if (pieceColor == 1) whiteQueens++;
            else blackQueens++;
        } else if (pieceType == 2 || pieceType == 3) { // Knight or Bishop
            if (pieceColor == 1) whiteMinorPieces++;
            else blackMinorPieces++;
        }
    }

    // Endgame condition 1: No queens on the board AND total minor pieces < 3
    if (whiteQueens == 0 && blackQueens == 0 && (whiteMinorPieces + blackMinorPieces) < 3) {
        return true;
    }

    // Endgame condition 2: One queen on the board AND total minor pieces < 2
    if ((whiteQueens + blackQueens) == 1 && (whiteMinorPieces + blackMinorPieces) < 2) {
        return true;
    }

    return false;
}

// Evaluates the current board state.
// A positive score favors White, a negative score favors Black.
int ChessAI::getEvaluation(GameManager* gameManager) const {
    int evaluation = 0;
    const ChessPiece* pieces = gameManager->getPieces(); // Access pieces from GameManager

    // Determine which King PST to use
    const int (*currentKingPST)[8];
    if (isEndgame(gameManager)) {
        currentKingPST = kingPST_endgame;
    } else {
        currentKingPST = kingPST_middlegame;
    }


    for (int i = 0; i < 32; ++i) {
        // Only consider pieces that are on the board
        if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
            evaluation += pieces[i].cost; // pieces[i].cost already stores value with sign for color

            int pieceGridX = (pieces[i].rect.x - OFFSET.x) / TILE_SIZE;
            int pieceGridY = (pieces[i].rect.y - OFFSET.y) / TILE_SIZE;
            int pieceType = std::abs(pieces[i].index);
            int pieceColor = pieces[i].index > 0 ? 1 : -1; // 1 for White, -1 for Black

            int positionalValue = 0;
            int row = pieceGridY;
            int col = pieceGridX;

            // Adjust row for Black pieces as PSTs are defined from White's perspective
            if (pieceColor == -1) { // If it's a Black piece
                row = 7 - row; // Mirror the row index
            }

            switch (pieceType) {
                case 6: // Pawn
                    positionalValue = pawnPST[row][col];
                    break;
                case 2: // Knight
                    positionalValue = knightPST[row][col];
                    break;
                case 3: // Bishop
                    positionalValue = bishopPST[row][col];
                    break;
                case 1: // Rook
                    positionalValue = rookPST[row][col];
                    break;
                case 4: // Queen
                    positionalValue = queenPST[row][col];
                    break;
                case 5: // King
                    positionalValue = currentKingPST[row][col]; // Use selected King PST
                    break;
                default:
                    positionalValue = 0; // Should not happen
                    break;
            }
            
            // Add positional value, applying the correct sign for piece color
            evaluation += (pieceColor * positionalValue);
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

    // Determine the search depth based on game phase
    int currentSearchDepth;
    if (isEndgame(gameManager)) {
        currentSearchDepth = AI_ENDGAME_SEARCH_DEPTH;
    } else {
        currentSearchDepth = AI_SEARCH_DEPTH;
    }

    // Generate all possible moves for the current player
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> moves = generatePseudoLegalMoves(gameManager, currentPlayer);

    for (const auto& move : moves) {
        int pieceIdx = std::get<0>(move);
        SDL_Point oldPos = std::get<1>(move);
        SDL_Point newPos = std::get<2>(move);

        // Apply the move (temporarily)
        applyMove(gameManager, pieceIdx, oldPos, newPos);

        // Call minimax for the opponent's turn with the determined depth
        int value = minimax(gameManager, currentSearchDepth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), !isMaximizingPlayer);

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
    // Base case: If depth is 0, enter quiescence search
    if (depth == 0) {
        return quiescenceSearch(gameManager, alpha, beta, isMaximizingPlayer);
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
        // Futility Pruning (Standard Futility Pruning for maximizing)
        // If current alpha is already very high and this branch is unlikely to beat it
        if (depth <= 2 && getEvaluation(gameManager) + FUTILITY_MARGIN <= alpha) {
            return alpha;
        }

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
        // Futility Pruning (Standard Futility Pruning for minimizing)
        // If current beta is already very low and this branch is unlikely to beat it
        if (depth <= 2 && getEvaluation(gameManager) - FUTILITY_MARGIN >= beta) {
            return beta;
        }

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

// Quiescence Search
int ChessAI::quiescenceSearch(GameManager* gameManager, int alpha, int beta, bool isMaximizingPlayer) {
    int standPat = getEvaluation(gameManager);

    if (isMaximizingPlayer) {
        if (standPat >= beta) {
            return beta; // Fail-hard beta cut-off
        }
        alpha = std::max(alpha, standPat);
        // Reverse Futility Pruning for maximizing (if standPat is already good enough, don't search captures)
        if (standPat + FUTILITY_MARGIN <= alpha) { // Check if we can prune even if we gain a little
            return alpha;
        }
    } else { // Minimizing player
        if (standPat <= alpha) {
            return alpha; // Fail-hard alpha cut-off
        }
        beta = std::min(beta, standPat);
        // Reverse Futility Pruning for minimizing (if standPat is already bad enough, don't search captures)
        if (standPat - FUTILITY_MARGIN >= beta) { // Check if we can prune even if we lose a little
            return beta;
        }
    }

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> allMoves = generatePseudoLegalMoves(gameManager, currentPlayer);

    // Filter for only tactical moves (captures for simplicity in this first pass)
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> tacticalMoves;
    const ChessPiece* pieces = gameManager->getPieces();

    for (const auto& move : allMoves) {
        int pieceIdx = std::get<0>(move);
        SDL_Point newPos = std::get<2>(move);

        bool isCapture = false;
        for (int p = 0; p < 32; ++p) {
            if (pieces[p].rect.x == newPos.x && pieces[p].rect.y == newPos.y && p != pieceIdx &&
                ((pieces[p].index > 0 && pieces[pieceIdx].index < 0) || (pieces[p].index < 0 && pieces[pieceIdx].index > 0))) {
                isCapture = true;
                break;
            }
        }
        
        if (isCapture) {
            tacticalMoves.push_back(move);
        }
    }

    // If no tactical moves, return standPat
    if (tacticalMoves.empty()) {
        return standPat;
    }

    if (isMaximizingPlayer) {
        for (const auto& move : tacticalMoves) {
            int pieceIdx = std::get<0>(move);
            SDL_Point oldPos = std::get<1>(move);
            SDL_Point newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = quiescenceSearch(gameManager, alpha, beta, false);
            undoMove(gameManager);

            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                return beta;
            }
        }
        return alpha;
    } else { // Minimizing player
        for (const auto& move : tacticalMoves) {
            int pieceIdx = std::get<0>(move);
            SDL_Point oldPos = std::get<1>(move);
            SDL_Point newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = quiescenceSearch(gameManager, alpha, beta, true);
            undoMove(gameManager);

            beta = std::min(beta, eval);
            if (beta <= alpha) {
                return alpha;
            }
        }
        return beta;
    }
}

// Helper to generate all pseudo-legal moves for the current player
std::vector<std::tuple<int, SDL_Point, SDL_Point>> ChessAI::generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer) {
    std::vector<std::tuple<int, SDL_Point, SDL_Point>> pseudoLegalMoves;
    const ChessPiece* pieces = gameManager->getPieces();

    int startIdx = (currentPlayer == PlayerColor::White) ? 16 : 0;
    int endIdx = (currentPlayer == PlayerColor::White) ? 32 : 16;

    for (int i = startIdx; i < endIdx; ++i) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;

        gameManager->calculatePossibleMoves(i);

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
