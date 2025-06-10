#ifndef EVALUATION_H
#define EVALUATION_H

#include "ChessBoard.h"
#include "Types.h"
#include "ChessBitboardUtils.h"
#include "Constants.h"
#include "ChessAI.h"

#include <cstdint>
#include <array>

namespace Evaluation {

    int calculate_pawn_shield_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square, uint64_t friendly_pawns_bb);

    int calculate_open_file_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square, uint64_t friendly_pawns_bb, uint64_t enemy_pawns_bb);

    int evaluate(const ChessBoard& board);

} // namespace Evaluation

#endif // EVALUATION_H
