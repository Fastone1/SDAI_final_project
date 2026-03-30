#pragma once
#include "chess.hpp"
#include "defs.hpp"
#include "tt.hpp"
#include "static_vector.hpp"
#include "array2d.hpp"
#include "polyglot.hpp"

#include <array>
#include <chrono>
#include <unordered_map>
#include <optional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <limits>

#ifdef DEBUG
#include <iostream> // only for debugging purposes
#endif

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
    constexpr int N_SEARCHERS = 4;

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

    const std::array<std::string, 4> profile_names = {"Opening", "Middlegame", "Endgame", "Hybrid"};

    struct Result {
        Move move;
        int score;
    };

    // Proposal structure for Searcher Agents to send to Manager
    struct AgentProposal {
        int score_cp = 0; // score in centipawns
        int depth_reached = 0;
        Move best_move = Move::null();
        AgentProfile profile;
    };

    /*
    Environment class to represent the chess board and handle moves updates between Agents and Users.
    */
    class Environment {
    private:
        Board env_board;
        std::mutex board_mutex;

        // Moves updates
        unsigned int move_count = 0;
        Move last_move = Move::null();
        std::condition_variable move_cv;
        
    public:
        // Signal to indicate when the game is over or when to stop waiting for moves
        std::atomic<bool> should_stop{false};
    
        Environment(const std::optional<std::string>& fen = std::nullopt) : env_board(fen) {}

        void make_move(const Move& move, unsigned int& user_move_count) {
            std::lock_guard<std::mutex> lock(board_mutex);
            last_move = move;
            ++user_move_count; // Update the caller's move count to reflect the new move
            ++move_count;
            env_board.push(move);
            move_cv.notify_all();
        }

        Move wait_for_move(unsigned int& user_move_count) {
            std::unique_lock<std::mutex> lock(board_mutex);
            // Wait until a new move is made on the board (last_move is updated)
            move_cv.wait(lock, [this, &user_move_count] { return move_count > user_move_count || should_stop.load(std::memory_order_acquire); });
            ++user_move_count; // Update the last seen move count for the caller
            return last_move;
        }

        bool is_game_over() {
            std::lock_guard<std::mutex> lock(board_mutex);
            return env_board.is_game_over();
        }

        Color get_turn() {
            std::lock_guard<std::mutex> lock(board_mutex);
            return env_board.turn;
        }

        Board get_board() {
            std::lock_guard<std::mutex> lock(board_mutex);
            return env_board;
        }

        void notify_stop() {
            {
                std::lock_guard<std::mutex> lock(board_mutex);
                ++move_count;   // Increment move count to unblock any waiting Searcher Agents
                should_stop.store(true, std::memory_order_release);
            }
            move_cv.notify_all();
        }
    };

    class Comms {
        private:
            // Searchers' proposals communication
            std::array<AgentProposal, N_SEARCHERS> agentProposals;
            std::array<std::mutex, N_SEARCHERS> proposal_mutexs;
        public:
            Comms() {
                for (int i = 0; i < N_SEARCHERS; ++i) {
                    agentProposals[i].best_move = Move::null();
                    agentProposals[i].score_cp = 0;
                    agentProposals[i].depth_reached = 0;
                    agentProposals[i].profile = static_cast<AgentProfile>(i);
                }
            }

            void update_proposal(int idx, const Move& move, int score_cp, int depth_reached) {
                std::lock_guard<std::mutex> lock(proposal_mutexs[idx]);
                agentProposals[idx].best_move = move;
                agentProposals[idx].score_cp = score_cp;
                agentProposals[idx].depth_reached = depth_reached;
            }

            std::array<AgentProposal, N_SEARCHERS> get_proposals() {
                std::array<AgentProposal, N_SEARCHERS> proposals_copy;
                for (int i = 0; i < N_SEARCHERS; ++i) {
                    std::lock_guard<std::mutex> lock(proposal_mutexs[i]);
                    proposals_copy[i] = agentProposals[i];
                    // Reset the proposal after copying to avoid stale data for the next round of search
                    agentProposals[i].best_move = Move::null();
                    agentProposals[i].score_cp = 0;
                    agentProposals[i].depth_reached = 0;
                }
                return proposals_copy;
            }
    };

    class Agent {
    public:
        std::shared_ptr<Environment> env;
        std::shared_ptr<Comms> comms;
        AgentType type;
        AgentProfile profile;
        Color color;
        int id;
        inline static int id_counter;
        
        Agent(AgentType type, AgentProfile profile, std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, Color color) 
            : env(env), comms(comms), type(type), profile(profile), color(color) {
            id = ++id_counter;
        }
        
        virtual ~Agent() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
    };
    
    class ManagerAgent : public Agent {
    public:
        struct MoveHash {
            std::size_t operator()(const Move& move) const noexcept {
                const std::uint32_t packed =
                    (static_cast<std::uint32_t>(move.from_square) & 0xFFu) |
                    ((static_cast<std::uint32_t>(move.to_square) & 0xFFu) << 8) |
                    ((static_cast<std::uint32_t>(move.promotion) & 0x0Fu) << 16);
                return std::hash<std::uint32_t>{}(packed);
            }
        };

        struct AggregatedMoveStats {
            double weighted_vote_sum = 0.0;
            int max_depth = -1;
            double trust_sum = 0.0;
            int contributors = 0;
            double score_sum = 0.0;
        };

        static constexpr double INITIAL_TRUST = 0.5;
        static constexpr double TRUST_LR = 0.05;
        static constexpr double TRUST_MIN = 0.05;
        static constexpr double TRUST_MAX = 0.95;

        MemoryMappedReader opening_book;
        std::thread manager_thread;
        std::array<double, N_SEARCHERS> trust;
        
        ManagerAgent(std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const Color color = WHITE)
                    : Agent(AgentType::Manager, AgentProfile::Hybrid, env, comms, color), opening_book("books/Titans.bin"), trust{} {
            trust.fill(INITIAL_TRUST);
        }

        ~ManagerAgent() {
            stop();
        }

        void update_trust(const std::array<AgentProposal, N_SEARCHERS>& proposals, const Move& chosen_move, int chosen_max_depth, double chosen_avg_score) {
            for (int i = 0; i < N_SEARCHERS; ++i) {
                const AgentProposal& proposal = proposals[i];
                double reward = 0.0;

                if (proposal.best_move == Move::null() || proposal.depth_reached <= 0) {
                    reward = 0.0;
                } else if (proposal.best_move == chosen_move) {
                    reward = 1.0;
                } else if (proposal.depth_reached >= std::max(0, chosen_max_depth - 1) &&
                            std::abs(static_cast<double>(proposal.score_cp) - chosen_avg_score) <= 80.0) {
                    // If the proposal was close to the chosen move in terms of depth and score, give a partial reward
                    reward = 0.7;
                } else if (proposal.depth_reached >= std::max(0, chosen_max_depth - 2) &&
                            std::abs(static_cast<double>(proposal.score_cp) - chosen_avg_score) <= 150.0) {
                    // If the proposal was close to the chosen move in terms of depth and score, give a partial reward
                    reward = 0.3;
                } else {
                    reward = 0.0;
                }

                const double updated = (1.0 - TRUST_LR) * trust[i] + TRUST_LR * reward;
                trust[i] = std::clamp(updated, TRUST_MIN, TRUST_MAX);
            }
        }
        
        Move decide_move() {
            // Wait for time limit
            std::this_thread::sleep_for(std::chrono::seconds(MAX_TIME_PER_MOVE));

            // Collect proposals from Searcher Agents
            std::array<AgentProposal, N_SEARCHERS> proposals = comms->get_proposals();

#ifdef DEBUG
            for (const AgentProposal& proposal : proposals) {
                std::cout << "Best Move: " << static_cast<std::string>(proposal.best_move)
                            << ", Score (cp): " << proposal.score_cp
                            << ", Depth Reached: " << proposal.depth_reached
                            << ", Profile: " << profile_names[static_cast<int>(proposal.profile)] << "\n";
            }
#endif

            // Check for opening book move
            Board current_board = env->get_board();
            Move book_move = opening_book.weighted_choice(current_board).move;
            if (book_move != Move::null()) {
#ifdef DEBUG
                std::cout << "Book move found: " << static_cast<std::string>(book_move) << "\n";
#endif
                return book_move;
            }

            int min_value = MATE_SCORE;
            bool has_valid = false;
            for (const AgentProposal& proposal : proposals) {
                if (proposal.best_move == Move::null() || proposal.depth_reached <= 0) {
                    continue;
                }
                has_valid = true;
                min_value = std::min(min_value, proposal.score_cp);
            }

            if (!has_valid) {
#ifdef DEBUG
                std::cout << "No valid proposal received from Searchers.\n";
#endif
                return Move::null();
            }

            // Cast votes based on score and depth, weighted by trust
            std::unordered_map<Move, int, MoveHash> votes;
            for (const AgentProposal& proposal : proposals) {
                if (proposal.best_move == Move::null() || proposal.depth_reached <= 0) {
                    continue;
                }

                const int base_vote = (proposal.score_cp - min_value + 11) * proposal.depth_reached;
                const int trust_scale = static_cast<int>(trust[static_cast<int>(proposal.profile)] * 1000.0);
                votes[proposal.best_move] += base_vote * std::max(1, trust_scale);
            }

            // Find the first valid proposal to initialize the best idx
            int best_idx = -1;
            for (int i = 0; i < N_SEARCHERS; ++i) {
                if (proposals[i].best_move != Move::null() && proposals[i].depth_reached > 0) {
                    best_idx = i;
                    break;
                }
            }
            // Compare proposals to find the best move
            for (int i = 0; i < N_SEARCHERS; ++i) {
                const AgentProposal& candidate = proposals[i];
                if (candidate.best_move == Move::null() || candidate.depth_reached <= 0) {
                    continue;
                }
                if (best_idx < 0) {
                    best_idx = i;
                    continue;
                }

                const AgentProposal& best = proposals[best_idx];
                const int cand_value = candidate.score_cp;
                const int best_value = best.score_cp;
                const Move cand_move = candidate.best_move;
                const Move best_move_current = best.best_move;

                // In case of checkmate scores, prefer shorter mate / longer being mated.
                if (std::abs(best_value) >= IS_MATE) {
                    if (cand_value > best_value) {
                        best_idx = i;
                    }
                }
                // Otherwise choose by vote table.
                else if (votes[cand_move] > votes[best_move_current]) {
                    best_idx = i;
                }
                // Same move: prefer stronger score, then deeper search.
                else if (cand_move == best_move_current &&
                         (std::abs(cand_value) > std::abs(best_value) ||
                          (std::abs(cand_value) == std::abs(best_value) && candidate.depth_reached > best.depth_reached))) {
                    best_idx = i;
                }
            }

            if (best_idx < 0) {
#ifdef DEBUG
                std::cout << "No valid proposal received from Searchers.\n";
#endif
                return Move::null();
            }

            const Move best_move = proposals[best_idx].best_move;

            int chosen_max_depth = -1;
            double chosen_score_sum = 0.0;
            double chosen_trust_sum = 0.0;
            int chosen_contributors = 0;
            for (int i = 0; i < N_SEARCHERS; ++i) {
                const AgentProposal& proposal = proposals[i];
                if (proposal.best_move != best_move || proposal.depth_reached <= 0) {
                    continue;
                }
                chosen_max_depth = std::max(chosen_max_depth, proposal.depth_reached);
                chosen_score_sum += static_cast<double>(proposal.score_cp);
                chosen_trust_sum += trust[i];
                ++chosen_contributors;
            }

            if (chosen_max_depth < 0) {
                chosen_max_depth = proposals[best_idx].depth_reached;
            }
            const double best_avg_score =
                chosen_contributors > 0 ? (chosen_score_sum / static_cast<double>(chosen_contributors))
                                        : static_cast<double>(proposals[best_idx].score_cp);

            update_trust(proposals, best_move, chosen_max_depth, best_avg_score);

#ifdef DEBUG
            std::cout << "Chosen Move: " << static_cast<std::string>(best_move)
                      << " | votes: " << votes[best_move]
                      << " | max depth: " << chosen_max_depth
                      << " | avg score: " << best_avg_score << "\n";
            std::cout << "Updated trust: ["
                      << trust[0] << ", " << trust[1] << ", " << trust[2] << ", " << trust[3]
                      << "]\n";
#endif

            return best_move;
        }

        void start() {
            manager_thread = std::thread(&ManagerAgent::run, this);
        }

        void run() {
            unsigned int move_count = 0;
            while (!env->should_stop.load(std::memory_order_acquire) && !env->is_game_over()) {
                Move move;
                if (env->get_turn() == color) {
                    // It's the Manager's turn to move
                    move = decide_move();
                    if (move != Move::null()) {
                        env->make_move(move, move_count);
                    }
                } else {
                    // Wait for opponent's move (could be from a user or another agent)
                    env->wait_for_move(move_count);
                }
            }
            env->notify_stop(); // Notify Searchers to stop
        }

        void stop() {
            if (manager_thread.joinable()) {
                manager_thread.join();
            }
        }
    };

    class SearcherAgent : public Agent {
    public:
        Array2D<Move, MAX_DEPTH + MAX_EXTENSION, 2> killer_moves;
        Array2D<int, 64, 64> history[2];
        Board board;
        TranspositionTable transposition_table;

        std::thread search_thread;
        std::thread comm_thread;

        Move updated_move = Move::null();
        unsigned int move_count = 0; 
        std::atomic<bool> should_pause{false};
        
        SearcherAgent(AgentProfile profile, std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            :  Agent(AgentType::Searcher, profile, env, comms, color), // Default color, adjust as needed
                killer_moves(), history{Array2D<int, 64, 64>(), Array2D<int, 64, 64>()},
                board(fen), transposition_table(), search_thread(), comm_thread() {
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

        ~SearcherAgent() {
            stop();
        }

        void update_proposal(const Move& move, int score_cp, int depth_reached) {
            int idx = static_cast<int>(profile);
            comms->update_proposal(idx, move, score_cp, depth_reached);
        }

        void search() {
            while (!env->should_stop.load(std::memory_order_acquire) && !board.is_game_over()) {
                //printf("Searcher Agent (%s, %d) starting new search iteration. Move count: %u\n", profile_names[static_cast<int>(profile)].c_str(), id, move_count);
                // Perform search and update proposal
                this->get_move();
                // Check for move updates
                if (static_cast<bool>(updated_move)) {
                    board.push(updated_move); // Update the board with the new move
                    updated_move = Move::null(); // Reset the updated move
                    should_pause.store(false, std::memory_order_release); // Reset pause flag for next search
                }
            }
        }

        void comms_loop() {
            while (!env->should_stop.load(std::memory_order_acquire)) {
                // Wait for move updates from the Manager
                updated_move = env->wait_for_move(move_count);

                // Search thread should pause
                should_pause.store(true, std::memory_order_release);
            }
        }

        void start() {
            should_pause.store(false, std::memory_order_release);
            search_thread = std::thread(&SearcherAgent::search, this);
            comm_thread = std::thread(&SearcherAgent::comms_loop, this);
        }

        void stop() {
            should_pause.store(true, std::memory_order_release);
            if (comm_thread.joinable()) {
                comm_thread.join();
            }
            if (search_thread.joinable()) {
                search_thread.join();
            }
        };
        
    private:
        int mvv_lva_score(const Move& move);

        void order_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& moves, const Move& first_move = Move::null(), int ply = -1);

        std::optional<int> quiesce(int alpha, int beta, int q_depth = 6);

        std::optional<int> negamax(int depth, int alpha, int beta, int numExtensions = 0, int ply = 1, bool can_null = true);

        Result root_move(int depth, int alpha = -MATE_SCORE, int beta = MATE_SCORE, const Move& ex_best_move = Move::null());

        void get_move();

        inline void age_history();

    protected:
        virtual int evaluate() const = 0; 

        int mopup_eval(int eg_eval) const;
    };

    class EarlyGameAgent : public SearcherAgent {
    public:
        EarlyGameAgent(std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            : SearcherAgent(AgentProfile::Opening, env, comms, fen, color) {}

    protected:
        int evaluate() const override {
            int score = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

            // Penalize undeveloped pieces (e.g., pieces still on their original squares)
            Bitboard starting_squares = color == WHITE ? BB_RANK_1 : BB_RANK_8;
            if (board.pieces_mask(KNIGHT, color) & starting_squares & BB_B1 & BB_G1 & BB_B8 & BB_G8) {
                score -= 20; // Penalize knights on original squares
            }
            if (board.pieces_mask(BISHOP, color) & starting_squares & BB_C1 & BB_F1 & BB_C8 & BB_F8) {
                score -= 20; // Penalize bishops on original squares
            }

            return score;
        }
    };

    class LateGameAgent : public SearcherAgent {
    public:
        LateGameAgent(std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            : SearcherAgent(AgentProfile::Endgame, env, comms, fen, color) {}

    protected:
        int evaluate() const override {
            int score = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

            // Bonus for king activity in the endgame
            score += this->mopup_eval(score);

            return score;
        }
    };

    class MidGameAgent : public SearcherAgent {
    public:
        MidGameAgent(std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            : SearcherAgent(AgentProfile::Middlegame, env, comms, fen, color) {}

    protected:
         int evaluate() const override {
            int mg_score = this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
                  this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn];

            int eg_score = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

                  
            int mg_phase = std::min(24, this->board.game_phase);
            int eg_phase = 24 - mg_phase;

            return (mg_score * mg_phase + eg_score * eg_phase) / 24;
        }
    };

    class HybridAgent : public SearcherAgent {
    public:
        HybridAgent(std::shared_ptr<Environment> env, std::shared_ptr<Comms> comms, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE)
            : SearcherAgent(AgentProfile::Hybrid, env, comms, fen, color) {}

    protected:
        int evaluate() const override {
            int mg_score = this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
                  this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn];

            int eg_score = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

                  
            int mg_phase = std::min(24, this->board.game_phase);
            int eg_phase = 24 - mg_phase;
            
            // Bonus for king activity in the endgame
            if (eg_phase > mg_phase) {
                eg_score += this->mopup_eval(eg_score);
            }

            // Mobility bonus in the midgame
            StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
            this->board.generate_legal_moves(legal_moves);
            int turn_legal_moves_size = legal_moves.size();
            this->board.generate_legal_moves_by_color(legal_moves, !this->board.turn);
            mg_score += (turn_legal_moves_size - legal_moves.size()) * 7;

            // Combine midgame and endgame scores based on the game phase
            return (mg_score * mg_phase + eg_score * eg_phase) / 24;
        }
    };

    /*
    MAS class to manage the Agents (one Manager, multiple Searchers) and interact with the Environment.
    */
    class MAS {
        private:
            std::shared_ptr<Environment> env;
            std::shared_ptr<Comms> comms;
            std::unique_ptr<ManagerAgent> manager;
            std::array<std::unique_ptr<SearcherAgent>, N_SEARCHERS> searchers;
        
        public:
            MAS(std::shared_ptr<Environment> env, const std::optional<std::string>& fen = std::nullopt, Color color = WHITE) : env(env) {
                comms = std::make_shared<Comms>();
                manager = std::make_unique<ManagerAgent>(env, comms, color);
                searchers[0] = std::make_unique<EarlyGameAgent>(env, comms, fen, color);
                searchers[1] = std::make_unique<MidGameAgent>(env, comms, fen, color);
                searchers[2] = std::make_unique<LateGameAgent>(env, comms, fen, color);
                searchers[3] = std::make_unique<HybridAgent>(env, comms, fen, color);
            }

            ~MAS() {
                stop();
            }

            void start() {
                manager->start();
                for (auto& searcher : searchers) {
                    searcher->start();
                }
            }

            void stop() {
                env->notify_stop(); // Notify threads to stop waiting for moves
                manager->stop();
                for (auto& searcher : searchers) {
                    searcher->stop();
                }
            }

            std::array<double, N_SEARCHERS> get_trust() const {
                return manager->trust;
            }
    };

} // namespace Agents