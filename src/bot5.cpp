#include "bot5.hpp"

inline void Bot5_space::Bot5::age_history() {
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 64; ++j) {
            for (int k = 0; k < 64; ++k) {
                history[i].at(j, k) = history[i].at(j, k) / 8;
            }
        }
    }
}

Array2D<int, 64, 64> Bot5_space::precompute_kings_distances() {
    Array2D<int, 64, 64> distances = Array2D<int, 64, 64>();
    for (Square square1 : SQUARES) {
        for (Square square2 : SQUARES) {
            distances.insert(square1, square2, 14 - square_manhattan_distance(square1, square2));
        }
    }
    return distances;
}

const Array2D<int, 64, 64> Bot5_space::kings_distances = Bot5_space::precompute_kings_distances();

Bot5_space::PassedPawnMasks Bot5_space::precompute_masks() {
    std::array<Bitboard, 64> white_passed_pawns_mask;
    std::array<Bitboard, 64> black_passed_pawns_mask;
    std::array<Bitboard, 64> white_pawn_support_mask;
    std::array<Bitboard, 64> black_pawn_support_mask;
    std::array<Bitboard, 64> white_knight_support_mask;
    std::array<Bitboard, 64> black_knight_support_mask;
    std::array<Bitboard, 64> white_pawn_shield_mask;
    std::array<Bitboard, 64> black_pawn_shield_mask;
    std::array<Bitboard, 64> king_zone_mask;

    for (Square square : SQUARES) {
        int file = square_file(square);
        int rank = square_rank(square);

        Bitboard adjacent_files = BB_FILE_A << std::max(0, file - 1) |
                                 (BB_FILE_A << std::min(7, file + 1));

        Bitboard white_forward_mask = ~(BB_ALL >> (64 - 8 * (rank + 1)));
        Bitboard black_forward_mask = ((BB_A1 << 8 * rank) - 1);

        white_passed_pawns_mask[square] = (BB_FILE_A << file | adjacent_files) & white_forward_mask;
        black_passed_pawns_mask[square] = (BB_FILE_A << file | adjacent_files) & black_forward_mask;

        Bitboard adjacent = (BB_A1 << (square - 1) | BB_A1 << (square + 1)) & adjacent_files;
        white_pawn_support_mask[square] = adjacent | (adjacent >> 8);
        black_pawn_support_mask[square] = adjacent | (adjacent << 8);
        white_knight_support_mask[square] = adjacent >> 8;
        black_knight_support_mask[square] = adjacent << 8;

        // squares of friendly pawns that form the typical "shield" in front of the king (3 pawn squares in front-left/center/front-right)
        white_pawn_shield_mask[square] = ((BB_A1 << (square + 7)) | (BB_A1 << (square + 8)) | (BB_A1 << (square + 9))) & BB_ALL;
        black_pawn_shield_mask[square] = ((BB_A1 << (square - 7)) | (BB_A1 << (square - 8)) | (BB_A1 << (square - 9))) & BB_ALL;

        // squares around king where attacks matter (eg. the 16-square zone: king square + adjacent files/ranks + a ring)
        king_zone_mask[square] = 0;
        for (int df = -2; df <= 2; ++df) {
            for (int dr = -2; dr <= 2; ++dr) {
                int f = file + df;
                int r = rank + dr;
                if (f >= 0 && f < 8 && r >= 0 && r < 8) {
                    king_zone_mask[square] |= (BB_A1 << (r * 8 + f));
                }
            }
        }
    }

    return {white_passed_pawns_mask, black_passed_pawns_mask, white_pawn_support_mask, black_pawn_support_mask, white_knight_support_mask, black_knight_support_mask, white_pawn_shield_mask, black_pawn_shield_mask, king_zone_mask};
}

const Bot5_space::PassedPawnMasks& Bot5_space::precomputed_masks = Bot5_space::precompute_masks();

