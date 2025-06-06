#ifndef CHESS_BITBOARD_UTILS_H
#define CHESS_BITBOARD_H

#include <cstdint> // For uint64_t
#include <iostream> // For potential debugging output (e.g., in print_bitboard)
#include "Types.h"  // For PlayerColor, GamePoint, PieceTypeIndex (enum class)

// Compiler intrinsics for bit scanning and population count
// MSVC
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(__popcnt64)
#endif

// GCC/Clang
#if defined(__GNUC__) || defined(__clang__)
// __builtin_ctzll (count trailing zeros long long)
// __builtin_popcountll (population count long long)
#endif


namespace ChessBitboardUtils {

    // --- Chessboard Square Definitions (0-63) ---
    // Standard mapping: A1=0, B1=1, ..., H1=7, A2=8, ..., H8=63
    // This provides integer indices for squares.
    // Rank 1 (row 0):
    const int A1_SQ = 0; const int B1_SQ = 1; const int C1_SQ = 2; const int D1_SQ = 3;
    const int E1_SQ = 4; const int F1_SQ = 5; const int G1_SQ = 6; const int H1_SQ = 7;
    // Rank 2 (row 1):
    const int A2_SQ = 8; const int B2_SQ = 9; const int C2_SQ = 10; const int D2_SQ = 11;
    const int E2_SQ = 12; const int F2_SQ = 13; const int G2_SQ = 14; const int H2_SQ = 15;
    // Rank 3 (row 2):
    const int A3_SQ = 16; const int B3_SQ = 17; const int C3_SQ = 18; const int D3_SQ = 19;
    const int E3_SQ = 20; const int F3_SQ = 21; const int G3_SQ = 22; const int H3_SQ = 23;
    // Rank 4 (row 3):
    const int A4_SQ = 24; const int B4_SQ = 25; const int C4_SQ = 26; const int D4_SQ = 27;
    const int E4_SQ = 28; const int F4_SQ = 29; const int G4_SQ = 30; const int H4_SQ = 31;
    // Rank 5 (row 4):
    const int A5_SQ = 32; const int B5_SQ = 33; const int C5_SQ = 34; const int D5_SQ = 35;
    const int E5_SQ = 36; const int F5_SQ = 37; const int G5_SQ = 38; const int H5_SQ = 39;
    // Rank 6 (row 5):
    const int A6_SQ = 40; const int B6_SQ = 41; const int C6_SQ = 42; const int D6_SQ = 43;
    const int E6_SQ = 44; const int F6_SQ = 45; const int G6_SQ = 46; const int H6_SQ = 47;
    // Rank 7 (row 6):
    const int A7_SQ = 48; const int B7_SQ = 49; const int C7_SQ = 50; const int D7_SQ = 51;
    const int E7_SQ = 52; const int F7_SQ = 53; const int G7_SQ = 54; const int H7_SQ = 55;
    // Rank 8 (row 7):
    const int A8_SQ = 56; const int B8_SQ = 57; const int C8_SQ = 58; const int D8_SQ = 59;
    const int E8_SQ = 60; const int F8_SQ = 61; const int G8_SQ = 62; const int H8_SQ = 63;

    // --- Chessboard Square Bitboard Masks (1ULL << square_idx) ---
    // These are often useful directly when setting up positions or checking specific squares.
    const uint64_t A1_SQ_BB = 1ULL << A1_SQ; const uint64_t B1_SQ_BB = 1ULL << B1_SQ;
    const uint64_t C1_SQ_BB = 1ULL << C1_SQ; const uint64_t D1_SQ_BB = 1ULL << D1_SQ;
    const uint64_t E1_SQ_BB = 1ULL << E1_SQ; const uint64_t F1_SQ_BB = 1ULL << F1_SQ;
    const uint64_t G1_SQ_BB = 1ULL << G1_SQ; const uint64_t H1_SQ_BB = 1ULL << H1_SQ;
    
    const uint64_t A8_SQ_BB = 1ULL << A8_SQ; const uint64_t B8_SQ_BB = 1ULL << B8_SQ;
    const uint64_t C8_SQ_BB = 1ULL << C8_SQ; const uint64_t D8_SQ_BB = 1ULL << D8_SQ;
    const uint64_t E8_SQ_BB = 1ULL << E8_SQ; const uint64_t F8_SQ_BB = 1ULL << F8_SQ;
    const uint64_t G8_SQ_BB = 1ULL << G8_SQ; const uint64_t H8_SQ_BB = 1ULL << H8_SQ;


