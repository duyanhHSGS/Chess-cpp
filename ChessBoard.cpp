#include "ChessBoard.h"
#include "ChessBitboardUtils.h" // For bit manipulation helpers and constants like RANK_2, A1_SQ_BB etc.
#include "Types.h"              // For PlayerColor, GamePoint, PieceTypeIndex (enum class), GameStatus
#include "Move.h"               // Required for the full definition of the Move struct in apply_move()
#include <random>    // For std::random_device, std::mt19937_64, std::uniform_int_distribution for Zobrist key generation
#include <chrono>    // For std::chrono::steady_clock to seed RNG
#include <iostream>  // For debugging output (std::cerr, std::cout)
#include <sstream>   // For std::stringstream to parse FEN strings
#include <cctype>    // For std::isdigit, std::isupper, std::tolower for FEN parsing
#include <algorithm> // For std::reverse (if used, not currently but commonly in string operations)

// ============================================================================
// Static Member Definitions (Zobrist Keys)
// ============================================================================

// Define and initialize the static member arrays for Zobrist hashing.
// These are declared in ChessBoard.h and must be defined (allocated) in a .cpp file.
uint64_t ChessBoard::zobrist_piece_keys[12][64]; // Stores a unique random key for each piece type (12) on each square (64)
uint64_t ChessBoard::zobrist_black_to_move_key; // Key for when it's Black's turn to move
uint64_t ChessBoard::zobrist_castling_keys[16];  // Keys for all 16 (2^4) possible castling rights combinations
uint64_t ChessBoard::zobrist_en_passant_keys[8]; // Keys for each of the 8 files that can be an en passant target square
bool ChessBoard::zobrist_keys_initialized = false; // Flag to ensure keys are initialized only once.

// ============================================================================
// Constructor and Initialization Methods
// ============================================================================

// Default constructor for ChessBoard.
// Initializes the board to the standard chess starting position.
ChessBoard::ChessBoard() {
    // Ensures Zobrist keys are generated only once across the entire program.
    if (!zobrist_keys_initialized) {
        initialize_zobrist_keys();
        zobrist_keys_initialized = true;
    }
    // Set the board state to the default chess starting position.
    reset_to_start_position();
}

// FEN string constructor for ChessBoard.
// Initializes the board state from a given FEN string.
ChessBoard::ChessBoard(const std::string& fen) {
    // Ensures Zobrist keys are generated only once.
    if (!zobrist_keys_initialized) {
        initialize_zobrist_keys();
        zobrist_keys_initialized = true;
    }
    // Parse the FEN string and set the board state accordingly.
    set_from_fen(fen);
}

// Initializes all static Zobrist hash keys with random 64-bit numbers.
// This function should be called exactly once during the program's lifetime.
void ChessBoard::initialize_zobrist_keys() {
    // Seed the random number generator using the current time for better randomness.
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist; // Creates a distribution for 64-bit unsigned integers

    // Generate unique random keys for each piece type on each square.
    // Loop through 12 piece types (6 white, 6 black) and 64 squares.
    for (int i = 0; i < 12; ++i) { // Piece type index (e.g., 0=White Pawn, 6=Black Pawn)
        for (int j = 0; j < 64; ++j) { // Square index (0-63)
            zobrist_piece_keys[i][j] = dist(rng);
        }
    }

    // Generate a unique random key for when it's Black's turn to move.
    zobrist_black_to_move_key = dist(rng);

    // Generate unique random keys for all 16 (2^4) possible castling rights combinations.
    // The castling_rights_mask ranges from 0 to 15.
    for (int i = 0; i < 16; ++i) {
        zobrist_castling_keys[i] = dist(rng);
    }

    // Generate unique random keys for each of the 8 files that can be an en passant target square.
    for (int i = 0; i < 8; ++i) {
        zobrist_en_passant_keys[i] = dist(rng);
    }
}

