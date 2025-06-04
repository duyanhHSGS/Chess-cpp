#include "GameManager.h"
#include "ChessAI.h"
#include "Constants.h"

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

std::streambuf* original_cerr_buffer = nullptr;

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}

void uciLoop(GameManager& gameManager) {
    const std::string logFilePath = "C:\\Users\\Laptop\\Desktop\\CodeProjects\\NeuralNetwork\\Carolyna_log.txt";

    log_file.open(logFilePath, std::ios_base::app);
    if (!log_file.is_open()) {
        std::cerr << "ERROR: Failed to open or create log file at: " << logFilePath << ". Logging to stderr." << std::endl;
    } else {
        original_cerr_buffer = std::cerr.rdbuf();
        std::cerr.rdbuf(log_file.rdbuf());
        log_file << "\n--- [" << getCurrentTimestamp() << "] Starting UCI Session ---\n" << std::endl;
    }

    gameManager.ai = std::make_unique<ChessAI>();

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    std::string line;
    PlayerColor currentTurn = PlayerColor::White;

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
            currentTurn = PlayerColor::White;
            log_file << "[" << getCurrentTimestamp() << "] INFO: New game command received. Board reset." << std::endl;
        } else if (command == "position") {
            std::string type;
            ss >> type;
            std::string fen = "";
            if (type == "startpos") {
                gameManager.createPieces();
                currentTurn = PlayerColor::White;
                log_file << "[" << getCurrentTimestamp() << "] INFO: Position startpos. Board reset." << std::endl;
            } else if (type == "fen") {
                std::string token;
                std::string fullFenString;
                while (ss >> token && token != "moves") {
                    fullFenString += token + " ";
                }
                gameManager.setBoardFromFEN(fullFenString);

                std::stringstream fenStream(fullFenString);
                std::string dummy;
                fenStream >> dummy;
                std::string turnChar;
                fenStream >> turnChar;
                if (turnChar == "b") {
                    currentTurn = PlayerColor::Black;
                } else {
                    currentTurn = PlayerColor::White;
                }
                log_file << "[" << getCurrentTimestamp() << "] INFO: Position FEN: " << fullFenString << ". Board set. Current turn: " << (currentTurn == PlayerColor::White ? "White" : "Black") << std::endl;
            }

            std::string movesKeyword;
            ss >> movesKeyword;

            std::string moveStr;
            while (ss >> moveStr) {
                gameManager.applyUCIStringMove(moveStr, currentTurn);
                currentTurn = (currentTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;
                log_file << "[" << getCurrentTimestamp() << "] INFO: Applied move: " << moveStr << ". Current turn: " << (currentTurn == PlayerColor::White ? "White" : "Black") << std::endl;
            }
        } else if (command == "go") {
            bool isMaximizingPlayer = (currentTurn == PlayerColor::White);

            log_file << "[" << getCurrentTimestamp() << "] INFO: 'go' command received. Calculating best move for " << (isMaximizingPlayer ? "White" : "Black") << "." << std::endl;

            SDL_Point bestNewPos = gameManager.ai->getBestMove(&gameManager, isMaximizingPlayer);

            int bestPieceIdx = gameManager.pieceIndexStack.top();
            gameManager.pieceIndexStack.pop();
            SDL_Point bestOldPos = gameManager.positionStack.top();
            gameManager.positionStack.pop();

            std::string algebraicMove = gameManager.convertMoveToAlgebraic(bestPieceIdx, bestOldPos, bestNewPos);
            
            std::cout << "bestmove " << algebraicMove << std::endl;
            log_file << "[" << getCurrentTimestamp() << "] SENT: bestmove " << algebraicMove << std::endl;

            currentTurn = (currentTurn == PlayerColor::White) ? PlayerColor::Black : PlayerColor::White;

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
        if (original_cerr_buffer) {
            std::cerr.rdbuf(original_cerr_buffer);
        }
    }
}

int main(int argc, char* args[]) {
        GameManager gameManager;
        uciLoop(gameManager);
    return 0;
}
