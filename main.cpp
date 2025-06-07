#include <iostream>
#include <string>
#include <vector>
#include <limits> // Required for std::numeric_limits
#include <algorithm> // For std::transform

#include "ChessBoard.h"
#include "MoveGenerator.h"
#include "ChessBitboardUtils.h" // Include the utility header to initialize static tables

// Function to print a bitboard for debugging
void print_bitboard(uint64_t bitboard, const std::string& name) {
    std::cout << "--- " << name << " Bitboard ---" << std::endl;
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square_idx = ChessBitboardUtils::rank_file_to_square(rank, file);
            if (ChessBitboardUtils::test_bit(bitboard, square_idx)) {
                std::cout << "1 ";
            } else {
                std::cout << "0 ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// Main function for the interactive chess engine debugger
int main() {
    // Initialize static attack tables in ChessBitboardUtils once at startup
    ChessBitboardUtils::initialize_attack_tables();

    std::cout << "Starting Chess Engine Interactive Debugger..." << std::endl;
    std::cout << "Static tables initialized." << std::endl;

    ChessBoard board; // Create a ChessBoard object

    std::string fen_input;
    bool new_game = true; // Flag to indicate if a new FEN is being entered

    while (true) {
        if (new_game) {
            std::cout << "\n==================================================" << std::endl;
            std::cout << "Enter FEN string to start a new game (or 'quit' to exit):" << std::endl;
            std::cout << "Default FEN: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" << std::endl;
            std::getline(std::cin, fen_input); // Read entire line including spaces

            if (fen_input == "quit") {
                break;
            }

            if (fen_input.empty()) {
                // If user just presses enter, reset to default FEN
                board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            } else {
                board.set_from_fen(fen_input); // Attempt to set from user-provided FEN
            }
            new_game = false; // FEN has been set, so it's no longer a "new game" input phase
        }
        
        std::cout << "\nBoard initialized from FEN: " << board.to_fen() << std::endl;

        // Print current board state (for visual debugging)
        std::cout << "\n--- Current Board State ---" << std::endl;
        print_bitboard(board.occupied_squares, "All Occupied Squares");

        std::cout << "\n  FEN: " << board.to_fen() << std::endl;
        std::cout << "  Turn: " << ((board.active_player == PlayerColor::White) ? "White" : "Black") << std::endl;
        
        std::string castling_str = "";
        if (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WK_BIT) castling_str += 'K';
        if (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WQ_BIT) castling_str += 'Q';
        if (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BK_BIT) castling_str += 'k';
        if (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BQ_BIT) castling_str += 'q';
        if (castling_str.empty()) castling_str = "-";
        std::cout << "  Castling Rights: " << castling_str << std::endl;

        std::cout << "  En Passant Target: ";
        if (board.en_passant_square_idx != 64) {
            std::cout << ChessBitboardUtils::square_to_string(board.en_passant_square_idx) << std::endl;
        } else {
            std::cout << "-" << std::endl;
        }
        std::cout << "  Halfmove Clock: " << board.halfmove_clock << std::endl;
        std::cout << "  Fullmove Number: " << board.fullmove_number << std::endl;
        std::cout << "  Zobrist Hash: " << std::hex << board.zobrist_hash << std::dec << std::endl;


        // Check if the current player's king is in check
        if (board.is_king_in_check(board.active_player)) {
            std::cout << "ATTENTION: " << ((board.active_player == PlayerColor::White) ? "White" : "Black") << " King is in check!" << std::endl;
        } else {
            std::cout << "King is NOT in check." << std::endl;
        }

        // Generate and print legal moves
        MoveGenerator move_gen;
        std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

        std::cout << "\nLegal Moves for " << ((board.active_player == PlayerColor::White) ? "White" : "Black") << " (" << legal_moves.size() << "):" << std::endl;
        if (legal_moves.empty()) {
            std::cout << "No legal moves available. ";
            if (board.is_king_in_check(board.active_player)) {
                std::cout << "Checkmate!" << std::endl;
            } else {
                std::cout << "Stalemate!" << std::endl;
            }
            // If game is over, prompt for new FEN
            new_game = true; 
            continue; // Skip move selection and go to next loop iteration
        } else {
            for (size_t i = 0; i < legal_moves.size(); ++i) {
                std::cout << i + 1 << ". " << ChessBitboardUtils::move_to_string(legal_moves[i]) << "   ";
                if ((i + 1) % 5 == 0) { // Print 5 moves per line
                    std::cout << std::endl;
                }
            }
            std::cout << std::endl;
        }

        // Allow user to choose a move
        int choice;
        while (true) {
            std::cout << "Enter the number of the move you want to apply (or '0' to enter a new FEN): ";
            std::cin >> choice;

            if (std::cin.fail()) {
                std::cout << "Invalid input. Please enter a number." << std::endl;
                std::cin.clear(); // Clear the error flag
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
                continue;
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume the rest of the line

            if (choice == 0) {
                new_game = true; // Signal to prompt for new FEN in next loop
                break;
            }

            if (choice > 0 && choice <= static_cast<int>(legal_moves.size())) {
                // Apply the chosen move
                StateInfo info_for_undo; // Not used for undoing from command line, but apply_move requires it
                board.apply_move(legal_moves[choice - 1], info_for_undo);
                // No need to undo here, as the move is actually applied for the next turn
                break; // Exit move selection loop, continue to next game state display
            } else {
                std::cout << "Invalid move number. Please choose a number between 1 and " << legal_moves.size() << ", or 0 for a new FEN." << std::endl;
            }
        }
    }

    std::cout << "Exiting Chess Engine Interactive Debugger." << std::endl;
    return 0;
}