// Resets the chessboard to the standard chess starting position.
// This corresponds to the FEN string: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void ChessBoard::reset_to_start_position() {
    // Clear all existing piece bitboards to ensure a clean slate.
    white_pawns = 0ULL;
    white_knights = 0ULL;
    white_bishops = 0ULL;
    white_rooks = 0ULL;
    white_queens = 0ULL;
    white_king = 0ULL;

    black_pawns = 0ULL;
    black_knights = 0ULL;
    black_bishops = 0ULL;
    black_rooks = 0ULL;
    black_queens = 0ULL;
    black_king = 0ULL;

    // Set up white pieces on their starting squares using precomputed bitmasks from ChessBitboardUtils.
    white_pawns = ChessBitboardUtils::RANK_2; // All pawns on the second rank (A2-H2)
    white_rooks = ChessBitboardUtils::A1_SQ_BB | ChessBitboardUtils::H1_SQ_BB; // Rooks on A1 and H1
    white_knights = ChessBitboardUtils::B1_SQ_BB | ChessBitboardUtils::G1_SQ_BB; // Knights on B1 and G1
    white_bishops = ChessBitboardUtils::C1_SQ_BB | ChessBitboardUtils::F1_SQ_BB; // Bishops on C1 and F1
    white_queens = ChessBitboardUtils::D1_SQ_BB; // Queen on D1
    white_king = ChessBitboardUtils::E1_SQ_BB; // King on E1

    // Set up black pieces on their starting squares.
    black_pawns = ChessBitboardUtils::RANK_7; // All pawns on the seventh rank (A7-H7)
    black_rooks = ChessBitboardUtils::A8_SQ_BB | ChessBitboardUtils::H8_SQ_BB; // Rooks on A8 and H8
    black_knights = ChessBitboardUtils::B8_SQ_BB | ChessBitboardUtils::G8_SQ_BB; // Knights on B8 and G8
    black_bishops = ChessBitboardUtils::C8_SQ_BB | ChessBitboardUtils::F8_SQ_BB; // Bishops on C8 and F8
    black_queens = ChessBitboardUtils::D8_SQ_BB; // Queen on D8
    black_king = ChessBitboardUtils::E8_SQ_BB; // King on E8

    // Recalculate the overall and color-specific occupied squares bitboards.
    // These are derived by bitwise ORing all piece bitboards for each color.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares; // All occupied squares on the board

    // Initialize standard game state flags for the starting position.
    active_player = PlayerColor::White; // White moves first
    castling_rights_mask = ChessBitboardUtils::CASTLE_WK_BIT | ChessBitboardUtils::CASTLE_WQ_BIT |
                           ChessBitboardUtils::CASTLE_BK_BIT | ChessBitboardUtils::CASTLE_BQ_BIT; // All castling rights
    en_passant_square_idx = 64; // No en passant target square initially (64 indicates invalid/none)
    halfmove_clock = 0; // Number of halfmoves since last pawn move or capture (for 50-move rule)
    fullmove_number = 1; // Fullmove counter, starts at 1 and increments after Black's move

    // Calculate the initial Zobrist hash for the board state.
    zobrist_hash = calculate_zobrist_hash_from_scratch();
}

// ============================================================================
// FEN Conversion Methods
// ============================================================================

