#include "MoveGenerator.h"
#include "ChessBitboardUtils.h"
#include "MagicTables.h"
#include <cmath>
#include <array>

bool MoveGenerator::is_square_attacked(int square_idx, PlayerColor attacking_color, const ChessBoard& board) {
    uint64_t enemy_pawns_bb = (attacking_color == PlayerColor::White) ? board.white_pawns : board.black_pawns;
    uint64_t enemy_knights_bb = (attacking_color == PlayerColor::White) ? board.white_knights : board.black_knights;
    uint64_t enemy_rooks_bb = (attacking_color == PlayerColor::White) ? board.white_rooks : board.black_rooks;
    uint64_t enemy_bishops_bb = (attacking_color == PlayerColor::White) ? board.white_bishops : board.black_bishops;
    uint64_t enemy_queens_bb = (attacking_color == PlayerColor::White) ? board.white_queens : board.black_queens;
    uint64_t enemy_king_bb = (attacking_color == PlayerColor::White) ? board.white_king : board.black_king;

    if (ChessBitboardUtils::is_pawn_attacked_by(square_idx, enemy_pawns_bb, attacking_color)) return true;

    if (ChessBitboardUtils::is_knight_attacked_by(square_idx, enemy_knights_bb)) return true;

    if (ChessBitboardUtils::is_king_attacked_by(square_idx, enemy_king_bb)) return true;

    uint64_t rook_queen_attackers = enemy_rooks_bb | enemy_queens_bb;
    if (ChessBitboardUtils::is_rook_queen_attacked_by(square_idx, rook_queen_attackers, board.occupied_squares)) return true;

    uint64_t bishop_queen_attackers = enemy_bishops_bb | enemy_queens_bb;
    
    if (ChessBitboardUtils::is_bishop_queen_attacked_by(square_idx, bishop_queen_attackers, board.occupied_squares)) return true;

    return false;
}


