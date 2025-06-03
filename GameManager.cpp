#include "GameManager.h"
#include "ChessAI.h"
#include "Constants.h"
#include "ChessPiece.h"

#include <algorithm>
#include <SDL_log.h> // Include SDL_log.h for SDL_Log

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
    window = nullptr;
    renderer = nullptr;
    boardTexture = nullptr;
    figuresTexture = nullptr;
    positiveMoveTexture = nullptr;
    redTexture = nullptr;
}

GameManager::~GameManager() {
    cleanUpSDL();
}

void GameManager::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        exit(1);
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("SDL_image could not initialize! IMG_Error: %s", IMG_GetError());
        SDL_Quit();
        exit(1);
    }

    window = SDL_CreateWindow("Chess - Alpha Beta Pruning Mode",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              TILE_SIZE * 9, TILE_SIZE * 9,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    figuresTexture = IMG_LoadTexture(renderer, "img/figures.png");
    if (!figuresTexture) {
        SDL_Log("Failed to load figures.png! IMG_Error: %s", IMG_GetError());
        cleanUpSDL();
        exit(1);
    }

    boardTexture = IMG_LoadTexture(renderer, "img/board1.png");
    if (!boardTexture) {
        SDL_Log("Failed to load board1.png! IMG_Error: %s", IMG_GetError());
        cleanUpSDL();
        exit(1);
    }

    positiveMoveTexture = IMG_LoadTexture(renderer, "img/no.png");
    if (!positiveMoveTexture) {
        SDL_Log("Failed to load no.png! IMG_Error: %s", IMG_GetError());
        cleanUpSDL();
        exit(1);
    }

    redTexture = IMG_LoadTexture(renderer, "img/red.png");
    if (!redTexture) {
        SDL_Log("Failed to load red.png! IMG_Error: %s", IMG_GetError());
        cleanUpSDL();
        exit(1);
    }
}

