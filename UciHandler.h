#ifndef UCI_HANDLER_H
#define UCI_HANDLER_H

#include <string>   // For std::string
#include <iostream> // For std::cout, std::cin, std::getline (internal usage)
#include <vector>   // For std::vector (for command arguments)

// Forward declaration of GameManager to avoid circular dependencies.
// UciHandler will interact with GameManager, but GameManager doesn't need UciHandler's full definition here.
class GameManager;

// The UciHandler class is responsible for all input and output operations
// for the chess engine. It will eventually parse UCI commands and format
// engine responses according to the UCI protocol.
class UciHandler {
public:
    // Constructor: Takes a reference to the GameManager to interact with it.
    explicit UciHandler(GameManager& game_manager);

    // The main loop that reads UCI commands from stdin and dispatches them.
    void run_uci_loop();

    // Sends a line of output to stdout (for UCI responses).
    void send_line(const std::string& message);

    // Sends an error message to stderr (for debugging/protocol violations, not UCI standard output).
    void send_error(const std::string& message);

private:
    // Reference to the GameManager instance. UciHandler orchestrates GameManager actions.
    GameManager& game_manager_;

    // Private helper methods for parsing and handling specific UCI commands.
    void handle_uci_command();
    void handle_isready_command();
    void handle_setoption_command(const std::vector<std::string>& tokens);
    void handle_ucinewgame_command();
    void handle_position_command(const std::vector<std::string>& tokens);
    void handle_go_command(const std::vector<std::string>& tokens);
    void handle_quit_command();

    // Helper to split a string into tokens by a delimiter (default space).
    std::vector<std::string> split_string(const std::string& str, char delimiter = ' ');
};

#endif // UCI_HANDLER_H
