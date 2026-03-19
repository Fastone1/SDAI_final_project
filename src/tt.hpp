#ifndef TT_HPP
#define TT_HPP

#include "chess.hpp"
#include "defs.hpp"

enum NodeType : uint8_t {
    UPPER_BOUND = 0,
    LOWER_BOUND = 1,
    EXACT = 2,
};

struct TTEntry {
    Bitboard zobrist_hash = 0;
    uint64_t data = 0; // Packed depth, score, move, NodeType
    uint64_t age = 0;
};

inline void extractMove(uint64_t data, Move& move) {
    move.from_square = static_cast<Square>((data >> 24) & 0xFF);
    move.to_square = static_cast<Square>((data >> 32) & 0xFF);
    move.promotion = static_cast<PieceType>((data >> 40) & 0xFF);
}

inline void extractMoveAndType(uint64_t data, int& depth, int& score, Move& move, NodeType& type) {
    depth = static_cast<int>(data & 0xFF);
    score = static_cast<int>((data >> 8) & 0xFFFF) - INF_BOUND;
    move.from_square = static_cast<Square>((data >> 24) & 0xFF);
    move.to_square = static_cast<Square>((data >> 32) & 0xFF);
    move.promotion = static_cast<PieceType>((data >> 40) & 0xFF);
    type = static_cast<NodeType>((data >> 48) & 0xFF);
}

inline int extractDepth(uint64_t data) {
    return static_cast<int>(data & 0xFF);
}

inline uint64_t packTTData(int depth, int score, const Move& move, NodeType type) {
    return static_cast<uint64_t>(depth) |
           (static_cast<uint64_t>(score + INF_BOUND) << 8) |
           (static_cast<uint64_t>(move.from_square) << 24) |
           (static_cast<uint64_t>(move.to_square) << 32) |
           (static_cast<uint64_t>(move.promotion) << 40) |
           (static_cast<uint64_t>(type) << 48);
}

constexpr size_t TT_MB = 64;  // e.g. 64 MiB
constexpr size_t TT_ENTRIES = (TT_MB * 1024 * 1024) / sizeof(TTEntry);

class TranspositionTable {
public:
    TranspositionTable() : size(TT_ENTRIES) {
        table = new TTEntry[size];
    }

    TranspositionTable(size_t size) : size(size) {
        table = new TTEntry[size];
    }

    ~TranspositionTable() {
        delete[] table;
    }

    void store2(const Bitboard& position, int depth, int score, const Move& move, NodeType type, int ply = 0) {
        size_t index = position % size;

        if (score > IS_MATE) score += ply;
        else if (score < -IS_MATE) score -= ply;
        
        uint64_t packed_data = packTTData(depth, score, move, type);
        table[index].zobrist_hash = position ^ packed_data;
        table[index].data = packed_data;
    }

    void store(const Bitboard& position, int depth, int score, const Move& move, NodeType type, int ply = 0) {
        size_t index = position % size;

        if (table[index].zobrist_hash != 0 &&   // Existing entry
            table[index].age >= this->current_age &&    // Same or newer age
            extractDepth(table[index].data) >= depth)   // Deeper or equal depth
            return; // Do not overwrite

        if (score > IS_MATE) score += ply;
        else if (score < -IS_MATE) score -= ply;
        
        uint64_t packed_data = packTTData(depth, score, move, type);
        table[index].zobrist_hash = position ^ packed_data;
        table[index].data = packed_data;
        table[index].age = this->current_age;
    }

    std::optional<TTEntry> get(const Bitboard& position) {
        size_t index = position % size;
        if ((table[index].zobrist_hash ^ table[index].data) == position) {
#ifdef DEBUG
            ++hits;
#endif
            return table[index];
        }
#ifdef DEBUG
        ++misses;
#endif
        return std::nullopt;
    }

    bool getPVMove(const Bitboard& position, Move& move) {
        size_t index = position % size;
        if ((table[index].zobrist_hash ^ table[index].data) == position) {
            extractMove(table[index].data, move);
            return true;
        }
        return false;
    }

    inline void increment_age() {
        ++current_age;
    }

    void clear() {
        for (size_t i = 0; i < size; ++i) {
            table[i].zobrist_hash = 0ULL;
            table[i].data = 0ULL;
        }
#ifdef DEBUG
        reset_stats();
#endif
        current_age = 0;
    }

    size_t getSize() const {
        return size;
    }

    size_t getUsedSize() const {
        size_t used = 0;
        for (size_t i = 0; i < size; ++i) {
            if (table[i].zobrist_hash != 0) {
                ++used;
            }
        }
        return used;
    }

    size_t getMemoryUsage() const {
        return size * sizeof(TTEntry);
    }

    double getLoadFactor() const {
        return static_cast<double>(getUsedSize()) / size;
    }

#ifdef DEBUG
    float getHitRate() const {
        size_t total = hits + misses;
        return total == 0 ? 0.0f : static_cast<float>(hits) / total;
    }

    int getHits() const {
        return hits;
    }

    int getMisses() const {
        return misses;
    }

    int getTotalAccesses() const {
        return hits + misses;
    }

    void reset_stats() {
        hits = 0;
        misses = 0;
    }
#endif

private:
#ifdef DEBUG
    unsigned int hits = 0, misses = 0;
#endif
    TTEntry *table;
    uint64_t current_age = 0;
    size_t size;
};

#endif // TT_HPP