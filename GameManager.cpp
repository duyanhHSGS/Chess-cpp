#include "GameManager.h"
#include "ChessAI.h"
#include "Constants.h"
#include "ChessPiece.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cctype>

GameManager::GameManager() :
    possibleMoveCount(0),
    whiteKingMoved(false),
    blackKingMoved(false),
    whiteRookKingsideMoved(false),
    whiteRookQueensideMoved(false),
    blackRookKingsideMoved(false),
    blackRookQueensideMoved(false),
    enPassantTargetSquare({-1, -1}),
    enPassantPawnIndex(-1)
{
    ai = std::make_unique<ChessAI>();
}

GameManager::~GameManager() {
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
}

void GameManager::movePiece(int pieceIdx, GamePoint oldPos, GamePoint newPos) {
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
        {-100, -100}
    };
    gameStateSnapshots.push(currentSnapshot);

    int oldX_grid = (oldPos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (oldPos.y - OFFSET.y) / TILE_SIZE;
    int newX_grid = (newPos.x - OFFSET.x) / TILE_SIZE;
    int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;

    positionStack.push(oldPos);
    positionStack.push(newPos);
    pieceIndexStack.push(pieceIdx);

    int capturedPieceActualIdx = -1;
    GamePoint capturedPieceActualOldPos = {-100, -100};

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
    }
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
                    break;
                }
            }
        }
    }
    positionStack.push(capturedPieceActualOldPos);
    pieceIndexStack.push(capturedPieceActualIdx);


    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX_grid - oldX_grid) == 2) {
        int rookIdx = -1;
        GamePoint rookOldPos;
        GamePoint rookNewPos;

        if (newX_grid == 6) {
            if (pieces[pieceIdx].index == 5) {
                for (int i = 16; i < 32; ++i) {
                    if (pieces[i].index == 1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 7 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 7) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {7 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
                rookNewPos = {5 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
            } else {
                for (int i = 0; i < 16; ++i) {
                    if (pieces[i].index == -1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 7 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 0) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {7 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
                rookNewPos = {5 * TILE_SIZE + OFFSET.x, 0 * TILE_SIZE + OFFSET.y};
            }
        }
        else if (newX_grid == 2) {
            if (pieces[pieceIdx].index == 5) {
                for (int i = 16; i < 32; ++i) {
                    if (pieces[i].index == 1 && (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == 0 && (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == 7) {
                        rookIdx = i;
                        break;
                    }
                }
                rookOldPos = {0 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
                rookNewPos = {3 * TILE_SIZE + OFFSET.x, 7 * TILE_SIZE + OFFSET.y};
            } else {
                for (int i = 0; i < 16; ++i) {
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
            pieces[rookIdx].rect.x = rookNewPos.x;
            pieces[rookIdx].rect.y = rookNewPos.y;
            gameStateSnapshots.top().castlingRookIdx = rookIdx;
            gameStateSnapshots.top().castlingRookOldPos = rookOldPos;
            gameStateSnapshots.top().castlingRookNewPos = rookNewPos;
        }
    }


    int promotedPawnActualIdx = -1;
    int originalPawnActualIndexValue = 0;
    if (newY_grid == 0 && pieces[pieceIdx].index == 6) {
        promotedPawnActualIdx = pieceIdx;
        originalPawnActualIndexValue = pieces[pieceIdx].index;
        pieces[pieceIdx].index = 4;
        pieces[pieceIdx].cost = 90;
    } else if (newY_grid == 7 && pieces[pieceIdx].index == -6) {
        promotedPawnActualIdx = pieceIdx;
        originalPawnActualIndexValue = pieces[pieceIdx].index;
        pieces[pieceIdx].index = -4;
        pieces[pieceIdx].cost = -90;
    }
    gameStateSnapshots.top().promotedPawnIdx = promotedPawnActualIdx;
    gameStateSnapshots.top().originalPawnIndexValue = originalPawnActualIndexValue;


    pieces[pieceIdx].rect.x = newPos.x;
    pieces[pieceIdx].rect.y = newPos.y;

    if (pieces[pieceIdx].index == 5) whiteKingMoved = true;
    else if (pieces[pieceIdx].index == -5) blackKingMoved = true;
    else if (pieces[pieceIdx].index == 1) {
        if (oldX_grid == 0 && oldY_grid == 7) whiteRookQueensideMoved = true;
        else if (oldX_grid == 7 && oldY_grid == 7) whiteRookKingsideMoved = true;
    }
    else if (pieces[pieceIdx].index == -1) {
        if (oldX_grid == 0 && oldY_grid == 0) blackRookQueensideMoved = true;
        else if (oldX_grid == 7 && oldY_grid == 0) blackRookKingsideMoved = true;
    }

    if (std::abs(pieces[pieceIdx].index) == 6 && std::abs(newY_grid - oldY_grid) == 2) {
        enPassantTargetSquare = {newX_grid, (newY_grid + oldY_grid) / 2};
        enPassantPawnIndex = pieceIdx;
    } else {
        enPassantTargetSquare = {-1, -1};
        enPassantPawnIndex = -1;
    }
}

void GameManager::undoLastMove() {
    if (gameStateSnapshots.empty()) {
        std::cerr << "No moves to undo." << std::endl;
        return;
    }

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

        if ((pieces[i].index > 0 && attackingColor > 0) || (pieces[i].index < 0 && attackingColor < 0)) {
            int pieceX = (pieces[i].rect.x - OFFSET.x) / TILE_SIZE;
            int pieceY = (pieces[i].rect.y - OFFSET.y) / TILE_SIZE;

            int pieceType = std::abs(pieces[i].index);
            int pieceCurrentColor = pieces[i].index > 0 ? 1 : -1;

            if (pieceType == 1) {
                for (int col = 0; col < 8; ++col) {
                    if (col == pieceX) continue;
                    if (col == targetX && pieceY == targetY) {
                         bool blocked = false;
                         int start = std::min(pieceX, col) + 1;
                         int end = std::max(pieceX, col);
                         for (int k = start; k < end; ++k) {
                             if (currentGrid[pieceY][k] != 0) { blocked = true; break; }
                         }
                         if (!blocked) return true;
                    }
                }
                for (int row = 0; row < 8; ++row) {
                    if (row == pieceY) continue;
                    if (row == targetY && pieceX == targetX) {
                        bool blocked = false;
                        int start = std::min(pieceY, row) + 1;
                        int end = std::max(pieceY, row);
                        for (int k = start; k < end; ++k) {
                            if (currentGrid[k][pieceX] != 0) { blocked = true; break; }
                        }
                        if (!blocked) return true;
                    }
                }
            } else if (pieceType == 2) {
                int dx[] = {-2, -2, -1, -1, 1, 1, 2, 2};
                int dy[] = {-1, 1, -2, 2, -2, 2, -1, 1};
                for (int k = 0; k < 8; ++k) {
                    if (pieceX + dx[k] == targetX && pieceY + dy[k] == targetY) return true;
                }
            } else if (pieceType == 3) {
                int d_x[] = {1, 1, -1, -1};
                int d_y[] = {1, -1, 1, -1};
                for (int dir = 0; dir < 4; ++dir) {
                    for (int step = 1; ; ++step) {
                        int newX = pieceX + d_x[dir] * step;
                        int newY = pieceY + d_y[dir] * step;
                        if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) break;
                        if (newX == targetX && newY == targetY) return true;
                        if (currentGrid[newY][newX] != 0) break;
                    }
                }
            } else if (pieceType == 4) {
                for (int col = 0; col < 8; ++col) {
                    if (col == pieceX) continue;
                    if (col == targetX && pieceY == targetY) {
                         bool blocked = false;
                         int start = std::min(pieceX, col) + 1;
                         int end = std::max(pieceX, col);
                         for (int k = start; k < end; ++k) {
                             if (currentGrid[pieceY][k] != 0) { blocked = true; break; }
                         }
                         if (!blocked) return true;
                    }
                }
                for (int row = 0; row < 8; ++row) {
                    if (row == pieceY) continue;
                    if (row == targetY && pieceX == targetX) {
                        bool blocked = false;
                        int start = std::min(pieceY, row) + 1;
                        int end = std::max(pieceY, row);
                        for (int k = start; k < end; ++k) {
                            if (currentGrid[k][pieceX] != 0) { blocked = true; break; }
                        }
                        if (!blocked) return true;
                    }
                }
                int d_x[] = {1, 1, -1, -1};
                int d_y[] = {1, -1, 1, -1};
                for (int dir = 0; dir < 4; ++dir) {
                    for (int step = 1; ; ++step) {
                        int newX = pieceX + d_x[dir] * step;
                        int newY = pieceY + d_y[dir] * step;
                        if (newX < 0 || newX >= 8 || newY < 0 || newY >= 8) break;
                        if (newX == targetX && newY == targetY) return true;
                        if (currentGrid[newY][newX] != 0) break;
                    }
                }
            } else if (pieceType == 5) {
                int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
                int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                for (int k = 0; k < 8; ++k) {
                    if (pieceX + dx[k] == targetX && pieceY + dy[k] == targetY) return true;
                }
            } else if (pieceType == 6) {
                int pawnDirection = (pieceCurrentColor == 1) ? -1 : 1;
                if ((pieceX - 1 == targetX && pieceY + pawnDirection == targetY) ||
                    (pieceX + 1 == targetX && pieceY + pawnDirection == targetY)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool GameManager::isKingInCheck(int kingColor) const {
    GamePoint kingPos = getKingPosition(kingColor);
    if (kingPos.x == -1 || kingPos.y == -1) {
        return false;
    }

    int kingX = (kingPos.x - OFFSET.x) / TILE_SIZE;
    int kingY = (kingPos.y - OFFSET.y) / TILE_SIZE;

    int attackingColor = -kingColor;

    return isSquareAttacked(kingX, kingY, attackingColor);
}

GamePoint GameManager::getKingPosition(int kingColor) const {
    int kingIndex = kingColor * 5;
    for (int i = 0; i < 32; ++i) {
        if (pieces[i].index == kingIndex && (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)) {
            return {pieces[i].rect.x, pieces[i].rect.y};
        }
    }
    return {-1, -1};
}

GamePoint GameManager::getRookPosition(int rookColor, bool isKingside) const {
    int rookIndex = rookColor * 1;
    int targetX = isKingside ? 7 : 0;
    int targetY = (rookColor == 1) ? 7 : 0;

    for (int i = 0; i < 32; ++i) {
        if (pieces[i].index == rookIndex &&
            (pieces[i].rect.x - OFFSET.x) / TILE_SIZE == targetX &&
            (pieces[i].rect.y - OFFSET.y) / TILE_SIZE == targetY &&
            (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)) {
            return {pieces[i].rect.x, pieces[i].rect.y};
        }
    }
    return {-1, -1};
}

void GameManager::calculatePossibleMoves(int pieceIdx) {
    possibleMoveCount = 0;

    GamePoint currentPos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
    int x = (currentPos.x - OFFSET.x) / TILE_SIZE;
    int y = (currentPos.y - OFFSET.y) / TILE_SIZE;

    int grid[9][9];
    getCurrentBoardGrid(grid);

    int pieceType = std::abs(pieces[pieceIdx].index);

    if (pieceType == 1) findRookMoves(pieceIdx, x, y, grid);
    else if (pieceType == 2) findKnightMoves(pieceIdx, x, y, grid);
    else if (pieceType == 3) findBishopMoves(pieceIdx, x, y, grid);
    else if (pieceType == 4) {
        findRookMoves(pieceIdx, x, y, grid);
        findBishopMoves(pieceIdx, x, y, grid);
    } else if (pieceType == 5) findKingMoves(pieceIdx, x, y, grid);
    else if (pieceType == 6) findPawnMoves(pieceIdx, x, y, grid);
}

bool GameManager::checkAndAddMove(int pieceIdx, int newX, int newY, int pieceColor) {
    GameStateSnapshot originalSnapshot = {
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
        {-100, -100}
    };

    GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
    GameRect tempNewPosRect = {newX * TILE_SIZE + OFFSET.x, newY * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

    int capturedSimIdx = -1;
    GamePoint capturedSimOldPos = {-100, -100};

    int oldX_grid = (originalPiecePos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (originalPiecePos.y - OFFSET.y) / TILE_SIZE;
    int pawnDirection = (pieceColor == 1) ? -1 : 1;

    if (std::abs(pieces[pieceIdx].index) == 6 &&
        std::abs(newX - oldX_grid) == 1 &&
        std::abs(newY - oldY_grid) == 1 &&
        newX == enPassantTargetSquare.x && newY == enPassantTargetSquare.y &&
        enPassantPawnIndex != -1 &&
        pieces[pieceIdx].index * pieces[enPassantPawnIndex].index < 0)
    {
        capturedSimIdx = enPassantPawnIndex;
        capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
        pieces[capturedSimIdx].rect.x = -100;
        pieces[capturedSimIdx].rect.y = -100;
    }
    else {
        for (int p = 0; p < 32; ++p) {
            if (pieces[p].rect.x == tempNewPosRect.x && pieces[p].rect.y == tempNewPosRect.y && p != pieceIdx) {
                if ((pieces[p].index > 0 && pieceColor < 0) || (pieces[p].index < 0 && pieceColor > 0)) {
                    capturedSimIdx = p;
                    capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
                    pieces[capturedSimIdx].rect.x = -100;
                    pieces[capturedSimIdx].rect.y = -100;
                    break;
                }
            }
        }
    }

    int simulatedRookIdx = -1;
    GamePoint simulatedRookOldPos = {-100, -100};
    GamePoint simulatedRookNewPos = {-100, -100};

    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX - oldX_grid) == 2) {
        if (newX == 6) {
            simulatedRookIdx = (pieceColor == 1) ? 23 : 7;
            simulatedRookOldPos = {(pieceColor == 1 ? 7 : 7) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
            simulatedRookNewPos = {(pieceColor == 1 ? 5 : 5) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
        } else if (newX == 2) {
            simulatedRookIdx = (pieceColor == 1) ? 16 : 0;
            simulatedRookOldPos = {(pieceColor == 1 ? 0 : 0) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
            simulatedRookNewPos = {(pieceColor == 1 ? 3 : 3) * TILE_SIZE + OFFSET.x, (pieceColor == 1 ? 7 : 0) * TILE_SIZE + OFFSET.y};
        }
        if (simulatedRookIdx != -1) {
            for(int i = 0; i < 32; ++i) {
                if(pieces[i].index == pieceColor * 1 && pieces[i].rect.x == simulatedRookOldPos.x && pieces[i].rect.y == simulatedRookOldPos.y) {
                    simulatedRookIdx = i;
                    break;
                }
            }
            if(simulatedRookIdx != -1) {
                pieces[simulatedRookIdx].rect.x = simulatedRookNewPos.x;
                pieces[simulatedRookIdx].rect.y = simulatedRookNewPos.y;
            }
        }
    }

    pieces[pieceIdx].rect.x = tempNewPosRect.x;
    pieces[pieceIdx].rect.y = tempNewPosRect.y;

    bool isLegal = !isKingInCheck(pieceColor);

    pieces[pieceIdx].rect.x = originalPiecePos.x;
    pieces[pieceIdx].rect.y = originalPiecePos.y;
    if (capturedSimIdx != -1) {
        pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
        pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;
    }
    if (simulatedRookIdx != -1 && simulatedRookIdx != -1) {
        pieces[simulatedRookIdx].rect.x = simulatedRookOldPos.x;
        pieces[simulatedRookIdx].rect.y = simulatedRookOldPos.y;
    }

    enPassantTargetSquare = originalSnapshot.enPassantTargetSquare;
    enPassantPawnIndex = originalSnapshot.enPassantPawnIndex;
    whiteKingMoved = originalSnapshot.whiteKingMoved;
    blackKingMoved = originalSnapshot.blackKingMoved;
    whiteRookKingsideMoved = originalSnapshot.whiteRookKingsideMoved;
    whiteRookQueensideMoved = originalSnapshot.whiteRookQueensideMoved;
    blackRookKingsideMoved = originalSnapshot.blackRookKingsideMoved;
    blackRookQueensideMoved = originalSnapshot.blackRookQueensideMoved;

    return isLegal;
}


void GameManager::findRookMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;

    for (int i = x + 1; i < 8; ++i) {
        if (grid[y][i] != 0) {
            if ((grid[y][i] > 0 && pieceColor < 0) || (grid[y][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, y, pieceColor)) addPossibleMove(i, y);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, y, pieceColor)) addPossibleMove(i, y);
    }
    for (int i = x - 1; i >= 0; --i) {
        if (grid[y][i] != 0) {
            if ((grid[y][i] > 0 && pieceColor < 0) || (grid[y][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, y, pieceColor)) addPossibleMove(i, y);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, y, pieceColor)) addPossibleMove(i, y);
    }
    for (int j = y + 1; j < 8; ++j) {
        if (grid[j][x] != 0) {
            if ((grid[j][x] > 0 && pieceColor < 0) || (grid[j][x] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, x, j, pieceColor)) addPossibleMove(x, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, x, j, pieceColor)) addPossibleMove(x, j);
    }
    for (int j = y - 1; j >= 0; --j) {
        if (grid[j][x] != 0) {
            if ((grid[j][x] > 0 && pieceColor < 0) || (grid[j][x] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, x, j, pieceColor)) addPossibleMove(x, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, x, j, pieceColor)) addPossibleMove(x, j);
    }
}

void GameManager::findBishopMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;

    for (int i = x + 1, j = y + 1; (i < 8 && j < 8); ++i, ++j) {
        if (grid[j][i] != 0) {
            if ((grid[j][i] > 0 && pieceColor < 0) || (grid[j][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
    }
    for (int i = x - 1, j = y + 1; (i >= 0 && j < 8); --i, ++j) {
        if (grid[j][i] != 0) {
            if ((grid[j][i] > 0 && pieceColor < 0) || (grid[j][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
    }
    for (int i = x + 1, j = y - 1; (i < 8 && j >= 0); ++i, --j) {
        if (grid[j][i] != 0) {
            if ((grid[j][i] > 0 && pieceColor < 0) || (grid[j][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
    }
    for (int i = x - 1, j = y - 1; (i >= 0 && j >= 0); --i, --j) {
        if (grid[j][i] != 0) {
            if ((grid[j][i] > 0 && pieceColor < 0) || (grid[j][i] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
            }
            break;
        }
        if (checkAndAddMove(pieceIdx, i, j, pieceColor)) addPossibleMove(i, j);
    }
}

void GameManager::findKnightMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;
    int dx[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    int dy[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; ++i) {
        int newX = x + dx[i];
        int newY = y + dy[i];

        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
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

        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            if (grid[newY][newX] == 0 || (grid[newY][newX] > 0 && pieceColor < 0) || (grid[newY][newX] < 0 && pieceColor > 0)) {
                if (checkAndAddMove(pieceIdx, newX, newY, pieceColor)) {
                    addPossibleMove(newX, newY);
                }
            }
        }
    }

    bool canCastleKingside = false;
    bool canCastleQueenside = false;

    if (!isKingInCheck(pieceColor)) {
        if (pieceColor == 1) {
            if (!whiteKingMoved && x == 4 && y == 7) {
                if (!whiteRookKingsideMoved && grid[7][5] == 0 && grid[7][6] == 0 &&
                    grid[7][7] == 1) {
                    if (!isSquareAttacked(5, 7, -1) && !isSquareAttacked(6, 7, -1)) {
                        canCastleKingside = true;
                    }
                }
                if (!whiteRookQueensideMoved && grid[7][1] == 0 && grid[7][2] == 0 && grid[7][3] == 0 &&
                    grid[7][0] == 1) {
                    if (!isSquareAttacked(3, 7, -1) && !isSquareAttacked(2, 7, -1)) {
                        canCastleQueenside = true;
                    }
                }
            }
        } else {
            if (!blackKingMoved && x == 4 && y == 0) {
                if (!blackRookKingsideMoved && grid[0][5] == 0 && grid[0][6] == 0 &&
                    grid[0][7] == -1) {
                    if (!isSquareAttacked(5, 0, 1) && !isSquareAttacked(6, 0, 1)) {
                        canCastleKingside = true;
                    }
                }
                if (!blackRookQueensideMoved && grid[0][1] == 0 && grid[0][2] == 0 && grid[0][3] == 0 &&
                    grid[0][0] == -1) {
                    if (!isSquareAttacked(3, 0, 1) && !isSquareAttacked(2, 0, 1)) {
                        canCastleQueenside = true;
                    }
                }
            }
        }
    }

    if (canCastleKingside) {
        addPossibleMove(x + 2, y);
    }
    if (canCastleQueenside) {
        addPossibleMove(x - 2, y);
    }
}

void GameManager::findPawnMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int pieceColor = grid[y][x] > 0 ? 1 : -1;
    int direction = (pieceColor == 1) ? -1 : 1;

    int newY_forward = y + direction;
    if (newY_forward >= 0 && newY_forward < 8 && grid[newY_forward][x] == 0) {
        if (checkAndAddMove(pieceIdx, x, newY_forward, pieceColor)) addPossibleMove(x, newY_forward);

        if (((pieceColor == 1 && y == 6) || (pieceColor == -1 && y == 1)) && grid[y + 2 * direction][x] == 0) {
            int newY_double_forward = y + 2 * direction;
            if (newY_double_forward >= 0 && newY_double_forward < 8) {
                if (checkAndAddMove(pieceIdx, x, newY_double_forward, pieceColor)) addPossibleMove(x, newY_double_forward);
            }
        }
    }

    int newX_left = x - 1;
    if (newX_left >= 0 && newY_forward >= 0 && newY_forward < 8) {
        if ((grid[newY_forward][newX_left] > 0 && pieceColor < 0) || (grid[newY_forward][newX_left] < 0 && pieceColor > 0)) {
            if (checkAndAddMove(pieceIdx, newX_left, newY_forward, pieceColor)) addPossibleMove(newX_left, newY_forward);
        }
    }
    int newX_right = x + 1;
    if (newX_right < 8 && newY_forward >= 0 && newY_forward < 8) {
        if ((grid[newY_forward][newX_right] > 0 && pieceColor < 0) || (grid[newY_forward][newX_right] < 0 && pieceColor > 0)) {
            if (checkAndAddMove(pieceIdx, newX_right, newY_forward, pieceColor)) addPossibleMove(newX_right, newY_forward);
        }
    }

    if ((pieceColor == 1 && y == 3) || (pieceColor == -1 && y == 4)) {
        if (x - 1 >= 0 &&
            (enPassantTargetSquare.x == x - 1 && enPassantTargetSquare.y == y + direction) &&
            std::abs(pieces[enPassantPawnIndex].index) == 6 &&
            pieces[enPassantPawnIndex].index * pieceColor < 0)
        {
            GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            GameRect tempNewPosRect = {(x - 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

            int capturedSimIdx = enPassantPawnIndex;
            GamePoint capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
            pieces[capturedSimIdx].rect.x = -100;
            pieces[capturedSimIdx].rect.y = -100;

            pieces[pieceIdx].rect.x = tempNewPosRect.x;
            pieces[pieceIdx].rect.y = tempNewPosRect.y;

            bool isLegal = !isKingInCheck(pieceColor);

            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;
            pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
            pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;

            if (isLegal) {
                addPossibleMove(x - 1, y + direction);
            }
        }
        if (x + 1 < 8 &&
            (enPassantTargetSquare.x == x + 1 && enPassantTargetSquare.y == y + direction) &&
            std::abs(pieces[enPassantPawnIndex].index) == 6 &&
            pieces[enPassantPawnIndex].index * pieceColor < 0)
        {
            GamePoint originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            GameRect tempNewPosRect = {(x + 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

            int capturedSimIdx = enPassantPawnIndex;
            GamePoint capturedSimOldPos = {pieces[capturedSimIdx].rect.x, pieces[capturedSimIdx].rect.y};
            pieces[capturedSimIdx].rect.x = -100;
            pieces[capturedSimIdx].rect.y = -100;

            pieces[pieceIdx].rect.x = tempNewPosRect.x;
            pieces[pieceIdx].rect.y = tempNewPosRect.y;

            bool isLegal = !isKingInCheck(pieceColor);

            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;
            pieces[capturedSimIdx].rect.x = capturedSimOldPos.x;
            pieces[capturedSimIdx].rect.y = capturedSimOldPos.y;

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
    return -1;
}

int GameManager::getPieceIndexAtGrid(int row, int col) const {
    GamePoint pixelPos = {col * TILE_SIZE + OFFSET.x, row * TILE_SIZE + OFFSET.y};
    return getPieceIndexAt(pixelPos);
}

void GameManager::setBoardFromFEN(const std::string& fen) {
    for (int i = 0; i < 32; ++i) {
        pieces[i].rect.x = -100;
        pieces[i].rect.y = -100;
        pieces[i].index = 0;
        pieces[i].cost = 0;
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

    std::stringstream ss(fen);
    std::string boardPart;
    ss >> boardPart;

    int currentPieceK = 0;
    int row = 0;
    int col = 0;

    for (char c : boardPart) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            col += (c - '0');
        } else {
            int pieceIndex = 0;
            int pieceValue = 0;
            int pieceColor = 0;

            if (isupper(c)) {
                pieceColor = 1;
            } else {
                pieceColor = -1;
            }

            switch (tolower(c)) {
                case 'r': pieceIndex = 1; pieceValue = 50; break;
                case 'n': pieceIndex = 2; pieceValue = 30; break;
                case 'b': pieceIndex = 3; pieceValue = 30; break;
                case 'q': pieceIndex = 4; pieceValue = 90; break;
                case 'k': pieceIndex = 5; pieceValue = 900; break;
                case 'p': pieceIndex = 6; pieceValue = 10; break;
                default: break;
            }

            if (pieceIndex != 0) {
                pieces[currentPieceK].index = pieceColor * pieceIndex;
                pieces[currentPieceK].rect = {col * TILE_SIZE + OFFSET.x, row * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};
                pieces[currentPieceK].cost = pieceColor * pieceValue;
                currentPieceK++;
            }
            col++;
        }
    }
}

std::string GameManager::convertMoveToAlgebraic(int pieceIdx, GamePoint oldPos, GamePoint newPos) const {
    std::string moveStr = "";
    moveStr += toFile((oldPos.x - OFFSET.x) / TILE_SIZE);
    moveStr += std::to_string(toRank((oldPos.y - OFFSET.y) / TILE_SIZE));
    moveStr += toFile((newPos.x - OFFSET.x) / TILE_SIZE);
    moveStr += std::to_string(toRank((newPos.y - OFFSET.y) / TILE_SIZE));

    int pieceType = std::abs(pieces[pieceIdx].index);
    int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;

    if (pieceType == 6 && (newY_grid == 0 || newY_grid == 7)) {
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
    int newX_grid = (newPixelPos.x - OFFSET.x) / TILE_SIZE;
    int newY_grid = (newPixelPos.y - OFFSET.y) / TILE_SIZE;

    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX_grid - oldX_grid) == 2) {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
    } else if (std::abs(pieces[pieceIdx].index) == 6 && (newY_grid == 0 || newY_grid == 7) && promotionChar != ' ') {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
        int promotedType = 0;
        if (tolower(promotionChar) == 'q') promotedType = 4;
        else if (tolower(promotionChar) == 'r') promotedType = 1;
        else if (tolower(promotionChar) == 'b') promotedType = 3;
        else if (tolower(promotionChar) == 'n') promotedType = 2;
        pieces[pieceIdx].index = (pieces[pieceIdx].index > 0 ? 1 : -1) * promotedType;
        int pieceValue = 0;
        if (promotedType == 1) pieceValue = 50;
        else if (promotedType == 2) pieceValue = 30;
        else if (promotedType == 3) pieceValue = 30;
        else if (promotedType == 4) pieceValue = 90;
        pieces[pieceIdx].cost = (pieces[pieceIdx].index > 0 ? 1 : -1) * pieceValue;
    }
    else {
        movePiece(pieceIdx, oldPixelPos, newPixelPos);
    }
}
