#include "bot4.hpp"
#include "bot5.hpp"

bool random_bool() {
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    // Create a uniform distribution for 0 and 1
    std::uniform_int_distribution<> dis(0, 1);
    // Generate a random number between 0 and 1
    return dis(gen);
}

int main() {
    Bot4_space::Bot4 bot1(STARTING_FEN, random_bool());
    Bot5_space::Bot5 bot2(STARTING_FEN, !bot1.color);

    printf("Bot size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1), sizeof(bot2));
    printf("Bot Board size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.board), sizeof(bot2.board));
    printf("Bot _BoardState size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(_BoardState(bot1.board)), sizeof(_BoardState(bot2.board)));
    printf("Bot TranspositionTable size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.transposition_table), sizeof(bot2.transposition_table));
    printf("Bot Timer size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.timer), sizeof(bot2.timer));
    /*printf("Bot Opening Book size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.opening_book), sizeof(bot2.opening_book));*/
    printf("Bot Killer Moves size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.killer_moves), sizeof(bot2.killer_moves));
    printf("Bot History size: Bot 1 (Bot4) - %zu bytes, Bot 2 (Bot5) - %zu bytes\n", 
           sizeof(bot1.history), sizeof(bot2.history));
    printf("Size of Bitboard attacks: Diagonal - %zu bytes, File - %zu bytes, Rank - %zu bytes\n", 
           sizeof(BB_DIAG_TABLE.attacks), sizeof(BB_FILE_TABLE.attacks), sizeof(BB_RANK_TABLE.attacks));

    while (true) {
        bot1.reset();
        bot2.reset();
        bot1.board.set_fen(STARTING_FEN);
        bot2.board.set_fen(STARTING_FEN);
        bot1.color = random_bool();
        bot2.color = !bot1.color;
        printf("Starting a new game between Bot 1 (%s) and Bot 2 (%s)\n", 
               bot1.color == WHITE ? "White" : "Black", 
               bot2.color == WHITE ? "White" : "Black");

        while (!bot1.board.is_game_over()) {
            if (bot1.board.turn == bot1.color) {
                Move best_move = bot1.get_move();
                if (best_move == Move::null()) {
                    printf("Bot 1 has no valid moves, game over.\n");
                    break; // No valid moves, game over
                }
                bot1.board.push(best_move);
                bot2.board.push(best_move);
            } else {
                Move best_move = bot2.get_move();
                if (best_move == Move::null()) {
                    printf("Bot 2 has no valid moves, game over.\n");
                    break; // No valid moves, game over
                }
                bot2.board.push(best_move);
                bot1.board.push(best_move);
            }
            printf("Board:\n%s\n", bot1.board.to_string().c_str());
        }

        printf("Game over! Final board state:\n%s\n", bot1.board.to_string().c_str());
        if (bot1.board.is_checkmate()) {
            printf("Checkmate! Winner: %s\n", bot1.board.turn == WHITE ? "Black" : "White");
        } else if (bot1.board.is_stalemate()) {
            printf("Stalemate! No winner.\n");
        } else if (bot1.board.is_insufficient_material()) {
            printf("Insufficient material to continue the game.\n");
        } else if (bot1.board.is_threefold_repetition()) {
            printf("Threefold repetition detected, game drawn.\n");
        } else if (bot1.board.is_fifty_moves()) {
            printf("Fifty moves without a pawn move or capture, game drawn.\n");
        }
    }
}