#include "GameManager.h"
#include "ChessBitboardUtils.h" // For print_bitboard and ChessBitboardUtils::initialize_attack_tables
#include <iostream>             // For std::cerr
#include <algorithm>            // For std::find_if, std::count
#include <random>               // For std::random_device, std::mt19937, std::uniform_int_distribution
#include <stdexcept>            // For std::runtime_error

// Constructor for GameManager.
GameManager::GameManager() : rng_(std::random_device()()) {
    // Ensure attack tables are initialized once.
    ChessBitboardUtils::initialize_attack_tables();
    // ChessBoard constructor handles Zobrist key initialization and resets to startpos.
    
    // Add the initial board's Zobrist hash to the history.
    position_history.push_back(current_board.zobrist_hash);
}

// Resets the game to the starting position and clears game history.
void GameManager::reset_game() {
    current_board.reset_to_start_position();
    position_history.clear();
    position_history.push_back(current_board.zobrist_hash);
}

// Helper function to parse a move string (e.g., "e2e4" or "e7e8q") into a Move object.
// This function needs to determine the piece moved, if it's a capture, promotion, etc.
// It searches for the move within the list of legal moves to populate the Move object fully.
Move GameManager::parse_algebraic_move(const std::string& move_str, const std::vector<Move>& legal_moves) const {
    if (move_str.length() < 4) {
        throw std::runtime_error("Move string too short. Expected format like 'e2e4' or 'e7e8q'.");
    }

    // Extract from and to square coordinates
    int from_file = move_str[0] - 'a';
    int from_rank = move_str[1] - '1';
    int to_file = move_str[2] - 'a';
    int to_rank = move_str[3] - '1';

    GamePoint from_gp = {from_file, from_rank};
    GamePoint to_gp = {to_file, to_rank};

    PieceTypeIndex promotion_piece = PieceTypeIndex::NONE;
    if (move_str.length() == 5) {
        char promo_char = std::tolower(move_str[4]);
        switch (promo_char) {
            case 'q': promotion_piece = PieceTypeIndex::QUEEN; break;
            case 'r': promotion_piece = PieceTypeIndex::ROOK; break;
            case 'b': promotion_piece = PieceTypeIndex::BISHOP; break;
            case 'n': promotion_piece = PieceTypeIndex::KNIGHT; break;
            default: throw std::runtime_error("Invalid promotion piece in move string for parsing.");
        }
    }

    // Find the exact move from the legal moves list
    auto it = std::find_if(legal_moves.begin(), legal_moves.end(), [&](const Move& m) {
        bool matches_from = (m.from_square.x == from_gp.x && m.from_square.y == from_gp.y);
        bool matches_to = (m.to_square.x == to_gp.x && m.to_square.y == to_gp.y);
        bool matches_promo = (!m.is_promotion && promotion_piece == PieceTypeIndex::NONE) ||
                             (m.is_promotion && m.promotion_piece_type_idx == promotion_piece);
        return matches_from && matches_to && matches_promo;
    });

    if (it != legal_moves.end()) {
        return *it; // Return the fully populated Move object
    } else {
        throw std::runtime_error("Move: " + move_str + " is not a legal move in this position.");
    }
}

// Sets the board position based on UCI "position" command.
// Handles "startpos" or "fen" and applies subsequent moves.
void GameManager::set_board_from_uci_position(std::string fen_string, const std::vector<std::string>& moves_str) {
    // If fen_string is empty, it means the "startpos" case was processed in UciHandler
    // and UciHandler already provided the full FEN for startpos.
    
    current_board.set_from_fen(fen_string);
    position_history.clear(); // Clear history for the new base position
    position_history.push_back(current_board.zobrist_hash); // Add the base FEN hash

    // Apply all moves from the UCI command
    for (const std::string& move_uci : moves_str) {
        std::vector<Move> legal_moves_for_current_pos = move_gen.generate_legal_moves(current_board);
        Move move_to_apply = parse_algebraic_move(move_uci, legal_moves_for_current_pos);
        
        StateInfo info_for_undo; // State info for undo (not used here but needed by apply_move)
        current_board.apply_move(move_to_apply, info_for_undo);
        position_history.push_back(current_board.zobrist_hash); // Add hash after applying each move
    }
}

// Finds the best move for the current position.
// For now, this will return a random legal move.
// In the future, this will trigger the main search algorithm.
Move GameManager::find_best_move() {
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(current_board);

//    if (legal_moves.empty()) {
//        // Return an invalid move if no legal moves are available (checkmate/stalemate)
//        // A move with from_square.x == 64 can signify an invalid move.
//        return Move(); // Default constructor for Move should make it invalid.
//    }

    // For now, pick a random legal move.
    std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
    size_t random_index = dist(rng_);
    return legal_moves[random_index];
}

// Helper function to determine the current game status.
GameStatus GameManager::get_current_game_status(const std::vector<Move>& legal_moves) const {
    // If there are no legal moves:
    if (legal_moves.empty()) {
        // Check if the current active player's king is in check.
        if (current_board.is_king_in_check(current_board.active_player)) {
            // If in check and no legal moves, it's checkmate.
            return (current_board.active_player == PlayerColor::White) ? GameStatus::CHECKMATE_BLACK_WINS : GameStatus::CHECKMATE_WHITE_WINS;
        } else {
            // If not in check and no legal moves, it's stalemate.
            return GameStatus::STALEMATE;
        }
    } 
    // Check for Fifty-Move Rule
    else if (current_board.halfmove_clock >= 100) { 
        return GameStatus::DRAW_FIFTY_MOVE;
    } 
    // Check for Threefold Repetition
    else {
        // Count occurrences of the current Zobrist hash in the history.
        // A position needs to occur 3 times for a draw claim.
        // If current_board.zobrist_hash exists twice in `position_history` (from previous turns),
        // then the current occurrence makes it the third.
        int repetition_count = std::count(position_history.begin(), position_history.end(), current_board.zobrist_hash);
        
        if (repetition_count >= 3) { 
            return GameStatus::DRAW_THREEFOLD_REPETITION;
        } else {
            return GameStatus::ONGOING;
        }
    }
}
