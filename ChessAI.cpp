#include "ChessAI.h"
#include "GameManager.h"
#include "Constants.h"
#include <tuple>
#include <iostream> // For std::cerr

const int FUTILITY_MARGIN = 10;

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
    transpositionTable.resize(TT_SIZE);
    // Initialize TT entries with default values (e.g., hash = 0, value = 0, depth = -1, type = EXACT)
    for (uint64_t i = 0; i < TT_SIZE; ++i) {
        transpositionTable[i] = {0, 0, -1, EXACT};
    }
}

bool ChessAI::isEndgame(GameManager* gameManager) const {
    const ChessPiece* pieces = gameManager->getPieces();
    int whiteQueens = 0;
    int blackQueens = 0;
    int whiteMinorPieces = 0;
    int blackMinorPieces = 0;

    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;

        int pieceType = std::abs(pieces[i].index);
        int pieceColor = pieces[i].index > 0 ? 1 : -1;

        if (pieceType == 4) {
            if (pieceColor == 1) whiteQueens++;
            else blackQueens++;
        } else if (pieceType == 2 || pieceType == 3) {
            if (pieceColor == 1) whiteMinorPieces++;
            else blackMinorPieces++;
        }
    }

    if (whiteQueens == 0 && blackQueens == 0 && (whiteMinorPieces + blackMinorPieces) < 3) {
        return true;
    }

    if ((whiteQueens + blackQueens) == 1 && (whiteMinorPieces + blackMinorPieces) < 2) {
        return true;
    }

    return false;
}

int ChessAI::getEvaluation(GameManager* gameManager) const {
    int evaluation = 0;
    const ChessPiece* pieces = gameManager->getPieces();

    const int (*currentKingPST)[8];
    if (isEndgame(gameManager)) {
        currentKingPST = kingPST_endgame;
    } else {
        currentKingPST = kingPST_middlegame;
    }


    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
            evaluation += pieces[i].cost;

            int pieceGridX = (pieces[i].rect.x - OFFSET.x) / TILE_SIZE;
            int pieceGridY = (pieces[i].rect.y - OFFSET.y) / TILE_SIZE;
            int pieceType = std::abs(pieces[i].index);
            int pieceColor = pieces[i].index > 0 ? 1 : -1;

            int positionalValue = 0;
            int row = pieceGridY;
            int col = pieceGridX;

            if (pieceColor == -1) {
                row = 7 - row;
            }

            switch (pieceType) {
                case 6:
                    positionalValue = pawnPST[row][col];
                    break;
                case 2:
                    positionalValue = knightPST[row][col];
                    break;
                case 3:
                    positionalValue = bishopPST[row][col];
                    break;
                case 1:
                    positionalValue = rookPST[row][col];
                    break;
                case 4:
                    positionalValue = queenPST[row][col];
                    break;
                case 5:
                    positionalValue = currentKingPST[row][col];
                    break;
                default:
                    positionalValue = 0;
                    break;
            }
            
            evaluation += (pieceColor * positionalValue);
        }
    }
    return evaluation;
}

TTEntry* ChessAI::probeHash(uint64_t hash, int depth, int alpha, int beta) {
    uint64_t index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];

    if (entry->hash == hash) {
        // Hash hit! Check if the stored depth is sufficient
        if (entry->depth >= depth) {
            // Check node type to see if we can use the value directly
            if (entry->type == EXACT) {
                std::cerr << "INFO: Hash hit (EXACT) for depth " << depth << " at hash " << hash << std::endl;
                return entry;
            }
            if (entry->type == ALPHA && entry->value >= beta) {
                std::cerr << "INFO: Hash hit (ALPHA cutoff) for depth " << depth << " at hash " << hash << std::endl;
                return entry;
            }
            if (entry->type == BETA && entry->value <= alpha) {
                std::cerr << "INFO: Hash hit (BETA cutoff) for depth " << depth << " at hash " << hash << std::endl;
                return entry;
            }
        }
    }
    return nullptr; // No useful hash hit
}

void ChessAI::storeHash(uint64_t hash, int value, int depth, NodeType type) {
    uint64_t index = hash % TT_SIZE;
    TTEntry* entry = &transpositionTable[index];

    // Simple replacement strategy: always replace if deeper, or if same depth and exact
    if (entry->depth < depth || (entry->depth == depth && type == EXACT)) {
        entry->hash = hash;
        entry->value = value;
        entry->depth = depth;
        entry->type = type;
    }
}

bool ChessAI::hasPositionInTable(uint64_t hash) const {
    uint64_t index = hash % TT_SIZE;
    const TTEntry& entry = transpositionTable[index];
    return entry.hash == hash;
}


