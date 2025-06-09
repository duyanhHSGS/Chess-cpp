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


ChessAI::ChessAI() : move_gen() {
    nodes_evaluated_count = 0;
    branches_explored_count = 0;
    current_search_depth_set = 0;
    transposition_table.resize(TT_SIZE);
    for (size_t i = 0; i < TT_SIZE; ++i) {
        transposition_table[i].hash = 0;
    }
}

int ChessAI::evaluate(const ChessBoard& board) const {
    int score = 0;
    const int PAWN_VALUE   = 100;
    const int KNIGHT_VALUE = 320;
    const int BISHOP_VALUE = 330;
    const int ROOK_VALUE   = 500;
    const int QUEEN_VALUE  = 900;
    const int KING_VALUE   = 20000;

    for (int i = 0; i < 64; ++i) {
        if (ChessBitboardUtils::test_bit(board.white_pawns, i))    score += (PAWN_VALUE + PAWN_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_knights, i)) score += (KNIGHT_VALUE + KNIGHT_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_bishops, i)) score += (BISHOP_VALUE + BISHOP_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_rooks, i))  score += (ROOK_VALUE + ROOK_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_queens, i)) score += (QUEEN_VALUE + QUEEN_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.white_king, i)) score += (KING_VALUE + KING_PST[i]);
        else if (ChessBitboardUtils::test_bit(board.black_pawns, i))   score -= (PAWN_VALUE + PAWN_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_knights, i)) score -= (KNIGHT_VALUE + KNIGHT_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_bishops, i)) score -= (BISHOP_VALUE + BISHOP_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_rooks, i))  score -= (ROOK_VALUE + ROOK_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_queens, i)) score -= (QUEEN_VALUE + QUEEN_PST[63 - i]);
        else if (ChessBitboardUtils::test_bit(board.black_king, i)) score -= (KING_VALUE + KING_PST[63 - i]);
    }
    return score;
}

int ChessAI::alphaBeta(ChessBoard& board, int depth, int alpha, int beta, std::vector<Move>& variation) {
    int original_alpha = alpha;

    uint64_t current_hash = board.zobrist_hash;
    size_t tt_index = current_hash % TT_SIZE;
    TTEntry& entry = transposition_table[tt_index];

    if (entry.hash == current_hash) {
        int tt_score = entry.score;
        if (std::abs(tt_score) > (MATE_VALUE - 1000)) {
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
        new_entry.best_move = Move({0,0},{0,0},PieceTypeIndex::NONE);
        transposition_table[tt_index] = new_entry;

        return eval_score;
    }

    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    branches_explored_count += legal_moves.size();

    if (legal_moves.empty()) {
        int terminal_score;
        if (board.is_king_in_check(board.active_player)) {
            terminal_score = -MATE_VALUE + (current_search_depth_set - depth);
        } else {
            terminal_score = 0;
        }

        TTEntry new_entry;
        new_entry.hash = current_hash;
        new_entry.score = terminal_score;
        new_entry.depth = depth;
        new_entry.flag = NodeType::EXACT;
        new_entry.best_move = Move({0,0},{0,0},PieceTypeIndex::NONE);
        transposition_table[tt_index] = new_entry;

        return terminal_score;
    }

    Move best_move_this_node = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    std::vector<Move> best_sub_variation_this_node;

    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        std::vector<Move> sub_variation;
        int score = -alphaBeta(board, depth - 1, -beta, -alpha, sub_variation);
        board.undo_move(move, info_for_undo);

        if (score >= beta) {
            TTEntry new_entry;
            new_entry.hash = current_hash;
            new_entry.score = beta;
            new_entry.depth = depth;
            new_entry.flag = NodeType::LOWER_BOUND;
            new_entry.best_move = move;
            transposition_table[tt_index] = new_entry;

            variation.push_back(move);
            variation.insert(variation.end(), sub_variation.begin(), sub_variation.end());
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_this_node = move;
            best_sub_variation_this_node = sub_variation;
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

    if (best_move_this_node.piece_moved_type_idx != PieceTypeIndex::NONE) {
        variation.push_back(best_move_this_node);
        variation.insert(variation.end(), best_sub_variation_this_node.begin(), best_sub_variation_this_node.end());
    }
    
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
    int best_eval = -MATE_VALUE - 1;
    int alpha = -MATE_VALUE - 1;
    int beta = MATE_VALUE + 1;
    std::vector<Move> best_variation;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        std::vector<Move> current_variation;
        int current_score = -alphaBeta(board, AI_SEARCH_DEPTH - 1, -beta, -alpha, current_variation);
        board.undo_move(move, info_for_undo);

        if (current_score > best_eval) {
            best_eval = current_score;
            final_chosen_move = move;
            best_variation = current_variation;
            best_variation.insert(best_variation.begin(), final_chosen_move);
        }
        alpha = std::max(alpha, current_score);
        if (alpha >= beta) {
            break;
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    long long duration_ms = duration_microseconds.count() / 1000;

    std::cerr << "DEBUG: Carolyna: Principal Variation: ";
    for (const auto& move : best_variation) {
        std::cerr << ChessBitboardUtils::move_to_string(move) << " ";
    }
    std::cerr << std::endl;

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
        final_display_score = -best_eval;
    }

    std::string score_string;
    if (std::abs(final_display_score) >= MATE_VALUE) {
        score_string = "mate " + std::to_string((MATE_VALUE - std::abs(final_display_score) + current_search_depth_set) * (final_display_score > 0 ? 1 : -1));
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
