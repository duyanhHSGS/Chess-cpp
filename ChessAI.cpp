#include "ChessAI.h"
#include "MoveGenerator.h"
#include "Constants.h"
#include "ChessBitboardUtils.h"

#include <iostream>
#include <vector>
#include <chrono> 
#include <algorithm>
#include <limits>
#include <cmath>
#include <string>
#include <array> // Still needed for std::array used in FILE_MASKS_LOCAL definition below.

// Out-of-class definitions for static constexpr members declared in ChessAI.h.
const int ChessAI::PAWN_PST[64];
const int ChessAI::KNIGHT_PST[64];
const int ChessAI::BISHOP_PST[64];
const int ChessAI::ROOK_PST[64];
const int ChessAI::QUEEN_PST[64];
const int ChessAI::KING_PST[64];

// Global constexpr array for file masks, accessible throughout ChessAI.cpp.
// It's defined here (in .cpp) to ensure it's a single instance and initialized at compile time.
const std::array<uint64_t, 8> FILE_MASKS_LOCAL = {
    ChessBitboardUtils::FILE_A, ChessBitboardUtils::FILE_B, ChessBitboardUtils::FILE_C,
    ChessBitboardUtils::FILE_D, ChessBitboardUtils::FILE_E, ChessBitboardUtils::FILE_F,
    ChessBitboardUtils::FILE_G, ChessBitboardUtils::FILE_H
};


ChessAI::ChessAI() : move_gen() {
    nodes_evaluated_count = 0;
    branches_explored_count = 0;
    current_search_depth_set = 0;
    transposition_table.resize(ChessAI::TT_SIZE);
    for (size_t i = 0; i < ChessAI::TT_SIZE; ++i) {
        transposition_table[i].hash = 0;
    }
}

