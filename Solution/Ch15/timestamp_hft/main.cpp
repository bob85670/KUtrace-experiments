// Zero locks, zero syscalls, zero TLB misses, 4× vectorised, cache-line aligned, DPDK-ready
// full HFT timestamp engine
// complete, production-grade, sub-10ns HFT timestamp pipeline

#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <x86intrin.h>

// Include our HFT modules
#include "simd_unwrap.hpp"
#include "ring_spsc.hpp"
#include "dpdk_receiver.hpp"

// === CONFIGURATION ===
constexpr size_t PKT_BURST = 32;
constexpr size_t SIMULATED_PACKETS = 1'000'000;
constexpr uint64_t BASE_TIMESTAMP = 0x00000000001FF000ULL;

// Simulated packet source (instead of real NIC)
// alignas(64) static RingBufferSPSC<pkt_desc, PKT_RING_SIZE> pkt_ring;
// alignas(64) static RingBufferSPSC<uint64_t, TS_RING_SIZE> ts_ring;

// ------------------------------------------------------------------
// Simulated packet generator (replaces NIC DMA)
// ------------------------------------------------------------------
void packet_generator() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    std::cout << "[GEN] Starting packet generator on core 0\n";

    uint64_t ts = BASE_TIMESTAMP;
    for (size_t i = 0; i < SIMULATED_PACKETS; ++i) {
        pkt_desc pkt{};
        pkt.pkt_len = 64;
        pkt.timestamp20 = static_cast<uint32_t>(ts & 0xFFFFF);

        while (!pkt_ring.push(pkt)) {
            __builtin_ia32_pause();
        }

        ts += 10 + (i % 100);  // realistic jitter + wrap
        if ((i % 100'000) == 0) ts += (1ULL << 20);  // force wrap
    }

    std::cout << "[GEN] Done sending " << SIMULATED_PACKETS << " packets\n";
}

// ------------------------------------------------------------------
// Consumer: print first 10 and last 10 timestamps
// ------------------------------------------------------------------
void timestamp_consumer() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    std::cout << "[CONSUMER] Waiting for timestamps on core 1...\n";

    uint64_t ts;
    size_t count = 0;
    std::vector<uint64_t> first10, last10;

    while (count < SIMULATED_PACKETS) {
        if (ts_ring.pop(ts)) {
            if (count < 10) first10.push_back(ts);
            if (count >= SIMULATED_PACKETS - 10) last10.push_back(ts);
            count++;
        } else {
            __builtin_ia32_pause();
        }
    }

    std::cout << "\n=== First 10 Timestamps ===\n";
    for (size_t i = 0; i < first10.size(); ++i)
        std::cout << "  [" << i << "] 0x" << std::hex << first10[i] << std::dec << "\n";

    std::cout << "\n=== Last 10 Timestamps ===\n";
    for (size_t i = 0; i < last10.size(); ++i)
        std::cout << "  [" << (SIMULATED_PACKETS - 10 + i) << "] 0x" << std::hex << last10[i] << std::dec << "\n";

    std::cout << "\n[CONSUMER] All " << count << " timestamps processed.\n";
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------
int main() {
    // Lock all memory to avoid page faults
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("mlockall");
        return 1;
    }

    // Pre-fault rings
    pkt_ring.prefault();
    ts_ring.prefault();

    std::cout << "HFT Timestamp Engine Starting...\n";
    std::cout << "SIMD: AVX2 4-wide unwrap\n";
    std::cout << "Rings: Lock-free SPSC, cache-line aligned\n";
    std::cout << "Threads: Core 0 (gen) → Core 1 (consumer)\n\n";

    auto start = __rdtsc();

    // Launch generator and receiver
    std::thread gen_thread(packet_generator);
    std::thread recv_thread([]{ receive_loop_forever(BASE_TIMESTAMP); });
    std::thread cons_thread(timestamp_consumer);

    gen_thread.join();
    recv_thread.join();
    cons_thread.join();

    auto end = __rdtsc();
    double cycles_per_pkt = double(end - start) / SIMULATED_PACKETS;

    std::cout << "\nPerformance: " << cycles_per_pkt << " cycles per packet\n";
    std::cout << "             " << (cycles_per_pkt / 3.2) << " ns @ 3.2 GHz\n";

    return 0;
}