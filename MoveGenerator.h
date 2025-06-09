#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <vector>   // For std::vector to store lists of moves
#include <array>    // For std::array for static delta lists
#include "ChessBoard.h" // To access the ChessBoard state
#include "Move.h"       // To define and use the Move struct
#include "Types.h"      // For PlayerColor, PieceTypeIndex etc.

// Define a maximum number of moves per position. While std::vector is used,
// this constant can still serve as a reference or for other optimizations.
const int MAX_MOVES = 256;

// The MoveGenerator struct is responsible for generating all legal moves
// for the current player on a given ChessBoard.
struct MoveGenerator {

    // Main function to generate all legal moves for the active player.
    // It takes a NON-CONST reference to the ChessBoard, allowing it to
    // apply and undo moves in-place for legality checking.
    std::vector<Move> generate_legal_moves(ChessBoard& board);

    // --- Helper functions for generating pseudo-legal moves for individual piece types ---
    // These functions take the current board, the square the piece is on,
    // and a reference to the vector where generated moves will be added.
    // `pseudo_legal_moves` contains moves that are valid according to piece movement rules,
    // but may leave the king in check.

    void generate_pawn_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);
    void generate_knight_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);
    void generate_bishop_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);
    void generate_rook_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);
    void generate_queen_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);
    void generate_king_moves(const ChessBoard& board, int square_idx, std::vector<Move>& pseudo_legal_moves);

private:
    // Helper for generating pseudo-legal sliding moves (Bishop, Rook, Queen).
    // It takes the specific delta array for the piece type.
    void generate_sliding_piece_moves_helper(const ChessBoard& board, int square_idx, PieceTypeIndex piece_type, std::vector<Move>& pseudo_legal_moves);

    // Helper to check if a square is attacked by a given color.
    // This helper works on a const board reference as it only queries attack state.
    bool is_square_attacked(int square_idx, PlayerColor attacking_color, const ChessBoard& board);

};

#endif // MOVE_GENERATOR_H
