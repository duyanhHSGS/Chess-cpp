#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <math.h>
#include <time.h>
#include <stack>
#include <algorithm>

const int TILE_SIZE = 56;
const SDL_FPoint OFFSET = {28.0f, 28.0f};

int board[8][8] = {
    {-1, -2, -3, -4, -5, -3, -2, -1},
    {-6, -6, -6, -6, -6, -6, -6, -6},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {6, 6, 6, 6, 6, 6, 6, 6},
    {1, 2, 3, 4, 5, 3, 2, 1}
};

struct ChessPiece {
    SDL_Texture* texture;
    SDL_Rect rect;
    int index;
    int cost;
};

class GameManager {
public:
    ChessPiece pieces[33];
    SDL_FPoint possibleMoves[32];
    int possibleMoveCount;
    std::stack<SDL_FPoint> positionStack;
    std::stack<int> pieceIndexStack;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* boardTexture = nullptr;
    SDL_Texture* figuresTexture = nullptr;
    SDL_Texture* positiveMoveTexture = nullptr;

    void initializeSDL();
    void cleanUpSDL();

    void movePiece(int pieceIdx, SDL_FPoint oldPos, SDL_FPoint newPos);
    void undoLastMove();
    void createPieces();
    void playGame();

    void findRookMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findBishopMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findKnightMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findKingMoves(int pieceIdx, int x, int y, int grid[9][9]);
    void findPawnMoves(int pieceIdx, int x, int y, int grid[9][9]);

    void addPossibleMove(int x, int y);
    void calculatePossibleMoves(int pieceIdx);

    int calculateBoardCost();
    int alphaBeta(int depth, bool isMaximizingPlayer, int alpha, int beta);
    SDL_FPoint getBestMove(bool isMaximizingPlayer);
};

void GameManager::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        exit(1);
    }

    window = SDL_CreateWindow("Chess - Alpha Beta Pruning Mode",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              504, 504,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    figuresTexture = IMG_LoadTexture(renderer, "img/figures.png");
    if (!figuresTexture) {
        std::cerr << "Failed to load figures.png! IMG_Error: " << IMG_GetError() << std::endl;
        cleanUpSDL();
        exit(1);
    }

    boardTexture = IMG_LoadTexture(renderer, "img/board1.png");
    if (!boardTexture) {
        std::cerr << "Failed to load board1.png! IMG_Error: " << IMG_GetError() << std::endl;
        cleanUpSDL();
        exit(1);
    }

    positiveMoveTexture = IMG_LoadTexture(renderer, "img/no.png");
    if (!positiveMoveTexture) {
        std::cerr << "Failed to load no.png! IMG_Error: " << IMG_GetError() << std::endl;
        cleanUpSDL();
        exit(1);
    }
}

