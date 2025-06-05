#include "GameManager.h"
#include "ChessAI.h"
#include "Constants.h" // Ensure PlayerColor enum is defined here

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <iomanip>

std::ofstream log_file;

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}

void uciLoop(GameManager& gameManager) {
    // Changed log file name as requested
    const std::string logFilePath = "log.txt"; 

    log_file.open(logFilePath, std::ios_base::app);
    if (!log_file.is_open()) {
        std::cerr << "ERROR: Failed to open or create log file at: " << logFilePath << ". Logging to stderr." << std::endl;
    } else {
        std::cerr.rdbuf(log_file.rdbuf());
        log_file << "\n--- [" << getCurrentTimestamp() << "] Starting UCI Session ---\n" << std::endl;
    }

    gameManager.ai = std::make_unique<ChessAI>();

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    // Moved 'line' declaration outside the while loop to resolve "not declared in this scope" error
    std::string line; 

    // currentTurn in main.cpp reflects whose turn it is in the game currently
    // This is the variable that the 'go' command relies on.
    PlayerColor currentTurn = PlayerColor::White; 
    std::vector<std::string> internalMoveHistory; // To keep track of moves applied by the engine

    log_file << "[" << getCurrentTimestamp() << "] INFO: PlayerColor::White raw value: " << static_cast<int>(PlayerColor::White) << std::endl;
    log_file << "[" << getCurrentTimestamp() << "] INFO: PlayerColor::Black raw value: " << static_cast<int>(PlayerColor::Black) << std::endl;


    while (std::getline(std::cin, line)) {
        log_file << "[" << getCurrentTimestamp() << "] RECEIVED: " << line << std::endl;

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "uci") {
            std::cout << "id name Carolyna" << std::endl;
            std::cout << "id author Duy Anh" << std::endl;
            std::cout << "uciok" << std::endl;
            log_file << "[" << getCurrentTimestamp() << "] SENT: id name Carolyna" << std::endl;
            log_file << "[" << getCurrentTimestamp() << "] SENT: id author Duy Anh" << std::endl;
            log_file << "[" << getCurrentTimestamp() << "] SENT: uciok" << std::endl;
        } else if (command == "isready") {
            std::cout << "readyok" << std::endl;
            log_file << "[" << getCurrentTimestamp() << "] SENT: readyok" << std::endl;
        } else if (command == "ucinewgame") {
            gameManager.createPieces();
            currentTurn = PlayerColor::White; // Reset to White for new game
            internalMoveHistory.clear(); // Clear history for a new game
            log_file << "[" << getCurrentTimestamp() << "] INFO: New game command received. Board reset. currentTurn set to White (Raw: " << static_cast<int>(currentTurn) << ")." << std::endl;
        } else if (command == "position") {
            std::string type;
            ss >> type;
            std::string baseFen = "";
            PlayerColor fenActivePlayer; // This stores the active player directly from the FEN string

            // Step 1: Set the base position (startpos or FEN)
            if (type == "startpos") {
                gameManager.createPieces();
                fenActivePlayer = PlayerColor::White; // Startpos always has White to move
                log_file << "[" << getCurrentTimestamp() << "] INFO: Position startpos. Board reset." << std::endl;
            } else if (type == "fen") {
                std::string token;
                std::string fullFenString;
                // Read FEN components until "moves" or end of string
                while (ss >> token && token != "moves") {
                    fullFenString += token + " ";
                }
                if (!fullFenString.empty()) {
                    fullFenString.pop_back(); // Remove trailing space
                }
                gameManager.setBoardFromFEN(fullFenString);

                // Extract active player from FEN
                std::stringstream fenStream(fullFenString);
                std::string dummy;
                fenStream >> dummy >> dummy; // Skip board and active color
                char turnChar;
                fenStream >> turnChar;
                if (turnChar == 'b') {
                    fenActivePlayer = PlayerColor::Black;
                } else {
                    fenActivePlayer = PlayerColor::White;
                }
                log_file << "[" << getCurrentTimestamp() << "] INFO: Position FEN: " << fullFenString << ". Board set. FEN active player: " << (fenActivePlayer == PlayerColor::White ? "White" : "Black") << " (Raw: " << static_cast<int>(fenActivePlayer) << ")" << std::endl;
            }

            // Step 2: Parse the moves list from the UCI command
            std::vector<std::string> incomingMoves;
            std::string moveStr;
            if (ss.peek() == ' ') { // Ensure we consume "moves" if it's still in the stream
                std::string dummy;
                ss >> dummy;
            }
            while (ss >> moveStr) {
                incomingMoves.push_back(moveStr);
            }

            // Step 3: Synchronize internal board history with incoming moves history
            size_t commonPrefixLength = 0;
            while (commonPrefixLength < internalMoveHistory.size() &&
                   commonPrefixLength < incomingMoves.size() &&
                   internalMoveHistory[commonPrefixLength] == incomingMoves[commonPrefixLength]) {
                commonPrefixLength++;
            }
            
            // At this point, the game state (board and turn) is as defined by the base FEN.
            // We need to advance the turn for each move in the common prefix to correctly
            // determine the turn for applying new moves.
            PlayerColor currentApplyingTurn = fenActivePlayer;
            for (size_t i = 0; i < commonPrefixLength; ++i) {
                currentApplyingTurn = (currentApplyingTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
            }

            // If our internal history is longer than the common prefix, we need to undo moves
            // This happens if the GUI sends a `position` command that goes back in history.
            while (internalMoveHistory.size() > incomingMoves.size()) { // Undo moves that are not in the incoming history
                gameManager.undoLastMove();
                internalMoveHistory.pop_back();
                currentApplyingTurn = (currentApplyingTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White; // Revert turn for undone move
                log_file << "[" << getCurrentTimestamp() << "] INFO: Undid move. Current turn: " << (currentApplyingTurn == PlayerColor::White ? "White" : "Black") << " (Raw: " << static_cast<int>(currentApplyingTurn) << ")" << std::endl;
            }
            
            // Apply new moves that are not in the common prefix
            // This handles new moves in the current line, or re-applies all moves if a full reset occurred
            for (size_t i = commonPrefixLength; i < incomingMoves.size(); ++i) {
                gameManager.applyUCIStringMove(incomingMoves[i], currentApplyingTurn);
                currentApplyingTurn = (currentApplyingTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White; // Toggle turn for the *next* move
                internalMoveHistory.push_back(incomingMoves[i]);
                log_file << "[" << getCurrentTimestamp() << "] INFO: Applied move: " << incomingMoves[i] << ". Current turn (after applying): " << (currentApplyingTurn == PlayerColor::White ? "White" : "Black") << " (Raw: " << static_cast<int>(currentApplyingTurn) << ")" << std::endl;
            }

            // After all moves in the `moves` list are processed, `currentApplyingTurn` holds the actual turn of the game.
            currentTurn = currentApplyingTurn; // Update the main `currentTurn` for the `go` command.

            // Explicit debug log to confirm currentTurn value immediately before the final INFO log
            log_file << "[" << getCurrentTimestamp() << "] DEBUG: Value of currentTurn before final log: " << (currentTurn == PlayerColor::White ? "White" : "Black") << " (Raw: " << static_cast<int>(currentTurn) << ")" << std::endl;

            log_file << "[" << getCurrentTimestamp() << "] INFO: Final board state (hash " << gameManager.currentZobristHash << ") is " << (gameManager.ai->hasPositionInTable(gameManager.currentZobristHash) ? "in" : "NOT in") << " the transposition table. Engine's current internal turn for next `go` (Raw: " << static_cast<int>(currentTurn) << ")." << std::endl;

        } else if (command == "go") {
            // The `currentTurn` variable here should accurately reflect whose turn it is to move,
            // as established by the preceding `position` command.
            bool isMaximizingPlayer = (currentTurn == PlayerColor::White);
            log_file << "[" << getCurrentTimestamp() << "] INFO: 'go' command received. Calculating best move for " << (isMaximizingPlayer ? "White" : "Black") << " (Raw turn: " << static_cast<int>(currentTurn) << ")." << std::endl;

            GamePoint bestNewPos = gameManager.ai->getBestMove(&gameManager, isMaximizingPlayer);

            // Handle case where no legal moves are found (stalemate/checkmate from AI perspective)
            if (bestNewPos.x == -1 && bestNewPos.y == -1) {
                std::cout << "bestmove (none)" << std::endl;
                log_file << "[" << getCurrentTimestamp() << "] SENT: bestmove (none) (No legal moves found by AI)" << std::endl;
            } else {
                int bestPieceIdx = gameManager.pieceIndexStack.top();
                gameManager.pieceIndexStack.pop();
                GamePoint bestOldPos = gameManager.positionStack.top();
                gameManager.positionStack.pop();

                std::string algebraicMove = gameManager.convertMoveToAlgebraic(bestPieceIdx, bestOldPos, bestNewPos);
                
                std::cout << "bestmove " << algebraicMove << std::endl;
                log_file << "[" << getCurrentTimestamp() << "] SENT: bestmove " << algebraicMove << std::endl;

                // Update internal history with the engine's move
                // Apply the engine's own move to its board to keep `gameManager` in sync
                gameManager.applyUCIStringMove(algebraicMove, currentTurn); 
                internalMoveHistory.push_back(algebraicMove);
                currentTurn = (currentTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White; // Toggle turn after engine's move
                log_file << "[" << getCurrentTimestamp() << "] INFO: Engine applied its move: " << algebraicMove << ". Current turn (after engine's move): " << (currentTurn == PlayerColor::White ? "White" : "Black") << " (Raw: " << static_cast<int>(currentTurn) << ")" << std::endl;
            }

        } else if (command == "quit") {
            log_file << "[" << getCurrentTimestamp() << "] INFO: Quit command received. Exiting." << std::endl;
            break;
        } else {
            log_file << "[" << getCurrentTimestamp() << "] WARNING: Unknown command received: " << command << std::endl;
        }
    }

    log_file << "\n--- [" << getCurrentTimestamp() << "] UCI Session Ended ---\n" << std::endl;
    if (log_file.is_open()) {
        log_file.close();
    }
}

int main(int argc, char* args[]) {
    GameManager gameManager;
    uciLoop(gameManager);
    return 0;
}
