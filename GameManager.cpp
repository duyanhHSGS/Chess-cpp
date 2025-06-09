#include "GameManager.h"
#include "ChessBitboardUtils.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>


GameManager::GameManager()
	: board(),
	  chess_ai(),
	  uci_handler() {
	ChessBitboardUtils::initialize_attack_tables();
}

void GameManager::run() {
	std::string line;
	while (std::getline(std::cin, line)) {
		std::stringstream ss(line);
		std::string command;
		ss >> command;

		if (command == "uci") {
			handleUciCommand();
		} else if (command == "isready") {
			handleIsReadyCommand();
		} else if (command == "ucinewgame") {
			handleUciNewGameCommand();
		} else if (command == "position") {
			handlePositionCommand(line);
		} else if (command == "go") {
			handleGoCommand();
		} else if (command == "quit") {
			break;
		} else if (command == "d") {
			break;
		}
	}
}

void GameManager::handleUciCommand() {
	this->uci_handler.sendUciIdentity();
	this->uci_handler.sendUciOk();
}

void GameManager::handleIsReadyCommand() {
	this->uci_handler.sendReadyOk();
}

void GameManager::handleUciNewGameCommand() {
	board.reset_to_start_position();
}

void GameManager::handlePositionCommand(const std::string& command_line) {
    std::stringstream ss(command_line);
    std::string token;
    std::string sub_command;
    std::string current_token_after_board_setup;

    ss >> token >> sub_command;

    if (sub_command == "startpos") {
        board.reset_to_start_position();
        if (ss >> current_token_after_board_setup) {
        }
    } else if (sub_command == "fen") {
        std::string fen_string_builder;
        int fen_components_read = 0;
        while (ss >> current_token_after_board_setup && current_token_after_board_setup != "moves" && fen_components_read < 6) {
            if (!fen_string_builder.empty()) {
                fen_string_builder += " ";
            }
            fen_string_builder += current_token_after_board_setup;
            fen_components_read++;
        }
        board.set_from_fen(fen_string_builder);
    } else {
        std::cerr << "DEBUG: Invalid position command: " << sub_command << std::endl;
        return;
    }

    if (current_token_after_board_setup == "moves") {
        MoveGenerator move_gen_local;
		std::string move_str;
		while (ss >> move_str) {
            std::vector<Move> current_legal_moves = move_gen_local.generate_legal_moves(board);

			bool move_found = false;
            Move found_move({0, 0}, {0, 0}, PieceTypeIndex::NONE);

			for (const auto& legal_move : current_legal_moves) {
				if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
					found_move = legal_move;
					move_found = true;
					break;
				}
			}

			if (move_found) {
				StateInfo info_for_undo;
				board.apply_move(found_move, info_for_undo);
			} else {
				std::cerr << "DEBUG: Invalid move encountered in 'position moves' command: " << move_str << std::endl;
				std::cerr << "DEBUG: Current FEN when invalid move was encountered: " << board.to_fen() << std::endl;
				break;
			}
		}
	}
}


void GameManager::handleGoCommand() {
	Move best_move = chess_ai.findBestMove(board);

	if (best_move.piece_moved_type_idx == PieceTypeIndex::NONE) {
		this->uci_handler.sendBestMove("(none)");
	} else {
		this->uci_handler.sendBestMove(ChessBitboardUtils::move_to_string(best_move));
	}
}
