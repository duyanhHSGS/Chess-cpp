#include "GameManager.h"
#include "UciHandler.h"
#include "ChessBitboardUtils.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>


GameManager::GameManager()
	: board(),
	  chess_ai() {
	ChessBitboardUtils::initialize_attack_tables();
}

void GameManager::run() {
	UciHandler uci_handler;

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
	UciHandler uci_handler;
	uci_handler.sendUciIdentity();
	uci_handler.sendUciOk();
}

void GameManager::handleIsReadyCommand() {
	UciHandler uci_handler;
	uci_handler.sendReadyOk();
}

void GameManager::handleUciNewGameCommand() {
	board.reset_to_start_position();
}

void GameManager::handlePositionCommand(const std::string& command_line) {
	std::stringstream ss(command_line);
	std::string token;
	ss >> token;

	std::string sub_command;
	ss >> sub_command;

	std::string temp_word;

	if (sub_command == "startpos") {
		board.reset_to_start_position();
		ss >> temp_word;
	} else if (sub_command == "fen") {
		std::string fen_string_part;
		while (ss >> temp_word && temp_word != "moves") {
			if (!fen_string_part.empty()) {
				fen_string_part += " ";
			}
			fen_string_part += temp_word;
		}
		board.set_from_fen(fen_string_part);
	} else {
		return;
	}

	if (temp_word == "moves") {
		std::string move_str;
		MoveGenerator move_gen;
		while (ss >> move_str) {
			std::vector<Move> legal_moves = move_gen.generate_legal_moves(board);
			bool move_found = false;
			for (const auto& legal_move : legal_moves) {
				if (ChessBitboardUtils::move_to_string(legal_move) == move_str) {
					StateInfo info_for_undo;
					board.apply_move(legal_move, info_for_undo);
					move_found = true;
					break;
				}
			}
			if (!move_found) {
                std::cerr << "DEBUG: Invalid move encountered in 'position moves' command: " << move_str << std::endl;
                std::cerr << "DEBUG: Current FEN when invalid move was encountered: " << board.to_fen() << std::endl;
				break;
			}
		}
	}
}

void GameManager::handleGoCommand() {
	UciHandler uci_handler;

	Move best_move = chess_ai.findBestMove(board);

	if (best_move.piece_moved_type_idx == PieceTypeIndex::NONE) {
		uci_handler.sendBestMove("(none)");
	} else {
		uci_handler.sendBestMove(ChessBitboardUtils::move_to_string(best_move));
	}
}
