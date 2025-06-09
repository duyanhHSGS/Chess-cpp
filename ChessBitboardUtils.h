#ifndef CHESS_BITBOARD_UTILS_H
#define CHESS_BITBOARD_UTILS_H

#include <cstdint> // For uint64_t
#include <string>  // For std::string
#include <vector>  // For std::vector (e.g., in get_set_bits)
#include <array>   // For std::array (used for deltas in MoveGenerator, indirectly related here)
#include "Types.h" // For PlayerColor, PieceTypeIndex, GamePoint

// ============================================================================
// Compiler Intrinsics Definitions (New additions for speed)
// ============================================================================

// Check for GCC/Clang built-in functions
#if defined(__GNUC__) || defined(__clang__)
#define HAS_BUILTIN_CTZLL // Count Trailing Zeros Long Long (LSB)
#define HAS_BUILTIN_CLZLL // Count Leading Zeros Long Long (MSB)
#define HAS_BUILTIN_POPCOUNTLL // Population Count Long Long (set bits)
#endif

// Check for MSVC intrinsics
#if defined(_MSC_VER)
#include <intrin.h> // Required for MSVC intrinsics
#define HAS_MSVC_INTRINSICS
#endif

// ============================================================================
// End Compiler Intrinsics Definitions
// ============================================================================

// Forward declaration of Move struct from Move.h, as it's used in move_to_string.
struct Move;

// The ChessBitboardUtils struct provides a collection of static utility functions
// and precomputed bitmasks/attack tables to aid in efficient bitboard manipulation
// and attack detection within a chess engine.
struct ChessBitboardUtils {
	// ============================================================================
	// Static Member Definitions (Bitmasks and Attack Tables)
	// ============================================================================

	// Rank masks: A bitboard with all bits set for a given rank (0-7).
	static const uint64_t RANK_1; // 0x00000000000000FFULL (rank 1 / index 0-7)
	static const uint64_t RANK_2;
	static const uint64_t RANK_3;
	static const uint64_t RANK_4;
	static const uint64_t RANK_5;
	static const uint64_t RANK_6;
	static const uint64_t RANK_7;
	static const uint64_t RANK_8; // 0xFF00000000000000ULL (rank 8 / index 56-63)

	// File masks: A bitboard with all bits set for a given file (A-H).
	static const uint64_t FILE_A; // 0x0101010101010101ULL
	static const uint64_t FILE_B;
	static const uint64_t FILE_C;
	static const uint64_t FILE_D;
	static const uint64_t FILE_E;
	static const uint64_t FILE_F;
	static const uint64_t FILE_G;
	static const uint64_t FILE_H; // 0x8080808080808080ULL

	// Individual square bitboards.
	static const uint64_t A1_SQ_BB; // 1ULL << 0
	static const uint64_t H1_SQ_BB; // 1ULL << 7
	static const uint64_t A8_SQ_BB; // 1ULL << 56
	static const uint64_t H8_SQ_BB; // 1ULL << 63
	static const uint64_t E1_SQ_BB; // 1ULL << 4
	static const uint64_t E8_SQ_BB; // 1ULL << 60
	static const uint64_t B1_SQ_BB;
	static const uint64_t G1_SQ_BB;
	static const uint64_t C1_SQ_BB;
	static const uint64_t F1_SQ_BB;
	static const uint64_t B8_SQ_BB;
	static const uint64_t G8_SQ_BB;
	static const uint64_t C8_SQ_BB;
	static const uint64_t F8_SQ_BB;
	static const uint64_t D1_SQ_BB;
	static const uint64_t D8_SQ_BB;


	// Individual square indices (0-63).
	static const int A1_SQ; // 0
	static const int B1_SQ;
	static const int C1_SQ;
	static const int D1_SQ;
	static const int E1_SQ;
	static const int F1_SQ;
	static const int G1_SQ;
	static const int H1_SQ; // 7

	static const int A8_SQ; // 56
	static const int B8_SQ;
	static const int C8_SQ;
	static const int D8_SQ;
	static const int E8_SQ;
	static const int F8_SQ;
	static const int G8_SQ;
	static const int H8_SQ; // 63

	// Castling bitmasks (for castling_rights_mask in ChessBoard).
	static const uint8_t CASTLE_WK_BIT; // White Kingside (K)
	static const uint8_t CASTLE_WQ_BIT; // White Queenside (Q)
	static const uint8_t CASTLE_BK_BIT; // Black Kingside (k)
	static const uint8_t CASTLE_BQ_BIT; // Black Queenside (q)

