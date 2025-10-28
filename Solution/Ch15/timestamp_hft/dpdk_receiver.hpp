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
constexpr size_t PKT_BURST     = 32;        // Max packets in a burst

extern std::atomic<bool> shutdown_requested;

// Global rings (placed in huge pages in real code)
alignas(64) static RingBufferSPSC<pkt_desc, PKT_RING_SIZE> pkt_ring;
alignas(64) static RingBufferSPSC<uint64_t, TS_RING_SIZE>  ts_ring;

// ------------------------------------------------------------------
// Producer: NIC DMA → pkt_ring (runs in kernel-bypass poll loop)
// ------------------------------------------------------------------
[[gnu::always_inline]]
inline void receive_loop_forever(uint64_t initial_timestamp)
{
    // Buffers on stack
    pkt_desc batch[PKT_BURST];
    uint32_t in_ts20[PKT_BURST];
    uint64_t out_ts64[PKT_BURST];

    uint64_t last_full_ts = initial_timestamp; // State for the unwrapper

    // Pre-fault rings (optional, good practice)
    pkt_ring.prefault();
    ts_ring.prefault();

    while (!shutdown_requested.load(std::memory_order_acquire)) {
        // ----- Burst receive (DPDK-style) -----
        size_t nb_rx = pkt_ring.pop_bulk(batch, PKT_BURST);
        if (nb_rx == 0) {
            __builtin_ia32_pause();
            continue;
        }

        // ----- Extract 20-bit timestamps -----
        for (size_t i = 0; i < nb_rx; ++i) {
            in_ts20[i] = batch[i].timestamp20;
        }

        // ----- SIMD unwrap -----
        extend_timestamps_simd(in_ts20, out_ts64, nb_rx, last_full_ts);

        // ----- Push full timestamps to consumer ring -----
        // ----- Push full timestamps to consumer ring -----
        size_t pushed = 0;
        while (pushed < nb_rx) {
            size_t n_pushed = ts_ring.push_bulk(out_ts64 + pushed, nb_rx - pushed);
            if (n_pushed == 0) {
                 if (shutdown_requested.load(std::memory_order_acquire)) goto done;
                __builtin_ia32_pause();
                continue;
            }
            pushed += n_pushed;
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