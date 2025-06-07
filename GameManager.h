#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "ChessBoard.h"     // For ChessBoard class
#include "MoveGenerator.h"  // For MoveGenerator class
#include "Types.h"          // For GameStatus enum class
#include "Move.h"           // For Move struct
#include "UciHandler.h"     // For UciHandler class (needed for private member)
#include <vector>           // For std::vector
#include <string>           // For std::string
#include <random>           // For std::random_device, std::mt19937, std::uniform_int_distribution

// The GameManager class orchestrates the overall game flow,
// manages game state, and integrates with the Chess AI and UCI protocol.
class GameManager {
public:
    // Constructor for GameManager.
    // Initializes the ChessBoard and MoveGenerator.
    GameManager();

    // Resets the game to the starting position and clears game history.
    // Called by UciHandler on "ucinewgame" command.
    void reset_game();

    // Sets the board position based on UCI "position" command.
    // Handles "startpos" or "fen" and applies subsequent moves.
    void set_board_from_uci_position(std::string fen_or_startpos, const std::vector<std::string>& moves_str);

    // Finds the best move for the current position.
    // For now, this will return a random legal move.
    // In the future, this will trigger the main search algorithm.
    Move find_best_move();

private:
    // The current state of the chessboard.
    ChessBoard current_board;
    // The move generator instance used to find legal moves.
    MoveGenerator move_gen;
    // History of Zobrist hashes to detect threefold repetition.
    std::vector<uint64_t> position_history;
    // Random number generator for selecting a random move (placeholder for AI).
    std::mt19937 rng_;

    // Note: The UciHandler is NOT a member here. It is passed into GameManager's methods
    // or GameManager calls a global/singleton logger. For simplicity, we are passing it in main.cpp to GameManager.
    // However, the current UciHandler implementation calls GameManager methods.
    // To resolve this, GameManager should *not* have a UciHandler member directly.
    // Instead, it should rely on UciHandler calling its public methods.

    // Helper function to determine the current game status.
    GameStatus get_current_game_status(const std::vector<Move>& legal_moves) const;

    // Helper function to parse a move string (e.g., "e2e4") into a Move object.
    // This is distinct from UciHandler's parsing, as it needs board context.
    Move parse_algebraic_move(const std::string& move_str, const std::vector<Move>& legal_moves) const;
};

#endif // GAME_MANAGER_H