std::array<Bitboard, 8> Bot5_space::precompute_adjacent_files_masks() {
    std::array<Bitboard, 8> masks;
    for (int i = 0; i < 8; ++i) {
        Bitboard left = (i > 0) ? (BB_FILE_A << (i - 1)) : 0;
        Bitboard right = (i < 7) ? (BB_FILE_A << (i + 1)) : 0;
        masks[i] = left | right;
    }
    return masks;
}

const std::array<Bitboard, 8>& Bot5_space::adjacent_files_masks = Bot5_space::precompute_adjacent_files_masks();

int Bot5_space::ply_to_mate_from_score(int score) {
    return MATE_SCORE - std::abs(score);
}

int Bot5_space::moves_to_mate_from_score(int score) {
    return (MATE_SCORE - std::abs(score) + 1) / 2;
}

int Bot5_space::Bot5::mvv_lva_score(const Move& move) {
    std::optional<Piece> victim_piece = this->board.piece_at(move.to_square);
    PieceType moved_piece_type = this->board.piece_type_at(move.from_square);

    int attacker = eg_values[moved_piece_type];
    int victim = victim_piece.has_value() ? eg_values[victim_piece->piece_type] : 0;
    
    Color turn = this->board.turn;
    int move_score = 0;

    if (victim != 0)
        move_score = victim - attacker;

    PieceType piece_promotion = move.promotion;
    if (piece_promotion != NULL_PIECE)
        move_score += eg_values[piece_promotion];

    if (this->board.gives_check(move))
        move_score += eg_values[KING];

    Bitboard attackers_sq = this->board.attackers_mask(!turn, move.to_square);
    Bitboard enemy_pawns = this->board.pieces_mask(PAWN, !turn);
    if ((attackers_sq & enemy_pawns) || (attackers_sq && moved_piece_type == QUEEN))
        move_score -= attacker;

    return move_score;
}

void Bot5_space::Bot5::order_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& legal_moves, const Move& first_move, int ply) {
    this->board.generate_legal_moves(legal_moves);

    // Sort by MVV-LVA.
    std::sort(legal_moves.begin(), legal_moves.end(), [this, ply, &first_move](const Move& move1, const Move& move2) {
        if (move1 == first_move)
            return true;
        if (move2 == first_move)
            return false;

        int score1 = this->mvv_lva_score(move1) * 1000;
        int score2 = this->mvv_lva_score(move2) * 1000;

        if (ply < 0) {
            // If ply is negative, we don't use killer moves or history heuristic.
            return score1 > score2;
        }

        // Killer move bonus.
        if (!this->board.is_capture(move1)) {
            if (move1 == this->killer_moves.at(ply, 0))
                score1 += 100000;
            else if (move1 == this->killer_moves.at(ply, 1))
                score1 += 90000;
        }

        if (!this->board.is_capture(move2)) {
            if (move2 == this->killer_moves.at(ply, 0))
                score2 += 100000;
            else if (move2 == this->killer_moves.at(ply, 1))
                score2 += 90000;
        }

        // History heuristic.
        Color turn = this->board.turn;
        score1 += history[turn].at(move1.from_square, move1.to_square);
        score2 += history[turn].at(move2.from_square, move2.to_square);

        return score1 > score2;
    });
}

int Bot5_space::Bot5::material_eval_only() const {
    int mg_phase = std::min(this->board.game_phase, 24);
    int eg_phase = 24 - mg_phase;
    return ((this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
             this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn]) * mg_phase +
            (this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
             this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn]) * eg_phase) / 24;
}

