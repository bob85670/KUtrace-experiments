/*
// g++ -std=c++17 -O3 -march=native -mtune=native -fno-exceptions -fno-rtti \
    timestamps_extend.cpp -o timestamps_extend

*/

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <iomanip>

// Cache line size (modern x86-64)
static constexpr size_t CACHE_LINE = 64;

// Page size (4KB typical)
static constexpr size_t PAGE_SIZE = 4096;

// Pre-fault a memory region to avoid TLB misses
inline void prefault_pages(void* addr, size_t size) {
    const char* ptr = (const char*)addr;
    const char* end = ptr + size;
    // Touch one byte per page
    for (; ptr < end; ptr += PAGE_SIZE) {
        // Volatile to prevent optimization
        volatile char touch = *ptr;
        (void)touch;
    }
}

// HFT-optimized: extend 20-bit wrapped timestamps
[[gnu::always_inline]]
inline void extend_timestamps_hft(
    const uint32_t* __restrict__ timestamps_20bit,   // input
    uint64_t*       __restrict__ out_timestamps,     // output
    size_t count,
    uint64_t base_timestamp
) {
    // Extract upper 44 bits (era)
    uint64_t era = base_timestamp & ~0xFFFFFULL;
    uint64_t prev_lower = base_timestamp & 0xFFFFF;

    // Prefetch first few cache lines
    __builtin_prefetch(timestamps_20bit, 0, 3);  // read, high temporal
    __builtin_prefetch(out_timestamps, 1, 3);    // write

    for (size_t i = 0; i < count; ++i) {
        // Prefetch next inputs (2 cache lines ahead)
        if (i + 8 < count) {
            __builtin_prefetch(&timestamps_20bit[i + 8], 0, 1);
            __builtin_prefetch(&out_timestamps[i + 8], 1, 1);
        }

        uint32_t t20 = timestamps_20bit[i] & 0xFFFFF;

        uint64_t candidate = era | t20;

        // --- Branch prediction: expect forward in time ---
        // Use likelihood hints (GCC/Clang)
        if (__builtin_expect(t20 < prev_lower, 0)) {  // rare: possible wrap
            if ((prev_lower - t20) > (1ULL << 19)) {
                candidate += (1ULL << 20);
                era += (1ULL << 20);
            }
        } else if (__builtin_expect(t20 > prev_lower, 0)) {
            if ((t20 - prev_lower) > (1ULL << 19)) {
                candidate -= (1ULL << 20);
                era -= (1ULL << 20);
            }
        }

        out_timestamps[i] = candidate;
        prev_lower = t20;
    }
}

// Aligned buffers
alignas(CACHE_LINE) static uint32_t input_buffer[1024];
alignas(CACHE_LINE) static uint64_t output_buffer[1024];

int main() {
    // Example 2: With wraparound
    uint64_t base = 0x00000000001FF000ULL;

    std::vector<uint32_t> input = {
        0x1FF000, 0x000010, 0x000020, 0x100005
    };

    // Copy to aligned buffer
    size_t n = input.size();
    std::memcpy(input_buffer, input.data(), n * sizeof(uint32_t));

    // Pre-fault input/output pages
    prefault_pages(input_buffer, n * sizeof(uint32_t));
    prefault_pages(output_buffer, n * sizeof(uint64_t));

    // Run HFT version
    extend_timestamps_hft(input_buffer, output_buffer, n, base);

    // Print results
    std::cout << "HFT-Optimized Output:\n";
    for (size_t i = 0; i < n; ++i) {
        std::cout << "  [" << i << "] 0x" 
                  << std::hex << std::setw(16) << std::setfill('0') 
                  << output_buffer[i] << std::dec << "\n";
    }

    return 0;
}


/*
unused technique:
// Unroll inner loop by 4 for better ILP (Instruction-Level Parallelism)
size_t i = 0;
for (; i + 3 < count; i += 4) {
    // Load 4x 20-bit deltas (masked)
    uint64_t d0 = deltas_20bit[i + 0] & 0xFFFFF;
    uint64_t d1 = deltas_20bit[i + 1] & 0xFFFFF;
    uint64_t d2 = deltas_20bit[i + 2] & 0xFFFFF;
    uint64_t d3 = deltas_20bit[i + 3] & 0xFFFFF;

    current += d0; out_timestamps[i + 0] = current;
    current += d1; out_timestamps[i + 1] = current;
    current += d2; out_timestamps[i + 2] = current;
    current += d3; out_timestamps[i + 3] = current;
}
*/