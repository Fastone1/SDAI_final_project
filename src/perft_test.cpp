#include "chess.hpp"
#include "timer.hpp"
#include <iostream>
#include <string>
// #include "windows.h" // Uncomment this line if you want to set the console output to UTF-8

using U64 = unsigned long long;

const std::string divider = "\n----------------------------------------\n";
/* 
    5,4865609,rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
	5,5617302,2b1b3/1r1P4/3K3p/1p6/2p5/6k1/1P3p2/4B3 w - - 0 42
	6,11030083,8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -
	5,15587335,r3k2r/pp3pp1/PN1pr1p1/4p1P1/4P3/3P4/P1P2PP1/R3K2R w KQkq - 4 4
	5,89941194,rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8
	4,3894594,r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10
	5,193690690,r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
	4,497787,r3k1nr/p2pp1pp/b1n1P1P1/1BK1Pp1q/8/8/2PP1PPP/6N1 w kq - 0 1
	6,1134888,3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1
	6,1440467,8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1
	6,661072,5k2/8/8/8/8/8/8/4K2R w K - 0 1
	7,15594314,3k4/8/8/8/8/8/8/R3K3 w Q - 0 1
	4,1274206,r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1
	5,58773923,r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1
	6,3821001,2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1
	5,1004658,8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1
	6,217342,4k3/1P6/8/8/8/8/K7/8 w - - 0 1
	6,92683,8/P1k5/K7/8/8/8/8/8 w - - 0 1
	10,5966690,K1k5/8/P7/8/8/8/8/8 w - - 0 1
	7,567584,8/k1P5/8/1K6/8/8/8/8 w - - 0 1
	6,3114998,8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1
	5,42761834,r1bq2r1/1pppkppp/1b3n2/pP1PP3/2n5/2P5/P3QPPP/RNB1K2R w KQ a6 0 12
	4,3050662,r3k2r/pppqbppp/3p1n1B/1N2p3/1nB1P3/3P3b/PPPQNPPP/R3K2R w KQkq - 11 10
	5,10574719,4k2r/1pp1n2p/6N1/1K1P2r1/4P3/P5P1/1Pp4P/R7 w k - 0 6
	4,6871272,1Bb3BN/R2Pk2r/1Q5B/4q2R/2bN4/4Q1BK/1p6/1bq1R1rb w - - 0 1
	6,71179139,n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1
	6,28859283,8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1
	9,7618365,8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1
	4,28181,3r4/2p1p3/8/1P1P1P2/3K4/5k2/8/8 b - - 0 1
	5,6323457,8/1p4p1/8/q1PK1P1r/3p1k2/8/4P3/4Q3 b - - 0 1
*/
const int FULL_NUM_TESTS = 30;
const std::string full_test_fen[FULL_NUM_TESTS] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "2b1b3/1r1P4/3K3p/1p6/2p5/6k1/1P3p2/4B3 w - - 0 42",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/pp3pp1/PN1pr1p1/4p1P1/4P3/3P4/P1P2PP1/R3K2R w KQkq - 4 4",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "r3k1nr/p2pp1pp/b1n1P1P1/1BK1Pp1q/8/8/2PP1PPP/6N1 w kq - 0 1",
    "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
    "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
    "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
    "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",
    "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",
    "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
    "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1",
    "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1",
    "4k3/1P6/8/8/8/8/K7/8 w - - 0 1",
    "8/P1k5/K7/8/8/8/8/8 w - - 0 1",
    "K1k5/8/P7/8/8/8/8/8 w - - 0 1",
    "8/k1P5/8/1K6/8/8/8/8 w - - 0 1",
    "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1",
    "r1bq2r1/1pppkppp/1b3n2/pP1PP3/2n5/2P5/P3QPPP/RNB1K2R w KQ a6 0 12",
    "r3k2r/pppqbppp/3p1n1B/1N2p3/1nB1P3/3P3b/PPPQNPPP/R3K2R w KQkq - 11 10",
    "4k2r/1pp1n2p/6N1/1K1P2r1/4P3/P5P1/1Pp4P/R7 w k - 0 6",
    "1Bb3BN/R2Pk2r/1Q5B/4q2R/2bN4/4Q1BK/1p6/1bq1R1rb w - - 0 1",
    "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1",
    "8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1",
    "8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1",
    "3r4/2p1p3/8/1P1P1P2/3K4/5k2/8/8 b - - 0 1",
    "8/1p4p1/8/q1PK1P1r/3p1k2/8/4P3/4Q3 b - - 0 1"
};
const int full_test_depths[FULL_NUM_TESTS] = {
    5,
    5,
    6,
    5,
    5,
    4,
    5,
    4,
    6,
    6,
    6,
    7,
    4,
    5,
    6,
    5,
    6,
    6,
    10,
    7,
    6,
    5,
    4,
    5,
    4,
    6,
    6,
    9,
    4,
    5,
};
const U64 full_expected_counts[FULL_NUM_TESTS] = {
    4865609,
	5617302,
	11030083,
	15587335,
	89941194,
	3894594,
	193690690,
	497787,
	1134888,
	1440467,
	661072,
	15594314,
	1274206,
	58773923,
	3821001,
	1004658,
	217342,
	92683,
	5966690,
	567584,
	3114998,
	42761834,
	3050662,
	10574719,
	6871272,
	71179139,
	28859283,
	7618365,
	28181,
	6323457,
};

