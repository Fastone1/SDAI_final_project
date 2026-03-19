#include "polyglot.hpp"

int _randint(std::optional<std::mt19937> &rng, int a, int b) {
    if (!rng.has_value()) {
        // Use default random engine.
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(a, b);
        return dist(gen);
    } else {
        std::uniform_int_distribution<> dist(a, b);
        return dist(rng.value());
    }
}

// size() method override is inherited from vector
size_t _EmptyMmap::size_mmap() const {
    return 0;
}

// close() method does nothing.
void _EmptyMmap::close() {
    // Do nothing.
}

// madvise() method does nothing.
void _EmptyMmap::madvise(int option) {
    (void)option;
    // Do nothing.
}

// Constructor: opens the file and memory maps it.
MemoryMappedReader::MemoryMappedReader(const StrOrBytesPath &filename) : empty(false) {
    // Open file in binary mode for reading.
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    // Read entire file contents into buffer.
    file.seekg(0, std::ios::end);
    std::streamsize filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (filesize <= 0) {
        // Use _EmptyMmap workaround for empty opening books.
        empty = true;
        mmap_data.clear();
    } else {
        mmap_data.resize(static_cast<size_t>(filesize));
        if (!file.read(mmap_data.data(), filesize)) {
            throw std::runtime_error("Error reading file: " + filename);
        }
    }

    // Check if the file size is a multiple of ENTRY_SIZE.
    if (mmap_data.size() % ENTRY_SIZE != 0) {
        std::ostringstream oss;
        oss << "invalid file size: ensure " << filename << " is a valid polyglot opening book";
        throw std::runtime_error(oss.str());
    }

    // Close the file.
    file.close();
}

// Destructor: calls close()
MemoryMappedReader::~MemoryMappedReader() {
    close();
}

// close: Closes the reader.
void MemoryMappedReader::close() {
    // In our implementation, we clear the buffer.
    mmap_data.clear();
}

// __enter__ equivalent: returns reference to this reader.
MemoryMappedReader &MemoryMappedReader::enter() {
    return *this;
}

// __exit__ equivalent: closes the reader.
void MemoryMappedReader::exit(std::exception_ptr /*exc_type*/, std::exception_ptr /*exc_value*/, std::exception_ptr /*traceback*/) {
    close();
}

// Returns the number of entries.
size_t MemoryMappedReader::length() const {
    return mmap_data.size() / ENTRY_SIZE;
}

// Helper function to convert big-endian bytes to uint64_t.
uint64_t MemoryMappedReader::read_uint64_be(const char* buf) {
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | static_cast<unsigned char>(buf[i]);
    }
    return value;
}

// Helper function to convert big-endian bytes to uint16_t.
uint16_t MemoryMappedReader::read_uint16_be(const char* buf) {
    uint16_t value = 0;
    for (int i = 0; i < 2; ++i) {
        value = (value << 8) | static_cast<unsigned char>(buf[i]);
    }
    return value;
}

// Helper function to convert big-endian bytes to uint32_t.
uint32_t MemoryMappedReader::read_uint32_be(const char* buf) {
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value = (value << 8) | static_cast<unsigned char>(buf[i]);
    }
    return value;
}

// __getitem__ equivalent: returns the entry at the given index.
MMEntry MemoryMappedReader::operator[](int index) const {
    // Handle negative index.
    if (index < 0) {
        index = static_cast<int>(length()) + index;
    }
    size_t offset = static_cast<size_t>(index) * ENTRY_SIZE;
    if (offset + ENTRY_SIZE > mmap_data.size()) {
        throw std::out_of_range("IndexError");
    }

    // Unpack key, raw_move, weight, learn from buffer.
    const char* ptr = mmap_data.data() + offset;
    uint64_t key = read_uint64_be(ptr);
    uint16_t raw_move = read_uint16_be(ptr + 8);
    uint16_t weight = read_uint16_be(ptr + 10);
    uint32_t learn = read_uint32_be(ptr + 12);

    // Extract source and target square.
    int to_square = raw_move & 0x3f;
    int from_square = (raw_move >> 6) & 0x3f;

    // Extract the promotion type.
    int promotion_part = (raw_move >> 12) & 0x7;
    Square promotion;
    if (promotion_part) {
        promotion = promotion_part + 1;
    } else {
        promotion = NULL_PIECE;
    }

    // Create move.
    Move move(from_square, to_square, promotion);

    return MMEntry(key, raw_move, weight, learn, move);
}

