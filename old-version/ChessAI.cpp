#include "ChessAI.h"
#include "MoveGenerator.h"
#include "Constants.h"
#include "ChessBitboardUtils.h"

#include <random>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits> // Required for std::numeric_limits
#include <future> // Required for std::async and std::future
#include <atomic> // Required for std::atomic

ChessAI::ChessAI() : rng_engine(std::random_device{}()) {
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
    const int KING_SAFETY_PENALTY = 100000;

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

    if (board.is_king_in_check(PlayerColor::White)) {
        score -= KING_SAFETY_PENALTY;
    }
    if (board.is_king_in_check(PlayerColor::Black)) {
        score += KING_SAFETY_PENALTY;
    }

    return score;
}

int ChessAI::minimax(ChessBoard& board, int depth) {
    nodes_evaluated_count++;

    // 1. Base case: Max depth reached. Return static evaluation.
    if (depth == 0) {
        return (board.active_player == PlayerColor::White) ? evaluate(board) : -evaluate(board);
    }

    MoveGenerator move_gen;
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    branches_explored_count += legal_moves.size();

    // 2. Base case: No legal moves. Check for checkmate or stalemate.
    if (legal_moves.empty()) {
        if (board.is_king_in_check(board.active_player)) {
            // Checkmate: Current player is checkmated. Return a very low score from their perspective.
            // Add depth to prefer shorter mates (or delay opponent's mate).
            return std::numeric_limits<int>::min() + (current_search_depth_set - depth);
        } else {
            // Stalemate: Return 0.
            return 0;
        }
    }

    // 3. Recursive step: Current player is maximizing.
    int best_eval = std::numeric_limits<int>::min();

    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        // Recursively call minimax for the opponent. Negate the result.
        int eval = -minimax(board, depth - 1); 

        best_eval = std::max(best_eval, eval);

        board.undo_move(move, info_for_undo);
    }
    return best_eval;
}

