#include "chess.hpp"

int find_first_of(const std::string str[], const std::string& target, int size) {
    // Finds the first occurrence of a target string in an array of strings.
    for (int i = 0; i < size; ++i) {
        if (str[i] == target) {
            return i;
        }
    }
    return -1; // Not found
}

void split(std::vector<std::string>& result, const std::string& s, char delimiter) {
    // Splits a string by a delimiter and appends the results to the provided vector.
    std::string token;
    std::istringstream stream(s);
    while (std::getline(stream, token, delimiter)) {
        result.push_back(token);
    }
}

template<typename T>
T accumulate(typename std::vector<T>::const_iterator begin, typename std::vector<T>::const_iterator end, T init) {
    for (auto it = begin; it != end; ++it) {
        init += *it;
    }
    return init;
}

Status operator|=(Status lhs, Status rhs) {
    return static_cast<Status>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

std::string piece_symbol(const PieceType &piece) {
    return std::string(1, PIECE_SYMBOLS[piece]);
}

std::string piece_name(const PieceType &piece) {
    return PIECE_NAMES[piece];
}

std::string Outcome::result() const {
    if (!winner.has_value()) {
        return "1/2-1/2";
    }
    return *winner ? "1-0" : "0-1";
}

bool Outcome::operator==(const Outcome& other) const {
    return termination == other.termination && winner == other.winner;
}

bool Outcome::operator!=(const Outcome& other) const {
    return !(*this == other);
}

std::ostream& Outcome::operator<<(std::ostream& os) const {
    os << result();
    return os;
}

Outcome::operator std::string() const {
    return result();
}

std::string Outcome::to_string() const {
    std::ostringstream out;
    out << result();
    return out.str();
}

Square parse_square(const std::string& name) {
    /*
    Gets the square index for the given square *name*
    (e.g., ``a1`` returns ``0``).

    :raises: :exc:`ValueError` if the square name is invalid.
    */
    for (unsigned int i = 0; i < 64; ++i) {
        if (SQUARE_NAMES[i] == name) {
            return SQUARES[i];
        }
    }
    throw std::invalid_argument("Invalid square name");
}

std::string square_name(Square square) {
    // Gets the name of the square, like ``a3``.
    return SQUARE_NAMES[square];
}

Square square(int file_index, int rank_index) {
    // Gets a square number by file and rank index.
    return rank_index * 8 + file_index;
}

int square_file(Square square) {
    // Gets the file index of the square where ``0`` is the a-file.
    return square & 7; // square % 8
}

int square_rank(Square square) {
    // Gets the rank index of the square where ``0`` is the first rank.
    return square >> 3; // square // 8
}

Square square_mirror(Square square) {
    // Mirrors the square vertically.
    return square ^ 0x38;   // square ^ 56
}

int lsb(const Bitboard& bb) {
    // Gets the least significant bit index.
    return __builtin_ctzll(bb);
}

std::vector<Square> scan_forward(Bitboard bb) {
    // Scans the bitboard and returns the square indices.
    std::vector<Square> squares;
    while (bb) {
        squares.push_back(lsb(bb));
        bb &= bb - 1;
    }
    return squares;
}

template<std::size_t T>
void scan_forward(Bitboard bb, StaticVector<Square, T>& squares) {
    // Scans the bitboard and returns the square indices.
    squares.clear();
    while (bb) {
        squares.push_back(lsb(bb));
        bb &= bb - 1;
    }
}

// Explicit template instantiations for commonly used StaticVector sizes.
// This ensures the compiler generates the needed symbol (prevents undefined reference at link time).
template void scan_forward<2>(Bitboard, StaticVector<Square, 2>&);
template void scan_forward<8>(Bitboard, StaticVector<Square, 8>&);
template void scan_forward<16>(Bitboard, StaticVector<Square, 16>&);
template void scan_forward<32>(Bitboard, StaticVector<Square, 32>&);
template void scan_forward<64>(Bitboard, StaticVector<Square, 64>&);

int msb(const Bitboard& bb) {
    // Gets the most significant bit index. If *bb* is empty, returns -1.
    return 63 - __builtin_clzll(bb);
}

std::vector<Square> scan_reversed(Bitboard bb) {
    // Scans the bitboard in reverse and returns the square indices.
    std::vector<Square> squares;
    while (bb) {
        Square square = msb(bb);
        squares.push_back(square);
        bb ^= BB_SQUARES[square];
    }
    return squares;
}

template<std::size_t T>
void scan_reversed(Bitboard bb, StaticVector<Square, T>& squares) {
    // Scans the bitboard in reverse and returns the square indices.
    squares.clear();
    while (bb) {
        Square square = msb(bb);
        squares.push_back(square);
        bb ^= BB_SQUARES[square];
    }
}

int square_distance(Square a, Square b) {
    // Gets the Chebyshev distance (i.e., the number of king steps) from square *a* to square *b*.
    return std::max(std::abs(square_file(a) - square_file(b)), std::abs(square_rank(a) - square_rank(b)));
}

int square_manhattan_distance(Square a, Square b) {
    // Gets the Manhattan/Taxicab distance (i.e., the number of orthogonal king steps) from square *a* to square *b*.
    return std::abs(square_file(a) - square_file(b)) + std::abs(square_rank(a) - square_rank(b));
}

int ceil(double x) {
    // Custom ceiling function to avoid including <cmath> in the header.
    int xi = static_cast<int>(x);
    return (x > xi) ? (xi + 1) : xi;
}

int square_knight_distance(Square a, Square b) {
    // Gets the Knight distance (i.e., the number of knight moves) from square *a* to square *b*.
    int dx = std::abs(square_file(a) - square_file(b));
    int dy = std::abs(square_rank(a) - square_rank(b));

    if (dx + dy == 1) {
        return 3;
    } else if (dx == 2 && dy == 2) {
        return 4;
    } else if (dx == 1 && dy == 1) {
        if ((BB_SQUARES[a] & BB_CORNERS) || (BB_SQUARES[b] & BB_CORNERS)) {
            return 4;
        }
    }

    int m = ceil(std::max(std::max(dx / 2.0, dy / 2.0), (dx + dy) / 3.0));
    return m + ((m + dx + dy) % 2);
}

Bitboard flip_vertical(Bitboard bb) {
    // Flips the bitboard vertically.
    bb = ((bb >> 8) & 0x00ff00ff00ff00ff) | ((bb & 0x00ff00ff00ff00ff) << 8);
    bb = ((bb >> 16) & 0x0000ffff0000ffff) | ((bb & 0x0000ffff0000ffff) << 16);
    bb = (bb >> 32) | ((bb & 0x00000000ffffffff) << 32);
    return bb;
}

Bitboard flip_horizontal(Bitboard bb) {
    // Flips the bitboard horizontally.
    bb = ((bb >> 1) & 0x5555555555555555) | ((bb & 0x5555555555555555) << 1);
    bb = ((bb >> 2) & 0x3333333333333333) | ((bb & 0x3333333333333333) << 2);
    bb = ((bb >> 4) & 0x0f0f0f0f0f0f0f0f) | ((bb & 0x0f0f0f0f0f0f0f0f) << 4);
    return bb;
}

Bitboard flip_diagonal(Bitboard bb) {
    // Flips the bitboard along the main diagonal.
    Bitboard t = (bb ^ (bb << 28)) & 0x0f0f0f0f00000000;
    bb = bb ^ (t ^ (t >> 28));
    t = (bb ^ (bb << 14)) & 0x3333000033330000;
    bb = bb ^ (t ^ (t >> 14));
    t = (bb ^ (bb << 7)) & 0x5500550055005500;
    bb = bb ^ (t ^ (t >> 7));
    return bb;
}

Bitboard flip_anti_diagonal(Bitboard bb) {
    // Flips the bitboard along the anti-diagonal.
    Bitboard t = bb ^ (bb << 36);
    bb = bb ^ ((t ^ (bb >> 36)) & 0xf0f0f0f00f0f0f0f);
    t = (bb ^ (bb << 18)) & 0xcccc0000cccc0000;
    bb = bb ^ (t ^ (t >> 18));
    t = (bb ^ (bb << 9)) & 0xaa00aa00aa00aa00;
    bb = bb ^ (t ^ (t >> 9));
    return bb;
}

Bitboard shift_down(Bitboard bb) {
    // Shifts the bitboard down by one rank.
    return bb >> 8;
}

Bitboard shift_2_down(Bitboard bb) {
    // Shifts the bitboard down by two ranks.
    return bb >> 16;
}

Bitboard shift_up(Bitboard bb) {
    // Shifts the bitboard up by one rank.
    return (bb << 8) & BB_ALL;
}

Bitboard shift_2_up(Bitboard bb) {
    // Shifts the bitboard up by two ranks.
    return (bb << 16) & BB_ALL;
}

Bitboard shift_right(Bitboard bb) {
    // Shifts the bitboard to the right by one file.
    return (bb << 1) & ~BB_FILE_A & BB_ALL;
}

Bitboard shift_2_right(Bitboard bb) {
    // Shifts the bitboard to the right by two files.
    return (bb << 2) & ~BB_FILE_A & ~BB_FILE_B & BB_ALL;
}

Bitboard shift_left(Bitboard bb) {
    // Shifts the bitboard to the left by one file.
    return (bb >> 1) & ~BB_FILE_H;
}

Bitboard shift_2_left(Bitboard bb) {
    // Shifts the bitboard to the left by two files.
    return (bb >> 2) & ~BB_FILE_H & ~BB_FILE_G;
}

Bitboard shift_up_right(Bitboard bb) {
    // Shifts the bitboard up and to the right by one rank and file.
    return (bb << 9) & ~BB_FILE_A & BB_ALL;
}

Bitboard shift_up_left(Bitboard bb) {
    // Shifts the bitboard up and to the left by one rank and file.
    return (bb << 7) & ~BB_FILE_H & BB_ALL;
}

Bitboard shift_down_right(Bitboard bb) {
    // Shifts the bitboard down and to the right by one rank and file.
    return (bb >> 7) & ~BB_FILE_A;
}

Bitboard shift_down_left(Bitboard bb) {
    // Shifts the bitboard down and to the left by one rank and file.
    return (bb >> 9) & ~BB_FILE_H;
}

Bitboard _sliding_attacks(Square square, const Bitboard& occupied, const std::vector<int>& deltas) {
    Bitboard attacks = BB_EMPTY;
    
    for (int delta : deltas) {
        Square target = square;
        
        while (true) {
            target += delta;
            if ( !(0 <= target && target < 64) || square_distance(target, target - delta) > 2)
            break;
            
            attacks |= BB_SQUARES[target];
            
            if (occupied & BB_SQUARES[target])
            break;
        }
    }
    return attacks;
}

Bitboard _step_attacks(Square square, const std::vector<int>& deltas) {
    return _sliding_attacks(square, BB_ALL, deltas);
}

std::array<Bitboard, 64> _bb_knight_attacks_init() {
    std::array<Bitboard, 64> bb_knight_attacks;
    for (int i = 0; i < 64; ++i) {
        bb_knight_attacks[i] = _step_attacks(SQUARES[i], {6, 10, 15, 17, -6, -10, -15, -17});
    }
    return bb_knight_attacks;
}

const std::array<Bitboard, 64> BB_KNIGHT_ATTACKS = _bb_knight_attacks_init();

std::array<Bitboard, 64> _bb_king_attacks_init() {
    std::array<Bitboard, 64> bb_king_attacks;
    for (int i = 0; i < 64; ++i) {
        bb_king_attacks[i] = _step_attacks(SQUARES[i], {1, 7, 8, 9, -1, -7, -8, -9});
    }
    return bb_king_attacks;
}

const std::array<Bitboard, 64> BB_KING_ATTACKS = _bb_king_attacks_init();

Array2D<Bitboard, 2, 64> _bb_pawn_attacks_init() {
    Array2D<Bitboard, 2, 64> bb_pawn_attacks;
    const std::vector<std::vector<int>> deltas = {{-7, -9}, {7, 9}};
    for (int i = 0; i < 64; ++i) {
        bb_pawn_attacks.insert(0, i, _step_attacks(SQUARES[i], deltas[0]));
        bb_pawn_attacks.insert(1, i, _step_attacks(SQUARES[i], deltas[1]));
    }
    return bb_pawn_attacks;
}

const Array2D<Bitboard, 2, 64> BB_PAWN_ATTACKS = _bb_pawn_attacks_init();

Bitboard _edges(Square square) {
    return (((BB_RANK_1 | BB_RANK_8) & ~BB_RANKS[square_rank(square)]) |
            ((BB_FILE_A | BB_FILE_H) & ~BB_FILES[square_file(square)]));
}

void _carry_rippler(const Bitboard& mask, std::vector<Bitboard>& ripples) {
    // Carry-Rippler trick to iterate subsets of mask.
    Bitboard subset = BB_EMPTY;
    while (true) {
        ripples.push_back(subset);
        subset = (subset - mask) & mask;
        if (subset == BB_EMPTY)
            break;
    }
}

const AttackTable _attack_table(const std::vector<int>& deltas) {
    AttackTable table{};

    for (Square square : SQUARES) {
        Bitboard mask = _sliding_attacks(square, BB_EMPTY, deltas) & ~_edges(square);
        table.masks[square] = mask;

        uint8_t bit_count = 0;
        Bitboard temp = mask;
        while (temp) {
            table.bit_positions[square][bit_count++] = static_cast<uint8_t>(lsb(temp));
            temp &= temp - 1;
        }
        table.relevant_bits[square] = bit_count;

        std::vector<Bitboard> subsets;
        _carry_rippler(mask, subsets);
        for (Bitboard subset : subsets) {
            const uint16_t idx = table.index(square, subset);
            table.attacks[square][idx] = _sliding_attacks(square, subset, deltas);
        }
    }

    return table;
}

const AttackTable BB_DIAG_TABLE = _attack_table({7, 9, -7, -9});
const AttackTable BB_FILE_TABLE = _attack_table({8, -8});
const AttackTable BB_RANK_TABLE = _attack_table({1, -1});

const Array2D<Bitboard, 64, 64> _rays() {
    Array2D<Bitboard, 64, 64> rays;
    for (int a = 0; a < 64; a++) {
        Bitboard bb_a = BB_SQUARES[a];
        std::array<Bitboard, 64> rays_row;
        for (int b = 0; b < 64; b++) {
            Bitboard bb_b = BB_SQUARES[b];
            if ((BB_DIAG_TABLE.get(a, BB_EMPTY)) & bb_b)
                rays_row[b] = ((BB_DIAG_TABLE.get(a, BB_EMPTY) & BB_DIAG_TABLE.get(b, BB_EMPTY)) | bb_a | bb_b);
            else if (BB_RANK_TABLE.get(a, BB_EMPTY) & bb_b)
                rays_row[b] = (BB_RANK_TABLE.get(a, BB_EMPTY) | bb_a);
            else if (BB_FILE_TABLE.get(a, BB_EMPTY) & bb_b)
                rays_row[b] = (BB_FILE_TABLE.get(a, BB_EMPTY) | bb_a);
            else
                rays_row[b] = BB_EMPTY;
        }
        rays.insert(a, rays_row);
    }
    return rays;
}

const Array2D<Bitboard, 64, 64> BB_RAYS = _rays();

Bitboard ray(Square a, Square b) {
    return BB_RAYS.at(a, b);
}

Bitboard between(Square a, Square b) {
    Bitboard bb = BB_RAYS.at(a, b) & ((BB_ALL << a) ^ (BB_ALL << b));
    return bb & (bb - 1);
}

std::string Piece::symbol() const {
    // Gets the symbol ``P``, ``N``, ``B``, ``R``, ``Q`` or ``K`` for white
    // pieces or the lower case symbol for black pieces.
    std::string symbol = piece_symbol(piece_type);
    if (color) {
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    }
    return symbol;
}

std::string Piece::unicode_symbol(const bool& invert_colors) const {
    // Gets the unicode symbol for the piece.
    std::string symbol = this->symbol();
    if (invert_colors && color == BLACK) {
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::tolower);
    } else if (invert_colors && color) {
        std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    }
    auto it = UNICODE_PIECE_SYMBOLS.find(symbol);
    if (it != UNICODE_PIECE_SYMBOLS.end()) {
        return it->second;
    }
    throw std::invalid_argument("Symbol not found in UNICODE_PIECE_SYMBOLS");
}

bool Piece::operator==(const Piece& other) const {
    return piece_type == other.piece_type && color == other.color;
}

bool Piece::operator!=(const Piece& other) const {
    return !(*this == other);
}

std::ostream& Piece::operator<<(std::ostream& os) const {
    os << symbol();
    return os;
}

Piece::operator std::string() const {
    return symbol();
}

Piece::operator bool() const {
    return piece_type != NULL_PIECE;
}

std::string Piece::to_string() const {
    std::ostringstream out;
    out << symbol();
    return out.str();
}

Piece Piece::from_symbol(const std::string& symbol) {
    // Creates a class:`Piece` instance from a symbol.
    // raise ValueError if the symbol is invalid.
    if (symbol.length() != 1) {
        throw std::invalid_argument("Invalid piece symbol");
    }
    std::string symbol_lower = symbol;
    std::transform(symbol_lower.begin(), symbol_lower.end(), symbol_lower.begin(), ::tolower);
    PieceType piece_type = PIECE_SYMBOLS.find_first_of(symbol_lower);
    return Piece(piece_type, symbol == symbol_lower ? BLACK : WHITE);
}

Piece Piece::null() {
    // Gets a null piece.
    return Piece(NULL_PIECE, WHITE);
}

bool Move::operator==(const Move& other) const {
    return from_square == other.from_square && to_square == other.to_square &&
            promotion == other.promotion;
}

