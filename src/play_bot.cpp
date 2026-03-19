#include "chess.hpp"

#ifdef BOT_PARALLEL
#include "bot_parallel.hpp"
#else
#include "bot5.hpp"
#endif

#include <iostream>
#include <random>

bool random_bool() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 1);
    return dis(gen) == 1;
}

using namespace std;

int main(int argc, char** argv) {
    const string DIVIDER = "\n----------------------------------------\n";

    if (argc > 1 && (string(argv[1]) == "-s" || string(argv[1]) == "--selfplay")) {
#ifdef BOT_PARALLEL
        Bot_parallel_space::Bot_parallel white_bot{STARTING_FEN, WHITE};
        Bot_parallel_space::Bot_parallel black_bot{STARTING_FEN, BLACK};
#else
        Bot5_space::Bot5 white_bot{STARTING_FEN, WHITE};
        Bot5_space::Bot5 black_bot{STARTING_FEN, BLACK};
#endif
        cout << "Self-play mode: White bot vs Black bot.\n" << DIVIDER;
        while (!white_bot.board.is_game_over()) {
            cout << "Current board:\n" << string(white_bot.board) << "\n";
            cout << "Turn: " << (white_bot.board.turn == WHITE ? "White" : "Black") << "\n";

            Move best_move;
            if (white_bot.board.turn == white_bot.color) {
                best_move = white_bot.get_move();
                white_bot.push(best_move);
                black_bot.push(best_move);
                cout << "White bot's move: " << best_move.uci() << "\n";
            } else {
                best_move = black_bot.get_move();
                white_bot.push(best_move);
                black_bot.push(best_move);
                cout << "Black bot's move: " << best_move.uci() << "\n";
            }
            cout << DIVIDER;
        }
        cout << "Game over: " << white_bot.board.result() << "\n";
        cout << "Final board:\n" << string(white_bot.board) << "\n";
        return 0;
    }

    // std::optional<std::string> fen = "8/pQ5p/3kb2B/8/P4R2/8/6rP/4K1NR w - - 0 1";
    // std::optional<std::string> fen = STARTING_FEN;
    std::optional<std::string> fen = "8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1";
#ifdef BOT_PARALLEL
    Bot_parallel_space::Bot_parallel bot{fen, random_bool()};
#else
    Bot5_space::Bot5 bot{fen, random_bool()};
#endif

    cout << "Welcome to the Chess Bot! You are playing against the bot.\n";
    cout << "You are playing as " << (bot.color == WHITE ? "Black" : "White") << ".\n";
    cout << "Enter your moves in UCI format (e.g., e2e4).\n";
    cout << "Type 'exit' to quit the game." << DIVIDER;

    while (!bot.board.is_game_over()) {
        cout << "Current board:\n" << string(bot.board) << "\n";
        cout << "Turn: " << (bot.board.turn == WHITE ? "White" : "Black") << "\n";

        if (bot.board.turn == bot.color) {
            cout << "Bot is thinking...\n";
            Move best_move = bot.get_move();
            cout << "Bot's move: " << best_move.uci();
            bot.push(best_move);
        } else {
            // Get a move from the user
            string move_str;
            cout << "Enter your move (in UCI format): ";
            cin >> move_str;
            if (move_str == "exit") {
                break;
            }
            try {
                bot.push_uci(move_str);
            } catch (const InvalidMoveError& e) {
                cerr << e.what() << "\n";
                cerr << "Please try again.\n";
                continue;
            } catch (const IllegalMoveError& e) {
                cerr << e.what() << "\n";
                cerr << "Please try again.\n";
                continue;
            } catch (const std::exception& e) {
                cerr << "An error occurred: " << e.what() << "\n";
                cerr << "Please try again.\n";
                continue;
            }
        }
        cout << DIVIDER;
    }
    if (bot.board.is_game_over()) {
        cout << "Game over: " << bot.board.result() << "\n";
        cout << "Final board:\n" << string(bot.board) << "\n";
    } else {
        cout << "Exiting the game. Goodbye!\n";
    }
    
    return 0;
}
