#ifndef CHESS_HPP
#define CHESS_HPP

#include "array2d.hpp"
#include "static_vector.hpp"
#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <cstdint>
#include <array>
#include <regex>
#include <variant>
#include <cstdarg>
#include <unordered_map>

#ifdef __BMI2__
#include <immintrin.h>
#define HAS_BMI2 1
#else
#define HAS_BMI2 0
#endif

void split(std::vector<std::string>& result, const std::string& s, char delimiter = ' ');

inline const int MOVE_HISTORY_SIZE = 128;
inline const int LEGAL_MOVES_SIZE = 216;
inline const int CAPTURES_SIZE = 64;
inline const int CASTLING_MOVES_SIZE = 2;
inline const int EP_CAPTURE_SIZE = 2;
inline const int EVASION_SIZE = 24;

enum class EnPassantSpec { legal, fen, xfen };

typedef bool Color;
inline constexpr Color WHITE = true;
inline constexpr Color BLACK = false;
inline constexpr Color COLORS[] = {WHITE, BLACK};
typedef std::string ColorName;
inline const ColorName COLOR_NAMES[] = {"black", "white"};

typedef int PieceType;
inline constexpr PieceType PAWN = 0;
inline constexpr PieceType KNIGHT = 1;
inline constexpr PieceType BISHOP = 2;
inline constexpr PieceType ROOK = 3;
inline constexpr PieceType QUEEN = 4;
inline constexpr PieceType KING = 5;
inline constexpr PieceType NULL_PIECE = 6;

inline constexpr PieceType PIECE_TYPES[] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
inline const std::string PIECE_SYMBOLS = "pnbrqk";
inline const std::string PIECE_NAMES[] = {"pawn", "knight", "bishop", "rook", "queen", "king"};

std::string piece_symbol(const PieceType &piece);

std::string piece_name(const PieceType &piece);

const std::unordered_map<std::string, std::string> UNICODE_PIECE_SYMBOLS = {
    {"R", "♖"}, {"r", "♜"},
    {"N", "♘"}, {"n", "♞"},
    {"B", "♗"}, {"b", "♝"},
    {"Q", "♕"}, {"q", "♛"},
    {"K", "♔"}, {"k", "♚"},
    {"P", "♙"}, {"p", "♟"}
};

inline const std::string FILE_NAMES[] = {"a", "b", "c", "d", "e", "f", "g", "h"};
inline const std::string RANK_NAMES[] = {"1", "2", "3", "4", "5", "6", "7", "8"};

inline const std::string STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
inline const std::string STARTING_BOARD_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

enum class Status {
    VALID = 0,
    NO_WHITE_KING = 1 << 0,
    NO_BLACK_KING = 1 << 1,
    TOO_MANY_KINGS = 1 << 2,
    TOO_MANY_WHITE_PAWNS = 1 << 3,
    TOO_MANY_BLACK_PAWNS = 1 << 4,
    PAWNS_ON_BACKRANK = 1 << 5,
    TOO_MANY_WHITE_PIECES = 1 << 6,
    TOO_MANY_BLACK_PIECES = 1 << 7,
    BAD_CASTLING_RIGHTS = 1 << 8,
    INVALID_EP_SQUARE = 1 << 9,
    OPPOSITE_CHECK = 1 << 10,
    EMPTY = 1 << 11,
    RACE_CHECK = 1 << 12,
    RACE_OVER = 1 << 13,
    RACE_MATERIAL = 1 << 14,
    TOO_MANY_CHECKERS = 1 << 15,
    IMPOSSIBLE_CHECK = 1 << 16
};

inline const Status STATUS_VALID = Status::VALID;
inline const Status STATUS_NO_WHITE_KING = Status::NO_WHITE_KING;
inline const Status STATUS_NO_BLACK_KING = Status::NO_BLACK_KING;
inline const Status STATUS_TOO_MANY_KINGS = Status::TOO_MANY_KINGS;
inline const Status STATUS_TOO_MANY_WHITE_PAWNS = Status::TOO_MANY_WHITE_PAWNS;
inline const Status STATUS_TOO_MANY_BLACK_PAWNS = Status::TOO_MANY_BLACK_PAWNS;
inline const Status STATUS_PAWNS_ON_BACKRANK = Status::PAWNS_ON_BACKRANK;
inline const Status STATUS_TOO_MANY_WHITE_PIECES = Status::TOO_MANY_WHITE_PIECES;
inline const Status STATUS_TOO_MANY_BLACK_PIECES = Status::TOO_MANY_BLACK_PIECES;
inline const Status STATUS_BAD_CASTLING_RIGHTS = Status::BAD_CASTLING_RIGHTS;
inline const Status STATUS_INVALID_EP_SQUARE = Status::INVALID_EP_SQUARE;
inline const Status STATUS_OPPOSITE_CHECK = Status::OPPOSITE_CHECK;
inline const Status STATUS_EMPTY = Status::EMPTY;
inline const Status STATUS_RACE_CHECK = Status::RACE_CHECK;
inline const Status STATUS_RACE_OVER = Status::RACE_OVER;
inline const Status STATUS_RACE_MATERIAL = Status::RACE_MATERIAL;
inline const Status STATUS_TOO_MANY_CHECKERS = Status::TOO_MANY_CHECKERS;
inline const Status STATUS_IMPOSSIBLE_CHECK = Status::IMPOSSIBLE_CHECK;

enum class Termination {
    CHECKMATE = 0,
    STALEMATE = 1,
    INSUFFICIENT_MATERIAL = 2,
    SEVENTYFIVE_MOVES = 3,
    FIVEFOLD_REPETITION = 4,
    FIFTY_MOVES = 5,
    THREEFOLD_REPETITION = 6,
    VARIANT_WIN = 7,
    VARIANT_DRAW = 8,
    VARIANT_LOSS = 9,
    REPETITION = 10,
};

class Outcome {
public:
Termination termination;
std::optional<Color> winner;

    Outcome(Termination term, std::optional<Color> win) : termination(term), winner(win) {}

    std::string result() const;

    bool operator==(const Outcome& other) const;

    bool operator!=(const Outcome& other) const;

    std::ostream& operator<<(std::ostream& os) const;

    operator std::string() const;

    std::string to_string() const;
};