void GameManager::cleanUpSDL() {
    if (positiveMoveTexture) SDL_DestroyTexture(positiveMoveTexture);
    if (boardTexture) SDL_DestroyTexture(boardTexture);
    if (figuresTexture) SDL_DestroyTexture(figuresTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

int GameManager::alphaBeta(int depth, bool isMaximizingPlayer, int alpha, int beta) {
    if (depth == 0) {
        return calculateBoardCost();
    }

    SDL_FPoint possibleMoveTemp[32];

    if (isMaximizingPlayer) {
        int bestMoveValue = -10000;
        for (int j = 16; j < 32; j++) {
            if (pieces[j].rect.x == -100 && pieces[j].rect.y == -100) continue;

            calculatePossibleMoves(j);
            int currentPossibleMoveCount = possibleMoveCount;
            possibleMoveCount = 0;

            for (int i = 0; i < currentPossibleMoveCount; i++) {
                possibleMoveTemp[i] = possibleMoves[i];
            }

            for (int i = 0; i < currentPossibleMoveCount; i++) {
                movePiece(j, {static_cast<float>(pieces[j].rect.x), static_cast<float>(pieces[j].rect.y)}, possibleMoveTemp[i]);
                bestMoveValue = std::max(bestMoveValue, alphaBeta(depth - 1, !isMaximizingPlayer, alpha, beta));
                undoLastMove();
                alpha = std::max(alpha, bestMoveValue);
                if (beta <= alpha) return bestMoveValue;
            }
        }
        return bestMoveValue;
    } else {
        int bestMoveValue = 10000;
        for (int j = 0; j < 16; j++) {
            if (pieces[j].rect.x == -100 && pieces[j].rect.y == -100) continue;

            calculatePossibleMoves(j);
            int currentPossibleMoveCount = possibleMoveCount;
            possibleMoveCount = 0;

            for (int i = 0; i < currentPossibleMoveCount; i++) {
                possibleMoveTemp[i] = possibleMoves[i];
            }

            for (int i = 0; i < currentPossibleMoveCount; i++) {
                movePiece(j, {static_cast<float>(pieces[j].rect.x), static_cast<float>(pieces[j].rect.y)}, possibleMoveTemp[i]);
                bestMoveValue = std::min(bestMoveValue, alphaBeta(depth - 1, !isMaximizingPlayer, alpha, beta));
                undoLastMove();
                beta = std::min(beta, bestMoveValue);
                if (beta <= alpha) return bestMoveValue;
            }
        }
        return bestMoveValue;
    }
}

void GameManager::addPossibleMove(int x, int y) {
    possibleMoves[possibleMoveCount] = {static_cast<float>(x * TILE_SIZE) + OFFSET.x, static_cast<float>(y * TILE_SIZE) + OFFSET.y};
    possibleMoveCount++;
}

void GameManager::movePiece(int pieceIdx, SDL_FPoint oldPos, SDL_FPoint newPos) {
    positionStack.push(oldPos);
    positionStack.push(newPos);
    pieceIndexStack.push(pieceIdx);

    int newY = static_cast<int>((newPos.y - OFFSET.y) / TILE_SIZE);

    if (newY == 0 && pieces[pieceIdx].index == 6) {
        pieceIndexStack.push(100);
        pieces[pieceIdx].index = 4;
        pieces[pieceIdx].cost = 90;
        pieces[pieceIdx].rect.w = TILE_SIZE;
        pieces[pieceIdx].rect.h = TILE_SIZE;
    }
    if (newY == 7 && pieces[pieceIdx].index == -6) {
        pieceIndexStack.push(-100);
        pieces[pieceIdx].index = -4;
        pieces[pieceIdx].cost = -90;
        pieces[pieceIdx].rect.w = TILE_SIZE;
        pieces[pieceIdx].rect.h = TILE_SIZE;
    }

    for (int i = 0; i < 32; i++) {
        SDL_Rect newPosRect = {static_cast<int>(newPos.x), static_cast<int>(newPos.y), TILE_SIZE, TILE_SIZE};
        if (SDL_HasIntersection(&pieces[i].rect, &newPosRect) && i != pieceIdx) {
            SDL_FPoint capturedOldPos = {static_cast<float>(pieces[i].rect.x), static_cast<float>(pieces[i].rect.y)};
            pieces[i].rect.x = -100;
            pieces[i].rect.y = -100;

            positionStack.push(capturedOldPos);
            positionStack.push({-100.0f, -100.0f});
            pieceIndexStack.push(i);
            break;
        }
    }
    pieces[pieceIdx].rect.x = static_cast<int>(newPos.x);
    pieces[pieceIdx].rect.y = static_cast<int>(newPos.y);
}

void GameManager::undoLastMove() {
    int pieceIdx = pieceIndexStack.top();
    pieceIndexStack.pop();

    SDL_FPoint newPos = positionStack.top();
    positionStack.pop();
    SDL_FPoint oldPos = positionStack.top();
    positionStack.pop();

    if (pieceIdx == 100) {
        pieceIdx = pieceIndexStack.top();
        pieceIndexStack.pop();
        pieces[pieceIdx].index = 6;
        pieces[pieceIdx].cost = 10;
    }
    if (pieceIdx == -100) {
        pieceIdx = pieceIndexStack.top();
        pieceIndexStack.pop();
        pieces[pieceIdx].index = -6;
        pieces[pieceIdx].cost = -10;
    }

    pieces[pieceIdx].rect.x = static_cast<int>(oldPos.x);
    pieces[pieceIdx].rect.y = static_cast<int>(oldPos.y);

    if (newPos.x == -100.0f && newPos.y == -100.0f) {
        undoLastMove();
    }
}

void GameManager::createPieces() {
    possibleMoveCount = 0;
    int k = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int n = board[i][j];
            if (!n) continue;

            int xTex = abs(n) - 1;
            int yTex = n > 0 ? 1 : 0;

            pieces[k].texture = figuresTexture;
            pieces[k].index = n;
            pieces[k].rect = {xTex * TILE_SIZE, yTex * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            pieces[k].rect.x = j * TILE_SIZE + static_cast<int>(OFFSET.x);
            pieces[k].rect.y = i * TILE_SIZE + static_cast<int>(OFFSET.y);
            pieces[k].rect.w = TILE_SIZE;
            pieces[k].rect.h = TILE_SIZE;

            int pieceValue = 0;
            int pieceType = abs(pieces[k].index);
            if (pieceType == 1) pieceValue = 50;
            else if (pieceType == 2 || pieceType == 3) pieceValue = 30;
            else if (pieceType == 4) pieceValue = 90;
            else if (pieceType == 5) pieceValue = 900;
            else if (pieceType == 6) pieceValue = 10;
            pieces[k].cost = pieces[k].index / pieceType * pieceValue;
            k++;
        }
    }
}

SDL_FPoint GameManager::getBestMove(bool isMaximizingPlayer) {
    SDL_FPoint oldPos, newPos, tempOldPos, tempNewPos;
    int minMaxTemp = 10000, minMax = 10000;
    int currentPossibleMoveCount, pieceToMoveIdx;
    SDL_FPoint possibleMoveTemp[32];

    for (int i = 0; i < 16; i++) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;

        calculatePossibleMoves(i);
        currentPossibleMoveCount = possibleMoveCount;
        possibleMoveCount = 0;

        for (int k = 0; k < currentPossibleMoveCount; k++) {
            possibleMoveTemp[k] = possibleMoves[k];
        }

        tempOldPos = {static_cast<float>(pieces[i].rect.x), static_cast<float>(pieces[i].rect.y)};
        minMaxTemp = 10000;

        for (int j = 0; j < currentPossibleMoveCount; j++) {
            movePiece(i, tempOldPos, possibleMoveTemp[j]);
            int alpha = -9999, beta = 9999;
            int tempScore = alphaBeta(3, !isMaximizingPlayer, alpha, beta);

            if (minMaxTemp > tempScore) {
                tempNewPos = possibleMoveTemp[j];
                minMaxTemp = tempScore;
            }
            undoLastMove();
        }

        if (minMax > minMaxTemp) {
            minMax = minMaxTemp;
            oldPos = tempOldPos;
            newPos = tempNewPos;
            pieceToMoveIdx = i;
        }
    }

    positionStack.push(oldPos);
    pieceIndexStack.push(pieceToMoveIdx);
    return newPos;
}