bool Move::operator!=(const Move& other) const {
    return !(*this == other);
}

Move::operator std::string() const {
    return uci();
}

Move::operator bool() const {
    return from_square != 0 || to_square != 0;
}

std::string Move::uci() const {
    /*
    Gets the UCI notation of the move.
    
    For example, a move from a7 to a8 would be ``a7a8`` or ``a7a8q``
    (if the latter is a promotion to a queen).
    
    The UCI representation of a null move is ``0000``.
    */
    if (promotion != NULL_PIECE) {
        return SQUARE_NAMES[from_square] + SQUARE_NAMES[to_square] + piece_symbol(promotion);
    } else if (static_cast<bool>(*this)) {
        return SQUARE_NAMES[from_square] + SQUARE_NAMES[to_square];
    } else {
        return "0000";
    }
}

std::string Move::xboard() const {
    return *this ? uci() : "@@@@";
}

Move Move::null() {
    /*
    Gets a null move.

    A null move just passes the turn to the other side (and possibly
    forfeits en passant capturing). Null moves evaluate to ``False`` in
    boolean contexts.
    */
    return Move(0, 0);
}

Move Move::from_uci(const std::string& uci) {
    /*
    Parses a UCI string.

    raises:
        :exc:`ValueError` if the UCI string is invalid.
    */
    
    if (uci == "0000") {
        return Move::null();
    } else if (4 <= uci.length() && uci.length() <= 5) {
        int from_square, to_square, promotion;
        try {
            from_square = find_first_of(SQUARE_NAMES, uci.substr(0, 2), 64);
            to_square = find_first_of(SQUARE_NAMES, uci.substr(2, 2), 64);
            promotion = uci.length() == 5 ? PIECE_SYMBOLS.find_first_of(uci[4]) : 0;
        } catch (std::exception& e) {
            throw InvalidMoveError("Invalid UCI: " + uci);
        }
        if (from_square == to_square)
            throw InvalidMoveError("Invalid UCI (use 0000 for null moves): " + uci);
        return Move(from_square, to_square, promotion == 0 ? NULL_PIECE : promotion);
    } else {
        throw InvalidMoveError("Expected UCI string of length 4 or 5, got: " + uci);
    }
}

SquareSet::SquareSet(const IntoSquareSet& squares) {
    if (const SquareSet* other_set = std::get_if<SquareSet>(&squares)) {
        mask = other_set->mask & BB_ALL;
        return;
    }
    if (const Bitboard* bb = std::get_if<Bitboard>(&squares)) {
        mask = *bb & BB_ALL;
        return;
    }
    if (const Square* square = std::get_if<Square>(&squares)) {
        mask = BB_SQUARES[*square] & BB_ALL;
        return;
    }
    if (const std::vector<Square>* square_vector = std::get_if<std::vector<Square>>(&squares)) {
        mask = BB_EMPTY;
        for (Square square : *square_vector) {
            add(square);
        }
        return;
    }

    // If none of the above, we assume it's an empty set.
    mask = BB_EMPTY;
}

bool SquareSet::contains__(Square square) {
    return static_cast<bool>(mask & BB_SQUARES[square]);
}

std::vector<Square>::iterator SquareSet::begin() {
    return scan_forward(mask).begin();
}

std::vector<Square>::iterator SquareSet::end() {
    return scan_forward(mask).end();
}

std::vector<Square>::reverse_iterator SquareSet::rbegin() {
    return scan_reversed(mask).rbegin();
}

std::vector<Square>::reverse_iterator SquareSet::rend() {
    return scan_reversed(mask).rend();
}

int SquareSet::length() {
    return std::bitset<64>(mask).count();
}

void SquareSet::add(Square square) {
    // Adds a square to the set.
    mask |= BB_SQUARES[square];
}

void SquareSet::discard(Square square) {
    // Discard a square from the set.
    mask &= ~BB_SQUARES[square];
}

bool SquareSet::isdisjoint(const IntoSquareSet& other) {
    // Test if the square sets are disjoint.
    return !(*this & other).mask;
}

bool SquareSet::issubset(const IntoSquareSet& other) {
    // Test if the square set is a subset of another set.
    return !static_cast<bool>(*this & ~SquareSet(other));
}

bool SquareSet::issuperset(const IntoSquareSet& other) {
    // Test if the square set is a superset of another set.
    return !static_cast<bool>(~*this & other);
}

SquareSet SquareSet::union__(const IntoSquareSet& other) {
    return *this | other;
}

SquareSet SquareSet::operator|(const IntoSquareSet& other) {
    SquareSet other_set = SquareSet(other);
    other_set.mask |= mask;
    return other_set;
}

SquareSet SquareSet::operator|= (const IntoSquareSet& other) {
    mask |= SquareSet(other).mask;
    return *this;
}

SquareSet SquareSet::intersection(const IntoSquareSet& other) {
    return *this & other;
}

SquareSet SquareSet::operator&(const IntoSquareSet& other) {
    SquareSet other_set = SquareSet(other);
    other_set.mask &= mask;
    return other_set;
}

SquareSet SquareSet::operator&= (const IntoSquareSet& other) {
    mask &= SquareSet(other).mask;
    return *this;
}

SquareSet SquareSet::difference(const IntoSquareSet& other) {
    return *this - other;
}

SquareSet SquareSet::operator-(const IntoSquareSet& other) {
    SquareSet other_set = SquareSet(other);
    other_set.mask = mask & ~other_set.mask;
    return other_set;
}

SquareSet SquareSet::operator-= (const IntoSquareSet& other) {
    mask &= ~SquareSet(other).mask;
    return *this;
}

SquareSet SquareSet::symmetric_difference(const IntoSquareSet& other) {
    return *this ^ other;
}

SquareSet SquareSet::operator^(const IntoSquareSet& other) {
    SquareSet other_set = SquareSet(other);
    other_set.mask ^= mask;
    return other_set;
}

SquareSet SquareSet::operator^= (const IntoSquareSet& other) {
    mask ^= SquareSet(other).mask;
    return *this;
}

SquareSet SquareSet::copy() {
    return SquareSet(mask);
}

void SquareSet::update(IntoSquareSet* others,...) {
    va_list args;
    va_start(args, others);
    for (IntoSquareSet* other = others; other != NULL; other = va_arg(args, IntoSquareSet*)) {
        *this |= *other;
    }
    va_end(args);
}

void SquareSet::intersection_update(IntoSquareSet* others,...) {
    va_list args;
    va_start(args, others);
    for (IntoSquareSet* other = others; other != NULL; other = va_arg(args, IntoSquareSet*)) {
        *this &= *other;
    }
    va_end(args);
}

void SquareSet::difference_update(const IntoSquareSet& other) {
    *this -= other;
}

void SquareSet::symmetric_difference_update(const IntoSquareSet& other) {
    *this ^= other;
}

void SquareSet::remove(Square square) {
    /*
    Removes a square from the set.

    :raises: :exc:`KeyError` if the given *square* was not in the set.
    */
    Bitboard mask = BB_SQUARES[square];
    if (this->mask & mask)
        this->mask ^= mask;
    else
        throw std::invalid_argument("Square not in set: " + square);
}

Square SquareSet::pop() {
    /*
    Removes and returns a square from the set.

    :raises: :exc:`KeyError` if the set is empty.
    */
    if (mask) {
        Square square = lsb(mask);
        mask &= (mask - 1);
        return square;
    } else {
        throw std::invalid_argument("pop from empty SquareSet");
    }
}

void SquareSet::clear() {
    // Removes all elements from the set.
    mask = BB_EMPTY;
}

std::vector<Bitboard> SquareSet::carry_rippler() {
    // Iterator over the subsets of this set.
    std::vector<Bitboard> ripples;
    _carry_rippler(mask, ripples);
    return ripples;
}

SquareSet SquareSet::mirror() {
    // Returns a vertically mirrored copy of this SquareSet.
    return SquareSet(flip_vertical(mask));
}

std::array<bool, 64> SquareSet::to_list() {
    // Converts the set to a list of bools.
    std::array<bool, 64> result = {false};
    for (Square square : *this) {
        result[square] = true;
    }
    return result;
}

SquareSet::operator bool() const {
    return static_cast<bool>(mask);
}

bool SquareSet::operator==(const IntoSquareSet& other) const {
    try {
        return mask == SquareSet(other).mask;
    } catch (std::exception& e) {
        return false;
    }
}

bool SquareSet::operator!=(const IntoSquareSet& other) const {
    return !(*this == other);
}

SquareSet SquareSet::operator<<(int shift) {
    return SquareSet((mask << shift) & BB_ALL);
}

SquareSet SquareSet::operator>>(int shift) {
    return SquareSet(mask >> shift);
}

SquareSet SquareSet::operator<<=(int shift) {
    mask = (mask << shift) & BB_ALL;
    return *this;
}

SquareSet SquareSet::operator>>=(int shift) {
    mask >>= shift;
    return *this;
}

SquareSet SquareSet::operator~() {
    return SquareSet(~mask & BB_ALL);
}

SquareSet::operator int() const {
    return mask;
}

SquareSet::operator Bitboard() const {
    return mask;
}

SquareSet::operator std::string() const {
    std::vector<std::string> builder;

    for (Square square : SQUARES_180) {
        Bitboard mask = BB_SQUARES[square];
        builder.emplace_back(mask & this->mask ? "1" : ".");

        if (!(mask & BB_FILE_H))
            builder.emplace_back(" ");
        else if (square != H1)
            builder.emplace_back("\n");
    }
    return accumulate(builder.begin(), builder.end(), std::string());
}

SquareSet SquareSet::ray(Square a, Square b) {
    /*
    All squares on the rank, file or diagonal with the two squares, if they
    are aligned.
    */
    return SquareSet(::ray(a, b));
}

SquareSet SquareSet::between(Square a, Square b) {
    /*
    All squares on the rank, file or diagonal between the two squares
    (bounds not included), if they are aligned.
    */
    return SquareSet(::between(a, b));
}

SquareSet SquareSet::from_square(Square square) {
    // Creates a :class:`~chess.SquareSet` from a single square.
    return SquareSet(BB_SQUARES[square]);
}

Baseboard::Baseboard(const std::optional<std::string>& board_fen) {
    occupied_co[WHITE] = BB_EMPTY;
    occupied_co[BLACK] = BB_EMPTY;

    if (!board_fen.has_value()) {
        _clear_board();
    } else if (*board_fen == STARTING_BOARD_FEN) {
        _reset_board();
    } else {
        _set_board_fen(*board_fen);
    }
}

void Baseboard::reset_board() {
    /*
    Reset the board to the standard chess starting position.
    
    :class:`~chess.Board` also resets the move stack, but not turn,
    castling rights and move counters. Use :func:`chess.Board.reset()` to
    fully restore the starting position.
    */
    _reset_board();
}

void Baseboard::clear_board() {
    /*
    Clears the board.
    
    :class:`~chess.Board` also clears the move stack.
    */
    _clear_board();
}

Bitboard Baseboard::pieces_mask(PieceType piece_type, Color color) const {
    Bitboard bb;
    if (piece_type == PAWN)
        bb = pawns;
    else if (piece_type == KNIGHT)
        bb = knights;
    else if (piece_type == BISHOP)
        bb = bishops;
    else if (piece_type == ROOK)
        bb = rooks;
    else if (piece_type == QUEEN)
        bb = queens;
    else if (piece_type == KING)
        bb = kings;
    else
        bb = BB_EMPTY;
    /* else
        throw std::invalid_argument("Expected PieceType, got: " + piece_type); */

    return bb & occupied_co[color];
}

SquareSet Baseboard::pieces(PieceType piece_type, Color color) const {
    /*
    Gets pieces of the given type and color.

    Returns a :class:`~chess.SquareSet` instance.
    */
    return SquareSet(pieces_mask(piece_type, color));
}

std::optional<Piece> Baseboard::piece_at(Square square) const {
    // Gets the :class:`~chess.Piece` on the given square.
    PieceType piece = piece_type_at(square);
    if (piece != NULL_PIECE) {
        Bitboard mask = BB_SQUARES[square];
        return Piece(piece, static_cast<bool>(occupied_co[WHITE] & mask));
    } else {
        return std::nullopt;
    }
}

PieceType Baseboard::piece_type_at(Square square) const {
    // Gets the piece type at the given square.
    Bitboard mask = BB_SQUARES[square];

    if (!(occupied & mask))
        return NULL_PIECE;
    else if (pawns & mask)
        return PAWN;
    else if (knights & mask)
        return KNIGHT;
    else if (bishops & mask)
        return BISHOP;
    else if (rooks & mask)
        return ROOK;
    else if (queens & mask)
        return QUEEN;
    else if (kings & mask)
        return KING;
    else
        return NULL_PIECE;
    /* else
        throw std::invalid_argument("Invalid piece at square: " + square); */
}

std::optional<Color> Baseboard::color_at(Square square) const {
    // Gets the color of the piece on the given square.
    Bitboard mask = BB_SQUARES[square];
    if (occupied_co[WHITE] & mask)
        return WHITE;
    else if (occupied_co[BLACK] & mask)
        return BLACK;
    else
        return std::nullopt;
}

Square Baseboard::king(Color color) const {
    /*
    Finds the king square of the given side. Returns ``None`` if there
    is no king of that color.
    */
    Bitboard king_mask = this->occupied_co[color] & this->kings;
    return king_mask ? msb(king_mask) : NULL_SQUARE;
}

Bitboard Baseboard::attacks_mask(Square square) const {
    Bitboard bb_square = BB_SQUARES[square];

    if (bb_square & pawns) {
        return BB_PAWN_ATTACKS.at(static_cast<bool>(occupied_co[WHITE] & bb_square), square);    // color and square
    } else if (bb_square & knights) {
        return BB_KNIGHT_ATTACKS[square];
    } else if (bb_square & kings) {
        return BB_KING_ATTACKS[square];
    } else {
        Bitboard attacks = BB_EMPTY;
        if (bb_square & bishops || bb_square & queens)
            attacks = BB_DIAG_TABLE.get(square, occupied);
        if (bb_square & rooks || bb_square & queens)
            attacks |= (BB_RANK_TABLE.get(square, occupied) |
                        BB_FILE_TABLE.get(square, occupied));
        return attacks;
    }
}

SquareSet Baseboard::attacks(Square square) const {
    /*
    Gets the set of attacked squares from the given square.

    There will be no attacks if the square is empty. Pinned pieces are
    still attacking other squares.

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    return SquareSet(attacks_mask(square));
}

Bitboard Baseboard::attackers_mask(Color color, Square square, const std::optional<Bitboard>& occupied_) const {
    Bitboard occupied = occupied_.has_value() ? *occupied_ : this->occupied;

    Bitboard queens_and_rooks = queens | rooks;
    Bitboard queens_and_bishops = queens | bishops;

    Bitboard attackers = ((BB_KING_ATTACKS[square] & kings) |
                            (BB_KNIGHT_ATTACKS[square] & knights) |
                            (BB_RANK_TABLE.get(square, occupied) & queens_and_rooks) |
                            (BB_FILE_TABLE.get(square, occupied) & queens_and_rooks) |
                            (BB_DIAG_TABLE.get(square, occupied) & queens_and_bishops) |
                            (BB_PAWN_ATTACKS.at(!color, square) & pawns));

    return attackers & occupied_co[color];
}

bool Baseboard::is_attacked_by(Color color, Square square, const std::optional<IntoSquareSet>& occupied_) const {
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
    return static_cast<bool>(attackers_mask(color, square, occupied_.has_value() ? std::optional<Bitboard>(SquareSet(*occupied_).mask) : std::nullopt));
}

SquareSet Baseboard::attackers(Color color, Square square, const std::optional<IntoSquareSet>& occupied_) const {
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
    return SquareSet(attackers_mask(color, square, occupied_.has_value() ? std::optional<Bitboard>(SquareSet(*occupied_).mask) : std::nullopt));
}

Bitboard Baseboard::pin_mask(Color color, Square square) const {
    Square king = this->king(color);
    if (king == NULL_SQUARE)
        return BB_ALL;

    Bitboard square_mask = BB_SQUARES[square];

    const std::array<std::pair<const AttackTable*, Bitboard>, 3> attack_sliders = {
        std::make_pair(&BB_FILE_TABLE, rooks | queens),
        std::make_pair(&BB_RANK_TABLE, rooks | queens),
        std::make_pair(&BB_DIAG_TABLE, bishops | queens)
    };

    for (const auto& [table, sliders] : attack_sliders) {
        Bitboard rays = table->get(king, BB_EMPTY);
        if (rays & square_mask) {
            Bitboard snipers = rays & sliders & occupied_co[!color];
            StaticVector<Square, 8> snipers_squares;
            scan_forward(snipers, snipers_squares);
            for (Square sniper : snipers_squares) {
                if ((between(king, sniper) & (occupied | square_mask)) == square_mask)
                    return ray(king, sniper);
            }
        }
    }

    return BB_ALL;
}

SquareSet Baseboard::pin(Color color, Square square) const {
    /*
    Detects an absolute pin (and its direction) of the given square to
    the king of the given color.

    Returns a :class:`set of squares <chess.SquareSet>` that mask the rank,
    file or diagonal of the pin. If there is no pin, then a mask of the
    entire board is returned.
    */
    return SquareSet(pin_mask(color, square));
}

bool Baseboard::is_pinned(Color color, Square square) const {
    // Detects if the given square is pinned to the king of the given color.
    return pin_mask(color, square) != BB_ALL;
}