GamePoint ChessAI::getBestMove(GameManager* gameManager, bool isMaximizingPlayer) {
    int bestValue = isMaximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    GamePoint bestNewPos = {-1, -1};
    int bestPieceIdx = -1;
    GamePoint bestOldPos = {-1, -1};

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;

    int currentSearchDepth;
    if (isEndgame(gameManager)) {
        currentSearchDepth = AI_ENDGAME_SEARCH_DEPTH;
    } else {
        currentSearchDepth = AI_SEARCH_DEPTH;
    }

    std::vector<std::tuple<int, GamePoint, GamePoint>> moves = generatePseudoLegalMoves(gameManager, currentPlayer);

    // Sort moves for better alpha-beta pruning (e.g., by estimated value or captures first)
    // For now, no sorting, but this is a common optimization.

    for (const auto& move : moves) {
        int pieceIdx = std::get<0>(move);
        GamePoint oldPos = std::get<1>(move);
        GamePoint newPos = std::get<2>(move);

        applyMove(gameManager, pieceIdx, oldPos, newPos);

        int value = minimax(gameManager, currentSearchDepth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), !isMaximizingPlayer);

        undoMove(gameManager);

        if (isMaximizingPlayer) {
            if (value > bestValue) {
                bestValue = value;
                bestPieceIdx = pieceIdx;
                bestOldPos = oldPos;
                bestNewPos = newPos;
            }
        } else {
            if (value < bestValue) {
                bestValue = value;
                bestPieceIdx = pieceIdx;
                bestOldPos = oldPos;
                bestNewPos = newPos;
            }
        }
    }

    // Only push if a valid move was found
    if (bestPieceIdx != -1) {
        gameManager->positionStack.push(bestOldPos);
        gameManager->pieceIndexStack.push(bestPieceIdx);
    } else {
        // This case should ideally not happen if generatePseudoLegalMoves finds legal moves.
        // It indicates an error or a position with no legal moves (stalemate/checkmate not handled correctly yet).
        std::cerr << "WARNING: getBestMove found no legal moves!" << std::endl;
        return {-1, -1}; // Return an invalid move
    }

    return bestNewPos;
}

int ChessAI::minimax(GameManager* gameManager, int depth, int alpha, int beta, bool isMaximizingPlayer) {
    // Probe transposition table
    TTEntry* ttEntry = probeHash(gameManager->currentZobristHash, depth, alpha, beta);
    if (ttEntry != nullptr) {
        return ttEntry->value;
    }

    if (depth == 0) {
        return quiescenceSearch(gameManager, alpha, beta, isMaximizingPlayer);
    }

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;
    std::vector<std::tuple<int, GamePoint, GamePoint>> moves = generatePseudoLegalMoves(gameManager, currentPlayer);

    if (moves.empty()) {
        if (gameManager->isKingInCheck(static_cast<int>(currentPlayer))) {
            // Checkmate
            return isMaximizingPlayer ? std::numeric_limits<int>::min() + 1000 : std::numeric_limits<int>::max() - 1000;
        } else {
            // Stalemate
            return 0;
        }
    }

    int originalAlpha = alpha; // Store original alpha for TT storage
    NodeType nodeType = EXACT; // Assume exact until proven otherwise

    if (isMaximizingPlayer) {
        int maxEval = std::numeric_limits<int>::min();
        if (depth <= 2 && getEvaluation(gameManager) + FUTILITY_MARGIN <= alpha) { // Futility pruning
            storeHash(gameManager->currentZobristHash, alpha, depth, ALPHA); // Store as lower bound
            return alpha;
        }

        for (const auto& move : moves) {
            int pieceIdx = std::get<0>(move);
            GamePoint oldPos = std::get<1>(move);
            GamePoint newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = minimax(gameManager, depth - 1, alpha, beta, false);
            undoMove(gameManager);

            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                nodeType = BETA; // Beta cutoff occurred
                break;
            }
        }
        // Store result in transposition table
        storeHash(gameManager->currentZobristHash, maxEval, depth, nodeType);
        return maxEval;
    } else {
        int minEval = std::numeric_limits<int>::max();
        if (depth <= 2 && getEvaluation(gameManager) - FUTILITY_MARGIN >= beta) { // Futility pruning
            storeHash(gameManager->currentZobristHash, beta, depth, BETA); // Store as upper bound
            return beta;
        }

        for (const auto& move : moves) {
            int pieceIdx = std::get<0>(move);
            GamePoint oldPos = std::get<1>(move);
            GamePoint newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = minimax(gameManager, depth - 1, alpha, beta, true);
            undoMove(gameManager);

            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) {
                nodeType = ALPHA; // Alpha cutoff occurred
                break;
            }
        }
        // Store result in transposition table
        storeHash(gameManager->currentZobristHash, minEval, depth, nodeType);
        return minEval;
    }
}

