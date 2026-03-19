#include "agents.hpp"

int main() {
    printf(" \
            Size of Board: %zu bytes\n \
            Size of Color: %zu bytes\n \
            Size of Transposition Table: %zu bytes\n \
            Size of Timer: %zu bytes\n \
            Size of MemoryMappedReader: %zu bytes\n \
            Size of Killer Moves Array: %zu bytes\n \
            Size of History Table: %zu bytes\n \
            Size of std::thread: %zu bytes\n \
            Size of std::mutex: %zu bytes\n \
            Size of std::atomic<bool>: %zu bytes\n \
            Size of std::condition_variable: %zu bytes\n \
            Size of AgentProposal: %zu bytes\n \
            Size of AgentType: %zu bytes\n \
            Size of AgentProfile: %zu bytes\n \
            Size of Move: %zu bytes\n",
            sizeof(Board),
            sizeof(Color),
            sizeof(TranspositionTable),
            sizeof(Timer),
            sizeof(MemoryMappedReader),
            sizeof(Array2D<Move, MAX_DEPTH + MAX_EXTENSION, 2>),
            sizeof(Array2D<int, 64, 64>) * 2,
            sizeof(std::thread),
            sizeof(std::mutex),
            sizeof(std::atomic<bool>),
            sizeof(std::condition_variable),
            sizeof(Agents::AgentProposal),
            sizeof(Agents::AgentType),
            sizeof(Agents::AgentProfile),
            sizeof(Move));
    return 0;
}