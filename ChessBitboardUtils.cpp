#include "ChessBitboardUtils.h" // Include its own header for declarations
#include "Types.h"              // For PlayerColor, PieceTypeIndex (enum class)
#include <iostream>             // For debugging output
#include <cmath>                // For std::abs in knight/king attack generation

namespace ChessBitboardUtils {

    // --- Static Member Definitions (Attack Tables) ---
    // These are the definitions (memory allocations) for the static arrays declared in the header.
    uint64_t pawn_attacks[2][64];
    uint64_t knight_attacks[64];
    uint64_t king_attacks[64];

    // Static definitions for ray attack tables.
    uint64_t rank_masks[64];      // Horizontal attacks for each square (full rank, ignoring blockers)
    uint64_t file_masks[64];      // Vertical attacks for each square (full file, ignoring blockers)
    uint64_t diag_a1_h8_masks[64]; // Diagonal attacks (A1-H8 type diagonal, ignoring blockers)
    uint64_t diag_h1_a8_masks[64]; // Diagonal attacks (H1-A8 type diagonal, ignoring blockers)

    bool attack_tables_initialized = false; // Initialize the flag to false.

    // Helper function to generate attacks along a single ray.
    // Used by initialize_attack_tables and sliding attack functions.
    // start_sq: The square from which the ray originates.
    // delta: The change in square index for each step along the ray (e.g., 1 for East, 8 for North).
    // exclude_start: If true, the start_sq itself is not included in the attack mask.
    //
    // Example: generate_ray_attacks(E1_SQ, 1) would generate attacks along the 1st rank from E1 to H1.
    // Example: generate_ray_attacks(A1_SQ, 9) would generate attacks along the A1-H8 diagonal.
    uint64_t generate_ray_attacks(int start_sq, int delta, bool exclude_start = true) {
        uint64_t attacks = 0ULL;
        int current_sq = start_sq;

        if (!exclude_start) {
            attacks |= (1ULL << start_sq);
        }

        while (true) {
            int prev_sq = current_sq; // Keep track of previous square to check for wrapping
            current_sq += delta;

            // Check if wrapping around board edges (e.g., A-file to H-file or vice-versa)
            // For horizontal rays (delta = +/-1): check if rank changed.
            if ((std::abs(delta) == 1) && (square_to_rank(current_sq) != square_to_rank(prev_sq))) break;
            // For vertical rays (delta = +/-8): rank changes, file doesn't.
            // Diagonal rays (delta = +/-7, +/-9): check if rank or file is out of bounds for new diagonal.
            // More robust check for diagonals: if (abs(file_diff) != abs(rank_diff)) then it's a wrap.
            // Simpler for fixed 8x8: check if current_sq is valid.

            if (current_sq < 0 || current_sq >= 64) break; // Out of bounds

            // Check for wrapping in a more general way for diagonals.
            // If the move crosses a file/rank boundary that isn't expected for the delta, it's a wrap.
            int prev_r = square_to_rank(prev_sq);
            int prev_f = square_to_file(prev_sq);
            int curr_r = square_to_rank(current_sq);
            int curr_f = square_to_file(current_sq);

            if (std::abs(delta) == 7 || std::abs(delta) == 9) { // Diagonal moves
                if (std::abs(curr_r - prev_r) != 1 || std::abs(curr_f - prev_f) != 1) break;
            }

            attacks |= (1ULL << current_sq);
        }
        return attacks;
    }


