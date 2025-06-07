#include "MoveGenerator.h"
#include "ChessBitboardUtils.h" // For bit manipulation and square constants
#include <cmath>                // For std::abs in wrapping checks
#include <array>                // REQUIRED for std::array definition and usage


// --- Static Member Definitions for MoveGenerator ---
// These define the actual static arrays declared in MoveGenerator.h.
const std::array<int, 4> MoveGenerator::BISHOP_DELTAS = {-9, -7, 7, 9};
const std::array<int, 4> MoveGenerator::ROOK_DELTAS = {-8, 8, -1, 1};
const std::array<int, 8> MoveGenerator::QUEEN_DELTAS = {-9, -8, -7, -1, 1, 7, 8, 9};


// Helper to check if a square is attacked by a given color.
// This is used for king safety (e.g., during castling checks or discovering pins).
bool MoveGenerator::is_square_attacked(int square_idx, PlayerColor attacking_color, const ChessBoard& board) {
    // Get bitboards for potential attackers of 'attacking_color'.
    uint64_t enemy_pawns_bb = (attacking_color == PlayerColor::White) ? board.white_pawns : board.black_pawns;
    uint64_t enemy_knights_bb = (attacking_color == PlayerColor::White) ? board.white_knights : board.black_knights;
    uint64_t enemy_rooks_bb = (attacking_color == PlayerColor::White) ? board.white_rooks : board.black_rooks;
    uint64_t enemy_bishops_bb = (attacking_color == PlayerColor::White) ? board.white_bishops : board.black_bishops;
    uint64_t enemy_queens_bb = (attacking_color == PlayerColor::White) ? board.white_queens : board.black_queens;
    uint64_t enemy_king_bb = (attacking_color == PlayerColor::White) ? board.white_king : board.black_king; // For king proximity check

    // Check for attacks from Pawns
    if (ChessBitboardUtils::is_pawn_attacked_by(square_idx, enemy_pawns_bb, attacking_color)) return true;

    // Check for attacks from Knights
    if (ChessBitboardUtils::is_knight_attacked_by(square_idx, enemy_knights_bb)) return true;

    // Check for attacks from Kings
    if (ChessBitboardUtils::is_king_attacked_by(square_idx, enemy_king_bb)) return true;

    // Check for attacks from Rooks and Queens (horizontal/vertical sliding attacks).
    // `occupied_squares` is crucial here to determine if a sliding piece's attack path is blocked.
    uint64_t rook_queen_attackers = enemy_rooks_bb | enemy_queens_bb;
    if (ChessBitboardUtils::is_rook_queen_attacked_by(square_idx, rook_queen_attackers, board.occupied_squares)) return true;

    // Check for attacks from Bishops and Queens (diagonal sliding attacks).
    // Again, `occupied_squares` is needed for blocking.
    uint64_t bishop_queen_attackers = enemy_bishops_bb | enemy_queens_bb;
    if (ChessBitboardUtils::is_bishop_queen_attacked_by(square_idx, bishop_queen_attackers, board.occupied_squares)) return true;

    return false; // Square is not attacked
}