// Sets the board state from a FEN string.
// Example FEN: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
void ChessBoard::set_from_fen(const std::string& fen) {
    // 1. Clear all current piece bitboards to prepare for new FEN state.
    white_pawns = white_knights = white_bishops = white_rooks = white_queens = white_king = 0ULL;
    black_pawns = black_knights = black_bishops = black_rooks = black_queens = black_king = 0ULL;

    // Use a stringstream to easily parse the different parts of the FEN string.
    std::stringstream ss(fen);
    std::string board_part, active_color_part, castling_part, en_passant_part, halfmove_part, fullmove_part;

    // Extract each part of the FEN string.
    ss >> board_part >> active_color_part >> castling_part >> en_passant_part >> halfmove_part >> fullmove_part;

    // 2. Parse Piece Placement (the first part of the FEN string, e.g., "rnbqkbnr/...")
    int file = 0; // Current file (column) index (0-7, 'a' to 'h')
    int rank = 7; // Current rank (row) index (0-7). FEN ranks are 8 (top) down to 1 (bottom),
                  // so we start at internal rank 7 (corresponding to FEN rank 8).

    for (char c : board_part) {
        if (c == '/') {
            rank--; // Move to the next lower rank in internal representation.
            file = 0; // Reset file for the new rank.
        } else if (std::isdigit(c)) {
            // If character is a digit, it represents empty squares. Skip that many files.
            file += (c - '0'); 
        } else {
            // If it's a piece character, set the corresponding bit in the correct bitboard.
            // Map FEN rank/file to the internal square index (A1=0, H8=63).
            int square_idx = ChessBitboardUtils::rank_file_to_square(rank, file);

            // Use a switch statement to identify the piece and set its bit.
            switch (c) {
                case 'P': ChessBitboardUtils::set_bit(white_pawns, square_idx); break;
                case 'N': ChessBitboardUtils::set_bit(white_knights, square_idx); break;
                case 'B': ChessBitboardUtils::set_bit(white_bishops, square_idx); break;
                case 'R': ChessBitboardUtils::set_bit(white_rooks, square_idx); break;
                case 'Q': ChessBitboardUtils::set_bit(white_queens, square_idx); break;
                case 'K': ChessBitboardUtils::set_bit(white_king, square_idx); break;
                case 'p': ChessBitboardUtils::set_bit(black_pawns, square_idx); break;
                case 'n': ChessBitboardUtils::set_bit(black_knights, square_idx); break;
                case 'b': ChessBitboardUtils::set_bit(black_bishops, square_idx); break;
                case 'r': ChessBitboardUtils::set_bit(black_rooks, square_idx); break;
                case 'q': ChessBitboardUtils::set_bit(black_queens, square_idx); break;
                case 'k': ChessBitboardUtils::set_bit(black_king, square_idx); break;
                default:
                    std::cerr << "ERROR: Unknown piece character in FEN: " << c << std::endl;
                    return; // Exit or throw an exception for invalid FEN.
            }
            file++; // Move to the next file (rightwards).
        }
    }

    // After setting all pieces, recalculate the derived occupancy bitboards.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    // 3. Parse Active Color (e.g., "w" or "b")
    active_player = (active_color_part == "w") ? PlayerColor::White : PlayerColor::Black;

    // 4. Parse Castling Rights (e.g., "KQkq", "Kq", or "-")
    castling_rights_mask = 0; // Reset the mask before setting new rights.
    if (castling_part.find('K') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_WK_BIT; // White Kingside
    if (castling_part.find('Q') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_WQ_BIT; // White Queenside
    if (castling_part.find('k') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_BK_BIT; // Black Kingside
    if (castling_part.find('q') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_BQ_BIT; // Black Queenside

    // 5. Parse En Passant Target Square (e.g., "e3" or "-")
    if (en_passant_part == "-") {
        en_passant_square_idx = 64; // Set to an invalid index if no en passant target.
    } else {
        // Convert algebraic notation (e.g., "e3") to a square index (0-63).
        int file_char_idx = en_passant_part[0] - 'a'; // 'a'->0, 'b'->1, etc.
        int rank_char_idx = en_passant_part[1] - '1'; // '1'->0, '2'->1, etc.
        en_passant_square_idx = ChessBitboardUtils::rank_file_to_square(rank_char_idx, file_char_idx);
    }

    // 6. Parse Halfmove Clock (for 50-move rule, e.g., "0")
    halfmove_clock = std::stoi(halfmove_part);

    // 7. Parse Fullmove Number (e.g., "1")
    fullmove_number = std::stoi(fullmove_part);

    // 8. Calculate the Zobrist Hash from the newly set board state.
    // This provides the hash for the current position after FEN parsing.
    zobrist_hash = calculate_zobrist_hash_from_scratch();
}

// Converts the current board state to a FEN string.
// Example output: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
std::string ChessBoard::to_fen() const {
    std::string fen_pieces = ""; // To build the piece placement part of the FEN.

    // 1. Generate Piece Placement (from rank 8 down to 1).
    for (int rank = 7; rank >= 0; --rank) { // Internal rank 7 (FEN rank 8) down to 0 (FEN rank 1)
        int empty_count = 0; // Counter for consecutive empty squares.
        for (int file = 0; file < 8; ++file) { // Internal file 0 ('a') to 7 ('h')
            int square_idx = ChessBitboardUtils::rank_file_to_square(rank, file); // Get 0-63 index for current square.

            char piece_char = ' '; // Default to an empty square character.
            // Check each piece bitboard to determine which piece (if any) occupies the current square.
            // Order of checks matters for performance in a real engine (most common pieces first),
            // but for correctness, any order is fine as long as it's exhaustive.
            if (ChessBitboardUtils::test_bit(white_pawns, square_idx)) piece_char = 'P';
            else if (ChessBitboardUtils::test_bit(white_knights, square_idx)) piece_char = 'N';
            else if (ChessBitboardUtils::test_bit(white_bishops, square_idx)) piece_char = 'B';
            else if (ChessBitboardUtils::test_bit(white_rooks, square_idx)) piece_char = 'R';
            else if (ChessBitboardUtils::test_bit(white_queens, square_idx)) piece_char = 'Q';
            else if (ChessBitboardUtils::test_bit(white_king, square_idx)) piece_char = 'K';
            else if (ChessBitboardUtils::test_bit(black_pawns, square_idx)) piece_char = 'p';
            else if (ChessBitboardUtils::test_bit(black_knights, square_idx)) piece_char = 'n';
            else if (ChessBitboardUtils::test_bit(black_bishops, square_idx)) piece_char = 'b';
            else if (ChessBitboardUtils::test_bit(black_rooks, square_idx)) piece_char = 'r';
            else if (ChessBitboardUtils::test_bit(black_queens, square_idx)) piece_char = 'q';
            else if (ChessBitboardUtils::test_bit(black_king, square_idx)) piece_char = 'k';

            if (piece_char != ' ') {
                // If a piece is found, append any accumulated empty count first.
                if (empty_count > 0) {
                    fen_pieces += std::to_string(empty_count); 
                    empty_count = 0; // Reset empty count.
                }
                fen_pieces += piece_char; // Append the piece character.
            } else {
                empty_count++; // Increment empty square count.
            }
        }
        // After iterating through all files in a rank, add any remaining empty count.
        if (empty_count > 0) {
            fen_pieces += std::to_string(empty_count); 
        }
        // Add a rank separator '/' unless it's the last rank (internal rank 0).
        if (rank > 0) {
            fen_pieces += '/'; 
        }
    }

    std::string fen_full = fen_pieces; // Start building the full FEN string.

    // 2. Append Active Color (e.g., " w" or " b")
    fen_full += (active_player == PlayerColor::White) ? " w" : " b";

    // 3. Append Castling Rights (e.g., "KQkq", "Kq", or "-")
    std::string castling_str = "";
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_WK_BIT) castling_str += 'K';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_WQ_BIT) castling_str += 'Q';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_BK_BIT) castling_str += 'k';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_BQ_BIT) castling_str += 'q';
    if (castling_str.empty()) castling_str = "-"; // If no castling rights, use '-'
    fen_full += " " + castling_str;

    // 4. Append En Passant Target Square (e.g., "e3" or " -")
    // Check if a valid en passant target exists (not 64).
    if (en_passant_square_idx >= 0 && en_passant_square_idx < 64) {
        // Convert the square index back to algebraic notation.
        char file_char = 'a' + ChessBitboardUtils::square_to_file(en_passant_square_idx);
        char rank_char = '1' + ChessBitboardUtils::square_to_rank(en_passant_square_idx);
        fen_full += " " + std::string(1, file_char) + std::string(1, rank_char);
    } else {
        fen_full += " -"; // No en passant target.
    }

    // 5. Append Halfmove Clock
    fen_full += " " + std::to_string(halfmove_clock);

    // 6. Append Fullmove Number
    fen_full += " " + std::to_string(fullmove_number);

    return fen_full;
}

