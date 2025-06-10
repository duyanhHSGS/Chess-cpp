#include "ChessAI.h"
#include "MoveGenerator.h"
#include "Constants.h" // Now includes all common constants
#include "ChessBitboardUtils.h"
#include "Types.h"
#include "Evaluation.h" // Include the new Evaluation header

#include <iostream>
#include <vector>
#include <chrono> 
#include <algorithm>
#include <limits>
#include <cmath>
#include <string>
#include <array>

// PSTs remain here as members of ChessAI, not global constants
const int ChessAI::PAWN_PST[64];
const int ChessAI::KNIGHT_PST[64];
const int ChessAI::BISHOP_PST[64];
const int ChessAI::ROOK_PST[64];
const int ChessAI::QUEEN_PST[64];
const int ChessAI::KING_PST[64];


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

// PIECE_SORT_VALUES remains local to ChessAI.cpp for move ordering heuristics
static constexpr int PIECE_SORT_VALUES[] = {
    100, // PAWN (PieceTypeIndex::PAWN = 0)
    320, // KNIGHT (PieceTypeIndex::KNIGHT = 1)
    330, // BISHOP (PieceTypeIndex::BISHOP = 2)
    500, // ROOK (PieceTypeIndex::ROOK = 3)
    900, // QUEEN (PieceTypeIndex::QUEEN = 4)
    0    // KING (PieceTypeIndex::KING = 5)
};


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

    // Call the new Evaluation::evaluate function
    int stand_pat = (board_ref.active_player == PlayerColor::White) ? Evaluation::evaluate(board_ref) : -Evaluation::evaluate(board_ref);

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

    if (depth == 0) {
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
            history_scores_storage[from_sq_idx * 64 + to_sq_idx] += depth * depth; 
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

        int current_score = -alphaBeta(board, AI_SEARCH_DEPTH - 1, -beta, -alpha); // Use AI_SEARCH_DEPTH directly
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
