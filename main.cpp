#include <iostream>    // Required for input/output operations (std::cin, std::cout)
#include <string>      // Required for std::string (though not directly used in this minimal main, good for future expansion)

// Include the new GameManager header, which orchestrates the engine logic
#include "GameManager.h"

// The main entry point of your UCI chess engine.
// This function is now much cleaner as it primarily delegates control to the GameManager.
int main() {
    // Create an instance of GameManager.
    // The GameManager's constructor handles the initialization of the ChessBoard
    // and ensures that ChessBitboardUtils::initialize_attack_tables() is called.
    GameManager game_manager;

    // Start the main engine loop.
    // The GameManager::run() method will now handle reading UCI commands,
    // parsing them, updating the board, and interacting with the AI.
    game_manager.run();

    return 0; // Successful program termination.
}