int ChessAI::evaluate(const ChessBoard& board) const {
    int score = 0;

    // --- Phase 1: Material and Basic Piece-Square Tables (iterating all 64 squares) ---
    for (int i = 0; i < 64; ++i) {
        if (ChessBitboardUtils::test_bit(board.white_pawns, i))    score += (ChessAI::PAWN_VALUE + ChessAI::PAWN_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_knights, i)) score += (ChessAI::KNIGHT_VALUE + ChessAI::KNIGHT_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_bishops, i)) score += (ChessAI::BISHOP_VALUE + ChessAI::BISHOP_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_rooks, i))  score += (ChessAI::ROOK_VALUE + ChessAI::ROOK_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_queens, i)) score += (ChessAI::QUEEN_VALUE + ChessAI::QUEEN_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_king, i)) score += (ChessAI::KING_VALUE + ChessAI::KING_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.black_pawns, i))   score -= (ChessAI::PAWN_VALUE + ChessAI::PAWN_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_knights, i)) score -= (ChessAI::KNIGHT_VALUE + ChessAI::KNIGHT_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_bishops, i)) score -= (ChessAI::BISHOP_VALUE + ChessAI::BISHOP_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_rooks, i))  score -= (ChessAI::ROOK_VALUE + ChessAI::ROOK_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_queens, i)) score -= (ChessAI::QUEEN_VALUE + ChessAI::QUEEN_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_king, i)) score -= (ChessAI::KING_VALUE + ChessAI::KING_PST[63 - i]);
    }

    // --- Phase 2: Advanced Pawn Structure (iterating only over pawns using bit scanning) ---
    int white_pawn_structure_score = 0;
    int black_pawn_structure_score = 0;

    // Track files that have already been penalized for doubled pawns to avoid double-counting.
    uint64_t white_doubled_files_penalized = 0ULL;
    uint64_t black_doubled_files_penalized = 0ULL;

    // --- Evaluate White Pawns ---
    uint64_t current_white_pawns_bb = board.white_pawns;
    while (current_white_pawns_bb) {
        int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_white_pawns_bb);
        int file = pawn_sq_idx % 8;
        int rank = pawn_sq_idx / 8;

        // I. Isolated Pawn Check (White)
        uint64_t adjacent_files_mask = 0ULL;
        if (file > 0) {
            adjacent_files_mask |= FILE_MASKS_LOCAL[file - 1]; // Corrected: Using global FILE_MASKS_LOCAL
        }
        if (file < 7) {
            adjacent_files_mask |= FILE_MASKS_LOCAL[file + 1]; // Corrected: Using global FILE_MASKS_LOCAL
        }
        if ((board.white_pawns & adjacent_files_mask) == 0ULL) {
            white_pawn_structure_score -= 15;
        }

        // II. Doubled Pawn Check (White)
        uint64_t current_file_mask = FILE_MASKS_LOCAL[file]; // Corrected: Using global FILE_MASKS_LOCAL
        if (!ChessBitboardUtils::test_bit(white_doubled_files_penalized, file)) {
            if (ChessBitboardUtils::count_set_bits(board.white_pawns & current_file_mask) > 1) {
                white_pawn_structure_score -= 20;
                white_doubled_files_penalized |= (1ULL << file); 
            }
        }

        // III. Passed Pawn Check (White)
        uint64_t white_passed_pawn_mask = 0ULL;
        for (int r = rank + 1; r < 8; ++r) {
            white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file));
            if (file > 0) {
                white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file - 1));
            }
            if (file < 7) {
                white_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file + 1));
            }
        }
        if ((board.black_pawns & white_passed_pawn_mask) == 0ULL) {
            int passed_pawn_bonus = 20;
            passed_pawn_bonus += (rank - 1) * 10;
            white_pawn_structure_score += passed_pawn_bonus;
        }

        // IV. Connected/Protected Pawn Check (White)
        uint64_t squares_attacking_this_pawn_mask = 0ULL;
        if (file > 0 && rank > 0) {
             squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank - 1, file - 1));
        }
        if (file < 7 && rank > 0) {
             squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank - 1, file + 1));
        }
        if ((board.white_pawns & squares_attacking_this_pawn_mask) != 0ULL) {
            white_pawn_structure_score += 10;
        }

        current_white_pawns_bb &= (current_white_pawns_bb - 1);
    }

    // --- Evaluate Black Pawns (similar logic, adjusted for black's perspective) ---
    uint64_t current_black_pawns_bb = board.black_pawns;
    while (current_black_pawns_bb) {
        int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_black_pawns_bb);
        int file = pawn_sq_idx % 8;
        int rank = pawn_sq_idx / 8;

        // I. Isolated Pawn Check (Black)
        uint64_t adjacent_files_mask = 0ULL;
        if (file > 0) { adjacent_files_mask |= FILE_MASKS_LOCAL[file - 1]; }
        if (file < 7) { adjacent_files_mask |= FILE_MASKS_LOCAL[file + 1]; }
        if ((board.black_pawns & adjacent_files_mask) == 0ULL) {
            black_pawn_structure_score -= 15;
        }

        // II. Doubled Pawn Check (Black)
        uint64_t current_file_mask = FILE_MASKS_LOCAL[file];
        if (!ChessBitboardUtils::test_bit(black_doubled_files_penalized, file)) {
            if (ChessBitboardUtils::count_set_bits(board.black_pawns & current_file_mask) > 1) {
                black_pawn_structure_score -= 20;
                black_doubled_files_penalized |= (1ULL << file);
            }
        }

        // III. Passed Pawn Check (Black)
        uint64_t black_passed_pawn_mask = 0ULL;
        for (int r = rank - 1; r >= 0; --r) {
            black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file));
            if (file > 0) {
                black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file - 1));
            }
            if (file < 7) {
                black_passed_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(r, file + 1));
            }
        }
        if ((board.white_pawns & black_passed_pawn_mask) == 0ULL) {
            int passed_pawn_bonus = 20;
            passed_pawn_bonus += (6 - rank) * 10; 
            black_pawn_structure_score += passed_pawn_bonus;
        }

        // IV. Connected/Protected Pawn Check (Black)
        uint64_t squares_attacking_this_pawn_mask = 0ULL;
        if (file > 0 && rank < 7) { 
             squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank + 1, file - 1));
        }
        if (file < 7 && rank < 7) { 
             squares_attacking_this_pawn_mask |= (1ULL << ChessBitboardUtils::rank_file_to_square(rank + 1, file + 1));
        }
        if ((board.black_pawns & squares_attacking_this_pawn_mask) != 0ULL) {
            black_pawn_structure_score += 10; 
        }
        
        current_black_pawns_bb &= (current_black_pawns_bb - 1);
    }

    // --- Final Score Adjustment for Pawn Structure ---
    score += white_pawn_structure_score;
    score -= black_pawn_structure_score; 

    return score;
}

