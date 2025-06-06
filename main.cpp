#include "ChessBoard.h"           // Include the ChessBoard struct
#include "ChessBitboardUtils.h"   // Include bitboard utility functions (for print_bitboard)
#include "Types.h"                // For PlayerColor
#include <iostream>               // For console input/output
#include <string>                 // For string manipulation
#include <limits>                 // For numeric_limits (used for cin.ignore)

// This main.cpp provides an interactive console for testing ChessBoard and
// ChessBitboardUtils functionalities, including FEN parsing, board drawing,
// Zobrist hash display, and check detection.

int main() {
    std::cout << "--- Chess Engine Interactive Test ---" << std::endl;
    std::cout << "Enter FEN strings to analyze board state, or type 'quit' to exit." << std::endl;
    std::cout << "Example FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" << std::endl;
    std::cout << "Example FEN (White King in check by Black Queen): 6K1/7Q/8/8/8/8/8/2k3r1 w - - 0 1" << std::endl;
    std::cout << "Example FEN (Black King in check by White Queen): 8/6k1/8/8/8/1q6/8/8 b - - 0 1" << std::endl;

    // IMPORTANT: Initialize attack tables once at the start of the program.
    // These tables are essential for attack detection (e.g., is_king_in_check).
    ChessBitboardUtils::initialize_attack_tables();

    std::string user_fen;
    while (true) {
        std::cout << "\nEnter FEN string (or 'quit'): ";
        std::getline(std::cin, user_fen); // Read the whole line

        if (user_fen == "quit") {
            break; // Exit the loop if user types 'quit'
        }

        try {
            // 1. Initialize ChessBoard from user-provided FEN
            ChessBoard board(user_fen);

            // 2. FEN Consistency Check
            std::cout << "\n--- FEN Consistency Test ---" << std::endl;
            std::string generated_fen = board.to_fen();
            std::cout << "Input FEN:     " << user_fen << std::endl;
            std::cout << "Generated FEN: " << generated_fen << std::endl;
            if (generated_fen == user_fen) {
                std::cout << "Result: PASSED (Input FEN matches generated FEN)." << std::endl;
            } else {
                std::cout << "Result: FAILED (Input FEN does NOT match generated FEN)." << std::endl;
                std::cerr << "WARNING: There might be an issue with FEN parsing or generation." << std::endl;
            }

            // 3. Draw the Board
            std::cout << "\n--- Board Visualization ---" << std::endl;
            std::cout << "Board (1 = Occupied, . = Empty):" << std::endl;
            ChessBitboardUtils::print_bitboard(board.occupied_squares);

            // 4. Display Zobrist Hash
            std::cout << "--- Zobrist Hash ---" << std::endl;
            std::cout << "Current Zobrist Hash: " << board.zobrist_hash << std::endl;

            // 5. Check Detection
            std::cout << "\n--- Check Detection ---" << std::endl;
            // Check for the active player
            bool active_player_in_check = board.is_king_in_check(board.active_player);
            std::cout << (board.active_player == PlayerColor::White ? "White" : "Black") << " King ("
                      << (board.active_player == PlayerColor::White ? "White to move" : "Black to move") << ") in check? "
                      << (active_player_in_check ? "Yes" : "No") << std::endl;

            // Check for the inactive player
            PlayerColor inactive_player = (board.active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
            bool inactive_player_in_check = board.is_king_in_check(inactive_player);
            std::cout << (inactive_player == PlayerColor::White ? "White" : "Black") << " King (Inactive) in check? "
                      << (inactive_player_in_check ? "Yes" : "No") << std::endl;

        } catch (const std::exception& e) {
            // Catch any exceptions (e.g., from std::stoi if FEN is malformed)
            std::cerr << "Error processing FEN: " << e.what() << ". Please enter a valid FEN string." << std::endl;
        } catch (...) {
            std::cerr << "An unknown error occurred. Please enter a valid FEN string." << std::endl;
        }

        // Clear any remaining input buffer to prevent issues in the next iteration
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    std::cout << "\n--- Exiting Chess Engine Test ---" << std::endl;
    return 0;
}