int Bot5_space::Bot5::evaluate() const {
    int mg_eval = this->board.material_mg[this->board.turn] + this->board.pawns_mg[this->board.turn] -
                  this->board.material_mg[!this->board.turn] - this->board.pawns_mg[!this->board.turn];
    int eg_eval = this->board.material_eg[this->board.turn] + this->board.pawns_eg[this->board.turn] -
                  this->board.material_eg[!this->board.turn] - this->board.pawns_eg[!this->board.turn];

    Bitboard friendly_pawns = this->board.pieces_mask(PAWN, this->board.turn);
    Bitboard enemy_pawns = this->board.pieces_mask(PAWN, !this->board.turn);

    std::array<Bitboard, 64> friendly_passed_masks = this->board.turn ? precomputed_masks.white_passed_pawns_masks : precomputed_masks.black_passed_pawns_masks;
    std::array<Bitboard, 64> enemy_passed_masks = this->board.turn ? precomputed_masks.black_passed_pawns_masks : precomputed_masks.white_passed_pawns_masks;

    std::array<Bitboard, 64> friendly_support_masks = this->board.turn ? precomputed_masks.white_pawn_support_masks : precomputed_masks.black_pawn_support_masks;
    std::array<Bitboard, 64> enemy_support_masks = this->board.turn ? precomputed_masks.black_pawn_support_masks : precomputed_masks.white_pawn_support_masks;
    
    std::array<Bitboard, 64> friendly_knight_support_masks = this->board.turn ? precomputed_masks.white_knight_support_masks : precomputed_masks.black_knight_support_masks;
    std::array<Bitboard, 64> enemy_knight_support_masks = this->board.turn ? precomputed_masks.black_knight_support_masks : precomputed_masks.white_knight_support_masks;

    int num_isolated_pawns = 0;
    StaticVector<Square, 16> squares;
    Bitboard friendly_passed_pawns = BB_EMPTY;
    Bitboard enemy_passed_pawns = BB_EMPTY;
    scan_forward(friendly_pawns, squares);
    for (Square square : squares) {
        // Passed pawns.
        if ((enemy_pawns & friendly_passed_masks[square]) == 0) {
            int rank = square_rank(square);
            int squares_ahead = this->board.turn ? 7 - rank : rank;
            eg_eval += passed_pawn_bonus[squares_ahead];
            friendly_passed_pawns |= BB_SQUARES[square];

            // Bonus for supported passed pawns.
            if ((friendly_pawns & friendly_support_masks[square]) != 0) {
                eg_eval += support_pawn_bonus[squares_ahead];
            }
        }

        // Isolated pawns.
        if ((friendly_pawns & adjacent_files_masks[square_file(square)]) == 0) {
            ++num_isolated_pawns;
        }
    }
    // Isolated pawns penalty.
    mg_eval += isolated_pawn_penalty_by_count[num_isolated_pawns];

    num_isolated_pawns = 0;
    scan_forward(enemy_pawns, squares);
    for (Square square : squares) {
        // Passed pawns.
        if ((friendly_pawns & enemy_passed_masks[square]) == 0) {
            int rank = square_rank(square);
            int squares_ahead = !this->board.turn ? rank : 7 - rank;
            eg_eval -= passed_pawn_bonus[squares_ahead];
            enemy_passed_pawns |= BB_SQUARES[square];

            // Bonus for supported passed pawns.
            if ((enemy_pawns & enemy_support_masks[square]) != 0) {
                eg_eval -= support_pawn_bonus[squares_ahead];
            }
        }
        
        // Isolated pawns.
        if ((enemy_pawns & adjacent_files_masks[square_file(square)]) == 0) {
            ++num_isolated_pawns;
        }
    }
    // Isolated pawns penalty.
    mg_eval -= isolated_pawn_penalty_by_count[num_isolated_pawns];

    // PIECE COORDINATION START
    // My_Rooks
    StaticVector<Square, 16> rook_squares;
    scan_forward(this->board.rooks & this->board.occupied_co[this->board.turn], rook_squares);
    for (Square r : rook_squares) {
        Bitboard file_bb = BB_FILE_A << square_file(r);
        Bitboard any_pawns_on_file = friendly_pawns | enemy_pawns;
        if ((any_pawns_on_file & file_bb) == 0) mg_eval += 30;
        else if ((friendly_pawns & file_bb) == 0) mg_eval += 15;

        // rook on 7th (white rank 6 index = 6? depends on square_rank semantics)
        int rank = square_rank(r);
        if ((this->board.turn == WHITE && rank == 6) || (this->board.turn == BLACK && rank == 1)) {
            mg_eval += 50;
        }

        // rook behind passed pawn: if file has our passed pawn ahead of rook
        // quick check: use our pawn bitboard as a conservative approximation
        if (friendly_passed_pawns & file_bb) {
            // check passed pawn is in front of rook (depends on orientation)
            StaticVector<Square, 16> passed_sq;
            scan_forward(friendly_passed_pawns & file_bb, passed_sq);
            for (Square ps : passed_sq) {
                if ((this->board.turn==WHITE && square_rank(ps) > square_rank(r)) ||
                    (this->board.turn==BLACK && square_rank(ps) < square_rank(r))) {
                    mg_eval += 25;
                    break;
                }
            }
        }
    }

    // connected rooks bonus
    if (rook_squares.size() >= 2) {
        for (std::size_t i = 0; i < rook_squares.size(); ++i) {
            for (std::size_t j = i + 1; j < rook_squares.size(); ++j) {
                if (square_rank(rook_squares[i]) == square_rank(rook_squares[j]) ||
                    square_file(rook_squares[i]) == square_file(rook_squares[j])) {
                    mg_eval += 20;
                }
            }
        }
    }

    // Enemy_Rooks
    rook_squares.clear();
    scan_forward(this->board.rooks & this->board.occupied_co[!this->board.turn], rook_squares);
    for (Square r : rook_squares) {
        Bitboard file_bb = BB_FILE_A << square_file(r);
        Bitboard any_pawns_on_file = friendly_pawns | enemy_pawns;
        if ((any_pawns_on_file & file_bb) == 0) mg_eval -= 30;
        else if ((enemy_pawns & file_bb) == 0) mg_eval -= 15;

        // rook on 7th (white rank 6 index = 6? depends on square_rank semantics)
        int rank = square_rank(r);
        if ((this->board.turn == BLACK && rank == 6) || (this->board.turn == WHITE && rank == 1)) {
            mg_eval -= 50;
        }

        // rook behind passed pawn: if file has our passed pawn ahead of rook
        // quick check: use our pawn bitboard as a conservative approximation
        if (enemy_passed_pawns & file_bb) {
            // check passed pawn is in front of rook (depends on orientation)
            StaticVector<Square, 16> passed_sq;
            scan_forward(enemy_passed_pawns & file_bb, passed_sq);
            for (Square ps : passed_sq) {
                if ((this->board.turn==BLACK && square_rank(ps) > square_rank(r)) ||
                    (this->board.turn==WHITE && square_rank(ps) < square_rank(r))) {
                    mg_eval -= 25;
                    break;
                }
            }
        }
    }

    // connected rooks bonus
    if (rook_squares.size() >= 2) {
        for (std::size_t i = 0; i < rook_squares.size(); ++i) {
            for (std::size_t j = i + 1; j < rook_squares.size(); ++j) {
                if (square_rank(rook_squares[i]) == square_rank(rook_squares[j]) ||
                    square_file(rook_squares[i]) == square_file(rook_squares[j])) {
                    mg_eval += 20;
                }
            }
        }
    }

    // My_Bishops
    Bitboard bishops = this->board.bishops & this->board.occupied_co[this->board.turn];
    // give bonus for bishop pair if we have two or more bishops on different color complexes
    if (__builtin_popcountll(bishops) >= 2 && 
        (bishops & BB_DARK_SQUARES) > 0 && 
        (bishops & BB_LIGHT_SQUARES) > 0) {
        mg_eval += 50; // bishop pair bonus (MG)
    }

    // Enemy_Bishops
    bishops = this->board.bishops & this->board.occupied_co[!this->board.turn];
    // give bonus for bishop pair if we have two or more bishops on different color complexes
    if (__builtin_popcountll(bishops) >= 2 && 
        (bishops & BB_DARK_SQUARES) > 0 && 
        (bishops & BB_LIGHT_SQUARES) > 0) {
        mg_eval -= 50; // bishop pair bonus (MG)
    }

    // My_Knights outpost
    Bitboard knights = this->board.knights & this->board.occupied_co[this->board.turn];
    StaticVector<Square, 16> knight_squares;
    scan_forward(knights, knight_squares);
    for (Square k : knight_squares) {
        // outpost: square not attackable by enemy pawns and supported by friendly pawn
        if ((friendly_passed_masks[k] & enemy_pawns) == 0 &&
            (friendly_knight_support_masks[k] & friendly_pawns) != 0) {
            // reward outpost, more if near center
            mg_eval += 40 + 10 * (4 - king_distance_to_center[k]);
        }
    }

    // Enemy_Knights outpost
    knights = this->board.knights & this->board.occupied_co[!this->board.turn];
    knight_squares.clear();
    scan_forward(knights, knight_squares);
    for (Square k : knight_squares) {
        // outpost: square not attackable by enemy pawns and supported by friendly pawn
        if ((enemy_passed_masks[k] & friendly_pawns) == 0 &&
            (enemy_knight_support_masks[k] & enemy_pawns) != 0) {
            // reward outpost, more if near center
            mg_eval -= 40 + 10 * (4 - king_distance_to_center[k]);
        }
    }
    
    // PIECE COORDINATION END

    // KING SAFETY START

    Square friendly_king_sq = this->board.king(this->board.turn);
    Bitboard friendly_shield_mask = precomputed_masks.white_pawn_shield_masks[friendly_king_sq]; // precomputed 3-square shield mask

    Square enemy_king_sq = this->board.king(!this->board.turn);
    Bitboard enemy_shield_mask = precomputed_masks.black_pawn_shield_masks[enemy_king_sq]; // precomputed 3-square shield mask

    // 1) Pawn shield: count how many of the shield squares contain friendly pawns
    int shield_pawns = __builtin_popcountll(friendly_pawns & friendly_shield_mask);
    int missing = 3 - std::min(3, shield_pawns); // assume shield_mask has up to 3 squares
    mg_eval -= missing * 35; // KS_SHIELD_MISSING_PENALTY
    shield_pawns = __builtin_popcountll(enemy_pawns & enemy_shield_mask);
    missing = 3 - std::min(3, shield_pawns); // assume shield mask has up to 3 squares
    mg_eval += missing * 35; // KS_SHIELD_MISSING_PENALTY

    // 2) Open/semi-open file in front of king: if there is no friendly pawn on that file,
    //    and an enemy heavy piece (rook/queen) controls file -> penalty.
    int file = square_file(friendly_king_sq);
    Bitboard file_bb = BB_FILE_A << file;
    bool friendly_pawn_on_file = (friendly_pawns & file_bb) != 0;
    bool friendly_pawn_on_left_files = (file > 0) && ((friendly_pawns & (BB_FILE_A << (file - 1))) != 0);
    bool friendly_pawn_on_right_files = (file < 7) && ((friendly_pawns & (BB_FILE_A << (file + 1))) != 0);
    if (!friendly_pawn_on_file &&
        ((this->board.rooks & this->board.occupied_co[!this->board.turn] & file_bb) != 0 ||
         (this->board.queens & this->board.occupied_co[!this->board.turn] & file_bb) != 0)) {
        mg_eval -= 40; // KS_OPEN_FILE_PENALTY// else if heavy pieces attack adjacent squares, apply half penalty
    } // end if no friendly pawn on adjacent files and enemy heavy pieces present
    else if (!friendly_pawn_on_left_files &&
             ((this->board.rooks & this->board.occupied_co[!this->board.turn] & (BB_FILE_A << std::max(0, file - 1))) != 0 ||
              (this->board.queens & this->board.occupied_co[!this->board.turn] & (BB_FILE_A << std::max(0, file - 1))) != 0)) {
        mg_eval -= 20; // half penalty
    }
    else if (!friendly_pawn_on_right_files &&
             ((this->board.rooks & this->board.occupied_co[!this->board.turn] & (BB_FILE_A << std::min(7, file + 1))) != 0 ||
              (this->board.queens & this->board.occupied_co[!this->board.turn] & (BB_FILE_A << std::min(7, file + 1))) != 0)) {
        mg_eval -= 20; // half penalty
    }

    // Repeat for enemy king
    file = square_file(enemy_king_sq);
    file_bb = BB_FILE_A << file;
    friendly_pawn_on_file = (enemy_pawns & file_bb) != 0;
    friendly_pawn_on_left_files = (file > 0) && ((enemy_pawns & (BB_FILE_A << (file - 1))) != 0);
    friendly_pawn_on_right_files = (file < 7) && ((enemy_pawns & (BB_FILE_A << (file + 1))) != 0);
    if (!friendly_pawn_on_file &&
        ((this->board.rooks & this->board.occupied_co[this->board.turn] & file_bb) != 0 ||
         (this->board.queens & this->board.occupied_co[this->board.turn] & file_bb) != 0)) {
        mg_eval += 40; // KS_OPEN_FILE_PENALTY
    } // end if no friendly pawn on adjacent files and enemy heavy pieces present
    else if (!friendly_pawn_on_left_files &&
             ((this->board.rooks & this->board.occupied_co[this->board.turn] & (BB_FILE_A << std::max(0, file - 1))) != 0 ||
              (this->board.queens & this->board.occupied_co[this->board.turn] & (BB_FILE_A << std::max(0, file - 1))) != 0)) {
        mg_eval += 20; // half penalty
    }
    else if (!friendly_pawn_on_right_files &&
             ((this->board.rooks & this->board.occupied_co[this->board.turn] & (BB_FILE_A << std::min(7, file + 1))) != 0 ||
              (this->board.queens & this->board.occupied_co[this->board.turn] & (BB_FILE_A << std::min(7, file + 1))) != 0)) {
        mg_eval += 20; // half penalty
    }

    // 3) Enemy attackers in the king zone: weight by piece type and discount by defenders
    

    // 4) Pawn storm: count enemy pawns on king's flank (files around king)
    //    Define flank_mask as files king_file-1, king_file, king_file+1 or more depending.
    Bitboard flank_mask = 0;
    int friendly_king_file = square_file(friendly_king_sq);
    if (friendly_king_file > 0) flank_mask |= BB_FILE_A << (friendly_king_file - 1);
    flank_mask |= BB_FILE_A << friendly_king_file;
    if (friendly_king_file < 7) flank_mask |= BB_FILE_A << (friendly_king_file + 1);
    // count enemy pawns in advanced ranks (depends on color)
    Bitboard advanced = enemy_pawns & flank_mask;
    StaticVector<Square, 64> adv_sq;
    scan_forward(advanced, adv_sq);
    for (Square s : adv_sq) {
        int rank = square_rank(s);
        if ((this->board.turn == WHITE && rank >= 5) || (this->board.turn == BLACK && rank <= 2)) {
            mg_eval -= 25; // KS_PAWN_STORM_PENALTY
        }
    }

    flank_mask = 0;
    int enemy_king_file = square_file(enemy_king_sq);
    if (enemy_king_file > 0) flank_mask |= BB_FILE_A << (enemy_king_file - 1);
    flank_mask |= BB_FILE_A << enemy_king_file;
    if (enemy_king_file < 7) flank_mask |= BB_FILE_A << (enemy_king_file + 1);
    // count friendly pawns in advanced ranks (depends on color)
    advanced = friendly_pawns & flank_mask;
    adv_sq.clear();
    scan_forward(advanced, adv_sq);
    for (Square s : adv_sq) {
        int rank = square_rank(s);
        if ((this->board.turn == WHITE && rank <= 2) || (this->board.turn == BLACK && rank >= 5)) {
            mg_eval += 25; // KS_PAWN_STORM_PENALTY
        }
    }

    // KING SAFETY END

    // Mobility.
    mg_eval += this->mobility_eval();

    int mg_phase = std::min(this->board.game_phase, 24);   // in case of early promotion
    int eg_phase = 24 - mg_phase;

    if (eg_phase > mg_phase)
        eg_eval += this->mopup_eval(eg_eval);

    return (mg_eval * mg_phase + eg_eval * eg_phase) / 24;
}

