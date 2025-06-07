// Core Chess Engine Component Includes
// These are the only external headers for chess logic.
#include "ChessBoard.h"         // Defines ChessBoard, StateInfo
#include "MoveGenerator.h"      // Defines MoveGenerator
#include "Types.h"              // Defines common types like PlayerColor, GamePoint, PieceTypeIndex, GameStatus
#include "Move.h"               // Defines the Move structure
#include "ChessBitboardUtils.h" // Provides utility functions for bitboard manipulation (e.g., print_bitboard, initialize_attack_tables)

// Standard Library Includes for I/O, Strings, Vectors, Randomness, etc.
#include <iostream>             // For std::cout, std::cin, std::cerr, std::getline
#include <string>               // For std::string and string manipulation
#include <vector>               // For std::vector (e.g., for legal moves, history)
#include <random>               // For random number generation (std::mt19937, std::random_device) - though not used for move selection in this version.
#include <algorithm>            // For algorithms like std::find_if, std::count
#include <limits>               // For std::numeric_limits (used with std::cin.ignore)
#include <cctype>               // For std::tolower (for promotion parsing)
#include <stdexcept>            // For standard exception types (e.g., std::runtime_error)
#include <sstream>              // For std::stringstream (if needed, though direct I/O is preferred here)


// --- Console I/O Helper Functions (formerly UciHandler methods, now global) ---
// These functions provide a consistent way to interact with the console.

// Reads a single line of input from standard input (console).
std::string console_read_line() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// Sends a line of output to standard output (console).
void console_send_line(const std::string& message) {
    std::cout << message << std::endl;
}

// Sends an error message to standard error (console), typically for debugging.
void console_send_error(const std::string& message) {
    std::cerr << "ERROR: " << message << std::endl;
}


// --- Game Logic Helper Functions (formerly GameManager methods, now global) ---
// These functions encapsulate specific parts of the game logic.

// Helper function to parse a user-input algebraic string (e.g., "e2e4", "e7e8q")
// into a structured Move object. It validates the input against the list of legal moves
// generated for the current board position.
// This function also takes the `active_player_color` for displaying accurate messages.
// Throws `std::runtime_error` if the move string is invalid or not found in legal moves.
Move parse_algebraic_move(const std::string& move_str, const std::vector<Move>& legal_moves, PlayerColor active_player_color) {
    if (move_str.length() < 4) {
        console_send_error("Move string too short. Expected format like 'e2e4' or 'e7e8q'.");
        throw std::runtime_error("Move string too short.");
    }

    // Convert algebraic file/rank characters to 0-7 indices.
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';

    // Create `GamePoint` structures from the parsed coordinates.
    GamePoint from_gp = {from_file, from_rank};
    GamePoint to_gp = {to_file, to_rank};

    // Handle optional promotion piece for pawn moves (5th character).
    PieceTypeIndex promotion_piece = PieceTypeIndex::NONE; // Default to no promotion.
    if (move_str.length() == 5) {
        char promo_char = std::tolower(move_str[4]); // Convert to lowercase for case-insensitive check.
        switch (promo_char) {
            case 'q': promotion_piece = PieceTypeIndex::QUEEN; break;
            case 'r': promotion_piece = PieceTypeIndex::ROOK; break;
            case 'b': promotion_piece = PieceTypeIndex::BISHOP; break;
            case 'n': promotion_piece = PieceTypeIndex::KNIGHT; break;
            default: // If the 5th character is not a valid promotion piece.
                console_send_error("Invalid promotion piece in move string.");
                throw std::runtime_error("Invalid promotion piece in move string.");
        }
    }

    // Search through the list of `legal_moves` to find a matching move.
    // A move matches if its `from_square`, `to_square`, and `promotion_piece_type_idx` (if applicable) are the same.
    auto it = std::find_if(legal_moves.begin(), legal_moves.end(), [&](const Move& m) {
        bool matches_from = (m.from_square.x == from_gp.x && m.from_square.y == from_gp.y);
        bool matches_to = (m.to_square.x == to_gp.x && m.to_square.y == to_gp.y);
        // For promotion, ensure both the legal move and the user's input are for promotion AND the promoted piece type matches.
        // If not a promotion, ensure the user didn't specify one.
        bool matches_promo = (!m.is_promotion && promotion_piece == PieceTypeIndex::NONE) ||
                             (m.is_promotion && m.promotion_piece_type_idx == promotion_piece);
        return matches_from && matches_to && matches_promo;
    });

    // If a matching legal move is found, return it.
    if (it != legal_moves.end()) {
        return *it;
    } else {
        // If no matching legal move is found, report an error and list all legal moves.
        console_send_error(std::string("Attempted move: ") + move_str + " is not a legal move.");
        std::string player_color_str = (active_player_color == PlayerColor::White ? "White" : "Black");
        console_send_line(std::string("Legal moves for ") + player_color_str + ":");
        
        std::string legal_moves_str = "";
        for(const auto& m : legal_moves) {
            // Convert the `Move` object back to algebraic string for display.
            legal_moves_str += (char)('a' + m.from_square.x);
            legal_moves_str += std::to_string(m.from_square.y + 1);
            legal_moves_str += (char)('a' + m.to_square.x);
            legal_moves_str += std::to_string(m.to_square.y + 1);
            if (m.is_promotion) {
                switch (m.promotion_piece_type_idx) {
                    case PieceTypeIndex::QUEEN: legal_moves_str += "q"; break;
                    case PieceTypeIndex::ROOK: legal_moves_str += "r"; break;
                    case PieceTypeIndex::BISHOP: legal_moves_str += "b"; break;
                    case PieceTypeIndex::KNIGHT: legal_moves_str += "n"; break;
                    default: break; // Should not happen with valid promotion types.
                }
            }
            legal_moves_str += " "; // Space between moves.
        }
        console_send_line(legal_moves_str); // Send the list of legal moves.
        throw std::runtime_error("Invalid move: Not found in the list of legal moves.");
    }
}

