//#include "bot5.hpp"
#include "chess.hpp"
#include "timer.hpp"
#include <iostream>
#include <string>
// #include "windows.h" // Uncomment this line if you want to set the console output to UTF-8

using U64 = unsigned long long;
U64 perft(int depth, Board& board, int& captures) {
    if (depth == 0) return 1ULL;
    U64 count = 0;
    StaticVector<Move, LEGAL_MOVES_SIZE> moves;
    board.generate_legal_moves(moves);
    for (const Move& move : moves) {
        if (board.is_capture(move) && depth == 1) captures++;
        board.push(move);
        count += perft(depth - 1, board, captures);
        board.pop();
    }
    
    return count;
}

void perft_test(int depth, Board& board, int& captures) {
    captures = 0;

    Timer timer;
    timer.start();

    U64 count = perft(depth, board, captures);

    timer.stop();

    std::cout << "Perft count for depth " << depth << ": " << count << "\n";
    std::cout << "Captures: " << captures << "\n";
    std::cout << "Time taken for depth " << depth << ": " << timer.elapsed() << " seconds\n\n";
}


int main() {
    /*
    // Uncomment the following lines if you want to
    // disable synchronization with C I/O and speed up I/O
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    */

    // SetConsoleOutputCP(CP_UTF8); // Uncomment this line if you want to set the console output to UTF-8
    std::string divider = "\n----------------------------------------\n";

    // test the generation of castling moves
    Board test_board("8/k7/3p4/p2Ppppp/P2P1P2/8/8/K7 w - - 0 0");
    StaticVector<Move, LEGAL_MOVES_SIZE> moves;
    test_board.generate_legal_moves(moves);
    std::cout << "Moves:\n";
    for (const Move& move : moves) {
        std::cout << move.uci() << "\n";
    }

    /* // Test the perft function
    Board perft_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int captures;
    perft_test(1, perft_board, captures); // 20
    perft_test(2, perft_board, captures); // 400
    perft_test(3, perft_board, captures); // 8_902
    perft_test(4, perft_board, captures); // 197_281
    perft_test(5, perft_board, captures); // 4_865_609
    perft_test(6, perft_board, captures); // 119_060_324
    std::cout << divider;


    // Wait for user input to continue
    std::cout << "Press Enter to continue...\n";
    std::cin.get();
    std::cout << divider; */

    // Create a chess board object
    // Board board("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 0");
    /*
    std::string fen = "8/3K4/4P3/8/8/8/6k1/7q w - - 0 0";
    fen = STARTING_FEN;
    Board board(fen);

    // Print the initial board state
    std::cout << divider;
    std::cout << "Initial board:\n" << std::string(board) << "\n";

    Bot5_space::Bot5 bot1(fen, BLACK);

    while (!board.is_game_over()) {
        std::cout << divider;
        std::cout << "Current board:\n" << std::string(board) << "\n";
        std::cout << "Turn: " << (board.turn == WHITE ? "White" : "Black") << "\n";

        bot1.evaluate();
        if (board.turn == bot1.color) {
            Move best_move = bot1.get_move();
            std::cout << "Bot's move: " << best_move.uci() << "\n";
            
            // Make the move on the board
            bot1.board.push(best_move);
            board.push(best_move);
        } else {
            // Get a move from the user
            std::string move_str;
            std::cout << "Enter your move (in UCI format): ";
            std::cin >> move_str;
            try {
                board.push_uci(move_str);
                bot1.board.push_uci(move_str);
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
        /*
        std::cout << divider;
        std::cout << "Current board:\n" << std::string(board) << "\n";
        std::cout << "Turn: " << (board.turn == WHITE ? "White" : "Black") << "\n";

        // Get a move from the user
        std::string move_str;
        std::cout << "Enter your move (in UCI format): ";
        std::cin >> move_str;
        try {
            board.push_uci(move_str);
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
        *//*
    }

    std::cout << divider;
    std::cout << "Game over! Result: " << board.result() << "\n";
    std::cout << "Final board:\n" << std::string(board) << "\n";
    std::cout << divider;*/

    return 0;
}