void GameManager::cleanUpSDL() {
    if (positiveMoveTexture) SDL_DestroyTexture(positiveMoveTexture);
    if (boardTexture) SDL_DestroyTexture(boardTexture);
    if (figuresTexture) SDL_DestroyTexture(figuresTexture);
    if (redTexture) SDL_DestroyTexture(redTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void GameManager::createPieces() {
    possibleMoveCount = 0;
    int k = 0;

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            int n = initial_board[i][j];

            if (n == 0) continue;

            int xTex = std::abs(n) - 1;
            int yTex = n > 0 ? 1 : 0;

            pieces[k].texture = figuresTexture;
            pieces[k].index = n;
            pieces[k].rect = {xTex * TILE_SIZE, yTex * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            pieces[k].rect.x = j * TILE_SIZE + OFFSET.x;
            pieces[k].rect.y = i * TILE_SIZE + OFFSET.y;
            pieces[k].rect.w = TILE_SIZE;
            pieces[k].rect.h = TILE_SIZE;

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
}

void GameManager::movePiece(int pieceIdx, SDL_Point oldPos, SDL_Point newPos) {
    int oldX_grid = (oldPos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (oldPos.y - OFFSET.y) / TILE_SIZE;
    int newX_grid = (newPos.x - OFFSET.x) / TILE_SIZE;
    int newY_grid = (newPos.y - OFFSET.y) / TILE_SIZE;

    positionStack.push(enPassantTargetSquare);
    pieceIndexStack.push(enPassantPawnIndex);
    pieceIndexStack.push(400);

    enPassantTargetSquare = {-1, -1};
    enPassantPawnIndex = -1;

    if (std::abs(pieces[pieceIdx].index) == 6 &&
        std::abs(newX_grid - oldX_grid) == 1 &&
        std::abs(newY_grid - oldY_grid) == 1 &&
        (newPos.x == (enPassantTargetSquare.x * TILE_SIZE + OFFSET.x) && newPos.y == (enPassantTargetSquare.y * TILE_SIZE + OFFSET.y)) &&
        pieces[pieceIdx].index * pieces[enPassantPawnIndex].index < 0)
    {
        SDL_Point capturedPawnOldPos = {pieces[enPassantPawnIndex].rect.x, pieces[enPassantPawnIndex].rect.y};
        pieces[enPassantPawnIndex].rect.x = -100;
        pieces[enPassantPawnIndex].rect.y = -100;

        positionStack.push(capturedPawnOldPos);
        positionStack.push({-100, -100});
        pieceIndexStack.push(enPassantPawnIndex);
        pieceIndexStack.push(300);
    }

    positionStack.push(oldPos);
    positionStack.push(newPos);
    pieceIndexStack.push(pieceIdx);

    if (std::abs(pieces[pieceIdx].index) == 5 && std::abs(newX_grid - oldX_grid) == 2) {
        int rookIdx = -1;
        SDL_Point rookOldPos;
        SDL_Point rookNewPos;

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
            positionStack.push(rookOldPos);
            positionStack.push(rookNewPos);
            pieceIndexStack.push(rookIdx);
            pieces[rookIdx].rect.x = rookNewPos.x;
            pieces[rookIdx].rect.y = rookNewPos.y;
            pieceIndexStack.push(200);
        }
    }

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

    if (newY_grid == 0 && pieces[pieceIdx].index == 6) {
        pieceIndexStack.push(100);
        pieces[pieceIdx].index = 4;
        pieces[pieceIdx].cost = 90;
    }
    if (newY_grid == 7 && pieces[pieceIdx].index == -6) {
        pieceIndexStack.push(-100);
        pieces[pieceIdx].index = -4;
        pieces[pieceIdx].cost = -90;
    }

    SDL_Rect newPosRect = {newPos.x, newPos.y, TILE_SIZE, TILE_SIZE};
    for (int i = 0; i < 32; ++i) {
        if (SDL_HasIntersection(&pieces[i].rect, &newPosRect) && i != pieceIdx) {
            SDL_Point capturedOldPos = {pieces[i].rect.x, pieces[i].rect.y};
            pieces[i].rect.x = -100;
            pieces[i].rect.y = -100;

            positionStack.push(capturedOldPos);
            positionStack.push({-100, -100});
            pieceIndexStack.push(i);
            break;
        }
    }

    pieces[pieceIdx].rect.x = newPos.x;
    pieces[pieceIdx].rect.y = newPos.y;

    if (std::abs(pieces[pieceIdx].index) == 6 && std::abs(newY_grid - oldY_grid) == 2) {
        enPassantTargetSquare = {newX_grid, (newY_grid + oldY_grid) / 2};
        enPassantPawnIndex = pieceIdx;
    }
}

void GameManager::undoLastMove() {
    if (pieceIndexStack.empty()) {
        SDL_Log("No moves to undo.");
        return;
    }

    int pieceIdx = pieceIndexStack.top();
    pieceIndexStack.pop();

    if (pieceIdx == 400) {
        enPassantPawnIndex = pieceIndexStack.top(); pieceIndexStack.pop();
        enPassantTargetSquare = positionStack.top(); positionStack.pop();
        undoLastMove();
        return;
    }

    if (pieceIdx == 200) {
        int rookMovedIdx = pieceIndexStack.top(); pieceIndexStack.pop();
        SDL_Point rookNewPos = positionStack.top(); positionStack.pop();
        SDL_Point rookOldPos = positionStack.top(); positionStack.pop();
        pieces[rookMovedIdx].rect.x = rookOldPos.x;
        pieces[rookMovedIdx].rect.y = rookOldPos.y;

        undoLastMove();
        return;
    }

    if (pieceIdx == 300) {
        int capturedPawnIdx = pieceIndexStack.top(); pieceIndexStack.pop();
        SDL_Point capturedPawnNewPos = positionStack.top(); positionStack.pop();
        SDL_Point capturedPawnOldPos = positionStack.top(); positionStack.pop();
        pieces[capturedPawnIdx].rect.x = capturedPawnOldPos.x;
        pieces[capturedPawnIdx].rect.y = capturedPawnOldPos.y;
        undoLastMove();
        return;
    }

    SDL_Point newPos = positionStack.top();
    positionStack.pop();
    SDL_Point oldPos = positionStack.top();
    positionStack.pop();

    if (pieceIdx == 100) {
        pieceIdx = pieceIndexStack.top();
        pieceIndexStack.pop();
        pieces[pieceIdx].index = 6;
        pieces[pieceIdx].cost = 10;
    } else if (pieceIdx == -100) {
        pieceIdx = pieceIndexStack.top();
        pieceIndexStack.pop();
        pieces[pieceIdx].index = -6;
        pieces[pieceIdx].cost = -10;
    }

    pieces[pieceIdx].rect.x = oldPos.x;
    pieces[pieceIdx].rect.y = oldPos.y;

    if (newPos.x == -100 && newPos.y == -100) {
        undoLastMove();
    }
}

void GameManager::addPossibleMove(int x, int y) {
    if (possibleMoveCount < 32) {
        possibleMoves[possibleMoveCount] = {x * TILE_SIZE + OFFSET.x, y * TILE_SIZE + OFFSET.y};
        possibleMoveCount++;
    } else {
        SDL_Log("Warning: possibleMoves array full. Some moves might be missed.");
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
    SDL_Point kingPos = getKingPosition(kingColor);
    if (kingPos.x == -1 || kingPos.y == -1) {
        return false;
    }

    int kingX = (kingPos.x - OFFSET.x) / TILE_SIZE;
    int kingY = (kingPos.y - OFFSET.y) / TILE_SIZE;

    int attackingColor = -kingColor;

    return isSquareAttacked(kingX, kingY, attackingColor);
}

SDL_Point GameManager::getKingPosition(int kingColor) const {
    int kingIndex = kingColor * 5;
    for (int i = 0; i < 32; ++i) {
        if (pieces[i].index == kingIndex && (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)) {
            return {pieces[i].rect.x, pieces[i].rect.y};
        }
    }
    return {-1, -1};
}

SDL_Point GameManager::getRookPosition(int rookColor, bool isKingside) const {
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

    SDL_Point currentPos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
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
    SDL_Point originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
    SDL_Rect tempNewPosRect = {newX * TILE_SIZE + OFFSET.x, newY * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

    SDL_Point originalTargetPiecePos = {-100, -100};
    int originalTargetPieceIdx = -1;

    for (int p = 0; p < 32; ++p) {
        if (SDL_HasIntersection(&pieces[p].rect, &tempNewPosRect) && p != pieceIdx) {
            originalTargetPiecePos = {pieces[p].rect.x, pieces[p].rect.y};
            originalTargetPieceIdx = p;
            pieces[p].rect.x = -100;
            pieces[p].rect.y = -100;
            break;
        }
    }

    int oldX_grid = (originalPiecePos.x - OFFSET.x) / TILE_SIZE;
    int oldY_grid = (originalPiecePos.y - OFFSET.y) / TILE_SIZE;
    int pawnDirection = (pieceColor == 1) ? -1 : 1;

    if (std::abs(pieces[pieceIdx].index) == 6 &&
        std::abs(newX - oldX_grid) == 1 &&
        std::abs(newY - oldY_grid) == 1 &&
        newX == enPassantTargetSquare.x && newY == enPassantTargetSquare.y &&
        enPassantPawnIndex != -1)
    {
        originalTargetPieceIdx = enPassantPawnIndex;
        originalTargetPiecePos = {pieces[originalTargetPieceIdx].rect.x, pieces[originalTargetPieceIdx].rect.y};
        pieces[originalTargetPieceIdx].rect.x = -100;
        pieces[originalTargetPieceIdx].rect.y = -100;
    }

    pieces[pieceIdx].rect.x = tempNewPosRect.x;
    pieces[pieceIdx].rect.y = tempNewPosRect.y;

    bool isLegal = !isKingInCheck(pieceColor);

    pieces[pieceIdx].rect.x = originalPiecePos.x;
    pieces[pieceIdx].rect.y = originalPiecePos.y;
    if (originalTargetPieceIdx != -1) {
        pieces[originalTargetPieceIdx].rect.x = originalTargetPiecePos.x;
        pieces[originalTargetPieceIdx].rect.y = originalTargetPiecePos.y;
    }
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
            SDL_Point originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            SDL_Rect tempNewPosRect = {(x - 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

            SDL_Point originalCapturedPawnPos = {pieces[enPassantPawnIndex].rect.x, pieces[enPassantPawnIndex].rect.y};
            pieces[enPassantPawnIndex].rect.x = -100;
            pieces[enPassantPawnIndex].rect.y = -100;

            pieces[pieceIdx].rect.x = tempNewPosRect.x;
            pieces[pieceIdx].rect.y = tempNewPosRect.y;

            if (!isKingInCheck(pieceColor)) {
                addPossibleMove(x - 1, y + direction);
            }

            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;
            pieces[enPassantPawnIndex].rect.x = originalCapturedPawnPos.x;
            pieces[enPassantPawnIndex].rect.y = originalCapturedPawnPos.y;
        }
        if (x + 1 < 8 &&
            (enPassantTargetSquare.x == x + 1 && enPassantTargetSquare.y == y + direction) &&
            std::abs(pieces[enPassantPawnIndex].index) == 6 &&
            pieces[enPassantPawnIndex].index * pieceColor < 0)
        {
            SDL_Point originalPiecePos = {pieces[pieceIdx].rect.x, pieces[pieceIdx].rect.y};
            SDL_Rect tempNewPosRect = {(x + 1) * TILE_SIZE + OFFSET.x, (y + direction) * TILE_SIZE + OFFSET.y, TILE_SIZE, TILE_SIZE};

            SDL_Point originalCapturedPawnPos = {pieces[enPassantPawnIndex].rect.x, pieces[enPassantPawnIndex].rect.y};
            pieces[enPassantPawnIndex].rect.x = -100;
            pieces[enPassantPawnIndex].rect.y = -100;

            pieces[pieceIdx].rect.x = tempNewPosRect.x;
            pieces[pieceIdx].rect.y = tempNewPosRect.y;

            if (!isKingInCheck(pieceColor)) {
                addPossibleMove(x + 1, y + direction);
            }

            pieces[pieceIdx].rect.x = originalPiecePos.x;
            pieces[pieceIdx].rect.y = originalPiecePos.y;
            pieces[enPassantPawnIndex].rect.x = originalCapturedPawnPos.x;
            pieces[enPassantPawnIndex].rect.y = originalCapturedPawnPos.y;
        }
    }
}

void GameManager::renderPieces() {
    for (int i = 0; i < 32; ++i) {
        if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
            int xTex = std::abs(pieces[i].index) - 1;
            int yTex = pieces[i].index > 0 ? 1 : 0;
            SDL_Rect srcRect = {xTex * TILE_SIZE, yTex * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            SDL_RenderCopy(renderer, pieces[i].texture, &srcRect, &pieces[i].rect);
        }
    }
}

void GameManager::playGame() {
    initializeSDL();
    createPieces();

    // Human controls White, AI controls Black
    PlayerColor currentPlayerTurn = PlayerColor::White;

    SDL_Point oldClickPos;
    int clickedPieceIdx = -1;
    int clickCount = 0;

    SDL_Event e;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    undoLastMove();
                    currentPlayerTurn = (currentPlayerTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
                    clickCount = 0;
                    clickedPieceIdx = -1;
                    possibleMoveCount = 0;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePos = {e.button.x, e.button.y};
                    clickCount++;

                    if (clickCount == 1) {
                        bool pieceSelected = false;
                        int startIdx = (currentPlayerTurn == PlayerColor::White) ? 16 : 0;
                        int endIdx = (currentPlayerTurn == PlayerColor::White) ? 32 : 16;

                        for (int i = startIdx; i < endIdx; ++i) {
                            if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100)
                            {
                                SDL_Rect mouseRect = {mousePos.x, mousePos.y, 1, 1};
                                if (SDL_HasIntersection(&pieces[i].rect, &mouseRect)) {
                                    pieceSelected = true;
                                    clickedPieceIdx = i;
                                    oldClickPos = {pieces[clickedPieceIdx].rect.x, pieces[clickedPieceIdx].rect.y};
                                    break;
                                }
                            }
                        }
                        if (!pieceSelected) {
                            clickCount = 0;
                        } else {
                            calculatePossibleMoves(clickedPieceIdx);
                        }
                    } else if (clickCount == 2) {
                        int destX = (mousePos.x - OFFSET.x) / TILE_SIZE;
                        int destY = (mousePos.y - OFFSET.y) / TILE_SIZE;
                        SDL_Point newMovePos = {destX * TILE_SIZE + OFFSET.x, destY * TILE_SIZE + OFFSET.y};

                        bool validMove = false;
                        for (int i = 0; i < possibleMoveCount; ++i) {
                            if (possibleMoves[i].x == newMovePos.x && possibleMoves[i].y == newMovePos.y) {
                                validMove = true;
                                break;
                            }
                        }

                        if (validMove) {
                            movePiece(clickedPieceIdx, oldClickPos, newMovePos);
                            // Get and print evaluation after human move
                            int currentEvaluation = ai->getEvaluation(this);
                            SDL_Log("After %s make a move, the current evaluation point is %+d",
                                    (currentPlayerTurn == PlayerColor::White ? "White" : "Black"), currentEvaluation);

                            currentPlayerTurn = (currentPlayerTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
                        }
                        possibleMoveCount = 0;
                        clickCount = 0;
                        clickedPieceIdx = -1;
                    }
                }
            }
        }

        PlayerColor AI_COLOR = (HUMAN_PLAYER_COLOR == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
        if (currentPlayerTurn == AI_COLOR) {
            SDL_Log("Thinking...");
            for (int i = 1; i <= 4; ++i) { // Simulate thinking time with incremental logs
                SDL_Log("Thinking... currently took %d", i);
                SDL_Delay(1000); // Delay for 1 second
            }
            SDL_Log("thinking completed, its the player's turn");

            bool isMaximizingPlayer = (AI_COLOR == PlayerColor::White);
            SDL_Point newAiPos = ai->getBestMove(this, isMaximizingPlayer);

            int aiPieceIdx = pieceIndexStack.top();
            pieceIndexStack.pop();
            SDL_Point oldAiPos = positionStack.top();
            positionStack.pop();

            movePiece(aiPieceIdx, oldAiPos, newAiPos);
            
            // Get and print evaluation after AI move
            int currentEvaluation = ai->getEvaluation(this);
            SDL_Log("After %s make a move, the current evaluation point is %+d",
                    (AI_COLOR == PlayerColor::White ? "White" : "Black"), currentEvaluation);

            currentPlayerTurn = (HUMAN_PLAYER_COLOR == PlayerColor::White) ? PlayerColor::White : PlayerColor::Black;

            clickCount = 0;
            clickedPieceIdx = -1;
            possibleMoveCount = 0;
        }

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, boardTexture, NULL, NULL);

        if (clickCount == 1 && clickedPieceIdx != -1) {
            for (int i = 0; i < possibleMoveCount; ++i) {
                SDL_Rect destRect = {possibleMoves[i].x, possibleMoves[i].y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, positiveMoveTexture, NULL, &destRect);
            }
        }

        int whiteKingColor = 1;
        if (isKingInCheck(whiteKingColor)) {
            SDL_Point kingPos = getKingPosition(whiteKingColor);
            if (kingPos.x != -1 && kingPos.y != -1) {
                SDL_Rect destRect = {kingPos.x, kingPos.y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, redTexture, NULL, &destRect);
            }
        }

        int blackKingColor = -1;
        if (isKingInCheck(blackKingColor)) {
            SDL_Point kingPos = getKingPosition(blackKingColor);
            if (kingPos.x != -1 && kingPos.y != -1) {
                SDL_Rect destRect = {kingPos.x, kingPos.y, TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, redTexture, NULL, &destRect);
            }
        }

        renderPieces();

        SDL_RenderPresent(renderer);
    }

    cleanUpSDL();
}
