// SIMD vectorized version (4x timestamps per cycle)

#pragma once
#include <immintrin.h>
#include <cstdint>
#include <cstddef>

[[gnu::always_inline]]
inline void extend_timestamps_simd(
    const uint32_t* __restrict src,      // 20-bit packed values
    uint64_t*       __restrict dst,      // full 64-bit output
    size_t          count,               // must be multiple of 4
    uint64_t        base_timestamp)      // reference point
{
    // Upper 44 bits (the "era")
    const uint64_t era_init = base_timestamp & ~0xFFFFFULL;
    const uint64_t half_cycle = 1ULL << 19;          // 524 288
    const uint64_t full_cycle = 1ULL << 20;          // 1 048 576

    // Broadcast constants
    const __m256i mask20   = _mm256_set1_epi32(0xFFFFF);
    const __m256i half     = _mm256_set1_epi64x(half_cycle);
    const __m256i full     = _mm256_set1_epi64x(full_cycle);

    __m256i v_era   = _mm256_set1_epi64x(era_init);
    __m256i v_prev  = _mm256_set1_epi64x(base_timestamp & 0xFFFFF);

    // Pre-fault first few cache lines
    __builtin_prefetch(src,  0, 3);
    __builtin_prefetch(dst, 1, 3);

    for (size_t i = 0; i < count; i += 4) {
        // ----- Load 4×uint32 -----
        __m128i src128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        __m256i v_t20  = _mm256_cvtepu32_epi64(src128);           // zero-extend to 64
        v_t20 = _mm256_and_si256(v_t20, mask20);                  // keep lower 20

        // ----- Candidate = era | t20 -----
        __m256i v_cand = _mm256_or_si256(v_era, v_t20);

        // ----- Detect wrap (vectorised) -----
        //  t20 < prev   →  sub →  sign bit set if wrap needed
        __m256i v_diff_back = _mm256_sub_epi64(v_prev, v_t20);
        __m256i v_wrap_fwd  = _mm256_and_si256(
                                 _mm256_cmpgt_epi64(v_diff_back, half),   // diff > half
                                 full);                                   // add full cycle

        //  t20 > prev   →  sub →  sign bit set if wrap backward
        __m256i v_diff_fwd  = _mm256_sub_epi64(v_t20, v_prev);
        __m256i v_wrap_bwd  = _mm256_and_si256(
                                 _mm256_cmpgt_epi64(v_diff_fwd, half),
                                 full);

        // Apply wraps
        v_cand = _mm256_add_epi64(v_cand, v_wrap_fwd);
        v_cand = _mm256_sub_epi64(v_cand, v_wrap_bwd);

        // Update era for *next* iteration (broadcast max wrap)
        __m256i v_wrap_net = _mm256_sub_epi64(v_wrap_fwd, v_wrap_bwd);
        v_era = _mm256_add_epi64(v_era, v_wrap_net);

        // Store
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), v_cand);

        // Update prev = current t20 (take highest 20-bit as representative)
        v_prev = v_t20;
        // To avoid branchy reduction we just reuse the last element
        uint64_t last_t20 = _mm256_extract_epi64(v_t20, 3);
        v_prev = _mm256_set1_epi64x(last_t20);
    }
}