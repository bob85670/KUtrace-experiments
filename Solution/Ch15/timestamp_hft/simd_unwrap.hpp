// SIMD scalar version

#pragma once
#include <cstdint>
#include <cstddef>

[[gnu::always_inline]]
inline void extend_timestamps_simd(
    const uint32_t* __restrict src,
    uint64_t*       __restrict dst,
    size_t          count,
    uint64_t        base_timestamp)
{
    uint64_t era = base_timestamp & ~0xFFFFFULL;
    uint64_t prev = base_timestamp & 0xFFFFF;

    for (size_t i = 0; i < count; ++i) {
        uint32_t t20 = src[i] & 0xFFFFF;
        uint64_t cand = era | t20;

        // Detect wrap
        if (t20 < prev) {
            if ((prev - t20) > (1ULL << 19)) {
                cand += (1ULL << 20);
                era += (1ULL << 20);
            }
        } else {
            if ((t20 - prev) > (1ULL << 19)) {
                cand -= (1ULL << 20);
                era -= (1ULL << 20);
            }
        }

        dst[i] = cand;
        prev = t20;
    }
}