// Generates all pseudo-legal pawn moves for the active player from a given square.
void MoveGenerator::generate_pawn_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint8_t rank = ChessBitboardUtils::square_to_rank(square_idx); 
    uint8_t file = ChessBitboardUtils::square_to_file(square_idx); 

    // Define pawn movement directions based on color.
    int forward_step = (color == PlayerColor::White) ? 8 : -8;
    uint8_t start_rank = (color == PlayerColor::White) ? 1 : 6; // 1st rank for white pawns, 6th for black pawns
    uint8_t promotion_rank = (color == PlayerColor::White) ? 7 : 0; // 8th rank for white, 1st for black

    uint64_t empty_squares = ~board.occupied_squares; // All unoccupied squares

    // --- Single Pawn Push ---
    int target_sq_single = square_idx + forward_step;
    // Check if target square is within bounds and is empty.
    if (target_sq_single >= 0 && target_sq_single < 64 && ChessBitboardUtils::test_bit(empty_squares, target_sq_single)) {
        // If pawn reaches promotion rank, generate promotion moves.
        if (ChessBitboardUtils::square_to_rank(target_sq_single) == promotion_rank) {
            // Non-capture promotions
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true /*is_promotion*/, PieceTypeIndex::QUEEN, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true /*is_promotion*/, PieceTypeIndex::ROOK, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true /*is_promotion*/, PieceTypeIndex::BISHOP, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true /*is_promotion*/, PieceTypeIndex::KNIGHT, false, false, false, false);
        } else {
            // Normal single push.
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN);
        }

        // --- Double Pawn Push (if from starting rank and path is clear) ---
        if (rank == start_rank) {
            int target_sq_double = square_idx + 2 * forward_step;
            // Check if intermediate square and final square are empty.
            if (target_sq_double >= 0 && target_sq_double < 64 &&
                ChessBitboardUtils::test_bit(empty_squares, target_sq_double) &&
                ChessBitboardUtils::test_bit(empty_squares, target_sq_single)) {
                pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_double)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, false, false, true);
            }
        }
    }

    // --- Pawn Captures ---
    int capture_left_delta = (color == PlayerColor::White) ? 7 : -9;  // NW for White, SW for Black
    int capture_right_delta = (color == PlayerColor::White) ? 9 : -7; // NE for White, SE for Black

    uint64_t enemy_occupied_squares = (color == PlayerColor::White) ? board.black_occupied_squares : board.white_occupied_squares;

    // Check capture to the left diagonal
    int target_sq_cl = square_idx + capture_left_delta;
    // Ensure target_sq_cl is on the board and that the move doesn't wrap around file.
    // The file check is crucial to prevent "wrapping" from 'a' file to 'h' file (or vice versa).
    bool is_valid_cl_move = (target_sq_cl >= 0 && target_sq_cl < 64) && 
                            (ChessBitboardUtils::square_to_file(target_sq_cl) == file - 1 || ChessBitboardUtils::square_to_file(target_sq_cl) == file + 1);


    if (is_valid_cl_move && ChessBitboardUtils::test_bit(enemy_occupied_squares, target_sq_cl)) {
        // Determine captured piece type for the move struct.
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        // Find which enemy piece is on target_sq_cl
        if (color == PlayerColor::White) { // White capturing black piece
            if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq_cl)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq_cl)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq_cl)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq_cl)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq_cl)) captured_type = PieceTypeIndex::QUEEN;
        } else { // Black capturing white piece
            if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq_cl)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq_cl)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq_cl)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq_cl)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq_cl)) captured_type = PieceTypeIndex::QUEEN;
        }

        if (ChessBitboardUtils::square_to_rank(target_sq_cl) == promotion_rank) {
            // Capture promotions
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::QUEEN, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::ROOK, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::BISHOP, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::KNIGHT, false, false, false, false);
        } else {
            // Normal capture
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true);
        }
    }

    // Check capture to the right diagonal
    int target_sq_cr = square_idx + capture_right_delta;
    // A simplified check for file wrapping (same as above, crucial)
    bool is_valid_cr_move = (target_sq_cr >= 0 && target_sq_cr < 64) && 
                            (ChessBitboardUtils::square_to_file(target_sq_cr) == file - 1 || ChessBitboardUtils::square_to_file(target_sq_cr) == file + 1);

    if (is_valid_cr_move && ChessBitboardUtils::test_bit(enemy_occupied_squares, target_sq_cr)) {
         PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        // Find which enemy piece is on target_sq_cr
        if (color == PlayerColor::White) {
            if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq_cr)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq_cr)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq_cr)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq_cr)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq_cr)) captured_type = PieceTypeIndex::QUEEN;
        } else {
            if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq_cr)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq_cr)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq_cr)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq_cr)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq_cr)) captured_type = PieceTypeIndex::QUEEN;
        }
        if (ChessBitboardUtils::square_to_rank(target_sq_cr) == promotion_rank) {
            // Capture promotions
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::QUEEN, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::ROOK, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::BISHOP, false, false, false, false);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::KNIGHT, false, false, false, false);
        } else {
            // Normal capture
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true);
        }
    }

    // --- En Passant Capture ---
    if (board.en_passant_square_idx != 64) {
        uint8_t ep_file = ChessBitboardUtils::square_to_file(board.en_passant_square_idx); 
        uint8_t ep_rank = ChessBitboardUtils::square_to_rank(board.en_passant_square_idx); 
        
        // Check if pawn can capture en passant to the left
        if (file - 1 == ep_file && ((color == PlayerColor::White && rank == 4 && ep_rank == 5) || (color == PlayerColor::Black && rank == 3 && ep_rank == 2))) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ep_file, ep_rank}, PieceTypeIndex::PAWN, PieceTypeIndex::PAWN, true, PieceTypeIndex::NONE, false, false, true, false);
        }
        // Check if pawn can capture en passant to the right
        if (file + 1 == ep_file && ((color == PlayerColor::White && rank == 4 && ep_rank == 5) || (color == PlayerColor::Black && rank == 3 && ep_rank == 2))) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ep_file, ep_rank}, PieceTypeIndex::PAWN, PieceTypeIndex::PAWN, true, PieceTypeIndex::NONE, false, false, true, false);
        }
    }
}