int ChessAI::alphaBeta(ChessBoard& board, int depth, int alpha, int beta) {
    nodes_evaluated_count++;

    // 1. Base case: Max depth reached. Return static evaluation.
    if (depth == 0) {
        return (board.active_player == PlayerColor::White) ? evaluate(board) : -evaluate(board);
    }

    MoveGenerator move_gen;
    std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);

    branches_explored_count += legal_moves.size();

    // 2. Base case: No legal moves. Check for checkmate or stalemate.
    if (legal_moves.empty()) {
        if (board.is_king_in_check(board.active_player)) {
            // Checkmate: Current player is checkmated. Return a very low score from their perspective.
            // Add depth to prefer shorter mates (or delay opponent's mate).
            return std::numeric_limits<int>::min() + (current_search_depth_set - depth);
        } else {
            // Stalemate: Return 0.
            return 0;
        }
    }

    // 3. Recursive step:
    for (const auto& move : legal_moves) {
        StateInfo info_for_undo;
        board.apply_move(move, info_for_undo);

        // Recursively call alphaBeta for the opponent. Negate score, swap alpha/beta.
        int score = -alphaBeta(board, depth - 1, -beta, -alpha);

        board.undo_move(move, info_for_undo);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
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
        std::cerr << "DEBUG: ChessAI: No legal moves found. Game is likely over (checkmate or stalemate)." << std::endl;
        return Move({0,0}, {0,0}, PieceTypeIndex::NONE);
    }

    switch (DEFAULT_AI_TYPE) {
        case AIType::RANDOM_MOVER: {
            std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
            Move chosen_move = legal_moves[dist(rng_engine)];
            std::cerr << "DEBUG: ChessAI (Random): Chose move: " << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
            return chosen_move;
        }
        case AIType::SIMPLE_EVALUATION: {
            PlayerColor original_active_player = board.active_player;

            int best_score;
            std::vector<Move> best_moves_candidates;

            if (original_active_player == PlayerColor::White) {
                best_score = -999999;
            } else {
                best_score = 999999;
            }
            
            for (const auto& move : legal_moves) {
                StateInfo info_for_undo;

                board.apply_move(move, info_for_undo);

                int current_score_from_white_perspective = evaluate(board); 

                if (original_active_player == PlayerColor::White) {
                    if (current_score_from_white_perspective > best_score) {
                        best_score = current_score_from_white_perspective;
                        best_moves_candidates.clear();
                        best_moves_candidates.push_back(move);
                    } else if (current_score_from_white_perspective == best_score) {
                        best_moves_candidates.push_back(move);
                    }
                } else {
                    if (current_score_from_white_perspective < best_score) {
                        best_score = current_score_from_white_perspective;
                        best_moves_candidates.clear();
                        best_moves_candidates.push_back(move);
                    } else if (current_score_from_white_perspective == best_score) {
                        best_moves_candidates.push_back(move);
                    }
                }

                board.undo_move(move, info_for_undo);
            }

            Move final_chosen_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
            
            if (!best_moves_candidates.empty()) {
                std::uniform_int_distribution<size_t> dist(0, best_moves_candidates.size() - 1);
                final_chosen_move = best_moves_candidates[dist(rng_engine)];
            } else {
                std::cerr << "DEBUG: ChessAI (Simple Evaluation): No best candidates found, returning default invalid move (fallback)." << std::endl;
            }

            std::cerr << "DEBUG: ChessAI (Simple Evaluation): Chose move: " << ChessBitboardUtils::move_to_string(final_chosen_move) 
                      << " with score (White's perspective): " << best_score << std::endl;
            return final_chosen_move;
        }
        case AIType::MINIMAX: {
            nodes_evaluated_count = 0;
            branches_explored_count = 0;
            current_search_depth_set = AI_SEARCH_DEPTH;

            PlayerColor original_active_player = board.active_player;

            Move final_chosen_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
            int best_eval = std::numeric_limits<int>::min(); // Use numeric_limits

            auto start_time = std::chrono::high_resolution_clock::now();

            if (NUMBER_OF_CORES_USED == 1) {
                
                size_t best_move_index = 0;

                for (size_t i = 0; i < legal_moves.size(); ++i) {
                    StateInfo info_for_undo;

                    board.apply_move(legal_moves[i], info_for_undo);

                    int current_eval = -minimax(board, AI_SEARCH_DEPTH - 1); 

                    if (current_eval > best_eval) {
                        best_eval = current_eval;
                        best_move_index = i;
                    }

                    board.undo_move(legal_moves[i], info_for_undo);
                }
                final_chosen_move = legal_moves[best_move_index];

            } else {
                size_t best_move_index = 0;
                std::vector<std::pair<int, size_t>> results_from_threads;

                if (legal_moves.size() <= NUMBER_OF_CORES_USED) { 
                    std::vector<std::future<std::pair<int, size_t>>> futures;
                    for (size_t i = 0; i < legal_moves.size(); ++i) {
                        futures.push_back(std::async(std::launch::async, 
                                                    [this, i, board, legal_moves]() -> std::pair<int, size_t> {
                            ChessBoard board_copy = board;
                            StateInfo info;
                            board_copy.apply_move(legal_moves[i], info);
                            
                            int eval = -minimax(board_copy, AI_SEARCH_DEPTH - 1);
                            return std::make_pair(eval, i);
                        }));
                    }
                    for (auto &f : futures) {
                        results_from_threads.push_back(f.get());
                    }
                } else {
                    unsigned int num_threads_to_use = NUMBER_OF_CORES_USED;
                    size_t moves_per_thread = legal_moves.size() / num_threads_to_use;
                    size_t current_move_idx = 0;
                    std::vector<std::future<std::pair<int, size_t>>> futures;

                    for (unsigned int i = 0; i < num_threads_to_use; ++i) {
                        size_t start_idx = current_move_idx;
                        size_t end_idx = (i == num_threads_to_use - 1) ? legal_moves.size() : (start_idx + moves_per_thread);

                        futures.push_back(std::async(std::launch::async,
                            [this, start_idx, end_idx, board, legal_moves]() -> std::pair<int, size_t> {
                                int local_best_eval = std::numeric_limits<int>::min();
                                size_t local_best_index = start_idx;

                                for (size_t j = start_idx; j < end_idx; ++j) {
                                    ChessBoard board_copy = board;
                                    StateInfo info;
                                    board_copy.apply_move(legal_moves[j], info);
                                    
                                    int eval = -minimax(board_copy, AI_SEARCH_DEPTH - 1);
                                    
                                    if (eval > local_best_eval) {
                                        local_best_eval = eval;
                                        local_best_index = j;
                                    }
                                }
                                return std::make_pair(local_best_eval, local_best_index);
                            }
                        ));
                        current_move_idx = end_idx;
                    }
                    for (auto &f : futures) {
                        results_from_threads.push_back(f.get());
                    }
                }
                
                std::vector<Move> best_moves_candidates;
                
                for (const auto& result : results_from_threads) {
                    if (result.first > best_eval) {
                        best_eval = result.first;
                        best_moves_candidates.clear();
                        best_moves_candidates.push_back(legal_moves[result.second]);
                    } else if (result.first == best_eval) {
                        best_moves_candidates.push_back(legal_moves[result.second]);
                    }
                }

                if (!best_moves_candidates.empty()) {
                    std::uniform_int_distribution<size_t> dist(0, best_moves_candidates.size() - 1);
                    final_chosen_move = best_moves_candidates[dist(rng_engine)];
                } else {
                    std::cerr << "DEBUG: ChessAI (Minimax Multithreaded): No best candidates found, returning default invalid move (fallback)." << std::endl;
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            long long duration_ms = duration_microseconds.count() / 1000;

            long long nodes_per_second = 0;
            if (duration_ms > 0) {
                nodes_per_second = (nodes_evaluated_count * 1000) / duration_ms;
            } else if (nodes_evaluated_count > 0) {
                nodes_per_second = nodes_evaluated_count * 1000000; 
            }

            std::cerr << "DEBUG: ChessAI (Minimax Cores Used: " << static_cast<int>(NUMBER_OF_CORES_USED) << "):" << std::endl;
            std::cerr << "Completed search to depth " << current_search_depth_set
                      << ". Nodes: " << nodes_evaluated_count
                      << ", Branches: " << branches_explored_count
                      << ", Time: " << duration_ms << "ms"
                      << ", NPS: " << nodes_per_second << std::endl;
            
            std::cerr << "DEBUG: ChessAI (Minimax Cores Used: " << static_cast<int>(NUMBER_OF_CORES_USED) << "): Chose move: "
                      << ChessBitboardUtils::move_to_string(final_chosen_move)
                      << " with score (current player's perspective at root): " << best_eval << std::endl;
            
            return final_chosen_move;
        }
        case AIType::ALPHA_BETA: {
            nodes_evaluated_count = 0;
            branches_explored_count = 0;
            current_search_depth_set = AI_SEARCH_DEPTH;

            PlayerColor original_active_player = board.active_player;

            Move final_chosen_move = Move({0,0}, {0,0}, PieceTypeIndex::NONE);
            int best_eval = std::numeric_limits<int>::min(); // Use numeric_limits
            
            int alpha = std::numeric_limits<int>::min(); // Use numeric_limits
            int beta = std::numeric_limits<int>::max(); // Use numeric_limits

            auto start_time = std::chrono::high_resolution_clock::now();

            if (NUMBER_OF_CORES_USED == 1) {
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
            } else {
                size_t best_move_index = 0;
                std::vector<std::pair<int, size_t>> results_from_threads;

                if (legal_moves.size() <= NUMBER_OF_CORES_USED) { 
                    std::vector<std::future<std::pair<int, size_t>>> futures;
                    for (size_t i = 0; i < legal_moves.size(); ++i) {
                        futures.push_back(std::async(std::launch::async, 
                                                    [this, i, board, legal_moves, beta_val = beta, alpha_val = alpha]() -> std::pair<int, size_t> {
                            ChessBoard board_copy = board;
                            StateInfo info;
                            board_copy.apply_move(legal_moves[i], info);
                            
                            int eval = -alphaBeta(board_copy, AI_SEARCH_DEPTH - 1, -beta_val, -alpha_val);
                            return std::make_pair(eval, i);
                        }));
                    }
                    for (auto &f : futures) {
                        results_from_threads.push_back(f.get());
                    }
                } else {
                    unsigned int num_threads_to_use = NUMBER_OF_CORES_USED;
                    size_t moves_per_thread = legal_moves.size() / num_threads_to_use;
                    size_t current_move_idx = 0;
                    std::vector<std::future<std::pair<int, size_t>>> futures;

                    for (unsigned int i = 0; i < num_threads_to_use; ++i) {
                        size_t start_idx = current_move_idx;
                        size_t end_idx = (i == num_threads_to_use - 1) ? legal_moves.size() : (start_idx + moves_per_thread);

                        futures.push_back(std::async(std::launch::async,
                            [this, start_idx, end_idx, board, legal_moves, beta_val = beta, alpha_val = alpha]() -> std::pair<int, size_t> {
                                int local_best_eval = std::numeric_limits<int>::min();
                                size_t local_best_index = start_idx;

                                for (size_t j = start_idx; j < end_idx; ++j) {
                                    ChessBoard board_copy = board;
                                    StateInfo info;
                                    board_copy.apply_move(legal_moves[j], info);
                                    
                                    int eval = -alphaBeta(board_copy, AI_SEARCH_DEPTH - 1, -beta_val, -alpha_val);
                                    
                                    if (eval > local_best_eval) {
                                        local_best_eval = eval;
                                        local_best_index = j;
                                    }
                                }
                                return std::make_pair(local_best_eval, local_best_index);
                            }
                        ));
                        current_move_idx = end_idx;
                    }
                    for (auto &f : futures) {
                        results_from_threads.push_back(f.get());
                    }
                }
                
                std::vector<Move> best_moves_candidates;
                
                for (const auto& result : results_from_threads) {
                    if (result.first > best_eval) {
                        best_eval = result.first;
                        best_moves_candidates.clear();
                        best_moves_candidates.push_back(legal_moves[result.second]);
                    } else if (result.first == best_eval) {
                        best_moves_candidates.push_back(legal_moves[result.second]);
                    }
                }

                if (!best_moves_candidates.empty()) {
                    std::uniform_int_distribution<size_t> dist(0, best_moves_candidates.size() - 1);
                    final_chosen_move = best_moves_candidates[dist(rng_engine)];
                } else {
                    std::cerr << "DEBUG: ChessAI (Alpha-Beta Multithreaded): No best candidates found, returning default invalid move (fallback)." << std::endl;
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            long long duration_ms = duration_microseconds.count() / 1000;

            long long nodes_per_second = 0;
            if (duration_ms > 0) {
                nodes_per_second = (nodes_evaluated_count * 1000) / duration_ms;
            } else if (nodes_evaluated_count > 0) {
                nodes_per_second = nodes_evaluated_count * 1000000; 
            }

            std::cerr << "DEBUG: ChessAI (Alpha-Beta Cores Used: " << static_cast<int>(NUMBER_OF_CORES_USED) << "):" << std::endl;
            std::cerr << "Completed search to depth " << current_search_depth_set
                      << ". Nodes: " << nodes_evaluated_count
                      << ", Branches: " << branches_explored_count
                      << ", Time: " << duration_ms << "ms"
                      << ", NPS: " << nodes_per_second << std::endl;
            
            std::cerr << "DEBUG: ChessAI (Alpha-Beta Cores Used: " << static_cast<int>(NUMBER_OF_CORES_USED) << "): Chose move: "
                      << ChessBitboardUtils::move_to_string(final_chosen_move)
                      << " with score (current player's perspective at root): " << best_eval << std::endl;
            
            return final_chosen_move;
        }
        default:
            std::cerr << "DEBUG: ChessAI: Unknown AIType configured. Falling back to random." << std::endl;
            {
                std::uniform_int_distribution<size_t> dist(0, legal_moves.size() - 1);
                Move chosen_move = legal_moves[dist(rng_engine)];
                std::cerr << "DEBUG: ChessAI (Fallback from Unknown AIType): Chose move: " 
                          << ChessBitboardUtils::move_to_string(chosen_move) << std::endl;
                return chosen_move;
            }
    }
}
