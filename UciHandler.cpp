#include "UciHandler.h" // Include the UciHandler header
// <iostream> and <sstream> are already included in UciHandler.h,
// so no need to include them again here.

/**
 * @brief Constructs a new UciHandler object.
 * This constructor doesn't need to do anything specific for this simple handler.
 */
UciHandler::UciHandler() {
    // Constructor can be empty for this simple class.
    // In more complex scenarios, you might set up logging, or configure I/O streams.
}

/**
 * @brief Reads a single line from standard input, representing a UCI command.
 *
 * This method blocks until a full line is available.
 * @return The raw command line as a string. Returns an empty string if EOF is reached.
 */
std::string UciHandler::readLine() {
    std::string line;
    // std::getline reads characters from cin into 'line' until a newline character is found.
    // The newline character is extracted from the input stream but not stored in 'line'.
    if (std::getline(std::cin, line)) {
        return line;
    }
    return ""; // Return empty string on EOF or error
}

/**
 * @brief Sends the engine's identity and capabilities as per the UCI protocol.
 * Typically called in response to the "uci" command.
 */
void UciHandler::sendUciIdentity() {
    // "id name <engine name>": Specifies the engine's name.
    std::cout << "id name Carolyna" << std::endl;
    // "id author <author name>": Specifies the author's name.
    std::cout << "id author Duy Anh" << std::endl;
    // Any UCI options would be sent here.
    // Example: std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
}

/**
 * @brief Sends the "uciok" response, signaling that the engine has finished
 * sending its UCI identity and options.
 */
void UciHandler::sendUciOk() {
    std::cout << "uciok" << std::endl;
}

/**
 * @brief Sends the "readyok" response, signaling that the engine is ready
 * to receive commands.
 */
void UciHandler::sendReadyOk() {
    std::cout << "readyok" << std::endl;
}

/**
 * @brief Sends the "bestmove" command with the engine's chosen move.
 * @param move_string The algebraic string representation of the best move (e.g., "e2e4").
 * @param ponder_string An optional string for the ponder move (if available).
 */
void UciHandler::sendBestMove(const std::string& move_string, const std::string& ponder_string) {
    std::cout << "bestmove " << move_string;
    if (!ponder_string.empty()) {
        std::cout << " ponder " << ponder_string;
    }
    std::cout << std::endl; // Flush the output buffer
}

/**
 * @brief Sends an "info" string to the GUI for debugging or status updates.
 * @param message The message string to send.
 */
void UciHandler::sendInfo(const std::string& message) {
    std::cout << "info string " << message << std::endl; // Flush the output buffer
}
