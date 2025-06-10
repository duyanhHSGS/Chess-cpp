#ifndef CHESS_AI_H
#define CHESS_AI_H

#include "ChessBoard.h"
#include "MoveGenerator.h"
#include "Constants.h"
#include "Move.h"
#include "Types.h"
#include <vector>
#include <cstdint>
#include <array> // Still needed for FILE_MASKS_LOCAL in ChessAI.cpp, though not directly in ChessAI.h

struct ChessAI {
	MoveGenerator move_gen;

	unsigned long long nodes_evaluated_count;
	unsigned long long branches_explored_count;
	int current_search_depth_set;

	static constexpr size_t TT_SIZE = 1048576;
	static constexpr int MATE_VALUE = 30000;
    static constexpr int MAX_PLY = 64; // Max search ply (corresponds to max depth)

	struct TTEntry {
		uint64_t hash;
		int score;
		int depth;
		NodeType flag;
		Move best_move;

        TTEntry() :
            hash(0ULL),
            score(0),
            depth(0),
            flag(NodeType::EXACT),
            best_move({0,0}, {0,0}, PieceTypeIndex::NONE)
        {}
	};
	std::vector<TTEntry> transposition_table;

	static constexpr int PAWN_VALUE   = 100;
	static constexpr int KNIGHT_VALUE = 320;
	static constexpr int BISHOP_VALUE = 330;
	static constexpr int ROOK_VALUE   = 500;
	static constexpr int QUEEN_VALUE  = 900;
	static constexpr int KING_VALUE   = 20000;

	static constexpr int PAWN_PST[64] = {
		0,   0,   0,   0,   0,   0,   0,   0,
		50,  50,  50,  50,  50,  50,  50,  50,
		10,  10,  20,  30,  30,  20,  10,  10,
		5,   5,  10,  25,  25,  10,   5,   5,
		0,   0,   0,  20,  20,   0,   0,   0,
		5,  -5, -10,   0,   0, -10,  -5,   5,
		5,  10,  10, -20, -20,  10,  10,   5,
		0,   0,   0,   0,   0,   0,   0,   0
	};

	static constexpr int KNIGHT_PST[64] = {
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50
	};

	static constexpr int BISHOP_PST[64] = {
		-20, -10, -10, -10, -10, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,  10,  10,   5,   0, -10,
		-10,   5,   5,  10,  10,   5,   5, -10,
		-10,   0,  10,  10,  10,  10,   0, -10,
		-10,  10,  10,  10,  10,  10,  10, -10,
		-10,   5,   0,   0,   0,   0,   5, -10,
		-20, -10, -10, -10, -10, -10, -10, -20
	};

	static constexpr int ROOK_PST[64] = {
		0,   0,   0,   0,   0,   0,   0,   0,
		5,  10,  10,  10,  10,  10,  10,   5,
		-5,   0,   0,   0,   0,   0,   0,  -5,
		-5,   0,   0,   0,   0,   0,   0,  -5,
		-5,   0,   0,   0,   0,   0,   0,  -5,
		-5,   0,   0,   0,   0,   0,   0,  -5,
		-5,   0,   0,   0,   0,   0,   0,  -5,
		0,   0,   0,   5,   5,   0,   0,   0
	};

	static constexpr int QUEEN_PST[64] = {
		-20, -10, -10,  -5,  -5, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,   5,   5,   5,   0, -10,
		-5,    0,   5,   5,   5,   5,   0,  -5,
		0,     0,   5,   5,   5,   5,   0,  -5,
		-10,   5,   5,   5,   5,   5,   0, -10,
		-10,   0,   5,   0,   0,   0,   0, -10,
		-20, -10, -10,  -5,  -5, -10, -10, -20
	};

	static constexpr int KING_PST[64] = {
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-20, -30, -30, -40, -40, -30, -30, -20,
		-10, -20, -20, -20, -20, -20, -20, -10,
		20,  20,   0,   0,   0,   0,  20,  20,
		20,  30,  10,   0,   0,  10,  30,  20
	};

    std::vector<Move> killer_moves_storage;
    std::vector<int> history_scores_storage;

	ChessAI();
	int evaluate(const ChessBoard& board) const;
	int alphaBeta(ChessBoard& board, int depth, int alpha, int beta);
	Move findBestMove(ChessBoard& board);

private:
    int calculate_pawn_shield_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square, uint64_t friendly_pawns_bb) const;
    int calculate_open_file_penalty_internal(const ChessBoard& board_ref, PlayerColor king_color, int king_square, uint64_t friendly_pawns_bb, uint64_t enemy_pawns_bb) const;
    // Reverted signature: quiescence search will now always generate its own moves
    int quiescence_search_internal(ChessBoard& board_ref, int alpha, int beta);
};

#endif // CHESS_AI_H