std::optional<Piece> Baseboard::remove_piece_at(Square square) {
    /*
    Removes the piece from the given square. Returns the
    :class:`~chess.Piece` or ``None`` if the square was already empty.

    :class:`~chess.Board` also clears the move stack.
    */
    Color color = static_cast<bool>(occupied_co[WHITE] & BB_SQUARES[square]);
    PieceType piece_type = _remove_piece_at(square);
    if (piece_type != NULL_PIECE)
        return Piece(piece_type, color);
    else
        return std::nullopt;
}

void Baseboard::set_piece_at(Square square, const std::optional<Piece>& piece) {
    /*
    Sets a piece at the given square.

    An existing piece is replaced. Setting *piece* to ``None`` is
    equivalent to :func:`~chess.Board.remove_piece_at()`.

    :class:`~chess.Board` also clears the move stack.
    */
    if (piece.has_value())
        _set_piece_at(square, piece->piece_type, piece->color);
    else
        _remove_piece_at(square);
}

std::string Baseboard::board_fen() const {
    /*
    Gets the board FEN (e.g.,
    ``rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR``).
    */
    std::vector<std::string> builder;
    int empty = 0;

    for (Square square : SQUARES) {
        std::optional<Piece> piece = piece_at(square);
        
        if (piece.has_value()) {
            if (empty > 0) {
                builder.emplace_back(std::to_string(empty));
                empty = 0;
            }
            builder.emplace_back(piece->symbol());
        } else {
            empty++;
        }

        if (BB_SQUARES[square] & BB_FILE_H) {
            if (empty > 0) {
                builder.emplace_back(std::to_string(empty));
                empty = 0;
            }
            if (square != H1)
                builder.emplace_back("/");
        }
    }

    return accumulate(builder.begin(), builder.end(), std::string());
}

void Baseboard::set_board_fen(const std::string& fen) {
    /*
    Parses *fen* and sets up the board, where *fen* is the board part of
    a FEN.

    :class:`~chess.Board` also clears the move stack.

    :raises: :exc:`ValueError` if syntactically invalid.
    */
    _set_board_fen(fen);
}

std::unordered_map<Square, Piece> Baseboard::piece_map(const Bitboard& mask) const {
    // Gets a dictionary of :class:`pieces <chess.Piece>` by square index.
    std::unordered_map<Square, Piece> result;
    StaticVector<Square, 32> squares;
    scan_forward(occupied & mask, squares);
    for (Square square : squares) {
        result[square] = *piece_at(square);
    }
    return result;
}

void Baseboard::set_piece_map(const std::unordered_map<Square, Piece>& pieces) {
    /*
    Sets up the board from a dictionary of :class:`pieces <chess.Piece>`
    by square index.

    :class:`~chess.Board` also clears the move stack.
    */
    _set_piece_map(pieces);
}

Baseboard::operator std::string() {
    std::vector<std::string> builder;

    for (Square square : SQUARES_180) {
        std::optional<Piece> piece = piece_at(square);

        if (piece.has_value()) {
            // color blue if (piece->color == WHITE) else red
            if (piece->color == WHITE)
                builder.emplace_back("\033[34m");
            else
                builder.emplace_back("\033[31m");
            builder.emplace_back(piece->symbol());
            builder.emplace_back("\033[0m");
        } else {
            builder.emplace_back(".");
        }

        if (BB_SQUARES[square] & BB_FILE_H) {
            if (square != H1)
                builder.emplace_back("\n");
        } else {
            builder.emplace_back(" ");
        }
    }
    return accumulate(builder.begin(), builder.end(), std::string());
}

std::string Baseboard::unicode(const bool& invert_color, const bool& borders, const std::string& empty_square, Color orientation) const {
    /*
    Returns a string representation of the board with Unicode pieces.
    Useful for pretty-printing to a terminal.

    :param invert_color: Invert color of the Unicode pieces.
    :param borders: Show borders and a coordinate margin.
    */
    std::vector<std::string> builder;
    if (orientation) {
        for (int rank_index = 0; rank_index < 8; rank_index++) {
            if (borders) {
                builder.emplace_back("  ");
                builder.emplace_back(std::string(25, '-'));
                builder.emplace_back("\n");

                builder.emplace_back(RANK_NAMES[rank_index]);
            }

            int i = 0;
            for (int file_index = 7; file_index >= 0; file_index--) {
                Square square_index = square(file_index, rank_index);

                if (borders)
                    builder.emplace_back(" |");
                else if (i > 0)
                    builder.emplace_back(" ");

                std::optional<Piece> piece = piece_at(square_index);

                if (piece.has_value())
                    builder.emplace_back(piece->unicode_symbol(invert_color));
                else
                    builder.emplace_back(empty_square);
                ++i;
            }
            if (borders)
                builder.emplace_back(" |");

            if (borders || rank_index < 7)
                builder.emplace_back("\n");
        }
        if (borders) {
            builder.emplace_back("  ");
            builder.emplace_back(std::string(25, '-'));
            builder.emplace_back("\n");
            std::string letters = "a  b  c  d  e  f  g  h";
            builder.emplace_back("   " + letters);
        }
    } else {
        for (int rank_index = 7; rank_index >= 0; rank_index--) {
            if (borders) {
                builder.emplace_back("  ");
                builder.emplace_back(std::string(25, '-'));
                builder.emplace_back("\n");

                builder.emplace_back(RANK_NAMES[rank_index]);
            }

            int i = 0;
            for (int file_index = 0; file_index < 8; file_index++) {
                Square square_index = square(file_index, rank_index);

                if (borders)
                    builder.emplace_back(" |");
                else if (i > 0)
                    builder.emplace_back(" ");

                std::optional<Piece> piece = piece_at(square_index);

                if (piece.has_value())
                    builder.emplace_back(piece->unicode_symbol(invert_color));
                else
                    builder.emplace_back(empty_square);
                ++i;
            }
            if (borders)
                builder.emplace_back("|");

            if (borders || rank_index > 0)
                builder.emplace_back("\n");
        }
        if (borders) {
            builder.emplace_back("  ");
            builder.emplace_back(std::string(25, '-'));
            builder.emplace_back("\n");
            std::string letters = "h  g  f  e  d  c  b  a";
            builder.emplace_back("   " + letters);
        }
    }
    return accumulate(builder.begin(), builder.end(), std::string());
}

bool Baseboard::operator==(const Baseboard& other) const {
    return occupied == other.occupied &&
           occupied_co[WHITE] == other.occupied_co[WHITE] &&
           pawns == other.pawns &&
           knights == other.knights &&
           bishops == other.bishops &&
           rooks == other.rooks &&
           queens == other.queens &&
           kings == other.kings;
}

void Baseboard::apply_transform(const std::function<Bitboard(Bitboard)>& f) {
    pawns = f(pawns);
    knights = f(knights);
    bishops = f(bishops);
    rooks = f(rooks);
    queens = f(queens);
    kings = f(kings);

    occupied_co[WHITE] = f(occupied_co[WHITE]);
    occupied_co[BLACK] = f(occupied_co[BLACK]);
    occupied = f(occupied);
}

Baseboard Baseboard::transform(const std::function<Bitboard(Bitboard)>& f) {
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
    Baseboard board = copy();
    board.apply_transform(f);
    return board;
}

void Baseboard::apply_mirror() {
    apply_transform(flip_vertical);
    Bitboard temp = occupied_co[WHITE];
    occupied_co[WHITE] = occupied_co[BLACK];
    occupied_co[BLACK] = temp;
}

Baseboard Baseboard::mirror() const {
    /*
    Returns a mirrored copy of the board (without move stack).

    The board is mirrored vertically and piece colors are swapped, so that
    the position is equivalent modulo color.

    Alternatively, :func:`~chess.BaseBoard.apply_mirror()` can be used
    to mirror the board.
    */
    Baseboard board = copy();
    board.apply_mirror();
    return board;
}

Baseboard Baseboard::copy() const {
    // Creates a copy of the board.
    Baseboard board = Baseboard(std::nullopt);

    board.pawns = this->pawns;
    board.knights = this->knights;
    board.bishops = this->bishops;
    board.rooks = this->rooks;
    board.queens = this->queens;
    board.kings = this->kings;

    board.occupied = this->occupied;
    board.occupied_co[WHITE] = this->occupied_co[WHITE];
    board.occupied_co[BLACK] = this->occupied_co[BLACK];

    return board;
}

Baseboard Baseboard::empty() {
    /*
    Creates a new empty board. Also see
    :func:`~chess.BaseBoard.clear_board()`.
    */
    return Baseboard(std::nullopt);
}

void Baseboard::_reset_board() {
    this->pawns = BB_RANK_2 | BB_RANK_7;
    this->knights = BB_B1 | BB_G1 | BB_B8 | BB_G8;
    this->bishops = BB_C1 | BB_F1 | BB_C8 | BB_F8;
    this->rooks = BB_CORNERS;
    this->queens = BB_D1 | BB_D8;
    this->kings = BB_E1 | BB_E8;
    this->occupied = BB_RANK_1 | BB_RANK_2 | BB_RANK_7 | BB_RANK_8;
    this->occupied_co[WHITE] = BB_RANK_1 | BB_RANK_2;
    this->occupied_co[BLACK] = BB_RANK_7 | BB_RANK_8;
    
    int knights_material_mg = 2 * mg_values[KNIGHT] + mg_pst[KNIGHT][B8] + mg_pst[KNIGHT][G8];
    int bishops_material_mg = 2 * mg_values[BISHOP] + mg_pst[BISHOP][C8] + mg_pst[BISHOP][F8];
    int rooks_material_mg = 2 * mg_values[ROOK] + mg_pst[ROOK][A8] + mg_pst[ROOK][H8];
    int queens_material_mg = mg_values[QUEEN] + mg_pst[QUEEN][D8];
    this->material_mg[WHITE] = knights_material_mg + bishops_material_mg + rooks_material_mg + queens_material_mg + mg_pst[KING][E8];
    this->material_mg[BLACK] = knights_material_mg + bishops_material_mg + rooks_material_mg + queens_material_mg + mg_pst[KING][E8];

    this->pawns_mg[WHITE] = 8 * mg_values[PAWN] + mg_pst[PAWN][A7] + mg_pst[PAWN][B7] + mg_pst[PAWN][C7] + mg_pst[PAWN][D7] + mg_pst[PAWN][E7] + mg_pst[PAWN][F7] + mg_pst[PAWN][G7] + mg_pst[PAWN][H7];
    this->pawns_mg[BLACK] = 8 * mg_values[PAWN] + mg_pst[PAWN][A7] + mg_pst[PAWN][B7] + mg_pst[PAWN][C7] + mg_pst[PAWN][D7] + mg_pst[PAWN][E7] + mg_pst[PAWN][F7] + mg_pst[PAWN][G7] + mg_pst[PAWN][H7];
    
    int knights_material_eg = 2 * eg_values[KNIGHT] + eg_pst[KNIGHT][B8] + eg_pst[KNIGHT][G8];
    int bishops_material_eg = 2 * eg_values[BISHOP] + eg_pst[BISHOP][C8] + eg_pst[BISHOP][F8];
    int rooks_material_eg = 2 * eg_values[ROOK] + eg_pst[ROOK][A8] + eg_pst[ROOK][H8];
    int queens_material_eg = eg_values[QUEEN] + eg_pst[QUEEN][D8];
    this->material_eg[WHITE] = knights_material_eg + bishops_material_eg + rooks_material_eg + queens_material_eg + eg_pst[KING][E8];
    this->material_eg[BLACK] = knights_material_eg + bishops_material_eg + rooks_material_eg + queens_material_eg + eg_pst[KING][E8];

    this->pawns_eg[WHITE] = 8 * eg_values[PAWN] + eg_pst[PAWN][A7] + eg_pst[PAWN][B7] + eg_pst[PAWN][C7] + eg_pst[PAWN][D7] + eg_pst[PAWN][E7] + eg_pst[PAWN][F7] + eg_pst[PAWN][G7] + eg_pst[PAWN][H7];
    this->pawns_eg[BLACK] = 8 * eg_values[PAWN] + eg_pst[PAWN][A7] + eg_pst[PAWN][B7] + eg_pst[PAWN][C7] + eg_pst[PAWN][D7] + eg_pst[PAWN][E7] + eg_pst[PAWN][F7] + eg_pst[PAWN][G7] + eg_pst[PAWN][H7];

    this->game_phase = 24;
}

void Baseboard::_clear_board() {
    this->pawns = BB_EMPTY;
    this->knights = BB_EMPTY;
    this->bishops = BB_EMPTY;
    this->rooks = BB_EMPTY;
    this->queens = BB_EMPTY;
    this->kings = BB_EMPTY;
    this->occupied = BB_EMPTY;
    this->occupied_co[WHITE] = BB_EMPTY;
    this->occupied_co[BLACK] = BB_EMPTY;
    this->material_mg[WHITE] = 0;
    this->material_mg[BLACK] = 0;
    this->material_eg[WHITE] = 0;
    this->material_eg[BLACK] = 0;
    this->pawns_mg[WHITE] = 0;
    this->pawns_mg[BLACK] = 0;
    this->pawns_eg[WHITE] = 0;
    this->pawns_eg[BLACK] = 0;
    this->game_phase = 0;
}

PieceType Baseboard::_remove_piece_at(Square square) {
    PieceType piece_type = piece_type_at(square);
    
    if (piece_type == NULL_PIECE)
        return NULL_PIECE;

    Bitboard mask = BB_SQUARES[square];
    Color color = static_cast<bool>(occupied_co[WHITE] & mask);
    Square pst_square = color ? square_mirror(square) : square;

    if (piece_type == PAWN) {
        this->pawns ^= mask;
        this->pawns_mg[color] -= mg_values[PAWN] + mg_pst[PAWN][pst_square];
        this->pawns_eg[color] -= eg_values[PAWN] + eg_pst[PAWN][pst_square];
    } else if (piece_type == KNIGHT) {
        this->knights ^= mask;
        this->material_mg[color] -= mg_values[KNIGHT] + mg_pst[KNIGHT][pst_square];
        this->material_eg[color] -= eg_values[KNIGHT] + eg_pst[KNIGHT][pst_square];
        this->game_phase -= gamephaseInc[KNIGHT];
    } else if (piece_type == BISHOP) {
        this->bishops ^= mask;
        this->material_mg[color] -= mg_values[BISHOP] + mg_pst[BISHOP][pst_square];
        this->material_eg[color] -= eg_values[BISHOP] + eg_pst[BISHOP][pst_square];
        this->game_phase -= gamephaseInc[BISHOP];
    } else if (piece_type == ROOK) {
        this->rooks ^= mask;
        this->material_mg[color] -= mg_values[ROOK] + mg_pst[ROOK][pst_square];
        this->material_eg[color] -= eg_values[ROOK] + eg_pst[ROOK][pst_square];
        this->game_phase -= gamephaseInc[ROOK];
    } else if (piece_type == QUEEN) {
        this->queens ^= mask;
        this->material_mg[color] -= mg_values[QUEEN] + mg_pst[QUEEN][pst_square];
        this->material_eg[color] -= eg_values[QUEEN] + eg_pst[QUEEN][pst_square];
        this->game_phase -= gamephaseInc[QUEEN];
    } else if (piece_type == KING) {
        this->kings ^= mask;
        this->material_mg[color] -= mg_pst[KING][pst_square];
        this->material_eg[color] -= eg_pst[KING][pst_square];
    } else
        return NULL_PIECE;

    this->occupied ^= mask;
    this->occupied_co[WHITE] &= ~mask;
    this->occupied_co[BLACK] &= ~mask;

    return piece_type;
}

void Baseboard::_set_piece_at(Square square, PieceType piece_type, Color color) {
    _remove_piece_at(square);

    Bitboard mask = BB_SQUARES[square];

    Square pst_square = color ? square_mirror(square) : square;

    if (piece_type == PAWN) {
        this->pawns |= mask;
        this->pawns_mg[color] += mg_values[PAWN] + mg_pst[PAWN][pst_square];
        this->pawns_eg[color] += eg_values[PAWN] + eg_pst[PAWN][pst_square];
    } else if (piece_type == KNIGHT) {
        this->knights |= mask;
        this->material_mg[color] += mg_values[KNIGHT] + mg_pst[KNIGHT][pst_square];
        this->material_eg[color] += eg_values[KNIGHT] + eg_pst[KNIGHT][pst_square];
        this->game_phase += gamephaseInc[KNIGHT];
    } else if (piece_type == BISHOP) {
        this->bishops |= mask;
        this->material_mg[color] += mg_values[BISHOP] + mg_pst[BISHOP][pst_square];
        this->material_eg[color] += eg_values[BISHOP] + eg_pst[BISHOP][pst_square];
        this->game_phase += gamephaseInc[BISHOP];
    } else if (piece_type == ROOK) {
        this->rooks |= mask;
        this->material_mg[color] += mg_values[ROOK] + mg_pst[ROOK][pst_square];
        this->material_eg[color] += eg_values[ROOK] + eg_pst[ROOK][pst_square];
        this->game_phase += gamephaseInc[ROOK];
    } else if (piece_type == QUEEN) {
        this->queens |= mask;
        this->material_mg[color] += mg_values[QUEEN] + mg_pst[QUEEN][pst_square];
        this->material_eg[color] += eg_values[QUEEN] + eg_pst[QUEEN][pst_square];
        this->game_phase += gamephaseInc[QUEEN];
    } else if (piece_type == KING) {
        this->kings |= mask;
        this->material_mg[color] += mg_pst[KING][pst_square];
        this->material_eg[color] += eg_pst[KING][pst_square];
    } else
        return;

    this->occupied ^= mask;
    this->occupied_co[color] ^= mask;
}

