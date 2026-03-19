#pragma once
#include "chess.hpp"
#include "defs.hpp"
#include "tt.hpp"
#include "timer.hpp"
#include "array2d.hpp"
#include "polyglot.hpp"

#include <atomic>
#include <thread>
#include <mutex>

/*
My project is the implementation of a Chess Engine with Agents.
The basic idea is the same as a classical Chess Engine (like Deep Blue),
to represent the board in some way and then generate and explore the possible nodes
starting from a root position using alpha-beta pruning.
Then, for the MAS part, there would be at least 4 Searcher Agents that search the Board State Tree
using the classical approach, but with different heuristics in the evaluation of a position
(I'm thinking of having an Agent for early game evaluation, one for late game,
and the last two a mix of these with other heuristics).
Finally a Manager Agent interacts with the Environment (makes the moves on a board,
look for the user moves) and decides which move to play between those proposed by the Searchers;
in order to do this it should implement a way do evaluate these moves,
like number of votes received or an updating trust ranking of the workers.
I intend to use C++ to develop this software, threads for the agents and shared data structures for communication.
*/

namespace Agents {
    enum class AgentType : uint8_t {
        Searcher,
        Manager
    };

    enum class AgentProfile : uint8_t {
        Opening,
        Middlegame,
        Endgame,
        Hybrid
    };

    struct Result {
        Move move;
        int score;
    };

    // Proposal structure for Searcher Agents to send to Manager
    struct AgentProposal {
        float norm_score = 0.0; // normalized score in [-1, 1]
        int raw_score_cp = 0; // score in centipawns
        int depth_reached = 0;
        Move best_move = Move::null();
        AgentProfile profile = AgentProfile::Hybrid;
    };

    // Shared proposals from searcher agents to manager
    std::array<std::atomic<AgentProposal>, 4> agentProposals;

    class Agent {
    public:
        AgentProposal proposal;
        AgentType type;
        AgentProfile profile;
        
        Agent(const AgentProposal& proposal, AgentType type, AgentProfile profile) 
            : proposal(proposal), type(type), profile(profile) {}
        
        virtual ~Agent() = default;
    };

    class SearcherAgent : public Agent {
    public:
        Array2D<Move, MAX_DEPTH + MAX_EXTENSION, 2> killer_moves;
        Array2D<int, 64, 64> history[2];
        Board board;
        MemoryMappedReader opening_book;
        TranspositionTable transposition_table;
        std::shared_ptr<Board> shared_board; // Shared board state with Manager
        std::thread search_thread;
        std::mutex result_mutex;
        std::atomic<bool> should_stop{false};
        Color color;
        
        SearcherAgent(const AgentProposal& proposal, AgentProfile profile, std::shared_ptr<Board> shared_board, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            :  Agent(proposal, AgentType::Searcher, profile),
                killer_moves(), history{Array2D<int, 64, 64>(), Array2D<int, 64, 64>()},
                board(fen), opening_book("books/computer.bin"), transposition_table(), color(color), search_thread(), shared_board(shared_board) {
            // Initialize killer moves and history
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
        }
        
        void search();

        void start_search() {
            should_stop = false;
            search_thread = std::thread(&SearcherAgent::search, this);
        }

        void stop_search() {
            should_stop = true;
            if (search_thread.joinable()) {
                search_thread.join();
            }
        }

        void update_board(const Move& move) {
            stop_search();
            board.push(move);
            start_search();
        }
        
    private:
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

        inline void age_history() {
            for (int color = 0; color < 2; ++color) {
                for (int from = 0; from < 64; ++from) {
                    for (int to = 0; to < 64; ++to) {
                        history[color].insert(from, to, history[color].at(from, to) >> 3); // Divide by 8 to age the history scores
                    }
                }
            }
        }
    };

    class ManagerAgent : public Agent {
    public:
        std::array<std::shared_ptr<SearcherAgent>, 4> searchers;
        std::shared_ptr<Board> shared_board;
        Color color;
        std::mutex board_mutex;
        
        ManagerAgent(const AgentProposal& proposal, const std::shared_ptr<Board> shared_board, const Color color = WHITE, const std::array<std::shared_ptr<SearcherAgent>, 4>& searchers = {})
            : Agent(proposal, AgentType::Manager, AgentProfile::Hybrid),
              shared_board(shared_board), color(color), searchers(searchers) {}
        
        Move decide_move(int time_limit_ms) {
            // Start all searchers
            for (auto& searcher : searchers) {
                //searcher->start_search();
            }
            
            // Wait for time limit
            std::this_thread::sleep_for(std::chrono::milliseconds(time_limit_ms));
            
            // Stop all searchers
            for (auto& searcher : searchers) {
                //searcher->stop_search();
            }
            
            // Collect results and decide best move
            Move best_move = Move::null();
            int best_consensus_score = -MATE_SCORE;
            std::map<Move, int> move_votes;
            
            for (auto& searcher : searchers) {
                /* Move move = searcher->get_best_move();
                if (move != Move::null()) {
                    move_votes[move]++;
                } */
            }
            
            // Select move with most votes
            for (const auto& [move, votes] : move_votes) {
                if (votes > move_votes[best_move]) {
                    best_move = move;
                }
            }
            
            return best_move;
        }
        
        void play_move(const Move& move) {
            std::lock_guard<std::mutex> lock(board_mutex);
            shared_board->push(move);
            
            // Update all searchers to new position
            for (auto& searcher : searchers) {
                //searcher->bot.push(move);
            }
        }
    };
}