// Generates all pseudo-legal knight moves.
void MoveGenerator::generate_knight_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint64_t friendly_occupied_squares = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    // Get the attack mask for the knight from the current square.
    uint64_t attacks = ChessBitboardUtils::knight_attacks[square_idx];
    
    // Remove squares occupied by friendly pieces.
    attacks &= ~friendly_occupied_squares;

    // Iterate through the attacked squares (possible target squares).
    while (attacks) {
        int target_sq = ChessBitboardUtils::pop_lsb_index(attacks); // Get and clear LSB
        
        // Determine if it's a capture.
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        bool is_capture = false;
        if (ChessBitboardUtils::test_bit(board.occupied_squares, target_sq)) { // Check if target is occupied by any piece
            is_capture = true;
            // Determine which enemy piece is on target_sq
            if (color == PlayerColor::White) { // White capturing black piece
                if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            } else { // Black capturing white piece
                if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            }
        }
        
        pseudo_legal_moves.emplace_back(GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
                                         GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
                                         PieceTypeIndex::KNIGHT, captured_type, is_capture);
    }
}

// Generates pseudo-legal sliding moves for a given piece (Bishop, Rook, Queen).
// This is a common helper used by generate_bishop_moves, generate_rook_moves, generate_queen_moves.
template<size_t N> // Use template to deduce array size for deltas
void MoveGenerator::generate_sliding_piece_moves_helper(const ChessBoard& board, int square_idx, PieceTypeIndex piece_type, std::vector<Move>& pseudo_legal_moves, const std::array<int, N>& deltas) {
    PlayerColor color = board.active_player;
    uint64_t friendly_occupied_squares = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;
    uint64_t enemy_occupied_squares = (color == PlayerColor::White) ? board.black_occupied_squares : board.white_occupied_squares;

    uint8_t current_rank = ChessBitboardUtils::square_to_rank(square_idx); 
    uint8_t current_file = ChessBitboardUtils::square_to_file(square_idx); 

    for (int delta : deltas) {
        int target_sq = square_idx;
        while (true) {
            target_sq += delta;

            if (target_sq < 0 || target_sq >= 64) break; // Out of bounds
            
            // Check for wrapping around files for horizontal moves (+/-1)
            // Cast to int before subtraction to prevent unsigned wrap-around.
            if (std::abs(delta) == 1 && ChessBitboardUtils::square_to_rank(target_sq) != current_rank) break;
            // Check for wrapping around ranks for vertical moves (+/-8)
            // Cast to int before subtraction to prevent unsigned wrap-around.
            if (std::abs(delta) == 8 && ChessBitboardUtils::square_to_file(target_sq) != current_file) break;
            // Check for wrapping for diagonal moves (+/-7, +/-9)
            // Cast to int before subtraction to prevent unsigned wrap-around.
            if ((std::abs(delta) == 7 || std::abs(delta) == 9) &&
                (std::abs(static_cast<int>(ChessBitboardUtils::square_to_rank(target_sq)) - static_cast<int>(current_rank)) > 1 || 
                 std::abs(static_cast<int>(ChessBitboardUtils::square_to_file(target_sq)) - static_cast<int>(current_file)) > 1)) break;


            if (ChessBitboardUtils::test_bit(friendly_occupied_squares, target_sq)) {
                // Square is occupied by a friendly piece, cannot move further in this direction.
                break;
            }

            // Determine if it's a capture.
            PieceTypeIndex captured_type = PieceTypeIndex::NONE;
            bool is_capture = false;
            if (ChessBitboardUtils::test_bit(enemy_occupied_squares, target_sq)) {
                is_capture = true;
                // Determine which enemy piece is on target_sq
                if (color == PlayerColor::White) {
                    if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                    else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                    else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                    else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                    else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
                    else if (ChessBitboardUtils::test_bit(board.black_king, target_sq)) captured_type = PieceTypeIndex::KING;
                } else {
                    if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                    else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                    else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                    else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                    else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
                    else if (ChessBitboardUtils::test_bit(board.white_king, target_sq)) captured_type = PieceTypeIndex::KING;
                }
            }

            pseudo_legal_moves.emplace_back(GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
                                             GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
                                             piece_type, captured_type, is_capture);

            if (is_capture) {
                // If a piece is captured, cannot move further in this direction.
                break;
            }
        }
    }
}

