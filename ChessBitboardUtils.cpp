#include "ChessBitboardUtils.h"
#include "Move.h"
#include "MagicTables.h" // DO NOT TOUCH
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cmath>

// ============================================================================
// Static Member Definitions (Bitmasks and Attack Tables)
// ============================================================================

// Rank masks: A bitboard with all bits set for a given rank.
const uint64_t ChessBitboardUtils::RANK_1 = 0x00000000000000FFULL;
const uint64_t ChessBitboardUtils::RANK_2 = 0x000000000000FF00ULL;
const uint64_t ChessBitboardUtils::RANK_3 = 0x0000000000FF0000ULL;
const uint64_t ChessBitboardUtils::RANK_4 = 0x00000000FF000000ULL;
const uint64_t ChessBitboardUtils::RANK_5 = 0x000000FF00000000ULL;
const uint64_t ChessBitboardUtils::RANK_6 = 0x0000FF0000000000ULL;
const uint64_t ChessBitboardUtils::RANK_7 = 0x00FF000000000000ULL;
const uint64_t ChessBitboardUtils::RANK_8 = 0xFF00000000000000ULL;

// File masks: A bitboard with all bits set for a given file.
const uint64_t ChessBitboardUtils::FILE_A = 0x0101010101010101ULL;
const uint64_t ChessBitboardUtils::FILE_B = 0x0202020202020202ULL;
const uint64_t ChessBitboardUtils::FILE_C = 0x0404040404040404ULL;
const uint64_t ChessBitboardUtils::FILE_D = 0x0808080808080808ULL;
const uint64_t ChessBitboardUtils::FILE_E = 0x1010101010101010ULL;
const uint64_t ChessBitboardUtils::FILE_F = 0x2020202020202020ULL;
const uint64_t ChessBitboardUtils::FILE_G = 0x4040404040404040ULL;
const uint64_t ChessBitboardUtils::FILE_H = 0x8080808080808080ULL;

// Individual square bitboards.
const uint64_t ChessBitboardUtils::A1_SQ_BB = 1ULL << 0;
const uint64_t ChessBitboardUtils::H1_SQ_BB = 1ULL << 7;
const uint64_t ChessBitboardUtils::A8_SQ_BB = 1ULL << 56;
const uint64_t ChessBitboardUtils::H8_SQ_BB = 1ULL << 63;
const uint64_t ChessBitboardUtils::E1_SQ_BB = 1ULL << 4;
const uint64_t ChessBitboardUtils::E8_SQ_BB = 1ULL << 60;
const uint64_t ChessBitboardUtils::B1_SQ_BB = 1ULL << 1;
const uint64_t ChessBitboardUtils::G1_SQ_BB = 1ULL << 6;
const uint64_t ChessBitboardUtils::C1_SQ_BB = 1ULL << 2;
const uint64_t ChessBitboardUtils::F1_SQ_BB = 1ULL << 5;
const uint64_t ChessBitboardUtils::B8_SQ_BB = 1ULL << 57;
const uint64_t ChessBitboardUtils::G8_SQ_BB = 1ULL << 62;
const uint64_t ChessBitboardUtils::C8_SQ_BB = 1ULL << 58;
const uint64_t ChessBitboardUtils::F8_SQ_BB = 1ULL << 61;
const uint64_t ChessBitboardUtils::D1_SQ_BB = 1ULL << 3;
const uint64_t ChessBitboardUtils::D8_SQ_BB = 1ULL << 59;

// Individual square indices.
const int ChessBitboardUtils::A1_SQ = 0;
const int ChessBitboardUtils::B1_SQ = 1;
const int ChessBitboardUtils::C1_SQ = 2;
const int ChessBitboardUtils::D1_SQ = 3;
const int ChessBitboardUtils::E1_SQ = 4;
const int ChessBitboardUtils::F1_SQ = 5;
const int ChessBitboardUtils::G1_SQ = 6;
const int ChessBitboardUtils::H1_SQ = 7;

const int ChessBitboardUtils::A8_SQ = 56;
const int ChessBitboardUtils::B8_SQ = 57;
const int ChessBitboardUtils::C8_SQ = 58;
const int ChessBitboardUtils::D8_SQ = 59;
const int ChessBitboardUtils::E8_SQ = 60;
const int ChessBitboardUtils::F8_SQ = 61;
const int ChessBitboardUtils::G8_SQ = 62;
const int ChessBitboardUtils::H8_SQ = 63;

// Castling bitmasks (for castling_rights_mask)
const uint8_t ChessBitboardUtils::CASTLE_WK_BIT = 0b1000; // White Kingside (K)
const uint8_t ChessBitboardUtils::CASTLE_WQ_BIT = 0b0100; // White Queenside (Q)
const uint8_t ChessBitboardUtils::CASTLE_BK_BIT = 0b0010; // Black Kingside (k)
const uint8_t ChessBitboardUtils::CASTLE_BQ_BIT = 0b0001; // Black Queenside (q)

