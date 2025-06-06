#ifndef CHESS_BITBOARD_UTILS_H
#define CHESS_BITBOARD_UTILS_H

#include <cstdint> // For uint64_t
#include <iostream> // For potential debugging output (e.g., in print_bitboard)

// Include ChessBoard.h if GamePoint or PlayerColor are needed for utility functions.
// For now, these basic bit operations will only use integer square indices.
// If you add conversion functions like square_to_rank/file, GamePoint would be useful.
// #include "ChessBoard.h" // Uncomment if GamePoint or PlayerColor are directly used in functions here.

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

    // --- Basic Bit Manipulation ---

    // Sets a bit at a specific square index (0-63) in a bitboard.
    // Example: set_bit(my_bitboard, 0) sets the A1 bit.
    inline void set_bit(uint64_t& bitboard, int square_idx) {
        bitboard |= (1ULL << square_idx);
    }

    // Clears a bit at a specific square index (0-63) in a bitboard.
    // Example: clear_bit(my_bitboard, 0) clears the A1 bit.
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
    // You might want to define a specific mapping convention (e.g., file-major or rank-major)

    // Converts a square index (0-63) to its file (column) index (0-7, 'a' to 'h').
    inline int square_to_file(int square_idx) {
        return square_idx % 8;
    }

    // Converts a square index (0-63) to its rank (row) index (0-7, 1st rank to 8th rank).
    // Assuming A1=0, H8=63 mapping:
    // Rank 1 (row 7 in visual board) -> 0-7
    // Rank 8 (row 0 in visual board) -> 56-63
    inline int square_to_rank(int square_idx) {
        return square_idx / 8;
    }
    
    // Converts file (0-7) and rank (0-7) to a square index (0-63).
    // This assumes row 0 is rank 8, and row 7 is rank 1.
    // If your internal square mapping (0-63) corresponds to A1=0, H1=7, A8=56, H8=63:
    // rank (0-7 where 0=rank 8, 7=rank 1), file (0-7 where 0='a', 7='h')
    inline int rank_file_to_square(int rank, int file) {
        return rank * 8 + file;
    }


    // --- Debugging / Visualization (Optional) ---
    // Prints a bitboard to console for visual debugging.
    inline void print_bitboard(uint64_t bitboard) {
        for (int rank = 7; rank >= 0; --rank) { // From rank 8 down to 1
            for (int file = 0; file < 8; ++file) { // From file a to h
                int square_idx = rank * 8 + file; // Assuming A1=0, H8=63
                if (test_bit(bitboard, square_idx)) {
                    std::cout << "1 ";
                } else {
                    std::cout << ". ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

} // namespace ChessBitboardUtils

#endif // CHESS_BITBOARD_UTILS_H
