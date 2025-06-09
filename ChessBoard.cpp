#include "ChessBoard.h"
#include "ChessBitboardUtils.h"
#include "Types.h"
#include "Move.h"
#include <random>
//#include <chrono>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iostream>

uint64_t ChessBoard::zobrist_piece_keys[12][64];
uint64_t ChessBoard::zobrist_black_to_move_key;
uint64_t ChessBoard::zobrist_castling_keys[16];
uint64_t ChessBoard::zobrist_en_passant_keys[8];
bool ChessBoard::zobrist_keys_initialized = false;

ChessBoard::ChessBoard() {
    if (!zobrist_keys_initialized) {
        initialize_zobrist_keys();
        zobrist_keys_initialized = true;
    }
    reset_to_start_position();
}

ChessBoard::ChessBoard(const std::string& fen) {
    if (!zobrist_keys_initialized) {
        initialize_zobrist_keys();
        zobrist_keys_initialized = true;
    }
    set_from_fen(fen);
}

void ChessBoard::initialize_zobrist_keys() {
    std::mt19937_64 rng(std::hash<std::string>{}("Carolyna is where my mind rests!"));
    std::uniform_int_distribution<uint64_t> dist;

    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 64; ++j) {
            zobrist_piece_keys[i][j] = dist(rng);
        }
    }

    zobrist_black_to_move_key = dist(rng);

    for (int i = 0; i < 16; ++i) {
        zobrist_castling_keys[i] = dist(rng);
    }

    for (int i = 0; i < 8; ++i) {
        zobrist_en_passant_keys[i] = dist(rng);
    }
}

void ChessBoard::reset_to_start_position() {
    white_pawns = 0ULL;
    white_knights = 0ULL;
    white_bishops = 0ULL;
    white_rooks = 0ULL;
    white_queens = 0ULL;
    white_king = 0ULL;

    black_pawns = 0ULL;
    black_knights = 0ULL;
    black_bishops = 0ULL;
    black_rooks = 0ULL;
    black_queens = 0ULL;
    black_king = 0ULL;

    white_pawns = ChessBitboardUtils::RANK_2;
    white_rooks = ChessBitboardUtils::A1_SQ_BB | ChessBitboardUtils::H1_SQ_BB;
    white_knights = ChessBitboardUtils::B1_SQ_BB | ChessBitboardUtils::G1_SQ_BB;
    white_bishops = ChessBitboardUtils::C1_SQ_BB | ChessBitboardUtils::F1_SQ_BB;
    white_queens = ChessBitboardUtils::D1_SQ_BB;
    white_king = ChessBitboardUtils::E1_SQ_BB;

    black_pawns = ChessBitboardUtils::RANK_7;
    black_rooks = ChessBitboardUtils::A8_SQ_BB | ChessBitboardUtils::H8_SQ_BB;
    black_knights = ChessBitboardUtils::B8_SQ_BB | ChessBitboardUtils::G8_SQ_BB;
    black_bishops = ChessBitboardUtils::C8_SQ_BB | ChessBitboardUtils::F8_SQ_BB;
    black_queens = ChessBitboardUtils::D8_SQ_BB;
    black_king = ChessBitboardUtils::E8_SQ_BB;

    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    active_player = PlayerColor::White;
    castling_rights_mask = ChessBitboardUtils::CASTLE_WK_BIT | ChessBitboardUtils::CASTLE_WQ_BIT |
                           ChessBitboardUtils::CASTLE_BK_BIT | ChessBitboardUtils::CASTLE_BQ_BIT;
    en_passant_square_idx = 64;
    halfmove_clock = 0;
    fullmove_number = 1;

    zobrist_hash = calculate_zobrist_hash_from_scratch();
}

