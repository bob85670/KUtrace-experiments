// Kernel bypass (DPDK-style) version

#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "ring_spsc.hpp"
#include "simd_unwrap.hpp"

#pragma pack(push,1)
struct pkt_desc {
    uint64_t pkt_addr;   // pointer to packet data (huge page)
    uint32_t pkt_len;
    uint32_t timestamp20;   // lower 20 bits embedded by hardware
};
#pragma pack(pop)

constexpr size_t PKT_RING_SIZE = 1 << 16;   // 64K entries
constexpr size_t TS_RING_SIZE  = 1 << 18;   // 256K 64-bit timestamps

extern std::atomic<bool> shutdown_requested;

// Global rings (placed in huge pages in real code)
alignas(64) static RingBufferSPSC<pkt_desc, PKT_RING_SIZE> pkt_ring;
alignas(64) static RingBufferSPSC<uint64_t, TS_RING_SIZE>  ts_ring;

// ------------------------------------------------------------------
// Producer: NIC DMA → pkt_ring (runs in kernel-bypass poll loop)
// ------------------------------------------------------------------
[[gnu::always_inline]]
inline void receive_loop_forever(uint64_t base_timestamp)
{
    pkt_desc batch[32];
    uint64_t out_ts[32];

    // Pre-fault rings
    pkt_ring.prefault();
    ts_ring.prefault();

    while (!shutdown_requested.load(std::memory_order_acquire)) {
        // ----- Burst receive (DPDK-style) -----
        size_t nb_rx = 0;
        for (; nb_rx < 32; ++nb_rx) {
            if (!pkt_ring.pop(batch[nb_rx])) break;
        }
        if (nb_rx == 0) {
            if (shutdown_requested.load(std::memory_order_acquire)) break;
            __builtin_ia32_pause();
            continue;
        }

        // ----- Extract 20-bit timestamps -----
        for (size_t i = 0; i < nb_rx; ++i) {
            // Assume hardware writes timestamp in pkt_desc
            // (real NICs fill it via descriptor)
            ((uint32_t*)out_ts)[i] = batch[i].timestamp20;
        }

        // ----- SIMD unwrap (4-wide) -----
        size_t simd_cnt = (nb_rx + 3) & ~3ULL;
        extend_timestamps_simd((const uint32_t*)out_ts,
                               out_ts,
                               simd_cnt,
                               base_timestamp);

        // ----- Push full timestamps to consumer ring -----
        for (size_t i = 0; i < nb_rx; ++i) {
            while (!ts_ring.push(out_ts[i])) {
                if (shutdown_requested.load(std::memory_order_acquire)) goto done;
                __builtin_ia32_pause();
            }
        }
    }
done:
    std::cout << "[RECV] Shutdown signal received. Exiting.\n";
}

// ------------------------------------------------------------------
// Consumer: read full timestamps (your strategy / order book)
// ------------------------------------------------------------------
inline void consumer_example()
{
    uint64_t ts;
    while (ts_ring.pop(ts)) {
        // ts is a fully unwrapped 64-bit timestamp
        // → feed to matching engine, risk checks, etc.
        (void)ts;
    }
}