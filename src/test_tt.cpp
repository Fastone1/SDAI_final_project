#include "tt.hpp"
#include <iostream>

int main() {
    // Create a new instance of the TT class
    std::cout << sizeof(TTEntry) << " bytes" << std::endl;

    TTEntry entry;
    entry.zobrist_hash = 123456789;
    entry.data = packTTData(10, -500, Move(Square(0), Square(1)), NodeType::EXACT);
    Bitboard hash = entry.zobrist_hash;
    std::cout << "Zobrist Hash: " << hash << std::endl;
    std::cout << "Packed Data: " << entry.data << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>(entry.data) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<32>(-500) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(10) & 0xFF)) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(-500) & 0xFFFF) << 8) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(Move(Square(0), Square(1)).from_square) << 24)) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(Move(Square(0), Square(1)).to_square) << 32)) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(Move(Square(0), Square(1)).promotion) << 40)) << std::endl;
    std::cout << "Paced Data Binary: " << std::bitset<64>((static_cast<uint64_t>(NodeType::EXACT) << 48)) << std::endl;
    Move move;
    NodeType type;
    int depth, score;
    extractMoveAndType(entry.data, depth, score, move, type);
    std::cout << "Extracted Move: " << move.from_square << " to " << move.to_square << ", Promotion: " << move.promotion << std::endl;
    std::cout << "Depth: " << depth << ", Score: " << score << ", Type: " << static_cast<int>(type) << std::endl;

    return 0;
}