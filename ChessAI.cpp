#include "ChessAI.h"
#include "MoveGenerator.h"
#include "Constants.h"
#include "ChessBitboardUtils.h"
#include "Types.h"

#include <iostream>
#include <vector>
#include <chrono> 
#include <algorithm>
#include <limits>
#include <cmath>
#include <string>
#include <array>

const int ChessAI::PAWN_PST[64];
const int ChessAI::KNIGHT_PST[64];
const int ChessAI::BISHOP_PST[64];
const int ChessAI::ROOK_PST[64];
const int ChessAI::QUEEN_PST[64];
const int ChessAI::KING_PST[64];

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
    killer_moves_storage.resize(MAX_PLY * 2, Move({0,0}, {0,0}, PieceTypeIndex::NONE));
    history_scores_storage.resize(64 * 64, 0);
}

static constexpr int PIECE_SORT_VALUES[] = {
    100, // PAWN (PieceTypeIndex::PAWN = 0)
    320, // KNIGHT (PieceTypeIndex::KNIGHT = 1)
    330, // BISHOP (PieceTypeIndex::BISHOP = 2)
    500, // ROOK (PieceTypeIndex::ROOK = 3)
    900, // QUEEN (PieceTypeIndex::QUEEN = 4)
    0    // KING (PieceTypeIndex::KING = 5)
};

int ChessAI::calculate_pawn_shield_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square,
                                                    uint64_t friendly_pawns_bb) const {
    int penalty = 0;
    
    const uint64_t WHITE_KINGSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::F1_SQ) | (1ULL << ChessBitboardUtils::G1_SQ) | (1ULL << ChessBitboardUtils::H1_SQ);
    const uint64_t WHITE_QUEENSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::A1_SQ) | (1ULL << ChessBitboardUtils::B1_SQ) | (1ULL << ChessBitboardUtils::C1_SQ);
    const uint64_t BLACK_KINGSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::F8_SQ) | (1ULL << ChessBitboardUtils::G8_SQ) | (1ULL << ChessBitboardUtils::H8_SQ);
    const uint64_t BLACK_QUEENSIDE_KING_ZONE = (1ULL << ChessBitboardUtils::A8_SQ) | (1ULL << ChessBitboardUtils::B8_SQ) | (1ULL << ChessBitboardUtils::C8_SQ);

    if (king_color == PlayerColor::White) {
        if ((board_ref.white_king & WHITE_KINGSIDE_KING_ZONE) != 0ULL) { 
            uint64_t white_kingside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(1, 5)) |
                                                  (1ULL << ChessBitboardUtils::rank_file_to_square(1, 6)) |
                                                  (1ULL << ChessBitboardUtils::rank_file_to_square(1, 7));
            uint64_t missing_white_kingside_pawns = white_kingside_shield_mask & (~friendly_pawns_bb);
            penalty -= ChessBitboardUtils::count_set_bits(missing_white_kingside_pawns) * 25;

            uint64_t white_kingside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(2, 5)) |
                                                    (1ULL << ChessBitboardUtils::rank_file_to_square(2, 6)) |
                                                    (1ULL << ChessBitboardUtils::rank_file_to_square(2, 7));
            uint64_t advanced_white_kingside_pawns = friendly_pawns_bb & white_kingside_advanced_mask;
            penalty -= ChessBitboardUtils::count_set_bits(advanced_white_kingside_pawns) * 15;
        }
        if ((board_ref.white_king & WHITE_QUEENSIDE_KING_ZONE) != 0ULL) { 
            uint64_t white_queenside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(1, 0)) |
                                                   (1ULL << ChessBitboardUtils::rank_file_to_square(1, 1)) |
                                                   (1ULL << ChessBitboardUtils::rank_file_to_square(1, 2));
            uint64_t missing_white_queenside_pawns = white_queenside_shield_mask & (~friendly_pawns_bb);
            penalty -= ChessBitboardUtils::count_set_bits(missing_white_queenside_pawns) * 25;

            uint64_t white_queenside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(2, 0)) |
                                                     (1ULL << ChessBitboardUtils::rank_file_to_square(2, 1)) |
                                                     (1ULL << ChessBitboardUtils::rank_file_to_square(2, 2));
            uint64_t advanced_white_queenside_pawns = friendly_pawns_bb & white_queenside_advanced_mask;
            penalty -= ChessBitboardUtils::count_set_bits(advanced_white_queenside_pawns) * 15;
        }
    } else {
        if ((board_ref.black_king & BLACK_KINGSIDE_KING_ZONE) != 0ULL) {
            uint64_t black_kingside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(6, 5)) |
                                                  (1ULL << ChessBitboardUtils::rank_file_to_square(6, 6)) |
                                                  (1ULL << ChessBitboardUtils::rank_file_to_square(6, 7));
            uint64_t missing_black_kingside_pawns = black_kingside_shield_mask & (~friendly_pawns_bb);
            penalty -= ChessBitboardUtils::count_set_bits(missing_black_kingside_pawns) * 25;

            uint64_t black_kingside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(5, 5)) |
                                                    (1ULL << ChessBitboardUtils::rank_file_to_square(5, 6)) |
                                                    (1ULL << ChessBitboardUtils::rank_file_to_square(5, 7));
            uint64_t advanced_black_kingside_pawns = friendly_pawns_bb & black_kingside_advanced_mask;
            penalty -= ChessBitboardUtils::count_set_bits(advanced_black_kingside_pawns) * 15;
        }
        if ((board_ref.black_king & BLACK_QUEENSIDE_KING_ZONE) != 0ULL) {
            uint64_t black_queenside_shield_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(6, 0)) |
                                                   (1ULL << ChessBitboardUtils::rank_file_to_square(6, 1)) |
                                                   (1ULL << ChessBitboardUtils::rank_file_to_square(6, 2));
            uint64_t missing_black_queenside_pawns = black_queenside_shield_mask & (~friendly_pawns_bb);
            penalty -= ChessBitboardUtils::count_set_bits(missing_black_queenside_pawns) * 25;

            uint64_t black_queenside_advanced_mask = (1ULL << ChessBitboardUtils::rank_file_to_square(5, 0)) |
                                                     (1ULL << ChessBitboardUtils::rank_file_to_square(5, 1)) |
                                                     (1ULL << ChessBitboardUtils::rank_file_to_square(5, 2));
            uint64_t advanced_black_queenside_pawns = friendly_pawns_bb & black_queenside_advanced_mask;
            penalty -= ChessBitboardUtils::count_set_bits(advanced_black_queenside_pawns) * 15;
        }
    }
    return penalty;
}