void MoveGenerator::generate_pawn_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint8_t rank = ChessBitboardUtils::square_to_rank(square_idx); 
    uint8_t file = ChessBitboardUtils::square_to_file(square_idx); 

    int forward_step = (color == PlayerColor::White) ? 8 : -8;
    uint8_t start_rank = (color == PlayerColor::White) ? 1 : 6;
    uint8_t promotion_rank = (color == PlayerColor::White) ? 7 : 0;

    uint64_t empty_squares = ~board.occupied_squares;

    int target_sq_single = square_idx + forward_step;
    if (target_sq_single >= 0 && target_sq_single < 64 && ChessBitboardUtils::test_bit(empty_squares, target_sq_single)) {
        if (ChessBitboardUtils::square_to_rank(target_sq_single) == promotion_rank) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true, PieceTypeIndex::QUEEN);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true, PieceTypeIndex::ROOK);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true, PieceTypeIndex::BISHOP);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true, PieceTypeIndex::KNIGHT);
        } else {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_single)}, PieceTypeIndex::PAWN);
        }

        if (rank == start_rank) {
            int target_sq_double = square_idx + 2 * forward_step;
            if (target_sq_double >= 0 && target_sq_double < 64 &&
                ChessBitboardUtils::test_bit(empty_squares, target_sq_double) &&
                ChessBitboardUtils::test_bit(empty_squares, target_sq_single)) {
                pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{file, ChessBitboardUtils::square_to_rank(target_sq_double)}, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, false, false, true);
            }
        }
    }

    int capture_left_delta = (color == PlayerColor::White) ? 7 : -9;
    int capture_right_delta = (color == PlayerColor::White) ? 9 : -7;

    uint64_t enemy_occupied_squares = (color == PlayerColor::White) ? board.black_occupied_squares : board.white_occupied_squares;

    int target_sq_cl = square_idx + capture_left_delta;
    bool is_valid_cl_move = (target_sq_cl >= 0 && target_sq_cl < 64) && 
                            (ChessBitboardUtils::square_to_file(target_sq_cl) == file - 1 || ChessBitboardUtils::square_to_file(target_sq_cl) == file + 1);


    if (is_valid_cl_move && ChessBitboardUtils::test_bit(enemy_occupied_squares, target_sq_cl)) {
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        if (color == PlayerColor::White) {
            if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq_cl)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq_cl)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq_cl)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq_cl)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq_cl)) captured_type = PieceTypeIndex::QUEEN;
        } else {
            if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq_cl)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq_cl)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq_cl)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq_cl)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq_cl)) captured_type = PieceTypeIndex::QUEEN;
        }

        if (ChessBitboardUtils::square_to_rank(target_sq_cl) == promotion_rank) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::QUEEN);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::ROOK);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::BISHOP);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::KNIGHT);
        } else {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cl), ChessBitboardUtils::square_to_rank(target_sq_cl)}, PieceTypeIndex::PAWN, captured_type);
        }
    }

    int target_sq_cr = square_idx + capture_right_delta;
    bool is_valid_cr_move = (target_sq_cr >= 0 && target_sq_cr < 64) && 
                            (ChessBitboardUtils::square_to_file(target_sq_cr) == file + 1 || ChessBitboardUtils::square_to_file(target_sq_cr) == file - 1); // Corrected this line to check +/- 1 file

    if (is_valid_cr_move && ChessBitboardUtils::test_bit(enemy_occupied_squares, target_sq_cr)) {
         PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        if (color == PlayerColor::White) {
            if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq_cr)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq_cr)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq_cr)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq_cr)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq_cr)) captured_type = PieceTypeIndex::QUEEN;
        } else {
            if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq_cr)) captured_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq_cr)) captured_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq_cr)) captured_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq_cr)) captured_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq_cr)) captured_type = PieceTypeIndex::QUEEN;
        }
        if (ChessBitboardUtils::square_to_rank(target_sq_cr) == promotion_rank) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::QUEEN);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::ROOK);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::BISHOP);
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type, true, PieceTypeIndex::KNIGHT);
        } else {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ChessBitboardUtils::square_to_file(target_sq_cr), ChessBitboardUtils::square_to_rank(target_sq_cr)}, PieceTypeIndex::PAWN, captured_type);
        }
    }

    if (board.en_passant_square_idx != 64) {
        uint8_t ep_file = ChessBitboardUtils::square_to_file(board.en_passant_square_idx); 
        uint8_t ep_rank = ChessBitboardUtils::square_to_rank(board.en_passant_square_idx); 
        
        if (file - 1 == ep_file && ((color == PlayerColor::White && rank == 4 && ep_rank == 5) || (color == PlayerColor::Black && rank == 3 && ep_rank == 2))) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ep_file, ep_rank}, PieceTypeIndex::PAWN, PieceTypeIndex::PAWN, false, PieceTypeIndex::NONE, false, false, true, false);
        }
        if (file + 1 == ep_file && ((color == PlayerColor::White && rank == 4 && ep_rank == 5) || (color == PlayerColor::Black && rank == 3 && ep_rank == 2))) {
            pseudo_legal_moves.emplace_back(GamePoint{file, rank}, GamePoint{ep_file, ep_rank}, PieceTypeIndex::PAWN, PieceTypeIndex::PAWN, false, PieceTypeIndex::NONE, false, false, true, false);
        }
    }
}

void MoveGenerator::generate_knight_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint64_t friendly_occupied_squares = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    uint64_t attacks = ChessBitboardUtils::knight_attacks[square_idx];
    
    attacks &= ~friendly_occupied_squares;

    while (attacks) {
        int target_sq = ChessBitboardUtils::pop_bit(attacks);
        
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        if (ChessBitboardUtils::test_bit(board.occupied_squares, target_sq)) {
            if (color == PlayerColor::White) {
                if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            } else {
                if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            }
        }
        
        pseudo_legal_moves.emplace_back(GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
                                         GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
                                         PieceTypeIndex::KNIGHT, captured_type);
    }
}