const int FAST_NUM_TESTS = 15;
const std::string fast_test_fen[FAST_NUM_TESTS] = {
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	"r3k2r/pp3pp1/PN1pr1p1/4p1P1/4P3/3P4/P1P2PP1/R3K2R w KQkq - 4 4",
	"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
	"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
	"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1",
	"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1",
	"r1bq2r1/1pppkppp/1b3n2/pP1PP3/2n5/2P5/P3QPPP/RNB1K2R w KQ a6 0 12",
	"4k2r/1pp1n2p/6N1/1K1P2r1/4P3/P5P1/1Pp4P/R7 w k - 0 6",
	"1Bb3BN/R2Pk2r/1Q5B/4q2R/2bN4/4Q1BK/1p6/1bq1R1rb w - - 0 1",
	"8/k1P5/8/1K6/8/8/8/8 w - - 0 1",
	"r3k1nr/p2pp1pp/b1n1P1P1/1BK1Pp1q/8/8/2PP1PPP/6N1 w kq - 0 1",
	"3r4/2p1p3/8/1P1P1P2/3K4/5k2/8/8 b - - 0 1",
	"8/1p4p1/8/q1PK1P1r/3p1k2/8/4P3/4Q3 b - - 0 1",
	"2b1b3/1r1P4/3K3p/1p6/2p5/6k1/1P3p2/4B3 w - - 0 42"
};

const int fast_test_depths[FAST_NUM_TESTS] = { 3, 4, 5, 4, 4, 5, 4, 4, 4, 4, 7, 4, 4, 5, 5 };
const U64 fast_expected_counts[FAST_NUM_TESTS] = {
    2812,
	4085603,
	15587335,
	3894594,
	1720476,
	1004658,
	23527,
	1280017,
	491364,
	6871272,
	567584,
	497787,
	28181,
	6323457,
	5617302,
};

U64 perft(int depth, Board& board) {
    if (depth == 0) return 1ULL;
    U64 count = 0;
    StaticVector<Move, LEGAL_MOVES_SIZE> moves;
    board.generate_legal_moves(moves);
    for (const Move& move : moves) {
        board.push(move);
        count += perft(depth - 1, board);
        board.pop();
    }
    
    return count;
}

