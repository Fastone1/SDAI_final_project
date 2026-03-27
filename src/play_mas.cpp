#include "agents.hpp"
#include <iostream>

int main() {
    std::string divider = "\n========================================\n";
    Color color_bot = WHITE;

    std::cout << divider << "Welcome to the Chess MAS!" << divider;
    std::cout << "Please choose your color (w for White, b for Black): ";
    char color_choice;
    std::cin >> color_choice;

    if (color_choice == 'w' || color_choice == 'W') {
        color_bot = BLACK;
    } else if (color_choice != 'b' && color_choice != 'B') {
        std::cout << "Invalid choice. Defaulting to Black.\n";
    }

    std::string starting_fen = STARTING_FEN;
    std::shared_ptr<Agents::Environment> env = std::make_shared<Agents::Environment>(starting_fen);
    Agents::MAS* agent = new Agents::MAS(env, starting_fen, color_bot);
    
    std::cout << "\nStarting game. You are playing as " << (color_bot == WHITE ? "Black" : "White") << ".\n";

    agent->start();

    unsigned int move_count = 0;
    while (!env->is_game_over()) {
        Board current_board = env->get_board();
        std::cout << divider << "Current Board:\n" << std::string(current_board) << "\n";
        std::cout << "It's " << (current_board.turn == WHITE ? "White" : "Black") << "'s turn.\n";
        if (current_board.turn == color_bot) {
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
                Move user_move = current_board.parse_uci(user_input);
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
    std::cout << divider << "Game over! Result: " << env->get_board().result() << "\n";
    std::cout << "Final Board:\n" << std::string(env->get_board()) << "\n" << divider;

    delete agent;

    return 0;
}