    // --- Initialization of Attack Tables ---
    // This function computes and populates all precomputed attack tables.
    // It should be called once at the start of the program.
    void initialize_attack_tables() {
        if (attack_tables_initialized) {
            std::cerr << "WARNING: ChessBitboardUtils::attack_tables already initialized." << std::endl;
            return;
        }

        // --- Initialize Pawn Attacks ---
        // For each square, calculate where a pawn of each color attacks from that square.
        for (int sq = 0; sq < 64; ++sq) {
            uint64_t square_bb = (1ULL << sq); // Bitboard with only the current square set.

            // White pawn attacks (moving "up" the board, capturing diagonally)
            // A white pawn on 'sq' attacks (sq + 7) or (sq + 9).
            uint64_t white_pawn_attack_mask = 0ULL;
            // North-West diagonal: target_sq = sq + 7. Valid if not on A-file (sq % 8 != 0).
            if (sq + 7 < 64 && (sq % 8 != 0)) { 
                white_pawn_attack_mask |= (square_bb << 7); 
            }
            // North-East diagonal: target_sq = sq + 9. Valid if not on H-file (sq % 8 != 7).
            if (sq + 9 < 64 && (sq % 8 != 7)) { 
                white_pawn_attack_mask |= (square_bb << 9); 
            }
            pawn_attacks[static_cast<int>(PlayerColor::White)][sq] = white_pawn_attack_mask;

            // Black pawn attacks (moving "down" the board, capturing diagonally)
            // A black pawn on 'sq' attacks (sq - 7) or (sq - 9).
            uint64_t black_pawn_attack_mask = 0ULL;
            // South-West diagonal: target_sq = sq - 9. Valid if not on A-file (sq % 8 != 0).
            if (sq - 9 >= 0 && (sq % 8 != 0)) { 
                black_pawn_attack_mask |= (square_bb >> 9); 
            }
            // South-East diagonal: target_sq = sq - 7. Valid if not on H-file (sq % 8 != 7).
            if (sq - 7 >= 0 && (sq % 8 != 7)) { 
                black_pawn_attack_mask |= (square_bb >> 7); 
            }
            pawn_attacks[static_cast<int>(PlayerColor::Black)][sq] = black_pawn_attack_mask;
        }

        // --- Initialize Knight Attacks ---
        // Knight moves are a fixed "L" shape.
        // For each square, calculate all 8 possible knight moves.
        int knight_offsets[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
        for (int sq = 0; sq < 64; ++sq) {
            uint64_t attack_mask = 0ULL;
            int r = square_to_rank(sq); // Current rank of the square
            int f = square_to_file(sq); // Current file of the square

            for (int offset : knight_offsets) {
                int target_sq = sq + offset;
                // Check if the potential target square is within board bounds (0-63).
                if (target_sq >= 0 && target_sq < 64) {
                    int target_r = square_to_rank(target_sq); // Rank of the target square
                    int target_f = square_to_file(target_sq); // File of the target square

                    // Additional check to prevent "wrapping around" the board edges.
                    // A knight's move always changes rank by 1 and file by 2, or vice versa.
                    // The absolute difference in files should be at most 2, and ranks at most 2.
                    // The sum of absolute differences should be 3 for an L-shape (1+2 or 2+1).
                    if (std::abs(r - target_r) <= 2 && std::abs(f - target_f) <= 2 &&
                        (std::abs(r - target_r) + std::abs(f - target_f) == 3)) { 
                        attack_mask |= (1ULL << target_sq); // Add this square to the attack mask.
                    }
                }
            }
            knight_attacks[sq] = attack_mask; // Store the computed mask for this square.
        }

        // --- Initialize King Attacks ---
        // King moves one square in any direction (8 directions).
        // For each square, calculate all 8 possible king moves.
        int king_offsets[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
        for (int sq = 0; sq < 64; ++sq) {
            uint64_t attack_mask = 0ULL;
            int r = square_to_rank(sq);
            int f = square_to_file(sq);

            for (int offset : king_offsets) {
                int target_sq = sq + offset;
                // Check if the potential target square is within board bounds (0-63).
                if (target_sq >= 0 && target_sq < 64) {
                    int target_r = square_to_rank(target_sq);
                    int target_f = square_to_file(target_sq);

                    // Prevent wrapping around board edges.
                    // A king's move changes rank by at most 1, and file by at most 1.
                    if (std::abs(r - target_r) <= 1 && std::abs(f - target_f) <= 1) {
                         attack_mask |= (1ULL << target_sq);
                    }
                }
            }
            king_attacks[sq] = attack_mask; // Store the computed mask.
        }

        // --- Initialize Sliding Piece Ray Masks (Unobstructed) ---
        // These are full lines/diagonals without considering any pieces on the board.
        // They will be used later with actual board occupancy to find blocked lines.
        for (int sq = 0; sq < 64; ++sq) {
            // Horizontal (rank) attacks: +/-1 offset
            rank_masks[sq] = generate_ray_attacks(sq, 1, false) | generate_ray_attacks(sq, -1, false);
            // Vertical (file) attacks: +/-8 offset
            file_masks[sq] = generate_ray_attacks(sq, 8, false) | generate_ray_attacks(sq, -8, false);
            // A1-H8 type diagonal attacks: +/-9 offset
            diag_a1_h8_masks[sq] = generate_ray_attacks(sq, 9, false) | generate_ray_attacks(sq, -9, false);
            // H1-A8 type diagonal attacks: +/-7 offset
            diag_h1_a8_masks[sq] = generate_ray_attacks(sq, 7, false) | generate_ray_attacks(sq, -7, false);
        }


        attack_tables_initialized = true; // Set flag to true after successful initialization.
        std::cerr << "INFO: ChessBitboardUtils attack tables initialized." << std::endl; // Debugging output
    }

    // --- Attack Generation Helper Implementations (using precomputed tables and ray tracing) ---
    // These functions check if a target square is attacked by a given bitboard of enemy pieces.

    // Checks if 'target_sq' is attacked by any pawn from 'enemy_pawns_bb' of 'attacking_color'.
    bool is_pawn_attacked_by(int target_sq, uint64_t enemy_pawns_bb, PlayerColor attacking_color) {
        // Calculate the squares from which a pawn of 'attacking_color' could attack 'target_sq'.
        uint64_t pawn_attackers_mask = 0ULL;

        if (attacking_color == PlayerColor::White) { // Checking if a WHITE pawn attacks `target_sq`
            // A white pawn attacks a square from the rank directly below it, diagonally.
            // So, from target_sq, we look for squares at (target_sq - 9) and (target_sq - 7).
            if (target_sq - 9 >= 0 && (square_to_file(target_sq) > 0)) pawn_attackers_mask |= (1ULL << (target_sq - 9)); // South-West diagonal from target
            if (target_sq - 7 >= 0 && (square_to_file(target_sq) < 7)) pawn_attackers_mask |= (1ULL << (target_sq - 7)); // South-East diagonal from target
        } else { // Checking if a BLACK pawn attacks `target_sq`
            // A black pawn attacks a square from the rank directly above it, diagonally.
            // So, from target_sq, we look for squares at (target_sq + 7) and (target_sq + 9).
            if (target_sq + 7 < 64 && (square_to_file(target_sq) > 0)) pawn_attackers_mask |= (1ULL << (target_sq + 7)); // North-West diagonal from target
            if (target_sq + 9 < 64 && (square_to_file(target_sq) < 7)) pawn_attackers_mask |= (1ULL << (target_sq + 9)); // North-East diagonal from target
        }
        
        // If any of the squares that can attack the target square are occupied by an enemy pawn, then it's attacked.
        return (pawn_attackers_mask & enemy_pawns_bb) != 0ULL;
    }


    // Checks if 'target_sq' is attacked by any knight from 'enemy_knights_bb'.
    bool is_knight_attacked_by(int target_sq, uint64_t enemy_knights_bb) {
        // Use the precomputed knight attack mask for the target square.
        // Bitwise AND this mask with the bitboard of enemy knights.
        // If the result is non-zero, it means an enemy knight attacks the target square.
        return (knight_attacks[target_sq] & enemy_knights_bb) != 0ULL;
    }

    // Checks if 'target_sq' is attacked by an enemy king (for proximity check).
    bool is_king_attacked_by(int target_sq, uint64_t enemy_king_bb) {
        return (king_attacks[target_sq] & enemy_king_bb) != 0ULL;
    }

    // Helper to get sliding attacks for a given square and direction, considering blockers.
    // Example: get_sliding_attacks(E1_SQ, 8, occupied_squares) would calculate vertical attacks North from E1.
    uint64_t get_sliding_attacks(int start_sq, int delta, uint64_t all_occupied_bb) {
        uint64_t attacks = 0ULL;
        int current_sq = start_sq;

        while (true) {
            int prev_sq = current_sq;
            current_sq += delta;

            if (current_sq < 0 || current_sq >= 64) break; // Out of bounds

            // Check for wrapping in a more general way for diagonals.
            int prev_r = square_to_rank(prev_sq);
            int prev_f = square_to_file(prev_sq);
            int curr_r = square_to_rank(current_sq);
            int curr_f = square_to_file(current_sq);

            if (std::abs(delta) == 1) { // Horizontal
                if (curr_r != prev_r) break; // Wrapped to another rank
            } else if (std::abs(delta) == 8) { // Vertical
                if (curr_f != prev_f) break; // Wrapped to another file
            } else if (std::abs(delta) == 7 || std::abs(delta) == 9) { // Diagonal
                if (std::abs(curr_r - prev_r) != 1 || std::abs(curr_f - prev_f) != 1) break;
            } else {
                break; // Invalid delta
            }

            attacks |= (1ULL << current_sq); // Add square to attacks

            // If the current square is occupied, it's a blocker, so stop.
            if (test_bit(all_occupied_bb, current_sq)) {
                break;
            }
        }
        return attacks;
    }


    // Checks if 'target_sq' is attacked by any rook or queen from 'rook_queen_attackers_bb'.
    // 'all_occupied_bb' is needed for sliding piece attacks to determine blockers.
    bool is_rook_queen_attacked_by(int target_sq, uint64_t rook_queen_attackers_bb, uint64_t all_occupied_bb) {
        uint64_t attacks_from_target = 0ULL;

        // Check horizontal attacks (East and West)
        attacks_from_target |= get_sliding_attacks(target_sq, 1, all_occupied_bb); // East
        attacks_from_target |= get_sliding_attacks(target_sq, -1, all_occupied_bb); // West

        // Check vertical attacks (North and South)
        attacks_from_target |= get_sliding_attacks(target_sq, 8, all_occupied_bb);  // North
        attacks_from_target |= get_sliding_attacks(target_sq, -8, all_occupied_bb); // South

        // If any of these calculated attacks intersect with the enemy rook/queen bitboard, then it's attacked.
        return (attacks_from_target & rook_queen_attackers_bb) != 0ULL;
    }

    // Checks if 'target_sq' is attacked by any bishop or queen from 'bishop_queen_attackers_bb'.
    // 'all_occupied_bb' is needed for sliding piece attacks to determine blockers.
    bool is_bishop_queen_attacked_by(int target_sq, uint64_t bishop_queen_attackers_bb, uint64_t all_occupied_bb) {
        uint64_t attacks_from_target = 0ULL;

        // Check diagonal attacks (North-East, South-West, North-West, South-East)
        attacks_from_target |= get_sliding_attacks(target_sq, 9, all_occupied_bb);  // North-East (A1-H8 diagonal)
        attacks_from_target |= get_sliding_attacks(target_sq, -9, all_occupied_bb); // South-West (A1-H8 diagonal)
        attacks_from_target |= get_sliding_attacks(target_sq, 7, all_occupied_bb);  // North-West (H1-A8 diagonal)
        attacks_from_target |= get_sliding_attacks(target_sq, -7, all_occupied_bb); // South-East (H1-A8 diagonal)
        
        // If any of these calculated attacks intersect with the enemy bishop/queen bitboard, then it's attacked.
        return (attacks_from_target & bishop_queen_attackers_bb) != 0ULL;
    }

    // --- Debugging / Visualization (Optional) ---
    // Prints a bitboard to console for visual debugging.
    void print_bitboard(uint64_t bitboard) {
        for (int rank = 7; rank >= 0; --rank) { // From rank 8 (internal 7) down to 1 (internal 0)
            for (int file = 0; file < 8; ++file) { // From file 'a' (internal 0) to 'h' (internal 7)
                int square_idx = rank_file_to_square(rank, file); // Get the square index for current (rank, file)
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