    // --- Rank Bitmasks ---
    const uint64_t RANK_1 = 0xFFULL;          // 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 1111 1111 (A1-H1)
    const uint64_t RANK_2 = 0xFF00ULL;        // (A2-H2)
    const uint64_t RANK_3 = 0xFF0000ULL;
    const uint64_t RANK_4 = 0xFF000000ULL;
    const uint64_t RANK_5 = 0xFF00000000ULL;
    const uint64_t RANK_6 = 0xFF0000000000ULL;
    const uint64_t RANK_7 = 0xFF000000000000ULL;
    const uint64_t RANK_8 = 0xFF00000000000000ULL; // 1111 1111 0000 0000 ... 0000 0000 0000 0000 (A8-H8)

    // --- File Bitmasks ---
    const uint64_t FILE_A = 0x0101010101010101ULL; // A1, A2, ..., A8
    const uint64_t FILE_B = 0x0202020202020202ULL;
    const uint64_t FILE_C = 0x0404040404040404ULL;
    const uint64_t FILE_D = 0x0808080808080808ULL;
    const uint64_t FILE_E = 0x1010101010101010ULL;
    const uint64_t FILE_F = 0x2020202020202020ULL;
    const uint64_t FILE_G = 0x4040404040404040ULL;
    const uint64_t FILE_H = 0x8080808080808080ULL; // H1, H2, ..., H8

    // --- Castling Rights Bitmask Definitions ---
    // These bits are used in the ChessBoard::castling_rights_mask
    const uint8_t CASTLE_BQ_BIT = 0b0001; // Black Queenside (q) - Bit 0
    const uint8_t CASTLE_BK_BIT = 0b0010; // Black Kingside (k) - Bit 1
    const uint8_t CASTLE_WQ_BIT = 0b0100; // White Queenside (Q) - Bit 2
    const uint8_t CASTLE_WK_BIT = 0b1000; // White Kingside (K) - Bit 3

    // --- Precomputed Attack Tables (Declarations) ---
    // These are static arrays that will hold precomputed attack masks for each square.
    // They are declared here, but defined (allocated and initialized) in ChessBitboardUtils.cpp.
    extern uint64_t pawn_attacks[2][64];  // 0 for White pawns, 1 for Black pawns
    extern uint64_t knight_attacks[64];
    extern uint64_t king_attacks[64];

    // Precomputed "ray" masks for sliding pieces (Rook/Bishop/Queen).
    // These masks represent all squares along a given ray (horizontal, vertical, diagonal)
    // from a source square *without considering any blockers*.
    extern uint64_t rank_masks[64];      // Horizontal attacks for each square (full rank)
    extern uint64_t file_masks[64];      // Vertical attacks for each square (full file)
    extern uint64_t diag_a1_h8_masks[64]; // Diagonal attacks (A1-H8 type diagonal) for each square
    extern uint64_t diag_h1_a8_masks[64]; // Diagonal attacks (H1-A8 type diagonal) for each square


    // Flag to ensure attack tables are initialized only once.
    extern bool attack_tables_initialized;

    // Declares the function to initialize all precomputed attack tables.
    // Its definition is in ChessBitboardUtils.cpp.
    void initialize_attack_tables();


    // --- Basic Bit Manipulation ---

    // Sets a bit at a specific square index (0-63) in a bitboard.
    inline void set_bit(uint64_t& bitboard, int square_idx) {
        bitboard |= (1ULL << square_idx);
    }

    // Clears a bit at a specific square index (0-63) in a bitboard.
    inline void clear_bit(uint64_t& bitboard, int square_idx) {
        bitboard &= ~(1ULL << square_idx);
    }

    // Tests if a bit is set at a specific square index (0-63) in a bitboard.
    // Returns true if the bit is set, false otherwise.
    inline bool test_bit(uint64_t bitboard, int square_idx) {
        return (bitboard & (1ULL << square_idx)) != 0;
    }

    // --- Bit Scanning (Finding set bits) ---