// Attack tables (precomputed for performance).
uint64_t ChessBitboardUtils::knight_attacks[64];
uint64_t ChessBitboardUtils::king_attacks[64];
// Consolidated pawn attacks into a 2D array [PlayerColor][square_idx]
uint64_t ChessBitboardUtils::pawn_attacks[2][64];
bool ChessBitboardUtils::tables_initialized = false; // Kept this for the specific file's existing structure

// ============================================================================
// Initialization of Static Attack Tables
// ============================================================================

// Initializes all static attack tables. This should be called once at program startup.
void ChessBitboardUtils::initialize_attack_tables() {
	if (tables_initialized) return; // Ensure tables are initialized only once

	for (int sq = 0; sq < 64; ++sq) {
		knight_attacks[sq] = generate_knight_attacks(sq);
		king_attacks[sq] = generate_king_attacks(sq);
		// Update to use the 2D pawn_attacks array
		pawn_attacks[static_cast<int>(PlayerColor::White)][sq] = generate_pawn_attacks(sq, PlayerColor::White);
		pawn_attacks[static_cast<int>(PlayerColor::Black)][sq] = generate_pawn_attacks(sq, PlayerColor::Black);
	}
	tables_initialized = true;
}

// Helper to generate knight attack bitmask for a given square.
uint64_t ChessBitboardUtils::generate_knight_attacks(int square_idx) {
	uint64_t attacks = 0ULL;
	// Direct calculation of rank and file from square_idx
	int r = square_idx / 8;
	int f = square_idx % 8;

	// Deltas for knight moves relative to (rank, file)
	int knight_dr[] = {-2, -2, -1, -1, 1, 1, 2, 2};
	int knight_df[] = {-1, 1, -2, 2, -2, 2, -1, 1};

	for (int i = 0; i < 8; ++i) {
		int new_r = r + knight_dr[i];
		int new_f = f + knight_df[i];

		if (new_r >= 0 && new_r < 8 && new_f >= 0 && new_f < 8) {
			attacks |= (1ULL << (new_r * 8 + new_f)); // Direct calculation of new square index
		}
	}
	return attacks;
}

// Helper to generate king attack bitmask for a given square.
uint64_t ChessBitboardUtils::generate_king_attacks(int square_idx) {
	uint64_t attacks = 0ULL;
	uint64_t king_bb = 1ULL << square_idx;

	// These shifts generate all 8 possible moves, including wrapping.
	// The bitwise AND with ~FILE_A, ~FILE_H, etc., masks out illegal wraps.
	attacks |= (king_bb << 1) & ~FILE_A; // Right
	attacks |= (king_bb >> 1) & ~FILE_H; // Left
	attacks |= (king_bb << 8);           // Up
	attacks |= (king_bb >> 8);           // Down
	attacks |= (king_bb << 9) & ~FILE_A; // Up-Right (N.E.)
	attacks |= (king_bb >> 9) & ~FILE_H; // Down-Left (S.W.)
	attacks |= (king_bb << 7) & ~FILE_H; // Up-Left (N.W.)
	attacks |= (king_bb >> 7) & ~FILE_A; // Down-Right (S.E.)
	return attacks;
}

// Helper to generate pawn attack bitmask for a given square and color.
uint64_t ChessBitboardUtils::generate_pawn_attacks(int square_idx, PlayerColor color) {
	uint64_t attacks = 0ULL;
	uint64_t pawn_bb = 1ULL << square_idx;

	if (color == PlayerColor::White) {
		// White pawns attack diagonally forward (N.W. and N.E.)
		attacks |= (pawn_bb << 9) & ~FILE_A; // Up-Right (N.E.)
		attacks |= (pawn_bb << 7) & ~FILE_H; // Up-Left (N.W.)
	} else { // Black
		// Black pawns attack diagonally forward (S.W. and S.E.)
		attacks |= (pawn_bb >> 9) & ~FILE_H; // Down-Left (S.W.)
		attacks |= (pawn_bb >> 7) & ~FILE_A; // Down-Right (S.E.)
	}
	return attacks;
}


// ============================================================================
// Bit Manipulation Functions (Optimized with intrinsics)
// ============================================================================

// Sets a bit at the given index in the bitboard.
void ChessBitboardUtils::set_bit(uint64_t& bitboard, int square_idx) {
	if (square_idx >= 0 && square_idx < 64) {
		bitboard |= (1ULL << square_idx);
	}
}