int GameManager::calculateBoardCost() {
    int totalCost = 0;
    for (int i = 0; i < 32; i++) {
        if (pieces[i].rect.x == -100 && pieces[i].rect.y == -100) continue;
        totalCost += pieces[i].cost;
    }
    return totalCost;
}

void GameManager::findPawnMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int direction = grid[x][y] / abs(grid[x][y]);

    if ((y == 1 || y == 6) && grid[x][y - direction] == 0 && grid[x][y - 2 * direction] == 0 && y - 2 * direction >= 0 && y - 2 * direction < 8) {
        addPossibleMove(x, y - 2 * direction);
    }
    if (grid[x][y - direction] == 0 && y - direction >= 0 && y - direction < 8) {
        addPossibleMove(x, y - direction);
    }
    if (x + 1 < 8 && y - direction >= 0 && y - direction < 8 && grid[x + 1][y - direction] * grid[x][y] < 0) {
        addPossibleMove(x + 1, y - direction);
    }
    if (x - 1 >= 0 && y - direction >= 0 && y - direction < 8 && grid[x - 1][y - direction] * grid[x][y] < 0) {
        addPossibleMove(x - 1, y - direction);
    }
}

void GameManager::findKingMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};

    for (int i = 0; i < 8; i++) {
        int newX = x + dx[i];
        int newY = y + dy[i];
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            if (grid[newX][newY] == 0 || grid[newX][newY] * grid[x][y] < 0) {
                addPossibleMove(newX, newY);
            }
        }
    }
}

