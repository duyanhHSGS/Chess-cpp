#include "ChessBitboardUtils.h"
#include "Move.h" // ADDED: Include the full definition of Move
#include <iostream> 
#include <vector>   
#include <stdexcept> 
#include <cmath> // For std::abs in sliding piece functions

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
uint64_t ChessBitboardUtils::white_pawn_attacks[64];
uint64_t ChessBitboardUtils::black_pawn_attacks[64];
bool ChessBitboardUtils::tables_initialized = false;

// ============================================================================
// Initialization of Static Attack Tables
// ============================================================================

// Initializes all static attack tables. This should be called once at program startup.
void ChessBitboardUtils::initialize_attack_tables() {
    if (tables_initialized) return; // Ensure tables are initialized only once

    for (int sq = 0; sq < 64; ++sq) {
        knight_attacks[sq] = generate_knight_attacks(sq);
        king_attacks[sq] = generate_king_attacks(sq);
        white_pawn_attacks[sq] = generate_pawn_attacks(sq, PlayerColor::White);
        black_pawn_attacks[sq] = generate_pawn_attacks(sq, PlayerColor::Black);
    }
    tables_initialized = true;
}

// Helper to generate knight attack bitmask for a given square.
uint64_t ChessBitboardUtils::generate_knight_attacks(int square_idx) {
    uint64_t attacks = 0ULL;
    // Corrected knight move logic using file/rank differences
    int r = square_to_rank(square_idx);
    int f = square_to_file(square_idx);

    // Deltas for knight moves relative to (rank, file)
    int knight_dr[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    int knight_df[] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; ++i) {
        int new_r = r + knight_dr[i];
        int new_f = f + knight_df[i];

        if (new_r >= 0 && new_r < 8 && new_f >= 0 && new_f < 8) {
            attacks |= (1ULL << rank_file_to_square(new_r, new_f));
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
// Bit Manipulation Functions
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
int ChessBitboardUtils::get_lsb_index(uint64_t bitboard) {
    if (bitboard == 0) {
        return 64; // Indicate no set bit
    }
#ifdef _MSC_VER
    unsigned long index;
    _BitScanForward64(&index, bitboard);
    return index;
#else
    return __builtin_ctzll(bitboard); // Count trailing zeros for GCC/Clang
#endif
}

// Pops (gets and clears) the least significant bit (LSB) from the bitboard.
// Returns the index of the LSB, or 64 if the bitboard was empty.
int ChessBitboardUtils::pop_lsb_index(uint64_t& bitboard) {
    if (bitboard == 0) {
        return 64; // Indicate no set bit
    }
    int lsb_idx = get_lsb_index(bitboard);
    clear_bit(bitboard, lsb_idx); // Clear the LSB
    return lsb_idx;
}

// Counts the number of set bits (population count) in a bitboard.
int ChessBitboardUtils::count_set_bits(uint64_t bitboard) {
#ifdef _MSC_VER
    return __popcnt64(bitboard); // Intrinsics for MSVC
#else
    return __builtin_popcountll(bitboard); // GCC/Clang intrinsic for popcount
#endif
}

// Returns a vector of square indices where bits are set in the bitboard.
std::vector<int> ChessBitboardUtils::get_set_bits(uint64_t bitboard) {
    std::vector<int> set_bits_indices;
    while (bitboard) {
        set_bits_indices.push_back(get_lsb_index(bitboard));
        pop_lsb_index(bitboard); // Use pop_lsb_index to clear the bit
    }
    return set_bits_indices;
}


// ============================================================================
// Square and Coordinate Conversion Functions
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
            case PieceTypeIndex::QUEEN: move_str += "q"; break;
            case PieceTypeIndex::ROOK:  move_str += "r"; break;
            case PieceTypeIndex::BISHOP: move_str += "b"; break;
            case PieceTypeIndex::KNIGHT: move_str += "n"; break;
            default: break; // Should not happen
        }
    }
    return move_str;
}

// ============================================================================
// Attack Detection Functions (Sliding Pieces)
// ============================================================================

// Generates sliding attacks (rook-like: horizontal and vertical) from a given square,
// considering the occupied_squares bitboard for blocking.
uint64_t ChessBitboardUtils::get_sliding_attacks(int start_sq, int delta, uint64_t occupied_bb) {
    uint64_t attacks = 0ULL;
    int current_sq = start_sq;
    uint8_t start_rank = square_to_rank(start_sq);
    uint8_t start_file = square_to_file(start_sq);

    while (true) {
        current_sq += delta;

        // Check bounds (0-63)
        if (current_sq < 0 || current_sq >= 64) {
            break;
        }

        uint8_t current_rank = square_to_rank(current_sq);
        uint8_t current_file = square_to_file(current_sq);

        // Handle wrapping for orthogonal moves (rank/file consistency)
        if (std::abs(delta) == 1 && current_rank != start_rank) { // Horizontal (left/right) moves
            break;
        }
        if (std::abs(delta) == 8 && current_file != start_file) { // Vertical (up/down) moves
            break;
        }
        // Fix for diagonal moves wrapping
        if (std::abs(delta) == 7 || std::abs(delta) == 9) { // It's a diagonal delta
            // Check if the current square is on the same diagonal as the start square
            // by comparing the absolute differences in rank and file from the start.
            // If the absolute rank difference is not equal to the absolute file difference,
            // it means the path has wrapped around the board.
            if (std::abs(static_cast<int>(current_rank) - static_cast<int>(start_rank)) !=
                std::abs(static_cast<int>(current_file) - static_cast<int>(start_file))) {
                break; // Wrapped around
            }
        }

        attacks |= (1ULL << current_sq); // Add square to attacks

        // If the square is occupied, the sliding attack is blocked.
        if (test_bit(occupied_bb, current_sq)) {
            break;
        }
    }
    return attacks;
}

// Checks if a target square is attacked by a pawn of the specified attacking_color.
bool ChessBitboardUtils::is_pawn_attacked_by(int target_sq, uint64_t pawn_attackers_bb, PlayerColor attacking_color) {
    // Generate the pawn attacks *from* the target square (as if a pawn were there)
    // then AND with the actual pawn attackers to see if any intersect.
    uint64_t attacks_from_target = (attacking_color == PlayerColor::White) ? black_pawn_attacks[target_sq] : white_pawn_attacks[target_sq];
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
    // Deltas for horizontal/vertical attacks (rook-like)
    std::array<int, 4> deltas = {-8, 8, -1, 1}; // Up, Down, Left, Right

    uint64_t total_attacks_from_target = 0ULL;
    for (int delta : deltas) {
        total_attacks_from_target |= get_sliding_attacks(target_sq, delta, occupied_bb);
    }
    return (total_attacks_from_target & rook_queen_attackers_bb) != 0ULL;
}

// Checks if a target square is attacked by a bishop or queen.
bool ChessBitboardUtils::is_bishop_queen_attacked_by(int target_sq, uint64_t bishop_queen_attackers_bb, uint64_t occupied_bb) {
    // Deltas for diagonal attacks (bishop-like)
    std::array<int, 4> deltas = {-9, -7, 7, 9}; // NW, NE, SW, SE

    uint64_t total_attacks_from_target = 0ULL;
    for (int delta : deltas) {
        total_attacks_from_target |= get_sliding_attacks(target_sq, delta, occupied_bb);
    }
    return (total_attacks_from_target & bishop_queen_attackers_bb) != 0ULL;
}