void Baseboard::_set_board_fen(std::string fen) {
    // Compatibility with set_fen().
    fen = std::regex_replace(fen, std::regex(" "), "");
    if (std::find(fen.begin(), fen.end(), ' ') != fen.end())
        throw std::invalid_argument("expected position part of fen, got multiple parts: " + fen);

    // Ensure the FEN is valid.
    std::vector<std::string> rows;
    split(rows, fen, '/');

    if (rows.size() != 8)
        throw std::invalid_argument("expected 8 rows in position part of fen: " + fen);

    // Validate each row.
    for (std::string row : rows) {
        int field_sum = 0;
        bool previous_was_digit = false;
        bool previous_was_piece = false;

        for (char c : row) {
            if (c >= '1' && c <= '8') {
                if (previous_was_digit)
                    throw std::invalid_argument("two subsequent digits in position part of fen: " + fen);
                field_sum += c - '0';
                previous_was_digit = true;
                previous_was_piece = false;
            } else if (c == '~') {
                if (!previous_was_piece)
                    throw std::invalid_argument("'~' not after piece in position part of fen: " + fen);
                previous_was_digit = false;
                previous_was_piece = false;
            } else if (PIECE_SYMBOLS.find_first_of(std::tolower(c)) != std::string::npos) {
                field_sum++;
                previous_was_digit = false;
                previous_was_piece = true;
            } else {
                throw std::invalid_argument("invalid character '" + std::string(c, 1) + "' in position part of fen: " + fen);
            }
        }

        if (field_sum != 8)
            throw std::invalid_argument("expected 8 columns in position part of fen: " + fen);
    }

    // Clear the board.
    this->_clear_board();

    // Put pieces on the board.
    int square_index = 0;
    for (char c : fen) {
        if (c >= '1' && c <= '8')
            square_index += c - '0';
        else if (PIECE_SYMBOLS.find_first_of(std::tolower(c)) != std::string::npos) {
            Piece piece = Piece::from_symbol(std::string(1, c));
            this->_set_piece_at(SQUARES_180[square_index], piece.piece_type, piece.color);
            square_index++;
        }
    }
}

void Baseboard::_set_piece_map(const std::unordered_map<Square, Piece>& pieces) {
    this->_clear_board();
    for (const auto& [square, piece] : pieces) {
        this->_set_piece_at(square, piece.piece_type, piece.color);
    }
}

_BoardState::_BoardState(const Board& board) {
    this->pawns = board.pawns;
    this->knights = board.knights;
    this->bishops = board.bishops;
    this->rooks = board.rooks;
    this->queens = board.queens;
    this->kings = board.kings;
    
    this->occupied_w = board.occupied_co[WHITE];
    this->occupied_b = board.occupied_co[BLACK];
    this->occupied = board.occupied;

    this->material_w_mg = board.material_mg[WHITE];
    this->material_b_mg = board.material_mg[BLACK];
    this->material_w_eg = board.material_eg[WHITE];
    this->material_b_eg = board.material_eg[BLACK];
    this->pawns_w_mg = board.pawns_mg[WHITE];
    this->pawns_b_mg = board.pawns_mg[BLACK];
    this->pawns_w_eg = board.pawns_eg[WHITE];
    this->pawns_b_eg = board.pawns_eg[BLACK];
    this->game_phase = board.game_phase;

    this->turn = board.turn;
    this->castling_rights = board.castling_rights;
    this->ep_square = board.ep_square;
    this->halfmove_clock = board.halfmove_clock;
    this->fullmove_number = board.fullmove_number;
}

void _BoardState::restore(Board& board) {
    board.pawns = this->pawns;
    board.knights = this->knights;
    board.bishops = this->bishops;
    board.rooks = this->rooks;
    board.queens = this->queens;
    board.kings = this->kings;

    board.occupied_co[WHITE] = this->occupied_w;
    board.occupied_co[BLACK] = this->occupied_b;
    board.occupied = this->occupied;

    board.material_mg[WHITE] = this->material_w_mg;
    board.material_mg[BLACK] = this->material_b_mg;
    board.material_eg[WHITE] = this->material_w_eg;
    board.material_eg[BLACK] = this->material_b_eg;
    board.pawns_mg[WHITE] = this->pawns_w_mg;
    board.pawns_mg[BLACK] = this->pawns_b_mg;
    board.pawns_eg[WHITE] = this->pawns_w_eg;
    board.pawns_eg[BLACK] = this->pawns_b_eg;
    board.game_phase = this->game_phase;

    board.turn = this->turn;
    board.castling_rights = this->castling_rights;
    board.ep_square = this->ep_square;
    board.halfmove_clock = this->halfmove_clock;
    board.fullmove_number = this->fullmove_number;
}

Board::Board(const std::optional<std::string>& fen) : Baseboard(std::nullopt) {
    ep_square = NULL_SQUARE;
    move_stack.clear();
    _stack.clear();
    move_stack.reserve(MOVE_HISTORY_SIZE);
    _stack.reserve(MOVE_HISTORY_SIZE);

    if (!fen.has_value())
        clear();
    else if (*fen == STARTING_FEN)
        reset();
    else
        set_fen(*fen);
}

Board::Board(const Baseboard& baseboard) : Baseboard(baseboard) {
    ep_square = NULL_SQUARE;
    move_stack.clear();
    _stack.clear();
    move_stack.reserve(MOVE_HISTORY_SIZE);
    _stack.reserve(MOVE_HISTORY_SIZE);
}

void Board::reset() {
    // Restore the starting position.
    turn = WHITE;
    castling_rights = BB_CORNERS;
    ep_square = NULL_SQUARE;
    halfmove_clock = 0;
    fullmove_number = 1;

    reset_board();
}

void Board::reset_board() {
    Baseboard::reset_board();
    clear_stack();
}

void Board::clear() {
    /*
    Clears the board.

    Resets move stack and move counters. The side to move is white. There
    are no rooks or kings, so castling rights are removed.

    In order to be in a valid :func:`~chess.Board.status()`, at least kings
    need to be put on the board.
    */
    turn = WHITE;
    castling_rights = BB_EMPTY;
    ep_square = NULL_SQUARE;
    halfmove_clock = 0;
    fullmove_number = 1;

    clear_board();
}

void Board::clear_board() {
    Baseboard::clear_board();
    clear_stack();
}

void Board::clear_stack() {
    // Clears the move stack.
    move_stack.clear();
    _stack.clear();
}

Board Board::root() {
    // Returns a copy of the root position.
    if (_stack.empty())
        return copy(false);
    
    Board board = Board(std::nullopt);
    _stack[0].restore(board);
    return board;
}

int Board::ply() const {
    /*
    Returns the number of half-moves since the start of the game, as
    indicated by :data:`~chess.Board.fullmove_number` and
    :data:`~chess.Board.turn`.

    If moves have been pushed from the beginning, this is usually equal to
    ``len(board.move_stack)``. But note that a board can be set up with
    arbitrary starting positions, and the stack can be cleared.
    */
    return 2 * (fullmove_number - 1) + (!turn ? 1 : 0);
}

std::optional<Piece> Board::remove_piece_at(Square square) {
    const std::optional<Piece>& piece = Baseboard::remove_piece_at(square);
    clear_stack();
    return piece;
}

void Board::set_piece_at(Square square, const std::optional<Piece>& piece) {
    Baseboard::set_piece_at(square, piece);
    clear_stack();
}

void Board::generate_pseudo_legal_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    Bitboard our_pieces = occupied_co[color];

    // Generate piece moves.
    Bitboard non_pawns = our_pieces & ~pawns & from_mask;
    StaticVector<Square, 16> starting_squares;
    StaticVector<Square, 32> target_squares;
    scan_forward(non_pawns, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard moves = attacks_mask(from_square) & ~our_pieces & to_mask;

        scan_forward(moves, target_squares);
        for (Square to_square : target_squares) {
            return_moves.emplace_back(from_square, to_square);
        }
    }

    // Generate castling moves.
    if (from_mask & kings) {
        StaticVector<Move, CASTLING_MOVES_SIZE> castling_moves;
        this->generate_castling_moves_by_color(castling_moves, color, from_mask, to_mask);
        return_moves.append(castling_moves);
    }

    // The remaining moves are all pawn ones.
    Bitboard pawns = this->pawns & occupied_co[color] & from_mask;
    if (!pawns)
        return;

    // Generate pawn captures.
    Bitboard capturers = pawns;
    scan_forward(capturers, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard targets = BB_PAWN_ATTACKS.at(color, from_square) & occupied_co[!color] & to_mask;

        scan_forward(targets, target_squares);
        for (Square to_square : target_squares) {
            int rank = square_rank(to_square);
            if (rank == 0 || rank == 7) {
                return_moves.emplace_back(from_square, to_square, QUEEN);
                return_moves.emplace_back(from_square, to_square, ROOK);
                return_moves.emplace_back(from_square, to_square, BISHOP);
                return_moves.emplace_back(from_square, to_square, KNIGHT);
            } else {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Prepare pawn advance generation
    Bitboard single_moves, double_moves;
    if (color) {
        single_moves = pawns << 8 & ~occupied;
        double_moves = single_moves << 8 & ~occupied & (BB_RANK_3 | BB_RANK_4);
    } else {
        single_moves = pawns >> 8 & ~occupied;
        double_moves = single_moves >> 8 & ~occupied & (BB_RANK_6 | BB_RANK_5);
    }

    single_moves &= to_mask;
    double_moves &= to_mask;

    // Generate single pawn moves.
    scan_forward(single_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!color ? 8 : -8);

        int rank = square_rank(to_square);
        if (rank == 0 || rank == 7) {
            return_moves.emplace_back(from_square, to_square, QUEEN);
            return_moves.emplace_back(from_square, to_square, ROOK);
            return_moves.emplace_back(from_square, to_square, BISHOP);
            return_moves.emplace_back(from_square, to_square, KNIGHT);
        } else {
            return_moves.emplace_back(from_square, to_square);
        }
    }

    // Generate double pawn moves.
    scan_forward(double_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!color ? 16 : -16);
        return_moves.emplace_back(from_square, to_square);
    }

    // Generate en passant captures.
    if (ep_square != NULL_SQUARE) {
        StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
        generate_pseudo_legal_ep_by_color(ep_moves, color, from_mask, to_mask);
        return_moves.append(ep_moves);
    }

    return;
}

void Board::generate_pseudo_legal_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    Bitboard our_pieces = occupied_co[this->turn];

    // Generate piece moves.
    Bitboard non_pawns = our_pieces & ~pawns & from_mask;
    StaticVector<Square, 16> starting_squares;
    StaticVector<Square, 32> target_squares;
    scan_forward(non_pawns, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard moves = attacks_mask(from_square) & ~our_pieces & to_mask;

        scan_forward(moves, target_squares);
        for (Square to_square : target_squares) {
            return_moves.emplace_back(from_square, to_square);
        }
    }

    // Generate castling moves.
    if (from_mask & kings) {
        StaticVector<Move, CASTLING_MOVES_SIZE> castling_moves;
        this->generate_castling_moves(castling_moves, from_mask, to_mask);
        return_moves.append(castling_moves);
    }

    // The remaining moves are all pawn ones.
    Bitboard pawns = this->pawns & occupied_co[this->turn] & from_mask;
    if (!pawns)
        return;

    // Generate pawn captures.
    Bitboard capturers = pawns;
    scan_forward(capturers, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard targets = BB_PAWN_ATTACKS.at(this->turn, from_square) & occupied_co[!this->turn] & to_mask;

        scan_forward(targets, target_squares);
        for (Square to_square : target_squares) {
            int rank = square_rank(to_square);
            if (rank == 0 || rank == 7) {
                return_moves.emplace_back(from_square, to_square, QUEEN);
                return_moves.emplace_back(from_square, to_square, ROOK);
                return_moves.emplace_back(from_square, to_square, BISHOP);
                return_moves.emplace_back(from_square, to_square, KNIGHT);
            } else {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Prepare pawn advance generation
    Bitboard single_moves, double_moves;
    if (this->turn) {
        single_moves = pawns << 8 & ~occupied;
        double_moves = single_moves << 8 & ~occupied & (BB_RANK_3 | BB_RANK_4);
    } else {
        single_moves = pawns >> 8 & ~occupied;
        double_moves = single_moves >> 8 & ~occupied & (BB_RANK_6 | BB_RANK_5);
    }

    single_moves &= to_mask;
    double_moves &= to_mask;

    // Generate single pawn moves.
    scan_forward(single_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!this->turn ? 8 : -8);

        int rank = square_rank(to_square);
        if (rank == 0 || rank == 7) {
            return_moves.emplace_back(from_square, to_square, QUEEN);
            return_moves.emplace_back(from_square, to_square, ROOK);
            return_moves.emplace_back(from_square, to_square, BISHOP);
            return_moves.emplace_back(from_square, to_square, KNIGHT);
        } else {
            return_moves.emplace_back(from_square, to_square);
        }
    }

    // Generate double pawn moves.
    scan_forward(double_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!this->turn ? 16 : -16);
        return_moves.emplace_back(from_square, to_square);
    }

    // Generate en passant captures.
    if (ep_square != NULL_SQUARE) {
        StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
        generate_pseudo_legal_ep(ep_moves, from_mask, to_mask);
        return_moves.append(ep_moves);
    }

    return;
}

