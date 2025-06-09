#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <string>
#include <vector>

#include "ChessBoard.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "ChessAI.h"
#include "UciHandler.h"

class GameManager {
public:
    GameManager();

    void run();

private:
    ChessBoard board;
    ChessAI chess_ai;
    UciHandler uci_handler; 

    void handleUciCommand();
    void handleIsReadyCommand();
    void handleUciNewGameCommand();
    void handlePositionCommand(const std::string& command_line);
    void handleGoCommand();
};

#endif // GAME_MANAGER_H