// ============================================================================
// Core Board Manipulation Methods (Make/Unmake)
// ============================================================================

// Applies a given move to the board, updating all bitboards and game state flags.
// It also fills the provided StateInfo object with the board's state *before* the move,
// which is essential for `undo_move`.
void ChessBoard::apply_move(const Move& move, StateInfo& state_info) {
    // 1. Save current board state into `state_info` for `undo_move`.
    state_info.previous_castling_rights_mask = castling_rights_mask;
    state_info.previous_en_passant_square_idx = en_passant_square_idx;
    state_info.previous_halfmove_clock = halfmove_clock;
    state_info.previous_fullmove_number = fullmove_number;
    state_info.previous_active_player = active_player; // Save the current active player
    state_info.captured_piece_type_idx = PieceTypeIndex::NONE; // Default, updated if capture occurs
    state_info.captured_piece_color = PlayerColor::White;      // Default
    state_info.captured_square_idx = 64;                       // Default

    // 2. Convert GamePoint (file, rank) from the `Move` struct to internal 0-63 square indices.
    int from_sq = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
    int to_sq = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);

    // 3. Update Halfmove Clock and Fullmove Number.
    // Reset halfmove clock if pawn moves or a piece is captured.
    if (move.piece_moved_type_idx == PieceTypeIndex::PAWN || move.piece_captured_type_idx != PieceTypeIndex::NONE) {
        halfmove_clock = 0; 
    } else {
        halfmove_clock++;
    }
    // Increment fullmove number after Black completes a move.
    if (active_player == PlayerColor::Black) {
        fullmove_number++; 
    }

    // 4. Update Zobrist Hash for the side to move (always toggle).
    // This MUST be done *before* flipping active_player.
    zobrist_hash ^= zobrist_black_to_move_key;

    // 5. Update Zobrist Hash for Castling Rights (XOR out old rights).
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask];

    // 6. Update Zobrist Hash for En Passant Target (XOR out old target if it existed).
    if (en_passant_square_idx != 64) { 
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }
    // IMPORTANT: en_passant_square_idx is reset BEFORE new one is set in step 12
    en_passant_square_idx = 64; // Reset to no en passant target by default for the next turn.

    // 7. Move the piece on bitboards and update its Zobrist hash.
    uint64_t* moving_piece_bb_ptr = nullptr; // Initialize to nullptr for safety
    if (active_player == PlayerColor::White) {
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &white_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &white_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &white_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &white_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &white_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &white_king; break;
            default: // Should not happen with valid moves from MoveGenerator
                std::cerr << "ERROR: Invalid piece_moved_type_idx in apply_move (White)." << std::endl;
                return; // Or throw exception
        }
    } else { // Black player is active.
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &black_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &black_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &black_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &black_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &black_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &black_king; break;
            default: // Should not happen
                std::cerr << "ERROR: Invalid piece_moved_type_idx in apply_move (Black)." << std::endl;
                return; // Or throw exception
        }
    }

    // Safety check: ensure pointer is valid before dereferencing
    if (moving_piece_bb_ptr == nullptr) {
        std::cerr << "CRITICAL ERROR: moving_piece_bb_ptr is null in apply_move." << std::endl;
        return; // Or throw an exception
    }


    // Toggle (XOR out) the piece's hash contribution from its original square.
    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, from_sq);
    // Clear the piece's bit from its original square on the bitboard.
    ChessBitboardUtils::clear_bit(*moving_piece_bb_ptr, from_sq);
    // Set the piece's bit on its new square on the bitboard.
    ChessBitboardUtils::set_bit(*moving_piece_bb_ptr, to_sq);
    // Toggle (XOR in) the piece's hash contribution at its new square.
    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq);

    // 8. Handle Captures (if `piece_captured_type_idx` is not NONE).
    if (move.piece_captured_type_idx != PieceTypeIndex::NONE) { 
        // Save captured piece info to state_info
        state_info.captured_piece_type_idx = move.piece_captured_type_idx;
        state_info.captured_piece_color = (active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
        
        int captured_sq = to_sq; // By default, captured piece is on the `to_sq`.

        uint64_t* captured_piece_bb_ptr = nullptr; // Initialize for safety
        if (state_info.captured_piece_color == PlayerColor::White) { // Captured White piece
            switch (move.piece_captured_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &white_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &white_queens; break;
                case PieceTypeIndex::KING: // King cannot be captured in a normal move
                case PieceTypeIndex::NONE: default: return; // Should not happen with valid move data
            }
        } else { // Captured Black piece
            switch (move.piece_captured_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &black_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &black_queens; break;
                case PieceTypeIndex::KING: // King cannot be captured
                case PieceTypeIndex::NONE: default: return; // Should not happen
            }
        }
        
        if (captured_piece_bb_ptr == nullptr) {
            std::cerr << "CRITICAL ERROR: captured_piece_bb_ptr is null in apply_move." << std::endl;
            return;
        }

        if (move.is_en_passant) {
            // For en passant, the captured pawn is not on `to_sq`.
            if (active_player == PlayerColor::White) { // White pawn captured black pawn
                captured_sq = to_sq - 8; // Black pawn was one rank below the target square.
            } else { // Black pawn captured white pawn
                captured_sq = to_sq + 8; // White pawn was one rank above the target square.
            }
        }
        state_info.captured_square_idx = captured_sq; // Save captured square for undo.

        // Clear captured piece from its square (Bitboards).
        ChessBitboardUtils::clear_bit(*captured_piece_bb_ptr, captured_sq);
        // Toggle captured piece from Zobrist hash (XOR out its hash contribution).
        toggle_zobrist_piece(move.piece_captured_type_idx, state_info.captured_piece_color, captured_sq);
    }

    // 9. Handle Castling (King and Rook move together).
    if (move.is_kingside_castle) { // Kingside castling (e.g., E1G1 for White, E8G8 for Black).
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::H1_SQ; // White Kingside Rook's starting square.
            rook_to_sq = ChessBitboardUtils::F1_SQ;   // White Kingside Rook's destination square.
            rook_bb_ptr = &white_rooks;
        } else { // Black Kingside
            rook_from_sq = ChessBitboardUtils::H8_SQ; // Black Kingside Rook's starting square.
            rook_to_sq = ChessBitboardUtils::F8_SQ;   // Black Kingside Rook's destination square.
            rook_bb_ptr = &black_rooks;
        }
        // Move the rook: clear from old, set to new, and toggle Zobrist hash.
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_from_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_to_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);
    } else if (move.is_queenside_castle) { // Queenside castling (e.g., E1C1 for White, E8C8 for Black).
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::A1_SQ; // White Queenside Rook's starting square.
            rook_to_sq = ChessBitboardUtils::D1_SQ;   // White Queenside Rook's destination square.
            rook_bb_ptr = &white_rooks;
        } else { // Black Queenside
            rook_from_sq = ChessBitboardUtils::A8_SQ; // Black Queenside Rook's starting square.
            rook_to_sq = ChessBitboardUtils::D8_SQ;   // Black Queenside Rook's destination square.
            rook_bb_ptr = &black_rooks;
        }
        // Move the rook: clear from old, set to new, and toggle Zobrist hash.
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_from_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_to_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);
    }

    // 10. Handle Pawn Promotion.
    if (move.is_promotion) {
        // First, remove the pawn from the board and from the Zobrist hash at the promotion square.
        // The pawn's hash at `to_sq` was added in step 7, so we need to remove it to replace with promoted piece.
        toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq); // Remove pawn hash
        if (active_player == PlayerColor::White) {
            ChessBitboardUtils::clear_bit(white_pawns, to_sq); // Clear pawn bit
        } else {
            ChessBitboardUtils::clear_bit(black_pawns, to_sq); // Clear pawn bit
        }

        // Then, add the promoted piece (Queen, Knight, Rook, Bishop) to the target square.
        uint64_t* promoted_bb_ptr = nullptr; // Initialize for safety
        if (active_player == PlayerColor::White) {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &white_queens; break;
                case PieceTypeIndex::NONE: // Fallthrough or error: must promote to valid piece
                case PieceTypeIndex::PAWN: // Cannot promote to pawn or king
                case PieceTypeIndex::KING: default: return; 
            }
        } else { // Black
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &black_queens; break;
                case PieceTypeIndex::NONE: // Fallthrough or error
                case PieceTypeIndex::PAWN: // Cannot promote to pawn or king
                case PieceTypeIndex::KING: default: return; 
            }
        }

        if (promoted_bb_ptr == nullptr) {
            std::cerr << "CRITICAL ERROR: promoted_bb_ptr is null in apply_move (promotion)." << std::endl;
            return;
        }

        ChessBitboardUtils::set_bit(*promoted_bb_ptr, to_sq); // Set bit for the new promoted piece.
        toggle_zobrist_piece(move.promotion_piece_type_idx, active_player, to_sq); // Add promoted piece's hash.
    }

    // 11. Update Castling Rights Mask.
    // If the king moved, all castling rights for that color are permanently lost.
    if (move.piece_moved_type_idx == PieceTypeIndex::KING) {
        if (active_player == PlayerColor::White) { // Active player is still white at this point
            castling_rights_mask &= ~(ChessBitboardUtils::CASTLE_WK_BIT | ChessBitboardUtils::CASTLE_WQ_BIT);
        } else { // Active player is still black at this point
            castling_rights_mask &= ~(ChessBitboardUtils::CASTLE_BK_BIT | ChessBitboardUtils::CASTLE_BQ_BIT);
        }
    }
    // If a rook moved from its starting square, or was captured on its starting square,
    // that specific castling right is lost.
    if (move.piece_moved_type_idx == PieceTypeIndex::ROOK) {
        if (from_sq == ChessBitboardUtils::A1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WQ_BIT; // White Queenside Rook
        if (from_sq == ChessBitboardUtils::H1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WK_BIT; // White Kingside Rook
        if (from_sq == ChessBitboardUtils::A8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BQ_BIT; // Black Queenside Rook
        if (from_sq == ChessBitboardUtils::H8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BK_BIT; // Black Kingside Rook
    }
    // Check if a rook was captured on its initial square.
    if (move.piece_captured_type_idx == PieceTypeIndex::ROOK) {
        if (to_sq == ChessBitboardUtils::A1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WQ_BIT;
        if (to_sq == ChessBitboardUtils::H1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WK_BIT;
        if (to_sq == ChessBitboardUtils::A8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BQ_BIT;
        if (to_sq == ChessBitboardUtils::H8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BK_BIT;
    }
    // The previous implementation was missing the XOR-in for new castling rights hash here.
    // As per user instruction, I am not adding it back.


    // 12. Set new En Passant Target Square (if the move was a double pawn push).
    if (move.is_double_pawn_push) {
        // The target square for en passant is the square *behind* the pawn that just moved two squares.
        if (active_player == PlayerColor::White) { // White pawn moved from rank 2 to 4 (e.g., E2 to E4)
            en_passant_square_idx = to_sq - 8; // Target is on rank 3 (e.g., E3).
        } else { // Black pawn moved from rank 7 to 5 (e.g., E7 to E5)
            en_passant_square_idx = to_sq + 8; // Target is on rank 6 (e.g., E6).
        }
        // Toggle (XOR in) the new en passant target hash.
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    // 13. Update Occupancy Bitboards after all piece movements, captures, and promotions are finalized.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    // 14. Toggle Active Player for the next turn. This is crucial for correct turn management.
    active_player = (active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
}

// Undoes a previously applied move, restoring the board to its state before the move.
// It uses the information from the provided StateInfo object (which was filled by apply_move).
void ChessBoard::undo_move(const Move& move, const StateInfo& state_info) {
    // 1. Convert GamePoint (file, rank) to internal square index (0-63).
    int from_sq = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
    int to_sq = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);

    // Order of operations in undo_move is the reverse of apply_move's Zobrist updates.

    // 2. Restore active player first (this affects which piece keys are XORed back).
    active_player = state_info.previous_active_player;
    // XOR back the side to move hash (since it was XORed in apply_move).
    zobrist_hash ^= zobrist_black_to_move_key;

    // 3. Reverse En Passant hash update.
    // If the new EP target was set, XOR it out.
    if (en_passant_square_idx != 64) {
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }
    // Restore the old EP target index.
    en_passant_square_idx = state_info.previous_en_passant_square_idx;
    // If the old EP target was valid, XOR its hash back in.
    if (en_passant_square_idx != 64) {
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    // 4. Reverse Castling Rights hash update.
    // The previous implementation was missing the XOR-in for new castling rights hash in apply_move,
    // so the corresponding XOR-out here is also logically incorrect if apply_move didn't add it.
    // As per user instruction, I am not modifying this logic.
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask]; // This XORs out the current (possibly incorrect) hash.
    castling_rights_mask = state_info.previous_castling_rights_mask; // Restore the old mask.
    // The previous implementation was missing the XOR-in for old castling rights hash here.
    // As per user instruction, I am not adding it back.


    // 5. Undo Pawn Promotion (reverse step 10 in apply_move).
    if (move.is_promotion) {
        // Remove the promoted piece (e.g., Queen) from `to_sq` from bitboard and hash.
        toggle_zobrist_piece(move.promotion_piece_type_idx, active_player, to_sq);
        uint64_t* promoted_bb_ptr = nullptr; // Initialize for safety, not directly used to modify for undo, but for correct switch logic
        
        if (active_player == PlayerColor::White) {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &white_queens; break;
                default: break; // Should not happen
            }
            if (promoted_bb_ptr) ChessBitboardUtils::clear_bit(*promoted_bb_ptr, to_sq); // Clear promoted piece
            ChessBitboardUtils::set_bit(white_pawns, to_sq);       // Put pawn back
        } else { // Black
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &black_queens; break;
                default: break; // Should not happen
            }
            if (promoted_bb_ptr) ChessBitboardUtils::clear_bit(*promoted_bb_ptr, to_sq); // Clear promoted piece
            ChessBitboardUtils::set_bit(black_pawns, to_sq);       // Put pawn back
        }
        toggle_zobrist_piece(PieceTypeIndex::PAWN, active_player, to_sq); // Add pawn hash back
    }

    // 6. Undo Castling Rook Move (reverse step 9 in apply_move).
    if (move.is_kingside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) { // Active player is original active player
            rook_from_sq = ChessBitboardUtils::H1_SQ;
            rook_to_sq = ChessBitboardUtils::F1_SQ;
            rook_bb_ptr = &white_rooks;
        } else { // Black
            rook_from_sq = ChessBitboardUtils::H8_SQ;
            rook_to_sq = ChessBitboardUtils::F8_SQ;
            rook_bb_ptr = &black_rooks;
        }
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq); // Remove rook hash from F1/F8
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_to_sq);             // Clear rook from F1/F8
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_from_sq);             // Set rook back to H1/H8
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq); // Add rook hash back to H1/H8
    } else if (move.is_queenside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::A1_SQ;
            rook_to_sq = ChessBitboardUtils::D1_SQ;
            rook_bb_ptr = &white_rooks;
        } else {
            rook_from_sq = ChessBitboardUtils::A8_SQ;
            rook_to_sq = ChessBitboardUtils::D8_SQ;
            rook_bb_ptr = &black_rooks;
        }
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq); // Remove rook hash from D1/D8
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_to_sq);             // Clear rook from D1/D8
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_from_sq);             // Set rook back to A1/A8
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq); // Add rook hash back to A1/A8
    }

    // 7. Move the piece back (reverse step 7 in apply_move).
    // Identify the bitboard for the moving piece.
    uint64_t* moving_piece_bb_ptr = nullptr; // Initialize for safety
    if (active_player == PlayerColor::White) { // Use active_player as it was *before* the move (now restored)
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &white_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &white_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &white_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &white_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &white_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &white_king; break;
            case PieceTypeIndex::NONE: default: return;
        }
    } else { // Black
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &black_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &black_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &black_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &black_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &black_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &black_king; break;
            case PieceTypeIndex::NONE: default: return;
        }
    }

    if (moving_piece_bb_ptr == nullptr) {
        std::cerr << "CRITICAL ERROR: moving_piece_bb_ptr is null in undo_move." << std::endl;
        return;
    }

    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq); // Remove hash from new square (where it was moved to)
    ChessBitboardUtils::clear_bit(*moving_piece_bb_ptr, to_sq);             // Clear bit from new square
    ChessBitboardUtils::set_bit(*moving_piece_bb_ptr, from_sq);             // Set bit back to old square
    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, from_sq); // Add hash back to old square


    // 8. Restore Halfmove Clock and Fullmove Number (reverse step 3 in apply_move).
    halfmove_clock = state_info.previous_halfmove_clock;
    fullmove_number = state_info.previous_fullmove_number;

    // 9. Update Occupancy Bitboards after all piece movements, captures, and promotions are reverted.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;
}