void Board::generate_pseudo_safe_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Color color, Square king, Bitboard blockers, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    Bitboard our_pieces = occupied_co[color];

    // Generate piece moves.
    Bitboard non_pawns = our_pieces & ~pawns & from_mask;
    StaticVector<Square, 16> starting_squares;
    StaticVector<Square, 32> target_squares;
    scan_forward(non_pawns, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard moves = attacks_mask(from_square) & ~our_pieces & to_mask;

        scan_forward(moves, target_squares);
        for (Square to_square : target_squares) {
            if (_is_safe_by_color(color, king, blockers, from_square, to_square)) {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Generate castling moves.
    if (from_mask & kings) {
        StaticVector<Move, CASTLING_MOVES_SIZE> castling_moves;
        this->generate_castling_moves_by_color(castling_moves, color, from_mask, to_mask);
        for (const Move& move : castling_moves) {
            if (_is_safe_by_color(color, king, blockers, move.from_square, move.to_square)) {
                return_moves.emplace_back(move);
            }
        }
    }

    // The remaining moves are all pawn ones.
    Bitboard pawns = this->pawns & occupied_co[color] & from_mask;
    if (!pawns)
        return;

    // Generate pawn captures.
    Bitboard capturers = pawns;
    scan_forward(capturers, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard targets = BB_PAWN_ATTACKS.at(color, from_square) & occupied_co[!color] & to_mask;

        scan_forward(targets, target_squares);
        for (Square to_square : target_squares) {
            int rank = square_rank(to_square);
            if (_is_safe_by_color(color, king, blockers, from_square, to_square)) {
                if (rank == 0 || rank == 7) {
                    return_moves.emplace_back(from_square, to_square, QUEEN);
                    return_moves.emplace_back(from_square, to_square, ROOK);
                    return_moves.emplace_back(from_square, to_square, BISHOP);
                    return_moves.emplace_back(from_square, to_square, KNIGHT);
                } else {
                    return_moves.emplace_back(from_square, to_square);
                }
            }
        }
    }

    // Prepare pawn advance generation
    Bitboard single_moves, double_moves;
    if (color) {
        single_moves = pawns << 8 & ~occupied;
        double_moves = single_moves << 8 & ~occupied & (BB_RANK_3 | BB_RANK_4);
    } else {
        single_moves = pawns >> 8 & ~occupied;
        double_moves = single_moves >> 8 & ~occupied & (BB_RANK_6 | BB_RANK_5);
    }

    single_moves &= to_mask;
    double_moves &= to_mask;

    // Generate single pawn moves.
    scan_forward(single_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!color ? 8 : -8);

        int rank = square_rank(to_square);
        if (_is_safe_by_color(color, king, blockers, from_square, to_square)) {
            if (rank == 0 || rank == 7) {
                return_moves.emplace_back(from_square, to_square, QUEEN);
                return_moves.emplace_back(from_square, to_square, ROOK);
                return_moves.emplace_back(from_square, to_square, BISHOP);
                return_moves.emplace_back(from_square, to_square, KNIGHT);
            } else {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Generate double pawn moves.
    scan_forward(double_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!color ? 16 : -16);
        if (_is_safe_by_color(color, king, blockers, from_square, to_square))
            return_moves.emplace_back(from_square, to_square);
    }

    // Generate en passant captures.
    if (ep_square != NULL_SQUARE) {
        StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
        generate_pseudo_legal_ep_by_color(ep_moves, color, from_mask, to_mask);
        for (const Move& move : ep_moves) {
            if (_is_safe_by_color(color, king, blockers, move.from_square, move.to_square)) {
                return_moves.emplace_back(move);
            }
        }
    }

    return;
}

void Board::generate_pseudo_safe_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& return_moves, Square king, Bitboard blockers, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    Bitboard our_pieces = occupied_co[this->turn];

    // Generate piece moves.
    Bitboard non_pawns = our_pieces & ~pawns & from_mask;
    StaticVector<Square, 16> starting_squares;
    StaticVector<Square, 32> target_squares;
    scan_forward(non_pawns, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard moves = attacks_mask(from_square) & ~our_pieces & to_mask;

        scan_forward(moves, target_squares);
        for (Square to_square : target_squares) {
            if (_is_safe(king, blockers, from_square, to_square)) {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Generate castling moves.
    if (from_mask & kings) {
        StaticVector<Move, CASTLING_MOVES_SIZE> castling_moves;
        this->generate_castling_moves(castling_moves, from_mask, to_mask);
        for (const Move& move : castling_moves) {
            if (_is_safe(king, blockers, move.from_square, move.to_square)) {
                return_moves.emplace_back(move);
            }
        }
    }

    // The remaining moves are all pawn ones.
    Bitboard pawns = this->pawns & occupied_co[this->turn] & from_mask;
    if (!pawns)
        return;

    // Generate pawn captures.
    Bitboard capturers = pawns;
    scan_forward(capturers, starting_squares);
    for (Square from_square : starting_squares) {
        Bitboard targets = BB_PAWN_ATTACKS.at(this->turn, from_square) & occupied_co[!this->turn] & to_mask;

        scan_forward(targets, target_squares);
        for (Square to_square : target_squares) {
            int rank = square_rank(to_square);
            if (_is_safe(king, blockers, from_square, to_square)) {
                if (rank == 0 || rank == 7) {
                    return_moves.emplace_back(from_square, to_square, QUEEN);
                    return_moves.emplace_back(from_square, to_square, ROOK);
                    return_moves.emplace_back(from_square, to_square, BISHOP);
                    return_moves.emplace_back(from_square, to_square, KNIGHT);
                } else {
                    return_moves.emplace_back(from_square, to_square);
                }
            }
        }
    }

    // Prepare pawn advance generation
    Bitboard single_moves, double_moves;
    if (this->turn) {
        single_moves = pawns << 8 & ~occupied;
        double_moves = single_moves << 8 & ~occupied & (BB_RANK_3 | BB_RANK_4);
    } else {
        single_moves = pawns >> 8 & ~occupied;
        double_moves = single_moves >> 8 & ~occupied & (BB_RANK_6 | BB_RANK_5);
    }

    single_moves &= to_mask;
    double_moves &= to_mask;

    // Generate single pawn moves.
    scan_forward(single_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!this->turn ? 8 : -8);

        int rank = square_rank(to_square);
        if (_is_safe(king, blockers, from_square, to_square)) {
            if (rank == 0 || rank == 7) {
                return_moves.emplace_back(from_square, to_square, QUEEN);
                return_moves.emplace_back(from_square, to_square, ROOK);
                return_moves.emplace_back(from_square, to_square, BISHOP);
                return_moves.emplace_back(from_square, to_square, KNIGHT);
            } else {
                return_moves.emplace_back(from_square, to_square);
            }
        }
    }

    // Generate double pawn moves.
    scan_forward(double_moves, target_squares);
    for (Square to_square : target_squares) {
        int from_square = to_square + (!this->turn ? 16 : -16);
        if (_is_safe(king, blockers, from_square, to_square))
            return_moves.emplace_back(from_square, to_square);
    }

    // Generate en passant captures.
    if (ep_square != NULL_SQUARE) {
        StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
        generate_pseudo_legal_ep(ep_moves, from_mask, to_mask);
        for (const Move& move : ep_moves) {
            if (_is_safe(king, blockers, move.from_square, move.to_square)) {
                return_moves.emplace_back(move);
            }
        }
    }

    return;
}

void Board::generate_pseudo_legal_ep_by_color(StaticVector<Move, EP_CAPTURE_SIZE>& return_moves, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    if (ep_square == NULL_SQUARE || !(BB_SQUARES[ep_square] & to_mask))
        return;

    Bitboard capturers = pawns & occupied_co[color] & from_mask &
                         BB_PAWN_ATTACKS.at(!color, ep_square) &
                         BB_RANKS[color ? 4 : 3];

    StaticVector<Square, 2> capturer_squares;
    scan_forward(capturers, capturer_squares);
    for (Square capturer : capturer_squares)
        return_moves.emplace_back(capturer, ep_square);

    return;
}

void Board::generate_pseudo_legal_ep(StaticVector<Move, EP_CAPTURE_SIZE>& return_moves, const Bitboard& from_mask, const Bitboard& to_mask) const {
    return_moves.clear();

    if (ep_square == NULL_SQUARE || !(BB_SQUARES[ep_square] & to_mask))
        return;

    Bitboard capturers = pawns & occupied_co[this->turn] & from_mask &
                         BB_PAWN_ATTACKS.at(!this->turn, ep_square) &
                         BB_RANKS[this->turn ? 4 : 3];

    StaticVector<Square, 2> capturer_squares;
    scan_forward(capturers, capturer_squares);
    for (Square capturer : capturer_squares)
        return_moves.emplace_back(capturer, ep_square);

    return;
}

void Board::generate_pseudo_legal_captures_by_color(StaticVector<Move, CAPTURES_SIZE>& pseudo_legal_moves, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    pseudo_legal_moves.clear();

    StaticVector<Move, LEGAL_MOVES_SIZE> pseudo_legal_moves_all;
    generate_pseudo_legal_moves_by_color(pseudo_legal_moves_all, color, from_mask, to_mask & occupied_co[!color]);
    StaticVector<Move, EP_CAPTURE_SIZE> pseudo_legal_ep;
    generate_pseudo_legal_ep_by_color(pseudo_legal_ep, color, from_mask, to_mask);

    pseudo_legal_moves.append(pseudo_legal_moves_all);
    pseudo_legal_moves.append(pseudo_legal_ep);
    return;
}

void Board::generate_pseudo_legal_captures(StaticVector<Move, CAPTURES_SIZE>& pseudo_legal_moves, const Bitboard& from_mask, const Bitboard& to_mask) const {
    pseudo_legal_moves.clear();

    StaticVector<Move, LEGAL_MOVES_SIZE> pseudo_legal_moves_all;
    generate_pseudo_legal_moves(pseudo_legal_moves_all, from_mask, to_mask & occupied_co[!this->turn]);
    StaticVector<Move, EP_CAPTURE_SIZE> pseudo_legal_ep;
    generate_pseudo_legal_ep(pseudo_legal_ep, from_mask, to_mask);

    pseudo_legal_moves.append(pseudo_legal_moves_all);
    pseudo_legal_moves.append(pseudo_legal_ep);
    return;
}

Bitboard Board::checkers_mask_by_color(Color color) const {
    Square king = this->king(color);
    return king != NULL_SQUARE ? attackers_mask(!color, king) : BB_EMPTY;
}

Bitboard Board::checkers_mask() const {
    Square king = this->king(this->turn);
    return king != NULL_SQUARE ? attackers_mask(!this->turn, king) : BB_EMPTY;
}

SquareSet Board::checkers_by_color(Color color) const {
    /*
    Gets the pieces currently giving check to the given color.

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    return SquareSet(checkers_mask_by_color(color));
}

SquareSet Board::checkers() const {
    /*
    Gets the pieces currently giving check.

    Returns a :class:`set of squares <chess.SquareSet>`.
    */
    return SquareSet(checkers_mask());
}

bool Board::is_check_by_color(Color color) const {
    // Test if the given side to move is in check.
    return static_cast<bool>(checkers_mask_by_color(color));
}

bool Board::is_check() const {
    // Test if the current side to move is in check.
    return static_cast<bool>(checkers_mask());
}

bool Board::gives_check(const Move& move) {
    // Probes if the given move would put the opponent in check. The move
    // must be at least pseudo-legal.
    push(move);
    bool check = is_check();
    pop();
    return check;
}

bool Board::is_into_check(const Move& move) const {
    Square king = this->king(this->turn);
    if (king == NULL_SQUARE)
        return false;

    // If already in check, look if it is an evasion.
    Bitboard checkers = attackers_mask(!this->turn, king);
    if (checkers) {
        StaticVector<Move, EVASION_SIZE> evasions;
        this->_generate_evasions(evasions, king, checkers, BB_SQUARES[move.from_square], BB_SQUARES[move.to_square]);
        if (std::find(evasions.begin(), evasions.end(), move) == evasions.end())
            return true;
    }
    
    return !_is_safe(king, _slider_blockers(king), move);
}

bool Board::was_into_check() const {
    Square king = this->king(!this->turn);
    return king != NULL_SQUARE && is_attacked_by(this->turn, king);
}

bool Board::is_pseudo_legal(const Move& move) const {
    // Null moves are not pseudo-legal.
    if (!static_cast<bool>(move))
        return false;

    // Source square must not be vacant.
    PieceType piece = piece_type_at(move.from_square);
    if (piece == NULL_PIECE)
        return false;
    
    // Get square masks.
    Bitboard from_mask = BB_SQUARES[move.from_square];
    Bitboard to_mask = BB_SQUARES[move.to_square];

    // Check turn.
    if (!(occupied_co[this->turn] & from_mask))
        return false;
    
    // Only pawns can promote and only on the backrank.
    if (move.promotion != NULL_PIECE) {
        if (piece != PAWN)
            return false;

        if (this->turn && square_rank(move.to_square) != 7)
            return false;
        else if (!this->turn && square_rank(move.to_square) != 0)
            return false;
    }

    // Handle castling.
    if (piece == KING) {
        Move move_adjusted_castling = this->_from_chess960(move.from_square, move.to_square);
        StaticVector<Move, CASTLING_MOVES_SIZE> castling_moves;
        this->generate_castling_moves(castling_moves);
        if (std::find(castling_moves.begin(), castling_moves.end(), move_adjusted_castling) != castling_moves.end())
            return true;
    }

    // Destination square ca not be occupied.
    if (occupied_co[this->turn] & to_mask)
        return false;
    
    // Handle pawn moves.
    if (piece == PAWN) {
        StaticVector<Move, LEGAL_MOVES_SIZE> pawn_moves;
        generate_pseudo_legal_moves(pawn_moves, from_mask, to_mask);
        return std::find(pawn_moves.begin(), pawn_moves.end(), move) != pawn_moves.end();
    }

    // Handle all other pieces.
    return static_cast<bool>(attacks_mask(move.from_square) & to_mask);
}

bool Board::is_legal(const Move& move) const {
    return is_pseudo_legal(move) && !is_into_check(move);
}

bool Board::is_game_over() {    // REMOVED parameter: bool claim_draw
    return outcome() != std::nullopt;
}

std::string Board::result() {   // REMOVED parameter: bool claim_draw
    std::optional<Outcome> outcome = this->outcome();
    return outcome.has_value() ? (*outcome).result() : "*";
}

std::optional<Outcome> Board::outcome(int repetition_count) {   // REMOVED parameter: bool claim_draw
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
    // Normal game end.
    if (is_checkmate())
        return Outcome(Termination::CHECKMATE, !this->turn);
    if (is_insufficient_material())
        return Outcome(Termination::INSUFFICIENT_MATERIAL, std::nullopt);
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    generate_legal_moves(legal_moves);
    if (std::none_of(legal_moves.begin(), legal_moves.end(), [](const Move& move) { return static_cast<bool>(move); }))
        return Outcome(Termination::STALEMATE, std::nullopt);
    
    /* // Automatic draws.
    if (is_seventyfive_moves())
        return Outcome(Termination::SEVENTYFIVE_MOVES, std::nullopt);
    if (is_fivefold_repetition())
        return Outcome(Termination::FIVEFOLD_REPETITION, std::nullopt);

    // Claimable draws.
    if (claim_draw) {
        if (can_claim_fifty_moves())
            return Outcome(Termination::FIFTY_MOVES, std::nullopt);
        if (can_claim_threefold_repetition())
            return Outcome(Termination::THREEFOLD_REPETITION, std::nullopt);
    } */

    if (is_fifty_moves())
        return Outcome(Termination::FIFTY_MOVES, std::nullopt);
    if (is_repetition(repetition_count))
        return Outcome(Termination::REPETITION, std::nullopt);

    return std::nullopt;
}

bool Board::is_checkmate() const {
    // Check if the current position is checkmate.
    if (!is_check())
        return false;

    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    generate_legal_moves(legal_moves);
    return std::none_of(legal_moves.begin(), legal_moves.end(), [](const Move& move) { return static_cast<bool>(move); });
}

bool Board::is_stalemate() const {
    // Check if the current position is stalemate.
    if (is_check())
        return false;

    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    generate_legal_moves(legal_moves);
    return std::none_of(legal_moves.begin(), legal_moves.end(), [](const Move& move) { return static_cast<bool>(move); });
}

bool Board::is_insufficient_material() const {
    // Checks if neither side has sufficient winning material
    // (:func:`~chess.Board.has_insufficient_material()`).
    return has_insufficient_material(WHITE) && has_insufficient_material(BLACK);
}

bool Board::has_insufficient_material(Color color) const {
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
    if (occupied_co[color] & (pawns | rooks | queens))
        return false;
        
    // Knights are only insufficient material if:
    // (1) We do not have any other pieces, including more than one knight.
    // (2) The opponent does not have pawns, knights, bishops or rooks.
    //     These would allow selfmate.
    if (occupied_co[color] & knights)
        return (std::bitset<64>(occupied_co[color]).count() <= 2 &&
                !(occupied_co[!color] & ~kings & ~queens));
    
    // Bishops are only insufficient material if:
    // (1) We do not have any other pieces, including bishops of the
    //     opposite color.
    // (2) The opponent does not have bishops of the opposite color,
    //     pawns or knights. These would allow selfmate.
    if (occupied_co[color] & bishops) {
        return ((!(bishops & BB_DARK_SQUARES)) || (!(bishops & BB_LIGHT_SQUARES))) && !pawns && !knights;
    }

    return true;
}

bool Board::_is_halfmoves(const int& n) const {
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    generate_legal_moves(legal_moves);
    return halfmove_clock >= n && std::any_of(legal_moves.begin(), legal_moves.end(), [](const Move& move) { return static_cast<bool>(move); });
}

bool Board::is_seventyfive_moves() const {
    /*
    Since the 1st of July 2014, a game is automatically drawn (without
    a claim by one of the players) if the half-move clock since a capture
    or pawn move is equal to or greater than 150. Other means to end a game
    take precedence.
    */
    return _is_halfmoves(150);
}

bool Board::is_fivefold_repetition() {
    /*
    Since the 1st of July 2014 a game is automatically drawn (without
    a claim by one of the players) if a position occurs for the fifth time.
    Originally this had to occur on consecutive alternating moves, but
    this has since been revised.
    */
   return is_repetition(5);
}

bool Board::can_claim_draw() {
    /*
    Checks if the player to move can claim a draw by the fifty-move rule or
    by threefold repetition.

    Note that checking the latter can be slow.
    */
    return can_claim_fifty_moves() || can_claim_threefold_repetition();
}

bool Board::is_fifty_moves() const {
    /*
    Checks that the clock of halfmoves since the last capture or pawn move
    is greater or equal to 100, and that no other means of ending the game
    (like checkmate) take precedence.
    */
   return _is_halfmoves(100);
}

bool Board::can_claim_fifty_moves() {
    /*
    Checks if the player to move can claim a draw by the fifty-move rule.

    In addition to :func:`~chess.Board.is_fifty_moves()`, the fifty-move
    rule can also be claimed if there is a legal move that achieves this
    condition.
    */
    if (is_fifty_moves())
        return true;
    
    if (halfmove_clock >= 90) {
        StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
        generate_legal_moves(legal_moves);
        for (const Move& move : legal_moves) {
            if (!is_zeroing(move)) {
                push(move);
                if (is_fifty_moves()) {
                    pop();
                    return true;
                }
                pop();
            }
        }
    }
    return false;
}

bool Board::is_threefold_repetition() {
    /*
    Checks if the current position has repeated 3 times.

    Unlike :func:`~chess.Board.can_claim_threefold_repetition()`, this does
    not consider a repetition that can be played on the next move.
    */
    return is_repetition(3);
}

struct BoardStateTupleHash {
    std::size_t operator()(const BoardStateTuple& key) const {
        return std::hash<unsigned long long>()(key.pawns) ^
               std::hash<unsigned long long>()(key.knights) ^
               std::hash<unsigned long long>()(key.bishops) ^
               std::hash<unsigned long long>()(key.rooks) ^
               std::hash<unsigned long long>()(key.queens) ^
               std::hash<unsigned long long>()(key.kings) ^
               std::hash<unsigned long long>()(key.occupied_white) ^
               std::hash<unsigned long long>()(key.occupied_black) ^
               std::hash<bool>()(key.turn) ^
               std::hash<unsigned long long>()(key.castling_rights) ^
               std::hash<int>()(key.ep_square);
    }
};

bool Board::can_claim_threefold_repetition() {
    /*
    Checks if the player to move can claim a draw by threefold repetition.
    
    Draw by threefold repetition can be claimed if the position on the
    board occurred for the third time or if such a repetition is reached
    with one of the possible legal moves.
    
    Note that checking this can be slow: In the worst case
    scenario, every legal move has to be tested and the entire game has to
    be replayed because there is no incremental transposition table.
    */
    const BoardStateTuple& transposition_key = _transposition_key();
    std::unordered_map<BoardStateTuple, int, BoardStateTupleHash> transpositions;
    transpositions[transposition_key] = 1;

    // Count positions.
    std::vector<Move> switchyard;
    switchyard.reserve(move_stack.size());
    while (!move_stack.empty()) {
        const Move& move = pop();
        switchyard.push_back(move);

        if (is_irreversible(move))
            break;

        transpositions[_transposition_key()]++;
    }

    while (!switchyard.empty()) {
        const Move& move = switchyard.back();
        switchyard.pop_back();
        push(move);
    }

    // Threefold repetition occurred.
    if (transpositions[transposition_key] >= 3)
        return true;

    // The next legal move is a threefold repetition.
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    generate_legal_moves(legal_moves);
    for (const Move& move : legal_moves) {
        push(move);
        if (transpositions[_transposition_key()] >= 2) {
            pop();
            return true;
        }
        pop();
    }

    return false;
}

bool Board::is_repetition(int count) {
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
    // Fast check, based on occupancy only.
    int maybe_repetitions = 1;
    for (std::vector<_BoardState>::reverse_iterator it = _stack.rbegin(); it != _stack.rend(); ++it) {
        const _BoardState& state = *it;
        if (state.occupied == occupied) {
            maybe_repetitions++;
            if (maybe_repetitions >= count)
                break;
        }
    }
    if (maybe_repetitions < count)
        return false;

    // Check full replay.
    const BoardStateTuple& transposition_key = _transposition_key();
    std::vector<Move> switchyard;
    switchyard.reserve(move_stack.size());

    while (true) {
        if (count <= 1) {
            while (!switchyard.empty()) {
                const Move& move = switchyard.back();
                switchyard.pop_back();
                push(move);
            }

            return true;
        }

        if (static_cast<int>(move_stack.size()) < count - 1)
            break;

        const Move& move = pop();
        switchyard.push_back(move);

        if (is_irreversible(move))
            break;

        if (transposition_key == _transposition_key()) 
            count--;
    }
    
    while (!switchyard.empty()) {
        const Move& move = switchyard.back();
        switchyard.pop_back();
        push(move);
    }

    return false;
}

void Board::push(const Move& move) {
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
    // Push move and remember board state.
    this->move_stack.push_back(move);
    this->_stack.emplace_back(*this);

    // Reset en passant square.
    Square ep_square = this->ep_square;
    this->ep_square = NULL_SQUARE;

    // Increment move counters.
    this->halfmove_clock++;
    if (!this->turn)
        this->fullmove_number++;

    // On a null move, simply swap turns and reset the en passant square.
    if (!static_cast<bool>(move)) {
        this->turn = !this->turn;
        return;
    }

    // Zero the half-move clock.
    if (is_zeroing(move))
        this->halfmove_clock = 0;

    // Normalize the move to Chess960.
    Square to_square = this->_is_to_chess960(move);
    if (to_square == NULL_SQUARE) {
        to_square = move.to_square;
    }

    Bitboard from_bb = BB_SQUARES[move.from_square];
    Bitboard to_bb = BB_SQUARES[to_square];

    PieceType piece_type = _remove_piece_at(move.from_square);
    /* if (piece_type == NULL_PIECE)
        throw std::invalid_argument("piece not found at source square"); */
    PieceType captured_piece_type = piece_type_at(to_square);

    // Update castling rights.
    this->castling_rights &= ~to_bb & ~from_bb;
    if (piece_type == KING) {
        if (this->turn)
            this->castling_rights &= ~BB_RANK_1;
        else
            this->castling_rights &= ~BB_RANK_8;
    }

    // Handle special pawn moves.
    if (piece_type == PAWN) {
        int diff = move.to_square - move.from_square;

        if (diff == 16 && square_rank(move.from_square) == 1)
            this->ep_square = move.from_square + 8;
        else if (diff == -16 && square_rank(move.from_square) == 6)
            this->ep_square = move.from_square - 8;
        else if (move.to_square == ep_square && (std::abs(diff) == 7 || std::abs(diff) == 9) && captured_piece_type == NULL_PIECE) {
            // Remove pawns captured en passant.
            int down = this->turn ? -8 : 8;
            _remove_piece_at(ep_square + down);
        } 
    }

    // Promotion.
    if (move.promotion != NULL_PIECE) {
        piece_type = move.promotion;
    }

    // Castling.
    bool castling = (piece_type == KING && (this->occupied_co[this->turn] & to_bb));
    if (castling) {
        bool a_side = square_file(to_square) < square_file(move.from_square);

        this->_remove_piece_at(move.from_square);
        this->_remove_piece_at(to_square);

        if (a_side) {
            this->_set_piece_at(this->turn ? C1 : C8, KING, this->turn);
            this->_set_piece_at(this->turn ? D1 : D8, ROOK, this->turn);
        } else {
            this->_set_piece_at(this->turn ? G1 : G8, KING, this->turn);
            this->_set_piece_at(this->turn ? F1 : F8, ROOK, this->turn);
        }
    }

    // Put the piece on the target square.
    if (!castling)
        _set_piece_at(move.to_square, piece_type, this->turn);

    // Swap turns.
    this->turn = !this->turn;
}

Move Board::pop() {
    /*
    Restores the previous position and returns the last move from the stack.

    :raises: :exc:`IndexError` if the move stack is empty.
    */
    const Move& move = this->move_stack.back();
    this->move_stack.pop_back();
    this->_stack.back().restore(*this);
    this->_stack.pop_back();
    return move;
}

Move Board::peek() const {
    /*
    Gets the last move from the move stack.

    :raises: :exc:`IndexError` if the move stack is empty.
    */
   return this->move_stack.back();
}

Move Board::find_move(Square from_square, Square to_square, PieceType promotion) const {
    /*
    Finds a matching legal move for an origin square, a target square, and
    an optional promotion piece type.

    For pawn moves to the backrank, the promotion piece type defaults to
    :data:`chess.QUEEN`, unless otherwise specified.

    Castling moves are normalized to king moves by two steps, except in
    Chess960.

    :raises: :exc:`IllegalMoveError` if no matching legal move is found.
    */
    if (promotion == NULL_PIECE && this->pawns & BB_SQUARES[from_square] && BB_SQUARES[to_square] & BB_BACKRANKS)
        promotion = QUEEN;

    const Move& move = this->_from_chess960(from_square, to_square, promotion);
    if (!this->is_legal(move))
        throw IllegalMoveError("No matching legal move for " + move.uci() + " (" + SQUARE_NAMES[from_square] + " -> " + SQUARE_NAMES[to_square] + ") in " + this->fen());

    return move;
}

std::string Board::castling_shredder_fen() const {
    Bitboard castling_rights = this->clean_castling_rights();
    if (!castling_rights)
        return "-";

    std::vector<std::string> builder;

    StaticVector<Square, 64> squares;
    scan_forward(castling_rights & BB_RANK_1, squares);
    for (Square square : squares) {
        std::string file_str = FILE_NAMES[square_file(square)];
        std::transform(file_str.begin(), file_str.end(), file_str.begin(), ::toupper);
        builder.push_back(file_str);
    }

    scan_forward(castling_rights & BB_RANK_8, squares);
    for (Square square : squares)
        builder.push_back(FILE_NAMES[square_file(square)]);

    return accumulate(builder.begin(), builder.end(), std::string());
}

bool Board::has_pseudo_legal_en_passant() const {
    // Checks if there is a pseudo-legal en passant capture.
    StaticVector<Move, EP_CAPTURE_SIZE> pseudo_legal_ep;
    this->generate_pseudo_legal_ep(pseudo_legal_ep);
    return this->ep_square != NULL_SQUARE && std::any_of(pseudo_legal_ep.begin(), pseudo_legal_ep.end(), [](const Move& move) { return static_cast<bool>(move); });
}

bool Board::has_legal_en_passant() const {
    // Checks if there is a legal en passant capture.
    StaticVector<Move, EP_CAPTURE_SIZE> legal_ep;
    this->generate_legal_ep(legal_ep);
    return this->ep_square != NULL_SQUARE && std::any_of(legal_ep.begin(), legal_ep.end(), [](const Move& move) { return static_cast<bool>(move); });
}

void Board::set_fen(const std::string& fen) {
    /*
    Parses a FEN and sets the position from it.

    :raises: :exc:`ValueError` if syntactically invalid. Use
        :func:`~chess.Board.is_valid()` to detect invalid positions.
    */
    std::vector<std::string> parts;
    split(parts, fen, ' ');

    std::string board_part, turn_part, castling_part, ep_part, halfmove_part, fullmove_part;
    bool exception = false;
    Color turn = WHITE;
    Square ep_square = NULL_SQUARE;
    int halfmove_clock = 0;
    int fullmove_number = 1;
    std::string castling_rights = "-";

    // Board part.
    try {
        board_part = parts[0];
        parts.erase(parts.begin());
    } catch (std::out_of_range& e) {
        throw std::invalid_argument("empty FEN");
    }

    // Turn part.
    try {
        turn_part = parts[0];
        parts.erase(parts.begin());
    } catch (std::out_of_range& e) {
        exception = true;
        turn = WHITE;
    }
    if (!exception) {
        if (turn_part == "w")
            turn = WHITE;
        else if (turn_part == "b")
            turn = BLACK;
        else
            throw std::invalid_argument("expected 'w' or 'b' for turn part of fen: " + fen);
    }

    // Validate castling part.
    exception = false;
    try {
        castling_part = parts[0];
        parts.erase(parts.begin());    
    } catch (std::out_of_range& e) {
        exception = true;
        castling_part = "-";
    }
    if (!exception) {
        if (!std::regex_match(castling_part, FEN_CASTLING_REGEX)) {
            throw std::invalid_argument("invalid castling part of fen: " + fen);
        }
    }

    // En passant part.
    exception = false;
    try {
        ep_part = parts[0];
        parts.erase(parts.begin());
    } catch (std::out_of_range& e) {
        exception = true;
        ep_square = NULL_SQUARE;
    }
    if (!exception) {
        try {
            ep_square = ep_part == "-" ? NULL_SQUARE : std::distance(SQUARE_NAMES, std::find(SQUARE_NAMES, SQUARE_NAMES + 64, ep_part));
        } catch (std::invalid_argument& e) {
            throw std::invalid_argument("invalid en passant part of fen: " + fen);
        }
    }

    // Check that the half-move part is valid.
    exception = false;
    try {
        halfmove_part = parts[0];
        parts.erase(parts.begin());
    } catch (std::out_of_range& e) {
        exception = true;
        halfmove_clock = 0;
    }
    if (!exception) {
        try {
            halfmove_clock = std::stoi(halfmove_part);
        } catch (std::invalid_argument& e) {
            throw std::invalid_argument("invalid half-move clock in fen: " + fen);
        }

        if (halfmove_clock < 0)
            throw std::invalid_argument("half-move clock cannot be negative: " + fen);
    }

    // Check that the full-move number part is valid.
    // 0 is allowed for compatibility, but later replaced with 1.
    exception = false;
    try {
        fullmove_part = parts[0];
        parts.erase(parts.begin());
    } catch (std::out_of_range& e) {
        exception = true;
        fullmove_number = 1;
    }
    if (!exception) {
        try {
            fullmove_number = std::stoi(fullmove_part);
        } catch (std::invalid_argument& e) {
            throw std::invalid_argument("invalid fullmove number in fen: " + fen);
        }

        if (fullmove_number < 0)
            throw std::invalid_argument("fullmove number cannot be negative: " + fen);
        
        fullmove_number = std::max(fullmove_number, 1);
    }

    // All parts should be consumed now.
    if (!parts.empty())
        throw std::invalid_argument("fen string has more parts than expected: " + fen);

    // Validate the board part and set it.
    _set_board_fen(board_part);

    // Apply.
    this->turn = turn;
    _set_castling_fen(castling_part);
    this->ep_square = ep_square;
    this->halfmove_clock = halfmove_clock;
    this->fullmove_number = fullmove_number;
    clear_stack();
}

void Board::_set_castling_fen(const std::string& castling_fen) {
    if (castling_fen.empty() || castling_fen == "-") {
        castling_rights = BB_EMPTY;
        return;
    }

    if (!std::regex_match(castling_fen, FEN_CASTLING_REGEX)) {
        throw std::invalid_argument("invalid castling fen: " + castling_fen);
    }

    castling_rights = BB_EMPTY;

    for (char flag : castling_fen) {
        Color color = std::isupper(flag) ? WHITE : BLACK;
        flag = std::tolower(flag);
        Bitboard backrank = color ? BB_RANK_1 : BB_RANK_8;
        Bitboard rooks = this->occupied_co[color] & this->rooks & backrank;
        Square king = this->king(color);

        if (rooks == BB_EMPTY || king == NULL_SQUARE)
            continue;

        if (flag == 'q') {
            // Select the leftmost rook.
            if (lsb(rooks) < king)
                this->castling_rights |= rooks & -rooks;
            else
                this->castling_rights |= BB_FILE_A & backrank;
        } else if (flag == 'k') {
            // Select the rightmost rook.
            Square rook = msb(rooks);
            if (rook > king)
                this->castling_rights |= BB_SQUARES[rook];
            else
                this->castling_rights |= BB_FILE_H & backrank;
        }
    }
}

void Board::set_castling_fen(const std::string& castling_fen) {
    /*
    Sets castling rights from a string in FEN notation like ``Qqk``.

    Also clears the move stack.

    :raises: :exc:`ValueError` if the castling FEN is syntactically
        invalid.
    */
    this->_set_castling_fen(castling_fen);
    this->clear_stack();
}

void Board::set_board_fen(const std::string& fen) {
    Baseboard::set_board_fen(fen);
    this->clear_stack();
}

void Board::set_piece_map(const std::unordered_map<Square, Piece>& pieces) {
    Baseboard::set_piece_map(pieces);
    this->clear_stack();
}

std::string Board::san(const Move& move) {
    // Gets the standard algebraic notation of the given move in the context of the current position.
    return this->_algebraic(move);
}

std::string Board::lan(const Move& move) {
    // Gets the long algebraic notation of the given move in the context of the current position.
    return this->_algebraic(move, true);
}

std::string Board::san_and_push(const Move& move) {
    return this->_algebraic_and_push(move);
}

std::string Board::_algebraic(const Move& move, const bool& long_algebraic) {
    std::string san = this->_algebraic_and_push(move, long_algebraic);
    this->pop();
    return san;
}

std::string Board::_algebraic_and_push(const Move& move, const bool& long_algebraic) {
    std::string san = this->_algebraic_without_suffix(move, long_algebraic);

    // Look ahead for check or checkmate.
    this->push(move);
    bool check = is_check();
    bool checkmate = check && this->is_checkmate();

    // Add check or checkmate suffix.
    if (checkmate && move)
        return san + "#";
    else if (check && move)
        return san + "+";
    else
        return san;
}

std::string Board::_algebraic_without_suffix(const Move& move, const bool& long_algebraic) {
    // Null move.
    if (!move)
        return "--";

    // Castling.
    if (this->is_castling(move)) {
        if (square_file(move.to_square) < square_file(move.from_square))
            return "O-O-O";
        else
            return "O-O";
    }

    PieceType piece_type = this->piece_type_at(move.from_square);
    if (piece_type == NULL_PIECE)
        throw std::invalid_argument("piece not found at source square");
    bool capture = this->is_capture(move);

    std::string san;
    if (piece_type == PAWN)
        san = "";
    else {
        san = piece_symbol(piece_type);
        std::transform(san.begin(), san.end(), san.begin(), ::toupper);
    }

    if (long_algebraic) {
        san += SQUARE_NAMES[move.from_square];
    } else if (piece_type != PAWN) {
        // Get ambiguous move candidates.
        // Relevant candidates: not exactly the current move,
        // but to the same square.
        Bitboard others = BB_EMPTY;
        Bitboard from_mask = this->pieces_mask(piece_type, this->turn);
        from_mask &= ~BB_SQUARES[move.from_square];
        Bitboard to_mask = BB_SQUARES[move.to_square];

        StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
        this->generate_legal_moves(legal_moves, from_mask, to_mask);
        for (const Move& candidate : legal_moves)
            others |= BB_SQUARES[candidate.from_square];

        // Disambiguate.
        if (others) {
            bool row = false, column = false;

            if (others & BB_RANKS[square_rank(move.from_square)])
                column = true;

            if (others & BB_FILES[square_file(move.from_square)])
                row = true;
            else
                column = false;

            if (column)
                san += FILE_NAMES[square_file(move.from_square)];
            if (row)
                san += SQUARE_NAMES[square_rank(move.from_square)];
        }
    } else if (capture) {
        san += FILE_NAMES[square_file(move.from_square)];
    }

    // Captures.
    if (capture)
        san += "x";
    else if (long_algebraic)
        san += "-";

    // Destination square.
    san += SQUARE_NAMES[move.to_square];

    // Promotion.
    if (move.promotion != NULL_PIECE) {
        san += "=";
        std::string promotion = piece_symbol(move.promotion);
        std::transform(promotion.begin(), promotion.end(), promotion.begin(), ::toupper);
        san += promotion;
    }

    return san;
}

std::string Board::uci(const Move& move) const {
    // Gets the UCI notation of the move.
    return move.uci();
}

Move Board::parse_uci(const std::string& uci) const {
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
    const Move& move = Move::from_uci(uci);

    if (!move)
        return move;
    
    if (!this->is_legal(move))
        throw IllegalMoveError("Illegal UCI: " + uci + " in " + this->fen());

    return move;
}

Move Board::push_uci(const std::string& uci) {
    /*
    Parses a move in UCI notation and puts it on the move stack.

    Returns the move.

    :raises:
        :exc:`ValueError` (specifically an exception specified below) if the move is invalid or illegal in the
        current position (but not a null move).

        - :exc:`InvalidMoveError` if the UCI is syntactically invalid.
        - :exc:`IllegalMoveError` if the UCI is illegal.
    */
    const Move& move = this->parse_uci(uci);
    this->push(move);
    return move;
}

bool Board::is_en_passant(const Move& move) const {
    // Checks if the given pseudo-legal move is an en passant capture.
    return (this->ep_square != NULL_SQUARE && move.to_square == this->ep_square &&
            static_cast<bool>(this->pawns & BB_SQUARES[move.from_square]) &&
            (std::abs(move.from_square - move.to_square) == 7 || std::abs(move.from_square - move.to_square) == 9) &&
            !(this->occupied & BB_SQUARES[move.to_square]));
}

bool Board::is_en_passant(Square from_square, Square to_square) const {
    // Checks if the given pseudo-legal move is an en passant capture.
    return (this->ep_square != NULL_SQUARE && to_square == this->ep_square &&
            static_cast<bool>(this->pawns & BB_SQUARES[from_square]) &&
            (std::abs(from_square - to_square) == 7 || std::abs(from_square - to_square) == 9) &&
            !(this->occupied & BB_SQUARES[to_square]));
}

bool Board::is_capture(const Move& move) const {
    // Checks if the given pseudo-legal move is a capture.
    Bitboard touched = BB_SQUARES[move.from_square] ^ BB_SQUARES[move.to_square];
    return static_cast<bool>(touched & this->occupied_co[!this->turn]) || this->is_en_passant(move);
}

bool Board::is_zeroing(const Move& move) const {
    // Checks if the given pseudo-legal move is a capture or pawn move.
    Bitboard touched = BB_SQUARES[move.from_square] ^ BB_SQUARES[move.to_square];
    return static_cast<bool>(touched & this->pawns || touched & this->occupied_co[!this->turn]);
}

bool Board::_reduces_castling_rights(const Move& move) const {
    Bitboard touched = BB_SQUARES[move.from_square] ^ BB_SQUARES[move.to_square];
    return static_cast<bool>(touched & this->castling_rights ||
                (this->castling_rights & BB_RANK_1 && touched & this->kings & this->occupied_co[WHITE]) ||
                (this->castling_rights & BB_RANK_8 && touched & this->kings & this->occupied_co[BLACK]));
}

bool Board::is_irreversible(const Move& move) const {
    /*
    Checks if the given pseudo-legal move is irreversible.

    In standard chess, pawn moves, captures, moves that destroy castling
    rights and moves that cede en passant are irreversible.

    This method has false-negatives with forced lines. For example, a check
    that will force the king to lose castling rights is not considered
    irreversible. Only the actual king move is.
    */
    return this->is_zeroing(move) || this->_reduces_castling_rights(move) || this->has_legal_en_passant();
}

bool Board::is_castling(const Move& move) const {
    // Checks if the given pseudo-legal move is a castling move.
    if (this->kings & BB_SQUARES[move.from_square]) {
        int diff = square_file(move.from_square) - square_file(move.to_square);
        return (std::abs(diff) > 1 || static_cast<bool>(this->rooks & this->occupied_co[this->turn] & BB_SQUARES[move.to_square]));
    }
    return false;
}

bool Board::is_castling(Square from_square, Square to_square) const {
    // Checks if the given pseudo-legal move is a castling move.
    if (this->kings & BB_SQUARES[from_square]) {
        int diff = square_file(from_square) - square_file(to_square);
        return (std::abs(diff) > 1 || static_cast<bool>(this->rooks & this->occupied_co[this->turn] & BB_SQUARES[to_square]));
    }
    return false;
}

bool Board::is_kingside_castling(const Move& move) const {
    // Checks if the given pseudo-legal move is a kingside castling move.
    return this->is_castling(move) && square_file(move.to_square) > square_file(move.from_square);
}

bool Board::is_queenside_castling(const Move& move) const {
    // Checks if the given pseudo-legal move is a queenside castling move.
    return this->is_castling(move) && square_file(move.to_square) < square_file(move.from_square);
}

Bitboard Board::clean_castling_rights() const {
    // Returns the castling rights of the current position filtered.
    if (!this->_stack.empty())
        // No new castling rights are assigned in a game
        // so we can assume they were already filtered.
        return this->castling_rights;

    Bitboard castling = this->castling_rights & this->rooks;
    Bitboard white_castling = castling & BB_RANK_1 & this->occupied_co[WHITE];
    Bitboard black_castling = castling & BB_RANK_8 & this->occupied_co[BLACK];

    // The rook must be on a1, h1, a8 or h8.
    white_castling &= (BB_A1 | BB_H1);
    black_castling &= (BB_A8 | BB_H8);

    // The king must be on e1 or e8.
    if (!(this->occupied_co[WHITE] & this->kings & BB_E1))
        white_castling = BB_EMPTY;
    if (!(this->occupied_co[BLACK] & this->kings & BB_E8))
        black_castling = BB_EMPTY;

    return white_castling | black_castling;
}

bool Board::has_castling_rights(Color color) const {
    // Checks if the given color has castling rights.
    Bitboard backrank = color ? BB_RANK_1 : BB_RANK_8;
    return static_cast<bool>(this->castling_rights & backrank);
}

bool Board::has_kingside_castling_rights(Color color) const {
    // Checks if the given color has kingside castling rights.
    Bitboard backrank = color ? BB_RANK_1 : BB_RANK_8;
    Bitboard king_mask = this->kings & this->occupied_co[color] & backrank;
    if (!king_mask)
        return false;

    Bitboard castle_rights = this->castling_rights & backrank;
    while (castle_rights) {
        Bitboard rook = castle_rights & -castle_rights;

        if (rook > king_mask)
            return true;

        castle_rights &= castle_rights - 1;
    }

    return false;
}

bool Board::has_queenside_castling_rights(Color color) const {
    // Checks if the given color has queenside castling rights.
    Bitboard backrank = color ? BB_RANK_1 : BB_RANK_8;
    Bitboard king_mask = this->kings & this->occupied_co[color] & backrank;
    if (!king_mask)
        return false;

    Bitboard castle_rights = this->castling_rights & backrank;
    while (castle_rights) {
        Bitboard rook = castle_rights & -castle_rights;

        if (rook < king_mask)
            return true;

        castle_rights &= castle_rights - 1;
    }

    return false;
}

Status Board::status() const {
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
    Status errors = STATUS_VALID;

    // There must be at least one piece.
    if (!this->occupied)
        errors |= STATUS_EMPTY;
    
    // There must be exactly one king of each color.
    if (!(this->kings & this->occupied_co[WHITE]))
        errors |= STATUS_NO_WHITE_KING;
    if (!(this->kings & this->occupied_co[BLACK]))
        errors |= STATUS_NO_BLACK_KING;
    if (std::bitset<64>(this->kings & this->occupied).count() > 2)
        errors |= STATUS_TOO_MANY_KINGS;

    // There can not be more than 16 pieces of each color.
    if (std::bitset<64>(this->occupied_co[WHITE]).count() > 16)
        errors |= STATUS_TOO_MANY_WHITE_PIECES;
    if (std::bitset<64>(this->occupied_co[BLACK]).count() > 16)
        errors |= STATUS_TOO_MANY_BLACK_PIECES;

    // There can not be more than 8 pawns of each color.
    if (std::bitset<64>(this->pawns & this->occupied_co[WHITE]).count() > 8)
        errors |= STATUS_TOO_MANY_WHITE_PAWNS;
    if (std::bitset<64>(this->pawns & this->occupied_co[BLACK]).count() > 8)
        errors |= STATUS_TOO_MANY_BLACK_PAWNS;

    // There can not be pawns on the backrank.
    if (this->pawns & BB_BACKRANKS)
        errors |= STATUS_PAWNS_ON_BACKRANK;
        
    // Castling rights.
    if (this->castling_rights != this->clean_castling_rights())
        errors |= STATUS_BAD_CASTLING_RIGHTS;
    
    // En passant square.
    Square valid_ep_square = this->_valid_ep_square();
    if (this->ep_square == valid_ep_square)
        errors |= STATUS_INVALID_EP_SQUARE;

    // Side to move giving check.
    if (this->was_into_check())
        errors |= STATUS_OPPOSITE_CHECK;

    // More than maximum number of possible checkers.
    Bitboard checkers = this->checkers_mask();
    Bitboard our_king = this->kings & this->occupied_co[this->turn];
    if (checkers) {
        if (std::bitset<64>(checkers).count() > 2)
            errors |= STATUS_TOO_MANY_CHECKERS;

        if (valid_ep_square != NULL_SQUARE) {
            Square pushed_to = valid_ep_square ^ A2;
            Square pushed_from = valid_ep_square ^ A4;
            Bitboard occupied_before = (this->occupied & ~BB_SQUARES[pushed_to]) | BB_SQUARES[pushed_from];
            if (std::bitset<64>(checkers).count() > 1 ||
                (msb(checkers) != pushed_to &&
                this->_attacked_for_king(our_king, occupied_before)))
                errors |= STATUS_IMPOSSIBLE_CHECK;
        } else {
            if (std::bitset<64>(checkers).count() > 2 ||
                (std::bitset<64>(checkers).count() == 2 && ray(lsb(checkers), msb(checkers)) & our_king))
                errors |= STATUS_IMPOSSIBLE_CHECK;
        }
    }

    return errors;
}

Square Board::_valid_ep_square() const {
    if (this->ep_square == NULL_SQUARE)
        return NULL_SQUARE;
    
    int ep_rank;
    Bitboard pawn_mask, seventh_rank_mask;
    if (this->turn) {
        ep_rank = 5;
        pawn_mask = shift_down(BB_SQUARES[this->ep_square]);
        seventh_rank_mask = shift_up(BB_SQUARES[this->ep_square]);
    } else {
        ep_rank = 2;
        pawn_mask = shift_up(BB_SQUARES[this->ep_square]);
        seventh_rank_mask = shift_down(BB_SQUARES[this->ep_square]);
    }

    // The en passant square must be on the 3rd or 6th rank.
    if (square_rank(this->ep_square) != ep_rank)
        return NULL_SQUARE;

    // The last move must have been a double pawn push, so there must
    // be a pawn of the correct color on the fourth or fifth rank.
    if (!(this->pawns & this->occupied_co[!this->turn] & pawn_mask))
        return NULL_SQUARE;

    // And the en passant square must be empty.
    if (this->occupied & BB_SQUARES[this->ep_square])
        return NULL_SQUARE;

    // And the second rank must be empty.
    if (this->occupied & seventh_rank_mask)
        return NULL_SQUARE;

    return this->ep_square;
}

bool Board::is_valid() const {
    return this->status() == STATUS_VALID;
}

bool Board::_ep_skewered_by_color(Color color, Square king, Square capturer) const {
    // Handle the special case where the king would be in check if the
    // pawn and its capturer disappear from the rank.

    // Vertical skewers of the captured pawn are not possible. (Pins on
    // the capturer are not handled here.)
    /* if (this->ep_square == NULL_SQUARE)
        throw std::invalid_argument("no en passant square"); */

    Square last_double = this->ep_square + (color ? -8 : 8);

    Bitboard occupancy = ((this->occupied & ~BB_SQUARES[last_double] &
                          ~BB_SQUARES[capturer]) | BB_SQUARES[this->ep_square]);

    // Horizontal attack on the 5th or 4th rank.
    Bitboard horizontal_attackers = this->occupied_co[!color] & (this->rooks | this->queens);
    if (BB_RANK_TABLE.get(king, occupancy) & horizontal_attackers)
        return true;

    // Diagonal skewers. These are not actually possible in a real game,
    // because if the latest double pawn move covers a diagonal attack,
    // then the other side would have been in check already.
    Bitboard diagonal_attackers = this->occupied_co[!color] & (this->bishops | this->queens);
    if (BB_DIAG_TABLE.get(king, occupancy) & diagonal_attackers)
        return true;

    return false;
}

bool Board::_ep_skewered(Square king, Square capturer) const {
    // Handle the special case where the king would be in check if the
    // pawn and its capturer disappear from the rank.

    // Vertical skewers of the captured pawn are not possible. (Pins on
    // the capturer are not handled here.)
    /* if (this->ep_square == NULL_SQUARE)
        throw std::invalid_argument("no en passant square"); */

    Square last_double = this->ep_square + (this->turn ? -8 : 8);

    Bitboard occupancy = ((this->occupied & ~BB_SQUARES[last_double] &
                          ~BB_SQUARES[capturer]) | BB_SQUARES[this->ep_square]);

    // Horizontal attack on the 5th or 4th rank.
    Bitboard horizontal_attackers = this->occupied_co[!this->turn] & (this->rooks | this->queens);
    if (BB_RANK_TABLE.get(king, occupancy) & horizontal_attackers)
        return true;

    // Diagonal skewers. These are not actually possible in a real game,
    // because if the latest double pawn move covers a diagonal attack,
    // then the other side would have been in check already.
    Bitboard diagonal_attackers = this->occupied_co[!this->turn] & (this->bishops | this->queens);
    if (BB_DIAG_TABLE.get(king, occupancy) & diagonal_attackers)
        return true;

    return false;
}

Bitboard Board::_slider_blockers_by_color(Color color, Square king) const {
    Bitboard rooks_and_queens = this->rooks | this->queens;
    Bitboard bishops_and_queens = this->bishops | this->queens;

    Bitboard snipers = ((BB_RANK_TABLE.get(king, BB_EMPTY) & rooks_and_queens) |
                        (BB_FILE_TABLE.get(king, BB_EMPTY) & rooks_and_queens) |
                        (BB_DIAG_TABLE.get(king, BB_EMPTY) & bishops_and_queens));

    Bitboard blockers = BB_EMPTY;

    StaticVector<Square, 16> squares;
    scan_forward(snipers & this->occupied_co[!color], squares);
    for (Square sniper : squares) {
        Bitboard b = between(king, sniper) & this->occupied;

        // Add to blockers if exactly one piece in between.
        if (b && BB_SQUARES[msb(b)] == b)
            blockers |= b;
    }

    return blockers & this->occupied_co[color];
}

Bitboard Board::_slider_blockers(Square king) const {
    Bitboard rooks_and_queens = this->rooks | this->queens;
    Bitboard bishops_and_queens = this->bishops | this->queens;

    Bitboard snipers = ((BB_RANK_TABLE.get(king, BB_EMPTY) & rooks_and_queens) |
                        (BB_FILE_TABLE.get(king, BB_EMPTY) & rooks_and_queens) |
                        (BB_DIAG_TABLE.get(king, BB_EMPTY) & bishops_and_queens));

    Bitboard blockers = BB_EMPTY;

    StaticVector<Square, 16> squares;
    scan_forward(snipers & this->occupied_co[!this->turn], squares);
    for (Square sniper : squares) {
        Bitboard b = between(king, sniper) & this->occupied;

        // Add to blockers if exactly one piece in between.
        if (b && BB_SQUARES[msb(b)] == b)
            blockers |= b;
    }

    return blockers & this->occupied_co[this->turn];
}

bool Board::_is_safe_by_color(Color color, Square king, const Bitboard& blockers, const Move& move) const {
   if (move.from_square == king) {
        if (this->is_castling(move))
            return true;
        else
            return !this->is_attacked_by(!color, move.to_square);
    } else if (this->is_en_passant(move)) {
        return static_cast<bool>(this->pin_mask(color, move.from_square) & BB_SQUARES[move.to_square] &&
                    !this->_ep_skewered_by_color(color, king, move.from_square));
    } else {
        return static_cast<bool>(!(blockers & BB_SQUARES[move.from_square]) ||
                    ray(move.from_square, move.to_square) & BB_SQUARES[king]);
    } 
}

bool Board::_is_safe(Square king, const Bitboard& blockers, const Move& move) const {
    if (move.from_square == king) {
        if (this->is_castling(move))
            return true;
        else
            return !this->is_attacked_by(!this->turn, move.to_square);
    } else if (this->is_en_passant(move)) {
        return static_cast<bool>(this->pin_mask(this->turn, move.from_square) & BB_SQUARES[move.to_square] &&
                    !this->_ep_skewered(king, move.from_square));
    } else {
        return static_cast<bool>(!(blockers & BB_SQUARES[move.from_square]) ||
                    ray(move.from_square, move.to_square) & BB_SQUARES[king]);
    }
}

bool Board::_is_safe_by_color(Color color, Square king, const Bitboard& blockers, Square from_square, Square to_square) const {
    if (from_square == king) {
        if (this->is_castling(from_square, to_square))
            return true;
        else
            return !this->is_attacked_by(!color, to_square);
    } else if (this->is_en_passant(from_square, to_square)) {
        return static_cast<bool>(this->pin_mask(color, from_square) & BB_SQUARES[to_square] &&
                    !this->_ep_skewered_by_color(color, king, from_square));
    } else {
        return static_cast<bool>(!(blockers & BB_SQUARES[from_square]) ||
                    ray(from_square, to_square) & BB_SQUARES[king]);
    }
}

bool Board::_is_safe(Square king, const Bitboard& blockers, Square from_square, Square to_square) const {
    if (from_square == king) {
        if (this->is_castling(from_square, to_square))
            return true;
        else
            return !this->is_attacked_by(!this->turn, to_square);
    } else if (this->is_en_passant(from_square, to_square)) {
        return static_cast<bool>(this->pin_mask(this->turn, from_square) & BB_SQUARES[to_square] &&
                    !this->_ep_skewered(king, from_square));
    } else {
        return static_cast<bool>(!(blockers & BB_SQUARES[from_square]) ||
                    ray(from_square, to_square) & BB_SQUARES[king]);
    }
}

void Board::_generate_evasions_by_color(StaticVector<Move, EVASION_SIZE>& evasions, Color color, Square king, const Bitboard& checkers, const Bitboard& from_mask, const Bitboard& to_mask) const {
    evasions.clear();
    
    Bitboard sliders = checkers & (this->rooks | this->queens | this->bishops);

    Bitboard attacked = BB_EMPTY;

    StaticVector<Square, 32> squares;
    scan_forward(sliders, squares);
    for (Square checker : squares)
        attacked |= ray(king, checker) & ~BB_SQUARES[checker];

    if (BB_SQUARES[king] & from_mask) {
        scan_forward(BB_KING_ATTACKS[king] & ~this->occupied_co[color] & ~attacked & to_mask, squares);
        for (Square to_square : squares)
            evasions.emplace_back(king, to_square);
    }

    Square checker = msb(checkers);
    if (BB_SQUARES[checker] == checkers) {
        // Capture or block a single checker.
        Bitboard target = between(king, checker) | checkers;

        StaticVector<Move, LEGAL_MOVES_SIZE> pseudo_legal_moves;
        this->generate_pseudo_legal_moves_by_color(pseudo_legal_moves, color, ~this->kings & from_mask, target & to_mask);
        evasions.append(pseudo_legal_moves);

        // Capture the checking pawn en passant (but avoiding yielding
        // duplicate moves).
        if (this->ep_square != NULL_SQUARE && !(BB_SQUARES[this->ep_square] & target)) {
            Square last_double = this->ep_square + (color ? -8 : 8);
            if (last_double == checker) {
                StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
                this->generate_pseudo_legal_ep_by_color(ep_moves, color, from_mask, to_mask);
                evasions.append(ep_moves);
            }
        }
    }

    return;
}

void Board::_generate_evasions(StaticVector<Move, EVASION_SIZE>& evasions, Square king, const Bitboard& checkers, const Bitboard& from_mask, const Bitboard& to_mask) const {
    evasions.clear();
    
    Bitboard sliders = checkers & (this->rooks | this->queens | this->bishops);

    Bitboard attacked = BB_EMPTY;

    StaticVector<Square, 32> squares;
    scan_forward(sliders, squares);
    for (Square checker : squares)
        attacked |= ray(king, checker) & ~BB_SQUARES[checker];

    if (BB_SQUARES[king] & from_mask) {
        scan_forward(BB_KING_ATTACKS[king] & ~this->occupied_co[this->turn] & ~attacked & to_mask, squares);
        for (Square to_square : squares)
            evasions.emplace_back(king, to_square);
    }

    Square checker = msb(checkers);
    if (BB_SQUARES[checker] == checkers) {
        // Capture or block a single checker.
        Bitboard target = between(king, checker) | checkers;

        StaticVector<Move, LEGAL_MOVES_SIZE> pseudo_legal_moves;
        this->generate_pseudo_legal_moves(pseudo_legal_moves, ~this->kings & from_mask, target & to_mask);
        evasions.append(pseudo_legal_moves);

        // Capture the checking pawn en passant (but avoiding yielding
        // duplicate moves).
        if (this->ep_square != NULL_SQUARE && !(BB_SQUARES[this->ep_square] & target)) {
            Square last_double = this->ep_square + (this->turn ? -8 : 8);
            if (last_double == checker) {
                StaticVector<Move, EP_CAPTURE_SIZE> ep_moves;
                this->generate_pseudo_legal_ep(ep_moves, from_mask, to_mask);
                evasions.append(ep_moves);
            }
        }
    }

    return;
}

void Board::generate_legal_moves_by_color(StaticVector<Move, LEGAL_MOVES_SIZE>& legal_moves, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_moves.clear();
    
    Bitboard king_mask = this->kings & this->occupied_co[color];
    StaticVector<Move, EVASION_SIZE> evasions;
    if (king_mask) {
        Square king = msb(king_mask);
        Bitboard blockers = this->_slider_blockers_by_color(color, king);
        Bitboard checkers = this->attackers_mask(!color, king);
        if (checkers) {
            this->_generate_evasions_by_color(evasions, color, king, checkers, from_mask, to_mask);
            for (const Move& move : evasions) {
                if (this->_is_safe_by_color(color, king, blockers, move))
                    legal_moves.push_back(move);
            }
        } else {
            this->generate_pseudo_safe_moves_by_color(legal_moves, color, king, blockers, from_mask, to_mask);
        }
    } else {
        this->generate_pseudo_legal_moves_by_color(legal_moves, color, from_mask, to_mask);
    }

    return;
}

void Board::generate_legal_moves(StaticVector<Move, LEGAL_MOVES_SIZE>& legal_moves, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_moves.clear();
    
    Bitboard king_mask = this->kings & this->occupied_co[this->turn];
    StaticVector<Move, EVASION_SIZE> evasions;
    if (king_mask) {
        Square king = msb(king_mask);
        Bitboard blockers = this->_slider_blockers(king);
        Bitboard checkers = this->attackers_mask(!this->turn, king);
        if (checkers) {
            this->_generate_evasions(evasions, king, checkers, from_mask, to_mask);
            for (const Move& move : evasions) {
                if (this->_is_safe(king, blockers, move))
                    legal_moves.push_back(move);
            }
        } else {
            this->generate_pseudo_safe_moves(legal_moves, king, blockers, from_mask, to_mask);
        }
    } else {
        this->generate_pseudo_legal_moves(legal_moves, from_mask, to_mask);
    }

    return;
}

void Board::generate_legal_ep_by_color(StaticVector<Move, EP_CAPTURE_SIZE>& legal_eps, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_eps.clear();
    StaticVector<Move, EP_CAPTURE_SIZE> tmp_moves;
    this->generate_pseudo_legal_ep_by_color(tmp_moves, color, from_mask, to_mask);
    for (const Move& move : tmp_moves) {
        if (!this->is_into_check(move))
            legal_eps.push_back(move);
    }
    return;
}

void Board::generate_legal_ep(StaticVector<Move, EP_CAPTURE_SIZE>& legal_eps, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_eps.clear();
    StaticVector<Move, EP_CAPTURE_SIZE> tmp_moves;
    this->generate_pseudo_legal_ep(tmp_moves, from_mask, to_mask);
    for (const Move& move : tmp_moves) {
        if (!this->is_into_check(move))
            legal_eps.push_back(move);
    }
    return;
}

void Board::generate_legal_captures_by_color(StaticVector<Move, CAPTURES_SIZE>& legal_captures, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_captures.clear();
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->generate_legal_moves_by_color(legal_moves, color, from_mask, to_mask & this->occupied_co[!color]);
    StaticVector<Move, EP_CAPTURE_SIZE> legal_eps;
    this->generate_legal_ep_by_color(legal_eps, from_mask, to_mask, color);
    legal_captures.append(legal_moves);
    legal_captures.append(legal_eps);
    return;
}

void Board::generate_legal_captures(StaticVector<Move, CAPTURES_SIZE>& legal_captures, const Bitboard& from_mask, const Bitboard& to_mask) const {
    legal_captures.clear();
    StaticVector<Move, LEGAL_MOVES_SIZE> legal_moves;
    this->generate_legal_moves(legal_moves, from_mask, to_mask & this->occupied_co[!this->turn]);
    StaticVector<Move, EP_CAPTURE_SIZE> legal_eps;
    this->generate_legal_ep(legal_eps, from_mask, to_mask);
    legal_captures.append(legal_moves);
    legal_captures.append(legal_eps);
    return;
}

bool Board::_attacked_for_king_by_color(Color color, const Bitboard& path, const Bitboard& occupied) const {
    StaticVector<Bitboard, 8> attackers;
    StaticVector<Square, 32> squares;
    scan_forward(path, squares);
    for (Square square : squares)
        attackers.push_back(attackers_mask(!color, square, occupied));

    return std::any_of(attackers.begin(), attackers.end(), [](Bitboard bb) { return bb; });
}

bool Board::_attacked_for_king(const Bitboard& path, const Bitboard& occupied) const {
    StaticVector<Bitboard, 8> attackers;
    StaticVector<Square, 32> squares;
    scan_forward(path, squares);
    for (Square square : squares)
        attackers.push_back(attackers_mask(!this->turn, square, occupied));

    return std::any_of(attackers.begin(), attackers.end(), [](Bitboard bb) { return bb; });
}

void Board::generate_castling_moves_by_color(StaticVector<Move, CASTLING_MOVES_SIZE>& castling_moves, Color color, const Bitboard& from_mask, const Bitboard& to_mask) const {
    castling_moves.clear();

    Bitboard backrank = color ? BB_RANK_1 : BB_RANK_8;
    Bitboard king = kings & occupied_co[color] & backrank & from_mask;
    king &= -king;
    if (!king)
        return;

    Bitboard bb_c = BB_FILE_C & backrank;
    Bitboard bb_d = BB_FILE_D & backrank;
    Bitboard bb_f = BB_FILE_F & backrank;
    Bitboard bb_g = BB_FILE_G & backrank;

    StaticVector<Square, 64> squares;
    scan_forward(this->castling_rights & backrank & to_mask, squares);
    for (Square candidate : squares) {
        Bitboard rook = BB_SQUARES[candidate];

        bool a_side = rook < king;
        Bitboard king_to = a_side ? bb_c : bb_g;
        Bitboard rook_to = a_side ? bb_d : bb_f;

        Bitboard king_path = between(msb(king), msb(king_to));
        Bitboard rook_path = between(candidate, msb(rook_to));

        if (!((occupied ^ king ^ rook) & (king_path | rook_path | king_to | rook_to) ||
            _attacked_for_king_by_color(color, king_path | king, occupied ^ king) ||
            _attacked_for_king_by_color(color, king_to, occupied ^ king ^ rook ^ rook_to)))
            castling_moves.emplace_back(_from_chess960(msb(king), candidate));
    }

    return;
}

void Board::generate_castling_moves(StaticVector<Move, CASTLING_MOVES_SIZE>& castling_moves, const Bitboard& from_mask, const Bitboard& to_mask) const {
    castling_moves.clear();

    Bitboard backrank = this->turn ? BB_RANK_1 : BB_RANK_8;
    Bitboard king = kings & occupied_co[this->turn] & backrank & from_mask;
    king &= -king;
    if (!king)
        return;

    Bitboard bb_c = BB_FILE_C & backrank;
    Bitboard bb_d = BB_FILE_D & backrank;
    Bitboard bb_f = BB_FILE_F & backrank;
    Bitboard bb_g = BB_FILE_G & backrank;

    StaticVector<Square, 2> squares;
    scan_forward(this->castling_rights & backrank & to_mask, squares);
    for (Square candidate : squares) {
        Bitboard rook = BB_SQUARES[candidate];

        bool a_side = rook < king;
        Bitboard king_to = a_side ? bb_c : bb_g;
        Bitboard rook_to = a_side ? bb_d : bb_f;

        Bitboard king_path = between(msb(king), msb(king_to));
        Bitboard rook_path = between(candidate, msb(rook_to));

        if (!((occupied ^ king ^ rook) & (king_path | rook_path | king_to | rook_to) ||
            _attacked_for_king(king_path | king, occupied ^ king) ||
            _attacked_for_king(king_to, occupied ^ king ^ rook ^ rook_to)))
            castling_moves.emplace_back(_from_chess960(msb(king), candidate));
    }

    return;
}

Move Board::_from_chess960(Square from_square, Square to_square, PieceType promotion) const {
    if (promotion == NULL_PIECE) {
        if (from_square == E1 && kings & BB_E1) {
            if (to_square == H1)
                return Move(E1, G1);
            else if (to_square == A1)
                return Move(E1, C1);
        } else if (from_square == E8 && kings & BB_E8) {
            if (to_square == H8)
                return Move(E8, G8);
            else if (to_square == A8)
                return Move(E8, C8);
        }
    }

    return Move(from_square, to_square, promotion);
}

Square Board::_is_from_chess960(const Move& move) const {
    if (move.promotion == NULL_PIECE) {
        if (move.from_square == E1 && (kings & BB_E1)) {
            if (move.to_square == H1)
                return G1;
            else if (move.to_square == A1)
                return C1;
        } else if (move.from_square == E8 && (kings & BB_E8)) {
            if (move.to_square == H8)
                return G8;
            else if (move.to_square == A8)
                return C8;
        }
    }
    return NULL_SQUARE;
}

void Board::_to_chess960(Move& move) const {
    if (move.from_square == E1 && kings & BB_E1) {
        if (move.to_square == G1 && !(rooks & BB_G1)) {
            move.to_square = H1;
        } else if (move.to_square == C1 && !(rooks & BB_C1)) {
            move.to_square = A1;
        }
    } else if (move.from_square == E8 && (kings & BB_E8)) {
        if (move.to_square == G8 && !(rooks & BB_G8)) {
            move.to_square = H8;
        } else if (move.to_square == C8 && !(rooks & BB_C8)) {
            move.to_square = A8;
        }
    }
}

Square Board::_is_to_chess960(const Move& move) const {
    if (move.from_square == E1 && (kings & BB_E1)) {
        if (move.to_square == G1 && !(rooks & BB_G1)) {
            return H1;
        } else if (move.to_square == C1 && !(rooks & BB_C1)) {
            return A1;
        }
    } else if (move.from_square == E8 && (kings & BB_E8)) {
        if (move.to_square == G8 && !(rooks & BB_G8)) {
            return H8;
        } else if (move.to_square == C8 && !(rooks & BB_C8)) {
            return A8;
        }
    }
    return NULL_SQUARE;
}

BoardStateTuple Board::_transposition_key() const {
    return BoardStateTuple(
        pawns, knights, bishops, rooks, queens, kings,
        occupied_co[WHITE], occupied_co[BLACK],
        turn, castling_rights,
        has_legal_en_passant() ? ep_square : NULL_SQUARE
    );
}

bool Board::operator==(const Board& other) const {
    return (this->halfmove_clock == other.halfmove_clock &&
            this->fullmove_number == other.fullmove_number &&
            /* Board::uci_variant == other.uci_variant && */
            this->_transposition_key() == other._transposition_key());
}

bool Board::operator!=(const Board& other) const {
    return !(*this == other);
}

void Board::apply_transform(const std::function<Bitboard(Bitboard)>& f) {
    Baseboard::apply_transform(f);
    this->clear_stack();
    this->ep_square = this->ep_square != NULL_SQUARE ? msb(f(BB_SQUARES[this->ep_square])) : NULL_SQUARE;
    this->castling_rights = f(this->castling_rights);
}

Board Board::transform(const std::function<Bitboard(Bitboard)>& f) {
    Board board = this->copy(0);
    board.apply_transform(f);
    return board;
}

void Board::apply_mirror() {
    Baseboard::apply_mirror();
    this->turn = !this->turn;
}

Board Board::mirror() const {
    /*
    Returns a mirrored copy of the board.

    The board is mirrored vertically and piece colors are swapped, so that
    the position is equivalent modulo color. Also swap the "en passant"
    square, castling rights and turn.

    Alternatively, :func:`~chess.Board.apply_mirror()` can be used
    to mirror the board.
    */
    Board board = this->copy();
    board.apply_mirror();
    return board;
}

Board Board::copy(int stack) const {
    /*
    Creates a copy of the board.

    Defaults to copying the entire move stack. Alternatively, *stack* can
    be ``False``, or an integer to copy a limited number of moves.
    */
    Board board(Baseboard::copy());

    board.ep_square = ep_square;
    board.castling_rights = castling_rights;
    board.turn = this->turn;
    board.fullmove_number = fullmove_number;
    board.halfmove_clock = halfmove_clock;

    if (stack) {
        stack = stack < 0 ? move_stack.size() : stack;
        stack = std::min(static_cast<int>(move_stack.size()), stack);
        board.move_stack = std::vector<Move>(move_stack.begin(), move_stack.begin() + stack);
        board._stack = std::vector<_BoardState>(_stack.begin(), _stack.begin() + stack);
    }

    return board;
}

std::string Board::to_string() {
    /*
    Returns a string representation of the board.

    The string is a human-readable representation of the board, with ranks
    and files. The pieces are represented by their symbols, and the empty
    squares are represented by dots. The ranks are numbered from 8 to 1,
    and the files are lettered from a to h.
    */
    return std::string(*this);
}

Board Board::empty() {
    // Creates a new empty board. Also see :func:`~chess.Board.clear()`.
    return Board(std::nullopt);
}

/*
std::vector<std::string> Board::aliases = {"Standard", "Chess", "Classical", "Normal", "Illegal", "From Position"};
std::optional<std::string> Board::uci_variant = "chess";
std::optional<std::string> Board::xboard_variant = "normal";
std::string Board::starting_fen = STARTING_FEN;
bool Board::connected_kings = false;
bool Board::one_king = true;
bool Board::captures_compulsory = false;
*/

Bitboard Board::hash() const {
    // Returns the hash of the current position.
    Bitboard hash = 0;

    // Pieces.
    for (int pivot = 0; pivot < 2; pivot++) {
        StaticVector<Square, 32> scan_squares;
        scan_forward(this->occupied_co[pivot], scan_squares);
        for (Square square : scan_squares) {
            /* if (piece_type == NULL_PIECE)
                throw std::invalid_argument("piece not found at square " + std::to_string(square)); */
            hash ^= POLYGLOT_RANDOM_ARRAY[(this->piece_type_at(square) * 2 + pivot) * 64 + square];
        }
    }

    // Castling rights.
    if (this->has_kingside_castling_rights(WHITE))
        hash ^= POLYGLOT_RANDOM_ARRAY[768];
    if (this->has_queenside_castling_rights(WHITE))
        hash ^= POLYGLOT_RANDOM_ARRAY[769];
    if (this->has_kingside_castling_rights(BLACK))
        hash ^= POLYGLOT_RANDOM_ARRAY[770];
    if (this->has_queenside_castling_rights(BLACK))
        hash ^= POLYGLOT_RANDOM_ARRAY[771];

    // En passant square.
    if (this->ep_square != NULL_SQUARE) {
        // hash only if there's actually a pawn ready to capture it.
        Bitboard ep_mask;
        if (this->turn)
            ep_mask = shift_down(BB_SQUARES[this->ep_square]);
        else
            ep_mask = shift_up(BB_SQUARES[this->ep_square]);
        ep_mask = shift_left(ep_mask) | shift_right(ep_mask);

        if (ep_mask & this->pawns & this->occupied_co[this->turn])
            hash ^= POLYGLOT_RANDOM_ARRAY[772 + square_file(this->ep_square)];
    }

    // Turn.
    hash ^= this->turn ? POLYGLOT_RANDOM_ARRAY[780] : 0;
    return hash;
}