int ChessAI::calculate_open_file_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square,
                                                 uint64_t friendly_pawns_bb, uint64_t enemy_pawns_bb) const {
    int penalty = 0;
    int king_file = king_square % 8;
    
    for (int f_offset = -1; f_offset <= 1; ++f_offset) {
        int current_file = king_file + f_offset;
        if (current_file >= 0 && current_file < 8) {
            uint64_t file_mask = FILE_MASKS_LOCAL[current_file];
            uint64_t file_contents = friendly_pawns_bb | enemy_pawns_bb;
            
            if (!((file_contents & file_mask) != 0ULL)) {
                penalty -= 15;
            } else if (!((friendly_pawns_bb & file_mask) != 0ULL)) {
                penalty -= 5;
            }
        }
    }
    return penalty;
}

// Reverted signature: quiescence search will now always generate its own moves
int ChessAI::quiescence_search_internal(ChessBoard& board_ref, int alpha, int beta) {
    nodes_evaluated_count++;

    uint64_t current_hash = board_ref.zobrist_hash;
    size_t tt_index = current_hash % ChessAI::TT_SIZE;
    TTEntry& entry = transposition_table[tt_index];

    if (entry.hash == current_hash) {
        if (entry.depth >= 0) {
            if (entry.flag == NodeType::EXACT) {
                return entry.score;
            }
            if (entry.flag == NodeType::LOWER_BOUND && entry.score >= beta) {
                return beta;
            }
            if (entry.flag == NodeType::UPPER_BOUND && entry.score <= alpha) {
                return alpha;
            }
        }
    }

    int stand_pat = (board_ref.active_player == PlayerColor::White) ? evaluate(board_ref) : -evaluate(board_ref);

    if (stand_pat >= beta) {
        TTEntry new_entry;
        new_entry.hash = current_hash;
        new_entry.score = beta;
        new_entry.depth = 0;
        new_entry.flag = NodeType::LOWER_BOUND;
        transposition_table[tt_index] = new_entry;
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Quiescence search now explicitly generates its own moves at each ply
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board_ref);

    std::vector<Move> noisy_moves;
    noisy_moves.reserve(legal_moves.size());

    for (const auto& move : legal_moves) {
        if (move.piece_captured_type_idx != PieceTypeIndex::NONE) {
            noisy_moves.push_back(move);
        }
        else if (move.promotion_piece_type_idx != PieceTypeIndex::NONE) {
            noisy_moves.push_back(move);
        }
    }

    std::sort(noisy_moves.begin(), noisy_moves.end(), [&](const Move& a, const Move& b) {
        int score_a = 0;
        int score_b = 0;

        if (a.piece_captured_type_idx != PieceTypeIndex::NONE) {
            score_a += PIECE_SORT_VALUES[static_cast<int>(a.piece_captured_type_idx)];
        }
        if (b.piece_captured_type_idx != PieceTypeIndex::NONE) {
            score_b += PIECE_SORT_VALUES[static_cast<int>(b.piece_captured_type_idx)];
        }
        
        if (a.promotion_piece_type_idx != PieceTypeIndex::NONE) {
            score_a += PIECE_SORT_VALUES[static_cast<int>(a.promotion_piece_type_idx)];
        }
        if (b.promotion_piece_type_idx != PieceTypeIndex::NONE) {
            score_b += PIECE_SORT_VALUES[static_cast<int>(b.promotion_piece_type_idx)];
        }
        
        return score_a > score_b;
    });

    if (noisy_moves.empty()) {
        TTEntry new_entry;
        new_entry.hash = current_hash;
        new_entry.score = stand_pat;
        new_entry.depth = 0;
        new_entry.flag = NodeType::EXACT;
        transposition_table[tt_index] = new_entry;
        return stand_pat;
    }

    Move best_q_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);

    for (const auto& move : noisy_moves) {
        branches_explored_count++;

        StateInfo info_for_undo;
        board_ref.apply_move(move, info_for_undo);

        // Recursive call to quiescence search, without passing pre-generated moves.
        // It will generate its own moves for this deeper tactical ply.
        int score = -quiescence_search_internal(board_ref, -beta, -alpha); 
        
        board_ref.undo_move(move, info_for_undo);

        if (score >= beta) {
            TTEntry new_entry;
            new_entry.hash = current_hash;
            new_entry.score = beta;
            new_entry.depth = 0;
            new_entry.flag = NodeType::LOWER_BOUND;
            new_entry.best_move = move;
            transposition_table[tt_index] = new_entry;
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_q_move = move;
        }
    }

    NodeType flag_to_store_q;
    if (alpha <= stand_pat) {
        flag_to_store_q = NodeType::UPPER_BOUND;
    } else {
        flag_to_store_q = NodeType::EXACT;
    }

    TTEntry new_entry;
    new_entry.hash = current_hash;
    new_entry.score = alpha;
    new_entry.depth = 0;
    new_entry.flag = flag_to_store_q;
    new_entry.best_move = best_q_move;
    transposition_table[tt_index] = new_entry;

    return alpha;
}


