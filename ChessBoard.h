#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <cstdint> // For uint64_t
#include <string>  // For std::string
#include "Types.h" // Include our new types header for PlayerColor, GamePoint, PieceTypeIndex

// Forward declaration of Move struct (defined in Move.h).
struct Move;

// The ChessBoard struct using bitboard representation.
struct ChessBoard {
    // --- Piece Bitboards ---
    // Each bitboard represents all squares occupied by a specific piece type.
    // Convention: bit 0 = A1, bit 63 = H8 (or any consistent mapping).
    // A bit is set (1) if a piece of that type exists on the corresponding square.
    uint64_t white_pawns;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_rooks;
    uint64_t white_queens;
    uint64_t white_king;

    uint64_t black_pawns;
    uint64_t black_knights;
    uint64_t black_bishops;
    uint64_t black_rooks;
    uint64_t black_queens;
    uint64_t black_king;

    // --- Occupancy Bitboards ---
    // These are derived from the piece bitboards for quick checks of occupied squares.
    uint64_t occupied_squares;       // All squares with any piece (white_occupied_squares | black_occupied_squares).
    uint64_t white_occupied_squares; // All squares with white pieces.
    uint64_t black_occupied_squares; // All squares with black pieces.

    // --- Game State Information ---
    PlayerColor active_player; // Denotes whose turn it is to move (White or Black).

    // Castling rights represented as a 4-bit mask within a uint8_t:
    // Bit 0 (1): Black Queenside (q)
    // Bit 1 (2): Black Kingside (k)
    // Bit 2 (4): White Queenside (Q)
    // Bit 3 (8): White Kingside (K)
    uint8_t castling_rights_mask; 
    
    // En passant target square represented as a 0-63 index.
    // A value like 64 or -1 can indicate no en passant target is available.
    int en_passant_square_idx; 
    
    // Halfmove clock for the 50-move rule: number of halfmoves since the last pawn advance or capture.
    int halfmove_clock; 
    // Fullmove number: increments after Black's move. Starts at 1.
    int fullmove_number; 

    // --- Zobrist Hashing (Current Board Hash) ---
    // The current Zobrist hash of the board state. Crucial for transposition tables and repetition detection.
    uint64_t zobrist_hash;

    // --- Zobrist Keys (Static for the entire program, declared at end of variables) ---
    // These keys are generated once at program startup and used for all hash calculations.
    static uint64_t zobrist_piece_keys[12][64]; // [piece_type_and_color_index (0-11)][square_index (0-63)].
    static uint64_t zobrist_black_to_move_key; // Key for black to move.
    // Keys for 16 possible castling rights combinations (represented by the 4-bit castling_rights_mask).
    static uint64_t zobrist_castling_keys[16]; 
    static uint64_t zobrist_en_passant_keys[8]; // Keys for 8 possible en passant files (file a-h).
    static bool zobrist_keys_initialized; // Flag to ensure keys are initialized only once.

    // --- Member Functions ---

    // Constructors:
    ChessBoard(); // Default constructor: Initializes to the standard starting position.
    explicit ChessBoard(const std::string& fen); // FEN constructor: Initializes from a FEN string.

    // Core Board Manipulation:
    // Resets the board to the standard starting position.
    void reset_to_start_position();
    // Sets the board state from a FEN string.
    void set_from_fen(const std::string& fen);
    // Converts the current board state to a FEN string.
    std::string to_fen() const;
    // Applies a given move to the board, updating all bitboards and game state flags.
    // This method should handle captures, promotions, castling, and en passant.
    // It also updates the Zobrist hash incrementally.
    void apply_move(const Move& move);

    // Auxiliary Game Logic:
    // Determines if the king of the given color is currently in check.
    // Leverages bitboards for efficient attack detection.
    bool is_king_in_check(PlayerColor king_color) const;
    // Helper to get the square index (0-63) of a specific piece type and color.
    // Returns a special value (e.g., 64) if piece is not found.
    int get_piece_square_index(PieceTypeIndex piece_type_idx, PlayerColor piece_color) const;

    // Zobrist-related Methods (grouped at the bottom for readability):
    // Initializes the static Zobrist keys (should be called once at program startup).
    static void initialize_zobrist_keys();
    // Calculates the Zobrist hash of the current board state from scratch.
    // Used for initial setup (e.g., in FEN constructor) or for verification.
    uint64_t calculate_zobrist_hash_from_scratch() const;
    // Helper to toggle a piece's hash contribution when it moves or is captured.
    // This is used for incremental hash updates.
    void toggle_zobrist_piece(PieceTypeIndex piece_type_idx, PlayerColor piece_color, int square_idx);
};

#endif // CHESS_BOARD_H
