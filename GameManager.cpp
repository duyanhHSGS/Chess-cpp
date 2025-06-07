#include "GameManager.h"    // Include the header for GameManager itself
#include "UciHandler.h"     // Include the header for UciHandler to perform I/O
#include "ChessBitboardUtils.h" // For move_to_string and initialize_attack_tables
#include <iostream>         // Required for std::cin, std::cout
#include <sstream>          // Required for std::stringstream for parsing
#include <random>           // Required for std::mt19937_64, std::uniform_int_distribution
#include <algorithm>        // For std::find, or other algorithms as needed


// Constructor for GameManager.
// It ensures that global/static resources (like attack tables) are initialized
// and sets up the initial board state.
GameManager::GameManager()
    : board(),           // Initialize the ChessBoard member
      chess_ai()         // Initialize the ChessAI member
{
    // Initialize static attack tables for various pieces once.
    // This is a crucial step for performance and should happen before
    // any board or move generation operations.
    // It's safe to call this here, as ChessBitboardUtils internally
    // ensures it's only initialized once via a static flag.
    ChessBitboardUtils::initialize_attack_tables();
    // The ChessBoard `board` member's constructor sets up the starting position and Zobrist keys.
}

// The main loop that drives the engine.
// It continuously reads commands from standard input (via UciHandler conceptually)
// and dispatches them to appropriate internal handlers.
void GameManager::run() {
    // Create an instance of UciHandler.
    // This object is responsible for all low-level UCI I/O.
    UciHandler uci_handler;

    // Main loop: Continuously read lines from standard input (where the GUI sends commands).
    std::string line;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line); // Use stringstream to parse the command line.
        std::string command;
        ss >> command; // Extract the primary command keyword.

        // Dispatch commands to dedicated handler methods.
        // This makes the `run` method clean and focuses on command routing.
        if (command == "uci") {
            handleUciCommand();
        } else if (command == "isready") {
            handleIsReadyCommand();
        } else if (command == "ucinewgame") {
            handleUciNewGameCommand();
        } else if (command == "position") {
            // Pass the entire command line to `handlePositionCommand` for parsing.
            handlePositionCommand(line);
        } else if (command == "go") {
            handleGoCommand();
        } else if (command == "quit") {
            break; // Exit the main loop, terminating the engine.
        } else {
            // For any unknown commands, you might log an error or simply ignore them.
            // For a UCI engine, silence is often preferred for unknown commands.
            // uci_handler.sendInfo("Unknown command: " + command);
        }
    }
}

// Handles the 'uci' command.
void GameManager::handleUciCommand() {
    UciHandler uci_handler; // Create a temporary UciHandler for this scope.
    uci_handler.sendUciIdentity(); // Send engine name and author.
    uci_handler.sendUciOk();       // Signal UCI initialization is complete.
}

// Handles the 'isready' command.
void GameManager::handleIsReadyCommand() {
    UciHandler uci_handler;
    // Perform any readiness checks here if needed (e.g., confirm all tables are loaded).
    uci_handler.sendReadyOk(); // Signal that the engine is ready.
}

// Handles the 'ucinewgame' command.
void GameManager::handleUciNewGameCommand() {
    // Reset the internal ChessBoard to its starting position.
    board.reset_to_start_position();
    // In a more complex engine, you would also clear transposition tables,
    // reset history heuristics, etc., here.
}

// Handles the 'position' command.
// This method parses the FEN/startpos and then applies any subsequent moves.
void GameManager::handlePositionCommand(const std::string& command_line) {
    std::stringstream ss(command_line);
    std::string token;
    ss >> token; // Consume "position"

    std::string sub_command;
    ss >> sub_command; // Read "startpos" or "fen"

    std::string temp_word; // Used to check for "moves" keyword.

    if (sub_command == "startpos") {
        board.reset_to_start_position(); // Set board to the standard starting position.
        ss >> temp_word; // Try to read "moves"
    } else if (sub_command == "fen") {
        std::string fen_string_part;
        // Read FEN components until "moves" keyword or end of line.
        while (ss >> temp_word && temp_word != "moves") {
            if (!fen_string_part.empty()) {
                fen_string_part += " "; // Add space between FEN components.
            }
            fen_string_part += temp_word;
        }
        board.set_from_fen(fen_string_part); // Initialize board from the parsed FEN.
    } else {
        // Handle invalid 'position' sub-command.
        // For robustness, you might send an info message or log this.
        return;
    }

    // If 'moves' keyword was found (either after 'startpos' or 'fen').
    if (temp_word == "moves") {
        std::string move_str;
        MoveGenerator move_gen; // Create a MoveGenerator to validate moves.
        // Apply each move specified in the command line.
        while (ss >> move_str) {
            std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);
            bool move_found = false;
            for (const auto& legal_move : legal_moves) {
                if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
                    StateInfo info_for_undo; // StateInfo to correctly undo the move later if needed (e.g., in search).
                    board.apply_move(legal_move, info_for_undo); // Apply the move to the board.
                    move_found = true;
                    break;
                }
            }
            if (!move_found) {
                // If a move from the position command is not found, it indicates an issue.
                // In a real engine, this might be an error or simply an illegal move from the GUI.
                // We'll break to prevent further incorrect state updates.
                break;
            }
        }
    }
}

// Handles the 'go' command.
// This is where the AI search would be initiated.
void GameManager::handleGoCommand() {
    UciHandler uci_handler;
    
    // Request the best move from the ChessAI instance.
    // The ChessAI handles its own move generation (using MoveGenerator internally).
    Move best_move = chess_ai.findBestMove(board);

    // After receiving the best move, check if it's a valid move.
    // If the AI returned an invalid move (e.g., default-constructed Move::NONE),
    // it implies no legal moves were found (checkmate/stalemate).
    if (best_move.piece_moved_type_idx == PieceTypeIndex::NONE) {
        uci_handler.sendBestMove("(none)"); // Indicate no move can be made.
    } else {
        // Send the chosen move back to the GUI via UciHandler.
        uci_handler.sendBestMove(ChessBitboardUtils::move_to_string(best_move));
    }
}