int ChessAI::evaluate(const ChessBoard& board) const {
    int score = 0;

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

    int white_pawn_structure_score = 0;
    int black_pawn_structure_score = 0;

    uint64_t white_doubled_files_penalized = 0ULL;
    uint64_t black_doubled_files_penalized = 0ULL;

    uint64_t current_white_pawns_bb = board.white_pawns;
    while (current_white_pawns_bb) {
        int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_white_pawns_bb);
        int file = pawn_sq_idx % 8;
        int rank = pawn_sq_idx / 8;

        uint64_t adjacent_files_mask = 0ULL;
        if (file > 0) {
            adjacent_files_mask |= FILE_MASKS_LOCAL[file - 1];
        }
        if (file < 7) {
            adjacent_files_mask |= FILE_MASKS_LOCAL[file + 1];
        }
        if ((board.white_pawns & adjacent_files_mask) == 0ULL) {
            white_pawn_structure_score -= 15;
        }

        uint64_t current_file_mask = FILE_MASKS_LOCAL[file];
        if (!ChessBitboardUtils::test_bit(white_doubled_files_penalized, file)) {
            if (ChessBitboardUtils::count_set_bits(board.white_pawns & current_file_mask) > 1) {
                white_pawn_structure_score -= 20;
                white_doubled_files_penalized |= (1ULL << file); 
            }
        }

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

    uint64_t current_black_pawns_bb = board.black_pawns;
    while (current_black_pawns_bb) {
        int pawn_sq_idx = ChessBitboardUtils::get_lsb_index(current_black_pawns_bb);
        int file = pawn_sq_idx % 8;
        int rank = pawn_sq_idx / 8;

        uint64_t adjacent_files_mask = 0ULL;
        if (file > 0) { adjacent_files_mask |= FILE_MASKS_LOCAL[file - 1]; }
        if (file < 7) { adjacent_files_mask |= FILE_MASKS_LOCAL[file + 1]; }
        if ((board.black_pawns & adjacent_files_mask) == 0ULL) {
            black_pawn_structure_score -= 15;
        }

        uint64_t current_file_mask = FILE_MASKS_LOCAL[file];
        if (!ChessBitboardUtils::test_bit(black_doubled_files_penalized, file)) {
            if (ChessBitboardUtils::count_set_bits(board.black_pawns & current_file_mask) > 1) {
                black_pawn_structure_score -= 20;
                black_doubled_files_penalized |= (1ULL << file);
            }
        }

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

    score += white_pawn_structure_score;
    score -= black_pawn_structure_score; 

    int white_king_safety_score = 0;
    int black_king_safety_score = 0;

    int white_king_sq = ChessBitboardUtils::get_lsb_index(board.white_king);
    int black_king_sq = ChessBitboardUtils::get_lsb_index(board.black_king);

    white_king_safety_score += calculate_pawn_shield_penalty_internal(board, PlayerColor::White, white_king_sq, board.white_pawns);
    black_king_safety_score += calculate_pawn_shield_penalty_internal(board, PlayerColor::Black, black_king_sq, board.black_pawns);

    if (!((board.castling_rights_mask & (1 << 3)) != 0) &&
        (board.white_king & (1ULL << ChessBitboardUtils::G1_SQ)) != 0ULL) {
        white_king_safety_score += 30;
    }
    if (!((board.castling_rights_mask & (1 << 2)) != 0) &&
        (board.white_king & (1ULL << ChessBitboardUtils::C1_SQ)) != 0ULL) {
        white_king_safety_score += 30;
    }

    if (!((board.castling_rights_mask & (1 << 1)) != 0) &&
        (board.black_king & (1ULL << ChessBitboardUtils::G8_SQ)) != 0ULL) {
        black_king_safety_score += 30;
    }
    if (!((board.castling_rights_mask & (1 << 0)) != 0) &&
        (board.black_king & (1ULL << ChessBitboardUtils::C8_SQ)) != 0ULL) {
        black_king_safety_score += 30;
    }

    white_king_safety_score += calculate_open_file_penalty_internal(board, PlayerColor::White, white_king_sq, board.white_pawns, board.black_pawns);
    black_king_safety_score += calculate_open_file_penalty_internal(board, PlayerColor::Black, black_king_sq, board.black_pawns, board.white_pawns);

    score += white_king_safety_score;
    score -= black_king_safety_score; 

    return score;
}

int ChessAI::alphaBeta(ChessBoard& board, int depth, int alpha, int beta) {
    int original_alpha = alpha;
    int current_ply = AI_SEARCH_DEPTH - depth;

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

    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    // Now, if we've reached the search horizon, transition to the fully functional quiescence search
    if (depth == 0) {
        // Quiescence search will generate its own moves, as intended.
        return quiescence_search_internal(board, alpha, beta);
    }

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

    std::vector<std::pair<Move, int>> scored_moves;
    scored_moves.reserve(legal_moves.size());

    for (const auto& move : legal_moves) {
        int move_score = 0;

        if (entry.best_move.piece_moved_type_idx != PieceTypeIndex::NONE &&
            move.from_square.x == entry.best_move.from_square.x &&
            move.from_square.y == entry.best_move.from_square.y &&
            move.to_square.x == entry.best_move.to_square.x &&
            move.to_square.y == entry.best_move.to_square.y &&
            move.piece_moved_type_idx == entry.best_move.piece_moved_type_idx) {
            move_score = 100000;
        }
        else if (move.piece_captured_type_idx != PieceTypeIndex::NONE) {
            move_score = PIECE_SORT_VALUES[static_cast<int>(move.piece_captured_type_idx)] * 10 
                         - PIECE_SORT_VALUES[static_cast<int>(move.piece_moved_type_idx)];
            move_score += 10000; 
        }
        else if (current_ply < MAX_PLY) {
            if (move.from_square.x == killer_moves_storage[current_ply * 2].from_square.x &&
                move.from_square.y == killer_moves_storage[current_ply * 2].from_square.y &&
                move.to_square.x == killer_moves_storage[current_ply * 2].to_square.x &&
                move.to_square.y == killer_moves_storage[current_ply * 2].to_square.y &&
                move.piece_moved_type_idx == killer_moves_storage[current_ply * 2].piece_moved_type_idx) {
                move_score = 9000;
            } else if (move.from_square.x == killer_moves_storage[current_ply * 2 + 1].from_square.x &&
                       move.from_square.y == killer_moves_storage[current_ply * 2 + 1].from_square.y &&
                       move.to_square.x == killer_moves_storage[current_ply * 2 + 1].to_square.x &&
                       move.to_square.y == killer_moves_storage[current_ply * 2 + 1].to_square.y &&
                       move.piece_moved_type_idx == killer_moves_storage[current_ply * 2 + 1].piece_moved_type_idx) {
                move_score = 8000;
            }
        }
        else {
            int from_sq_idx = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
            int to_sq_idx = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);
            move_score = history_scores_storage[from_sq_idx * 64 + to_sq_idx];
        }

        scored_moves.push_back({move, move_score});
    }

    std::sort(scored_moves.begin(), scored_moves.end(), [](const std::pair<Move, int>& a, const std::pair<Move, int>& b) {
        return a.second > b.second;
    });

    Move best_move_this_node = Move({0,0}, {0,0}, PieceTypeIndex::NONE);

    for (const auto& scored_move_pair : scored_moves) {
        const Move& move = scored_move_pair.first;

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

            if (move.piece_captured_type_idx == PieceTypeIndex::NONE &&
                move.promotion_piece_type_idx == PieceTypeIndex::NONE &&
                current_ply < MAX_PLY) {
                killer_moves_storage[current_ply * 2 + 1] = killer_moves_storage[current_ply * 2];
                killer_moves_storage[current_ply * 2] = move;
            }
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_this_node = move;

            if (move.piece_captured_type_idx == PieceTypeIndex::NONE &&
                move.promotion_piece_type_idx == PieceTypeIndex::NONE) {
                int from_sq_idx = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
                int to_sq_idx = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);
                history_scores_storage[from_sq_idx * 64 + to_sq_idx] += depth * depth; 
            }
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

    for (int i = 0; i < MAX_PLY; ++i) {
        killer_moves_storage[i * 2] = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
        killer_moves_storage[i * 2 + 1] = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    }
    for (size_t i = 0; i < history_scores_storage.size(); ++i) {
        history_scores_storage[i] = 0;
    }


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

        // Call alphaBeta, which will now transition correctly to quiescence_search_internal
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
