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

ChessAI::ChessAI() {
    nodes_evaluated_count = 0;
    branches_explored_count = 0;
    current_search_depth_set = 0;
}

int ChessAI::evaluate(const ChessBoard& board) const {
    int score = 0;
    const int PAWN_VALUE   = 100;
    const int KNIGHT_VALUE = 320;
    const int BISHOP_VALUE = 330;
    const int ROOK_VALUE   = 500;
    const int QUEEN_VALUE  = 900;

    for (int i = 0; i < 64; ++i) {
        if (ChessBitboardUtils::test_bit(board.white_pawns, i))    score += PAWN_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_knights, i)) score += KNIGHT_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_bishops, i)) score += BISHOP_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_rooks, i))  score += ROOK_VALUE;
        else if (ChessBitboardUtils::test_bit(board.white_queens, i)) score += QUEEN_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_pawns, i))   score -= PAWN_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_knights, i)) score -= KNIGHT_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_bishops, i)) score -= BISHOP_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_rooks, i))  score -= ROOK_VALUE;
        else if (ChessBitboardUtils::test_bit(board.black_queens, i)) score -= QUEEN_VALUE;
    }
    return score;
}

int ChessAI::alphaBeta(ChessBoard& board, int depth, int alpha, int beta, std::vector<Move>& variation) {
    nodes_evaluated_count++;
    variation.clear();

    if (depth == 0) {
        return (board.active_player == PlayerColor::White) ? evaluate(board) : -evaluate(board);
    }

    MoveGenerator move_gen;
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    branches_explored_count += legal_moves.size();

    if (legal_moves.empty()) {
        return (board.active_player == PlayerColor::White) ? evaluate(board) : -evaluate(board);
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
            variation.emplace_back(move.from_square, move.to_square,
                                   move.piece_moved_type_idx, move.piece_captured_type_idx,
                                   move.is_promotion, move.promotion_piece_type_idx,
                                   move.is_kingside_castle, move.is_queenside_castle,
                                   move.is_en_passant, move.is_double_pawn_push);
            variation.insert(variation.end(), sub_variation.begin(), sub_variation.end());
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            best_move_this_node = move;
            best_sub_variation_this_node = sub_variation;
        }
    }

    if (best_move_this_node.piece_moved_type_idx != PieceTypeIndex::NONE) {
        variation.emplace_back(best_move_this_node.from_square, best_move_this_node.to_square,
                               best_move_this_node.piece_moved_type_idx, best_move_this_node.piece_captured_type_idx,
                               best_move_this_node.is_promotion, best_move_this_node.promotion_piece_type_idx,
                               best_move_this_node.is_kingside_castle, best_move_this_node.is_queenside_castle,
                               best_move_this_node.is_en_passant, best_move_this_node.is_double_pawn_push);
        variation.insert(variation.end(), best_sub_variation_this_node.begin(), best_sub_variation_this_node.end());
    }
    return alpha;
}

Move ChessAI::findBestMove(ChessBoard& board) {
    nodes_evaluated_count = 0;
    branches_explored_count = 0;
    current_search_depth_set = AI_SEARCH_DEPTH;

    MoveGenerator move_gen;
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    if (legal_moves.empty()) {
        std::cerr << "DEBUG: Carolyna: No legal moves found. Game is likely over (checkmate or stalemate)." << std::endl;
        return Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    }

    PlayerColor original_active_player = board.active_player;

    Move final_chosen_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    int best_eval = -99999999;
    int alpha = -99999999;
    int beta = 99999999;
    std::vector<Move> best_variation;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        // The ChessBoard temp_board_for_eval = board; was removed here.
        // Instead, we apply the move directly to the 'board' passed by reference.
        board.apply_move(move, info_for_undo);

        std::vector<Move> current_variation;
        // Pass 'board' directly to alphaBeta, as the move has now been applied to it.
        int current_score = -alphaBeta(board, AI_SEARCH_DEPTH - 1, -beta, -alpha, current_variation);
        // Undo the move immediately after the recursive call returns to restore the board state.
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
        nodes_per_second = (nodes_evaluated_count * 1000) / duration_ms;
    } else if (nodes_evaluated_count > 0) {
        nodes_per_second = nodes_evaluated_count * 1000000;
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
    if (final_display_score > 0) {
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