// ============================================================================
// Auxiliary Game Logic Methods
// ============================================================================

// Determines if the king of the given color is currently in check.
// This function leverages bitboards and precomputed attack tables for efficiency.
bool ChessBoard::is_king_in_check(PlayerColor king_color) const {
    uint64_t king_bitboard = (king_color == PlayerColor::White) ? white_king : black_king;
    // If for some reason the king bitboard is empty (e.g., in a test scenario), it cannot be in check.
    if (king_bitboard == 0ULL) return false;

    // Get the square index of the king. Assumes there's only one king for the given color.
    int king_sq = ChessBitboardUtils::get_lsb_index(king_bitboard); 

    // Determine the color of the potential attacking pieces (opposite of the king's color).
    PlayerColor attacking_color = (king_color == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;

    // --- Check for attacks from various enemy piece types ---
    // These `is_X_attacked_by` functions are implemented in ChessBitboardUtils.h/.cpp
    // and use precomputed attack tables or sliding piece logic.

    // 1. Check for attacks from enemy Pawns.
    uint64_t enemy_pawns_bb = (attacking_color == PlayerColor::White) ? white_pawns : black_pawns;
    if (ChessBitboardUtils::is_pawn_attacked_by(king_sq, enemy_pawns_bb, attacking_color)) return true;

    // 2. Check for attacks from enemy Knights.
    uint64_t enemy_knights_bb = (attacking_color == PlayerColor::White) ? white_knights : black_knights;
    if (ChessBitboardUtils::is_knight_attacked_by(king_sq, enemy_knights_bb)) return true;

    // 3. Check for attacks from enemy Kings (mainly for ensuring kings don't move into adjacent squares).
    uint64_t enemy_king_bb = (attacking_color == PlayerColor::White) ? white_king : black_king;
    if (ChessBitboardUtils::is_king_attacked_by(king_sq, enemy_king_bb)) return true;

    // 4. Check for attacks from Rooks and Queens (horizontal/vertical sliding attacks).
    uint64_t enemy_rooks_bb = (attacking_color == PlayerColor::White) ? white_rooks : black_rooks;
    uint64_t enemy_queens_bb = (attacking_color == PlayerColor::White) ? white_queens : black_queens;
    // Combine their bitboards as they share attack patterns.
    uint64_t rook_queen_attackers = enemy_rooks_bb | enemy_queens_bb;
    // `occupied_squares` is crucial here to determine if a sliding piece's attack path is blocked.
    if (ChessBitboardUtils::is_rook_queen_attacked_by(king_sq, rook_queen_attackers, occupied_squares)) return true;

    // 5. Check for attacks from Bishops and Queens (diagonal sliding attacks).
    uint64_t enemy_bishops_bb = (attacking_color == PlayerColor::White) ? white_bishops : black_bishops;
    // Combine their bitboards.
    uint64_t bishop_queen_attackers = enemy_bishops_bb | enemy_queens_bb;
    // Again, `occupied_squares` is needed for blocking.
    if (ChessBitboardUtils::is_bishop_queen_attacked_by(king_sq, bishop_queen_attackers, occupied_squares)) return true;

    return false; // If no attacks found, the king is not in check.
}

// Helper to get the square index (0-63) of a specific piece type and color.
// This is typically used for finding the king's position or a specific piece in a known unique scenario.
// piece_type_idx: The type of piece (e.g., PieceTypeIndex::PAWN).
// piece_color: The color of the piece (e.g., PlayerColor::White).
// Returns 64 if the piece is not found on the board.
int ChessBoard::get_piece_square_index(PieceTypeIndex piece_type_idx, PlayerColor piece_color) const {
    uint64_t target_bb = 0ULL; // Initialize with an empty bitboard.

    // Select the correct piece bitboard based on color and type.
    if (piece_color == PlayerColor::White) {
        switch (piece_type_idx) {
            case PieceTypeIndex::PAWN: target_bb = white_pawns; break;
            case PieceTypeIndex::KNIGHT: target_bb = white_knights; break;
            case PieceTypeIndex::BISHOP: target_bb = white_bishops; break;
            case PieceTypeIndex::ROOK: target_bb = white_rooks; break;
            case PieceTypeIndex::QUEEN: target_bb = white_queens; break;
            case PieceTypeIndex::KING: target_bb = white_king; break;
            case PieceTypeIndex::NONE: return 64; // No piece type specified.
        }
    } else { // Black pieces
        switch (piece_type_idx) {
            case PieceTypeIndex::PAWN: target_bb = black_pawns; break;
            case PieceTypeIndex::KNIGHT: target_bb = black_knights; break;
            case PieceTypeIndex::BISHOP: target_bb = black_bishops; break;
            case PieceTypeIndex::ROOK: target_bb = black_rooks; break;
            case PieceTypeIndex::QUEEN: target_bb = black_queens; break;
            case PieceTypeIndex::KING: target_bb = black_king; break;
            case PieceTypeIndex::NONE: return 64; // No piece type specified.
        }
    }

    // If the target bitboard has any bits set, return the index of the least significant set bit (LSB).
    // For pieces like the King, there should only be one bit set. For other pieces, this returns one arbitrary location.
    if (target_bb != 0ULL) {
        return ChessBitboardUtils::get_lsb_index(target_bb);
    }
    return 64; // Return 64 to indicate the piece was not found.
}

// ============================================================================
// Zobrist-related Methods (Grouped for Readability)
// ============================================================================

// Calculates the Zobrist hash of the current board state from scratch.
// This is typically done once when the board is initialized (e.g., from FEN).
uint64_t ChessBoard::calculate_zobrist_hash_from_scratch() const {
    uint64_t hash = 0ULL; // Start with a zero hash.

    // 1. XOR in hash keys for all pieces on their respective squares.
    // Iterate through all 64 squares and identify the piece occupying each square, then XOR its corresponding key.
    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        // Check for white pieces.
        if (ChessBitboardUtils::test_bit(white_pawns, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::PAWN) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_knights, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KNIGHT) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_bishops, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::BISHOP) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_rooks, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::ROOK) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_queens, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::QUEEN) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_king, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KING) + 0][square_idx];
        // Check for black pieces (PieceTypeIndex values are offset by 6 for black).
        else if (ChessBitboardUtils::test_bit(black_pawns, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::PAWN) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_knights, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KNIGHT) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_bishops, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::BISHOP) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_rooks, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::ROOK) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_queens, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::QUEEN) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_king, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KING) + 6][square_idx];
    }

    // 2. XOR in the hash key for the current side to move.
    if (active_player == PlayerColor::Black) {
        hash ^= zobrist_black_to_move_key;
    }

    // 3. XOR in the hash key for the current castling rights.
    hash ^= zobrist_castling_keys[castling_rights_mask];

    // 4. XOR in the hash key for the en passant target square (if one exists).
    // The key is based only on the file of the en passant target square.
    if (en_passant_square_idx != 64) { // 64 indicates no en passant target.
        hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    return hash;
}

