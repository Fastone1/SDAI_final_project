#include "agents.hpp"
#include "bot.hpp"
#include <iostream>

int main() {
    std::string divider = "\n========================================\n";
    Color color_mas = WHITE;

    std::array<int, 3> wins = {0, 0, 0}; // MAS wins, Bot wins, Draws
    std::array<std::array<double, 4>, 100> trust_history; // Store trust values for each game

    std::cout << divider << "Chess MAS vs Chess Bot" << divider;
    std::string starting_fen = STARTING_FEN;

    for (int i = 0; i < 100; ++i) {
        std::shared_ptr<Agents::Environment> env = std::make_shared<Agents::Environment>(starting_fen);
        Color color = (i % 2 == 0) ? WHITE : BLACK; // Alternate colors each game
        Agents::MAS* agent = new Agents::MAS(env, starting_fen, color);
        Bot_space::Bot bot(starting_fen, !color);
        
        std::cout << "\nStarting game.\nMAS is playing as " << (color == WHITE ? "White" : "Black") << ", Bot is playing as " << (color == WHITE ? "Black" : "White") << ".\n";

        agent->start();

        unsigned int move_count = 0;
        Move last_move;
        while (!env->is_game_over()) {
            Board current_board = env->get_board();
            std::cout << divider << "Current Board:\n" << std::string(current_board) << "\n";
            std::cout << "It's " << (current_board.turn == WHITE ? "White" : "Black") << "'s turn.\n";
            if (current_board.turn == color) {
                // Manager will decide the move and make it on the board
                last_move = env->wait_for_move(move_count); // Wait for the Manager to make a move
                bot.push(last_move); // Update bot's internal board state
            } else {
                // Bot calculates its move and makes it on the board
                last_move = bot.get_move();
                bot.push(last_move); // Update bot's internal board state
                env->make_move(last_move, move_count);   // Make the move on the environment and update move count
            }
        }
        Board final_board = env->get_board();
        std::string result = final_board.result();
        std::cout << divider << "Game " << i + 1 << " over! Result: " << result << "\n";
        if ((result == "1-0" && color == WHITE) || (result == "0-1" && color == BLACK)) {
            wins[0]++;
        } else if ((result == "1-0" && color == BLACK) || (result == "0-1" && color == WHITE)) {
            wins[1]++;
        } else {
            wins[2]++;
        }
        trust_history[i] = agent->get_trust(); // Store trust values after each game

        delete agent;
    }
    std::cout << divider << "Final results after 100 games:\nMAS wins: " << wins[0] << "\nBot wins: " << wins[1] << "\nDraws: " << wins[2] << "\n";
    std::cout << "Trust statistics:\n";
    double avg_trust[4] = {0.0, 0.0, 0.0, 0.0};
    for (const auto& trust_values : trust_history) {
        for (int j = 0; j < 4; ++j) {
            avg_trust[j] += trust_values[j];
        }
    }
    for (int j = 0; j < 4; ++j) {
        avg_trust[j] /= 100.0; // Average over 100 games
        std::cout << "Average trust for Searcher Agent " << Agents::profile_names[j] << ": " << avg_trust[j] << "\n";
    }

    return 0;
}