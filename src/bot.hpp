#pragma once
#include "chess.hpp"
#include "tt.hpp"
#include "timer.hpp"
#include "polyglot.hpp"
#include "defs.hpp"

namespace Bot_space
{

    struct Result {
        Move move;
        int score;
    };

    class Bot {
    public:
        Board board;
        Color color;
        TranspositionTable transposition_table;
        Timer timer;
        MemoryMappedReader opening_book;
        Array2D<Move, MAX_DEPTH + MAX_EXTENSION, 2> killer_moves;   // killer moves for move ordering
        Array2D<int, 64, 64> history[2];            // history table for move ordering
#ifdef DEBUG
        uint64_t nodes_searched = 0;
        uint64_t quiescence_nodes = 0;
        std::vector<Move> pv_line;
#endif
#ifdef TIMED
        bool stop = false;
#endif

        Bot(std::optional<std::string> fen = std::nullopt, Color color = WHITE) : board(fen), color(color), transposition_table(), timer(), opening_book("books/computer.bin"), killer_moves(), history() {
            // Initialize killer moves
            for (int i = 0; i < MAX_DEPTH + MAX_EXTENSION; ++i) {
                killer_moves.insert(i, 0, Move::null());
                killer_moves.insert(i, 1, Move::null());
            }
            // Initialize history table
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 64; ++j) {
                    for (int k = 0; k < 64; ++k) {
                        history[i].insert(j, k, 0);
                    }
                }
            }
        }

        void reset() {
            board.reset();
            transposition_table.clear();
            timer.reset();
            for (int i = 0; i < MAX_DEPTH + MAX_EXTENSION; ++i) {
                killer_moves.insert(i, 0, Move::null());
                killer_moves.insert(i, 1, Move::null());
            }
            for (int i = 0; i < 2; ++i) {
                for (int j = 0; j < 64; ++j) {
                    for (int k = 0; k < 64; ++k) {
                        history[i].insert(j, k, 0);
                    }
                }
            }
#ifdef TIMED
            stop = false;
#endif
        }

        void push(const Move& move) {
            board.push(move);
        }

        void push_uci(const std::string& uci_move) {
            board.push_uci(uci_move);
        }

        void pop() {
            board.pop();
        }

        int mvv_lva_score(const Move& move);

        void order_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& moves, const Move& first_move = Move::null(), int ply = -1);

        int material_eval_only() const;

        int evaluate() const;

        int mopup_eval(int eg_eval) const;

        int mobility_eval() const;

        int evaluate_lazy(int alpha, int beta) const;

        std::optional<int> quiesce(int alpha, int beta, int q_depth = 6);

        std::optional<int> negamax(int depth, int alpha, int beta, int NumExtensions = 0, int ply = 1, bool can_null = true);

        Result root_move(int depth, int alpha = -MATE_SCORE, int beta = MATE_SCORE, const Move& ex_best_move = Move::null());

        Move get_move();

        inline void age_history();

        inline void clear_for_new_search();
        
#ifdef DEBUG
        void get_pv_line(int depth);
#endif
    };
}