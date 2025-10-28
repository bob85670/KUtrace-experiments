// SIMD scalar version

#pragma once
#include <cstdint>
#include <cstddef>

// Unwrapper state must be maintained across calls.
// last_full_ts holds the last successfully unwrapped 64-bit timestamp.
[[gnu::always_inline]]
inline void extend_timestamps_simd(
    const uint32_t* __restrict src,
    uint64_t*       __restrict dst,
    size_t          count,
    uint64_t&       last_full_ts) // State is passed by reference
{
    uint64_t era = last_full_ts & ~0xFFFFFULL;
    uint32_t prev_t20 = last_full_ts & 0xFFFFF;

    for (size_t i = 0; i < count; ++i) {
        uint32_t t20 = src[i] & 0xFFFFF;

        // The original logic for wrap detection was sound, but it was applied to a stateless 'era'.
        // We need to detect the wrap relative to the last known timestamp and update the 'era'
        // for the current batch.
        if (t20 < prev_t20) { // Potential forward wrap
            if ((prev_t20 - t20) > (1 << 19)) { // It is a forward wrap
                era += (1ULL << 20);
            }
        } else { // Potential backward wrap
            if ((t20 - prev_t20) > (1 << 19)) { // It is a backward wrap
                era -= (1ULL << 20);
            }
        }
        
        uint64_t cand = era | t20;
        dst[i] = cand;
        prev_t20 = t20; // Update prev_t20 for the next item in the batch
    }

    if (count > 0) {
        // Update the state for the next call with the last timestamp from this batch
        last_full_ts = dst[count - 1];
    }
}