void GameManager::findKnightMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    int dx[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    int dy[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; i++) {
        int newX = x + dx[i];
        int newY = y + dy[i];
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            if (grid[newX][newY] == 0 || grid[newX][newY] * grid[x][y] < 0) {
                addPossibleMove(newX, newY);
            }
        }
    }
}

void GameManager::findBishopMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    for (int i = x + 1, j = y + 1; (i < 8 && j < 8); i++, j++) {
        if (grid[i][j] != 0) {
            if (grid[i][j] * grid[x][y] < 0) addPossibleMove(i, j);
            break;
        }
        addPossibleMove(i, j);
    }
    for (int i = x - 1, j = y + 1; (i >= 0 && j < 8); i--, j++) {
        if (grid[i][j] != 0) {
            if (grid[i][j] * grid[x][y] < 0) addPossibleMove(i, j);
            break;
        }
        addPossibleMove(i, j);
    }
    for (int i = x + 1, j = y - 1; (i < 8 && j >= 0); i++, j--) {
        if (grid[i][j] != 0) {
            if (grid[i][j] * grid[x][y] < 0) addPossibleMove(i, j);
            break;
        }
        addPossibleMove(i, j);
    }
    for (int i = x - 1, j = y - 1; (i >= 0 && j >= 0); i--, j--) {
        if (grid[i][j] != 0) {
            if (grid[i][j] * grid[x][y] < 0) addPossibleMove(i, j);
            break;
        }
        addPossibleMove(i, j);
    }
}

void GameManager::findRookMoves(int pieceIdx, int x, int y, int grid[9][9]) {
    for (int i = x + 1; i < 8; i++) {
        if (grid[i][y] != 0) {
            if (grid[i][y] * grid[x][y] < 0) addPossibleMove(i, y);
            break;
        }
        addPossibleMove(i, y);
    }
    for (int i = x - 1; i >= 0; i--) {
        if (grid[i][y] != 0) {
            if (grid[i][y] * grid[x][y] < 0) addPossibleMove(i, y);
            break;
        }
        addPossibleMove(i, y);
    }
    for (int j = y + 1; j < 8; j++) {
        if (grid[x][j] != 0) {
            if (grid[x][j] * grid[x][y] < 0) addPossibleMove(x, j);
            break;
        }
        addPossibleMove(x, j);
    }
    for (int j = y - 1; j >= 0; j--) {
        if (grid[x][j] != 0) {
            if (grid[x][j] * grid[x][y] < 0) addPossibleMove(x, j);
            break;
        }
        addPossibleMove(x, j);
    }
}

void GameManager::calculatePossibleMoves(int pieceIdx) {
    SDL_FPoint currentPos = {static_cast<float>(pieces[pieceIdx].rect.x), static_cast<float>(pieces[pieceIdx].rect.y)};
    int x = static_cast<int>((currentPos.x - OFFSET.x) / TILE_SIZE);
    int y = static_cast<int>((currentPos.y - OFFSET.y) / TILE_SIZE);

    int grid[9][9];
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            grid[i][j] = 0;
        }
    }

    for (int j = 0; j < 32; j++) {
        if (pieces[j].rect.x != -100 && pieces[j].rect.y != -100) {
            int pieceGridX = static_cast<int>((pieces[j].rect.x - OFFSET.x) / TILE_SIZE);
            int pieceGridY = static_cast<int>((pieces[j].rect.y - OFFSET.y) / TILE_SIZE);
            if (pieceGridX >= 0 && pieceGridX < 8 && pieceGridY >= 0 && pieceGridY < 8) {
                    grid[pieceGridX][pieceGridY] = pieces[j].index;
            }
        }
    }

    int pieceType = abs(pieces[pieceIdx].index);
    if (pieceType == 1) findRookMoves(pieceIdx, x, y, grid);
    else if (pieceType == 2) findKnightMoves(pieceIdx, x, y, grid);
    else if (pieceType == 3) findBishopMoves(pieceIdx, x, y, grid);
    else if (pieceType == 4) {
        findRookMoves(pieceIdx, x, y, grid);
        findBishopMoves(pieceIdx, x, y, grid);
    } else if (pieceType == 5) findKingMoves(pieceIdx, x, y, grid);
    else findPawnMoves(pieceIdx, x, y, grid);
}