	// Precomputed attack tables for non-sliding pieces.
	static uint64_t knight_attacks[64];      // Stores attack masks for a knight on each square.
	static uint64_t king_attacks[64];        // Stores attack masks for a king on each square.
	// Consolidated pawn attacks into a 2D array [PlayerColor][square_idx]
	static uint64_t pawn_attacks[2][64];
	static bool tables_initialized; // Flag to ensure tables are initialized only once.


	// ============================================================================
	// Initialization of Static Attack Tables
	// ============================================================================

	// Initializes all static attack tables. Must be called once at program startup.
	static void initialize_attack_tables();

	// Helper functions to generate individual attack masks.
	static uint64_t generate_knight_attacks(int square_idx);
	static uint64_t generate_king_attacks(int square_idx);
	static uint64_t generate_pawn_attacks(int square_idx, PlayerColor color);

	// ============================================================================
	// Bit Manipulation Functions (Optimized with intrinsics)
	// ============================================================================

	// Sets the bit at 'square_idx' in 'bitboard'.
	static void set_bit(uint64_t& bitboard, int square_idx);
	// Clears the bit at 'square_idx' in 'bitboard'.
	static void clear_bit(uint64_t& bitboard, int square_idx);
	// Returns true if the bit at 'square_idx' in 'bitboard' is set.
	static bool test_bit(uint64_t bitboard, int square_idx);

	// Returns the index of the least significant bit (LSB) set in 'bitboard'.
	// Returns 64 if bitboard is 0. (Optimized with intrinsics)
	static uint8_t get_lsb_index(uint64_t bitboard);

	// Returns the index of the most significant bit (MSB) set in 'bitboard'.
	// Returns 64 if bitboard is 0. (New function, optimized with intrinsics)
	static uint8_t get_msb_index(uint64_t bitboard);

	// Counts the number of set bits (population count) in a bitboard.
	// (Optimized with intrinsics)
	static uint8_t count_set_bits(uint64_t bitboard);

	// Pops (clears and returns the index of) the least significant bit (LSB) from a bitboard.
	// Returns 64 if bitboard is 0 before popping. (Optimized to use new get_lsb_index)
	static uint8_t pop_bit(uint64_t& bitboard);

	// Returns a vector of all square indices where bits are set in the bitboard.
	// (Updated to use pop_bit for efficiency)
	static std::vector<int> get_set_bits(uint64_t bitboard);


	// ============================================================================
	// Square and Coordinate Conversion Functions (No changes)
	// ============================================================================

	// Converts a 0-63 square index to its file (0-7, 'a' to 'h').
	static uint8_t square_to_file(int square_idx);
	// Converts a 0-63 square index to its rank (0-7, '1' to '8').
	static uint8_t square_to_rank(int square_idx);
	// Converts a rank (0-7) and file (0-7) to a 0-63 square index.
	static int rank_file_to_square(uint8_t rank, uint8_t file);
	// Converts a square index to algebraic notation (e.g., 0 -> "a1", 63 -> "h8").
	static std::string square_to_string(int square_idx);
	// Converts a Move object to a standard algebraic notation string (e.g., "e2e4", "e7e8q").
	static std::string move_to_string(const Move& move);


	// ============================================================================
	// Attack Detection Functions (Sliding Pieces - No changes to logic, only pawn attacks updated)
	// ============================================================================

	static uint64_t get_rook_attacks(int square, uint64_t occupancy);
	static uint64_t get_bishop_attacks(int square, uint64_t occupancy);
	// Checks if 'target_sq' is attacked by any pawn of 'attacking_color' on 'pawn_attackers_bb'.
	static bool is_pawn_attacked_by(int target_sq, uint64_t pawn_attackers_bb, PlayerColor attacking_color);
	// Checks if 'target_sq' is attacked by any knight on 'knight_attackers_bb'.
	static bool is_knight_attacked_by(int target_sq, uint64_t knight_attackers_bb);
	// Checks if 'target_sq' is attacked by any king on 'king_attackers_bb'.
	static bool is_king_attacked_by(int target_sq, uint64_t king_attackers_bb);
	// Checks if 'target_sq' is attacked by any rook or queen on 'rook_queen_attackers_bb',
	// considering pieces on 'occupied_bb' as blockers.
	static bool is_rook_queen_attacked_by(int target_sq, uint64_t rook_queen_attackers_bb, uint64_t occupied_bb);
	// Checks if 'target_sq' is attacked by any bishop or queen on 'bishop_queen_attackers_bb',
	// considering pieces on 'occupied_bb' as blockers.
	static bool is_bishop_queen_attacked_by(int target_sq, uint64_t bishop_queen_attackers_bb, uint64_t occupied_bb);
};

#endif // CHESS_BITBOARD_UTILS_