int Bot5_space::Bot5::mopup_eval(int eg_eval) const {
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

        // Our king should be shoved to the edge of the board.
        mopup_eval -= king_distance_to_center[other_king_square] * 47;
        // Other king should be close to our king.
        mopup_eval -= kings_distances.at(king_square, other_king_square) * 16;
    }

    return mopup_eval;
}

int Bot5_space::Bot5::mobility_eval() const {
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->board.generate_legal_moves(legal_moves);
    int turn_legal_moves_size = legal_moves.size();
    this->board.generate_legal_moves_by_color(legal_moves, !this->board.turn);
    return (turn_legal_moves_size - legal_moves.size()) * 7;
}

int Bot5_space::Bot5::evaluate_lazy(int alpha, int beta) const {
    int lazy = this->material_eval_only();

    if (lazy + LAZY_MARGIN <= alpha)
        return lazy;

    if (lazy - LAZY_MARGIN >= beta)
        return lazy;

    return this->evaluate();
}

std::optional<int> Bot5_space::Bot5::quiesce(int alpha, int beta, int q_depth) {
#ifdef DEBUG
    if (this->stop)
        return std::nullopt;
    ++this->nodes_searched;
    ++this->quiescence_nodes;
#else
    if (this->timer.time() > MAX_TIME_PER_MOVE)
        return std::nullopt;
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

    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->order_moves(legal_moves);

    for (const Move& move : legal_moves) {
        if (this->board.is_capture(move) || this->board.gives_check(move) || move.promotion != NULL_PIECE) {
            this->board.push(move);
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
    }
    
    return alpha;
}

std::optional<int> Bot5_space::Bot5::negamax(int depth, int alpha, int beta, int numExtensions, int ply, bool can_null) {
#ifdef DEBUG
    if (this->stop)
        return std::nullopt;
    ++this->nodes_searched;
#else
    if (this->timer.time() > MAX_TIME_PER_MOVE)
        return std::nullopt;
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
    if ((depth < 3) && (!is_check) && (std::abs(beta - 1) > -MATE_SCORE + 100)) {
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
            opt_score = this->negamax(depth / 2, -alpha - 1, -alpha, numExtensions, ply + 1);
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

Bot5_space::Result Bot5_space::Bot5::root_move(int depth, int alpha, int beta, const Move& ex_best_move) {
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

        if (!opt_score.has_value())
            return {best_move, alpha};
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

Move Bot5_space::Bot5::get_move() {
    this->clear_for_new_search();   // start timer, reset nodes, clear search data structures

    // Move opening_move = Move::null();
    // if (this->board.fullmove_number < 5)
    Move opening_move = this->opening_book.weighted_choice(this->board).move;
    //Move opening_move = MemoryMappedReader("books/computer.bin").weighted_choice(this->board).move;

    Move best_move = Move::null();
    int best_eval = -MATE_SCORE;
    int depth = 1;
    for (; depth <= MAX_DEPTH; ++depth) {
#ifdef DEBUG
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
#ifdef DEBUG
            if (!static_cast<bool>(result.move) && this->stop)
                break;
#else
            if (!static_cast<bool>(result.move) && this->timer.time() > MAX_TIME_PER_MOVE) {
                // timeout -> abort whole ID search
                break;
            }
#endif

            // Detect fail-low / fail-high / success
            if (result.score <= alpha) {
                // fail-low: true score <= alpha. Widen and retry.
                a_delta *= 2;
                ++retries;
                // continue loop to retry with bigger a_delta
                continue;
            } else if (result.score >= beta) {
                // fail-high: true score >= beta. Widen and retry.
                b_delta *= 2;
                ++retries;
                // continue loop to retry with bigger b_delta
                continue;
            } else {
                // Success: result inside window
                best_eval = result.score;
                best_move = result.move;
                //success = true;
                break;
            }
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

inline void Bot5_space::Bot5::clear_for_new_search() {
    this->timer.start();
    this->age_history();
    this->killer_moves.fill(Move::null());
    this->transposition_table.increment_age();
    
#ifdef DEBUG
    this->stop = false;
    this->nodes_searched = 0;
    this->quiescence_nodes = 0;
    this->transposition_table.reset_stats();
#endif
}

#ifdef DEBUG
void Bot5_space::Bot5::get_pv_line(int depth) {
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