// Generates all pseudo-legal bishop moves.
void MoveGenerator::generate_bishop_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::BISHOP, pseudo_legal_moves, BISHOP_DELTAS);
}

// Generates all pseudo-legal rook moves.
void MoveGenerator::generate_rook_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::ROOK, pseudo_legal_moves, ROOK_DELTAS);
}

// Generates all pseudo-legal queen moves.
void MoveGenerator::generate_queen_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::QUEEN, pseudo_legal_moves, QUEEN_DELTAS);
}

// Generates all pseudo-legal king moves.
void MoveGenerator::generate_king_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint64_t friendly_occupied_squares = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    // Get the attack mask for the king from the current square.
    uint64_t attacks = ChessBitboardUtils::king_attacks[square_idx];
    
    // Remove squares occupied by friendly pieces.
    attacks &= ~friendly_occupied_squares;

    // Iterate through the attacked squares (possible target squares).
    while (attacks) {
        int target_sq = ChessBitboardUtils::pop_lsb_index(attacks);
        
        // Determine if it's a capture.
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        bool is_capture = false;
        if (ChessBitboardUtils::test_bit(board.occupied_squares, target_sq)) {
            is_capture = true;
            // Determine which enemy piece is on target_sq
            if (color == PlayerColor::White) {
                if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            } else {
                if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            }
        }
        
        pseudo_legal_moves.emplace_back(GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
                                         GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
                                         PieceTypeIndex::KING, captured_type, is_capture);
    }

    // --- Castling Moves ---
    // Check castling rights and if path is clear/not attacked.
    uint8_t king_rank = ChessBitboardUtils::square_to_rank(square_idx); 
    uint8_t king_file = ChessBitboardUtils::square_to_file(square_idx); 
    PlayerColor enemy_color = (color == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;

    // Kingside Castling
    // White Kingside: E1-G1 (H1 rook)
    if (color == PlayerColor::White && square_idx == ChessBitboardUtils::E1_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WK_BIT)) {
        // Squares F1 (5) and G1 (6) must be empty and not attacked.
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::F1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::G1_SQ)) {
            // Check if king moves through or into check
            if (!is_square_attacked(ChessBitboardUtils::E1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::F1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::G1_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::G1_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::G1_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, true, false, false, false);
            }
        }
    }
    // Black Kingside: E8-G8 (H8 rook)
    else if (color == PlayerColor::Black && square_idx == ChessBitboardUtils::E8_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BK_BIT)) {
        // Squares F8 (61) and G8 (62) must be empty and not attacked.
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::F8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::G8_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::F8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::G8_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::G8_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::G8_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, true, false, false, false);
            }
        }
    }

    // Queenside Castling
    // White Queenside: E1-C1 (A1 rook)
    if (color == PlayerColor::White && square_idx == ChessBitboardUtils::E1_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WQ_BIT)) {
        // Squares D1 (3), C1 (2), B1 (1) must be empty.
        // King passes through D1 and C1, so D1 and C1 must not be attacked.
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::D1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::C1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::B1_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::D1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::C1_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::C1_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::C1_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, true, false, false);
            }
        }
    }
    // Black Queenside: E8-C8 (A8 rook)
    else if (color == PlayerColor::Black && square_idx == ChessBitboardUtils::E8_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BQ_BIT)) {
        // Squares D8 (59), C8 (58), B8 (57) must be empty.
        // King passes through D8 and C8, so D8 and C8 must not be attacked.
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::D8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::C8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::B8_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::D8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::C8_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::C8_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::C8_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, true, false, false);
            }
        }
    }
}