// Helper function to toggle (XOR in/out) a piece's contribution to the Zobrist hash.
// This is used for incremental hash updates (e.g., when a piece moves or is captured/promoted).
// piece_type_idx: The type of piece (e.g., PieceTypeIndex::PAWN).
// piece_color: The color of the piece (e.g., PlayerColor::White).
// square_idx: The square index (0-63) where the piece is being toggled.
// Example: To remove a white pawn from A2 (square 8) from the hash:
//   toggle_zobrist_piece(PieceTypeIndex::PAWN, PlayerColor::White, ChessBitboardUtils::A2_SQ);
// Example: To add a white pawn to A3 (square 16) to the hash:
//   toggle_zobrist_piece(PieceTypeIndex::PAWN, PlayerColor::White, ChessBitboardUtils::A3_SQ);
void ChessBoard::toggle_zobrist_piece(PieceTypeIndex piece_type_idx, PlayerColor piece_color, int square_idx) {
    // Determine the correct overall Zobrist index (0-11) for the piece based on its type and color.
    int zobrist_index = static_cast<int>(piece_type_idx) + (piece_color == PlayerColor::White ? 0 : 6);
    // XOR the current Zobrist hash with the key for this piece on this square.
    // XORing a value twice with the same key will revert it, which is how incremental updates work.
    zobrist_hash ^= zobrist_piece_keys[zobrist_index][square_idx];
}
