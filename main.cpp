#include <iostream>    // Required for input/output operations (std::cin, std::cout, std::cerr)
#include <string>      // Required for std::string manipulation
#include <vector>      // Required for std::vector (e.g., storing legal moves)
#include <sstream>     // Required for std::stringstream to parse UCI commands
#include <random>      // Required for random number generation (std::mt19937_64, std::uniform_int_distribution)
#include <algorithm>   // Required for various algorithms (though not directly used in this UCI loop, it's a common C++ include)

// Include custom headers for the chess engine components
#include "ChessBoard.h"          // Manages the chess board state and rules
#include "MoveGenerator.h"       // Generates pseudo-legal and legal moves
#include "ChessBitboardUtils.h"  // Provides utility functions for bitboard manipulation and attack tables
#include "Move.h"                // Defines the structure for a chess move

// Global random number generator for move selection.
// We use a Mersenne Twister engine for good statistical properties,
// seeded with std::random_device for non-determinism across runs.
std::mt19937_64 rng(std::random_device{}());

// Main function - the entry point of the UCI chess engine.
// This function continuously reads commands from standard input (stdin)
// and writes responses to standard output (stdout), as per the UCI protocol.
// Debugging information is printed to standard error (stderr) to avoid
// interfering with UCI communication.
int main() {
    // Initialize static attack tables for various pieces.
    // This precomputation is done once at the very beginning to speed up
    // move generation and attack detection throughout the engine's lifetime.
    ChessBitboardUtils::initialize_attack_tables();

    // Create a ChessBoard object. This object will hold the current state
    // of the chess game, including piece positions, active player, castling rights, etc.
    ChessBoard board;

    // Main loop: Continuously read lines from standard input (where the GUI sends commands).
    std::string line;
    while (std::getline(std::cin, line)) {
        // // DEBUGGING: Print the entire received position command string for inspection.
        // std::cerr << "DEBUG: command received, this is the received string: " << std::endl;
        // std::cerr << "DEBUG: " << line << std::endl; // The original line read from stdin.

        // Use a stringstream to easily parse tokens from the received command line.
        std::stringstream ss(line);
        std::string command;
        ss >> command; // Read the first token, which is the command name.

        // --- UCI Command Handling ---

        // 1. 'uci' command: Sent by the GUI to initialize the engine.
        // The engine must respond with its identity and capabilities.
        // Example: "uci"
        if (command == "uci") {
            // "id name <engine name>": Specifies the engine's name.
            std::cout << "id name Carolyna" << std::endl;
            // "id author <author name>": Specifies the author's name.
            std::cout << "id author Duy Anh" << std::endl;
            // Additional options could be defined here (e.g., "option name Hash type spin default 16 min 1 max 1024").
            // "uciok": Signals that the engine has finished sending its UCI identity and options.
            std::cout << "uciok" << std::endl;
        }
        // 2. 'isready' command: Sent by the GUI to check if the engine is ready for commands.
        // The engine should perform any necessary initialization and respond with "readyok".
        // Example: "isready"
        else if (command == "isready") {
            std::cout << "readyok" << std::endl;
        }
        // 3. 'ucinewgame' command: Sent by the GUI to signal the start of a new game.
        // The engine should reset its internal state (e.g., clear transposition tables, reset board).
        // Example: "ucinewgame"
        else if (command == "ucinewgame") {
            // Reset the board to the standard starting position (empty or default FEN).
            // Using set_from_fen here ensures robust initialization from a known good FEN string.
            board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        // 4. 'position' command: Sets up the board position.
        // This is one of the most complex commands as it can specify a starting position
        // either by 'startpos' or a 'fen' string, followed by an optional list of 'moves'.
        // Examples:
        //   "position startpos"
        //   "position startpos moves e2e4 e7e5"
        //   "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4"
        else if (command == "position") {
            // // DEBUGGING: Print the entire received position command string for inspection.
            // std::cerr << "DEBUG: command received, this is the received string: " << std::endl;
            // std::cerr << "DEBUG: " << line << std::endl; // The original line read from stdin.

            std::string sub_command;
            ss >> sub_command; // Read the next token, which should be "startpos" or "fen".

            // This 'temp_word' variable is crucial for parsing both 'fen' and 'startpos' branches
            // to correctly identify and process the 'moves' keyword later.
            std::string temp_word; 

            // Handle 'startpos' subcommand: Sets the board to the standard initial chess position.
            if (sub_command == "startpos") {
                board.reset_to_start_position(); // Or board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                
                // After setting 'startpos', explicitly check if 'moves' follows on the same line.
                if (ss >> temp_word && temp_word == "moves") {
                    // If 'moves' keyword is found, proceed to apply the moves.
                    std::string move_str;
                    MoveGenerator move_gen; // Create a MoveGenerator to validate and apply moves.
                    while (ss >> move_str) { // Read each subsequent move string (e.g., "e2e4").
                        // Generate all legal moves for the current board state.
                        // This is necessary to convert the algebraic move string (e.g., "e2e4")
                        // into a structured 'Move' object that can be applied by 'apply_move'.
                        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 
                        bool move_found = false;
                        for (const auto& legal_move : legal_moves) {
                            // Convert the 'Move' object back to string for comparison.
                            if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
                                StateInfo info_for_undo; // A temporary StateInfo object (not used for undo in UCI loop directly)
                                board.apply_move(legal_move, info_for_undo); // Apply the move, updating the board state and flipping the turn.
                                move_found = true;
                                // // DEBUGGING: Print FEN after each individual move within the 'moves' list.
                                // std::cerr << "DEBUG: FEN after move " << move_str << ": " << board.to_fen() << std::endl;
                                break; // Move found and applied, break from inner loop.
                            }
                        }
                        if (!move_found) {
                            // // Log an error if the received move string is not found among legal moves.
                            // std::cerr << "UCI ERROR: Could not find move '" << move_str << "' in legal moves for current position." << std::endl;
                            // In a production engine, this might indicate a serious discrepancy
                            // between the GUI's expected moves and the engine's move generation.
                            // For now, we continue, but this needs attention in a robust engine.
                        }
                    }
                }
            }
            // Handle 'fen' subcommand: Sets the board from a provided FEN string.
            else if (sub_command == "fen") {
                std::string fen_string_part;
                // Read parts of the FEN string until the "moves" keyword is encountered
                // or the end of the line is reached. The 'temp_word' captures the "moves" token if found.
                while (ss >> temp_word && temp_word != "moves") {
                    if (!fen_string_part.empty()) {
                        fen_string_part += " "; // Add space between FEN components.
                    }
                    fen_string_part += temp_word;
                }
                board.set_from_fen(fen_string_part); // Initialize the board using the parsed FEN string.
                
                // If the loop above terminated because "moves" was found (i.e., temp_word is "moves"),
                // then proceed to apply the subsequent moves.
                if (temp_word == "moves") { 
                    std::string move_str;
                    MoveGenerator move_gen;
                    while (ss >> move_str) { // Read each subsequent move string.
                        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 
                        bool move_found = false;
                        for (const auto& legal_move : legal_moves) {
                            if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
                                StateInfo info_for_undo;
                                board.apply_move(legal_move, info_for_undo);
                                move_found = true;
                                // // DEBUGGING: Print FEN after each individual move within the 'moves' list.
                                // std::cerr << "DEBUG: FEN after move " << move_str << ": " << board.to_fen() << std::endl;
                                break;
                            }
                        }
                        if (!move_found) {
                            // std::cerr << "UCI ERROR: Could not find move '" << move_str << "' in legal moves for current position." << std::endl;
                        }
                    }
                }
            }
            // Handle unknown sub-commands for 'position'.
            else {
                // std::cerr << "UCI ERROR: Unknown position sub-command: " << sub_command << std::endl;
                continue; 
            }

            // // DEBUGGING: After processing the entire 'position' command (initial setup + all moves),
            // // print the current FEN string of the board. This is extremely useful to verify that
            // // the board state (including piece positions, active player, clocks) is as expected.
            // std::cerr << "DEBUG: Board FEN after 'position' command: " << board.to_fen() << std::endl;

        }
        // 5. 'go' command: Instructs the engine to start searching for the best move.
        // It can include various parameters (e.g., time limits, depth, ponder).
        // For this basic engine, we ignore parameters and just pick a random legal move.
        // Example: "go wtime 360000 btime 360000"
        else if (command == "go") {
            MoveGenerator move_gen;
            // Generate all legal moves for the current board state and active player.
            std::vector<Move> legal_moves = move_gen.generate_legal_moves(board); 

            // // DEBUGGING: Print the list of legal moves generated for the current turn.
            // std::cerr << "DEBUG: Legal Moves for " << ((board.active_player == PlayerColor::White) ? "White" : "Black") << " (" << legal_moves.size() << "):" << std::endl;
            if (legal_moves.empty()) {
                // std::cerr << "DEBUG: No legal moves found for the current position." << std::endl;
                std::cout << "bestmove (none)" << std::endl; // Inform GUI there's no move (e.g., checkmate/stalemate).
            } else {
                // If there are legal moves, iterate and print them to stderr.
                // for (size_t i = 0; i < legal_moves.size(); ++i) {
                //     std::cerr << ChessBitboardUtils::move_to_string(legal_moves[i]) << " ";
                // }
                // std::cerr << std::endl; // Newline after listing all moves.

                // Select a random legal move from the generated list.
                // In a real engine, this would be replaced by a sophisticated search algorithm (e.g., Minimax, Alpha-Beta).
                std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
                size_t random_index = dist(rng); // Get a random index.
                Move chosen_move = legal_moves[random_index]; // Get the move at that index.
                
                // Print the chosen best move in UCI format.
                // Example: "bestmove e2e4"
                std::cout << "bestmove " << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
            }
        }
        // 6. 'quit' command: Sent by the GUI to terminate the engine.
        // The engine should exit cleanly.
        // Example: "quit"
        else if (command == "quit") {
            break; // Exit the main input reading loop, leading to program termination.
        }
        // Handle any other unknown or unexpected commands from the GUI.
        // Log them to stderr for debugging purposes.
        else {
            // std::cerr << "DEBUG: Received unknown command: " << command << std::endl;
        }
    }

    return 0; // Successful program termination.
}
