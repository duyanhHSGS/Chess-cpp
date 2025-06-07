#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

// Include necessary standard library headers
#include <string> // For std::string to handle command and FEN strings
#include <vector> // For std::vector to manage lists of moves

// Include our custom chess engine component headers
#include "ChessBoard.h"     // The core class managing the chess board state
#include "Move.h"           // Defines the 'Move' structure for chess moves
#include "MoveGenerator.h"  // Provides functionality to generate legal moves
#include "ChessAI.h"        // Include ChessAI to integrate it into GameManager

/**
 * @class GameManager
 * @brief Manages the overall flow of the chess engine, orchestrating UCI communication,
 * board state, and AI interaction.
 *
 * This class acts as the central hub, decoupling the UCI protocol handling from
 * the core chess logic and AI.
 *
 * Primary Responsibilities:
 * - **Game Flow Management:** Initializes the board, manages turns, tracks game
 * state (though game end conditions like checkmate/stalemate detection are
 * planned for future implementation within the ChessBoard/MoveGenerator).
 * - **UCI Command Dispatch:** Receives parsed UCI commands (e.g., from a UciHandler)
 * and dispatches them to appropriate internal methods.
 * - **AI Integration:** Passes the current ChessBoard state to the ChessAI
 * component to find the best move and then applies that move.
 * - **Ownership:** Owns the primary ChessBoard and ChessAI instances.
 */
class GameManager {
public:
    /**
     * @brief Constructs a new GameManager object.
     * Initializes the ChessBoard to its starting position and sets up the ChessAI.
     * Also responsible for ensuring static resources like bitboard attack tables
     * and Zobrist keys are initialized once.
     */
    GameManager();

    /**
     * @brief The main loop that drives the engine.
     * Continuously reads parsed UCI commands (e.g., from a UciHandler) and
     * processes them. This is where the engine's primary execution thread resides.
     */
    void run();

private:
    // Core game state component: The chess board.
    // This is a direct member, implying GameManager owns and manages its lifetime.
    ChessBoard board;

    // AI component: Responsible for calculating the best move.
    // This is a direct member, implying GameManager owns and manages its lifetime.
    ChessAI chess_ai;

    // --- Private Methods for Handling Specific UCI Commands ---
    // These methods encapsulate the logic for responding to different UCI commands.
    // They assume the command has already been parsed into a manageable format.

    /**
     * @brief Handles the 'uci' command.
     * Responds with engine identity and capabilities (id name, id author, uciok).
     */
    void handleUciCommand();

    /**
     * @brief Handles the 'isready' command.
     * Ensures any necessary engine setup (e.g., table initialization) is complete
     * and responds with 'readyok'.
     */
    void handleIsReadyCommand();

    /**
     * @brief Handles the 'ucinewgame' command.
     * Resets the board to the starting position and clears any internal search data.
     */
    void handleUciNewGameCommand();

    /**
     * @brief Handles the 'position' command.
     * Sets up the board state from a FEN string or 'startpos', and applies
     * a sequence of moves if provided.
     * @param command_line The full string for the 'position' command,
     * e.g., "position startpos moves e2e4 e7e5" or
     * "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4".
     */
    void handlePositionCommand(const std::string& command_line);

    /**
     * @brief Handles the 'go' command.
     * Initiates the AI's search for the best move given the current board state
     * and outputs the result via UCI protocol (e.g., 'bestmove').
     */
    void handleGoCommand();
};

#endif // GAME_MANAGER_H