int ChessAI::quiescenceSearch(GameManager* gameManager, int alpha, int beta, bool isMaximizingPlayer) {
    int standPat = getEvaluation(gameManager);

    if (isMaximizingPlayer) {
        if (standPat >= beta) {
            return beta;
        }
        alpha = std::max(alpha, standPat);
        // Futility pruning in quiescence search (optional, can be tuned)
        if (standPat + FUTILITY_MARGIN <= alpha) {
            return alpha;
        }
    } else {
        if (standPat <= alpha) {
            return alpha;
        }
        beta = std::min(beta, standPat);
        // Futility pruning in quiescence search (optional, can be tuned)
        if (standPat - FUTILITY_MARGIN >= beta) {
            return beta;
        }
    }

    PlayerColor currentPlayer = isMaximizingPlayer ? PlayerColor::White : PlayerColor::Black;
    std::vector<std::tuple<int, GamePoint, GamePoint>> allMoves = generatePseudoLegalMoves(gameManager, currentPlayer);

    std::vector<std::tuple<int, GamePoint, GamePoint>> tacticalMoves;
    const ChessPiece* pieces = gameManager->getPieces();

    for (const auto& move : allMoves) {
        int pieceIdx = std::get<0>(move);
        GamePoint newPos = std::get<2>(move);

        bool isCapture = false;
        // Check for captures at the new position
        for (int p = 0; p < 32; ++p) {
            if (pieces[p].rect.x == newPos.x && pieces[p].rect.y == newPos.y && p != pieceIdx &&
                ((pieces[p].index > 0 && pieces[pieceIdx].index < 0) || (pieces[p].index < 0 && pieces[pieceIdx].index > 0))) {
                isCapture = true;
                break;
            }
        }
        
        // Also check for pawn promotions as tactical moves
        int pieceType = std::abs(pieces[pieceIdx].index);
        int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;
        bool isPromotion = (pieceType == 6 && (newY_grid == 0 || newY_grid == 7));

        if (isCapture || isPromotion) {
            tacticalMoves.push_back(move);
        }
    }

    if (tacticalMoves.empty()) {
        return standPat;
    }

    // Sort tactical moves for better pruning (e.g., MVV-LVA - Most Valuable Victim, Least Valuable Attacker)
    // For now, no sorting, but this is a common optimization.

    if (isMaximizingPlayer) {
        for (const auto& move : tacticalMoves) {
            int pieceIdx = std::get<0>(move);
            GamePoint oldPos = std::get<1>(move);
            GamePoint newPos = std::get<2>(move);

            applyMove(gameManager, pieceIdx, oldPos, newPos);
            int eval = quiescenceSearch(gameManager, alpha, beta, false);
            undoMove(gameManager);

            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                return beta;
            }
        }
        return alpha;
    } else {
        for (const auto& move : tacticalMoves) {
            int pieceIdx = std::get<0>(move);
            GamePoint oldPos = std::get<1>(move);
            GamePoint newPos = std::get<2>(move);

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

std::vector<std::tuple<int, GamePoint, GamePoint>> ChessAI::generatePseudoLegalMoves(GameManager* gameManager, PlayerColor currentPlayer) {
    std::vector<std::tuple<int, GamePoint, GamePoint>> pseudoLegalMoves;
    const ChessPiece* pieces = gameManager->getPieces();

    // Iterate through all possible piece indices
    for (int i = 0; i < 32; ++i) {
        // Skip if piece is captured or not on board
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;

        // Check if the piece belongs to the current player
        int pieceColor = (pieces[i].index > 0) ? static_cast<int>(PlayerColor::White) : static_cast<int>(PlayerColor::Black);
        if (pieceColor != static_cast<int>(currentPlayer)) {
            continue; // Skip if it's not the current player's piece
        }

        gameManager->calculatePossibleMoves(i);

        GamePoint currentPiecePos = {pieces[i].rect.x, pieces[i].rect.y};
        for (int j = 0; j < gameManager->possibleMoveCount; ++j) {
            pseudoLegalMoves.emplace_back(i, currentPiecePos, gameManager->possibleMoves[j]);
        }
    }
    return pseudoLegalMoves;
}

void ChessAI::applyMove(GameManager* gameManager, int pieceIdx, GamePoint oldPos, GamePoint newPos) {
    gameManager->movePiece(pieceIdx, oldPos, newPos);
}

void ChessAI::undoMove(GameManager* gameManager) {
    gameManager->undoLastMove();
}