void MoveGenerator::generate_sliding_piece_moves_helper(const ChessBoard& board, int square_idx, PieceTypeIndex piece_type, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint64_t occupancy = board.occupied_squares;
    uint64_t attacks = 0ULL;

    switch (piece_type) {
        case PieceTypeIndex::ROOK:
            attacks = ChessBitboardUtils::get_rook_attacks(square_idx, occupancy);
            break;
        case PieceTypeIndex::BISHOP:
            attacks = ChessBitboardUtils::get_bishop_attacks(square_idx, occupancy);
            break;
        case PieceTypeIndex::QUEEN:
            attacks = ChessBitboardUtils::get_rook_attacks(square_idx, occupancy) | ChessBitboardUtils::get_bishop_attacks(square_idx, occupancy);
            break;
        default:
            return;
    }

    uint64_t friendly_occupied = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;
    attacks &= ~friendly_occupied;

    while (attacks) {
        int target_sq = ChessBitboardUtils::pop_bit(attacks);

        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        if (ChessBitboardUtils::test_bit(board.occupied_squares, target_sq)) {
            if (color == PlayerColor::White) {
                if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
                else if (ChessBitboardUtils::test_bit(board.black_king, target_sq)) captured_type = PieceTypeIndex::KING;
            } else {
                if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
                else if (ChessBitboardUtils::test_bit(board.white_king, target_sq)) captured_type = PieceTypeIndex::KING;
            }
        }

        pseudo_legal_moves.emplace_back(
            GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
            GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
            piece_type,
            captured_type
        );
    }
}

void MoveGenerator::generate_bishop_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::BISHOP, pseudo_legal_moves);
}

void MoveGenerator::generate_rook_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::ROOK, pseudo_legal_moves);
}

void MoveGenerator::generate_queen_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    generate_sliding_piece_moves_helper(board, square_idx, PieceTypeIndex::QUEEN, pseudo_legal_moves);
}

void MoveGenerator::generate_king_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves) {
    PlayerColor color = board.active_player;
    uint64_t friendly_occupied_squares = (color == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    uint64_t attacks = ChessBitboardUtils::king_attacks[square_idx];
    
    attacks &= ~friendly_occupied_squares;

    while (attacks) {
        int target_sq = ChessBitboardUtils::pop_bit(attacks);
        
        PieceTypeIndex captured_type = PieceTypeIndex::NONE;
        if (ChessBitboardUtils::test_bit(board.occupied_squares, target_sq)) {
            if (color == PlayerColor::White) {
                if (ChessBitboardUtils::test_bit(board.black_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.black_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.black_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.black_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.black_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            } else {
                if (ChessBitboardUtils::test_bit(board.white_pawns, target_sq)) captured_type = PieceTypeIndex::PAWN;
                else if (ChessBitboardUtils::test_bit(board.white_knights, target_sq)) captured_type = PieceTypeIndex::KNIGHT;
                else if (ChessBitboardUtils::test_bit(board.white_bishops, target_sq)) captured_type = PieceTypeIndex::BISHOP;
                else if (ChessBitboardUtils::test_bit(board.white_rooks, target_sq)) captured_type = PieceTypeIndex::ROOK;
                else if (ChessBitboardUtils::test_bit(board.white_queens, target_sq)) captured_type = PieceTypeIndex::QUEEN;
            }
        }
        
        pseudo_legal_moves.emplace_back(GamePoint{ChessBitboardUtils::square_to_file(square_idx), ChessBitboardUtils::square_to_rank(square_idx)},
                                         GamePoint{ChessBitboardUtils::square_to_file(target_sq), ChessBitboardUtils::square_to_rank(target_sq)},
                                         PieceTypeIndex::KING, captured_type);
    }

    uint8_t king_rank = ChessBitboardUtils::square_to_rank(square_idx); 
    uint8_t king_file = ChessBitboardUtils::square_to_file(square_idx); 
    PlayerColor enemy_color = (color == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;

    if (color == PlayerColor::White && square_idx == ChessBitboardUtils::E1_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WK_BIT)) {
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::F1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::G1_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::F1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::G1_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::G1_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::G1_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, true, false, false, false);
            }
        }
    }
    else if (color == PlayerColor::Black && square_idx == ChessBitboardUtils::E8_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BK_BIT)) {
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::F8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::G8_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::F8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::G8_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::G8_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::G8_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, true, false, false, false);
            }
        }
    }

    if (color == PlayerColor::White && square_idx == ChessBitboardUtils::E1_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_WQ_BIT)) {
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::D1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::C1_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::B1_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::D1_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::C1_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::C1_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::C1_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, true, false, false);
            }
        }
    }
    else if (color == PlayerColor::Black && square_idx == ChessBitboardUtils::E8_SQ && (board.castling_rights_mask & ChessBitboardUtils::CASTLE_BQ_BIT)) {
        if (!ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::D8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::C8_SQ) &&
            !ChessBitboardUtils::test_bit(board.occupied_squares, ChessBitboardUtils::B8_SQ)) {
            if (!is_square_attacked(ChessBitboardUtils::E8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::D8_SQ, enemy_color, board) &&
                !is_square_attacked(ChessBitboardUtils::C8_SQ, enemy_color, board)) {
                pseudo_legal_moves.emplace_back(GamePoint{king_file, king_rank}, GamePoint{ChessBitboardUtils::square_to_file(ChessBitboardUtils::C8_SQ), ChessBitboardUtils::square_to_rank(ChessBitboardUtils::C8_SQ)}, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, true, false, false);
            }
        }
    }
}