    // Returns the index (0-63) of the least significant bit (LSB) that is set in the bitboard.
    // Assumes bitboard is not zero. Behavior is undefined for 0.
    // Uses compiler intrinsics for high performance.
    inline int get_lsb_index(uint64_t bitboard) {
        #ifdef _MSC_VER
            unsigned long index;
            _BitScanForward64(&index, bitboard);
            return static_cast<int>(index);
        #elif defined(__GNUC__) || defined(__clang__)
            return __builtin_ctzll(bitboard);
        #else
            // Fallback for other compilers (less efficient)
            for (int i = 0; i < 64; ++i) {
                if (test_bit(bitboard, i)) {
                    return i;
                }
            }
            return -1; // Should not reach here if bitboard is non-zero
        #endif
    }

    // Returns the index (0-63) of the least significant bit (LSB) that is set in the bitboard,
    // and then clears that bit from the bitboard. This is useful for iterating through set bits.
    // Assumes bitboard is not zero. Behavior is undefined for 0.
    inline int pop_lsb_index(uint64_t& bitboard) {
        int lsb_idx = get_lsb_index(bitboard);
        clear_bit(bitboard, lsb_idx); // Clear the identified LSB
        return lsb_idx;
    }

    // --- Population Count (Counting set bits) ---

    // Returns the number of set bits (1s) in a bitboard.
    // Uses compiler intrinsics for high performance.
    inline int count_set_bits(uint64_t bitboard) {
        #ifdef _MSC_VER
            return static_cast<int>(__popcnt64(bitboard));
        #elif defined(__GNUC__) || defined(__clang__)
            return __builtin_popcountll(bitboard);
        #else
            // Fallback for other compilers (less efficient)
            int count = 0;
            while (bitboard > 0) {
                bitboard &= (bitboard - 1); // Brian Kernighan's algorithm
                count++;
            }
            return count;
        #endif
    }

    // --- Coordinate Conversion Helpers (assuming A1=0, H1=7, A8=56, H8=63) ---
    // Rank 0 is the 1st rank (bottom), Rank 7 is the 8th rank (top)
    // File 0 is 'a', File 7 is 'h'

    // Converts a square index (0-63) to its file (column) index (0-7, 'a' to 'h').
    inline int square_to_file(int square_idx) {
        return square_idx % 8;
    }

    // Converts a square index (0-63) to its rank (row) index (0-7).
    inline int square_to_rank(int square_idx) {
        return square_idx / 8;
    }
    
    // Converts file (0-7) and rank (0-7) to a square index (0-63).
    // This assumes internal rank 0 is FEN rank 1, internal rank 7 is FEN rank 8.
    inline int rank_file_to_square(int rank, int file) {
        return rank * 8 + file;
    }

    // --- Attack Generation Helpers (Using Precomputed Tables for Fixed-Movement Pieces) ---
    // These functions check if a target square is attacked by a given bitboard of enemy pieces.

    // Checks if 'target_sq' is attacked by any pawn from 'enemy_pawns_bb' of 'attacking_color'.
    bool is_pawn_attacked_by(int target_sq, uint64_t enemy_pawns_bb, PlayerColor attacking_color);

    // Checks if 'target_sq' is attacked by any knight from 'enemy_knights_bb'.
    bool is_knight_attacked_by(int target_sq, uint64_t enemy_knights_bb);

    // Checks if 'target_sq' is attacked by an enemy king (for proximity check).
    bool is_king_attacked_by(int target_sq, uint64_t enemy_king_bb);

    // Checks if 'target_sq' is attacked by any rook or queen from 'rook_queen_attackers_bb'.
    // 'all_occupied_bb' is needed for sliding piece attacks to determine blockers.
    bool is_rook_queen_attacked_by(int target_sq, uint64_t rook_queen_attackers_bb, uint64_t all_occupied_bb);

    // Checks if 'target_sq' is attacked by any bishop or queen from 'bishop_queen_attackers_bb'.
    // 'all_occupied_bb' is needed for sliding piece attacks to determine blockers.
    bool is_bishop_queen_attacked_by(int target_sq, uint64_t bishop_queen_attackers_bb, uint64_t all_occupied_bb);

    // --- Debugging / Visualization (Optional) ---
    // Prints a bitboard to console for visual debugging.
    void print_bitboard(uint64_t bitboard); // Declaration only, definition in .cpp

} // namespace ChessBitboardUtils

#endif // CHESS_BITBOARD_UTILS_H
