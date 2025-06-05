#include "GameManager.h"
#include "ChessAI.h"
#include "Constants.h"
#include "ChessPiece.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cctype>
#include <random> // For std::mt19937 and std::uniform_int_distribution
#include <chrono> // Required for std::chrono::steady_clock

// Initialize static members
uint64_t GameManager::zobristPieceKeys[12][64];
uint64_t GameManager::zobristBlackToMoveKey;
uint64_t GameManager::zobristCastlingKeys[16];
uint64_t GameManager::zobristEnPassantKeys[8];
bool GameManager::zobristKeysInitialized = false;

GameManager::GameManager() :
    possibleMoveCount(0),
    whiteKingMoved(false),
    blackKingMoved(false),
    whiteRookKingsideMoved(false),
    whiteRookQueensideMoved(false),
    blackRookKingsideMoved(false),
    blackRookQueensideMoved(false),
    enPassantTargetSquare({-1, -1}),
    enPassantPawnIndex(-1),
    currentZobristHash(0) // Initialize hash to 0
{
    ai = std::make_unique<ChessAI>();
    if (!zobristKeysInitialized) {
        initializeZobristKeys();
        zobristKeysInitialized = true;
    }
}

GameManager::~GameManager() {
}

void GameManager::initializeZobristKeys() {
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist;

    for (int i = 0; i < 12; ++i) { // 6 piece types * 2 colors
        for (int j = 0; j < 64; ++j) { // 64 squares
            zobristPieceKeys[i][j] = dist(rng);
        }
    }

    zobristBlackToMoveKey = dist(rng);

    for (int i = 0; i < 16; ++i) { // 16 castling combinations (0-15)
        zobristCastlingKeys[i] = dist(rng);
    }

    for (int i = 0; i < 8; ++i) { // 8 files for en passant
        zobristEnPassantKeys[i] = dist(rng);
    }
}

void GameManager::toggleZobristPiece(int pieceIndex, GamePoint pos) {
    int pieceType = std::abs(pieceIndex); // 1-6
    int pieceColor = (pieceIndex > 0) ? 0 : 1; // 0 for white, 1 for black
    int zobristPieceType = (pieceColor * 6) + (pieceType - 1); // 0-11 index for zobristPieceKeys

    int square = (pos.y - OFFSET.y) / TILE_SIZE * 8 + (pos.x - OFFSET.x) / TILE_SIZE;
    currentZobristHash ^= zobristPieceKeys[zobristPieceType][square];
}

uint64_t GameManager::calculateZobristHash() const {
    uint64_t hash = 0;
    int grid[9][9];
    getCurrentBoardGrid(grid);

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int pieceIndex = grid[r][c];
            if (pieceIndex != 0) {
                int pieceType = std::abs(pieceIndex);
                int pieceColor = (pieceIndex > 0) ? 0 : 1; // 0 for white, 1 for black
                int zobristPieceType = (pieceColor * 6) + (pieceType - 1);
                hash ^= zobristPieceKeys[zobristPieceType][r * 8 + c];
            }
        }
    }

    // Side to move is handled in main.cpp's `currentTurn` and its impact on the AI's search.
    // However, for the Zobrist hash calculation itself, the side to move is part of the position's identity.
    // The FEN parsing in `setBoardFromFEN` should correctly set the `currentTurn` in `main.cpp`
    // which in turn will correctly set `isMaximizingPlayer` for the AI search.
    // For the Zobrist key, we assume that if it's black's turn, we XOR the black to move key.
    // This part should reflect the current player after a move is made or after a FEN is loaded.
    // `setBoardFromFEN` currently does NOT take PlayerColor, so this hash calculation is based on the
    // current board state, and the `zobristBlackToMoveKey` is XORed in `movePiece`.
    // When `setBoardFromFEN` is called, `currentZobristHash` is calculated *after* the board and castling/en passant
    // flags are set. The `zobristBlackToMoveKey` needs to be applied if it's currently Black's turn.
    // This is implicitly handled by `main.cpp` passing `currentTurn` to `applyUCIStringMove` and `getBestMove`.
    // For `calculateZobristHash` from scratch, we need to know the actual side to move from the FEN.
    // This will be handled by the FEN parsing in `setBoardFromFEN`.

    // Castling rights
    int castlingRights = 0;
    if (!whiteKingMoved && !whiteRookKingsideMoved) castlingRights |= 0b1000; // K
    if (!whiteKingMoved && !whiteRookQueensideMoved) castlingRights |= 0b0100; // Q
    if (!blackKingMoved && !blackRookKingsideMoved) castlingRights |= 0b0010; // k
    if (!blackKingMoved && !blackRookQueensideMoved) castlingRights |= 0b0001; // q
    hash ^= zobristCastlingKeys[castlingRights];

    // En passant target square
    if (enPassantTargetSquare.x != -1) {
        hash ^= zobristEnPassantKeys[enPassantTargetSquare.x]; // Use grid X, not pixel X
    }

    return hash;
}

void GameManager::createPieces() {
    possibleMoveCount = 0;
    int k = 0;

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            int n = initial_board[i][j];

            if (n == 0) continue;

            pieces[k].index = n;
            pieces[k].rect = {j * TILE_SIZE + OFFSET.x, i * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

            int pieceValue = 0;
            int pieceType = std::abs(pieces[k].index);
            if (pieceType == 1) pieceValue = 50;
            else if (pieceType == 2) pieceValue = 30;
            else if (pieceType == 3) pieceValue = 30;
            else if (pieceType == 4) pieceValue = 90;
            else if (pieceType == 5) pieceValue = 900;
            else if (pieceType == 6) pieceValue = 10;
            pieces[k].cost = pieces[k].index / pieceType * pieceValue;

            k++;
        }
    }
    while(!gameStateSnapshots.empty()) gameStateSnapshots.pop();
    while(!positionStack.empty()) positionStack.pop();
    while(!pieceIndexStack.empty()) pieceIndexStack.pop();

    whiteKingMoved = false;
    blackKingMoved = false;
    whiteRookKingsideMoved = false;
    whiteRookQueensideMoved = false;
    blackRookKingsideMoved = false;
    blackRookQueensideMoved = false;
    enPassantTargetSquare = {-1, -1};
    enPassantPawnIndex = -1;

    currentZobristHash = calculateZobristHash(); // Calculate initial hash
}

