// g++ -std=c++17 -O2 -Wall timestamps.cpp -o timestamps

#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>

// Extend 20-bit wrapped timestamps to full 64-bit using a base reference
void extend_timestamps(
    const std::vector<uint32_t>& timestamps_20bit,  // input: 20-bit values
    std::vector<uint64_t>& out_timestamps,          // output: full 64-bit
    uint64_t base_timestamp                         // known full timestamp
) {
    uint64_t era = base_timestamp & ~0xFFFFFULL;  // upper 44 bits
    uint64_t prev_lower = base_timestamp & 0xFFFFF;

    for (size_t i = 0; i < timestamps_20bit.size(); ++i) {
        uint32_t t20 = timestamps_20bit[i] & 0xFFFFF;  // extract 20 bits

        // Start with current era + this 20-bit value
        uint64_t candidate = era | t20;

        // Heuristic: if big jump backward → wrapped forward
        if (t20 < prev_lower && (prev_lower - t20) > (1ULL << 19)) {
            candidate += (1ULL << 20);
            era += (1ULL << 20);  // advance era for next
        }
        // If big jump forward → wrapped backward (rare in sorted data)
        else if (t20 > prev_lower && (t20 - prev_lower) > (1ULL << 19)) {
            candidate -= (1ULL << 20);
            era -= (1ULL << 20);
        }

        out_timestamps[i] = candidate;
        prev_lower = t20;  // update for next comparison
    }
}

// Helper to print results
void print_example(const char* title,
                   uint64_t base,
                   const std::vector<uint32_t>& input_20bit,
                   const std::vector<uint64_t>& expected_64bit) {
    std::cout << "=== " << title << " ===\n";
    std::cout << "Base (64-bit): 0x" << std::hex << std::setw(16) << std::setfill('0') << base << std::dec << "\n";

    std::vector<uint64_t> output(input_20bit.size());
    extend_timestamps(input_20bit, output, base);

    std::cout << "Input 20-bit: ";
    for (auto v : input_20bit) std::cout << "0x" << std::hex << std::setw(5) << std::setfill('0') << v << ' ';
    std::cout << std::dec << "\n";

    std::cout << "Output 64-bit:\n";
    for (size_t i = 0; i < output.size(); ++i) {
        std::cout << "  [" << i << "] 0x" << std::hex << std::setw(16) << std::setfill('0') << output[i]
                  << "  (expected: 0x" << std::setw(16) << std::setfill('0') << expected_64bit[i] << ")"
                  << std::dec << "\n";
    }
    std::cout << "\n";
}

int main() {
    // ------------------------------------------------------------------
    // Example 1: Small values, no wraparound
    // ------------------------------------------------------------------
    uint64_t base1 = 0x0000000000100000ULL;  // ~1,048,576
    std::vector<uint32_t> input1 = {
        0x100010,  // lower 20: 0x00010
        0x100020,  // 0x00020
        0x100030   // 0x00030
    };
    std::vector<uint64_t> expected1 = {
        0x0000000000100010ULL,
        0x0000000000100020ULL,
        0x0000000000100030ULL
    };

    // ------------------------------------------------------------------
    // Example 2: Wraparound! Crosses 2^20 boundary
    // ------------------------------------------------------------------
    uint64_t base2 = 0x00000000001FF000ULL;  // just below wrap
    std::vector<uint32_t> input2 = {
        0x1FF000,  // 0xFF000 → near end
        0x000010,  // 0x00010 → wrapped!
        0x000020,  // 0x00020
        0x100005   // 0x00005 → next cycle
    };
    std::vector<uint64_t> expected2 = {
        0x00000000001FF000ULL,
        0x0000000000200010ULL,  // +1<<20
        0x0000000000200020ULL,
        0x0000000000210005ULL   // +2<<20
    };

    print_example("Example 1: No Wraparound", base1, input1, expected1);
    print_example("Example 2: With Wraparound", base2, input2, expected2);

    return 0;
}