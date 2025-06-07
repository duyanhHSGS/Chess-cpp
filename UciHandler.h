#ifndef UCI_HANDLER_H
#define UCI_HANDLER_H

// Include necessary standard library headers
#include <string>     // For std::string
#include <iostream>   // For std::cin, std::cout
#include <sstream>    // For std::stringstream (useful for parsing input)

// Note: UciHandler should *not* include ChessBoard, Move, etc.,
// as it should be decoupled from the core game logic.
// It only deals with raw UCI string input/output.

/**
 * @class UciHandler
 * @brief Handles the low-level Universal Chess Interface (UCI) communication.
 *
 * This class is responsible solely for reading raw UCI commands from standard input
 * and writing UCI responses to standard output. It acts as a bridge between the
 * chess GUI and the GameManager, but does not interpret the semantics of the
 * chess commands itself, beyond identifying the command type.
 *
 * Primary Responsibilities:
 * - **Reading UCI Input:** Reads full lines from `std::cin`.
 * - **Basic Command Parsing:** Extracts the primary command keyword (e.g., "uci", "position", "go").
 * - **Sending UCI Output:** Provides methods to send standard UCI responses like
 * "id name", "uciok", "readyok", "bestmove", and "info".
 */
class UciHandler {
public:
    /**
     * @brief Constructs a new UciHandler object.
     */
    UciHandler();

    /**
     * @brief Reads a single line from standard input, representing a UCI command.
     *
     * This method blocks until a full line is available.
     * @return The raw command line as a string. Returns an empty string if EOF is reached.
     */
    std::string readLine();

    /**
     * @brief Sends the engine's identity and capabilities as per the UCI protocol.
     * Typically called in response to the "uci" command.
     */
    void sendUciIdentity();

    /**
     * @brief Sends the "uciok" response, signaling that the engine has finished
     * sending its UCI identity and options.
     */
    void sendUciOk();

    /**
     * @brief Sends the "readyok" response, signaling that the engine is ready
     * to receive commands.
     */
    void sendReadyOk();

    /**
     * @brief Sends the "bestmove" command with the engine's chosen move.
     * @param move_string The algebraic string representation of the best move (e.g., "e2e4").
     * @param ponder_string An optional string for the ponder move (if available).
     */
    void sendBestMove(const std::string& move_string, const std::string& ponder_string = "");

    /**
     * @brief Sends an "info" string to the GUI for debugging or status updates.
     * @param message The message string to send.
     */
    void sendInfo(const std::string& message);

private:
    // No private members or helper methods are strictly necessary for this simple
    // handler, as its main job is direct I/O.
};

#endif // UCI_HANDLER_H