void GameManager::movePiece(int pieceIdx, GamePoint oldPos, GamePoint newPos) {
    // Save current state for undo, including current Zobrist hash
    GameStateSnapshot currentSnapshot = {
        enPassantTargetSquare,
        enPassantPawnIndex,
        whiteKingMoved,
        blackKingMoved,
        whiteRookKingsideMoved,
        whiteRookQueensideMoved,
        blackRookKingsideMoved,
        blackRookQueensideMoved,
        -1,
        {-100, -100},
        -1,
        0,
        -1,
        {-100, -100},
        {-100, -100},
        currentZobristHash // Store the hash *before* this move
    };
    gameStateSnapshots.push(currentSnapshot);

    // Update Zobrist hash for side to move (always toggles)
    currentZobristHash ^= zobristBlackToMoveKey;

    int oldX_grid = (oldPos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (oldPos.y - OFFSET.y) / TILE_SIZE;
    int newX_grid = (newPos.x - OFFSET.x) / TILE_SIZE;
    int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;

    // Store move details for undo
    positionStack.push(oldPos);
    positionStack.push(newPos);
    pieceIndexStack.push(pieceIdx);

    int capturedPieceActualIdx = -1;
    GamePoint capturedPieceActualOldPos = {-100, -100};

    // Zobrist: Remove old castling rights if they are about to change
    int oldCastlingRights = 0;
    if (!whiteKingMoved && !whiteRookKingsideMoved) oldCastlingRights |= 0b1000;
    if (!whiteKingMoved && !whiteRookQueensideMoved) oldCastlingRights |= 0b0100;
    if (!blackKingMoved && !blackRookKingsideMoved) oldCastlingRights |= 0b0010;
    if (!blackKingMoved && !blackRookQueensideMoved) oldCastlingRights |= 0b0001;
    currentZobristHash ^= zobristCastlingKeys[oldCastlingRights];

    // Zobrist: Remove old en passant target square if it exists
    if (enPassantTargetSquare.x != -1) {
        currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
    }

    // Zobrist: Remove piece from old position
    toggleZobristPiece(pieces[pieceIdx].index, oldPos);

    // Handle En Passant Capture
    if (std::abs(pieces[pieceIdx].index) == 6 &&
        std::abs(newX_grid - oldX_grid) == 1 &&
        std::abs(newY_grid - oldY_grid) == 1 &&
        (newPos.x == (currentSnapshot.enPassantTargetSquare.x * TILE_SIZE + OFFSET.x) && newPos.y == (currentSnapshot.enPassantTargetSquare.y * TILE_SIZE + OFFSET.y)) &&
        currentSnapshot.enPassantPawnIndex != -1 &&
        pieces[pieceIdx].index * pieces[currentSnapshot.enPassantPawnIndex].index < 0)
    {
        capturedPieceActualIdx = currentSnapshot.enPassantPawnIndex;
        capturedPieceActualOldPos = {pieces[capturedPieceActualIdx].rect.x, pieces[capturedPieceActualIdx].rect.y};
        pieces[capturedPieceActualIdx].rect.x = -100;
        pieces[capturedPieceActualIdx].rect.y = -100;
        // Zobrist: Remove captured pawn from its square
        toggleZobristPiece(pieces[capturedPieceActualIdx].index, capturedPieceActualOldPos);
    }
    // Handle Normal Capture
    else {
        GameRect newPosRect = {newPos.x, newPos.y, TILE_SIZE, TILE_SIZE};
        for (int i = 0; i < 32; ++i) {
            if (pieces[i].rect.x == newPosRect.x && pieces[i].rect.y == newPosRect.y && i != pieceIdx) {
                if ((pieces[i].index > 0 && pieces[pieceIdx].index < 0) ||
                    (pieces[i].index < 0 && pieces[pieceIdx].index > 0)) {
                    capturedPieceActualIdx = i;
                    capturedPieceActualOldPos = {pieces[i].rect.x, pieces[i].rect.y};
                    pieces[i].rect.x = -100;
                    pieces[i].rect.y = -100;
                    // Zobrist: Remove captured piece from its square
                    toggleZobristPiece(pieces[capturedPieceActualIdx].index, capturedPieceActualOldPos);
                    break;
                }
            }
        }
    }
    positionStack.push(capturedPieceActualOldPos);
    pieceIndexStack.push(capturedPieceActualIdx);


    // Handle Castling Move
    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX_grid - oldX_grid) == 2) {
        int rookIdx = -1;
        GamePoint rookOldPos;
        GamePoint rookNewPos;

        if (newX_grid == 6) { // Kingside castling
            if (pieces[pieceIdx].index == 5) { // White King
                for (int i = 16; i < 32; ++i) { // Search white pieces for rook
                    if (pieces[i].index == 1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 7 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 7) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {7 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
                rookNewPos = {5 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
            } else { // Black King
                for (int i = 0; i < 16; ++i) { // Search black pieces for rook
                    if (pieces[i].index == -1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 7 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 0) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {7 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
                rookNewPos = {5 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
            }
        }
        else if (newX_grid == 2) { // Queenside castling
            if (pieces[pieceIdx].index == 5) { // White King
                for (int i = 16; i < 32; ++i) { // Search white pieces for rook
                    if (pieces[i].index == 1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 0 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 7) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {0 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
                rookNewPos = {3 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
            } else { // Black King
                for (int i = 0; i < 16; ++i) { // Search black pieces for rook
                    if (pieces[i].index == -1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 0 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 0) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {0 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
                rookNewPos = {3 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
            }
        }

        if (rookIdx != -1) {
            // Zobrist: Remove rook from old position
            toggleZobristPiece(pieces[rookIdx].index, rookOldPos);
            pieces[rookIdx].rect.x = rookNewPos.x;
            pieces[rookIdx].rect.y = rookNewPos.y;
            // Zobrist: Add rook to new position
            toggleZobristPiece(pieces[rookIdx].index, rookNewPos);

            gameStateSnapshots.top().castlingRookIdx = rookIdx;
            gameStateSnapshots.top().castlingRookOldPos = rookOldPos;
            gameStateSnapshots.top().castlingRookNewPos = rookNewPos;
        }
    }

    // Handle Pawn Promotion
    int promotedPawnActualIdx = -1;
    int originalPawnActualIndexValue = 0;
    if (newY_grid == 0 && pieces[pieceIdx].index == 6) { // White pawn promotes
        // Zobrist: Remove pawn from board (it's no longer a pawn)
        toggleZobristPiece(pieces[pieceIdx].index, newPos); // Use newPos as piece is already moved
        promotedPawnActualIdx = pieceIdx;
        originalPawnActualIndexValue = pieces[pieceIdx].index;
        pieces[pieceIdx].index = 4; // Promote to Queen
        pieces[pieceIdx].cost = 90;
        // Zobrist: Add new queen to board
        toggleZobristPiece(pieces[pieceIdx].index, newPos);
    } else if (newY_grid == 7 && pieces[pieceIdx].index == -6) { // Black pawn promotes
        // Zobrist: Remove pawn from board
        toggleZobristPiece(pieces[pieceIdx].index, newPos);
        promotedPawnActualIdx = pieceIdx;
        originalPawnActualIndexValue = pieces[pieceIdx].index;
        pieces[pieceIdx].index = -4; // Promote to Queen
        pieces[pieceIdx].cost = -90;
        // Zobrist: Add new queen to board
        toggleZobristPiece(pieces[pieceIdx].index, newPos);
    }
    gameStateSnapshots.top().promotedPawnIdx = promotedPawnActualIdx;
    gameStateSnapshots.top().originalPawnIndexValue = originalPawnActualIndexValue;

    // Move the piece
    pieces[pieceIdx].rect.x = newPos.x;
    pieces[pieceIdx].rect.y = newPos.y;
    // Zobrist: Add piece to new position
    toggleZobristPiece(pieces[pieceIdx].index, newPos);

    // Update castling rights flags
    if (pieces[pieceIdx].index == 5) whiteKingMoved = true;
    else if (pieces[pieceIdx].index == -5) blackKingMoved = true;
    else if (pieces[pieceIdx].index == 1) { // Rook moved
        if (oldX_grid == 0 && oldY_grid == 7) whiteRookQueensideMoved = true; // White A1 rook
        else if (oldX_grid == 7 && oldY_grid == 7) whiteRookKingsideMoved = true; // White H1 rook
    }
    else if (pieces[pieceIdx].index == -1) { // Rook moved
        if (oldX_grid == 0 && oldY_grid == 0) blackRookQueensideMoved = true; // Black A8 rook
        else if (oldX_grid == 7 && oldY_grid == 0) blackRookKingsideMoved = true; // Black H8 rook
    }

    // Update en passant target square
    if (std::abs(pieces[pieceIdx].index) == 6 && std::abs(newY_grid - oldY_grid) == 2) {
        enPassantTargetSquare = {newX_grid, (newY_grid + oldY_grid) / 2};
        enPassantPawnIndex = pieceIdx;
    } else {
        enPassantTargetSquare = {-1, -1};
        enPassantPawnIndex = -1;
    }

    // Zobrist: Add new castling rights hash
    int newCastlingRights = 0;
    if (!whiteKingMoved && !whiteRookKingsideMoved) newCastlingRights |= 0b1000;
    if (!whiteKingMoved && !whiteRookQueensideMoved) newCastlingRights |= 0b0100;
    if (!blackKingMoved && !blackRookKingsideMoved) newCastlingRights |= 0b0010;
    if (!blackKingMoved && !blackRookQueensideMoved) newCastlingRights |= 0b0001;
    currentZobristHash ^= zobristCastlingKeys[newCastlingRights];

    // Zobrist: Add new en passant target square if it exists
    if (enPassantTargetSquare.x != -1) {
        currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
    }
}

void GameManager::undoLastMove() {
    if (gameStateSnapshots.empty()) {
        std::cerr << "No moves to undo." << std::endl;
        return;
    }

    // Restore Zobrist hash from snapshot
    currentZobristHash = gameStateSnapshots.top().prevZobristHash;

    GameStateSnapshot prevState = gameStateSnapshots.top();
    gameStateSnapshots.pop();

    enPassantTargetSquare = prevState.enPassantTargetSquare;
    enPassantPawnIndex = prevState.enPassantPawnIndex;
    whiteKingMoved = prevState.whiteKingMoved;
    blackKingMoved = prevState.blackKingMoved;
    whiteRookKingsideMoved = prevState.whiteRookKingsideMoved;
    whiteRookQueensideMoved = prevState.whiteRookQueensideMoved;
    blackRookKingsideMoved = prevState.blackRookKingsideMoved;
    blackRookQueensideMoved = prevState.blackRookQueensideMoved;

    if (pieceIndexStack.empty()) {
        std::cerr << "Error: Piece stack empty during undo after state restoration." << std::endl;
        return;
    }
    int movedPieceIdx = pieceIndexStack.top(); pieceIndexStack.pop();
    GamePoint movedPieceNewPos = positionStack.top(); positionStack.pop();
    GamePoint movedPieceOldPos = positionStack.top(); positionStack.pop();
    pieces[movedPieceIdx].rect.x = movedPieceOldPos.x;
    pieces[movedPieceIdx].rect.y = movedPieceOldPos.y;

    int capturedPieceIdx = pieceIndexStack.top(); pieceIndexStack.pop();
    GamePoint capturedPieceOldPos = positionStack.top(); positionStack.pop();
    if (capturedPieceIdx != -1) {
        pieces[capturedPieceIdx].rect.x = capturedPieceOldPos.x;
        pieces[capturedPieceIdx].rect.y = capturedPieceOldPos.y;
    }

    if (prevState.castlingRookIdx != -1) {
        pieces[prevState.castlingRookIdx].rect.x = prevState.castlingRookOldPos.x;
        pieces[prevState.castlingRookIdx].rect.y = prevState.castlingRookOldPos.y;
    }

    if (prevState.promotedPawnIdx != -1) {
        pieces[prevState.promotedPawnIdx].index = prevState.originalPawnIndexValue;
        int pieceType = std::abs(pieces[prevState.promotedPawnIdx].index);
        int pieceValue = 0;
        if (pieceType == 1) pieceValue = 50;
        else if (pieceType == 2) pieceValue = 30;
        else if (pieceType == 3) pieceValue = 30;
        else if (pieceType == 4) pieceValue = 90;
        else if (pieceType == 5) pieceValue = 900;
        else if (pieceType == 6) pieceValue = 10;
        pieces[prevState.promotedPawnIdx].cost = pieces[prevState.promotedPawnIdx].index / pieceType * pieceValue;
    }
}

void GameManager::addPossibleMove(int x, int y) {
    if (possibleMoveCount < 32) {
        possibleMoves[possibleMoveCount] = {x * TILE_SIZE + OFFSET.x, y * TILE_SIZE + OFFSET.y};
        possibleMoveCount++;
    } else {
        std::cerr << "Warning: possibleMoves array full. Some moves might be missed." << std::endl;
    }
}

void GameManager::getCurrentBoardGrid(int grid[9][9]) const {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            grid[i][j] = 0;
        }
    }

    for (int j = 0; j < 32; ++j) {
        if (pieces[j].rect.x != -100 || pieces[j].rect.y != -100) {
            int pieceGridX = (pieces[j].rect.x - OFFSET.x) / TILE_SIZE;
            int pieceGridY = (pieces[j].rect.y - OFFSET.y) / TILE_SIZE;
            if (pieceGridX >= 0 && pieceGridX < 8 && pieceGridY >= 0 && pieceGridY < 8) {
                grid[pieceGridY][pieceGridX] = pieces[j].index;
            }
        }
    }
}

bool GameManager::isSquareAttacked(int targetX, int targetY, int attackingColor) const {
    int currentGrid[9][9];
    getCurrentBoardGrid(currentGrid);

    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;

        int pieceX = (pieces[i].rect.x - OFFSET.x) / TILE_SIZE;
        int pieceY = (pieces[i].rect.y - OFFSET.y) / TILE_SIZE;

        int pieceType = std::abs(pieces[i].index);
        int pieceCurrentColor = pieces[i].index > 0 ? 1 : -1;

        if (pieceCurrentColor == attackingColor) { // Only consider pieces of the attacking color
            if (pieceType == 1) { // Rook
                // Check Horizontal and Vertical lines
                // Horizontal
                if (pieceY == targetY) { // Target is on the same row
                    bool blocked = false;
                    int step = (targetX > pieceX) ? 1 : -1;
                    for (int k = pieceX + step; k != targetX; k += step) {
                        if (currentGrid[pieceY][k] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true; // Attack is valid if not blocked
                }
                // Vertical
                if (pieceX == targetX) { // Target is on the same column
                    bool blocked = false;
                    int step = (targetY > pieceY) ? 1 : -1;
                    for (int k = pieceY + step; k != targetY; k += step) {
                        if (currentGrid[k][pieceX] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true; // Attack is valid if not blocked
                }
            } else if (pieceType == 2) { // Knight
                int dx[] = {-2, -2, -1, -1, 1, 1, 2, 2};
                int dy[] = {-1, 1, -2, 2, -2, 2, -1, 1};
                for (int k = 0; k < 8; ++k) {
                    if (pieceX + dx[k] == targetX && pieceY + dy[k] == targetY) return true;
                }
            } else if (pieceType == 3) { // Bishop
                // Check Diagonals
                if (std::abs(pieceX - targetX) == std::abs(pieceY - targetY)) { // Target is on a diagonal
                    int dx = (targetX > pieceX) ? 1 : -1;
                    int dy = (targetY > pieceY) ? 1 : -1;
                    bool blocked = false;
                    for (int k_x = pieceX + dx, k_y = pieceY + dy; k_x != targetX; k_x += dx, k_y += dy) {
                        if (currentGrid[k_y][k_x] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true; // Attack is valid if not blocked
                }
            } else if (pieceType == 4) { // Queen
                // Queen combines Rook and Bishop moves
                // Horizontal and Vertical (Rook part)
                if (pieceY == targetY) {
                    bool blocked = false;
                    int step = (targetX > pieceX) ? 1 : -1;
                    for (int k = pieceX + step; k != targetX; k += step) {
                        if (currentGrid[pieceY][k] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true;
                }
                if (pieceX == targetX) {
                    bool blocked = false;
                    int step = (targetY > pieceY) ? 1 : -1;
                    for (int k = pieceY + step; k != targetY; k += step) {
                        if (currentGrid[k][pieceX] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true;
                }
                // Diagonals (Bishop part)
                if (std::abs(pieceX - targetX) == std::abs(pieceY - targetY)) {
                    int dx = (targetX > pieceX) ? 1 : -1;
                    int dy = (targetY > pieceY) ? 1 : -1;
                    bool blocked = false;
                    for (int k_x = pieceX + dx, k_y = pieceY + dy; k_x != targetX; k_x += dx, k_y += dy) {
                        if (currentGrid[k_y][k_x] != 0) { blocked = true; break; }
                    }
                    if (!blocked) return true;
                }
            } else if (pieceType == 5) { // King
                int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
                int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                for (int k = 0; k < 8; ++k) {
                    if (pieceX + dx[k] == targetX && pieceY + dy[k] == targetY) return true;
                }
            } else if (pieceType == 6) { // Pawn
                int pawnDirection = (pieceCurrentColor == 1) ? -1 : 1;
                // Pawn captures (diagonal)
                if ((pieceX - 1 == targetX && pieceY + pawnDirection == targetY) ||
                    (pieceX + 1 == targetX && pieceY + pawnDirection == targetY)) {
                    return true;
                }
            }
        }
    }
    return false; // No attacking piece found
}

bool GameManager::isKingInCheck(int kingColor) const {
    GamePoint kingPos = getKingPosition(kingColor);
    if (kingPos.x == -1 || kingPos.y == -1) {
        // This should ideally not happen if a king is on the board
        std::cerr << "Error: King of color " << kingColor << " not found on board!" << std::endl;
        return false; // Cannot be in check if no king
    }

    int kingX = (kingPos.x - OFFSET.x) / TILE_SIZE;
    int kingY = (kingPos.y - OFFSET.y) / TILE_SIZE;

    int attackingColor = -kingColor; // Opposite color

    return isSquareAttacked(kingX, kingY, attackingColor);
}

GamePoint GameManager::getKingPosition(int kingColor) const {
    int kingIndex = kingColor * 5; // 5 for King, multiplied by color for sign
    for (int i = 0; i < 32; ++i) {
        if (pieces[i].index == kingIndex && (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)) {
            return {pieces[i].rect.x, pieces[i].rect.y};
        }
    }
    return {-1, -1}; // King not found
}

GamePoint GameManager::getRookPosition(int rookColor, bool isKingside) const {
    int rookIndex = rookColor * 1; // 1 for Rook, multiplied by color for sign
    int targetX = isKingside ? 7 : 0;
    int targetY = (rookColor == 1) ? 7 : 0; // Row 7 for White, Row 0 for Black

    for (int i = 0; i < 32; ++i) {
        if (pieces[i].index == rookIndex &&
            (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == targetX &&
            (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == targetY &&
            (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)) {
            return {pieces[i].rect.x, pieces[i].rect.y};
        }
    }
    return {-1, -1}; // Rook not found
}

void GameManager::calculatePossibleMoves(int pieceIdx) {
    possibleMoveCount = 0; // Reset possible moves for this piece

    GamePoint currentPos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
    int x = (currentPos.x - OFFSET.x) / TILE_SIZE; // Grid X
    int y = (currentPos.y - OFFSET.y) / TILE_SIZE; // Grid Y

    int grid[9][9]; // Get current board state in grid format
    getCurrentBoardGrid(grid);

    int pieceType = std::abs(pieces[pieceIdx].index);

    // Call appropriate move generation function based on piece type
    if (pieceType == 1) findRookMoves(pieceIdx, x, y, grid);
    else if (pieceType == 2) findKnightMoves(pieceIdx, x, y, grid);
    else if (pieceType == 3) findBishopMoves(pieceIdx, x, y, grid);
    else if (pieceType == 4) { // Queen
        findRookMoves(pieceIdx, x, y, grid);
        findBishopMoves(pieceIdx, x, y, grid);
    } else if (pieceType == 5) findKingMoves(pieceIdx, x, y, grid);
    else if (pieceType == 6) findPawnMoves(pieceIdx, x, y, grid);
}

bool GameManager::checkAndAddMove(int pieceIdx, int newX, int newY, int pieceColor) {
    // Store original state to restore after speculative move
    GameStateSnapshot originalSnapshot = {
        enPassantTargetSquare,
        enPassantPawnIndex,
        whiteKingMoved,
        blackKingMoved,
        whiteRookKingsideMoved,
        whiteRookQueensideMoved,
        blackRookKingsideMoved,
        blackRookQueensideMoved,
        -1, // capturedPieceIdx (will be set below)
        {-100, -100}, // capturedPieceOldPos
        -1, // promotedPawnIdx
        0, // originalPawnIndexValue
        -1, // castlingRookIdx
        {-100, -100}, // castlingRookOldPos
        {-100, -100}, // castlingRookNewPos
        currentZobristHash // Store the hash *before* this speculative move
    };

    // Temporarily apply changes for hashing *before* piece moves
    currentZobristHash ^= zobristBlackToMoveKey; // Toggle side to move

    int oldX_grid = (pieces[pieceIdx].rect.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (pieces[pieceIdx].rect.y - OFFSET.y) / TILE_SIZE;

    // Zobrist: Remove old castling rights if they are about to change
    int oldCastlingRights = 0;
    if (!whiteKingMoved && !whiteRookKingsideMoved) oldCastlingRights |= 0b1000;
    if (!whiteKingMoved && !whiteRookQueensideMoved) oldCastlingRights |= 0b0100;
    if (!blackKingMoved && !blackRookKingsideMoved) oldCastlingRights |= 0b0010;
    if (!blackKingMoved && !blackRookQueensideMoved) oldCastlingRights |= 0b0001;
    currentZobristHash ^= zobristCastlingKeys[oldCastlingRights];

    // Zobrist: Remove old en passant target square if it exists
    if (enPassantTargetSquare.x != -1) {
        currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
    }

    // Zobrist: Remove piece from old position
    toggleZobristPiece(pieces[pieceIdx].index, {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y});

    // Simulate move
    GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
    GamePoint tempNewPixelPos = {newX * TILE_SIZE + OFFSET.x, newY * TILE_SIZE + OFFSET.y};

    int capturedSimIdx = -1;
    GamePoint capturedSimOldPos = {-100, -100};

    // Simulate En Passant Capture
    if (std::abs(pieces[pieceIdx].index) == 6 && // If it's a pawn
        std::abs(newX - oldX_grid) == 1 && // Moving diagonally
        std::abs(newY - oldY_grid) == 1 && // Moving diagonally
        newX == originalSnapshot.enPassantTargetSquare.x && newY == originalSnapshot.enPassantTargetSquare.y && // To the en passant target square
        originalSnapshot.enPassantPawnIndex != -1 && // And there was an en passant pawn
        pieces[pieceIdx].index * pieces[originalSnapshot.enPassantPawnIndex].index < 0) // And it's an enemy pawn
    {
        capturedSimIdx = originalSnapshot.enPassantPawnIndex;
        capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
        pieces[capturedSimIdx].rect.x = -100;
        pieces[capturedSimIdx].rect.y = -100;
        toggleZobristPiece(pieces[capturedSimIdx].index, capturedSimOldPos); // Zobrist: Remove captured pawn
    }
    // Simulate Normal Capture
    else {
        // Check if there's a piece at the new position
        for (int p = 0; p < 32; ++p) {
            if (pieces[p].rect.x == tempNewPixelPos.x && pieces[p].rect.y == tempNewPixelPos.y && p != pieceIdx) {
                // If it's an enemy piece, it's a capture
                if ((pieces[p].index > 0 && pieceColor < 0) || (pieces[p].index < 0 && pieceColor > 0)) {
                    capturedSimIdx = p;
                    capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
                    pieces[capturedSimIdx].rect.x = -100;
                    pieces[capturedSimIdx].rect.y = -100;
                    toggleZobristPiece(pieces[capturedSimIdx].index, capturedSimOldPos); // Zobrist: Remove captured piece
                    break;
                }
            }
        }
    }

    // Simulate Castling Rook Move
    int simulatedRookIdx = -1;
    GamePoint simulatedRookOldPos = {-100, -100};
    GamePoint simulatedRookNewPos = {-100, -100};

    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX - oldX_grid) == 2) { // If King is castling
        if (newX == 6) { // Kingside castling
            simulatedRookOldPos = {(pieceColor == 1 ? 7 : 7) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
            simulatedRookNewPos = {(pieceColor == 1 ? 5 : 5) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
        } else if (newX == 2) { // Queenside castling
            simulatedRookOldPos = {(pieceColor == 1 ? 0 : 0) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
            simulatedRookNewPos = {(pieceColor == 1 ? 3 : 3) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
        }
        
        // Find the actual rook index at simulatedRookOldPos in case pieces array order changed
        for(int i = 0; i < 32; ++i) {
            if(pieces[i].index == pieceColor * 1 && pieces[i].rect.x == simulatedRookOldPos.x && pieces[i].rect.y == simulatedRookOldPos.y) {
                simulatedRookIdx = i;
                break;
            }
        }
        if(simulatedRookIdx != -1) {
            toggleZobristPiece(pieces[simulatedRookIdx].index, simulatedRookOldPos); // Zobrist: Remove rook from old position
            pieces[simulatedRookIdx].rect.x = simulatedRookNewPos.x;
            pieces[simulatedRookIdx].rect.y = simulatedRookNewPos.y;
            toggleZobristPiece(pieces[simulatedRookIdx].index, simulatedRookNewPos); // Zobrist: Add rook to new position
        }
    }

    // Simulate pawn promotion
    int promotedPawnSimIdx = -1;
    int originalPawnSimIndexValue = 0;
    if (std::abs(pieces[pieceIdx].index) == 6 && (newY == 0 || newY == 7)) { // Pawn reaches last rank
        // Zobrist: Remove pawn from board (it's no longer a pawn)
        toggleZobristPiece(pieces[pieceIdx].index, tempNewPixelPos);
        promotedPawnSimIdx = pieceIdx;
        originalPawnSimIndexValue = pieces[pieceIdx].index;
        pieces[pieceIdx].index = (pieceColor == 1) ? 4 : -4; // Assume queen promotion
        pieces[pieceIdx].cost = (pieceColor == 1) ? 90 : -90;
        // Zobrist: Add new queen to board
        toggleZobristPiece(pieces[pieceIdx].index, tempNewPixelPos);
    }

    pieces[pieceIdx].rect.x = tempNewPixelPos.x;
    pieces[pieceIdx].rect.y = tempNewPixelPos.y;
    toggleZobristPiece(pieces[pieceIdx].index, tempNewPixelPos); // Zobrist: Add piece to new position

    // Temporarily update castling flags (important for `isKingInCheck` for castling legality)
    bool tempWhiteKingMoved = whiteKingMoved;
    bool tempBlackKingMoved = blackKingMoved;
    bool tempWhiteRookKingsideMoved = whiteRookKingsideMoved;
    bool tempWhiteRookQueensideMoved = whiteRookQueensideMoved;
    bool tempBlackRookKingsideMoved = blackRookKingsideMoved;
    bool tempBlackRookQueensideMoved = blackRookQueensideMoved;

    if (std::abs(pieces[pieceIdx].index) == 5) { // King moved
        if (pieceColor == 1) whiteKingMoved = true;
        else blackKingMoved = true;
    } else if (std::abs(pieces[pieceIdx].index) == 1) { // Rook moved
        if (pieceColor == 1) {
            if (oldX_grid == 0 && oldY_grid == 7) whiteRookQueensideMoved = true;
            else if (oldX_grid == 7 && oldY_grid == 7) whiteRookKingsideMoved = true;
        } else {
            if (oldX_grid == 0 && oldY_grid == 0) blackRookQueensideMoved = true;
            else if (oldX_grid == 7 && oldY_grid == 0) blackRookKingsideMoved = true;
        }
    }

    // Temporarily update en passant target square
    GamePoint tempEnPassantTargetSquare = enPassantTargetSquare;
    int tempEnPassantPawnIndex = enPassantPawnIndex;
    if (std::abs(pieces[pieceIdx].index) == 6 && std::abs(newY - oldY_grid) == 2) {
        enPassantTargetSquare = {newX, (newY + oldY_grid) / 2};
        enPassantPawnIndex = pieceIdx;
    } else {
        enPassantTargetSquare = {-1, -1};
        enPassantPawnIndex = -1;
    }

    // Zobrist: Add new castling rights hash
    int newCastlingRights = 0;
    if (!whiteKingMoved && !whiteRookKingsideMoved) newCastlingRights |= 0b1000;
    if (!whiteKingMoved && !whiteRookQueensideMoved) newCastlingRights |= 0b0100;
    if (!blackKingMoved && !blackRookKingsideMoved) newCastlingRights |= 0b0010;
    if (!blackKingMoved && !blackRookQueensideMoved) newCastlingRights |= 0b0001;
    currentZobristHash ^= zobristCastlingKeys[newCastlingRights];

    // Zobrist: Add new en passant target square if it exists
    if (enPassantTargetSquare.x != -1) {
        currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
    }

    bool isLegal = !isKingInCheck(pieceColor);

    // Restore board state
    if (capturedSimIdx != -1) {
        pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
        pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;
    }
    if (simulatedRookIdx != -1) {
        pieces[simulatedRookIdx].rect.x = simulatedRookOldPos.x;
        pieces[simulatedRookIdx].rect.y = simulatedRookOldPos.y;
    }
    if (promotedPawnSimIdx != -1) {
        pieces[promotedPawnSimIdx].index = originalPawnSimIndexValue;
        int pieceType = std::abs(pieces[promotedPawnSimIdx].index);
        int pieceValue = 0;
        if (pieceType == 1) pieceValue = 50;
        else if (pieceType == 2) pieceValue = 30;
        else if (pieceType == 3) pieceValue = 30;
        else if (pieceType == 4) pieceValue = 90;
        else if (pieceType == 5) pieceValue = 900;
        else if (pieceType == 6) pieceValue = 10;
        pieces[promotedPawnSimIdx].cost = pieces[promotedPawnSimIdx].index / pieceType * pieceValue;
    }
    pieces[pieceIdx].rect.x = originalPiecePos.x;
    pieces[pieceIdx].rect.y = originalPiecePos.y;

    // Restore Zobrist hash to original state before this speculative move
    currentZobristHash = originalSnapshot.prevZobristHash;

    // Restore castling and en passant flags
    whiteKingMoved = tempWhiteKingMoved;
    blackKingMoved = tempBlackKingMoved;
    whiteRookKingsideMoved = tempWhiteRookKingsideMoved;
    whiteRookQueensideMoved = tempWhiteRookQueensideMoved;
    blackRookKingsideMoved = tempBlackRookKingsideMoved;
    blackRookQueensideMoved = tempBlackRookQueensideMoved;
    enPassantTargetSquare = tempEnPassantTargetSquare;
    enPassantPawnIndex = tempEnPassantPawnIndex;

    return isLegal;
}


void GameManager::findRookMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;

    // Directions for Rook: (0,1), (0,-1), (1,0), (-1,0)
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};

    for (int dir = 0; dir < 4; ++dir) {
        for (int step = 1; ; ++step) {
            int newX = x + dx[dir] * step;
            int newY = y + dy[dir] * step;

            if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) break; // Out of bounds

            if (grid[newY][newX] != 0) { // Square is occupied
                // If it's an enemy piece, it's a potential capture
                if ((grid[newY][newX] > 0 && pieceColor < 0) || (grid[newY][newX] < 0 && pieceColor > 0)) {
                    if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) addPossibleMove(newX, newY);
                }
                break; // Stop looking further in this direction (blocked)
            }
            // Empty square, it's a legal move
            if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) addPossibleMove(newX, newY);
        }
    }
}

void GameManager::findBishopMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;

    // Directions for Bishop: (1,1), (1,-1), (-1,1), (-1,-1)
    int dx[] = {1, 1, -1, -1};
    int dy[] = {1, -1, 1, -1};

    for (int dir = 0; dir < 4; ++dir) {
        for (int step = 1; ; ++step) {
            int newX = x + dx[dir] * step;
            int newY = y + dy[dir] * step;

            if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) break; // Out of bounds

            if (grid[newY][newX] != 0) { // Square is occupied
                // If it's an enemy piece, it's a potential capture
                if ((grid[newY][newX] > 0 && pieceColor < 0) || (grid[newY][newX] < 0 && pieceColor > 0)) {
                    if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) addPossibleMove(newX, newY);
                }
                break; // Stop looking further in this direction (blocked)
            }
            // Empty square, it's a legal move
            if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) addPossibleMove(newX, newY);
        }
    }
}

void GameManager::findKnightMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;
    int dx[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    int dy[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; ++i) {
        int newX = x + dx[i];
        int newY = y + dy[i];

        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) { // Check bounds
            // Check if the square is empty or contains an enemy piece
            if (grid[newY][newX] == 0 || (grid[newY][newX] > 0 && pieceColor < 0) || (grid[newY][newX] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) addPossibleMove(newX, newY);
            }
        }
    }
}

void GameManager::findKingMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int i = 0; i < 8; ++i) {
        int newX = x + dx[i];
        int newY = y + dy[i];

        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) { // Check bounds
            // Check if the square is empty or contains an enemy piece
            if (grid[newY][newX] == 0 || (grid[newY][newX] > 0 && pieceColor < 0) || (grid[newY][newX] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) {
                    addPossibleMove(newX, newY);
                }
            }
        }
    }

    // Castling
    bool canCastleKingside = false;
    bool canCastleQueenside = false;

    if (!isKingInCheck(pieceColor)) { // Can only castle if king is not in check
        if (pieceColor == 1) { // White King (starting on e1, grid 4,7)
            if (!whiteKingMoved && x == 4 && y == 7) {
                // Kingside castling (h1 rook, grid 7,7)
                if (!whiteRookKingsideMoved && grid[7][5] == 0 && grid[7][6] == 0 && grid[7][7] == 1) { // f1, g1 empty, h1 rook unmoved
                    if (!isSquareAttacked(5, 7, -1) && !isSquareAttacked(6, 7, -1)) { // f1, g1 not attacked
                        canCastleKingside = true;
                    }
                }
                // Queenside castling (a1 rook, grid 0,7)
                if (!whiteRookQueensideMoved && grid[7][1] == 0 && grid[7][2] == 0 && grid[7][3] == 0 && grid[7][0] == 1) { // b1, c1, d1 empty, a1 rook unmoved
                    if (!isSquareAttacked(3, 7, -1) && !isSquareAttacked(2, 7, -1)) { // c1, d1 not attacked
                        canCastleQueenside = true;
                    }
                }
            }
        } else { // Black King (starting on e8, grid 4,0)
            if (!blackKingMoved && x == 4 && y == 0) {
                // Kingside castling (h8 rook, grid 7,0)
                if (!blackRookKingsideMoved && grid[0][5] == 0 && grid[0][6] == 0 && grid[0][7] == -1) { // f8, g8 empty, h8 rook unmoved
                    if (!isSquareAttacked(5, 0, 1) && !isSquareAttacked(6, 0, 1)) { // f8, g8 not attacked
                        canCastleKingside = true;
                    }
                }
                // Queenside castling (a8 rook, grid 0,0)
                if (!blackRookQueensideMoved && grid[0][1] == 0 && grid[0][2] == 0 && grid[0][3] == 0 && grid[0][0] == -1) { // b8, c8, d8 empty, a8 rook unmoved
                    if (!isSquareAttacked(3, 0, 1) && !isSquareAttacked(2, 0, 1)) { // c8, d8 not attacked
                        canCastleQueenside = true;
                    }
                }
            }
        }
    }

    if (canCastleKingside) {
        addPossibleMove(x + 2, y); // King moves two squares
    }
    if (canCastleQueenside) {
        addPossibleMove(x - 2, y); // King moves two squares
    }
}

void GameManager::findPawnMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;
    int direction = (pieceColor == 1) ? -1 : 1; // -1 for white (up), 1 for black (down)

    // Single step forward
    int newY_forward = y + direction;
    if (newY_forward >= 0 && newY_forward < 8 && grid[newY_forward][x] == 0) { // Check bounds and if square is empty
        if (checkAndAddMove(pieceIdx, x, newY_forward, pieceColor)) addPossibleMove(x, newY_forward);

        // Double step forward (only from starting rank)
        if (((pieceColor == 1 && y == 6) || (pieceColor == -1 && y == 1)) && grid[y + 2 * direction][x] == 0) { // Check if starting rank and square is empty
            int newY_double_forward = y + 2 * direction;
            if (newY_double_forward >= 0 && newY_double_forward < 8) { // Check bounds
                if (checkAndAddMove(pieceIdx, x, newY_double_forward, pieceColor)) addPossibleMove(x, newY_double_forward);
            }
        }
    }

    // Captures (diagonal)
    int newX_left = x - 1;
    if (newX_left >= 0 && newY_forward >= 0 && newY_forward < 8) { // Check bounds
        // Check if square contains an enemy piece
        if ((grid[newY_forward][newX_left] > 0 && pieceColor < 0) || (grid[newY_forward][newX_left] < 0 && pieceColor > 0)) {
            if (checkAndAddMove(pieceIdx, newX_left, newY_forward, pieceColor)) addPossibleMove(newX_left, newY_forward);
        }
    }
    int newX_right = x + 1;
    if (newX_right < 8 && newY_forward >= 0 && newY_forward < 8) { // Check bounds
        // Check if square contains an enemy piece
        if ((grid[newY_forward][newX_right] > 0 && pieceColor < 0) || (grid[newY_forward][newX_right] < 0 && pieceColor > 0)) {
            if (checkAndAddMove(pieceIdx, newX_right, newY_forward, pieceColor)) addPossibleMove(newX_right, newY_forward);
        }
    }

    // En Passant
    // A pawn can capture en passant if:
    // 1. It's on its 5th rank (y=3 for white, y=4 for black)
    // 2. The target square matches `enPassantTargetSquare` (which implies an enemy pawn just moved two squares)
    if ((pieceColor == 1 && y == 3) || (pieceColor == -1 && y == 4)) {
        // Check left for en passant
        if (x - 1 >= 0 && enPassantTargetSquare.x == (x - 1) && enPassantTargetSquare.y == (y + direction))
        {
            // Simulate the en passant move to check legality (king in check)
            // Save temporary state for checkAndAddMove's internal restoration
            GameStateSnapshot originalSnapshot = {
                enPassantTargetSquare, enPassantPawnIndex,
                whiteKingMoved, blackKingMoved,
                whiteRookKingsideMoved, whiteRookQueensideMoved,
                blackRookKingsideMoved, blackRookQueensideMoved,
                -1, {-100,-100}, -1, 0, -1, {-100,-100}, {-100,-100},
                currentZobristHash
            };

            // Temporarily update Zobrist hash for side to move
            currentZobristHash ^= zobristBlackToMoveKey;

            // Zobrist: Remove old castling rights (not changed by en passant but handled for hash consistency)
            int oldCastlingRights = 0;
            if (!whiteKingMoved && !whiteRookKingsideMoved) oldCastlingRights |= 0b1000;
            if (!whiteKingMoved && !whiteRookQueensideMoved) oldCastlingRights |= 0b0100;
            if (!blackKingMoved && !blackRookKingsideMoved) oldCastlingRights |= 0b0010;
            if (!blackKingMoved && !blackRookQueensideMoved) oldCastlingRights |= 0b0001;
            currentZobristHash ^= zobristCastlingKeys[oldCastlingRights];

            // Zobrist: Remove old en passant target square
            if (enPassantTargetSquare.x != -1) {
                currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
            }

            // Zobrist: Remove pawn from old position
            toggleZobristPiece(pieces[pieceIdx].index, {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y});

            GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            GamePoint tempNewPixelPos = {(x - 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y};

            // Identify and remove the captured pawn (the one on the same rank as the moving pawn, but adjacent file)
            int capturedSimIdx = getPieceIndexAtGrid(y, x - 1); // The pawn to be captured
            GamePoint capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
            pieces[capturedSimIdx].rect.x = -100;
            pieces[capturedSimIdx].rect.y = -100;
            toggleZobristPiece(pieces[capturedSimIdx].index, capturedSimOldPos); // Zobrist: Remove captured pawn

            // Move the attacking pawn
            pieces[pieceIdx].rect.x = tempNewPixelPos.x;
            pieces[pieceIdx].rect.y = tempNewPixelPos.y;
            toggleZobristPiece(pieces[pieceIdx].index, tempNewPixelPos); // Zobrist: Add pawn to new pos

            // En passant clears the target square for next turn
            enPassantTargetSquare = {-1, -1};
            enPassantPawnIndex = -1;

            bool isLegal = !isKingInCheck(pieceColor);

            // Restore state
            pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
            pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;
            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;

            // Restore Zobrist hash to original state before this speculative move
            currentZobristHash = originalSnapshot.prevZobristHash;

            // Restore en passant target square and index
            enPassantTargetSquare = originalSnapshot.enPassantTargetSquare;
            enPassantPawnIndex = originalSnapshot.enPassantPawnIndex;

            if (isLegal) {
                addPossibleMove(x - 1, y + direction);
            }
        }
        // Check right for en passant
        if (x + 1 < 8 && enPassantTargetSquare.x == (x + 1) && enPassantTargetSquare.y == (y + direction))
        {
            // Simulate the en passant move to check legality (king in check)
            GameStateSnapshot originalSnapshot = {
                enPassantTargetSquare, enPassantPawnIndex,
                whiteKingMoved, blackKingMoved,
                whiteRookKingsideMoved, whiteRookQueensideMoved,
                blackRookKingsideMoved, blackRookQueensideMoved,
                -1, {-100,-100}, -1, 0, -1, {-100,-100}, {-100,-100},
                currentZobristHash
            };

            currentZobristHash ^= zobristBlackToMoveKey; // Toggle side to move

            int oldCastlingRights = 0; // Castling rights don't change for en passant
            if (!whiteKingMoved && !whiteRookKingsideMoved) oldCastlingRights |= 0b1000;
            if (!whiteKingMoved && !whiteRookQueensideMoved) oldCastlingRights |= 0b0100;
            if (!blackKingMoved && !blackRookKingsideMoved) oldCastlingRights |= 0b0010;
            if (!blackKingMoved && !blackRookQueensideMoved) oldCastlingRights |= 0b0001;
            currentZobristHash ^= zobristCastlingKeys[oldCastlingRights];

            if (enPassantTargetSquare.x != -1) {
                currentZobristHash ^= zobristEnPassantKeys[enPassantTargetSquare.x];
            }
            toggleZobristPiece(pieces[pieceIdx].index, {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y});

            GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            GamePoint tempNewPixelPos = {(x + 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y};

            int capturedSimIdx = getPieceIndexAtGrid(y, x + 1); // The pawn to be captured
            GamePoint capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
            pieces[capturedSimIdx].rect.x = -100;
            pieces[capturedSimIdx].rect.y = -100;
            toggleZobristPiece(pieces[capturedSimIdx].index, capturedSimOldPos); // Zobrist: Remove captured pawn

            pieces[pieceIdx].rect.x = tempNewPixelPos.x;
            pieces[pieceIdx].rect.y = tempNewPixelPos.y;
            toggleZobristPiece(pieces[pieceIdx].index, tempNewPixelPos); // Zobrist: Add pawn to new pos

            // En passant clears the target square for next turn
            enPassantTargetSquare = {-1, -1};
            enPassantPawnIndex = -1;

            bool isLegal = !isKingInCheck(pieceColor);

            // Restore state
            pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
            pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;
            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;

            currentZobristHash = originalSnapshot.prevZobristHash; // Restore hash
            enPassantTargetSquare = originalSnapshot.enPassantTargetSquare; // Restore en passant flags
            enPassantPawnIndex = originalSnapshot.enPassantPawnIndex;

            if (isLegal) {
                addPossibleMove(x + 1, y + direction);
            }
        }
    }
}

char GameManager::toFile(int col) const {
    return 'a' + col;
}

int GameManager::toRank(int row) const {
    return 8 - row;
}

GamePoint GameManager::algebraicToSquare(const std::string& algebraic) const {
    if (algebraic.length() < 2) return {-1, -1};
    int col = algebraic[0] - 'a';
    int row = 8 - (algebraic[1] - '0');
    return {col * TILE_SIZE + OFFSET.x, row * TILE_SIZE + OFFSET.y};
}

int GameManager::getPieceIndexAt(GamePoint pixelPos) const {
    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x == pixelPos.x && pieces[i].rect.y == pixelPos.y) {
            return i;
        }
    }
    return -1; // No piece found at this pixel position
}

int GameManager::getPieceIndexAtGrid(int row, int col) const {
    GamePoint pixelPos = {col * TILE_SIZE + OFFSET.x, row * TILE_SIZE + OFFSET.y};
    return getPieceIndexAt(pixelPos);
}

void GameManager::setBoardFromFEN(const std::string& fen) {
    // Clear existing pieces and reset their state
    for (int i = 0; i < 32; ++i) {
        pieces[i].rect.x = -100;
        pieces[i].rect.y = -100;
        pieces[i].index = 0; // 0 indicates empty slot
        pieces[i].cost = 0;
    }
    // Clear move history and snapshots
    while(!gameStateSnapshots.empty()) gameStateSnapshots.pop();
    while(!positionStack.empty()) positionStack.pop();
    while(!pieceIndexStack.empty()) pieceIndexStack.pop();

    // Reset game state flags to a default 'unmoved' or 'no-enpassant' state
    // These will be overridden by FEN castling availability part
    whiteKingMoved = true; // Assume moved until FEN says otherwise
    blackKingMoved = true;
    whiteRookKingsideMoved = true;
    whiteRookQueensideMoved = true;
    blackRookKingsideMoved = true;
    blackRookQueensideMoved = true;
    enPassantTargetSquare = {-1, -1};
    enPassantPawnIndex = -1; // FEN does not specify which pawn, only the target square

    std::stringstream ss(fen);
    std::string boardPart, turnPart, castlingPart, enPassantPart, halfmoveClockPart, fullmoveNumberPart;
    ss >> boardPart >> turnPart >> castlingPart >> enPassantPart >> halfmoveClockPart >> fullmoveNumberPart;

    int currentPieceK = 0; // Counter for populating the pieces array
    int row = 0;
    int col = 0;

    // Parse piece placement
    for (char c : boardPart) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            col += (c - '0'); // Skip empty squares
        } else {
            int pieceIndex = 0;
            int pieceValue = 0;
            int pieceColor = 0;

            if (isupper(c)) {
                pieceColor = 1; // White piece
            } else {
                pieceColor = -1; // Black piece
            }

            switch (tolower(c)) {
                case 'r': pieceIndex = 1; pieceValue = 50; break;
                case 'n': pieceIndex = 2; pieceValue = 30; break;
                case 'b': pieceIndex = 3; pieceValue = 30; break;
                case 'q': pieceIndex = 4; pieceValue = 90; break;
                case 'k': pieceIndex = 5; pieceValue = 900; break;
                case 'p': pieceIndex = 6; pieceValue = 10; break;
                default: break; // Should not happen with valid FEN input
            }

            // Populate the ChessPiece array
            if (pieceIndex != 0 && currentPieceK < 32) { // Ensure we don't go out of bounds
                pieces[currentPieceK].index = pieceColor * pieceIndex;
                pieces[currentPieceK].rect = {col * TILE_SIZE + OFFSET.x, row * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};
                pieces[currentPieceK].cost = pieceColor * pieceValue;
                currentPieceK++;
            }
            col++; // Move to next column
        }
    }

    // Parse castling availability from FEN string
    // Set to false only if the corresponding castling right is explicitly present in the FEN
    if (castlingPart.find('K') != std::string::npos) { whiteKingMoved = false; whiteRookKingsideMoved = false; }
    if (castlingPart.find('Q') != std::string::npos) { whiteKingMoved = false; whiteRookQueensideMoved = false; }
    if (castlingPart.find('k') != std::string::npos) { blackKingMoved = false; blackRookKingsideMoved = false; }
    if (castlingPart.find('q') != std::string::npos) { blackKingMoved = false; blackRookQueensideMoved = false; }

    // Parse en passant target square from FEN
    if (enPassantPart != "-") {
        GamePoint tempPixelPos = algebraicToSquare(enPassantPart);
        enPassantTargetSquare = {(tempPixelPos.x - OFFSET.x) / TILE_SIZE, (tempPixelPos.y - OFFSET.y) / TILE_SIZE};
    } else {
        enPassantTargetSquare = {-1, -1};
    }

    // The halfmove clock and fullmove number are not currently used in this engine.

    currentZobristHash = calculateZobristHash(); // Calculate the Zobrist hash for the fully parsed board
}

std::string GameManager::convertMoveToAlgebraic(int pieceIdx, GamePoint oldPos, GamePoint newPos) const {
    if (pieceIdx == -1) {
        return "(invalid)"; // Return a specific string for invalid moves to avoid issues
    }

    std::string moveStr = "";
    moveStr += toFile((oldPos.x - OFFSET.x) / TILE_SIZE);
    moveStr += std::to_string(toRank((oldPos.y - OFFSET.y) / TILE_SIZE));
    moveStr += toFile((newPos.x - OFFSET.x) / TILE_SIZE);
    moveStr += std::to_string(toRank((newPos.y - OFFSET.y) / TILE_SIZE));

    int pieceType = std::abs(pieces[pieceIdx].index);
    int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;

    // Append promotion character if it's a pawn promotion
    if (pieceType == 6 && (newY_grid == 0 || newY_grid == 7)) {
        // For UCI, promotions typically default to 'q' (queen) if not specified
        moveStr += "q";
    }

    return moveStr;
}

void GameManager::applyUCIStringMove(const std::string& moveStr, PlayerColor currentPlayer) {
    if (moveStr.length() < 4) {
        std::cerr << "Invalid UCI move string: " << moveStr << std::endl;
        return;
    }

    std::string oldSquareStr = moveStr.substr(0, 2);
    std::string newSquareStr = moveStr.substr(2, 2);
    char promotionChar = (moveStr.length() == 5) ? moveStr[4] : ' ';

    GamePoint oldPixelPos = algebraicToSquare(oldSquareStr);
    GamePoint newPixelPos = algebraicToSquare(newSquareStr);

    int pieceIdx = getPieceIndexAt(oldPixelPos);

    if (pieceIdx == -1) {
        std::cerr << "Could not find piece at " << oldSquareStr << " for move " << moveStr << std::endl;
        return;
    }

    int oldX_grid = (oldPixelPos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (oldPixelPos.y - OFFSET.y) / TILE_SIZE;
    int newX_grid = (newPixelPos.x - OFFSET.x) / TILE_SIZE; // Fix: Use TILE_SIZE instead of TIL_SIZE
    int newY_grid = (newPixelPos.y - OFFSET.y) / TILE_SIZE;

    // Determine if it's a castling move (king moves two squares)
    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX_grid - oldX_grid) == 2) {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
    }
    // Determine if it's a pawn promotion
    else if (std::abs(pieces[pieceIdx].index) == 6 && (newY_grid == 0 || newY_grid == 7) && promotionChar != ' ') {
        // Apply the base pawn move first
        movePiece(pieceIdx, oldPixelPos, newPixelPos);

        // Then, handle the promotion by changing the piece type
        // Zobrist: Remove the old pawn's hash from the new square
        toggleZobristPiece(pieces[pieceIdx].index, newPixelPos);

        int promotedType = 0;
        if (tolower(promotionChar) == 'q') promotedType = 4;
        else if (tolower(promotionChar) == 'r') promotedType = 1;
        else if (tolower(promotionChar) == 'b') promotedType = 3;
        else if (tolower(promotionChar) == 'n') promotedType = 2;

        pieces[pieceIdx].index = (pieces[pieceIdx].index > 0 ? 1 : -1) * promotedType;
        int pieceValue = 0; // Update piece cost
        if (promotedType == 1) pieceValue = 50;
        else if (promotedType == 2) pieceValue = 30;
        else if (promotedType == 3) pieceValue = 30;
        else if (promotedType == 4) pieceValue = 90;
        pieces[pieceIdx].cost = (pieces[pieceIdx].index > 0 ? 1 : -1) * pieceValue;

        // Zobrist: Add the new promoted piece's hash at the new square
        toggleZobristPiece(pieces[pieceIdx].index, newPixelPos);
    }
    // Handle en passant capture (pawn moves diagonally to empty square, target pawn captured)
    // This case requires checking if the target square is indeed empty
    // and if the `enPassantTargetSquare` matches, as `movePiece` needs to know it's an en passant.
    else if (std::abs(pieces[pieceIdx].index) == 6 &&
             std::abs(newX_grid - oldX_grid) == 1 &&
             std::abs(newY_grid - oldY_grid) == 1 &&
             getPieceIndexAt(newPixelPos) == -1 && // Target square must be empty
             newX_grid == enPassantTargetSquare.x && newY_grid == enPassantTargetSquare.y) // Must be the en passant target square
    {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
    }
    // Handle normal moves
    else {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
    }
}
