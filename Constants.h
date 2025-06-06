// Constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint> // For uint8_t
#include "ChessBoard.h" // Include ChessBoard.h to use the ChessBoard struct

// --- Player and AI Constants ---
// Define the search depth for the Alpha-Beta pruning algorithm
// Higher depth means stronger AI but more computation time
const int AI_SEARCH_DEPTH = 3;

// Optional: Number of CPU cores/threads to use for parallel search (e.g., for root moves)
const uint8_t NUMBER_OF_CORES_USED = 1; 

// --- Initial Game State ---
// An initial instance of the ChessBoard struct representing the standard starting position.
// This assumes the ChessBoard() default constructor sets the board to startpos.
const ChessBoard INITIAL_BOARD_STATE = ChessBoard();

#endif // CONSTANTS_H