// Helper function to determine the current game status (e.g., Ongoing, Checkmate, Stalemate, Draw).
// This function takes the `ChessBoard` state, list of `legal_moves`, and `position_history` for analysis.
GameStatus get_current_game_status(const ChessBoard& board, const std::vector<Move>& legal_moves, const std::vector<uint64_t>& position_history) {
    // If there are no legal moves possible for the active player.
    if (legal_moves.empty()) {
        // Check if the active player's king is currently in check.
        if (board.is_king_in_check(board.active_player)) {
            // If in check and no legal moves, it's checkmate.
            return (board.active_player == PlayerColor::White) ? GameStatus::CHECKMATE_BLACK_WINS : GameStatus::CHECKMATE_WHITE_WINS;
        } else {
            // If not in check and no legal moves, it's a stalemate (draw).
            return GameStatus::STALEMATE;
        }
    }
    // Check for the 50-move rule: 100 halfmoves (50 full moves) without a pawn move or capture.
    else if (board.halfmove_clock >= 100) {
        return GameStatus::DRAW_FIFTY_MOVE;
    }
    // Check for threefold repetition: same position occurs three times.
    else {
        // Count occurrences of the current Zobrist hash in the history.
        // The current position's hash is already in `position_history` from `set_from_fen` or `apply_move`.
        // A count of 3 means the current position is the third occurrence.
        int repetition_count = std::count(position_history.begin(), position_history.end(), board.zobrist_hash);
        
        if (repetition_count >= 3) {
            return GameStatus::DRAW_THREEFOLD_REPETITION;
        } else {
            // If none of the above conditions are met, the game is still ongoing.
            return GameStatus::ONGOING;
        }
    }
}


// --- Main Program Entry Point ---

