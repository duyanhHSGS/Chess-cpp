#include "UciHandler.h"
#include "GameManager.h" // Full definition of GameManager is needed here.
#include <sstream>       // For std::istringstream for parsing
#include <algorithm>     // For std::transform, std::tolower

// Constructor: Initializes the GameManager reference.
UciHandler::UciHandler(GameManager& game_manager) : game_manager_(game_manager) {
    // No specific initialization for UCI protocol itself yet, just setup the reference.
}

// Helper to split a string into tokens by a delimiter.
std::vector<std::string> UciHandler::split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(str);
    while (std::getline(token_stream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Sends a line of output to stdout (for UCI responses).
void UciHandler::send_line(const std::string& message) {
    std::cout << message << std::endl;
}

// Sends an error message to stderr (for debugging/protocol violations).
void UciHandler::send_error(const std::string& message) {
    std::cerr << "ERROR: " << message << std::endl;
}

// Handles the "uci" command from the GUI.
// The engine should respond with its identity and options, then "uciok".
void UciHandler::handle_uci_command() {
    send_line("id name CarolynaChessEngine");
    send_line("id author Duy Anh"); // Updated with your name
    // TODO: Add UCI options here later (e.g., "option name Hash type spin default 16 min 1 max 2048")
    send_line("uciok");
}

// Handles the "isready" command.
// The engine should respond with "readyok" when it's ready for new commands.
void UciHandler::handle_isready_command() {
    send_line("readyok");
}

// Handles the "setoption" command.
// For now, we'll just parse it without implementing specific option changes.
void UciHandler::handle_setoption_command(const std::vector<std::string>& tokens) {
    // Expected format: setoption name <id> [value <v>]
    // Example: setoption name Hash value 128
    if (tokens.size() >= 4 && tokens[1] == "name") {
        std::string option_name;
        size_t value_pos = std::string::npos;
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i] == "value" && (i + 1) < tokens.size()) {
                value_pos = i;
                break;
            }
            if (i > 2) option_name += " "; // Add space for multi-word names
            option_name += tokens[i];
        }

        std::string option_value = (value_pos != std::string::npos) ? tokens[value_pos + 1] : "";
        // send_error("INFO: Received setoption: " + option_name + " = " + option_value); // For debugging
        // Here, you would store the option in GameManager or a dedicated Options struct.
    } else {
        send_error("Malformed setoption command.");
    }
}

// Handles the "ucinewgame" command.
// Resets the engine's internal state for a new game.
void UciHandler::handle_ucinewgame_command() {
    game_manager_.reset_game(); // Calls GameManager to reset its state.
    // send_error("INFO: New game started (UCI)."); // For debugging
}

// Handles the "position" command.
// Sets the board to a specific FEN or startpos, and then applies a sequence of moves.
void UciHandler::handle_position_command(const std::vector<std::string>& tokens) {
    // Expected formats:
    // position startpos moves e2e4 d2d4 ...
    // position fen <fenstring> moves e2e4 d2d4 ...

    if (tokens.size() < 2) {
        send_error("Malformed position command.");
        return;
    }

    std::string fen_or_startpos = tokens[1];
    std::string fen_string = "";
    std::vector<std::string> moves_list;

    size_t moves_idx = std::string::npos;

    if (fen_or_startpos == "startpos") {
        fen_string = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; // Standard startpos FEN
        // Find "moves" keyword
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i] == "moves") {
                moves_idx = i;
                break;
            }
        }
    } else if (fen_or_startpos == "fen") {
        if (tokens.size() < 3) {
            send_error("Malformed fen position command: missing FEN string.");
            return;
        }
        // Reconstruct FEN string (can be multiple tokens if it contains spaces before 'moves')
        size_t current_token_idx = 2;
        while (current_token_idx < tokens.size() && tokens[current_token_idx] != "moves") {
            fen_string += tokens[current_token_idx] + " ";
            current_token_idx++;
        }
        // Remove trailing space if any
        if (!fen_string.empty()) {
            fen_string.pop_back();
        }
        
        moves_idx = current_token_idx; // "moves" keyword index
    } else {
        send_error("Unknown position type: " + fen_or_startpos);
        return;
    }

    // Collect moves if "moves" keyword was found
    if (moves_idx != std::string::npos && (moves_idx + 1) < tokens.size()) {
        for (size_t i = moves_idx + 1; i < tokens.size(); ++i) {
            moves_list.push_back(tokens[i]);
        }
    }
    
    // Call GameManager to set the board and apply moves.
    game_manager_.set_board_from_uci_position(fen_string, moves_list);
    // send_error("INFO: Position set. FEN: " + fen_string + ", Moves: " + std::to_string(moves_list.size())); // For debugging
}

// Handles the "go" command.
// Tells the engine to start searching for the best move.
// For now, we'll just find a random legal move and output it.
void UciHandler::handle_go_command(const std::vector<std::string>& tokens) {
    // Parse go parameters (wtime, btime, movetime, depth, infinite, etc.)
    // For now, we ignore them.
    // TODO: Implement actual search and move selection here.

    // Call GameManager to find the best move.
    Move best_move = game_manager_.find_best_move(); // This will return a random legal move for now.

    // Output the best move in UCI format.
    std::string best_move_str = "";
    if (best_move.from_square.x != 64) { // Check if a valid move was found
        best_move_str += (char)('a' + best_move.from_square.x);
        best_move_str += std::to_string(best_move.from_square.y + 1);
        best_move_str += (char)('a' + best_move.to_square.x);
        best_move_str += std::to_string(best_move.to_square.y + 1);
        if (best_move.is_promotion) {
            switch (best_move.promotion_piece_type_idx) {
                case PieceTypeIndex::QUEEN: best_move_str += "q"; break;
                case PieceTypeIndex::ROOK: best_move_str += "r"; break;
                case PieceTypeIndex::BISHOP: best_move_str += "b"; break;
                case PieceTypeIndex::KNIGHT: best_move_str += "n"; break;
                default: break; // Should not happen
            }
        }
    } else {
        // No legal moves found, implies game is over (checkmate/stalemate)
        // In UCI, if no legal moves, no bestmove is returned. However, for a simple random AI,
        // we might still output 'bestmove (none)' or similar if it's a draw/loss.
        // For now, we'll just skip printing bestmove if no move was found.
        send_error("ERROR: No legal moves found for 'go' command, cannot find bestmove.");
        return;
    }
    send_line("bestmove " + best_move_str);
}

// Handles the "quit" command.
void UciHandler::handle_quit_command() {
    // The main loop will break and exit.
    // send_error("INFO: Quitting engine (UCI)."); // For debugging
}

// The main loop that reads UCI commands from stdin and dispatches them.
void UciHandler::run_uci_loop() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> tokens = split_string(line);
        if (tokens.empty()) {
            continue;
        }

        const std::string& command = tokens[0];

        if (command == "uci") {
            handle_uci_command();
        } else if (command == "isready") {
            handle_isready_command();
        } else if (command == "setoption") {
            handle_setoption_command(tokens);
        } else if (command == "ucinewgame") {
            handle_ucinewgame_command();
        } else if (command == "position") {
            handle_position_command(tokens);
        } else if (command == "go") {
            handle_go_command(tokens);
        } else if (command == "quit") {
            handle_quit_command();
            break; // Exit the loop
        } else {
            send_error("Unknown UCI command: " + command);
        }
    }
}
