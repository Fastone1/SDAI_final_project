#include "agents.hpp"
#include <iostream>

int main() {
    // Test code for the agents
    Color color_bot = WHITE; // Change to BLACK to test as Black
    std::string starting_fen = STARTING_FEN;
    std::shared_ptr<Agents::Environment> env = std::make_shared<Agents::Environment>(starting_fen);
    Agents::MAS* agent = new Agents::MAS(env, starting_fen, color_bot);
    
    std::cout << "Starting game. You are playing as " << (color_bot == WHITE ? "White" : "Black") << ".\n";

    agent->start();

    unsigned int move_count = 0;
    while (!env->is_game_over()) {
        std::cout << "Current Board:\n" << std::string(env->get_board()) << "\n";
        std::cout << "It's " << (env->get_turn() == WHITE ? "White" : "Black") << "'s turn.\n";
        if (env->get_turn() == color_bot) {
            // Manager will decide the move and make it on the board
            env->wait_for_move(move_count); // Wait for the Manager to make a move
        } else {
            // Ask user for input move in UCI format
            std::string user_input;
            std::cout << "Enter your move in UCI format (e.g., e2e4): ";
            std::cin >> user_input;
            if (user_input == "exit") {
                break;
            } else if (user_input == "help") {
                std::cout << "Enter your moves in UCI format (from square + to square, e.g., e2e4).\n";
                std::cout << "For pawn promotion, specify the promotion piece (e.g., e7e8q for promoting to a queen).\n";
                std::cout << "You can also enter 'pop' to undo the last turn, or 'help' to see these instructions again.\n";
                std::cout << "Type 'exit' to quit the game.\n";
                continue;
            }
            try {
                Move user_move = env->get_board().parse_uci(user_input);
                env->make_move(user_move, move_count);
            } catch (const InvalidMoveError& e) {
                std::cerr << e.what() << "\n";
                std::cerr << "Please try again.\n";
                continue;
            } catch (const IllegalMoveError& e) {
                std::cerr << e.what() << "\n";
                std::cerr << "Please try again.\n";
                continue;
            } catch (const std::exception& e) {
                std::cerr << "An error occurred: " << e.what() << "\n";
                std::cerr << "Please try again.\n";
                continue;
            }
        }
    }
    std::cout << "Game over! Result: " << env->get_board().result() << "\n";
    std::cout << "Final Board:\n" << std::string(env->get_board()) << "\n";

    delete agent;

    return 0;
}