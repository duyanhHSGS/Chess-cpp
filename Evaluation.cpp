#include "Evaluation.h"
#include "Constants.h" // To get all evaluation constants
#include "ChessAI.h"   // To get PSTs from ChessAI

namespace Evaluation {

	/**
	 * @brief Calculates the pawn shield penalty for a given king's color.
	 *
	 * This function assesses the strength of the pawn "shield" in front of the king.
	 * A good pawn shield typically consists of three pawns on the 2nd (for White) or 7th (for Black) ranks,
	 * directly in front of the castled king. Missing pawns or advanced pawns in this zone
	 * can expose the king to attacks, leading to penalties.
	 *
	 * The function specifically checks for:
	 * - Missing pawns: If a pawn that should be part of the ideal shield is not present.
	 * - Advanced pawns: If a pawn that was part of the shield has moved forward,
	 * potentially creating holes or weakening the king's direct defense.
	 *
	 * The penalties are applied based on predefined constants from `Constants.h`.
	 *
	 * @param board_ref The current ChessBoard state.
	 * @param king_color The color of the king for which to calculate the shield (PlayerColor::White or PlayerColor::Black).
	 * @param king_square The square index (0-63) of the king.
	 * @param friendly_pawns_bb The bitboard of all friendly pawns (used to check for missing/advanced shield pawns).
	 * @return The calculated pawn shield penalty score. A negative score indicates a penalty.
	 */
	int calculate_pawn_shield_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square,
	        uint64_t friendly_pawns_bb) {
		int penalty = 0;

		// Define ideal king safety zones for both kingside and queenside castling for White and Black.
		// These are broad zones that help determine if the king is on the kingside or queenside.
		const uint64_t WHITE_KINGSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::F1_SQ) | (1ULL << ChessBitboardUtils::G1_SQ) | (1ULL << ChessBitboardUtils::H1_SQ);
		const uint64_t WHITE_QUEENSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::A1_SQ) | (1ULL << ChessBitboardUtils::B1_SQ) | (1ULL << ChessBitboardUtils::C1_SQ);
		const uint64_t BLACK_KINGSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::F8_SQ) | (1ULL << ChessBitboardUtils::G8_SQ) | (1ULL << ChessBitboardUtils::H8_SQ);
		const uint64_t BLACK_QUEENSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::A8_SQ) | (1ULL << ChessBitboardUtils::B8_SQ) | (1ULL << ChessBitboardUtils::C8_SQ);

		if (king_color == PlayerColor::White) {
			// Check if White king is on the kingside (typical castling position)
			if ((board_ref.white_king & WHITE_KINGSIDE_KING_ZONE) != 0ULL) {
				// Define the ideal pawn shield squares for White kingside castling (f2, g2, h2 pawns)
				uint64_t white_kingside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(1, 5)) | // f2
				                                      (1ULL << ChessBitboardUtils::rank_file_to_square(1, 6)) | // g2
				                                      (1ULL << ChessBitboardUtils::rank_file_to_square(1, 7)); // h2
				// Identify missing pawns in the ideal shield
				uint64_t missing_white_kingside_pawns = white_kingside_shield_mask & (~friendly_pawns_bb);
				// Apply penalty based on the number of missing pawns
				penalty -= ChessBitboardUtils::count_set_bits(missing_white_kingside_pawns) * PAWN_SHIELD_MISSING_PAWN_PENALTY;

				// Define squares one rank advanced from the ideal kingside shield (f3, g3, h3)
				uint64_t white_kingside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(2, 5)) | // f3
				                                        (1ULL << ChessBitboardUtils::rank_file_to_square(2, 6)) | // g3
				                                        (1ULL << ChessBitboardUtils::rank_file_to_square(2, 7)); // h3
				// Identify friendly pawns that have advanced into this zone
				uint64_t advanced_white_kingside_pawns = friendly_pawns_bb & white_kingside_advanced_mask;
				// Apply penalty based on the number of advanced pawns
				penalty -= ChessBitboardUtils::count_set_bits(advanced_white_kingside_pawns) * PAWN_SHIELD_ADVANCED_PAWN_PENALTY;
			}
			// Check if White king is on the queenside (typical castling position)
			if ((board_ref.white_king & WHITE_QUEENSIDE_KING_ZONE) != 0ULL) {
				// Define the ideal pawn shield squares for White queenside castling (a2, b2, c2 pawns)
				uint64_t white_queenside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(1, 0)) | // a2
				                                       (1ULL << ChessBitboardUtils::rank_file_to_square(1, 1)) | // b2
				                                       (1ULL << ChessBitboardUtils::rank_file_to_square(1, 2)); // c2
				// Identify missing pawns
				uint64_t missing_white_queenside_pawns = white_queenside_shield_mask & (~friendly_pawns_bb);
				penalty -= ChessBitboardUtils::count_set_bits(missing_white_queenside_pawns) * PAWN_SHIELD_MISSING_PAWN_PENALTY;

				// Define squares one rank advanced from the ideal queenside shield (a3, b3, c3)
				uint64_t white_queenside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(2, 0)) | // a3
				        (1ULL << ChessBitboardUtils::rank_file_to_square(2, 1)) | // b3
				        (1ULL << ChessBitboardUtils::rank_file_to_square(2, 2)); // c3
				// Identify advanced pawns
				uint64_t advanced_white_queenside_pawns = friendly_pawns_bb & white_queenside_advanced_mask;
				penalty -= ChessBitboardUtils::count_set_bits(advanced_white_queenside_pawns) * PAWN_SHIELD_ADVANCED_PAWN_PENALTY;
			}
		} else { // King color is Black
			// Check if Black king is on the kingside (typical castling position)
			if ((board_ref.black_king & BLACK_KINGSIDE_KING_ZONE) != 0ULL) {
				// Define the ideal pawn shield squares for Black kingside castling (f7, g7, h7 pawns)
				uint64_t black_kingside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(6, 5)) | // f7
				                                      (1ULL << ChessBitboardUtils::rank_file_to_square(6, 6)) | // g7
				                                      (1ULL << ChessBitboardUtils::rank_file_to_square(6, 7)); // h7
				// Identify missing pawns
				uint64_t missing_black_kingside_pawns = black_kingside_shield_mask & (~friendly_pawns_bb);
				penalty -= ChessBitboardUtils::count_set_bits(missing_black_kingside_pawns) * PAWN_SHIELD_MISSING_PAWN_PENALTY;

				// Define squares one rank advanced from the ideal kingside shield (f6, g6, h6)
				uint64_t black_kingside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(5, 5)) | // f6
				                                        (1ULL << ChessBitboardUtils::rank_file_to_square(5, 6)) | // g6
				                                        (1ULL << ChessBitboardUtils::rank_file_to_square(5, 7)); // h6
				// Identify advanced pawns
				uint64_t advanced_black_kingside_pawns = friendly_pawns_bb & black_kingside_advanced_mask;
				penalty -= ChessBitboardUtils::count_set_bits(advanced_black_kingside_pawns) * PAWN_SHIELD_ADVANCED_PAWN_PENALTY;
			}
			// Check if Black king is on the queenside (typical castling position)
			if ((board_ref.black_king & BLACK_QUEENSIDE_KING_ZONE) != 0ULL) {
				// Define the ideal pawn shield squares for Black queenside castling (a7, b7, c7 pawns)
				uint64_t black_queenside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(6, 0)) | // a7
				                                       (1ULL << ChessBitboardUtils::rank_file_to_square(6, 1)) | // b7
				                                       (1ULL << ChessBitboardUtils::rank_file_to_square(6, 2)); // c7
				// Identify missing pawns
				uint64_t missing_black_queenside_pawns = black_queenside_shield_mask & (~friendly_pawns_bb);
				penalty -= ChessBitboardUtils::count_set_bits(missing_black_queenside_pawns) * PAWN_SHIELD_MISSING_PAWN_PENALTY;

				// Define squares one rank advanced from the ideal queenside shield (a6, b6, c6)
				uint64_t black_queenside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(5, 0)) | // a6
				        (1ULL << ChessBitboardUtils::rank_file_to_square(5, 1)) | // b6
				        (1ULL << ChessBitboardUtils::rank_file_to_square(5, 2)); // c6
				// Identify advanced pawns
				uint64_t advanced_black_queenside_pawns = friendly_pawns_bb & black_queenside_advanced_mask;
				penalty -= ChessBitboardUtils::count_set_bits(advanced_black_queenside_pawns) * PAWN_SHIELD_ADVANCED_PAWN_PENALTY;
			}
		}
		return penalty;
	}

	/**
	 * @brief Calculates the penalty for open or semi-open files near the king.
	 *
	 * Open files (no pawns on them) and semi-open files (only friendly pawns, no enemy pawns)
	 * can be dangerous for the king, especially if there are enemy rooks or queens.
	 * This function assesses files adjacent to the king's file to apply penalties.
	 *
	 * @param board_ref The current ChessBoard state.
	 * @param king_color The color of the king for which to calculate the penalty.
	 * @param king_square The square index (0-63) of the king.
	 * @param friendly_pawns_bb The bitboard of all friendly pawns.
	 * @param enemy_pawns_bb The bitboard of all enemy pawns.
	 * @return The calculated open file penalty score. A negative score indicates a penalty.
	 */
	int calculate_open_file_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square,
	        uint64_t friendly_pawns_bb, uint64_t enemy_pawns_bb) {
		int penalty = 0;
		int king_file = ChessBitboardUtils::square_to_file(king_square); // Get the file of the king

		// Iterate over the king's file and the two adjacent files (-1, 0, +1 offset from king's file)
		for (int f_offset = -1; f_offset <= 1; ++f_offset) {
			int current_file = king_file + f_offset;
			// Ensure the current file index is within the valid range (0-7)
			if (current_file >= 0 && current_file < 8) {
				// Use FILE_MASKS_ARRAY from Constants.h, accessed directly without namespace
				uint64_t file_mask = FILE_MASKS_ARRAY[current_file];
				uint64_t file_contents = friendly_pawns_bb | enemy_pawns_bb; // Combine all pawns on the board

				// Check if the file is completely open (no friendly or enemy pawns)
				if (!((file_contents & file_mask) != 0ULL)) {
					penalty -= OPEN_FILE_FULL_OPEN_PENALTY;
				}
				// Check if the file is semi-open (contains friendly pawns but no enemy pawns)
				else if (!((friendly_pawns_bb & file_mask) != 0ULL)) {
					penalty -= OPEN_FILE_SEMI_OPEN_PENALTY;
				}
			}
		}
		return penalty;
	}


	/**
	 * @brief Evaluates the current state of the chessboard for the active player.
	 *
	 * This function calculates a static evaluation score for the given board position.
	 * A positive score indicates an advantage for White, while a negative score indicates
	 * an advantage for Black. The evaluation is broken down into several phases:
	 *
	 * 1.  **Material and Piece-Square Table (PST) Scores:**
	 * - Sums the value of all pieces on the board.
	 * - Adds/substracts scores based on the position of each piece (PSTs),
	 * which represent ideal or detrimental squares for specific piece types.
	 *
	 * 2.  **Pawn Structure:**
	 * - Penalties for isolated pawns (no friendly pawns on adjacent files).
	 * - Penalties for doubled pawns (multiple friendly pawns on the same file).
	 * - Bonuses for passed pawns (no enemy pawns in front or on adjacent files),
	 * with an additional bonus for how far they have advanced.
	 * - Bonuses for connected pawns (friendly pawns diagonally supporting each other).
	 *
	 * 3.  **King Safety:**
	 * - Penalties related to the "pawn shield" in front of the king (missing or advanced pawns).
	 * - Bonuses for castling, encouraging king safety in the opening.
	 * - Penalties for open or semi-open files near the king, as these can be avenues for attack.
	 *
	 * 4.  **Piece Mobility:**
	 * - A bonus for each square a piece can pseudo-legally move to (attacks/pushes).
	 * This encourages active pieces that control more of the board.
	 * Pawns consider both captures and pushes. Sliding pieces use magic bitboards
	 * for efficient attack generation given current occupancy.
	 *
	 * All adjustable numeric constants for evaluation are now managed in `Constants.h`.
	 *
	 * @param board The ChessBoard object representing the current game state.
	 * @return An integer representing the static evaluation score of the board.
	 */
	int evaluate(const ChessBoard& board) {
		int score = 0;

		// Phase 1: Material and Piece-Square Table (PST) scores
		// Iterates through all 64 squares of the board.
		for (int i = 0; i < 64; ++i) {
			// Check for White pieces and add their material value + PST value for their square.
			// PSTs are mirrored for Black (63 - i) to reflect their perspective.
			if (ChessBitboardUtils::test_bit(board.white_pawns, i))    score += (PAWN_VALUE + ChessAI::PAWN_PST[i]);
			else if (ChessBitboardUtils::test_bit(board.white_knights, i)) score += (KNIGHT_VALUE + ChessAI::KNIGHT_PST[i]);
			else if (ChessBitboardUtils::test_bit(board.white_bishops, i)) score += (BISHOP_VALUE + ChessAI::BISHOP_PST[i]);
			else if (ChessBitboardUtils::test_bit(board.white_rooks, i))  score += (ROOK_VALUE + ChessAI::ROOK_PST[i]);
			else if (ChessBitboardUtils::test_bit(board.white_queens, i)) score += (QUEEN_VALUE + ChessAI::QUEEN_PST[i]);
			else if (ChessBitboardUtils::test_bit(board.white_king, i)) score += (KING_VALUE + ChessAI::KING_PST[i]);
			// Check for Black pieces and subtract their material value + PST value (mirrored).
			else if (ChessBitboardUtils::test_bit(board.black_pawns, i))   score -= (PAWN_VALUE + ChessAI::PAWN_PST[63 - i]);
			else if (ChessBitboardUtils::test_bit(board.black_knights, i)) score -= (KNIGHT_VALUE + ChessAI::KNIGHT_PST[63 - i]);
			else if (ChessBitboardUtils::test_bit(board.black_bishops, i)) score -= (BISHOP_VALUE + ChessAI::BISHOP_PST[63 - i]);
			else if (ChessBitboardUtils::test_bit(board.black_rooks, i))  score -= (ROOK_VALUE + ChessAI::ROOK_PST[63 - i]);
			else if (ChessBitboardUtils::test_bit(board.black_queens, i)) score -= (QUEEN_VALUE + ChessAI::QUEEN_PST[63 - i]);
			else if (ChessBitboardUtils::test_bit(board.black_king, i)) score -= (KING_VALUE + ChessAI::KING_PST[63 - i]);
		}

		// Phase 2: Pawn Structure Evaluation (Isolated, Doubled, Passed, and Connected Pawns)
		int white_pawn_structure_score = 0;
		int black_pawn_structure_score = 0;

		// Bitboards to track files that have already received a doubled pawn penalty,
		// preventing multiple penalties for pawns on the same file if there are more than two.
		uint64_t white_doubled_files_penalized = 0ULL;
		uint64_t black_doubled_files_penalized = 0ULL;

		// Iterate through White's pawns to evaluate their structure.
		uint64_t current_white_pawns_bb = board.white_pawns;
		while (current_white_pawns_bb) {
			int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_white_pawns_bb); // Get LSB and clear it
			int file = ChessBitboardUtils::square_to_file(pawn_sq_idx); // Get file of the pawn
			int rank = ChessBitboardUtils::square_to_rank(pawn_sq_idx); // Get rank of the pawn

			// Check for Isolated Pawns: A pawn is isolated if there are no friendly pawns
			// on the adjacent files (to its left or right).
			uint64_t adjacent_files_mask = 0ULL;
			if (file > 0) {
				adjacent_files_mask |= FILE_MASKS_ARRAY[file - 1]; // Mask for file to the left
			}
			if (file < 7) {
				adjacent_files_mask |= FILE_MASKS_ARRAY[file + 1]; // Mask for file to the right
			}
			if ((board.white_pawns & adjacent_files_mask) == 0ULL) {
				white_pawn_structure_score -= ISOLATED_PAWN_PENALTY; // Apply penalty
			}

			// Check for Doubled Pawns: A pawn is doubled if there's another friendly pawn
			// on the same file. Apply penalty once per file.
			uint64_t current_file_mask = FILE_MASKS_ARRAY[file];
			// Check if this file has already been penalized for doubled pawns.
			if (!ChessBitboardUtils::test_bit(white_doubled_files_penalized, file)) {
				// If more than one white pawn exists on this file, it's a doubled pawn.
				if (ChessBitboardUtils::count_set_bits(board.white_pawns & current_file_mask) > 1) {
					white_pawn_structure_score -= DOUBLED_PAWN_PENALTY; // Apply penalty
					white_doubled_files_penalized |= (1ULL << file); // Mark this file as penalized
				}
			}

			// Check for Passed Pawns: A pawn is passed if there are no enemy pawns
			// directly in front of it, or on adjacent files in front of it.
			uint64_t white_passed_pawn_mask = 0ULL;
			// Iterate through ranks ahead of the current pawn's rank
			for (int r = rank + 1; r < 8; ++r) {
				white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file)); // Squares directly ahead
				if (file > 0) {
					white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file - 1)); // Squares diagonally left
				}
				if (file < 7) {
					white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file + 1)); // Squares diagonally right
				}
			}
			// If no black pawns are found in the mask, it's a passed pawn.
			if ((board.black_pawns & white_passed_pawn_mask) == 0ULL) {
				int passed_pawn_bonus = PASSED_PAWN_BASE_BONUS;
				// Add bonus based on rank (pawns closer to promotion are more valuable)
				passed_pawn_bonus += (rank - 1) * PASSED_PAWN_RANK_BONUS_FACTOR; // rank 1 = 0 bonus, rank 6 = 50 bonus
				white_pawn_structure_score += passed_pawn_bonus; // Apply bonus
			}

			// Check for Connected Pawns: Pawns on adjacent files that are diagonally connected.
			uint64_t squares_attacking_this_pawn_mask = 0ULL;
			if (file > 0 && rank > 0) { // Check backward-left diagonal (for white, this is support)
				squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank - 1, file - 1));
			}
			if (file < 7 && rank > 0) { // Check backward-right diagonal
				squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank - 1, file + 1));
			}
			// If this pawn is supported by another white pawn.
			if ((board.white_pawns & squares_attacking_this_pawn_mask) != 0ULL) {
				white_pawn_structure_score += CONNECTED_PAWN_BONUS; // Apply bonus
			}

			// Clear the LSB to process the next pawn in the bitboard.
			current_white_pawns_bb &= (current_white_pawns_bb - 1);
		}

		// Iterate through Black's pawns to evaluate their structure (symmetric to White).
		uint64_t current_black_pawns_bb = board.black_pawns;
		while (current_black_pawns_bb) {
			int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_black_pawns_bb);
			int file = ChessBitboardUtils::square_to_file(pawn_sq_idx);
			int rank = ChessBitboardUtils::square_to_rank(pawn_sq_idx);

			// Isolated Pawns for Black.
			uint64_t adjacent_files_mask = 0ULL;
			if (file > 0) {
				adjacent_files_mask |= FILE_MASKS_ARRAY[file - 1];
			}
			if (file < 7) {
				adjacent_files_mask |= FILE_MASKS_ARRAY[file + 1];
			}
			if ((board.black_pawns & adjacent_files_mask) == 0ULL) {
				black_pawn_structure_score -= ISOLATED_PAWN_PENALTY;
			}

			// Doubled Pawns for Black.
			uint64_t current_file_mask = FILE_MASKS_ARRAY[file];
			if (!ChessBitboardUtils::test_bit(black_doubled_files_penalized, file)) {
				if (ChessBitboardUtils::count_set_bits(board.black_pawns & current_file_mask) > 1) {
					black_pawn_structure_score -= DOUBLED_PAWN_PENALTY;
					black_doubled_files_penalized |= (1ULL << file);
				}
			}

			// Passed Pawns for Black.
			uint64_t black_passed_pawn_mask = 0ULL;
			for (int r = rank - 1; r >= 0; --r) { // Iterate through ranks "forward" for black (downwards)
				black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file));
				if (file > 0) {
					black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file - 1));
				}
				if (file < 7) {
					black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file + 1));
				}
			}
			if ((board.white_pawns & black_passed_pawn_mask) == 0ULL) {
				int passed_pawn_bonus = PASSED_PAWN_BASE_BONUS;
				// Rank bonus for black pawns: (6 - rank) since rank 6 is base and rank 0 is promotion.
				passed_pawn_bonus += (6 - rank) * PASSED_PAWN_RANK_BONUS_FACTOR;
				black_pawn_structure_score += passed_pawn_bonus;
			}

			// Connected Pawns for Black.
			uint64_t squares_attacking_this_pawn_mask = 0ULL;
			if (file > 0 && rank < 7) { // Check backward-left diagonal (for black, this is support)
				squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank + 1, file - 1));
			}
			if (file < 7 && rank < 7) { // Check backward-right diagonal
				squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank + 1, file + 1));
			}
			if ((board.black_pawns & squares_attacking_this_pawn_mask) != 0ULL) {
				black_pawn_structure_score += CONNECTED_PAWN_BONUS;
			}

			current_black_pawns_bb &= (current_black_pawns_bb - 1);
		}

		score += white_pawn_structure_score;
		score -= black_pawn_structure_score;

		// Phase 3: King Safety Evaluation
		int white_king_safety_score = 0;
		int black_king_safety_score = 0;

		// Get the square index of each king.
		int white_king_sq = ChessBitboardUtils::get_lsb_index(board.white_king);
		int black_king_sq = ChessBitboardUtils::get_lsb_index(board.black_king);

		// King Safety Component I: Pawn Shield Analysis (uses helper function)
		white_king_safety_score += calculate_pawn_shield_penalty_internal(board, PlayerColor::White, white_king_sq, board.white_pawns);
		black_king_safety_score += calculate_pawn_shield_penalty_internal(board, PlayerColor::Black, black_king_sq, board.black_pawns);

		// King Safety Component II: Castling Bonus
		// Award a bonus if the king has castled kingside (moved to G1 for White, G8 for Black).
		// The castling_rights_mask bits are cleared once castling is no longer possible,
		// so checking if the bit is NOT set ensures castling has occurred and rights are gone.
		// White Kingside castling
		if (!((board.castling_rights_mask & (1 << 3)) != 0) && // Check if WK castling right is gone
		        (board.white_king & (1ULL << ChessBitboardUtils::G1_SQ)) != 0ULL) { // Check if king is on G1
			white_king_safety_score += CASTLING_BONUS_KINGSIDE;
		}
		// White Queenside castling
		if (!((board.castling_rights_mask & (1 << 2)) != 0) && // Check if WQ castling right is gone
		        (board.white_king & (1ULL << ChessBitboardUtils::C1_SQ)) != 0ULL) { // Check if king is on C1
			white_king_safety_score += CASTLING_BONUS_QUEENSIDE;
		}

		// Black Kingside castling
		if (!((board.castling_rights_mask & (1 << 1)) != 0) && // Check if BK castling right is gone
		        (board.black_king & (1ULL << ChessBitboardUtils::G8_SQ)) != 0ULL) { // Check if king is on G8
			black_king_safety_score += CASTLING_BONUS_KINGSIDE;
		}
		// Black Queenside castling
		if (!((board.castling_rights_mask & (1 << 0)) != 0) && // Check if BQ castling right is gone
		        (board.black_king & (1ULL << ChessBitboardUtils::C8_SQ)) != 0ULL) { // Check if king is on C8
			black_king_safety_score += CASTLING_BONUS_QUEENSIDE;
		}

		// King Safety Component III: Open/Semi-Open Files Near the King (uses helper function)
		white_king_safety_score += calculate_open_file_penalty_internal(board, PlayerColor::White, white_king_sq, board.white_pawns, board.black_pawns);
		black_king_safety_score += calculate_open_file_penalty_internal(board, PlayerColor::Black, black_king_sq, board.black_pawns, board.white_pawns);

		score += white_king_safety_score;
		score -= black_king_safety_score;

		// Phase 4: Piece Mobility (Bonus for controlled squares)
		int white_mobility_score = 0;
		int black_mobility_score = 0;

		// Get a bitboard of all occupied squares for efficient attack generation.
		uint64_t all_occupied_bb = board.occupied_squares;

		// Evaluate White Piece Mobility
		uint64_t temp_piece_bb; // Temporary bitboard for iterating through pieces
		int piece_sq_idx;       // Current square index of the piece
		uint64_t attacks_bb;    // Bitboard to store squares attacked/controlled by the current piece

		// White Pawns mobility (captures + pushes)
		temp_piece_bb = board.white_pawns;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb); // Get pawn square and clear it from temp_bb
			// Pawn attacks (diagonal captures) are precomputed in pawn_attacks table.
			attacks_bb = ChessBitboardUtils::pawn_attacks[static_cast<int>(PlayerColor::White)][piece_sq_idx];

			// Pawn pushes (forward moves) are calculated manually here, considering blockers.
			// One square forward push:
			if (ChessBitboardUtils::square_to_rank(piece_sq_idx) < 7) { // Pawns cannot push if on 8th rank
				// Calculate the square one step forward for a white pawn.
				int one_step_forward_sq = piece_sq_idx + 8;
				// Check if the square one step forward is empty.
				if (!ChessBitboardUtils::test_bit(all_occupied_bb, one_step_forward_sq)) {
					attacks_bb |= (1ULL << one_step_forward_sq); // Add to mobility if empty

					// Two squares forward push (only from 2nd rank):
					if (ChessBitboardUtils::square_to_rank(piece_sq_idx) == 1) { // If pawn is on its starting (2nd) rank
						int two_steps_forward_sq = piece_sq_idx + 16;
						// Check if the square two steps forward is also empty.
						if (!ChessBitboardUtils::test_bit(all_occupied_bb, two_steps_forward_sq)) {
							attacks_bb |= (1ULL << two_steps_forward_sq); // Add to mobility if empty
						}
					}
				}
			}
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb); // Add count of reachable squares to total mobility
		}

		// White Knights mobility
		temp_piece_bb = board.white_knights;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// Knight attacks are precomputed and stored in knight_attacks table.
			attacks_bb = ChessBitboardUtils::knight_attacks[piece_sq_idx];
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// White Bishops mobility
		temp_piece_bb = board.white_bishops;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// Bishop attacks are calculated using magic bitboards, dependent on current board occupancy.
			attacks_bb = ChessBitboardUtils::get_bishop_attacks(piece_sq_idx, all_occupied_bb);
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// White Rooks mobility
		temp_piece_bb = board.white_rooks;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// Rook attacks are calculated using magic bitboards, dependent on current board occupancy.
			attacks_bb = ChessBitboardUtils::get_rook_attacks(piece_sq_idx, all_occupied_bb);
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// White Queens mobility
		temp_piece_bb = board.white_queens;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// Queen attacks are a combination of rook attacks and bishop attacks.
			attacks_bb = ChessBitboardUtils::get_rook_attacks(piece_sq_idx, all_occupied_bb) |
			             ChessBitboardUtils::get_bishop_attacks(piece_sq_idx, all_occupied_bb);
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// White King mobility: While the king's squares are crucial for safety,
		// its "mobility" in terms of squares it attacks doesn't usually contribute
		// significantly to a general mobility bonus in the same way as other pieces.
		// It's included here for completeness and can be weighted with a smaller or zero bonus.
		temp_piece_bb = board.white_king;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// King attacks are precomputed and stored in king_attacks table.
			attacks_bb = ChessBitboardUtils::king_attacks[piece_sq_idx];
			white_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}


		// Evaluate Black Piece Mobility (symmetric calculations to White)
		// Black Pawns mobility
		temp_piece_bb = board.black_pawns;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			// Pawn attacks
			attacks_bb = ChessBitboardUtils::pawn_attacks[static_cast<int>(PlayerColor::Black)][piece_sq_idx];
			// Pawn pushes (one square forward)
			if (ChessBitboardUtils::square_to_rank(piece_sq_idx) > 0) { // Cannot push if on 1st rank
				int one_step_forward_sq = piece_sq_idx - 8; // Black pawn moves "down"
				if (!ChessBitboardUtils::test_bit(all_occupied_bb, one_step_forward_sq)) {
					attacks_bb |= (1ULL << one_step_forward_sq);
					// Pawn pushes (two squares forward from 7th rank)
					if (ChessBitboardUtils::square_to_rank(piece_sq_idx) == 6) { // If pawn is on its starting (7th) rank
						int two_steps_forward_sq = piece_sq_idx - 16;
						if (!ChessBitboardUtils::test_bit(all_occupied_bb, two_steps_forward_sq)) {
							attacks_bb |= (1ULL << two_steps_forward_sq);
						}
					}
				}
			}
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Black Knights mobility
		temp_piece_bb = board.black_knights;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			attacks_bb = ChessBitboardUtils::knight_attacks[piece_sq_idx];
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Black Bishops mobility
		temp_piece_bb = board.black_bishops;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			attacks_bb = ChessBitboardUtils::get_bishop_attacks(piece_sq_idx, all_occupied_bb);
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Black Rooks mobility
		temp_piece_bb = board.black_rooks;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			attacks_bb = ChessBitboardUtils::get_rook_attacks(piece_sq_idx, all_occupied_bb);
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Black Queens mobility
		temp_piece_bb = board.black_queens;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			attacks_bb = ChessBitboardUtils::get_rook_attacks(piece_sq_idx, all_occupied_bb) |
			             ChessBitboardUtils::get_bishop_attacks(piece_sq_idx, all_occupied_bb);
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Black King mobility
		temp_piece_bb = board.black_king;
		while (temp_piece_bb) {
			piece_sq_idx = ChessBitboardUtils::pop_bit(temp_piece_bb);
			attacks_bb = ChessBitboardUtils::king_attacks[piece_sq_idx];
			black_mobility_score += ChessBitboardUtils::count_set_bits(attacks_bb);
		}

		// Apply mobility scores to the total evaluation.
		// White's mobility is added, Black's mobility is subtracted.
		score += white_mobility_score * MOBILITY_BONUS_PER_SQUARE;
		score -= black_mobility_score * MOBILITY_BONUS_PER_SQUARE;


		return score;
	}

} // namespace Evaluation