// This main.cpp provides an interactive console for testing ChessBoard and
// MoveGenerator functionalities, including FEN parsing, board drawing,
// Zobrist hash display, legal move generation, move application, and check/game status detection.
int main() {
    // Display introductory messages and FEN examples.
    console_send_line("--- Chess Engine Interactive Test ---");
    console_send_line("Enter FEN strings to analyze board state, or type 'quit' to exit.");
    console_send_line("To start a new game, type 'new fen'.");
    console_send_line("Example FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    console_send_line("Example FEN (White King in check by Black Queen): 6K1/7Q/8/8/8/8/8/2k3r1 w - - 0 1");
    console_send_line("Example FEN (Black King in check by White Queen): 8/6k1/8/8/1q6/8/8 b - - 0 1");
    console_send_line("Example FEN (for 50-move rule): 8/8/8/8/8/8/4k3/7K w - - 98 50");
    console_send_line("Example FEN (for threefold repetition - initial): 8/8/8/8/8/7k/4q3/7K w - - 0 1");

    // IMPORTANT: Initialize attack tables once at the start of the program.
    // This is crucial for move generation and check detection.
    ChessBitboardUtils::initialize_attack_tables();
    // The ChessBoard constructor will also implicitly call initialize_zobrist_keys() once.

    // Declare instances of ChessBoard, MoveGenerator, and position_history.
    ChessBoard current_board; // The main chessboard instance.
    MoveGenerator move_gen;   // The move generator instance.
    std::vector<uint64_t> position_history; // History for threefold repetition detection.

    std::string user_input_fen; // Variable to store FEN input from the user.
    std::string user_input;     // Variable to store move inputs or commands ('new fen', 'quit').

    // Prompt for the initial FEN string to set up the board.
    console_send_line("\nEnter initial FEN string: ");
    user_input_fen = console_read_line();

    // Check if the user wants to quit immediately after the initial FEN prompt.
    if (user_input_fen == "quit") {
        console_send_line("\n--- Exiting Chess Engine Test ---");
        return 0; // Exit program.
    }

    // Attempt to set the board from the initial FEN provided by the user.
    try {
        current_board.set_from_fen(user_input_fen);
        position_history.clear(); // Clear history for the new game.
        position_history.push_back(current_board.zobrist_hash); // Add the hash of the starting position to history.
    } catch (const std::exception& e) {
        // Catch and report any standard exceptions during FEN initialization.
        console_send_error(std::string("Error initializing board from FEN: ") + e.what() + ". Exiting.");
        return 1; // Exit with error code.
    } catch (...) {
        // Catch any other unknown exceptions during FEN initialization.
        console_send_error("An unknown error occurred during initial FEN setup. Exiting.");
        return 1; // Exit with error code.
    }

    // Main interactive game loop. This loop continues until the user types 'quit'.
    while (true) {
        console_send_line("\n-----------------------------------------------------");
        // Display the current FEN string of the board.
        console_send_line(std::string("Current FEN: ") + current_board.to_fen());
        
        // Display a visual representation of the board using ChessBitboardUtils.
        console_send_line("\n--- Board Visualization ---");
        console_send_line("Board (1 = Occupied, . = Empty):");
        ChessBitboardUtils::print_bitboard(current_board.occupied_squares);

        // Display the current Zobrist hash of the board.
        console_send_line("--- Zobrist Hash ---");
        console_send_line(std::string("Current Zobrist Hash: ") + std::to_string(current_board.zobrist_hash));

        // Display check status for both the active and inactive players.
        console_send_line("\n--- Check Detection ---");
        bool active_player_in_check = current_board.is_king_in_check(current_board.active_player);
        console_send_line((current_board.active_player == PlayerColor::White ? std::string("White") : std::string("Black")) + " King ("
                  + (current_board.active_player == PlayerColor::White ? std::string("White to move") : std::string("Black to move")) + ") in check? "
                  + (active_player_in_check ? "Yes" : "No"));

        PlayerColor inactive_player = (current_board.active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
        bool inactive_player_in_check = current_board.is_king_in_check(inactive_player);
        console_send_line((inactive_player == PlayerColor::White ? std::string("White") : std::string("Black")) + " King (Inactive) in check? "
                  + (inactive_player_in_check ? "Yes" : "No"));

        // Generate all legal moves for the current active player using `MoveGenerator`.
        std::vector<Move> legal_moves = move_gen.generate_legal_moves(current_board);
        // Determine the current game status based on legal moves and board state.
        GameStatus status = get_current_game_status(current_board, legal_moves, position_history);

        // Display the current game status to the user.
        console_send_line("\n--- Game Status ---");
        switch (status) {
            case GameStatus::ONGOING:
                console_send_line("Game Status: Ongoing.");
                break;
            case GameStatus::CHECKMATE_WHITE_WINS:
                console_send_line("Game Status: CHECKMATE! White wins!");
                break;
            case GameStatus::CHECKMATE_BLACK_WINS:
                console_send_line("Game Status: CHECKMATE! Black wins!");
                break;
            case GameStatus::STALEMATE:
                console_send_line("Game Status: STALEMATE! It's a draw.");
                break;
            case GameStatus::DRAW_FIFTY_MOVE:
                console_send_line("Game Status: DRAW by Fifty-Move Rule.");
                break;
            case GameStatus::DRAW_THREEFOLD_REPETITION:
                console_send_line("Game Status: DRAW by Threefold Repetition.");
                break;
        }

        // Display the list of legal moves for the active player.
        console_send_line(std::string("\n--- Legal Moves for ") + (current_board.active_player == PlayerColor::White ? "White" : "Black") + " ---");
        if (legal_moves.empty()) {
            console_send_line("No legal moves available.");
        } else {
            console_send_line(std::string("Found ") + std::to_string(legal_moves.size()) + " legal moves:");
            std::string moves_list_str = "";
            for (size_t i = 0; i < legal_moves.size(); ++i) {
                const Move& m = legal_moves[i];
                // Convert the Move object back to algebraic string for display.
                moves_list_str += (char)('a' + m.from_square.x);
                moves_list_str += std::to_string(m.from_square.y + 1);
                moves_list_str += (char)('a' + m.to_square.x);
                moves_list_str += std::to_string(m.to_square.y + 1);
                if (m.is_promotion) {
                    switch (m.promotion_piece_type_idx) {
                        case PieceTypeIndex::QUEEN: moves_list_str += "q"; break;
                        case PieceTypeIndex::ROOK: moves_list_str += "r"; break;
                        case PieceTypeIndex::BISHOP: moves_list_str += "b"; break;
                        case PieceTypeIndex::KNIGHT: moves_list_str += "n"; break;
                        default: break; // Should not happen.
                    }
                }
                moves_list_str += " "; // Add a space after each move.
                // Print moves in batches of 10 for better readability in the console.
                if ((i + 1) % 10 == 0) {
                    console_send_line(moves_list_str);
                    moves_list_str = ""; // Reset string for the next batch.
                }
            }
            if (!moves_list_str.empty()) { // Print any remaining moves in the last batch.
                console_send_line(moves_list_str);
            }
        }
        
        // Prompt the user for their next action (move, new FEN, or quit).
        if (status != GameStatus::ONGOING) {
            // If the game has ended, prompt to start a new game or quit.
            console_send_line("Game has ended. Type 'new fen' to start a new game or 'quit' to exit: ");
        } else {
            // If the game is ongoing, prompt for the next move.
            console_send_line("Enter next move (e.g., 'e2e4', 'e7e8q') or 'new fen' or 'quit'): ");
        }

        user_input = console_read_line(); // Read the user's input.

        // Handle user commands: 'quit' or 'new fen'.
        if (user_input == "quit") {
            break; // Exit the main game loop.
        } else if (user_input == "new fen") {
            console_send_line("\nEnter new FEN string: ");
            user_input_fen = console_read_line();
            try {
                current_board.set_from_fen(user_input_fen); // Set board to new FEN.
                position_history.clear(); // Clear history for new game.
                position_history.push_back(current_board.zobrist_hash); // Add new initial position to history.
            } catch (const std::exception& e) {
                console_send_error(std::string("Error setting new FEN: ") + e.what() + ". Board state unchanged.");
            } catch (...) {
                console_send_error("An unknown error occurred setting new FEN. Board state unchanged.");
            }
            continue; // Continue to the next iteration of the loop (display updated board).
        }

        // Attempt to apply the user's chosen move if the game is ongoing.
        try {
            if (status == GameStatus::ONGOING) {
                // Pass current_board.active_player to parse_algebraic_move.
                Move chosen_move = parse_algebraic_move(user_input, legal_moves, current_board.active_player); 
                
                StateInfo info_for_undo; // StateInfo is used by `apply_move` to store board state for undo functionality.
                current_board.apply_move(chosen_move, info_for_undo); // Apply the move to the board.
                position_history.push_back(current_board.zobrist_hash); // Add new position hash to history for repetition detection.
            } else {
                console_send_error("Game is over. Cannot make moves. Please use 'new fen' or 'quit'.");
            }

        } catch (const std::exception& e) {
            // Catch and report errors that occur during move parsing or application.
            console_send_error(std::string("Error applying move: ") + e.what());
        } catch (...) {
            console_send_error("An unknown error occurred during move application.");
        }
        // The loop continues, displaying the new board state or an error message.
    }

    console_send_line("\n--- Exiting Chess Engine Test ---");
    return 0; // Program exits gracefully.
}