// bisect_key_left: binary search for the leftmost entry with the given key.
size_t MemoryMappedReader::bisect_key_left(uint64_t key) const {
    size_t lo = 0;
    size_t hi = length();
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        // Unpack the key at mid.
        size_t offset = mid * ENTRY_SIZE;
        const char* ptr = mmap_data.data() + offset;
        uint64_t mid_key = read_uint64_be(ptr);
        if (mid_key < key) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

// __contains__ equivalent: returns true if an entry matching the given entry is found.
bool MemoryMappedReader::contains(const MMEntry &entry) const {
    // We create a vector from find_all and check for equality.
    // Using find_all overload that accepts a key (as int) in board parameter.
    for (const MMEntry &current : find_all_static(entry.key, /*minimum_weight=*/entry.weight)) {
        if (current.key == entry.key &&
            current.raw_move == entry.raw_move &&
            current.weight == entry.weight &&
            current.learn == entry.learn &&
            current.move == entry.move) {
            return true;
        }
    }
    return false;
}

// find_all: Seeks a specific position and yields corresponding entries.
// Overload when board parameter is provided as Board.
std::vector<MMEntry> MemoryMappedReader::find_all(const Board &board, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    uint64_t key = board.hash();
    const Board* context = &board;

    std::vector<MMEntry> results;
    size_t i = bisect_key_left(key);
    size_t size = length();

    while (i < size) {
        MMEntry entry = (*this)[static_cast<int>(i)];
        ++i;
        if (entry.key != key) {
            break;
        }
        if (entry.weight < minimum_weight)
            continue;

        // If context is provided, update the move using _from_chess960.
        if (context) {
            Move move = context->_from_chess960(entry.move.from_square, entry.move.to_square, entry.move.promotion);
            entry = MMEntry(entry.key, entry.raw_move, entry.weight, entry.learn, move);
        }

        // Exclude moves if necessary.
        if (!exclude_moves.empty()) {
            if (std::find(exclude_moves.begin(), exclude_moves.end(), entry.move) != exclude_moves.end())
                continue;
        }

        if (context && !context->is_legal(entry.move))
            continue;

        results.push_back(entry);
    }
    return results;
}

// find_all overload when board parameter is provided as a key integer.
std::vector<MMEntry> MemoryMappedReader::find_all(uint64_t key, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    // No context provided.
    std::vector<MMEntry> results;
    size_t i = bisect_key_left(key);
    size_t size = length();

    while (i < size) {
        MMEntry entry = (*this)[static_cast<int>(i)];
        ++i;
        if (entry.key != key) {
            break;
        }
        if (entry.weight < minimum_weight)
            continue;
        // Since no board context, skip chess960 conversion.
        if (!exclude_moves.empty()) {
            if (std::find(exclude_moves.begin(), exclude_moves.end(), entry.move) != exclude_moves.end())
                continue;
        }
        results.push_back(entry);
    }
    return results;
}

// Static helper for __contains__ implementation.
std::vector<MMEntry> MemoryMappedReader::find_all_static(uint64_t key, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    return find_all(key, minimum_weight, exclude_moves);
}

// find: Finds the main entry for the given position or Zobrist hash.
// Overload for Board.
MMEntry MemoryMappedReader::find(const Board &board, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    auto entries = find_all(board, minimum_weight, exclude_moves);
    if (entries.empty()) {
        throw std::out_of_range("IndexError");
    }
    // Find entry with the maximum weight.
    return *std::max_element(entries.begin(), entries.end(), [](const MMEntry &a, const MMEntry &b) {
        return a.weight < b.weight;
    });
}

// find: Overload for key integer.
MMEntry MemoryMappedReader::find(uint64_t key, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    auto entries = find_all(key, minimum_weight, exclude_moves);
    if (entries.empty()) {
        throw std::out_of_range("IndexError");
    }
    return *std::max_element(entries.begin(), entries.end(), [](const MMEntry &a, const MMEntry &b) {
        return a.weight < b.weight;
    });
}

// get: Returns the main entry for the given position or default.
// Overload for Board.
std::optional<MMEntry> MemoryMappedReader::get(const Board &board, const std::optional<MMEntry> &default_entry, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    try {
        return find(board, minimum_weight, exclude_moves);
    } catch (const std::out_of_range &) {
        return default_entry;
    }
}

// get: Overload for key integer.
std::optional<MMEntry> MemoryMappedReader::get(uint64_t key, const std::optional<MMEntry> &default_entry, int minimum_weight, const std::vector<Move>& exclude_moves) const {
    try {
        return find(key, minimum_weight, exclude_moves);
    } catch (const std::out_of_range &) {
        return default_entry;
    }
}

// choice: Uniformly selects a random entry for the given position.
// Overload for Board.
MMEntry MemoryMappedReader::choice(const Board &board, int minimum_weight, const std::vector<Move>& exclude_moves, std::optional<std::mt19937> random_gen) const {
    std::optional<MMEntry> chosen_entry = std::nullopt;
    auto entries = find_all(board, minimum_weight, exclude_moves);
    int i = 0;
    for (const auto &entry : entries) {
        if (!chosen_entry.has_value() || _randint(random_gen, 0, i) == i) {
            chosen_entry = entry;
        }
        ++i;
    }
    if (!chosen_entry.has_value())
        throw std::out_of_range("IndexError");
    return chosen_entry.value();
}

// choice: Overload for key integer.
MMEntry MemoryMappedReader::choice(uint64_t key, int minimum_weight, const std::vector<Move>& exclude_moves, std::optional<std::mt19937> random_gen) const {
    std::optional<MMEntry> chosen_entry = std::nullopt;
    auto entries = find_all(key, minimum_weight, exclude_moves);
    int i = 0;
    for (const auto &entry : entries) {
        if (!chosen_entry.has_value() || _randint(random_gen, 0, i) == i) {
            chosen_entry = entry;
        }
        ++i;
    }
    if (!chosen_entry.has_value())
        throw std::out_of_range("IndexError");
    return chosen_entry.value();
}

// weighted_choice: Selects a random entry distributed by the weights of the entries.
// Overload for Board.
MMEntry MemoryMappedReader::weighted_choice(const Board &board, const std::vector<Move>& exclude_moves, std::optional<std::mt19937> random_gen) const {
    auto entries = find_all(board, 1, exclude_moves);
    // Calculate total weights.
    uint32_t total_weights = 0;
    for (const auto &entry : entries) {
        total_weights += entry.weight;
    }
    if (total_weights == 0) {
        return MMEntry(); // Return an empty entry if no weights.
    }
    int choice_val = _randint(random_gen, 0, total_weights - 1);
    uint32_t current_sum = 0;
    for (const auto &entry : entries) {
        current_sum += entry.weight;
        if (current_sum > static_cast<uint32_t>(choice_val)) {
            return entry;
        }
    }
    return entries[0]; // To satisfy compiler, should never be reached.
}

// weighted_choice: Overload for key integer.
MMEntry MemoryMappedReader::weighted_choice(uint64_t key, const std::vector<Move>& exclude_moves, std::optional<std::mt19937> random_gen) const {
    auto entries = find_all(key, 1, exclude_moves);
    uint32_t total_weights = 0;
    for (const auto &entry : entries) {
        total_weights += entry.weight;
    }
    if (total_weights == 0) {
        throw std::out_of_range("IndexError");
    }
    int choice_val = _randint(random_gen, 0, total_weights - 1);
    uint32_t current_sum = 0;
    for (const auto &entry : entries) {
        current_sum += entry.weight;
        if (current_sum > static_cast<uint32_t>(choice_val)) {
            return entry;
        }
    }
    return entries[0];
}

// open_reader function: creates a MemoryMappedReader for the given file.
MemoryMappedReader open_reader(const StrOrBytesPath &path) {
    return MemoryMappedReader(path);
}