void fast_perft_test(Board& board) {
    double time;
    Timer timer;
    double total_time = 0.0;
    int nodes_count = 0;

    std::cout << "\n\033[1mRunning Fast Test Suite (" << FAST_NUM_TESTS << " tests)\033[0m\n\n";
    for (int i = 0; i < FAST_NUM_TESTS; ++i) {
        board.set_fen(fast_test_fen[i]);

        timer.start();
        U64 count = perft(fast_test_depths[i], board);
        timer.stop();

        std::cout << "Test \033[1m" << i + 1 << "\033[0m\t";
        std::cout << fast_expected_counts[i] << "/" << count << " nodes\t";

        time = timer.time_ms();
        // blue text
        std::cout << "\033[34m" << time << "\033[0m ms\t";
        total_time += time;

        if (count != fast_expected_counts[i]) {
            std::cout << "\033[31mFailed\033[0m\t";
        } else {
            std::cout << "\033[32mPassed\033[0m\t";
        }

        nodes_count += count;

        std::cout << "FEN: \033[90m" << fast_test_fen[i] << "\033[0m\n";
    }
    std::cout << divider;
    std::cout << "Total time taken for all tests: " << total_time << " ms\n";
    std::cout << "Nodes per second: " << (nodes_count / (total_time / 1000.0)) << "\n";
}