// Main function to generate all legal moves for the active player.
// This implements the "Generate Pseudo-Legal, then Filter by Make/Unmake" pattern.
std::vector<Move> MoveGenerator::generate_legal_moves(ChessBoard& board) { // Changed to non-const reference
    std::vector<Move> pseudo_legal_moves;
    std::vector<Move> legal_moves;

    PlayerColor current_player = board.active_player; // Store the original active player
    uint64_t player_pieces_bb = (current_player == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    // Iterate through all squares to find pieces belonging to the active player.
    uint64_t temp_player_pieces_bb = player_pieces_bb; // Use a temporary to iterate
    while (temp_player_pieces_bb != 0ULL) {
        int square_idx = ChessBitboardUtils::pop_lsb_index(temp_player_pieces_bb); // Get piece square and clear bit

        // Determine piece type on this square.
        PieceTypeIndex piece_type = PieceTypeIndex::NONE;
        if (current_player == PlayerColor::White) {
            if (ChessBitboardUtils::test_bit(board.white_pawns, square_idx)) piece_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, square_idx)) piece_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, square_idx)) piece_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, square_idx)) piece_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, square_idx)) piece_type = PieceTypeIndex::QUEEN;
            else if (ChessBitboardUtils::test_bit(board.white_king, square_idx)) piece_type = PieceTypeIndex::KING;
        } else { // Black
            if (ChessBitboardUtils::test_bit(board.black_pawns, square_idx)) piece_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, square_idx)) piece_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, square_idx)) piece_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, square_idx)) piece_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, square_idx)) piece_type = PieceTypeIndex::QUEEN;
            else if (ChessBitboardUtils::test_bit(board.black_king, square_idx)) piece_type = PieceTypeIndex::KING;
        }

        // Call the appropriate pseudo-legal move generator for this piece type.
        switch (piece_type) {
            case PieceTypeIndex::PAWN: generate_pawn_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::KNIGHT: generate_knight_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::BISHOP: generate_bishop_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::ROOK: generate_rook_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::QUEEN: generate_queen_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::KING: generate_king_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::NONE: // Should not happen if player_pieces_bb is correct.
            default: break;
        }
    }

    // --- Filter Pseudo-Legal Moves for Legality (using Make/Unmake) ---
    for (const auto& move : pseudo_legal_moves) {
        StateInfo info_for_undo; // StateInfo object to capture board state before move.
        
        board.apply_move(move, info_for_undo); // Apply the move to the ACTUAL board

        // After applying the move, the 'active_player' on `board` has *flipped* to the opponent's turn.
        // We need to check if the KING of the *original* active player is now in check.
        // `current_player` holds the original active player.
        if (!board.is_king_in_check(current_player)) {
            legal_moves.push_back(move);
        }
        
        board.undo_move(move, info_for_undo); // Undo the move to restore the board for the next iteration
    }
    
    return legal_moves;
}
