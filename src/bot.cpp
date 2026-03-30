#include "bot.hpp"

inline void Bot_space::Bot::age_history() {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 64; ++j) {
            for (int k = 0; k < 64; ++k) {
                history[i].at(j, k) = history[i].at(j, k) / 8;
            }
        }
    }
}

// Fast MVV-LVA scoring: victim value - attacker value + promotion bonus
// Removes expensive gives_check() and attackers_mask() calls to dramatically improve performance
int Bot_space::Bot::mvv_lva_score(const Move& move) {
    PieceType victim_piece_type = this->board.piece_type_at(move.to_square);
    PieceType moved_piece_type = this->board.piece_type_at(move.from_square);

    // Simple MVV-LVA: victim - attacker
    int victim = victim_piece_type != NULL_PIECE ? eg_values[victim_piece_type] : 0;
    int attacker = eg_values[moved_piece_type];
    int move_score = victim - attacker;

    // Bonus for promotion
    PieceType piece_promotion = move.promotion;
    if (piece_promotion != NULL_PIECE)
        move_score += eg_values[piece_promotion];

    return move_score;
}

void Bot_space::Bot::order_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& legal_moves, const Move& first_move, int ply) {
    this->board.generate_legal_moves(legal_moves);
    
    StaticVector<Result, LEGAL_MOVES_SIZE> scored_moves;
    
    for (const Move& move : legal_moves) {
        int score = 0;
        
        // First move priority
        if (move == first_move) {
            score = 10000000;
        } else if (this->board.is_capture(move)) {
            // Captures: use MVV-LVA
            score = this->mvv_lva_score(move) + 1000000;
        } else {
            // Quiet moves: use killer moves + history
            if (ply >= 0) {
                if (move == this->killer_moves.at(ply, 0))
                    score += 100000;
                else if (move == this->killer_moves.at(ply, 1))
                    score += 90000;
                score += history[this->board.turn].at(move.from_square, move.to_square);

                // Bonus for promotions
                if (move.promotion != NULL_PIECE) {
                    score += eg_values[move.promotion] * 1000;
                }
            }
        }
        scored_moves.push_back({move, score});
    }
    
    // Sort all moves by score
    auto cmp = [](const Result& a, const Result& b) { return a.score > b.score; };
    std::sort(scored_moves.begin(), scored_moves.end(), cmp);
    
    // Clear and rebuild legal_moves with scored moves
    legal_moves.clear();
    for (const Result& result : scored_moves) {
        legal_moves.push_back(result.move);
    }
}

int Bot_space::Bot::material_eval_only() const {
    int mg_phase = std::min(this->board.game_phase, 24);
    int eg_phase = 24 - mg_phase;
    return ((this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
             this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn]) * mg_phase +
            (this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
             this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn]) * eg_phase) / 24;
}

int Bot_space::Bot::evaluate() const {
    int mg_eval = this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
                  this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn];
    int eg_eval = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

    // Mobility.
    mg_eval += this->mobility_eval();

    int mg_phase = std::min(this->board.game_phase, 24);   // in case of early promotion
    int eg_phase = 24 - mg_phase;

    if (eg_phase > mg_phase)
        eg_eval += this->mopup_eval(eg_eval);

    return (mg_eval * mg_phase + eg_eval * eg_phase) / 24;
}

int Bot_space::Bot::mopup_eval(int eg_eval) const {
    int mopup_eval = 0;
    if (eg_eval > TWO_PAWNS) {
        Square king_square = this->board.king(this->board.turn);
        Square other_king_square = this->board.king(!this->board.turn);

        // Other king should be shoved to the edge of the board.
        mopup_eval += king_distance_to_center[other_king_square] * 47;
        // Our king should be close to the other king.
        mopup_eval += kings_distances.at(king_square, other_king_square) * 16;
    } else if (eg_eval < -TWO_PAWNS) {
        Square king_square = this->board.king(!this->board.turn);
        Square other_king_square = this->board.king(this->board.turn);

        // Our king should not be shoved to the edge of the board.
        mopup_eval -= king_distance_to_center[king_square] * 47;
        // Other king should not be close to our king.
        mopup_eval -= kings_distances.at(king_square, other_king_square) * 16;
    }
    return mopup_eval;
}