std::vector<Move> MoveGenerator::generate_legal_moves(ChessBoard& board) {
    std::vector<Move> pseudo_legal_moves;
    pseudo_legal_moves.reserve(MAX_MOVES); 
    std::vector<Move> legal_moves;
    
    PlayerColor current_player = board.active_player;
    uint64_t player_pieces_bb = (current_player == PlayerColor::White) ? board.white_occupied_squares : board.black_occupied_squares;

    uint64_t temp_player_pieces_bb = player_pieces_bb;
    while (temp_player_pieces_bb != 0ULL) {
        int square_idx = ChessBitboardUtils::pop_bit(temp_player_pieces_bb);

        PieceTypeIndex piece_type = PieceTypeIndex::NONE;
        if (current_player == PlayerColor::White) {
            if (ChessBitboardUtils::test_bit(board.white_pawns, square_idx)) piece_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.white_knights, square_idx)) piece_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.white_bishops, square_idx)) piece_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.white_rooks, square_idx)) piece_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.white_queens, square_idx)) piece_type = PieceTypeIndex::QUEEN;
            else if (ChessBitboardUtils::test_bit(board.white_king, square_idx)) piece_type = PieceTypeIndex::KING;
        } else {
            if (ChessBitboardUtils::test_bit(board.black_pawns, square_idx)) piece_type = PieceTypeIndex::PAWN;
            else if (ChessBitboardUtils::test_bit(board.black_knights, square_idx)) piece_type = PieceTypeIndex::KNIGHT;
            else if (ChessBitboardUtils::test_bit(board.black_bishops, square_idx)) piece_type = PieceTypeIndex::BISHOP;
            else if (ChessBitboardUtils::test_bit(board.black_rooks, square_idx)) piece_type = PieceTypeIndex::ROOK;
            else if (ChessBitboardUtils::test_bit(board.black_queens, square_idx)) piece_type = PieceTypeIndex::QUEEN;
            else if (ChessBitboardUtils::test_bit(board.black_king, square_idx)) piece_type = PieceTypeIndex::KING;
        }

        switch (piece_type) {
            case PieceTypeIndex::PAWN: generate_pawn_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::KNIGHT: generate_knight_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::BISHOP: generate_bishop_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::ROOK: generate_rook_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::QUEEN: generate_queen_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::KING: generate_king_moves(board, square_idx, pseudo_legal_moves); break;
            case PieceTypeIndex::NONE:
            default: break;
        }
    }

    for (const auto& move : pseudo_legal_moves) {
        StateInfo info_for_undo;
        
        board.apply_move(move, info_for_undo);

        if (!board.is_king_in_check(current_player)) {
            legal_moves.push_back(move);
        }
        
        board.undo_move(move, info_for_undo);
    }
    
    return legal_moves;
}
