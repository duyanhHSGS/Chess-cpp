#include <iostream>
#include <string>
#include <vector>
#include <sstream>  // For parsing UCI commands
#include <random>   // For std::mt19937_64 and std::uniform_int_distribution
#include <algorithm> // For std::transform (though not explicitly used in UCI processing directly)

#include "ChessBoard.h"
#include "MoveGenerator.h"
#include "ChessBitboardUtils.h" // Include the utility header to initialize static tables
#include "Move.h" // Ensure Move struct is available for parsing/generating moves

// Global random number generator for move selection
std::mt19937_64 rng(std::random_device{}());

// Main function for the UCI chess engine
int main() {
    // Initialize static attack tables in ChessBitboardUtils once at startup
    ChessBitboardUtils::initialize_attack_tables();

    ChessBoard board; // Create a ChessBoard object

    std::string line;
    while (std::getline(std::cin, line)) { // Read commands line by line
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "uci") {
            std::cout << "id name Carolyna" << std::endl;
            std::cout << "id author Duy Anh" << std::endl;
            // Optionally, define UCI options here if your engine had any
            // std::cout << "option name Debug Log File type string default " << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (command == "ucinewgame") {
            // Reset the board to the starting position for a new game
            // Use set_from_fen for robust initialization.
            board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        } else if (command == "position") {
            std::string sub_command;
            ss >> sub_command;

            // Declare temp_word here, as it's used in both branches
            std::string temp_word; 

            if (sub_command == "startpos") {
                board.reset_to_start_position();
                // Explicitly check for and consume the "moves" token after "startpos"
                if (ss >> temp_word && temp_word == "moves") {
                    std::string move_str;
                    MoveGenerator move_gen; 
                    while (ss >> move_str) {
                        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 
                        bool move_found = false;
                        for (const auto& legal_move : legal_moves) {
                            if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
                                StateInfo info_for_undo; 
                                board.apply_move(legal_move, info_for_undo);
                                move_found = true;
                                break;
                            }
                        }
                        if (!move_found) {
                            std::cerr << "UCI ERROR: Could not find move '" << move_str << "' in legal moves for current position." << std::endl;
                        }
                    }
                }
            } else if (sub_command == "fen") {
                std::string fen_string_part;
                // Read FEN parts until "moves" is encountered or end of line
                while (ss >> temp_word && temp_word != "moves") {
                    if (!fen_string_part.empty()) {
                        fen_string_part += " "; 
                    }
                    fen_string_part += temp_word;
                }
                board.set_from_fen(fen_string_part);
                
                // If "moves" was encountered by the loop, process subsequent moves.
                // 'temp_word' will hold "moves" if the loop terminated because it read "moves".
                if (temp_word == "moves") { 
                    std::string move_str;
                    MoveGenerator move_gen;
                    while (ss >> move_str) {
                        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 
                        bool move_found = false;
                        for (const auto& legal_move : legal_moves) {
                            if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
                                StateInfo info_for_undo;
                                board.apply_move(legal_move, info_for_undo);
                                move_found = true;
                                break;
                            }
                        }
                        if (!move_found) {
                            std::cerr << "UCI ERROR: Could not find move '" << move_str << "' in legal moves for current position." << std::endl;
                        }
                    }
                }
            } else {
                // Unknown position sub-command
                std::cerr << "UCI ERROR: Unknown position sub-command: " << sub_command << std::endl;
                continue; 
            }

            // DEBUGGING: Print the FEN after processing the position command
            std::cerr << "DEBUG: Board FEN after 'position' command: " << board.to_fen() << std::endl;

        } else if (command == "go") {
            MoveGenerator move_gen;
            std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 

            if (legal_moves.empty()) {
                std::cout << "bestmove (none)" << std::endl;
            } else {
                // Select a random legal move
                std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
                size_t random_index = dist(rng);
                Move chosen_move = legal_moves[random_index];
                std::cout << "bestmove " << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
            }
        } else if (command == "quit") {
            break; // Exit the engine loop
        }
        // Unknown commands or other internal debugging can be logged to std::cerr
        // std::cerr << "Received unknown command: " << command << std::endl;
    }

    return 0;
}