int ChessAI::alphaBeta(ChessBoard& board, int depth, int alpha, int beta) {
    int original_alpha = alpha;

    uint64_t current_hash = board.zobrist_hash;
    size_t tt_index = current_hash % ChessAI::TT_SIZE;
    TTEntry& entry = transposition_table[tt_index];

    if (entry.hash == current_hash) {
        int tt_score = entry.score;
        if (std::abs(tt_score) > (ChessAI::MATE_VALUE - 1000)) {
            if (tt_score > 0) {
                tt_score -= (current_search_depth_set - depth);
            } else {
                tt_score += (current_search_depth_set - depth);
            }
        }

        if (entry.depth >= depth) {
            if (entry.flag == NodeType::EXACT) {
                return tt_score;
            }
            if (entry.flag == NodeType::LOWER_BOUND && tt_score >= beta) {
                return beta;
            }
            if (entry.flag == NodeType::UPPER_BOUND && tt_score <= alpha) {
                return alpha;
            }
        }
    }
    
    nodes_evaluated_count++;

    if (depth == 0) {
        int eval_score = (board.active_player == PlayerColor::White) ? evaluate(board) : -evaluate(board);
        
        TTEntry new_entry;
        new_entry.hash = current_hash;
        new_entry.score = eval_score;
        new_entry.depth = depth;
        new_entry.flag = NodeType::EXACT;
        transposition_table[tt_index] = new_entry;

        return eval_score;
    }

    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    branches_explored_count += legal_moves.size();

    if (legal_moves.empty()) {
        int terminal_score;
        if (board.is_king_in_check(board.active_player)) {
            terminal_score = -ChessAI::MATE_VALUE + (current_search_depth_set - depth);
        } else {
            terminal_score = 0;
        }

        TTEntry new_entry;
        new_entry.hash = current_hash;
        new_entry.score = terminal_score;
        new_entry.depth = depth;
        new_entry.flag = NodeType::EXACT;
        transposition_table[tt_index] = new_entry;

        return terminal_score;
    }

    Move best_move_this_node = Move({0,0}, {0,0}, PieceTypeIndex::NONE);

    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        int score = -alphaBeta(board, depth - 1, -beta, -alpha);
        board.undo_move(move, info_for_undo);

        if (score >= beta) {
            TTEntry new_entry;
            new_entry.hash = current_hash;
            new_entry.score = beta;
            new_entry.depth = depth;
            new_entry.flag = NodeType::LOWER_BOUND;
            new_entry.best_move = move; 
            transposition_table[tt_index] = new_entry;

            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_this_node = move;
        }
    }

    NodeType flag_to_store;
    if (alpha <= original_alpha) {
        flag_to_store = NodeType::UPPER_BOUND;
    } else {
        flag_to_store = NodeType::EXACT;
    }

    TTEntry new_entry;
    new_entry.hash = current_hash;
    new_entry.score = alpha;
    new_entry.depth = depth;
    new_entry.flag = flag_to_store;
    new_entry.best_move = best_move_this_node; 
    transposition_table[tt_index] = new_entry;
    
    return alpha;
}

Move ChessAI::findBestMove(ChessBoard& board) {
    nodes_evaluated_count = 0;
    branches_explored_count = 0;
    current_search_depth_set = AI_SEARCH_DEPTH;

    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    if (legal_moves.empty()) {
        std::cerr << "DEBUG: Carolyna: No legal moves found. Game is likely over (checkmate or stalemate)." << std::endl;
        return Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    }

    PlayerColor original_active_player = board.active_player;

    Move final_chosen_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    int best_eval = -ChessAI::MATE_VALUE - 1;
    int alpha = -ChessAI::MATE_VALUE - 1;
    int beta = ChessAI::MATE_VALUE + 1;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        int current_score = -alphaBeta(board, AI_SEARCH_DEPTH - 1, -beta, -alpha);
        board.undo_move(move, info_for_undo);

        if (current_score > best_eval) {
            best_eval = current_score;
            final_chosen_move = move;
        }
        alpha = std::max(alpha, current_score);
        if (alpha >= beta) {
            break;
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    long long duration_ms = duration_microseconds.count() / 1000;

    long long nodes_per_second = 0;
    if (duration_ms > 0) {
        nodes_per_second = (static_cast<long long>(nodes_evaluated_count) * 1000) / duration_ms;
    } else if (nodes_evaluated_count > 0) {
        nodes_per_second = static_cast<long long>(nodes_evaluated_count) * 1000000;
    }

    std::cerr << "DEBUG: Carolyna: Completed search to depth " << current_search_depth_set
              << ". Nodes: " << nodes_evaluated_count
              << ", Branches: " << branches_explored_count
              << ", Time: " << duration_ms << "ms"
              << ", NPS: " << nodes_per_second << std::endl;
    
    int final_display_score = best_eval;
    if (original_active_player == PlayerColor::Black) {
        final_display_score = -final_display_score;
    }

    std::string score_string;
    if (std::abs(final_display_score) >= ChessAI::MATE_VALUE) {
        score_string = "mate " + std::to_string((ChessAI::MATE_VALUE - std::abs(final_display_score) + current_search_depth_set) * (final_display_score > 0 ? 1 : -1));
    } else if (final_display_score > 0) {
        score_string = "+" + std::to_string(final_display_score);
    } else if (final_display_score < 0) {
        score_string = std::to_string(final_display_score);
    } else {
        score_string = "0";
    }
    
    std::cerr << "DEBUG: Carolyna : Chose move for "
              << (original_active_player == PlayerColor::White ? "White" : "Black")
              << ": " << ChessBitboardUtils::move_to_string(final_chosen_move)
              << " with score: " << score_string << std::endl;

    return final_chosen_move;
}