// Clears a bit at the given index in the bitboard.
void ChessBitboardUtils::clear_bit(uint64_t& bitboard, int square_idx) {
	if (square_idx >= 0 && square_idx < 64) {
		bitboard &= ~(1ULL << square_idx);
	}
}

// Tests if a bit is set at the given index in the bitboard.
bool ChessBitboardUtils::test_bit(uint64_t bitboard, int square_idx) {
	if (square_idx >= 0 && square_idx < 64) {
		return (bitboard & (1ULL << square_idx)) != 0;
	}
	return false;
}

// Gets the index of the least significant bit (LSB) that is set to 1.
// Returns 64 if the bitboard is empty.
// Optimized with compiler intrinsics.
uint8_t ChessBitboardUtils::get_lsb_index(uint64_t bitboard) {
	if (bitboard == 0) {
		return 64; // Indicate no set bit
	}
#if defined(HAS_MSVC_INTRINSICS)
	unsigned long index;
	_BitScanForward64(&index, bitboard);
	return static_cast<uint8_t>(index);
#elif defined(HAS_BUILTIN_CTZLL)
	return static_cast<uint8_t>(__builtin_ctzll(bitboard)); // Count trailing zeros for GCC/Clang
#else
	// Fallback loop version (less efficient)
	uint8_t count = 0;
	while ((bitboard & 1) == 0) {
		bitboard >>= 1;
		count++;
	}
	return count;
#endif
}

// Gets the index of the most significant bit (MSB) that is set to 1.
// Returns 64 if the bitboard is empty.
// New function, optimized with compiler intrinsics.
uint8_t ChessBitboardUtils::get_msb_index(uint64_t bitboard) {
	if (bitboard == 0) {
		return 64; // Indicate no set bit
	}
#if defined(HAS_MSVC_INTRINSICS)
	unsigned long index;
	_BitScanReverse64(&index, bitboard);
	return static_cast<uint8_t>(index);
#elif defined(HAS_BUILTIN_CLZLL)
	// __builtin_clzll counts leading zeros. For a 64-bit unsigned long long,
	// the MSB index is 63 - (number of leading zeros).
	return static_cast<uint8_t>(63 - __builtin_clzll(bitboard));
#else
	// Fallback loop version (less efficient)
	uint8_t count = 63;
	while (!((bitboard >> count) & 1)) { // Check from MSB down
		count--;
	}
	return count;
#endif
}

// Pops (gets and clears) the least significant bit (LSB) from the bitboard.
// Returns the index of the LSB, or 64 if the bitboard was empty.
// Optimized to use intrinsics or bit manipulation tricks.
uint8_t ChessBitboardUtils::pop_bit(uint64_t& bitboard) {
	if (bitboard == 0) {
		return 64; // Indicate no set bit
	}
#if defined(HAS_MSVC_INTRINSICS)
	unsigned long index;
	_BitScanForward64(&index, bitboard);
	bitboard = _blsr_u64(bitboard); // Use _blsr_u64 for efficient LSB clear (resets lowest set bit)
	return static_cast<uint8_t>(index);
#elif defined(HAS_BUILTIN_CTZLL)
	uint8_t lsb_idx = static_cast<uint8_t>(__builtin_ctzll(bitboard));
	bitboard &= (bitboard - 1); // Clear the LSB (standard trick: Kernighan's algorithm)
	return lsb_idx;
#else
	// Fallback loop version (less efficient)
	uint8_t lsb_idx = get_lsb_index(bitboard); // This will use the slower loop fallback if intrinsics aren't available
	clear_bit(bitboard, lsb_idx); // Clear the LSB using the generic clear_bit
	return lsb_idx;
#endif
}


// Counts the number of set bits (population count) in a bitboard.
// Optimized with compiler intrinsics.
uint8_t ChessBitboardUtils::count_set_bits(uint64_t bitboard) {
#if defined(HAS_MSVC_INTRINSICS)
	return static_cast<uint8_t>(__popcnt64(bitboard)); // Intrinsics for MSVC
#elif defined(HAS_BUILTIN_POPCOUNTLL)
	return static_cast<uint8_t>(__builtin_popcountll(bitboard)); // GCC/Clang intrinsic for popcount
#else
	// Fallback loop version (less efficient)
	uint8_t count = 0;
	while (bitboard > 0) {
		bitboard &= (bitboard - 1); // Brian Kernighan's algorithm
		count++;
	}
	return count;
#endif
}

// Returns a vector of square indices where bits are set in the bitboard.
// Updated to use the optimized pop_bit function.
std::vector<int> ChessBitboardUtils::get_set_bits(uint64_t bitboard) {
	std::vector<int> set_bits_indices;
	// Pre-reserve an estimated capacity to reduce reallocations. Max 64 bits.
	set_bits_indices.reserve(count_set_bits(bitboard));

	while (bitboard) {
		set_bits_indices.push_back(pop_bit(bitboard)); // Use pop_bit to get index and clear bit
	}
	return set_bits_indices;
}


