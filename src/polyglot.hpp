#ifndef POLYGLOT_HPP
#define POLYGLOT_HPP

#include "chess.hpp"
#include <fstream>
#include <random>

using StrOrBytesPath = std::string;

static const int ENTRY_SIZE = 16; 

// Definition of MMEntry as per polyglot entry
struct MMEntry {
    uint64_t key;        // The Zobrist hash of the position.
    uint16_t raw_move;   // The raw binary representation of the move. Use MMEntry::move instead.
    uint16_t weight;     // An integer value that can be used as the weight for this entry.
    uint32_t learn;      // Another integer value that can be used for extra information.
    Move move;    // The chess::Move.

    // Constructor
    MMEntry(uint64_t k, uint16_t rm, uint16_t w, uint32_t l, const Move &m)
        : key(k), raw_move(rm), weight(w), learn(l), move(m) {}

    // Default constructor
    MMEntry() : key(0), raw_move(0), weight(0), learn(0), move(Move::null()) {}
};

// _EmptyMmap class: A workaround for empty opening books.
class _EmptyMmap : public std::vector<char> {
public:
    // size() method override is inherited from vector
    size_t size_mmap() const;

    // close() method does nothing.
    void close();

    // madvise() method does nothing.
    void madvise(int option);
};

// _randint helper function: returns a random integer between a and b (inclusive).
int _randint(std::optional<std::mt19937> &rng, int a, int b);

// MemoryMappedReader class: Maps a Polyglot opening book to memory.
class MemoryMappedReader {
private:
    // This vector represents the memory-mapped file content.
    std::vector<char> mmap_data;
    // Flag to indicate if we are using _EmptyMmap emulation.
    bool empty;
public:
    // Constructor: opens the file and memory maps it.
    MemoryMappedReader(const StrOrBytesPath &filename);

    // Destructor: calls close()
    ~MemoryMappedReader();

    // close: Closes the reader.
    void close();

    // __enter__ equivalent: returns reference to this reader.
    MemoryMappedReader &enter();

    // __exit__ equivalent: closes the reader.
    void exit(std::exception_ptr /*exc_type*/, std::exception_ptr /*exc_value*/, std::exception_ptr /*traceback*/);

    // Returns the number of entries.
    size_t length() const;

    // Helper function to convert big-endian bytes to uint64_t.
    static uint64_t read_uint64_be(const char* buf);

    // Helper function to convert big-endian bytes to uint16_t.
    static uint16_t read_uint16_be(const char* buf);

    // Helper function to convert big-endian bytes to uint32_t.
    static uint32_t read_uint32_be(const char* buf);

    // __getitem__ equivalent: returns the entry at the given index.
    MMEntry operator[](int index) const;

    // bisect_key_left: binary search for the leftmost entry with the given key.
    size_t bisect_key_left(uint64_t key) const;

    // __contains__ equivalent: returns true if an entry matching the given entry is found.
    bool contains(const MMEntry &entry) const;

    // find_all: Seeks a specific position and yields corresponding entries.
    // Overload when board parameter is provided as Board.
    std::vector<MMEntry> find_all(const Board &board, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // find_all overload when board parameter is provided as a key integer.
    std::vector<MMEntry> find_all(uint64_t key, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // Static helper for __contains__ implementation.
    std::vector<MMEntry> find_all_static(uint64_t key, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // find: Finds the main entry for the given position or Zobrist hash.
    // Overload for Board.
    MMEntry find(const Board &board, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // find: Overload for key integer.
    MMEntry find(uint64_t key, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // get: Returns the main entry for the given position or default.
    // Overload for Board.
    std::optional<MMEntry> get(const Board &board, const std::optional<MMEntry> &default_entry = std::nullopt, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // get: Overload for key integer.
    std::optional<MMEntry> get(uint64_t key, const std::optional<MMEntry> &default_entry = std::nullopt, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}) const;

    // choice: Uniformly selects a random entry for the given position.
    // Overload for Board.
    MMEntry choice(const Board &board, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}, std::optional<std::mt19937> random_gen = std::nullopt) const;

    // choice: Overload for key integer.
    MMEntry choice(uint64_t key, int minimum_weight = 1, const std::vector<Move>& exclude_moves = {}, std::optional<std::mt19937> random_gen = std::nullopt) const;

    // weighted_choice: Selects a random entry distributed by the weights of the entries.
    // Overload for Board.
    MMEntry weighted_choice(const Board &board, const std::vector<Move>& exclude_moves = {}, std::optional<std::mt19937> random_gen = std::nullopt) const;

    // weighted_choice: Overload for key integer.
    MMEntry weighted_choice(uint64_t key, const std::vector<Move>& exclude_moves = {}, std::optional<std::mt19937> random_gen = std::nullopt) const;
};

// open_reader function: creates a MemoryMappedReader for the given file.
MemoryMappedReader open_reader(const StrOrBytesPath &path);

#endif // POLYGLOT_HPP