class InvalidMoveError : public std::exception {
private:
    std::string message;

public:
    InvalidMoveError(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

class IllegalMoveError : public std::exception {
private:
    std::string message;

public:
    IllegalMoveError(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

class AmbiguousMoveError : public std::exception {
private:
    std::string message;

public:
    AmbiguousMoveError(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};

typedef int Square;
inline constexpr Square A1 = 0;
inline constexpr Square B1 = 1;
inline constexpr Square C1 = 2;
inline constexpr Square D1 = 3;
inline constexpr Square E1 = 4;
inline constexpr Square F1 = 5;
inline constexpr Square G1 = 6;
inline constexpr Square H1 = 7;
inline constexpr Square A2 = 8;
inline constexpr Square B2 = 9;
inline constexpr Square C2 = 10;
inline constexpr Square D2 = 11;
inline constexpr Square E2 = 12;
inline constexpr Square F2 = 13;
inline constexpr Square G2 = 14;
inline constexpr Square H2 = 15;
inline constexpr Square A3 = 16;
inline constexpr Square B3 = 17;
inline constexpr Square C3 = 18;
inline constexpr Square D3 = 19;
inline constexpr Square E3 = 20;
inline constexpr Square F3 = 21;
inline constexpr Square G3 = 22;
inline constexpr Square H3 = 23;
inline constexpr Square A4 = 24;
inline constexpr Square B4 = 25;
inline constexpr Square C4 = 26;
inline constexpr Square D4 = 27;
inline constexpr Square E4 = 28;
inline constexpr Square F4 = 29;
inline constexpr Square G4 = 30;
inline constexpr Square H4 = 31;
inline constexpr Square A5 = 32;
inline constexpr Square B5 = 33;
inline constexpr Square C5 = 34;
inline constexpr Square D5 = 35;
inline constexpr Square E5 = 36;
inline constexpr Square F5 = 37;
inline constexpr Square G5 = 38;
inline constexpr Square H5 = 39;
inline constexpr Square A6 = 40;
inline constexpr Square B6 = 41;
inline constexpr Square C6 = 42;
inline constexpr Square D6 = 43;
inline constexpr Square E6 = 44;
inline constexpr Square F6 = 45;
inline constexpr Square G6 = 46;
inline constexpr Square H6 = 47;
inline constexpr Square A7 = 48;
inline constexpr Square B7 = 49;
inline constexpr Square C7 = 50;
inline constexpr Square D7 = 51;
inline constexpr Square E7 = 52;
inline constexpr Square F7 = 53;
inline constexpr Square G7 = 54;
inline constexpr Square H7 = 55;
inline constexpr Square A8 = 56;
inline constexpr Square B8 = 57;
inline constexpr Square C8 = 58;
inline constexpr Square D8 = 59;
inline constexpr Square E8 = 60;
inline constexpr Square F8 = 61;
inline constexpr Square G8 = 62;
inline constexpr Square H8 = 63;
inline constexpr Square NULL_SQUARE = 64;
inline const Square SQUARES[] = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};
inline const std::string SQUARE_NAMES[] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

Square parse_square(const std::string& name);

std::string square_name(Square square);

Square square(int file_index, int rank_index);

int square_file(Square square);

int square_rank(Square square);

Square square_mirror(Square square);

inline const Square SQUARES_180[] = {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1
};

typedef uint64_t Bitboard;
inline const Bitboard BB_EMPTY = 0;
inline const Bitboard BB_ALL = 0xffffffffffffffff;

inline const Bitboard BB_A1 = 1ULL << A1;
inline const Bitboard BB_B1 = 1ULL << B1;
inline const Bitboard BB_C1 = 1ULL << C1;
inline const Bitboard BB_D1 = 1ULL << D1;
inline const Bitboard BB_E1 = 1ULL << E1;
inline const Bitboard BB_F1 = 1ULL << F1;
inline const Bitboard BB_G1 = 1ULL << G1;
inline const Bitboard BB_H1 = 1ULL << H1;
inline const Bitboard BB_A2 = 1ULL << A2;
inline const Bitboard BB_B2 = 1ULL << B2;
inline const Bitboard BB_C2 = 1ULL << C2;
inline const Bitboard BB_D2 = 1ULL << D2;
inline const Bitboard BB_E2 = 1ULL << E2;
inline const Bitboard BB_F2 = 1ULL << F2;
inline const Bitboard BB_G2 = 1ULL << G2;
inline const Bitboard BB_H2 = 1ULL << H2;
inline const Bitboard BB_A3 = 1ULL << A3;
inline const Bitboard BB_B3 = 1ULL << B3;
inline const Bitboard BB_C3 = 1ULL << C3;
inline const Bitboard BB_D3 = 1ULL << D3;
inline const Bitboard BB_E3 = 1ULL << E3;
inline const Bitboard BB_F3 = 1ULL << F3;
inline const Bitboard BB_G3 = 1ULL << G3;
inline const Bitboard BB_H3 = 1ULL << H3;
inline const Bitboard BB_A4 = 1ULL << A4;
inline const Bitboard BB_B4 = 1ULL << B4;
inline const Bitboard BB_C4 = 1ULL << C4;
inline const Bitboard BB_D4 = 1ULL << D4;
inline const Bitboard BB_E4 = 1ULL << E4;
inline const Bitboard BB_F4 = 1ULL << F4;
inline const Bitboard BB_G4 = 1ULL << G4;
inline const Bitboard BB_H4 = 1ULL << H4;
inline const Bitboard BB_A5 = 1ULL << A5;
inline const Bitboard BB_B5 = 1ULL << B5;
inline const Bitboard BB_C5 = 1ULL << C5;
inline const Bitboard BB_D5 = 1ULL << D5;
inline const Bitboard BB_E5 = 1ULL << E5;
inline const Bitboard BB_F5 = 1ULL << F5;
inline const Bitboard BB_G5 = 1ULL << G5;
inline const Bitboard BB_H5 = 1ULL << H5;
inline const Bitboard BB_A6 = 1ULL << A6;
inline const Bitboard BB_B6 = 1ULL << B6;
inline const Bitboard BB_C6 = 1ULL << C6;
inline const Bitboard BB_D6 = 1ULL << D6;
inline const Bitboard BB_E6 = 1ULL << E6;
inline const Bitboard BB_F6 = 1ULL << F6;
inline const Bitboard BB_G6 = 1ULL << G6;
inline const Bitboard BB_H6 = 1ULL << H6;
inline const Bitboard BB_A7 = 1ULL << A7;
inline const Bitboard BB_B7 = 1ULL << B7;
inline const Bitboard BB_C7 = 1ULL << C7;
inline const Bitboard BB_D7 = 1ULL << D7;
inline const Bitboard BB_E7 = 1ULL << E7;
inline const Bitboard BB_F7 = 1ULL << F7;
inline const Bitboard BB_G7 = 1ULL << G7;
inline const Bitboard BB_H7 = 1ULL << H7;
inline const Bitboard BB_A8 = 1ULL << A8;
inline const Bitboard BB_B8 = 1ULL << B8;
inline const Bitboard BB_C8 = 1ULL << C8;
inline const Bitboard BB_D8 = 1ULL << D8;
inline const Bitboard BB_E8 = 1ULL << E8;
inline const Bitboard BB_F8 = 1ULL << F8;
inline const Bitboard BB_G8 = 1ULL << G8;
inline const Bitboard BB_H8 = 1ULL << H8;
inline const Bitboard BB_SQUARES[] = {
    BB_A1, BB_B1, BB_C1, BB_D1, BB_E1, BB_F1, BB_G1, BB_H1,
    BB_A2, BB_B2, BB_C2, BB_D2, BB_E2, BB_F2, BB_G2, BB_H2,
    BB_A3, BB_B3, BB_C3, BB_D3, BB_E3, BB_F3, BB_G3, BB_H3,
    BB_A4, BB_B4, BB_C4, BB_D4, BB_E4, BB_F4, BB_G4, BB_H4,
    BB_A5, BB_B5, BB_C5, BB_D5, BB_E5, BB_F5, BB_G5, BB_H5,
    BB_A6, BB_B6, BB_C6, BB_D6, BB_E6, BB_F6, BB_G6, BB_H6,
    BB_A7, BB_B7, BB_C7, BB_D7, BB_E7, BB_F7, BB_G7, BB_H7,
    BB_A8, BB_B8, BB_C8, BB_D8, BB_E8, BB_F8, BB_G8, BB_H8
};

inline const Bitboard BB_CORNERS = BB_A1 | BB_H1 | BB_A8 | BB_H8;
inline const Bitboard BB_CENTER = BB_E4 | BB_D4 | BB_E5 | BB_D5;

inline const Bitboard BB_LIGHT_SQUARES = 0xaa55aa55aa55aa55;
inline const Bitboard BB_DARK_SQUARES = 0x55aa55aa55aa55aa;

inline const Bitboard BB_FILE_A = 0x0101010101010101 << 0;
inline const Bitboard BB_FILE_B = 0x0101010101010101 << 1;
inline const Bitboard BB_FILE_C = 0x0101010101010101 << 2;
inline const Bitboard BB_FILE_D = 0x0101010101010101 << 3;
inline const Bitboard BB_FILE_E = 0x0101010101010101 << 4;
inline const Bitboard BB_FILE_F = 0x0101010101010101 << 5;
inline const Bitboard BB_FILE_G = 0x0101010101010101 << 6;
inline const Bitboard BB_FILE_H = 0x0101010101010101 << 7;
inline const Bitboard BB_FILES[] = {BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F, BB_FILE_G, BB_FILE_H};

inline const Bitboard BB_RANK_1 = static_cast<Bitboard>(0xff) << (8 * 0);
inline const Bitboard BB_RANK_2 = static_cast<Bitboard>(0xff) << (8 * 1);
inline const Bitboard BB_RANK_3 = static_cast<Bitboard>(0xff) << (8 * 2);
inline const Bitboard BB_RANK_4 = static_cast<Bitboard>(0xff) << (8 * 3);
inline const Bitboard BB_RANK_5 = static_cast<Bitboard>(0xff) << (8 * 4);
inline const Bitboard BB_RANK_6 = static_cast<Bitboard>(0xff) << (8 * 5);
inline const Bitboard BB_RANK_7 = static_cast<Bitboard>(0xff) << (8 * 6);
inline const Bitboard BB_RANK_8 = static_cast<Bitboard>(0xff) << (8 * 7);
inline const Bitboard BB_RANKS[] = {BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8};

inline const Bitboard BB_BACKRANKS = BB_RANK_1 | BB_RANK_8;

int lsb(const Bitboard& bb);

std::vector<Square> scan_forward(Bitboard bb);
template<std::size_t T>
void scan_forward(Bitboard bb, StaticVector<Square, T>& squares);

int msb(const Bitboard& bb);

std::vector<Square> scan_reversed(Bitboard bb);
template<std::size_t T>
void scan_reversed(Bitboard bb, StaticVector<Square, T>& squares);

int square_distance(Square a, Square b);

int square_manhattan_distance(Square a, Square b);

int square_knight_distance(Square a, Square b);

Bitboard flip_vertical(Bitboard bb);

Bitboard flip_horizontal(Bitboard bb);

Bitboard flip_diagonal(Bitboard bb);

Bitboard flip_anti_diagonal(Bitboard bb);

Bitboard shift_down(Bitboard bb);

Bitboard shift_2_down(Bitboard bb);

Bitboard shift_up(Bitboard bb);

Bitboard shift_2_up(Bitboard bb);

Bitboard shift_right(Bitboard bb);

Bitboard shift_2_right(Bitboard bb);

Bitboard shift_left(Bitboard bb);

Bitboard shift_2_left(Bitboard bb);

Bitboard shift_up_right(Bitboard bb);

Bitboard shift_up_left(Bitboard bb);

Bitboard shift_down_right(Bitboard bb);

Bitboard shift_down_left(Bitboard bb);

Bitboard _sliding_attacks(Square square, const Bitboard& occupied, const std::vector<int>& deltas);

Bitboard _step_attacks(Square square, const std::vector<int>& deltas);

std::array<Bitboard, 64> _bb_knight_attacks_init();

const extern std::array<Bitboard, 64> BB_KNIGHT_ATTACKS;

std::array<Bitboard, 64> _bb_king_attacks_init();

const extern std::array<Bitboard, 64> BB_KING_ATTACKS;

Array2D<Bitboard, 2, 64> _bb_pawn_attacks_init();

const extern Array2D<Bitboard, 2, 64> BB_PAWN_ATTACKS;

Bitboard _edges(Square square);

void _carry_rippler(const Bitboard& mask, std::vector<Bitboard>& ripples);

struct AttackTable {
    // Cache-aware layout: each square has a dense 4K entry attack table.
    alignas(64) std::array<std::array<Bitboard, 4096>, 64> attacks;
    std::array<Bitboard, 64> masks;
#if !HAS_BMI2
    std::array<uint8_t, 64> relevant_bits;
    std::array<std::array<uint8_t, 12>, 64> bit_positions;
#endif

    inline uint16_t index(Square square, Bitboard occupied) const {
        Bitboard mask = masks[square];
        Bitboard masked = occupied & mask;
#if HAS_BMI2
        return static_cast<uint16_t>(_pext_u64(masked, mask));
#else
        uint16_t idx = 0;
        const uint8_t bits = relevant_bits[square];
        for (uint8_t i = 0; i < bits; ++i) {
            idx |= static_cast<uint16_t>(((masked >> bit_positions[square][i]) & 1ULL) << i);
        }
        return idx;
#endif
    }

    inline Bitboard get(Square square, Bitboard occupied) const {
        return attacks[square][index(square, occupied)];
    }
};

const AttackTable _attack_table(const std::vector<int>& deltas);

const extern AttackTable BB_DIAG_TABLE;
const extern AttackTable BB_FILE_TABLE;
const extern AttackTable BB_RANK_TABLE;

const Array2D<Bitboard, 64, 64> _rays();

const extern Array2D<Bitboard, 64, 64> BB_RAYS;

Bitboard ray(Square a, Square b);

Bitboard between(Square a, Square b);

inline const std::regex SAN_REGEX = std::regex(R"~(^([NBRQK])?([a-h])?([1-8])?[\-x]?([a-h][1-8])(=?[nbrqkNBRQK])?[\+#]?)~");
inline const std::regex FEN_CASTLING_REGEX = std::regex(R"~(^(?:-|[KQABCDEFGH]{0,2}[kqabcdefgh]{0,2}))~");

class Piece {
public:
    PieceType piece_type;
    Color color;

    Piece() = default;

    Piece(PieceType t, Color c) : piece_type(t), color(c) {}

    Piece(const Piece& other) : piece_type(other.piece_type), color(other.color) {}

    std::string symbol() const;

    std::string unicode_symbol(const bool& invert_colors = false) const;

    bool operator==(const Piece& other) const;

    bool operator!=(const Piece& other) const;

    std::ostream& operator<<(std::ostream& os) const;

    operator std::string() const;

    operator bool() const;

    std::string to_string() const;

    static Piece from_symbol(const std::string& symbol);

    static Piece null();
};

namespace std {
    template <>
    struct hash<Piece> {
        std::size_t operator()(const Piece& piece) const {
            return piece.piece_type + (piece.color ? -1 : 5);
        }
    };
}

class Move {
public:
    Square from_square : 8 = NULL_SQUARE;
    Square to_square : 8 = NULL_SQUARE;
    PieceType promotion : 4 = NULL_PIECE;

    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const;

    operator std::string() const;
    operator bool() const;

    constexpr Move() = default; // Default constructor
    constexpr Move(Square from, Square to, PieceType promo = NULL_PIECE) : from_square(from), to_square(to), promotion(promo) {}

    Move(const Move&) = default; // Copy constructor
    Move(Move&&) = default; // Move constructor
    Move& operator=(const Move&) = default; // Copy assignment
    Move& operator=(Move&&) = default; // Move assignment
    ~Move() = default; // Destructor
    
    std::string uci() const;

    std::string xboard() const;

    static Move null();

    static Move from_uci(const std::string& uci);
};

class SquareSet;

typedef std::variant<Bitboard, std::vector<Square>, Square, SquareSet> IntoSquareSet;

class SquareSet {
public:
    Bitboard mask;

    SquareSet(const IntoSquareSet& squares = BB_EMPTY);

    bool contains__(Square square);

    std::vector<Square>::iterator begin();

    std::vector<Square>::iterator end();

    std::vector<Square>::reverse_iterator rbegin();

    std::vector<Square>::reverse_iterator rend();

    int length();

    void add(Square square);

    void discard(Square square);

    bool isdisjoint(const IntoSquareSet& other);

    bool issubset(const IntoSquareSet& other);

    bool issuperset(const IntoSquareSet& other);

    SquareSet union__(const IntoSquareSet& other);

    SquareSet operator|(const IntoSquareSet& other);

    SquareSet operator|= (const IntoSquareSet& other);

    SquareSet intersection(const IntoSquareSet& other);

    SquareSet operator&(const IntoSquareSet& other);

    SquareSet operator&= (const IntoSquareSet& other);

    SquareSet difference(const IntoSquareSet& other);

    SquareSet operator-(const IntoSquareSet& other);

    SquareSet operator-= (const IntoSquareSet& other);

    SquareSet symmetric_difference(const IntoSquareSet& other);

    SquareSet operator^(const IntoSquareSet& other);

    SquareSet operator^= (const IntoSquareSet& other);

    SquareSet copy();

    void update(IntoSquareSet* others,...);

    void intersection_update(IntoSquareSet* others,...);

    void difference_update(const IntoSquareSet& other);

    void symmetric_difference_update(const IntoSquareSet& other);

    void remove(Square square);

    Square pop();

    void clear();

    std::vector<Bitboard> carry_rippler();

    SquareSet mirror();

    std::array<bool, 64> to_list();

    explicit operator bool() const;

    bool operator==(const IntoSquareSet& other) const;
    bool operator!=(const IntoSquareSet& other) const;

    SquareSet operator<<(int shift);

    SquareSet operator>>(int shift);

    SquareSet operator<<=(int shift);

    SquareSet operator>>=(int shift);

    SquareSet operator~();

    explicit operator int() const;

    explicit operator Bitboard() const;

    explicit operator std::string() const;

    static SquareSet ray(Square a, Square b);

    static SquareSet between(Square a, Square b);

    static SquareSet from_square(Square square);
};

const int mg_values[6] = { 82, 337, 365, 477, 1025, 20000 };
const int eg_values[6] = { 94, 281, 297, 512, 936, 20000 };

const std::array<int, 64> mg_pst[6] = {
    {0,   0,   0,   0,   0,   0,  0,   0,
    98, 134,  61,  95,  68, 126, 34, -11,
    -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
    0,   0,   0,   0,   0,   0,  0,   0},   // PAWN
    {-167, -89, -34, -49,  61, -97, -15, -107,
    -73, -41,  72,  36,  23,  62,   7,  -17,
    -47,  60,  37,  65,  84, 129,  73,   44,
    -9,  17,  19,  53,  37,  69,  18,   22,
    -13,   4,  16,  13,  28,  19,  21,   -8,
    -23,  -9,  12,  10,  19,  17,  25,  -16,
    -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23},  // KNIGHT
    {-29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
    -4,   5,  19,  50,  37,  37,   7,  -2,
    -6,  13,  13,  26,  34,  12,  10,   4,
    0,  15,  15,  15,  14,  27,  18,  10,
    4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21},  // BISHOP
    {32,  42,  32,  51, 63,  9,  31,  43,
    27,  32,  58,  62, 80, 67,  26,  44,
    -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -13, -19,   1,  11, 10,  7, -20, -13},  // ROOK
    {-28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
    -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
    -1, -18,  -9,  10, -15, -25, -31, -50},  // QUEEN
    {-65,  23,  16, -15, -56, -34,   2,  13,
    29,  -1, -20,  -7,  -8,  -4, -38, -29,
    -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
    1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14}  // KING
};

const std::array<int, 64> eg_pst[6] = {
    {0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
    94, 100,  85,  67,  56,  53,  82,  84,
    32,  24,  13,   5,  -2,   4,  17,  17,
    13,   9,  -3,  -7,  -7,  -8,   3,  -1,
    4,   7,  -6,   1,   0,  -5,  -1,  -8,
    13,   8,   8,  10,  13,   0,   2,  -7,
    0,   0,   0,   0,   0,   0,   0,   0},   // PAWN
    {-58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64},  // KNIGHT
    {-14, -21, -11,  -8, -7,  -9, -17, -24,
    -8,  -4,   7, -12, -3, -13,  -4, -14,
    2,  -8,   0,  -1, -2,   6,   0,   4,
    -3,   9,  12,   9, 14,  10,   3,   2,
    -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17},  // BISHOP
    {13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
    7,  7,  7,  5,  4,  -3,  -5,  -3,
    4,  3, 13,  1,  2,   1,  -1,   2,
    3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20},  // ROOK
    {-9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
    3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41},  // QUEEN
    {-74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
    10,  17,  23,  15,  20,  45,  44,  13,
    -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43}  // KING
};

const int gamephaseInc[6] = { 0, 1, 1, 2, 4, 0 };

/*
A board representing the position of chess pieces. See
:class:`~chess.Board` for a full board with move generation.

The board is initialized with the standard chess starting position, unless
otherwise specified in the optional *board_fen* argument. If *board_fen*
is ``None``, an empty board is created.
*/
class Baseboard {
public:
    Bitboard occupied_co[2];
    Bitboard occupied;
    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;
    int material_mg[2];
    int material_eg[2];
    int pawns_mg[2];
    int pawns_eg[2];
    int game_phase;

    Baseboard(const std::optional<std::string>& board_fen = STARTING_BOARD_FEN);

    /*
    Resets pieces to the starting position.

    :class:`~chess.Board` also resets the move stack, but not turn,
    castling rights and move counters. Use :func:`chess.Board.reset()` to
    fully restore the starting position.
    */
    void reset_board();

    /*
    Clears the board.

    :class:`~chess.Board` also clears the move stack.
    */
    void clear_board();

    Bitboard pieces_mask(PieceType piece_type, Color color) const;

    /*
    Gets pieces of the given type and color.

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    SquareSet pieces(PieceType piece_type, Color color) const;

    // Gets the :class:`piece <chess.Piece>` at the given square.
    std::optional<Piece> piece_at(Square square) const;

    // Gets the piece type at the given square.
    PieceType piece_type_at(Square square) const;

    // Gets the color of the piece at the given square.
    std::optional<Color> color_at(Square square) const;

    /*
    Finds the king square of the given side. Returns ``None`` if there
    is no king of that color.
    */
    Square king(Color color) const;

    Bitboard attacks_mask(Square square) const;

    /*
    Gets the set of attacked squares from the given square.

    There will be no attacks if the square is empty. Pinned pieces are
    still attacking other squares.

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    SquareSet attacks(Square square) const;

    Bitboard attackers_mask(Color color, Square square, const std::optional<Bitboard>& occupied_ = std::nullopt) const;

    /*
    Checks if the given side attacks the given square.

    Pinned pieces still count as attackers. Pawns that can be captured
    en passant are **not** considered attacked.

    *occupied* determines which squares are considered to block attacks.
    For example,
    ``board.occupied ^ board.pieces_mask(chess.KING, board.turn)`` can be
    used to consider X-ray attacks through the king.
    Defaults to ``board.occupied`` (all pieces including the king,
    no X-ray attacks).
    */
    bool is_attacked_by(Color color, Square square, const std::optional<IntoSquareSet>& occupied_ = std::nullopt) const;
    
    /*
    Gets the set of attackers of the given color for the given square.

    Pinned pieces still count as attackers.

    *occupied* determines which squares are considered to block attacks.
    For example,
    ``board.occupied ^ board.pieces_mask(chess.KING, board.turn)`` can be
    used to consider X-ray attacks through the king.
    Defaults to ``board.occupied`` (all pieces including the king,
    no X-ray attacks).

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    SquareSet attackers(Color color, Square square, const std::optional<IntoSquareSet>& occupied_ = std::nullopt) const;

    Bitboard pin_mask(Color color, Square square) const;

    /*
    Detects an absolute pin (and its direction) of the given square to
    the king of the given color.

    Returns a :class:`set of squares <chess.SquareSet>` that mask the rank,
    file or diagonal of the pin. If there is no pin, then a mask of the
    entire board is returned.
    */
    SquareSet pin(Color color, Square square) const;

    // Detects if the given square is pinned to the king of the given color.
    bool is_pinned(Color color, Square square) const;

    /*
    Removes the piece from the given square. Returns the
    :class:`~chess.Piece` or ``None`` if the square was already empty.

    :class:`~chess.Board` also clears the move stack.
    */
    std::optional<Piece> remove_piece_at(Square square);

    /*
    Sets a piece at the given square.

    An existing piece is replaced. Setting *piece* to ``None`` is
    equivalent to :func:`~chess.Board.remove_piece_at()`.

    :class:`~chess.Board` also clears the move stack.
    */
    void set_piece_at(Square square, const std::optional<Piece>& piece);

    // Gets the board FEN (e.g., ``rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR``).
    std::string board_fen() const;

    /*
    Parses *fen* and sets up the board, where *fen* is the board part of
    a FEN.

    :class:`~chess.Board` also clears the move stack.

    :raises: :exc:`ValueError` if syntactically invalid.
    */
    void set_board_fen(const std::string& fen);

    // Gets a dictionary of :class:`pieces <chess.Piece>` by square index.
    std::unordered_map<Square, Piece> piece_map(const Bitboard& mask = BB_ALL) const;

    /*
    Sets up the board from a dictionary of :class:`pieces <chess.Piece>`
    by square index.

    :class:`~chess.Board` also clears the move stack.
    */
    void set_piece_map(const std::unordered_map<Square, Piece>& pieces);

    explicit operator std::string();

    /*
    Returns a string representation of the board with Unicode pieces.
    Useful for pretty-printing to a terminal.

    :param invert_color: Invert color of the Unicode pieces.
    :param borders: Show borders and a coordinate margin.
    */
    std::string unicode(const bool& invert_color = false, const bool& borders = false, const std::string& empty_square = "⭘", Color orientation = WHITE) const;

    bool operator==(const Baseboard& other) const;

    void apply_transform(const std::function<Bitboard(Bitboard)>& f);

    /*
    Returns a transformed copy of the board (without move stack)
    by applying a bitboard transformation function.

    Available transformations include :func:`chess.flip_vertical()`,
    :func:`chess.flip_horizontal()`, :func:`chess.flip_diagonal()`,
    :func:`chess.flip_anti_diagonal()`, :func:`chess.shift_down()`,
    :func:`chess.shift_up()`, :func:`chess.shift_left()`, and
    :func:`chess.shift_right()`.

    Alternatively, :func:`~chess.BaseBoard.apply_transform()` can be used
    to apply the transformation on the board.
    */
    Baseboard transform(const std::function<Bitboard(Bitboard)>& f);

    void apply_mirror();

    /*
    Returns a mirrored copy of the board (without move stack).

    The board is mirrored vertically and piece colors are swapped, so that
    the position is equivalent modulo color.

    Alternatively, :func:`~chess.BaseBoard.apply_mirror()` can be used
    to mirror the board.
    */
    Baseboard mirror() const;

    // Creates a copy of the board.
    Baseboard copy() const;

    /*
    Creates a new empty board. Also see
    :func:`~chess.BaseBoard.clear_board()`.
    */
    static Baseboard empty();

    void _reset_board();

    void _clear_board();

    PieceType _remove_piece_at(Square square);

    PieceType _remove_piece_at_known(Square square, PieceType piece_type, Color color);

    void _set_piece_at(Square square, PieceType piece_type, Color color);

    void _set_piece_at_empty(Square square, PieceType piece_type, Color color);

    void _set_board_fen(std::string fen);

    void _set_piece_map(const std::unordered_map<Square, Piece>& pieces);
};

class _BoardState;

class BoardStateTuple {
public:
    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;
    Bitboard occupied_white;
    Bitboard occupied_black;
    Bitboard castling_rights;
    Square ep_square;
    Color turn;

    BoardStateTuple() = default;

    BoardStateTuple(Bitboard p, Bitboard n, Bitboard b, Bitboard r, Bitboard q, Bitboard k,
                    Bitboard ow, Bitboard ob, Color t, Bitboard cr, Square ep) :
        pawns(p), knights(n), bishops(b), rooks(r), queens(q), kings(k),
        occupied_white(ow), occupied_black(ob), turn(t), castling_rights(cr), ep_square(ep) {};

    bool operator==(const BoardStateTuple& other) const {
        return pawns == other.pawns &&
               knights == other.knights &&
               bishops == other.bishops &&
               rooks == other.rooks &&
               queens == other.queens &&
               kings == other.kings &&
               occupied_white == other.occupied_white &&
               occupied_black == other.occupied_black &&
               turn == other.turn &&
               castling_rights == other.castling_rights &&
               ep_square == other.ep_square;
    }

    bool operator!=(const BoardStateTuple& other) const {
        return !(*this == other);
    }
};

class Board : public Baseboard {
/*
A :class:`~chess.BaseBoard`, additional information representing
a chess position, and a :data:`move stack <chess.Board.move_stack>`.

Provides :data:`move generation <chess.Board.legal_moves>`, validation,
:func:`parsing <chess.Board.parse_san()>`, attack generation,
:func:`game end detection <chess.Board.is_game_over()>`,
and the capability to :func:`make <chess.Board.push()>` and
:func:`unmake <chess.Board.pop()>` moves.

The board is initialized to the standard chess starting position,
unless otherwise specified in the optional *fen* argument.
If *fen* is ``None``, an empty board is created.

Optionally supports *chess960*. In Chess960, castling moves are encoded
by a king move to the corresponding rook square.
Use :func:`chess.Board.from_chess960_pos()` to create a board with one
of the Chess960 starting positions.

It's safe to set :data:`~Board.turn`, :data:`~Board.castling_rights`,
:data:`~Board.ep_square`, :data:`~Board.halfmove_clock` and
:data:`~Board.fullmove_number` directly.

.. warning::
    It is possible to set up and work with invalid positions. In this
    case, :class:`~chess.Board` implements a kind of "pseudo-chess"
    (useful to gracefully handle errors or to implement chess variants).
    Use :func:`~chess.Board.is_valid()` to detect invalid positions.
*/
public:
    std::vector<_BoardState> _stack;
    
    /*
    The move stack. Use :func:`Board.push() <chess.Board.push()>`,
    :func:`Board.pop() <chess.Board.pop()>`,
    :func:`Board.peek() <chess.Board.peek()>` and
    :func:`Board.clear_stack() <chess.Board.clear_stack()>` for
    manipulation.
    */
    std::vector<Move> move_stack;

    /*
    Bitmask of the rooks with castling rights.
    
    Use :func:`~chess.Board.set_castling_fen()` to set multiple castling
    rights. Also see :func:`~chess.Board.has_castling_rights()`,
    :func:`~chess.Board.has_kingside_castling_rights()`,
    :func:`~chess.Board.has_queenside_castling_rights()`,
    :func:`~chess.Board.has_chess960_castling_rights()`,
    :func:`~chess.Board.clean_castling_rights()`.
    */
    Bitboard castling_rights;
   
   
    // Counts move pairs. Starts at `1` and is incremented after every move of the black side.
    int fullmove_number;
    
    // The number of half-moves since the last capture or pawn move.
    int halfmove_clock;
   
    /*
    The potential en passant square on the third or sixth rank or ``None``.
    
    Use :func:`~chess.Board.has_legal_en_passant()` to test if en passant
    capturing would actually be possible on the next move.
    */
    Square ep_square;

    // The side to move (``chess.WHITE`` or ``chess.BLACK``).
    Color turn;

    Board(const std::optional<std::string>& fen = STARTING_FEN);

    Board(const Board& other) : Baseboard(other), turn(other.turn), castling_rights(other.castling_rights),
        ep_square(other.ep_square), fullmove_number(other.fullmove_number), halfmove_clock(other.halfmove_clock),
        move_stack(other.move_stack), _stack(other._stack) {}

    Board(const Baseboard& other);

    // Restore the starting position.
    void reset();

    void reset_board();

    /*
    Clears the board.

    Resets move stack and move counters. The side to move is white. There
    are no rooks or kings, so castling rights are removed.

    In order to be in a valid :func:`~chess.Board.status()`, at least kings
    need to be put on the board.
    */
    void clear();

    void clear_board();

    // Clears the move stack.
    void clear_stack();

    // Returns a copy of the root position.
    Board root();

    /*
    Returns the number of half-moves since the start of the game, as
    indicated by :data:`~chess.Board.fullmove_number` and
    :data:`~chess.Board.turn`.

    If moves have been pushed from the beginning, this is usually equal to
    ``len(board.move_stack)``. But note that a board can be set up with
    arbitrary starting positions, and the stack can be cleared.
    */
    int ply() const;

    std::optional<Piece> remove_piece_at(Square square);

    void set_piece_at(Square square, const std::optional<Piece>& piece);

    void generate_pseudo_legal_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_pseudo_legal_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    
    void generate_pseudo_safe_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Color color, Square king, Bitboard blockers, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_pseudo_safe_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Square king, Bitboard blockers, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    void generate_pseudo_legal_ep_by_color(StaticVector<Move, CASTLING_MOVES_SIZE>& return_moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_pseudo_legal_ep(StaticVector<Move, CASTLING_MOVES_SIZE>& return_moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    void generate_pseudo_legal_captures_by_color(StaticVector<Move, CAPTURES_SIZE>& pseudo_legal_moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_pseudo_legal_captures(StaticVector<Move, CAPTURES_SIZE>& pseudo_legal_moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    Bitboard checkers_mask_by_color(Color color) const;
    Bitboard checkers_mask() const;

    SquareSet checkers_by_color(Color color) const;
    SquareSet checkers() const;

    // Test if the given color is in check.
    bool is_check_by_color(Color color) const;
    // Test if the current side to move is in check.
    bool is_check() const;

    // Probes if the given move would put the opponent in check. The move
    // must be at least pseudo-legal.
    bool gives_check(const Move& move);

    bool is_into_check(const Move& move) const;

    bool was_into_check() const;

    bool is_pseudo_legal(const Move& move) const;

    bool is_legal(const Move& move) const;

    bool is_game_over();    // REMOVED parameter: bool claim_draw = false

    std::string result();   // REMOVED parameter: bool claim_draw = false

    /*
    Checks if the game is over due to
    :func:`checkmate <chess.Board.is_checkmate()>`,
    :func:`stalemate <chess.Board.is_stalemate()>`,
    :func:`insufficient material <chess.Board.is_insufficient_material()>`,
    the :func:`seventyfive-move rule <chess.Board.is_seventyfive_moves()>`,
    :func:`fivefold repetition <chess.Board.is_fivefold_repetition()>`,
    or a :func:`variant end condition <chess.Board.is_variant_end()>`.
    Returns the :class:`chess.Outcome` if the game has ended, otherwise
    ``None``.

    Alternatively, use :func:`~chess.Board.is_game_over()` if you are not
    interested in who won the game and why.

    The game is not considered to be over by the
    :func:`fifty-move rule <chess.Board.can_claim_fifty_moves()>` or
    :func:`threefold repetition <chess.Board.can_claim_threefold_repetition()>`,
    unless *claim_draw* is given. Note that checking the latter can be
    slow.
    */
    std::optional<Outcome> outcome(int repetition_count = 3);   // REMOVED parameter: bool claim_draw = false

    // Checks if the current position is a checkmate.
    bool is_checkmate() const;

    // Checks if the current position is a stalemate.
    bool is_stalemate() const;

    // Checks if neither side has sufficient winning material
    // (:func:`~chess.Board.has_insufficient_material()`).
    bool is_insufficient_material() const;

    /*
    Checks if *color* has insufficient winning material.

    This is guaranteed to return ``False`` if *color* can still win the
    game.

    The converse does not necessarily hold:
    The implementation only looks at the material, including the colors
    of bishops, but not considering piece positions. So fortress
    positions or positions with forced lines may return ``False``, even
    though there is no possible winning line.
    */
    bool has_insufficient_material(Color color) const;

    bool _is_halfmoves(const int& n) const;

    /*
    Since the 1st of July 2014, a game is automatically drawn (without
    a claim by one of the players) if the half-move clock since a capture
    or pawn move is equal to or greater than 150. Other means to end a game
    take precedence.
    */
    bool is_seventyfive_moves() const;

    /*
    Since the 1st of July 2014 a game is automatically drawn (without
    a claim by one of the players) if a position occurs for the fifth time.
    Originally this had to occur on consecutive alternating moves, but
    this has since been revised.
    */
    bool is_fivefold_repetition();

    /*
    Checks if the player to move can claim a draw by the fifty-move rule or
    by threefold repetition.

    Note that checking the latter can be slow.
    */
    bool can_claim_draw();

    /*
    Checks that the clock of halfmoves since the last capture or pawn move
    is greater or equal to 100, and that no other means of ending the game
    (like checkmate) take precedence.
    */
    bool is_fifty_moves() const;

    /*
    Checks if the player to move can claim a draw by the fifty-move rule.

    In addition to :func:`~chess.Board.is_fifty_moves()`, the fifty-move
    rule can also be claimed if there is a legal move that achieves this
    condition.
    */
    bool can_claim_fifty_moves();

    /*
    Checks if the current position has repeated 3 times.
    This is the same as :func:`~chess.Board.is_repetition()` with the
    default argument of 3.
    */
    bool is_threefold_repetition();

    /*
    Checks if the player to move can claim a draw by threefold repetition.

    Draw by threefold repetition can be claimed if the position on the
    board occurred for the third time or if such a repetition is reached
    with one of the possible legal moves.

    Note that checking this can be slow: In the worst case
    scenario, every legal move has to be tested and the entire game has to
    be replayed because there is no incremental transposition table.
    */
    bool can_claim_threefold_repetition();

    /*
    Checks if the current position has repeated 3 (or a given number of)
    times.

    Unlike :func:`~chess.Board.can_claim_threefold_repetition()`,
    this does not consider a repetition that can be played on the next
    move.

    Note that checking this can be slow: In the worst case, the entire
    game has to be replayed because there is no incremental transposition
    table.
    */
    bool is_repetition(int = 3);

    /*
    Updates the position with the given *move* and puts it onto the
    move stack.

    Null moves just increment the move counters, switch turns and forfeit
    en passant capturing.

    .. warning::
        Moves are not checked for legality. It is the caller's
        responsibility to ensure that the move is at least pseudo-legal or
        a null move.
    */
    void push(const Move& move);

    /*
    Restores the previous position and returns the last move from the stack.

    :raises: :exc:`IndexError` if the move stack is empty.
    */
    Move pop();

    /*
    Gets the last move from the move stack.

    :raises: :exc:`IndexError` if the move stack is empty.
    */
    Move peek() const;

    /*
    Finds a matching legal move for an origin square, a target square, and
    an optional promotion piece type.

    For pawn moves to the backrank, the promotion piece type defaults to
    :data:`chess.QUEEN`, unless otherwise specified.

    Castling moves are normalized to king moves by two steps, except in
    Chess960.

    :raises: :exc:`IllegalMoveError` if no matching legal move is found.
    */
    Move find_move(Square from_square, Square to_square, PieceType promotion = NULL_PIECE) const;

    std::string castling_shredder_fen() const;

    // TODO
    std::string castling_xfen() const {return "";};

    // Checks if there is a pseudo-legal en passant capture.
    bool has_pseudo_legal_en_passant() const;

    // Checks if there is a legal en passant capture.
    bool has_legal_en_passant() const;

    // TODO
    std::string fen(bool = false, EnPassantSpec = EnPassantSpec::legal, std::optional<bool> = std::nullopt) const {return "";};

    // TODO
    std::string shredder_fen(EnPassantSpec = EnPassantSpec::legal, std::optional<bool> = std::nullopt) const {return "";};

    /*
    Parses a FEN and sets the position from it.
 
    :raises: :exc:`ValueError` if syntactically invalid. Use
        :func:`~chess.Board.is_valid()` to detect invalid positions.
    */
    void set_fen(const std::string& fen);
 
    void _set_castling_fen(const std::string& fen);

    /*
    Sets castling rights from a string in FEN notation like ``Qqk``.

    Also clears the move stack.

    :raises: :exc:`ValueError` if the castling FEN is syntactically
        invalid.
    */
    void set_castling_fen(const std::string& castling_fen);

    void set_board_fen(const std::string& fen);

    void set_piece_map(const std::unordered_map<Square, Piece>& pieces);

    // Gets the standard algebraic notation of the given move in the context of the current position.
    std::string san(const Move& move);

    // Gets the long algebraic notation of the given move in the context of the current position.
    std::string lan(const Move& move);

    std::string san_and_push(const Move& move);

    std::string _algebraic(const Move& move, const bool& long_algebraic = false);

    std::string _algebraic_and_push(const Move& move, const bool& long_algebraic = false);

    std::string _algebraic_without_suffix(const Move& move, const bool& long_algebraic = false);

    // Gets the UCI notation of the move.
    std::string uci(const Move& move) const;

    /*
    Parses the given move in UCI notation.

    Supports both Chess960 and standard UCI notation.

    The returned move is guaranteed to be either legal or a null move.

    :raises:
        :exc:`ValueError` (specifically an exception specified below) if the move is invalid or illegal in the
        current position (but not a null move).

        - :exc:`InvalidMoveError` if the UCI is syntactically invalid.
        - :exc:`IllegalMoveError` if the UCI is illegal.
    */
    Move parse_uci(const std::string& uci) const;

    /*
    Parses a move in UCI notation and puts it on the move stack.

    Returns the move.

    :raises:
        :exc:`ValueError` (specifically an exception specified below) if the move is invalid or illegal in the
        current position (but not a null move).

        - :exc:`InvalidMoveError` if the UCI is syntactically invalid.
        - :exc:`IllegalMoveError` if the UCI is illegal.
    */
    Move push_uci(const std::string& uci);

    // Checks if the given pseudo-legal move is an en passant capture.
    bool is_en_passant(const Move& move) const;
    bool is_en_passant(Square from_square, Square to_square) const;

    // Checks if the given pseudo-legal move is a capture.
    bool is_capture(const Move& move) const;

    // Checks if the given pseudo-legal move is a capture or pawn move.
    bool is_zeroing(const Move& move) const;

    bool _reduces_castling_rights(const Move& move) const;

    /*
    Checks if the given pseudo-legal move is irreversible.

    In standard chess, pawn moves, captures, moves that destroy castling
    rights and moves that cede en passant are irreversible.

    This method has false-negatives with forced lines. For example, a check
    that will force the king to lose castling rights is not considered
    irreversible. Only the actual king move is.
    */
    bool is_irreversible(const Move& move) const;

    // Checks if the given pseudo-legal move is a castling move.
    bool is_castling(const Move& move) const;
    bool is_castling(Square from_square, Square to_square) const;

    bool is_kingside_castling(const Move& move) const;

    bool is_queenside_castling(const Move& move) const;

    // Returns valid castling rights filtered from :data:`~chess.Board.castling_rights`.
    Bitboard clean_castling_rights() const;

    // Checks if the given side has castling rights.
    bool has_castling_rights(Color color) const;

    // Checks if the given side has kingside castling rights.
    bool has_kingside_castling_rights(Color color) const;

    // Checks if the given side has queenside castling rights.
    bool has_queenside_castling_rights(Color color) const;

    /*
    Gets a bitmask of possible problems with the position.

    :data:`~chess.STATUS_VALID` if all basic validity requirements are met.
    This does not imply that the position is actually reachable with a
    series of legal moves from the starting position.

    Otherwise, bitwise combinations of:
    :data:`~chess.STATUS_NO_WHITE_KING`,
    :data:`~chess.STATUS_NO_BLACK_KING`,
    :data:`~chess.STATUS_TOO_MANY_KINGS`,
    :data:`~chess.STATUS_TOO_MANY_WHITE_PAWNS`,
    :data:`~chess.STATUS_TOO_MANY_BLACK_PAWNS`,
    :data:`~chess.STATUS_PAWNS_ON_BACKRANK`,
    :data:`~chess.STATUS_TOO_MANY_WHITE_PIECES`,
    :data:`~chess.STATUS_TOO_MANY_BLACK_PIECES`,
    :data:`~chess.STATUS_BAD_CASTLING_RIGHTS`,
    :data:`~chess.STATUS_INVALID_EP_SQUARE`,
    :data:`~chess.STATUS_OPPOSITE_CHECK`,
    :data:`~chess.STATUS_EMPTY`,
    :data:`~chess.STATUS_RACE_CHECK`,
    :data:`~chess.STATUS_RACE_OVER`,
    :data:`~chess.STATUS_RACE_MATERIAL`,
    :data:`~chess.STATUS_TOO_MANY_CHECKERS`,
    :data:`~chess.STATUS_IMPOSSIBLE_CHECK`.
    */
    Status status() const;

    Square _valid_ep_square() const;

    /*
    Checks some basic validity requirements.

    See :func:`~chess.Board.status()` for details.
    */
    bool is_valid() const;

    bool _ep_skewered_by_color(Color color, Square king, Square capturer) const;
    bool _ep_skewered(Square king, Square capturer) const;

    Bitboard _slider_blockers_by_color(Color color, Square king) const;
    Bitboard _slider_blockers(Square king) const;

    bool _is_safe_by_color(Color color, Square king, const Bitboard& blockers, const Move& move) const;
    bool _is_safe(Square king, const Bitboard& blockers, const Move& move) const;
    bool _is_safe_by_color(Color color, Square king, const Bitboard& blockers, Square from_square, Square to_square) const;
    bool _is_safe(Square king, const Bitboard& blockers, Square from_square, Square to_square) const;

    void _generate_evasions_by_color(StaticVector<Move, EVASION_SIZE>& moves, Color color, Square king, const Bitboard& checkers, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void _generate_evasions(StaticVector<Move, EVASION_SIZE>& moves, Square king, const Bitboard& checkers, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    void generate_legal_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_legal_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    void generate_legal_ep_by_color(StaticVector<Move, EP_CAPTURE_SIZE>& moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_legal_ep(StaticVector<Move, EP_CAPTURE_SIZE>& moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    void generate_legal_captures_by_color(StaticVector<Move, CAPTURES_SIZE>& moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_legal_captures(StaticVector<Move, CAPTURES_SIZE>& moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    bool _attacked_for_king_by_color(Color color, const Bitboard& path, const Bitboard& occupied) const;
    bool _attacked_for_king(const Bitboard& path, const Bitboard& occupied) const;

    void generate_castling_moves_by_color(StaticVector<Move, CASTLING_MOVES_SIZE>& moves, Color color, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;
    void generate_castling_moves(StaticVector<Move, CASTLING_MOVES_SIZE>& moves, const Bitboard& from_mask = BB_ALL, const Bitboard& to_mask = BB_ALL) const;

    Move _from_chess960(Square from_square, Square to_square, PieceType promotion = NULL_PIECE) const;

    Square _is_from_chess960(const Move& move) const;

    void _to_chess960(Move& move) const;

    Square _is_to_chess960(const Move& move) const;

    BoardStateTuple _transposition_key() const;

    bool operator==(const Board& other) const;
    bool operator!=(const Board& other) const;

    void apply_transform(const std::function<Bitboard(Bitboard)>& f);

    Board transform(const std::function<Bitboard(Bitboard)>& f);

    void apply_mirror();

    /*
    Returns a mirrored copy of the board.

    The board is mirrored vertically and piece colors are swapped, so that
    the position is equivalent modulo color. Also swap the "en passant"
    square, castling rights and turn.

    Alternatively, :func:`~chess.Board.apply_mirror()` can be used
    to mirror the board.
    */
    Board mirror() const;

    /*
    Creates a copy of the board.

    Defaults to copying the entire move stack. Alternatively, *stack* can
    be ``False``, or an integer to copy a limited number of moves.
    */
    Board copy(int stack = -1) const;

    std::string to_string();

    Bitboard hash() const;

    // Creates a new empty board. Also see :func:`~chess.Board.clear()`.
    static Board empty();
};

class _BoardState {
public:
    Bitboard occupied_w;
    Bitboard occupied_b;
    Bitboard occupied;
    Bitboard pawns;
    Bitboard knights;
    Bitboard bishops;
    Bitboard rooks;
    Bitboard queens;
    Bitboard kings;
    Bitboard castling_rights;
    int material_w_mg;
    int material_w_eg;
    int material_b_mg;
    int material_b_eg;
    int pawns_w_mg;
    int pawns_w_eg;
    int pawns_b_mg;
    int pawns_b_eg;
    int fullmove_number;
    int halfmove_clock;
    int game_phase;
    Square ep_square;
    Color turn;

    _BoardState(const Board& board);

    void restore(Board&);
};

const uint64_t POLYGLOT_RANDOM_ARRAY[] = {
    0x9D39247E33776D41, 0x2AF7398005AAA5C7, 0x44DB015024623547, 0x9C15F73E62A76AE2,
    0x75834465489C0C89, 0x3290AC3A203001BF, 0x0FBBAD1F61042279, 0xE83A908FF2FB60CA,
    0x0D7E765D58755C10, 0x1A083822CEAFE02D, 0x9605D5F0E25EC3B0, 0xD021FF5CD13A2ED5,
    0x40BDF15D4A672E32, 0x011355146FD56395, 0x5DB4832046F3D9E5, 0x239F8B2D7FF719CC,
    0x05D1A1AE85B49AA1, 0x679F848F6E8FC971, 0x7449BBFF801FED0B, 0x7D11CDB1C3B7ADF0,
    0x82C7709E781EB7CC, 0xF3218F1C9510786C, 0x331478F3AF51BBE6, 0x4BB38DE5E7219443,
    0xAA649C6EBCFD50FC, 0x8DBD98A352AFD40B, 0x87D2074B81D79217, 0x19F3C751D3E92AE1,
    0xB4AB30F062B19ABF, 0x7B0500AC42047AC4, 0xC9452CA81A09D85D, 0x24AA6C514DA27500,
    0x4C9F34427501B447, 0x14A68FD73C910841, 0xA71B9B83461CBD93, 0x03488B95B0F1850F,
    0x637B2B34FF93C040, 0x09D1BC9A3DD90A94, 0x3575668334A1DD3B, 0x735E2B97A4C45A23,
    0x18727070F1BD400B, 0x1FCBACD259BF02E7, 0xD310A7C2CE9B6555, 0xBF983FE0FE5D8244,
    0x9F74D14F7454A824, 0x51EBDC4AB9BA3035, 0x5C82C505DB9AB0FA, 0xFCF7FE8A3430B241,
    0x3253A729B9BA3DDE, 0x8C74C368081B3075, 0xB9BC6C87167C33E7, 0x7EF48F2B83024E20,
    0x11D505D4C351BD7F, 0x6568FCA92C76A243, 0x4DE0B0F40F32A7B8, 0x96D693460CC37E5D,
    0x42E240CB63689F2F, 0x6D2BDCDAE2919661, 0x42880B0236E4D951, 0x5F0F4A5898171BB6,
    0x39F890F579F92F88, 0x93C5B5F47356388B, 0x63DC359D8D231B78, 0xEC16CA8AEA98AD76,
    0x5355F900C2A82DC7, 0x07FB9F855A997142, 0x5093417AA8A7ED5E, 0x7BCBC38DA25A7F3C,
    0x19FC8A768CF4B6D4, 0x637A7780DECFC0D9, 0x8249A47AEE0E41F7, 0x79AD695501E7D1E8,
    0x14ACBAF4777D5776, 0xF145B6BECCDEA195, 0xDABF2AC8201752FC, 0x24C3C94DF9C8D3F6,
    0xBB6E2924F03912EA, 0x0CE26C0B95C980D9, 0xA49CD132BFBF7CC4, 0xE99D662AF4243939,
    0x27E6AD7891165C3F, 0x8535F040B9744FF1, 0x54B3F4FA5F40D873, 0x72B12C32127FED2B,
    0xEE954D3C7B411F47, 0x9A85AC909A24EAA1, 0x70AC4CD9F04F21F5, 0xF9B89D3E99A075C2,
    0x87B3E2B2B5C907B1, 0xA366E5B8C54F48B8, 0xAE4A9346CC3F7CF2, 0x1920C04D47267BBD,
    0x87BF02C6B49E2AE9, 0x092237AC237F3859, 0xFF07F64EF8ED14D0, 0x8DE8DCA9F03CC54E,
    0x9C1633264DB49C89, 0xB3F22C3D0B0B38ED, 0x390E5FB44D01144B, 0x5BFEA5B4712768E9,
    0x1E1032911FA78984, 0x9A74ACB964E78CB3, 0x4F80F7A035DAFB04, 0x6304D09A0B3738C4,
    0x2171E64683023A08, 0x5B9B63EB9CEFF80C, 0x506AACF489889342, 0x1881AFC9A3A701D6,
    0x6503080440750644, 0xDFD395339CDBF4A7, 0xEF927DBCF00C20F2, 0x7B32F7D1E03680EC,
    0xB9FD7620E7316243, 0x05A7E8A57DB91B77, 0xB5889C6E15630A75, 0x4A750A09CE9573F7,
    0xCF464CEC899A2F8A, 0xF538639CE705B824, 0x3C79A0FF5580EF7F, 0xEDE6C87F8477609D,
    0x799E81F05BC93F31, 0x86536B8CF3428A8C, 0x97D7374C60087B73, 0xA246637CFF328532,
    0x043FCAE60CC0EBA0, 0x920E449535DD359E, 0x70EB093B15B290CC, 0x73A1921916591CBD,
    0x56436C9FE1A1AA8D, 0xEFAC4B70633B8F81, 0xBB215798D45DF7AF, 0x45F20042F24F1768,
    0x930F80F4E8EB7462, 0xFF6712FFCFD75EA1, 0xAE623FD67468AA70, 0xDD2C5BC84BC8D8FC,
    0x7EED120D54CF2DD9, 0x22FE545401165F1C, 0xC91800E98FB99929, 0x808BD68E6AC10365,
    0xDEC468145B7605F6, 0x1BEDE3A3AEF53302, 0x43539603D6C55602, 0xAA969B5C691CCB7A,
    0xA87832D392EFEE56, 0x65942C7B3C7E11AE, 0xDED2D633CAD004F6, 0x21F08570F420E565,
    0xB415938D7DA94E3C, 0x91B859E59ECB6350, 0x10CFF333E0ED804A, 0x28AED140BE0BB7DD,
    0xC5CC1D89724FA456, 0x5648F680F11A2741, 0x2D255069F0B7DAB3, 0x9BC5A38EF729ABD4,
    0xEF2F054308F6A2BC, 0xAF2042F5CC5C2858, 0x480412BAB7F5BE2A, 0xAEF3AF4A563DFE43,
    0x19AFE59AE451497F, 0x52593803DFF1E840, 0xF4F076E65F2CE6F0, 0x11379625747D5AF3,
    0xBCE5D2248682C115, 0x9DA4243DE836994F, 0x066F70B33FE09017, 0x4DC4DE189B671A1C,
    0x51039AB7712457C3, 0xC07A3F80C31FB4B4, 0xB46EE9C5E64A6E7C, 0xB3819A42ABE61C87,
    0x21A007933A522A20, 0x2DF16F761598AA4F, 0x763C4A1371B368FD, 0xF793C46702E086A0,
    0xD7288E012AEB8D31, 0xDE336A2A4BC1C44B, 0x0BF692B38D079F23, 0x2C604A7A177326B3,
    0x4850E73E03EB6064, 0xCFC447F1E53C8E1B, 0xB05CA3F564268D99, 0x9AE182C8BC9474E8,
    0xA4FC4BD4FC5558CA, 0xE755178D58FC4E76, 0x69B97DB1A4C03DFE, 0xF9B5B7C4ACC67C96,
    0xFC6A82D64B8655FB, 0x9C684CB6C4D24417, 0x8EC97D2917456ED0, 0x6703DF9D2924E97E,
    0xC547F57E42A7444E, 0x78E37644E7CAD29E, 0xFE9A44E9362F05FA, 0x08BD35CC38336615,
    0x9315E5EB3A129ACE, 0x94061B871E04DF75, 0xDF1D9F9D784BA010, 0x3BBA57B68871B59D,
    0xD2B7ADEEDED1F73F, 0xF7A255D83BC373F8, 0xD7F4F2448C0CEB81, 0xD95BE88CD210FFA7,
    0x336F52F8FF4728E7, 0xA74049DAC312AC71, 0xA2F61BB6E437FDB5, 0x4F2A5CB07F6A35B3,
    0x87D380BDA5BF7859, 0x16B9F7E06C453A21, 0x7BA2484C8A0FD54E, 0xF3A678CAD9A2E38C,
    0x39B0BF7DDE437BA2, 0xFCAF55C1BF8A4424, 0x18FCF680573FA594, 0x4C0563B89F495AC3,
    0x40E087931A00930D, 0x8CFFA9412EB642C1, 0x68CA39053261169F, 0x7A1EE967D27579E2,
    0x9D1D60E5076F5B6F, 0x3810E399B6F65BA2, 0x32095B6D4AB5F9B1, 0x35CAB62109DD038A,
    0xA90B24499FCFAFB1, 0x77A225A07CC2C6BD, 0x513E5E634C70E331, 0x4361C0CA3F692F12,
    0xD941ACA44B20A45B, 0x528F7C8602C5807B, 0x52AB92BEB9613989, 0x9D1DFA2EFC557F73,
    0x722FF175F572C348, 0x1D1260A51107FE97, 0x7A249A57EC0C9BA2, 0x04208FE9E8F7F2D6,
    0x5A110C6058B920A0, 0x0CD9A497658A5698, 0x56FD23C8F9715A4C, 0x284C847B9D887AAE,
    0x04FEABFBBDB619CB, 0x742E1E651C60BA83, 0x9A9632E65904AD3C, 0x881B82A13B51B9E2,
    0x506E6744CD974924, 0xB0183DB56FFC6A79, 0x0ED9B915C66ED37E, 0x5E11E86D5873D484,
    0xF678647E3519AC6E, 0x1B85D488D0F20CC5, 0xDAB9FE6525D89021, 0x0D151D86ADB73615,
    0xA865A54EDCC0F019, 0x93C42566AEF98FFB, 0x99E7AFEABE000731, 0x48CBFF086DDF285A,
    0x7F9B6AF1EBF78BAF, 0x58627E1A149BBA21, 0x2CD16E2ABD791E33, 0xD363EFF5F0977996,
    0x0CE2A38C344A6EED, 0x1A804AADB9CFA741, 0x907F30421D78C5DE, 0x501F65EDB3034D07,
    0x37624AE5A48FA6E9, 0x957BAF61700CFF4E, 0x3A6C27934E31188A, 0xD49503536ABCA345,
    0x088E049589C432E0, 0xF943AEE7FEBF21B8, 0x6C3B8E3E336139D3, 0x364F6FFA464EE52E,
    0xD60F6DCEDC314222, 0x56963B0DCA418FC0, 0x16F50EDF91E513AF, 0xEF1955914B609F93,
    0x565601C0364E3228, 0xECB53939887E8175, 0xBAC7A9A18531294B, 0xB344C470397BBA52,
    0x65D34954DAF3CEBD, 0xB4B81B3FA97511E2, 0xB422061193D6F6A7, 0x071582401C38434D,
    0x7A13F18BBEDC4FF5, 0xBC4097B116C524D2, 0x59B97885E2F2EA28, 0x99170A5DC3115544,
    0x6F423357E7C6A9F9, 0x325928EE6E6F8794, 0xD0E4366228B03343, 0x565C31F7DE89EA27,
    0x30F5611484119414, 0xD873DB391292ED4F, 0x7BD94E1D8E17DEBC, 0xC7D9F16864A76E94,
    0x947AE053EE56E63C, 0xC8C93882F9475F5F, 0x3A9BF55BA91F81CA, 0xD9A11FBB3D9808E4,
    0x0FD22063EDC29FCA, 0xB3F256D8ACA0B0B9, 0xB03031A8B4516E84, 0x35DD37D5871448AF,
    0xE9F6082B05542E4E, 0xEBFAFA33D7254B59, 0x9255ABB50D532280, 0xB9AB4CE57F2D34F3,
    0x693501D628297551, 0xC62C58F97DD949BF, 0xCD454F8F19C5126A, 0xBBE83F4ECC2BDECB,
    0xDC842B7E2819E230, 0xBA89142E007503B8, 0xA3BC941D0A5061CB, 0xE9F6760E32CD8021,
    0x09C7E552BC76492F, 0x852F54934DA55CC9, 0x8107FCCF064FCF56, 0x098954D51FFF6580,
    0x23B70EDB1955C4BF, 0xC330DE426430F69D, 0x4715ED43E8A45C0A, 0xA8D7E4DAB780A08D,
    0x0572B974F03CE0BB, 0xB57D2E985E1419C7, 0xE8D9ECBE2CF3D73F, 0x2FE4B17170E59750,
    0x11317BA87905E790, 0x7FBF21EC8A1F45EC, 0x1725CABFCB045B00, 0x964E915CD5E2B207,
    0x3E2B8BCBF016D66D, 0xBE7444E39328A0AC, 0xF85B2B4FBCDE44B7, 0x49353FEA39BA63B1,
    0x1DD01AAFCD53486A, 0x1FCA8A92FD719F85, 0xFC7C95D827357AFA, 0x18A6A990C8B35EBD,
    0xCCCB7005C6B9C28D, 0x3BDBB92C43B17F26, 0xAA70B5B4F89695A2, 0xE94C39A54A98307F,
    0xB7A0B174CFF6F36E, 0xD4DBA84729AF48AD, 0x2E18BC1AD9704A68, 0x2DE0966DAF2F8B1C,
    0xB9C11D5B1E43A07E, 0x64972D68DEE33360, 0x94628D38D0C20584, 0xDBC0D2B6AB90A559,
    0xD2733C4335C6A72F, 0x7E75D99D94A70F4D, 0x6CED1983376FA72B, 0x97FCAACBF030BC24,
    0x7B77497B32503B12, 0x8547EDDFB81CCB94, 0x79999CDFF70902CB, 0xCFFE1939438E9B24,
    0x829626E3892D95D7, 0x92FAE24291F2B3F1, 0x63E22C147B9C3403, 0xC678B6D860284A1C,
    0x5873888850659AE7, 0x0981DCD296A8736D, 0x9F65789A6509A440, 0x9FF38FED72E9052F,
    0xE479EE5B9930578C, 0xE7F28ECD2D49EECD, 0x56C074A581EA17FE, 0x5544F7D774B14AEF,
    0x7B3F0195FC6F290F, 0x12153635B2C0CF57, 0x7F5126DBBA5E0CA7, 0x7A76956C3EAFB413,
    0x3D5774A11D31AB39, 0x8A1B083821F40CB4, 0x7B4A38E32537DF62, 0x950113646D1D6E03,
    0x4DA8979A0041E8A9, 0x3BC36E078F7515D7, 0x5D0A12F27AD310D1, 0x7F9D1A2E1EBE1327,
    0xDA3A361B1C5157B1, 0xDCDD7D20903D0C25, 0x36833336D068F707, 0xCE68341F79893389,
    0xAB9090168DD05F34, 0x43954B3252DC25E5, 0xB438C2B67F98E5E9, 0x10DCD78E3851A492,
    0xDBC27AB5447822BF, 0x9B3CDB65F82CA382, 0xB67B7896167B4C84, 0xBFCED1B0048EAC50,
    0xA9119B60369FFEBD, 0x1FFF7AC80904BF45, 0xAC12FB171817EEE7, 0xAF08DA9177DDA93D,
    0x1B0CAB936E65C744, 0xB559EB1D04E5E932, 0xC37B45B3F8D6F2BA, 0xC3A9DC228CAAC9E9,
    0xF3B8B6675A6507FF, 0x9FC477DE4ED681DA, 0x67378D8ECCEF96CB, 0x6DD856D94D259236,
    0xA319CE15B0B4DB31, 0x073973751F12DD5E, 0x8A8E849EB32781A5, 0xE1925C71285279F5,
    0x74C04BF1790C0EFE, 0x4DDA48153C94938A, 0x9D266D6A1CC0542C, 0x7440FB816508C4FE,
    0x13328503DF48229F, 0xD6BF7BAEE43CAC40, 0x4838D65F6EF6748F, 0x1E152328F3318DEA,
    0x8F8419A348F296BF, 0x72C8834A5957B511, 0xD7A023A73260B45C, 0x94EBC8ABCFB56DAE,
    0x9FC10D0F989993E0, 0xDE68A2355B93CAE6, 0xA44CFE79AE538BBE, 0x9D1D84FCCE371425,
    0x51D2B1AB2DDFB636, 0x2FD7E4B9E72CD38C, 0x65CA5B96B7552210, 0xDD69A0D8AB3B546D,
    0x604D51B25FBF70E2, 0x73AA8A564FB7AC9E, 0x1A8C1E992B941148, 0xAAC40A2703D9BEA0,
    0x764DBEAE7FA4F3A6, 0x1E99B96E70A9BE8B, 0x2C5E9DEB57EF4743, 0x3A938FEE32D29981,
    0x26E6DB8FFDF5ADFE, 0x469356C504EC9F9D, 0xC8763C5B08D1908C, 0x3F6C6AF859D80055,
    0x7F7CC39420A3A545, 0x9BFB227EBDF4C5CE, 0x89039D79D6FC5C5C, 0x8FE88B57305E2AB6,
    0xA09E8C8C35AB96DE, 0xFA7E393983325753, 0xD6B6D0ECC617C699, 0xDFEA21EA9E7557E3,
    0xB67C1FA481680AF8, 0xCA1E3785A9E724E5, 0x1CFC8BED0D681639, 0xD18D8549D140CAEA,
    0x4ED0FE7E9DC91335, 0xE4DBF0634473F5D2, 0x1761F93A44D5AEFE, 0x53898E4C3910DA55,
    0x734DE8181F6EC39A, 0x2680B122BAA28D97, 0x298AF231C85BAFAB, 0x7983EED3740847D5,
    0x66C1A2A1A60CD889, 0x9E17E49642A3E4C1, 0xEDB454E7BADC0805, 0x50B704CAB602C329,
    0x4CC317FB9CDDD023, 0x66B4835D9EAFEA22, 0x219B97E26FFC81BD, 0x261E4E4C0A333A9D,
    0x1FE2CCA76517DB90, 0xD7504DFA8816EDBB, 0xB9571FA04DC089C8, 0x1DDC0325259B27DE,
    0xCF3F4688801EB9AA, 0xF4F5D05C10CAB243, 0x38B6525C21A42B0E, 0x36F60E2BA4FA6800,
    0xEB3593803173E0CE, 0x9C4CD6257C5A3603, 0xAF0C317D32ADAA8A, 0x258E5A80C7204C4B,
    0x8B889D624D44885D, 0xF4D14597E660F855, 0xD4347F66EC8941C3, 0xE699ED85B0DFB40D,
    0x2472F6207C2D0484, 0xC2A1E7B5B459AEB5, 0xAB4F6451CC1D45EC, 0x63767572AE3D6174,
    0xA59E0BD101731A28, 0x116D0016CB948F09, 0x2CF9C8CA052F6E9F, 0x0B090A7560A968E3,
    0xABEEDDB2DDE06FF1, 0x58EFC10B06A2068D, 0xC6E57A78FBD986E0, 0x2EAB8CA63CE802D7,
    0x14A195640116F336, 0x7C0828DD624EC390, 0xD74BBE77E6116AC7, 0x804456AF10F5FB53,
    0xEBE9EA2ADF4321C7, 0x03219A39EE587A30, 0x49787FEF17AF9924, 0xA1E9300CD8520548,
    0x5B45E522E4B1B4EF, 0xB49C3B3995091A36, 0xD4490AD526F14431, 0x12A8F216AF9418C2,
    0x001F837CC7350524, 0x1877B51E57A764D5, 0xA2853B80F17F58EE, 0x993E1DE72D36D310,
    0xB3598080CE64A656, 0x252F59CF0D9F04BB, 0xD23C8E176D113600, 0x1BDA0492E7E4586E,
    0x21E0BD5026C619BF, 0x3B097ADAF088F94E, 0x8D14DEDB30BE846E, 0xF95CFFA23AF5F6F4,
    0x3871700761B3F743, 0xCA672B91E9E4FA16, 0x64C8E531BFF53B55, 0x241260ED4AD1E87D,
    0x106C09B972D2E822, 0x7FBA195410E5CA30, 0x7884D9BC6CB569D8, 0x0647DFEDCD894A29,
    0x63573FF03E224774, 0x4FC8E9560F91B123, 0x1DB956E450275779, 0xB8D91274B9E9D4FB,
    0xA2EBEE47E2FBFCE1, 0xD9F1F30CCD97FB09, 0xEFED53D75FD64E6B, 0x2E6D02C36017F67F,
    0xA9AA4D20DB084E9B, 0xB64BE8D8B25396C1, 0x70CB6AF7C2D5BCF0, 0x98F076A4F7A2322E,
    0xBF84470805E69B5F, 0x94C3251F06F90CF3, 0x3E003E616A6591E9, 0xB925A6CD0421AFF3,
    0x61BDD1307C66E300, 0xBF8D5108E27E0D48, 0x240AB57A8B888B20, 0xFC87614BAF287E07,
    0xEF02CDD06FFDB432, 0xA1082C0466DF6C0A, 0x8215E577001332C8, 0xD39BB9C3A48DB6CF,
    0x2738259634305C14, 0x61CF4F94C97DF93D, 0x1B6BACA2AE4E125B, 0x758F450C88572E0B,
    0x959F587D507A8359, 0xB063E962E045F54D, 0x60E8ED72C0DFF5D1, 0x7B64978555326F9F,
    0xFD080D236DA814BA, 0x8C90FD9B083F4558, 0x106F72FE81E2C590, 0x7976033A39F7D952,
    0xA4EC0132764CA04B, 0x733EA705FAE4FA77, 0xB4D8F77BC3E56167, 0x9E21F4F903B33FD9,
    0x9D765E419FB69F6D, 0xD30C088BA61EA5EF, 0x5D94337FBFAF7F5B, 0x1A4E4822EB4D7A59,
    0x6FFE73E81B637FB3, 0xDDF957BC36D8B9CA, 0x64D0E29EEA8838B3, 0x08DD9BDFD96B9F63,
    0x087E79E5A57D1D13, 0xE328E230E3E2B3FB, 0x1C2559E30F0946BE, 0x720BF5F26F4D2EAA,
    0xB0774D261CC609DB, 0x443F64EC5A371195, 0x4112CF68649A260E, 0xD813F2FAB7F5C5CA,
    0x660D3257380841EE, 0x59AC2C7873F910A3, 0xE846963877671A17, 0x93B633ABFA3469F8,
    0xC0C0F5A60EF4CDCF, 0xCAF21ECD4377B28C, 0x57277707199B8175, 0x506C11B9D90E8B1D,
    0xD83CC2687A19255F, 0x4A29C6465A314CD1, 0xED2DF21216235097, 0xB5635C95FF7296E2,
    0x22AF003AB672E811, 0x52E762596BF68235, 0x9AEBA33AC6ECC6B0, 0x944F6DE09134DFB6,
    0x6C47BEC883A7DE39, 0x6AD047C430A12104, 0xA5B1CFDBA0AB4067, 0x7C45D833AFF07862,
    0x5092EF950A16DA0B, 0x9338E69C052B8E7B, 0x455A4B4CFE30E3F5, 0x6B02E63195AD0CF8,
    0x6B17B224BAD6BF27, 0xD1E0CCD25BB9C169, 0xDE0C89A556B9AE70, 0x50065E535A213CF6,
    0x9C1169FA2777B874, 0x78EDEFD694AF1EED, 0x6DC93D9526A50E68, 0xEE97F453F06791ED,
    0x32AB0EDB696703D3, 0x3A6853C7E70757A7, 0x31865CED6120F37D, 0x67FEF95D92607890,
    0x1F2B1D1F15F6DC9C, 0xB69E38A8965C6B65, 0xAA9119FF184CCCF4, 0xF43C732873F24C13,
    0xFB4A3D794A9A80D2, 0x3550C2321FD6109C, 0x371F77E76BB8417E, 0x6BFA9AAE5EC05779,
    0xCD04F3FF001A4778, 0xE3273522064480CA, 0x9F91508BFFCFC14A, 0x049A7F41061A9E60,
    0xFCB6BE43A9F2FE9B, 0x08DE8A1C7797DA9B, 0x8F9887E6078735A1, 0xB5B4071DBFC73A66,
    0x230E343DFBA08D33, 0x43ED7F5A0FAE657D, 0x3A88A0FBBCB05C63, 0x21874B8B4D2DBC4F,
    0x1BDEA12E35F6A8C9, 0x53C065C6C8E63528, 0xE34A1D250E7A8D6B, 0xD6B04D3B7651DD7E,
    0x5E90277E7CB39E2D, 0x2C046F22062DC67D, 0xB10BB459132D0A26, 0x3FA9DDFB67E2F199,
    0x0E09B88E1914F7AF, 0x10E8B35AF3EEAB37, 0x9EEDECA8E272B933, 0xD4C718BC4AE8AE5F,
    0x81536D601170FC20, 0x91B534F885818A06, 0xEC8177F83F900978, 0x190E714FADA5156E,
    0xB592BF39B0364963, 0x89C350C893AE7DC1, 0xAC042E70F8B383F2, 0xB49B52E587A1EE60,
    0xFB152FE3FF26DA89, 0x3E666E6F69AE2C15, 0x3B544EBE544C19F9, 0xE805A1E290CF2456,
    0x24B33C9D7ED25117, 0xE74733427B72F0C1, 0x0A804D18B7097475, 0x57E3306D881EDB4F,
    0x4AE7D6A36EB5DBCB, 0x2D8D5432157064C8, 0xD1E649DE1E7F268B, 0x8A328A1CEDFE552C,
    0x07A3AEC79624C7DA, 0x84547DDC3E203C94, 0x990A98FD5071D263, 0x1A4FF12616EEFC89,
    0xF6F7FD1431714200, 0x30C05B1BA332F41C, 0x8D2636B81555A786, 0x46C9FEB55D120902,
    0xCCEC0A73B49C9921, 0x4E9D2827355FC492, 0x19EBB029435DCB0F, 0x4659D2B743848A2C,
    0x963EF2C96B33BE31, 0x74F85198B05A2E7D, 0x5A0F544DD2B1FB18, 0x03727073C2E134B1,
    0xC7F6AA2DE59AEA61, 0x352787BAA0D7C22F, 0x9853EAB63B5E0B35, 0xABBDCDD7ED5C0860,
    0xCF05DAF5AC8D77B0, 0x49CAD48CEBF4A71E, 0x7A4C10EC2158C4A6, 0xD9E92AA246BF719E,
    0x13AE978D09FE5557, 0x730499AF921549FF, 0x4E4B705B92903BA4, 0xFF577222C14F0A3A,
    0x55B6344CF97AAFAE, 0xB862225B055B6960, 0xCAC09AFBDDD2CDB4, 0xDAF8E9829FE96B5F,
    0xB5FDFC5D3132C498, 0x310CB380DB6F7503, 0xE87FBB46217A360E, 0x2102AE466EBB1148,
    0xF8549E1A3AA5E00D, 0x07A69AFDCC42261A, 0xC4C118BFE78FEAAE, 0xF9F4892ED96BD438,
    0x1AF3DBE25D8F45DA, 0xF5B4B0B0D2DEEEB4, 0x962ACEEFA82E1C84, 0x046E3ECAAF453CE9,
    0xF05D129681949A4C, 0x964781CE734B3C84, 0x9C2ED44081CE5FBD, 0x522E23F3925E319E,
    0x177E00F9FC32F791, 0x2BC60A63A6F3B3F2, 0x222BBFAE61725606, 0x486289DDCC3D6780,
    0x7DC7785B8EFDFC80, 0x8AF38731C02BA980, 0x1FAB64EA29A2DDF7, 0xE4D9429322CD065A,
    0x9DA058C67844F20C, 0x24C0E332B70019B0, 0x233003B5A6CFE6AD, 0xD586BD01C5C217F6,
    0x5E5637885F29BC2B, 0x7EBA726D8C94094B, 0x0A56A5F0BFE39272, 0xD79476A84EE20D06,
    0x9E4C1269BAA4BF37, 0x17EFEE45B0DEE640, 0x1D95B0A5FCF90BC6, 0x93CBE0B699C2585D,
    0x65FA4F227A2B6D79, 0xD5F9E858292504D5, 0xC2B5A03F71471A6F, 0x59300222B4561E00,
    0xCE2F8642CA0712DC, 0x7CA9723FBB2E8988, 0x2785338347F2BA08, 0xC61BB3A141E50E8C,
    0x150F361DAB9DEC26, 0x9F6A419D382595F4, 0x64A53DC924FE7AC9, 0x142DE49FFF7A7C3D,
    0x0C335248857FA9E7, 0x0A9C32D5EAE45305, 0xE6C42178C4BBB92E, 0x71F1CE2490D20B07,
    0xF1BCC3D275AFE51A, 0xE728E8C83C334074, 0x96FBF83A12884624, 0x81A1549FD6573DA5,
    0x5FA7867CAF35E149, 0x56986E2EF3ED091B, 0x917F1DD5F8886C61, 0xD20D8C88C8FFE65F,
    0x31D71DCE64B2C310, 0xF165B587DF898190, 0xA57E6339DD2CF3A0, 0x1EF6E6DBB1961EC9,
    0x70CC73D90BC26E24, 0xE21A6B35DF0C3AD7, 0x003A93D8B2806962, 0x1C99DED33CB890A1,
    0xCF3145DE0ADD4289, 0xD0E4427A5514FB72, 0x77C621CC9FB3A483, 0x67A34DAC4356550B,
    0xF8D626AAAF278509
};

#endif