void ChessBoard::set_from_fen(const std::string& fen) {
    white_pawns = white_knights = white_bishops = white_rooks = white_queens = white_king = 0ULL;
    black_pawns = black_knights = black_bishops = black_rooks = black_queens = black_king = 0ULL;

    std::stringstream ss(fen);
    std::string board_part, active_color_part, castling_part, en_passant_part, halfmove_part, fullmove_part;

    ss >> board_part >> active_color_part >> castling_part >> en_passant_part >> halfmove_part >> fullmove_part;

    int file = 0;
    int rank = 7;

    for (char c : board_part) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += (c - '0');
        } else {
            int square_idx = ChessBitboardUtils::rank_file_to_square(rank, file);

            switch (c) {
                case 'P': ChessBitboardUtils::set_bit(white_pawns, square_idx); break;
                case 'N': ChessBitboardUtils::set_bit(white_knights, square_idx); break;
                case 'B': ChessBitboardUtils::set_bit(white_bishops, square_idx); break;
                case 'R': ChessBitboardUtils::set_bit(white_rooks, square_idx); break;
                case 'Q': ChessBitboardUtils::set_bit(white_queens, square_idx); break;
                case 'K': ChessBitboardUtils::set_bit(white_king, square_idx); break;
                case 'p': ChessBitboardUtils::set_bit(black_pawns, square_idx); break;
                case 'n': ChessBitboardUtils::set_bit(black_knights, square_idx); break;
                case 'b': ChessBitboardUtils::set_bit(black_bishops, square_idx); break;
                case 'r': ChessBitboardUtils::set_bit(black_rooks, square_idx); break;
                case 'q': ChessBitboardUtils::set_bit(black_queens, square_idx); break;
                case 'k': ChessBitboardUtils::set_bit(black_king, square_idx); break;
                default:
                    return;
            }
            file++;
        }
    }

    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    active_player = (active_color_part == "w") ? PlayerColor::White : PlayerColor::Black;

    castling_rights_mask = 0;
    if (castling_part.find('K') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_WK_BIT;
    if (castling_part.find('Q') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_WQ_BIT;
    if (castling_part.find('k') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_BK_BIT;
    if (castling_part.find('q') != std::string::npos) castling_rights_mask |= ChessBitboardUtils::CASTLE_BQ_BIT;

    if (en_passant_part == "-") {
        en_passant_square_idx = 64;
    } else {
        int file_char_idx = en_passant_part[0] - 'a';
        int rank_char_idx = en_passant_part[1] - '1';
        en_passant_square_idx = ChessBitboardUtils::rank_file_to_square(rank_char_idx, file_char_idx);
    }

    halfmove_clock = std::stoi(halfmove_part);

    fullmove_number = std::stoi(fullmove_part);

    zobrist_hash = calculate_zobrist_hash_from_scratch();

    // std::cerr << "DEBUG: ChessBoard::set_from_fen completed. FEN: " << to_fen() << std::endl;
    // std::cerr << "DEBUG: ChessBoard::set_from_fen Zobrist Hash: " << zobrist_hash << std::endl;
}

std::string ChessBoard::to_fen() const {
    std::string fen_pieces = "";

    for (int rank = 7; rank >= 0; --rank) {
        int empty_count = 0;
        for (int file = 0; file < 8; ++file) {
            int square_idx = ChessBitboardUtils::rank_file_to_square(rank, file);

            char piece_char = ' ';
            if (ChessBitboardUtils::test_bit(white_pawns, square_idx)) piece_char = 'P';
            else if (ChessBitboardUtils::test_bit(white_knights, square_idx)) piece_char = 'N';
            else if (ChessBitboardUtils::test_bit(white_bishops, square_idx)) piece_char = 'B';
            else if (ChessBitboardUtils::test_bit(white_rooks, square_idx)) piece_char = 'R';
            else if (ChessBitboardUtils::test_bit(white_queens, square_idx)) piece_char = 'Q';
            else if (ChessBitboardUtils::test_bit(white_king, square_idx)) piece_char = 'K';
            else if (ChessBitboardUtils::test_bit(black_pawns, square_idx)) piece_char = 'p';
            else if (ChessBitboardUtils::test_bit(black_knights, square_idx)) piece_char = 'n';
            else if (ChessBitboardUtils::test_bit(black_bishops, square_idx)) piece_char = 'b';
            else if (ChessBitboardUtils::test_bit(black_rooks, square_idx)) piece_char = 'r';
            else if (ChessBitboardUtils::test_bit(black_queens, square_idx)) piece_char = 'q';
            else if (ChessBitboardUtils::test_bit(black_king, square_idx)) piece_char = 'k';

            if (piece_char != ' ') {
                if (empty_count > 0) {
                    fen_pieces += std::to_string(empty_count);
                    empty_count = 0;
                }
                fen_pieces += piece_char;
            } else {
                empty_count++;
            }
        }
        if (empty_count > 0) {
            fen_pieces += std::to_string(empty_count);
        }
        if (rank > 0) {
            fen_pieces += '/';
        }
    }

    std::string fen_full = fen_pieces;

    fen_full += (active_player == PlayerColor::White) ? " w" : " b";

    std::string castling_str = "";
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_WK_BIT) castling_str += 'K';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_WQ_BIT) castling_str += 'Q';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_BK_BIT) castling_str += 'k';
    if (castling_rights_mask & ChessBitboardUtils::CASTLE_BQ_BIT) castling_str += 'q';
    if (castling_str.empty()) castling_str = "-";
    fen_full += " " + castling_str;

    if (en_passant_square_idx >= 0 && en_passant_square_idx < 64) {
        char file_char = 'a' + ChessBitboardUtils::square_to_file(en_passant_square_idx);
        char rank_char = '1' + ChessBitboardUtils::square_to_rank(en_passant_square_idx);
        fen_full += " " + std::string(1, file_char) + std::string(1, rank_char);
    } else {
        fen_full += " -";
    }

    fen_full += " " + std::to_string(halfmove_clock);

    fen_full += " " + std::to_string(fullmove_number);

    return fen_full;
}