int Bot_space::Bot::mobility_eval() const {
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->board.generate_legal_moves(legal_moves);
    int turn_legal_moves_size = legal_moves.size();
    this->board.generate_legal_moves_by_color(legal_moves, !this->board.turn);
    return (turn_legal_moves_size - legal_moves.size()) * 7;
}

int Bot_space::Bot::evaluate_lazy(int alpha, int beta) const {
    int lazy = this->material_eval_only();

    if (lazy + LAZY_MARGIN <= alpha)
        return lazy;

    if (lazy - LAZY_MARGIN >= beta)
        return lazy;

    return this->evaluate();
}

std::optional<int> Bot_space::Bot::quiesce(int alpha, int beta, int q_depth) {
#ifdef TIMED
    if (this->stop)
        return std::nullopt;
#else
    if (this->timer.time() > MAX_TIME_PER_MOVE)
        return std::nullopt;
#endif
#ifdef DEBUG
    ++this->nodes_searched;
    ++this->quiescence_nodes;
#endif

    //int stand_pat = this->evaluate();
    //int stand_pat = this->material_eval_only();
    int stand_pat = this->evaluate_lazy(alpha, beta);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    if (q_depth <= 0)
        return stand_pat;

    // Quiescence considers tactical continuations only.
    StaticVector<Result, LEGAL_MOVES_SIZE> tactical_moves;

    StaticVector<Move, CAPTURES_SIZE> captures;
    this->board.generate_legal_captures(captures);
    for (const Move& move : captures) {
        tactical_moves.push_back({move, this->mvv_lva_score(move)});
    }

    // Keep non-capture promotions, which can drastically change evaluation.
    StaticVector<Move, LEGAL_MOVES_SIZE> promotion_moves;
    Bitboard promotion_rank = this->board.turn ? BB_RANK_8 : BB_RANK_1;
    this->board.generate_legal_moves(promotion_moves, this->board.pieces_mask(PAWN, this->board.turn), promotion_rank);
    for (const Move& move : promotion_moves) {
        if (move.promotion != NULL_PIECE && !this->board.is_capture(move)) {
            tactical_moves.push_back({move, 100000 + eg_values[move.promotion]});
        }
    }

    std::sort(tactical_moves.begin(), tactical_moves.end(), [](const Result& a, const Result& b) {
        return a.score > b.score;
    });

    for (const Result& result : tactical_moves) {
        this->board.push(result.move);
        std::optional<int> opt_score = this->quiesce(-beta, -alpha, q_depth - 1);
        this->board.pop();

        if (!opt_score.has_value())
            return std::nullopt;
        int score = -(*opt_score);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    
    return alpha;
}

std::optional<int> Bot_space::Bot::negamax(int depth, int alpha, int beta, int numExtensions, int ply, bool can_null) {
#ifdef TIMED
    if (this->stop)
        return std::nullopt;
#else
    if (this->timer.time() > MAX_TIME_PER_MOVE)
        return std::nullopt;
#endif
#ifdef DEBUG
    ++this->nodes_searched;
#endif

    // Skip this position if a mating sequence has already been found earlier in the search, which would be shorter
    // than any mate we could find from here. This is done by observing that alpha can't possibly be worse
    // (and likewise beta can't  possibly be better) than being mated in the current position.
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta = std::min(beta, MATE_SCORE - ply);
    if (alpha >= beta)
        return alpha;

    if (this->board.is_repetition(2) || this->board.is_insufficient_material())
        return 0;

    Move ex_best_move = Move::null();
    std::optional<TTEntry> tt_entry = this->transposition_table.get(this->board.hash());
    if (tt_entry.has_value()) {
        int tt_depth;
        int tt_score;
        NodeType n_type;
        extractMoveAndType(tt_entry->data, tt_depth, tt_score, ex_best_move, n_type);
        
        if (tt_score > IS_MATE) tt_score -= ply;
        else if (tt_score < -IS_MATE) tt_score += ply;

        if (tt_depth >= depth) {
            if (n_type == NodeType::EXACT)
                return tt_score;
            else if (n_type == NodeType::LOWER_BOUND)
                alpha = std::max(alpha, tt_score);
            else if (n_type == NodeType::UPPER_BOUND)
                beta = std::min(beta, tt_score);

            if (alpha >= beta)
                return tt_score;
        }
    }
    
    if (depth <= 0)
        return this->quiesce(alpha, beta, 6);

    bool is_check = this->board.is_check();

    // STATIC NULL MOVE
    if ((depth < 3) && (!is_check) && (this->board.material_mg[this->board.turn] > 1200)) {
        int static_eval = this->evaluate();

        int eval_margin =  120 * depth;
        if (static_eval - eval_margin >= beta)
            return static_eval - eval_margin;
    }

    // Null move pruning.
    if ((depth > 2) && (can_null) && (!is_check) && (this->board.material_mg[this->board.turn] > 1200) && (this->evaluate() >= beta)) {
        this->board.push(Move::null());
        char R = depth > 6 ? 3 : 2;  // Reduction factor
        std::optional<int> opt_score = this->negamax(depth - R - 1, -beta, -beta + 1, numExtensions, ply + 1, false);
        this->board.pop();

        if (!opt_score.has_value())
            return std::nullopt;

        int score = -(*opt_score);
        if (score >= beta)
            return score;
    }

    // FUTILITY PRUNING
    bool f_prune = false;
    if (depth <= 3 && !is_check && std::abs(alpha) < 9000 && this->evaluate() + futility_margin[depth] <= alpha)
        f_prune = true;

    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->order_moves(legal_moves, ex_best_move, ply);
    if (legal_moves.empty())
        return is_check ? -MATE_SCORE + ply : 0;  // No legal moves, return checkmate or stalemate score.
    else if (this->board.halfmove_clock >= 100)
        return 0;  // Fifty-move rule draw.

    Move best_move = Move::null();
    NodeType node_type = NodeType::UPPER_BOUND;
    int i = 0;
    for (const Move& move : legal_moves) {
        bool is_capture = this->board.is_capture(move);
        this->board.push(move);
        bool in_check = this->board.is_check();

        // FUTILITY PRUNING
        if (f_prune && !is_capture && move.promotion == NULL_PIECE && !in_check) {
            this->board.pop();
            continue;
        }

        // Calculate extension depth.
        int extension = 0;
        if (numExtensions < MAX_EXTENSION) {
            PieceType moved_piece_type = this->board.piece_type_at(move.to_square);
            int target_rank = square_rank(move.to_square);
            if (in_check || (moved_piece_type == PAWN && (target_rank == 1 || target_rank == 6)))
                extension = 1;
        }

        // Move reduction.
        bool needs_full_search = true;
        std::optional<int> opt_score = std::nullopt;
        if (extension == 0 && depth >= 3 && i >= 3 && !is_capture) {
            opt_score = this->negamax(depth - 2, -alpha - 1, -alpha, numExtensions, ply + 1);
            if (opt_score.has_value())
                needs_full_search = -(*opt_score) > alpha;
        }

        if (needs_full_search)
            opt_score = this->negamax(depth - 1 + extension, -beta, -alpha, numExtensions + extension, ply + 1);
        
        this->board.pop();

        if (!opt_score.has_value())
            return std::nullopt;
        int score = -(*opt_score);
        
        if (score > alpha) {
            node_type = NodeType::EXACT;
            alpha = score;
            best_move = move;
        }

        if (score >= beta) {
            node_type = NodeType::LOWER_BOUND;
            // killer moves
            if (!is_capture) {
                // Killer move heuristic
                if (this->killer_moves.at(ply, 0) != move) {
                    this->killer_moves.insert(ply, 1, this->killer_moves.at(ply, 0));
                    this->killer_moves.insert(ply, 0, move);
                }

                // History heuristic using the gravity formula.
                history[this->board.turn].at(move.from_square, move.to_square) += depth * depth;
            }
            break;
        }

        ++i;
    }

    this->transposition_table.store(this->board.hash(), depth, alpha, best_move, node_type);

    return alpha;
}

Bot_space::Result Bot_space::Bot::root_move(int depth, int alpha, int beta, const Move& ex_best_move) {
    if (this->board.is_repetition(3) || this->board.is_insufficient_material())
        return {Move::null(), 0};

    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->order_moves(legal_moves, ex_best_move, 0);
    if (legal_moves.empty())
        return {Move::null(), this->board.is_check() ? -MATE_SCORE : 0};  // No legal moves, return checkmate or stalemate score.
    else if (this->board.halfmove_clock >= 100)
        return {Move::null(), 0};  // Fifty-move rule draw.

    Move best_move = Move::null();
    NodeType node_type = NodeType::UPPER_BOUND;
    int i = 0;
    for (const Move& move : legal_moves) {
        bool is_capture = this->board.is_capture(move);
        this->board.push(move);

        // Calculate extension depth.
        int extension = 0;
        PieceType moved_piece_type = this->board.piece_type_at(move.to_square);
        int target_rank = square_rank(move.to_square);
        if (this->board.is_check() || (moved_piece_type == PAWN && (target_rank == 1 || target_rank == 6)))
            extension = 1;

        // Move reduction.
        bool needs_full_search = true;
        std::optional<int> opt_score = std::nullopt;
        if (extension == 0 && depth >= 3 && i >= 3 && !is_capture) {
            opt_score = this->negamax(depth - 2, -alpha - 1, -alpha, extension, 1);
            if (opt_score.has_value())
                needs_full_search = -(*opt_score) > alpha;
        }

        if (needs_full_search)
            opt_score = this->negamax(depth - 1 + extension, -beta, -alpha, extension, 1);
        
        this->board.pop();

        if (!opt_score.has_value()) {
            // Save partial best_move to TT before returning on timeout
            if (static_cast<bool>(best_move)) {
                this->transposition_table.store(this->board.hash(), depth, alpha, best_move, node_type);
            }
            return {best_move, alpha};
        }
        int score = -(*opt_score);
        
        if (score > alpha) {
            node_type = NodeType::EXACT;
            alpha = score;
            best_move = move;
        }

        if (score >= beta) {
            node_type = NodeType::LOWER_BOUND;
            // killer moves
            if (!is_capture) {
                // Killer move heuristic
                if (this->killer_moves.at(0, 0) != move) {
                    this->killer_moves.insert(0, 1, this->killer_moves.at(0, 0));
                    this->killer_moves.insert(0, 0, move);
                }

                // History heuristic using the gravity formula.
                history[this->board.turn].at(move.from_square, move.to_square) += depth * depth;
            }
            break;
        }

        ++i;
    }

    this->transposition_table.store(this->board.hash(), depth, alpha, best_move, node_type);

    return {best_move, alpha};
}

Move Bot_space::Bot::get_move() {
    this->clear_for_new_search();   // start timer, reset nodes, clear search data structures

    // Move opening_move = Move::null();
    // if (this->board.fullmove_number < 5)
    Move opening_move = this->opening_book.weighted_choice(this->board).move;
    //Move opening_move = MemoryMappedReader("books/computer.bin").weighted_choice(this->board).move;

    Move best_move = Move::null();
    int best_eval = -MATE_SCORE;
    int depth = 1;
    for (; depth <= MAX_DEPTH; ++depth) {
#ifdef TIMED
        if (this->stop)
            break;
#else
        if (this->timer.time() > MAX_TIME_PER_MOVE)
            break;
#endif

        // If no prior score (depth == 1), do full-window. Or if mate score found.
        if (best_eval == -MATE_SCORE || depth == 1 || is_mate_score(best_eval)) {
            // Full-window search
            const Result& result = this->root_move(depth, -MATE_SCORE, MATE_SCORE, best_move);

            if (!static_cast<bool>(result.move))
                break; // Time's up

            best_eval = result.score;
            best_move = result.move;

            continue;
        }

        // Aspiration loop
        int a_delta = INITIAL_ASP;
        int b_delta = INITIAL_ASP;
        int retries = 0;
        //bool success = false;
        int score = best_eval;

        while (true) {
            // clamp alpha/beta to avoid integer overflow and handle mates
            int alpha = score - a_delta;
            int beta  = score + b_delta;

            // If window covers full range or exceeded MAX_ASP_RETRIES, request full-window (no benefit to aspiration)
            if (alpha <= -MATE_SCORE || beta >= MATE_SCORE || retries > MAX_ASP_RETRIES) {
                const Result& result = this->root_move(depth, -MATE_SCORE, MATE_SCORE, best_move);

                if (!static_cast<bool>(result.move))
                    break; // Time's up

                best_eval = result.score;
                best_move = result.move;
                //success = true;

                break;
            }

            // Call negamax with the aspiration window
            const Result& result = this->root_move(depth, alpha, beta, best_move);
#ifdef TIMED
            if (!static_cast<bool>(result.move) && this->stop)
                break;
#else
            if (!static_cast<bool>(result.move) && this->timer.time() > MAX_TIME_PER_MOVE) {
                // timeout -> abort whole ID search
                break;
            }
#endif
            score = result.score;

            // Detect fail-low / fail-high / success
            if (score <= alpha) {
                // fail-low: true score <= alpha. Widen and retry.
                a_delta *= 2;
            } else if (score >= beta) {
                // fail-high: true score >= beta. Widen and retry.
                b_delta *= 2;
            } else {
                // Success: result inside window
                best_eval = score;
                best_move = result.move;
                //success = true;
                break;
            }
            ++retries;
        } // end aspiration loop

        /*
        if (!success) {
            // Defensive: do a final full-window search (shouldn't happen)
            auto opt_score = this->negamax(depth, -MATE_SCORE, MATE_SCORE, 0, 0, true);
            if (!opt_score.has_value())
                return std::nullopt;
            prev_score = *opt_score;
        }
        */
#ifdef DEBUG
        printf("[INFO] Depth %d Score %d Nodes %llu Time %.2f ms PV ", depth, best_eval, this->nodes_searched, this->timer.time_ms());
        this->get_pv_line(depth);
        for (const Move& m : this->pv_line)
            printf("%s ", std::string(m).c_str());
        printf("\n");
#endif

        if (is_mate_score(best_eval) && ply_to_mate_from_score(best_eval) <= depth)
            break;
    }

#ifdef DEBUG
    if (is_mate_score(best_eval)) {
        int moves_to_mate = moves_to_mate_from_score(best_eval);
        if (best_eval > 0)
            printf("Depth reached: %d, Best move: %s, Mating in %d\n", depth - 1, std::string(best_move).c_str(), moves_to_mate);
        else
            printf("Depth reached: %d, Best move: %s, Getting mated in %d\n", depth - 1, std::string(best_move).c_str(), moves_to_mate);
    } else {
        best_eval = color ? best_eval : -best_eval;
        printf("Depth reached: %d, Best move: %s, Best score: %d\n", depth - 1, std::string(best_move).c_str(), best_eval);
    }
    if (opening_move)
        printf("Using opening book move: %s\n", std::string(opening_move).c_str());
    printf("Time taken: %f ms\n", timer.time());
    printf("Transposition table load factor: %.2f%%\n", transposition_table.getLoadFactor() * 100);
    printf("Hit rate: %.2f%%\n", transposition_table.getHitRate() * 100);
    printf("Absolute hits: %d\n", transposition_table.getHits());
    printf("Absolute misses: %d\n", transposition_table.getMisses());
    printf("Total accesses: %d\n", transposition_table.getTotalAccesses());
    printf("Nodes searched: %llu\n", this->nodes_searched);
    printf("Quiescence nodes searched: %llu\n", this->quiescence_nodes);
#endif

    return opening_move ? opening_move : best_move;
}

inline void Bot_space::Bot::clear_for_new_search() {
    this->timer.start();
    this->age_history();
    this->killer_moves.fill(Move::null());
    this->transposition_table.increment_age();
    
#ifdef TIMED
    this->stop = false;
#endif
#ifdef DEBUG
    this->nodes_searched = 0;
    this->quiescence_nodes = 0;
    this->transposition_table.reset_stats();
#endif
}

#ifdef DEBUG
void Bot_space::Bot::get_pv_line(int depth) {
    this->pv_line.clear();
    Move move;
    extractMove(this->transposition_table.get(this->board.hash())->data, move);
    for (int count = 0; move && count < depth; ++count) {
        this->pv_line.push_back(move);
        this->board.push(move);
        extractMove(this->transposition_table.get(this->board.hash())->data, move);
    }

    for (size_t i = 0; i < this->pv_line.size(); ++i)
        this->board.pop();
}
#endif
