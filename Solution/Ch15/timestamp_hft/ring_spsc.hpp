// Lock-free ring buffer integration

#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <algorithm>

constexpr size_t CACHE_LINE = 64;

template<typename T, size_t N>
class RingBufferSPSC {
    static_assert((N & (N-1)) == 0, "N must be power of 2");

    alignas(CACHE_LINE) std::atomic<size_t> write_idx{0};
    alignas(CACHE_LINE) std::atomic<size_t> read_idx{0};
    alignas(CACHE_LINE) T buffer[N]{};

public:
    [[nodiscard]] bool push(const T& item) noexcept {
        size_t w = write_idx.load(std::memory_order_relaxed);
        size_t r = read_idx.load(std::memory_order_acquire);
        if ((w - r) == N) return false;                 // full

        buffer[w & (N-1)] = item;
        write_idx.store(w + 1, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool pop(T& out) noexcept {
        size_t r = read_idx.load(std::memory_order_relaxed);
        size_t w = write_idx.load(std::memory_order_acquire);
        if (r == w) return false;                       // empty

        out = buffer[r & (N-1)];
        read_idx.store(r + 1, std::memory_order_release);
        return true;
    }

    // Bulk push, returns number of items pushed
    [[nodiscard]] size_t push_bulk(const T* items, size_t n) noexcept {
        size_t w = write_idx.load(std::memory_order_relaxed);
        size_t r = read_idx.load(std::memory_order_acquire);
        
        size_t free_space = N - (w - r);
        if (free_space == 0) return 0;

        size_t count = std::min(free_space, n);

        // This could be optimized with two memcpys to handle wrap-around
        for (size_t i = 0; i < count; ++i) {
            buffer[(w + i) & (N - 1)] = items[i];
        }

        write_idx.store(w + count, std::memory_order_release);
        return count;
    }

    // Bulk pop, returns number of items popped
    [[nodiscard]] size_t pop_bulk(T* out, size_t n) noexcept {
        size_t r = read_idx.load(std::memory_order_relaxed);
        size_t w = write_idx.load(std::memory_order_acquire);
        
        size_t avail = w - r;
        if (avail == 0) return 0;

        size_t count = std::min(avail, n);
        
        // This could be optimized with two memcpys to handle wrap-around
        for(size_t i = 0; i < count; ++i) {
            out[i] = buffer[(r + i) & (N - 1)];
        }

        read_idx.store(r + count, std::memory_order_release);
        return count;
    }

    // Pre-fault whole ring (call once at start)
    void prefault() const {
        for (size_t i = 0; i < N; ++i) {
            volatile T touch = buffer[i];
            (void)touch;
        }
    }
};