void GameManager::playGame() {
    initializeSDL();
    createPieces();

    bool isPlayerTurn = true;
    SDL_FPoint oldClickPos;
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
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    SDL_Point mousePos = {e.button.x, e.button.y};
                    clickCount++;

                    if (clickCount == 1) {
                        bool pieceSelected = false;
                        for (int i = 16; i < 32; i++) {
                            SDL_Rect mouseRect = {mousePos.x, mousePos.y, 1, 1};
                            if (SDL_HasIntersection(&pieces[i].rect, &mouseRect)) {
                                pieceSelected = true;
                                clickedPieceIdx = i;
                                oldClickPos = {static_cast<float>(pieces[clickedPieceIdx].rect.x), static_cast<float>(pieces[clickedPieceIdx].rect.y)};
                                break;
                            }
                        }
                        if (!pieceSelected) {
                            clickCount = 0;
                        } else {
                            calculatePossibleMoves(clickedPieceIdx);
                        }
                    } else if (clickCount == 2) {
                        int destX = (mousePos.x - static_cast<int>(OFFSET.x)) / TILE_SIZE;
                        int destY = (mousePos.y - static_cast<int>(OFFSET.y)) / TILE_SIZE;
                        SDL_FPoint newMovePos = {static_cast<float>(destX * TILE_SIZE) + OFFSET.x, static_cast<float>(destY * TILE_SIZE) + OFFSET.y};

                        bool validMove = false;
                        for (int i = 0; i < possibleMoveCount; i++) {
                            if (std::abs(possibleMoves[i].x - newMovePos.x) < 0.1f && std::abs(possibleMoves[i].y - newMovePos.y) < 0.1f) {
                                validMove = true;
                                break;
                            }
                        }

                        if (validMove) {
                            movePiece(clickedPieceIdx, oldClickPos, newMovePos);
                            isPlayerTurn = !isPlayerTurn;
                        }
                        possibleMoveCount = 0;
                        clickCount = 0;
                        clickedPieceIdx = -1;
                    }
                }
            }
        }

        if (!isPlayerTurn) {
            SDL_FPoint newAiPos = getBestMove(isPlayerTurn);
            int aiPieceIdx = pieceIndexStack.top();
            pieceIndexStack.pop();
            SDL_FPoint oldAiPos = positionStack.top();
            positionStack.pop();
            
            movePiece(aiPieceIdx, oldAiPos, newAiPos);
            isPlayerTurn = !isPlayerTurn;
            
            clickCount = 0;
            clickedPieceIdx = -1;
            possibleMoveCount = 0;
        }

        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, boardTexture, NULL, NULL);

        if (clickCount == 1 && clickedPieceIdx != -1) {
            for (int i = 0; i < possibleMoveCount; i++) {
                SDL_Rect destRect = {static_cast<int>(possibleMoves[i].x), static_cast<int>(possibleMoves[i].y), TILE_SIZE, TILE_SIZE};
                SDL_RenderCopy(renderer, positiveMoveTexture, NULL, &destRect);
            }
        }

        for (int i = 0; i < 32; i++) {
            int xTex = abs(pieces[i].index) - 1;
            int yTex = pieces[i].index > 0 ? 1 : 0;
            SDL_Rect srcRect = {xTex * TILE_SIZE, yTex * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            if (pieces[i].rect.x != -100 || pieces[i].rect.y != -100) {
                SDL_RenderCopy(renderer, pieces[i].texture, &srcRect, &pieces[i].rect);
            }
        }

        SDL_RenderPresent(renderer);
    }

    cleanUpSDL();
}

int main(int argc, char* args[]) {
    GameManager gameManager;
    gameManager.playGame();
    return 0;
}