void fast_measure_push_legal_moves(Board& board) {
    for (int i = 0; i < FAST_NUM_TESTS; ++i) {
        board.set_fen(fast_test_fen[i]);
        std::cout << "Test \033[1m" << i + 1 << "\033[0m\t";
    
        double total_push_time = 0.0;
        double total_pop_time = 0.0;

        Timer timer;
        timer.start();
        StaticVector<Move, LEGAL_MOVES_SIZE> moves;
        board.generate_legal_moves(moves);
        timer.stop();
        double legal_moves_time = timer.time_ms();
        std::cout << "\033[1mlegal moves: \033[0m\033[34m" << legal_moves_time << "\033[0m ms\t";

        int num_moves = moves.size();
        for (const Move& move : moves) {
            timer.start();
            board.push(move);
            timer.stop();
            total_push_time += timer.time_ms();

            timer.start();
            board.pop();
            timer.stop();
            total_pop_time += timer.time_ms();
        }
        std::cout << "\033[1mTotal push: \033[0m\033[34m" << total_push_time << "\033[0m ms\t";
        std::cout << "\033[1mAverage push: \033[0m\033[34m" << (total_push_time / num_moves) << "\033[0m ms\t";
        std::cout << "\033[1mTotal pop: \033[0m\033[34m" << total_pop_time << "\033[0m ms\t";
        std::cout << "\033[1mAverage pop: \033[0m\033[34m" << (total_pop_time / num_moves) << "\033[0m ms\n";

        double total_time = legal_moves_time + total_push_time + total_pop_time;
        std::cout << "\033[1mPop Perc time: \033[0m\033[34m" << (100 * (total_pop_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mPush Perc time: \033[0m\033[34m" << (100 * (total_push_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mLegal Moves Perc time: \033[0m\033[34m" << (100 * (legal_moves_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mTotal time: \033[0m\033[34m" << total_time << "\033[0m ms\n";
    }
}

void full_perft_test(Board& board) {
    double time;
    Timer timer;
    double total_time = 0.0;
    int nodes_count = 0;

    std::cout << "\n\033[1mRunning Full Test Suite (" << FULL_NUM_TESTS << " tests)\033[0m\n\n";
    for (int i = 0; i < FULL_NUM_TESTS; ++i) {
        board.set_fen(full_test_fen[i]);

        timer.start();
        U64 count = perft(full_test_depths[i], board);
        timer.stop();

        std::cout << "Test \033[1m" << i + 1 << "\033[0m\t";
        std::cout << full_expected_counts[i] << "/" << count << " nodes\t";

        time = timer.time_ms();
        // blue text
        std::cout << "\033[34m" << time << "\033[0m ms\t";
        total_time += time;

        if (count != full_expected_counts[i]) {
            std::cout << "\033[31mFailed\033[0m\t";
        } else {
            std::cout << "\033[32mPassed\033[0m\t";
        }

        nodes_count += count;

        std::cout << "FEN: \033[90m" << full_test_fen[i] << "\033[0m\n";
    }
    std::cout << divider;
    std::cout << "Total time taken for all tests: " << total_time << " ms\n";
    std::cout << "Nodes per second: " << (nodes_count / (total_time / 1000.0)) << "\n";
}

void full_measure_push_legal_moves(Board& board) {
    for (int i = 0; i < FULL_NUM_TESTS; ++i) {
        board.set_fen(full_test_fen[i]);
        std::cout << "Test \033[1m" << i + 1 << "\033[0m\t";
    
        double total_push_time = 0.0;
        double total_pop_time = 0.0;

        Timer timer;
        timer.start();
        StaticVector<Move, LEGAL_MOVES_SIZE> moves;
        board.generate_legal_moves(moves);
        timer.stop();
        double legal_moves_time = timer.time_ms();
        std::cout << "\033[1mlegal moves: \033[0m\033[34m" << legal_moves_time << "\033[0m ms\t";

        int num_moves = moves.size();
        for (const Move& move : moves) {
            timer.start();
            board.push(move);
            timer.stop();
            total_push_time += timer.time_ms();

            timer.start();
            board.pop();
            timer.stop();
            total_pop_time += timer.time_ms();
        }
        std::cout << "\033[1mTotal push: \033[0m\033[34m" << total_push_time << "\033[0m ms\t";
        std::cout << "\033[1mAverage push: \033[0m\033[34m" << (total_push_time / num_moves) << "\033[0m ms\t";
        std::cout << "\033[1mTotal pop: \033[0m\033[34m" << total_pop_time << "\033[0m ms\t";
        std::cout << "\033[1mAverage pop: \033[0m\033[34m" << (total_pop_time / num_moves) << "\033[0m ms\n";

        double total_time = legal_moves_time + total_push_time + total_pop_time;
        std::cout << "\033[1mPop Perc time: \033[0m\033[34m" << (100 * (total_pop_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mPush Perc time: \033[0m\033[34m" << (100 * (total_push_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mLegal Moves Perc time: \033[0m\033[34m" << (100 * (legal_moves_time / total_time)) << "\033[0m %\t";
        std::cout << "\033[1mTotal time: \033[0m\033[34m" << total_time << "\033[0m ms\n";
    }
}

int main(int argc, char* argv[]) {
    size_t n_tests = 1;
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--fast") {
            Board perft_board(STARTING_FEN);
            fast_perft_test(perft_board);
            return 0;
        } else if (arg == "--full") {
            Board perft_board(STARTING_FEN);
            full_perft_test(perft_board);
            return 0;
        } else if (arg == "--measure-full") {
            Board perft_board(STARTING_FEN);
            full_measure_push_legal_moves(perft_board);
            return 0;
        } else if (arg == "--measure-fast") {
            Board perft_board(STARTING_FEN);
            fast_measure_push_legal_moves(perft_board);
            return 0;
        } else if (arg == "--tests") {
            if (argc > 2) {
                n_tests = std::stoul(argv[2]);
            }
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            std::cerr << "Usage: " << argv[0] << " [--fast | --full | --measure-full | --measure-fast | --tests N]\n";
            return 1;
        }
    }
    // Test the perft function
    Board perft_board(STARTING_FEN);
    for (size_t i = 0; i < n_tests; ++i) {
        fast_perft_test(perft_board);
        std::cout << divider;
        full_perft_test(perft_board);
        std::cout << divider;
    }
    
    std::cout << "Testing push and pop legal moves\n\n";
    fast_measure_push_legal_moves(perft_board);
    std::cout << divider;

    std::cout << "Testing start position at depth 6\n\n";
    perft_board.set_fen(STARTING_FEN);
    Timer timer;
    timer.start();
    U64 count = perft(6, perft_board);
    timer.stop();
    double time = timer.time_ms();
    std::cout << "Count: " << count << "\n";
    std::cout << "Time: " << time << " ms\n";
    std::cout << "Nodes per second: " << (count / (time / 1000.0)) << "\n";
}