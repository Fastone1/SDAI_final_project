#pragma once
#include "chess.hpp"
#include "array2d.hpp"
// general parameters for the chess engine
#define INF_BOUND 22000
#define MATE_SCORE 20000
#define MAX_TIME_PER_MOVE 1.0
#define MAX_DEPTH 64
#define MAX_EXTENSION 16
#define INITIAL_ASP 50
#define MAX_ASP_RETRIES 6
#define TWO_PAWNS 200
#define LAZY_MARGIN 150
#define NUM_THREADS 16
#define IS_MATE (MATE_SCORE - MAX_DEPTH)
#define is_mate_score(score) (score > IS_MATE || score < -IS_MATE)

// functions and precomputed tables for evaluation
constexpr inline int ply_to_mate_from_score(int score) {
    return MATE_SCORE - std::abs(score);
}

constexpr inline int moves_to_mate_from_score(int score) {
    return (MATE_SCORE - std::abs(score) + 1) / 2;
}

constexpr inline Array2D<int, 64, 64> precompute_kings_distances() {
    Array2D<int, 64, 64> distances = {};
    for (Square sq1 : SQUARES) {
        for (Square sq2 : SQUARES) {
            distances.insert(sq1, sq2, 14 - square_manhattan_distance(sq1, sq2));
        }
    }
    return distances;
}
const Array2D<int, 64, 64> kings_distances = precompute_kings_distances();

// evaluation parameters
const int passed_pawn_bonus[] = { 0, 50, 40, 30, 20, 10, 10 };
const int support_pawn_bonus[] = { 0, 40, 30, 20, 10, 10, 0 };
const int isolated_pawn_penalty_by_count[] = { 0, -10, -25, -50, -75, -75, -75, -75, -75 };
const int futility_margin[] = { 0, 200, 300, 500 };
const int king_distance_to_center[] = {
    6, 5, 4, 3, 3, 4, 5, 6,
    5, 4, 3, 2, 2, 3, 4, 5,
    4, 3, 2, 1, 1, 2, 3, 4,
    3, 2, 1, 0, 0, 1, 2, 3,
    3, 2, 1, 0, 0, 1, 2, 3,
    4, 3, 2, 1, 1, 2, 3, 4,
    5, 4, 3, 2, 2, 3, 4, 5,
    6, 5, 4, 3, 3, 4, 5, 6 
};