void ChessBoard::apply_move(const Move& move, StateInfo& state_info) {
    // std::cerr << "DEBUG: ChessBoard::apply_move BEFORE move " << ChessBitboardUtils::move_to_string(move) << std::endl;
    // std::cerr << "DEBUG:   Current FEN: " << to_fen() << std::endl;
    // std::cerr << "DEBUG:   Current Zobrist Hash: " << zobrist_hash << std::endl;
    // std::cerr << "DEBUG:   Active Player (Before Move): " << ((active_player == PlayerColor::White) ? "White" : "Black") << std::endl;

    // 1. Save current board state into `state_info` for `undo_move`.
    state_info.previous_castling_rights_mask = castling_rights_mask;
    state_info.previous_en_passant_square_idx = en_passant_square_idx;
    state_info.previous_halfmove_clock = halfmove_clock;
    state_info.previous_fullmove_number = fullmove_number;
    state_info.previous_active_player = active_player;
    state_info.captured_piece_type_idx = PieceTypeIndex::NONE;
    state_info.captured_piece_color = PlayerColor::White;
    state_info.captured_square_idx = 64;

    // 2. Convert GamePoint (file, rank) from the `Move` struct to internal 0-63 square indices.
    int from_sq = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
    int to_sq = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);

    // std::cerr << "DEBUG:   Move Details: from=" << ChessBitboardUtils::square_to_string(from_sq)
    //           << ", to=" << ChessBitboardUtils::square_to_string(to_sq)
    //           << ", piece_moved=" << static_cast<int>(move.piece_moved_type_idx)
    //           << ", is_capture=" << move.is_capture
    //           << ", captured_type=" << static_cast<int>(move.piece_captured_type_idx)
    //           << ", is_promo=" << move.is_promotion
    //           << ", promo_type=" << static_cast<int>(move.promotion_piece_type_idx)
    //           << ", is_ks_castle=" << move.is_kingside_castle
    //           << ", is_qs_castle=" << move.is_queenside_castle
    //           << ", is_ep=" << move.is_en_passant
    //           << ", is_double_push=" << move.is_double_pawn_push << std::endl;


    // 3. Update Halfmove Clock and Fullmove Number.
    if (move.piece_moved_type_idx == PieceTypeIndex::PAWN || move.piece_captured_type_idx != PieceTypeIndex::NONE) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }
    if (active_player == PlayerColor::Black) {
        fullmove_number++;
    }

    // 4. Update Zobrist Hash for the side to move (always toggle).
    zobrist_hash ^= zobrist_black_to_move_key;

    // 5. Update Zobrist Hash for En Passant Target (XOR out old target if it existed).
    if (en_passant_square_idx != 64) {
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }
    en_passant_square_idx = 64;

    // 6. Get moving piece bitboard pointer.
    uint64_t* moving_piece_bb_ptr = nullptr;
    if (active_player == PlayerColor::White) {
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &white_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &white_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &white_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &white_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &white_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &white_king; break;
            default: return;
        }
    } else {
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &black_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &black_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &black_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &black_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &black_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &black_king; break;
            default: return;
        }
    }

    if (moving_piece_bb_ptr == nullptr) {
        return;
    }

    // 7. Handle Captures FIRST
    if (move.piece_captured_type_idx != PieceTypeIndex::NONE) {
        state_info.captured_piece_type_idx = move.piece_captured_type_idx;
        state_info.captured_piece_color = (active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
        
        int captured_sq = to_sq;

        uint64_t* captured_piece_bb_ptr = nullptr;
        if (state_info.captured_piece_color == PlayerColor::White) {
            switch (move.piece_captured_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &white_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &white_queens; break;
                default: return;
            }
        } else {
            switch (move.piece_captured_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &black_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &black_queens; break;
                default: return;
            }
        }
        
        if (captured_piece_bb_ptr == nullptr) {
            return;
        }

        if (move.is_en_passant) {
            if (active_player == PlayerColor::White) {
                captured_sq = to_sq - 8;
            } else {
                captured_sq = to_sq + 8;
            }
        }
        state_info.captured_square_idx = captured_sq;

        // std::cerr << "DEBUG:   BITBOARD: Before clearing captured piece " << static_cast<int>(move.piece_captured_type_idx)
        //           << " at " << ChessBitboardUtils::square_to_string(captured_sq)
        //           << ". Captured BB: " << std::hex << *captured_piece_bb_ptr << std::dec << std::endl;
        
        ChessBitboardUtils::clear_bit(*captured_piece_bb_ptr, captured_sq);
        toggle_zobrist_piece(move.piece_captured_type_idx, state_info.captured_piece_color, captured_sq);
        
        // std::cerr << "DEBUG:   BITBOARD: After clearing captured piece. Captured BB: " << std::hex << *captured_piece_bb_ptr << std::dec << std::endl;
        // std::cerr << "DEBUG:   FEN after capture (" << ChessBitboardUtils::square_to_string(captured_sq) << "): " << to_fen() << std::endl;
        // std::cerr << "DEBUG:   Zobrist Hash after capture: " << zobrist_hash << std::endl;
    }


    // 8. Move the piece on bitboards and update its Zobrist hash.
    // std::cerr << "DEBUG:   BITBOARD: Before moving piece " << static_cast<int>(move.piece_moved_type_idx)
    //           << " from " << ChessBitboardUtils::square_to_string(from_sq)
    //           << " to " << ChessBitboardUtils::square_to_string(to_sq)
    //           << ". Moving BB: " << std::hex << *moving_piece_bb_ptr << std::dec << std::endl;

    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, from_sq);
    ChessBitboardUtils::clear_bit(*moving_piece_bb_ptr, from_sq);
    ChessBitboardUtils::set_bit(*moving_piece_bb_ptr, to_sq);
    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq);

    // std::cerr << "DEBUG:   BITBOARD: After moving piece. Moving BB: " << std::hex << *moving_piece_bb_ptr << std::dec << std::endl;
    // std::cerr << "DEBUG:   FEN after piece moved (" << ChessBitboardUtils::square_to_string(from_sq) << " to " << ChessBitboardUtils::square_to_string(to_sq) << "): " << to_fen() << std::endl;
    // std::cerr << "DEBUG:   Zobrist Hash after piece move: " << zobrist_hash << std::endl;


    // 9. Handle Castling (King and Rook move together).
    if (move.is_kingside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::H1_SQ;
            rook_to_sq = ChessBitboardUtils::F1_SQ;
            rook_bb_ptr = &white_rooks;
        } else {
            rook_from_sq = ChessBitboardUtils::H8_SQ;
            rook_to_sq = ChessBitboardUtils::F8_SQ;
            rook_bb_ptr = &black_rooks;
        }

        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_from_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_to_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);

        // std::cerr << "DEBUG:   FEN after kingside castling rook move: " << to_fen() << std::endl;
        // std::cerr << "DEBUG:   Zobrist Hash after kingside castling rook move: " << zobrist_hash << std::endl;

    } else if (move.is_queenside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::A1_SQ;
            rook_to_sq = ChessBitboardUtils::D1_SQ;
            rook_bb_ptr = &white_rooks;
        } else {
            rook_from_sq = ChessBitboardUtils::A8_SQ;
            rook_to_sq = ChessBitboardUtils::D8_SQ;
            rook_bb_ptr = &black_rooks;
        }

        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_from_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_to_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);

        // std::cerr << "DEBUG:   FEN after queenside castling rook move: " << to_fen() << std::endl;
        // std::cerr << "DEBUG:   Zobrist Hash after queenside castling rook move: " << zobrist_hash << std::endl;
    }

    // 10. Handle Pawn Promotion.
    if (move.is_promotion) {
        toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq);
        
        uint64_t* pawn_bb_ptr = (active_player == PlayerColor::White) ? &white_pawns : &black_pawns;
        ChessBitboardUtils::clear_bit(*pawn_bb_ptr, to_sq);
        
        uint64_t* promoted_bb_ptr = nullptr;
        if (active_player == PlayerColor::White) {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &white_queens; break;
                default: return;
            }
        } else {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &black_queens; break;
                default: return;
            }
        }

        if (promoted_bb_ptr == nullptr) {
            return;
        }

        ChessBitboardUtils::set_bit(*promoted_bb_ptr, to_sq);
        toggle_zobrist_piece(move.promotion_piece_type_idx, active_player, to_sq);

        // std::cerr << "DEBUG:   FEN after promotion to " << static_cast<int>(move.promotion_piece_type_idx) << ": " << to_fen() << std::endl;
        // std::cerr << "DEBUG:   Zobrist Hash after promotion: " << zobrist_hash << std::endl;
    }

    // 11. Update Castling Rights Mask and its Zobrist hash.
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask]; // XOR out old mask hash

    if (move.piece_moved_type_idx == PieceTypeIndex::KING) {
        if (active_player == PlayerColor::White) {
            castling_rights_mask &= ~(ChessBitboardUtils::CASTLE_WK_BIT | ChessBitboardUtils::CASTLE_WQ_BIT);
        } else {
            castling_rights_mask &= ~(ChessBitboardUtils::CASTLE_BK_BIT | ChessBitboardUtils::CASTLE_BQ_BIT);
        }
    }
    if (move.piece_moved_type_idx == PieceTypeIndex::ROOK) {
        if (from_sq == ChessBitboardUtils::A1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WQ_BIT;
        if (from_sq == ChessBitboardUtils::H1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WK_BIT;
        if (from_sq == ChessBitboardUtils::A8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BQ_BIT;
        if (from_sq == ChessBitboardUtils::H8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BK_BIT;
    }
    if (move.piece_captured_type_idx == PieceTypeIndex::ROOK) {
        if (to_sq == ChessBitboardUtils::A1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WQ_BIT;
        if (to_sq == ChessBitboardUtils::H1_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_WK_BIT;
        if (to_sq == ChessBitboardUtils::A8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BQ_BIT;
        if (to_sq == ChessBitboardUtils::H8_SQ) castling_rights_mask &= ~ChessBitboardUtils::CASTLE_BK_BIT;
    }
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask]; // XOR in new mask hash


    // 12. Set new En Passant Target Square (if the move was a double pawn push).
    if (move.is_double_pawn_push) {
        if (active_player == PlayerColor::White) {
            en_passant_square_idx = to_sq - 8;
        } else {
            en_passant_square_idx = to_sq + 8;
        }
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    // 13. Update Occupancy Bitboards.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    // std::cerr << "DEBUG:   Active Player (Before Flip): " << ((active_player == PlayerColor::White) ? "White" : "Black") << std::endl;

    // 14. Toggle Active Player for the next turn.
    active_player = (active_player == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
    
    // std::cerr << "DEBUG: ChessBoard::apply_move AFTER move " << ChessBitboardUtils::move_to_string(move) << " fully applied." << std::endl;
    // std::cerr << "DEBUG:   Final FEN: " << to_fen() << std::endl;
    // std::cerr << "DEBUG:   Final Zobrist Hash: " << zobrist_hash << std::endl;
    // std::cerr << "DEBUG:   Active Player (After Flip): " << ((active_player == PlayerColor::White) ? "White" : "Black") << std::endl;
}

void ChessBoard::undo_move(const Move& move, const StateInfo& state_info) {
    // std::cerr << "DEBUG: ChessBoard::undo_move BEFORE undoing move " << ChessBitboardUtils::move_to_string(move) << std::endl;
    // std::cerr << "DEBUG:   Current FEN: " << to_fen() << std::endl;
    // std::cerr << "DEBUG:   Current Zobrist Hash: " << zobrist_hash << std::endl;
    // std::cerr << "DEBUG:   Active Player (Before Undo): " << ((active_player == PlayerColor::White) ? "White" : "Black") << std::endl;

    // 1. Convert GamePoint to internal square index.
    int from_sq = ChessBitboardUtils::rank_file_to_square(move.from_square.y, move.from_square.x);
    int to_sq = ChessBitboardUtils::rank_file_to_square(move.to_square.y, move.to_square.x);

    // 2. Restore active player first.
    active_player = state_info.previous_active_player;
    zobrist_hash ^= zobrist_black_to_move_key;
    // std::cerr << "DEBUG:   Active Player (After Restore): " << ((active_player == PlayerColor::White) ? "White" : "Black") << std::endl;


    // 3. Reverse En Passant hash update.
    if (en_passant_square_idx != 64) {
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }
    en_passant_square_idx = state_info.previous_en_passant_square_idx;
    if (en_passant_square_idx != 64) {
        zobrist_hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    // 4. Reverse Castling Rights hash update.
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask]; // XOR out current hash
    castling_rights_mask = state_info.previous_castling_rights_mask;
    zobrist_hash ^= zobrist_castling_keys[castling_rights_mask]; // XOR in previous hash


    // 5. Undo Pawn Promotion.
    if (move.is_promotion) {
        toggle_zobrist_piece(move.promotion_piece_type_idx, active_player, to_sq);
        
        uint64_t* promoted_bb_ptr = nullptr;
        
        if (active_player == PlayerColor::White) {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &white_queens; break;
                default: break;
            }
            if (promoted_bb_ptr) {
                ChessBitboardUtils::clear_bit(*promoted_bb_ptr, to_sq);
            }
            ChessBitboardUtils::set_bit(white_pawns, to_sq);
        } else {
            switch (move.promotion_piece_type_idx) {
                case PieceTypeIndex::KNIGHT: promoted_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: promoted_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: promoted_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: promoted_bb_ptr = &black_queens; break;
                default: break;
            }
            if (promoted_bb_ptr) {
                ChessBitboardUtils::clear_bit(*promoted_bb_ptr, to_sq);
            }
            ChessBitboardUtils::set_bit(black_pawns, to_sq);
        }
        toggle_zobrist_piece(PieceTypeIndex::PAWN, active_player, to_sq);
    }

    // 6. Undo Castling Rook Move.
    if (move.is_kingside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::H1_SQ;
            rook_to_sq = ChessBitboardUtils::F1_SQ;
            rook_bb_ptr = &white_rooks;
        } else {
            rook_from_sq = ChessBitboardUtils::H8_SQ;
            rook_to_sq = ChessBitboardUtils::F8_SQ;
            rook_bb_ptr = &black_rooks;
        }

        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_to_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_from_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
    } else if (move.is_queenside_castle) {
        int rook_from_sq, rook_to_sq;
        uint64_t* rook_bb_ptr;
        if (active_player == PlayerColor::White) {
            rook_from_sq = ChessBitboardUtils::A1_SQ;
            rook_to_sq = ChessBitboardUtils::D1_SQ;
            rook_bb_ptr = &white_rooks;
        } else {
            rook_from_sq = ChessBitboardUtils::A8_SQ;
            rook_to_sq = ChessBitboardUtils::D8_SQ;
            rook_bb_ptr = &black_rooks;
        }
        
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_to_sq);
        ChessBitboardUtils::clear_bit(*rook_bb_ptr, rook_to_sq);
        ChessBitboardUtils::set_bit(*rook_bb_ptr, rook_from_sq);
        toggle_zobrist_piece(PieceTypeIndex::ROOK, active_player, rook_from_sq);
    }

    // 7. Move the piece back.
    uint64_t* moving_piece_bb_ptr = nullptr;
    if (active_player == PlayerColor::White) {
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &white_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &white_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &white_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &white_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &white_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &white_king; break;
            default: return;
        }
    } else {
        switch (move.piece_moved_type_idx) {
            case PieceTypeIndex::PAWN: moving_piece_bb_ptr = &black_pawns; break;
            case PieceTypeIndex::KNIGHT: moving_piece_bb_ptr = &black_knights; break;
            case PieceTypeIndex::BISHOP: moving_piece_bb_ptr = &black_bishops; break;
            case PieceTypeIndex::ROOK: moving_piece_bb_ptr = &black_rooks; break;
            case PieceTypeIndex::QUEEN: moving_piece_bb_ptr = &black_queens; break;
            case PieceTypeIndex::KING: moving_piece_bb_ptr = &black_king; break;
            default: return;
        }
    }

    if (moving_piece_bb_ptr == nullptr) {
        return;
    }

    // std::cerr << "DEBUG:   BITBOARD: Before moving piece back " << static_cast<int>(move.piece_moved_type_idx)
    //           << " from " << ChessBitboardUtils::square_to_string(to_sq)
    //           << " to " << ChessBitboardUtils::square_to_string(from_sq)
    //           << ". Moving BB: " << std::hex << *moving_piece_bb_ptr << std::dec << std::endl;

    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, to_sq);
    ChessBitboardUtils::clear_bit(*moving_piece_bb_ptr, to_sq);
    ChessBitboardUtils::set_bit(*moving_piece_bb_ptr, from_sq);
    toggle_zobrist_piece(move.piece_moved_type_idx, active_player, from_sq);

    // std::cerr << "DEBUG:   BITBOARD: After moving piece back. Moving BB: " << std::hex << *moving_piece_bb_ptr << std::dec << std::endl;


    // 8. Restore Captured Piece.
    if (state_info.captured_piece_type_idx != PieceTypeIndex::NONE) {
        uint64_t* captured_piece_bb_ptr = nullptr;
        if (state_info.captured_piece_color == PlayerColor::White) {
            switch (state_info.captured_piece_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &white_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &white_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &white_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &white_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &white_queens; break;
                default: break;
            }
        } else {
            switch (state_info.captured_piece_type_idx) {
                case PieceTypeIndex::PAWN: captured_piece_bb_ptr = &black_pawns; break;
                case PieceTypeIndex::KNIGHT: captured_piece_bb_ptr = &black_knights; break;
                case PieceTypeIndex::BISHOP: captured_piece_bb_ptr = &black_bishops; break;
                case PieceTypeIndex::ROOK: captured_piece_bb_ptr = &black_rooks; break;
                case PieceTypeIndex::QUEEN: captured_piece_bb_ptr = &black_queens; break;
                default: break;
            }
        }

        if (captured_piece_bb_ptr) {
            // std::cerr << "DEBUG:   BITBOARD: Before restoring captured piece " << static_cast<int>(state_info.captured_piece_type_idx)
            //           << " at " << ChessBitboardUtils::square_to_string(state_info.captured_square_idx)
            //           << ". Captured BB: " << std::hex << *captured_piece_bb_ptr << std::dec << std::endl;

            ChessBitboardUtils::set_bit(*captured_piece_bb_ptr, state_info.captured_square_idx);
            toggle_zobrist_piece(state_info.captured_piece_type_idx, state_info.captured_piece_color, state_info.captured_square_idx);
            
            // std::cerr << "DEBUG:   BITBOARD: After restoring captured piece. Captured BB: " << std::hex << *captured_piece_bb_ptr << std::dec << std::endl;
        }
    }

    // 9. Restore Halfmove Clock and Fullmove Number.
    halfmove_clock = state_info.previous_halfmove_clock;
    fullmove_number = state_info.previous_fullmove_number;

    // 10. Update Occupancy Bitboards.
    white_occupied_squares = white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king;
    black_occupied_squares = black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;
    occupied_squares = white_occupied_squares | black_occupied_squares;

    // std::cerr << "DEBUG: undo_move AFTER: FEN: " << to_fen()
    //           << ", Zobrist: " << zobrist_hash
    //           << ", Castling: " << static_cast<int>(castling_rights_mask)
    //           << ", EnPassant: " << en_passant_square_idx << std::endl;
}

bool ChessBoard::is_king_in_check(PlayerColor king_color) const {
    uint64_t king_bitboard = (king_color == PlayerColor::White) ? white_king : black_king;
    if (king_bitboard == 0ULL) {
        return false;
    }

    int king_sq = ChessBitboardUtils::get_lsb_index(king_bitboard);

    PlayerColor attacking_color = (king_color == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;

    uint64_t enemy_pawns_bb = (attacking_color == PlayerColor::White) ? white_pawns : black_pawns;
    if (ChessBitboardUtils::is_pawn_attacked_by(king_sq, enemy_pawns_bb, attacking_color)) {
        return true;
    }

    uint64_t enemy_knights_bb = (attacking_color == PlayerColor::White) ? white_knights : black_knights;
    if (ChessBitboardUtils::is_knight_attacked_by(king_sq, enemy_knights_bb)) {
        return true;
    }

    uint64_t enemy_king_bb = (attacking_color == PlayerColor::White) ? white_king : black_king;
    if (ChessBitboardUtils::is_king_attacked_by(king_sq, enemy_king_bb)) {
        return true;
    }

    uint64_t enemy_rooks_bb = (attacking_color == PlayerColor::White) ? white_rooks : black_rooks;
    uint64_t enemy_queens_bb = (attacking_color == PlayerColor::White) ? white_queens : black_queens;
    uint64_t rook_queen_attackers = enemy_rooks_bb | enemy_queens_bb;
    
    if (ChessBitboardUtils::is_rook_queen_attacked_by(king_sq, rook_queen_attackers, occupied_squares)) {
        return true;
    }

    uint64_t enemy_bishops_bb = (attacking_color == PlayerColor::White) ? white_bishops : black_bishops;
    uint64_t bishop_queen_attackers = enemy_bishops_bb | enemy_queens_bb;
    
    if (ChessBitboardUtils::is_bishop_queen_attacked_by(king_sq, bishop_queen_attackers, occupied_squares)) {
        return true;
    }

    return false;
}

int ChessBoard::get_piece_square_index(PieceTypeIndex piece_type_idx, PlayerColor piece_color) const {
    uint64_t target_bb = 0ULL;

    if (piece_color == PlayerColor::White) {
        switch (piece_type_idx) {
            case PieceTypeIndex::PAWN: target_bb = white_pawns; break;
            case PieceTypeIndex::KNIGHT: target_bb = white_knights; break;
            case PieceTypeIndex::BISHOP: target_bb = white_bishops; break;
            case PieceTypeIndex::ROOK: target_bb = white_rooks; break;
            case PieceTypeIndex::QUEEN: target_bb = white_queens; break;
            case PieceTypeIndex::KING: target_bb = white_king; break;
            case PieceTypeIndex::NONE: return 64;
        }
    } else {
        switch (piece_type_idx) {
            case PieceTypeIndex::PAWN: target_bb = black_pawns; break;
            case PieceTypeIndex::KNIGHT: target_bb = black_knights; break;
            case PieceTypeIndex::BISHOP: target_bb = black_bishops; break;
            case PieceTypeIndex::ROOK: target_bb = black_rooks; break;
            case PieceTypeIndex::QUEEN: target_bb = black_queens; break;
            case PieceTypeIndex::KING: target_bb = black_king; break;
            case PieceTypeIndex::NONE: return 64;
        }
    }

    if (target_bb != 0ULL) {
        return ChessBitboardUtils::get_lsb_index(target_bb);
    }
    return 64;
}

uint64_t ChessBoard::calculate_zobrist_hash_from_scratch() const {
    uint64_t hash = 0ULL;

    for (int square_idx = 0; square_idx < 64; ++square_idx) {
        if (ChessBitboardUtils::test_bit(white_pawns, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::PAWN) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_knights, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KNIGHT) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_bishops, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::BISHOP) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_rooks, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::ROOK) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_queens, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::QUEEN) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(white_king, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KING) + 0][square_idx];
        else if (ChessBitboardUtils::test_bit(black_pawns, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::PAWN) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_knights, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KNIGHT) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_bishops, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::BISHOP) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_rooks, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::ROOK) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_queens, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::QUEEN) + 6][square_idx];
        else if (ChessBitboardUtils::test_bit(black_king, square_idx)) hash ^= zobrist_piece_keys[static_cast<int>(PieceTypeIndex::KING) + 6][square_idx];
    }

    if (active_player == PlayerColor::Black) {
        hash ^= zobrist_black_to_move_key;
    }

    hash ^= zobrist_castling_keys[castling_rights_mask];

    if (en_passant_square_idx != 64) {
        hash ^= zobrist_en_passant_keys[ChessBitboardUtils::square_to_file(en_passant_square_idx)];
    }

    return hash;
}

void ChessBoard::toggle_zobrist_piece(PieceTypeIndex piece_type_idx, PlayerColor piece_color, int square_idx) {
    int zobrist_index = static_cast<int>(piece_type_idx) + (piece_color == PlayerColor::White ? 0 : 6);
    zobrist_hash ^= zobrist_piece_keys[zobrist_index][square_idx];
}