// ============================================================================
// Square and Coordinate Conversion Functions (No changes)
// ============================================================================

// Converts 0-63 square index to file (0-7).
uint8_t ChessBitboardUtils::square_to_file(int square_idx) {
	return square_idx % 8;
}

// Converts 0-63 square index to rank (0-7).
uint8_t ChessBitboardUtils::square_to_rank(int square_idx) {
	return square_idx / 8;
}

// Converts rank (0-7) and file (0-7) to 0-63 square index.
int ChessBitboardUtils::rank_file_to_square(uint8_t rank, uint8_t file) {
	return rank * 8 + file;
}

// Converts a square index to algebraic notation (e.g., 0 -> "a1", 63 -> "h8").
std::string ChessBitboardUtils::square_to_string(int square_idx) {
	if (square_idx < 0 || square_idx >= 64) {
		return "Invalid"; // Or throw an error
	}
	char file_char = 'a' + square_to_file(square_idx);
	char rank_char = '1' + square_to_rank(square_idx);
	return std::string(1, file_char) + std::string(1, rank_char);
}

// Converts a Move object to a standard algebraic notation string (e.g., "e2e4", "g1f3", "e7e8q").
std::string ChessBitboardUtils::move_to_string(const Move& move) {
	std::string move_str = "";
	move_str += square_to_string(rank_file_to_square(move.from_square.y, move.from_square.x));
	move_str += square_to_string(rank_file_to_square(move.to_square.y, move.to_square.x));

	if (move.is_promotion) {
		switch (move.promotion_piece_type_idx) {
			case PieceTypeIndex::QUEEN:
				move_str += "q";
				break;
			case PieceTypeIndex::ROOK:
				move_str += "r";
				break;
			case PieceTypeIndex::BISHOP:
				move_str += "b";
				break;
			case PieceTypeIndex::KNIGHT:
				move_str += "n";
				break;
			default:
				break; // Should not happen
		}
	}
	return move_str;
}

// ============================================================================
// Attack Detection Functions (Sliding Pieces)
// ============================================================================
// Checks if a target square is attacked by a pawn of the specified attacking_color.
uint64_t ChessBitboardUtils::get_rook_attacks(int square, uint64_t occupancy) {
	const MagicEntry& m = rook_magics[square];
	return m.attacks[((occupancy & m.mask) * m.magic) >> m.shift];
}

uint64_t ChessBitboardUtils::get_bishop_attacks(int square, uint64_t occupancy) {
	const MagicEntry& m = bishop_magics[square];
	return m.attacks[((occupancy & m.mask) * m.magic) >> m.shift];
}

bool ChessBitboardUtils::is_pawn_attacked_by(int target_sq, uint64_t pawn_attackers_bb, PlayerColor attacking_color) {
	// Generate the pawn attacks *from* the target square (as if a pawn were there)
	// then AND with the actual pawn attackers to see if any intersect.
	// Corrected to use the 2D pawn_attacks array:
	uint64_t attacks_from_target = pawn_attacks[static_cast<int>(attacking_color == PlayerColor::White ? PlayerColor::Black : PlayerColor::White)][target_sq];
	return (attacks_from_target & pawn_attackers_bb) != 0ULL;
}

// Checks if a target square is attacked by a knight.
bool ChessBitboardUtils::is_knight_attacked_by(int target_sq, uint64_t knight_attackers_bb) {
	// Get precomputed knight attacks from the target square, then AND with actual knight attackers.
	return (knight_attacks[target_sq] & knight_attackers_bb) != 0ULL;
}

// Checks if a target square is attacked by a king (i.e., is within a king's movement range).
bool ChessBitboardUtils::is_king_attacked_by(int target_sq, uint64_t king_attackers_bb) {
	// Get precomputed king attacks from the target square, then AND with actual king attackers.
	return (king_attacks[target_sq] & king_attackers_bb) != 0ULL;
}

// Checks if a target square is attacked by a rook or queen.
bool ChessBitboardUtils::is_rook_queen_attacked_by(int target_sq, uint64_t rook_queen_attackers_bb, uint64_t occupied_bb) {
	uint64_t attacks = get_rook_attacks(target_sq, occupied_bb);
	return (attacks & rook_queen_attackers_bb) != 0ULL;
}

// Checks if a target square is attacked by a bishop or queen.
bool ChessBitboardUtils::is_bishop_queen_attacked_by(int target_sq, uint64_t bishop_queen_attackers_bb, uint64_t occupied_bb) {
	uint64_t attacks = get_bishop_attacks(target_sq, occupied_bb);
	return (attacks & bishop_queen_attackers_bb) != 0ULL;
}
