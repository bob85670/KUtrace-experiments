# KUtrace Experiments & Performance Engineering Showcase

This repository contains a set of C++ experiments and tools for use with KUtrace, a low-overhead tracing tool for Linux.

Beyond being a collection of tools, this repository serves as a detailed portfolio of my knowledge in low-level system performance, latency optimization, and mechanical sympathy. The experiments in the `Solution/` directory are designed to measure, analyze, and optimize software performance by understanding the behavior of the underlying hardware and software systems. Each chapter represents a deep dive into a specific concept, building a comprehensive picture of modern performance engineering.

---

## Performance Insights & Key Concepts

This section provides a detailed, chapter-by-chapter analysis of the experiments, showcasing the core concepts and methodologies that are critical for latency-sensitive applications, such as those in quantitative finance.

### **Chapter 2: Instruction Latency & Throughput**

-   **Concept**: The foundation of performance analysis is understanding that not all CPU instructions have the same cost. While modern CPUs can execute multiple instructions per cycle, operations have different latencies and throughputs. A simple integer addition is one of the fastest operations, while a floating-point division is one of the slowest. Quantifying this difference is the first step in writing mechanically sympathetic code.

-   **Methodology**: This experiment uses a tight loop to perform a billion 64-bit integer additions, measuring the total time with a high-resolution timer. The core challenge in such a "micro-benchmark" is to measure the hardware's true performance without the compiler optimizing the measurement away. Two key techniques are used:
    1.  **High-Resolution Timers**: It uses the CPU's own Time Stamp Counter (`rdtsc`) via a `GetCycles()` function. This provides sub-nanosecond precision, which is essential for measuring operations that take only a single clock cycle.
    2.  **Defeating Compiler Optimizations**: To prevent the compiler from recognizing that the loop's result can be calculated mathematically at compile-time, the increment value is derived from a run-time source (`time(NULL)`). Furthermore, the final result of the summation is made "live" by printing it, ensuring the compiler cannot discard the entire loop as dead code.

-   **Takeaway**: This demonstrates the ability to measure and reason about performance at the most fundamental level: the single CPU instruction. It shows a disciplined approach to micro-benchmarking and an awareness of the compiler's role in performance.

### **Chapter 3: Empirical Discovery of the CPU Cache Hierarchy**

-   **Concept**: The performance of most real-world applications is dictated by memory access latency, not raw computation speed. The CPU's multi-level cache (L1, L2, L3) is a complex hierarchy designed to hide this latency. To write high-performance, cache-aware code, one must know the exact architecture (geometry) of the cache. This experiment discovers that geometry empirically.

-   **Methodology**: This experiment uses a sophisticated **pointer-chasing** technique to measure memory latency accurately. Instead of a simple array scan (which is easily defeated by hardware prefetchers), it creates a long, scrambled linked list. To find the address of the next element, the CPU *must* first complete the load of the current element, serializing the memory accesses and revealing the true load-to-use latency. The experiment then proceeds in three phases:
    1.  **Finds Cache Line Size**: It measures access times at various memory strides. Latency drops significantly and then plateaus once the stride equals or exceeds the hardware cache line size (typically 64 bytes).
    2.  **Finds Total Cache Size**: It measures the time to re-access a working set of data of increasing size. Sharp performance "cliffs" appear when the working set's size exceeds the capacity of the L1, then L2, then L3 caches, revealing their respective sizes.
    3.  **Finds Set Associativity**: In the most clever phase, it crafts a memory access pattern where multiple accesses are guaranteed to map to the *exact same cache set*. By measuring the number of accesses required to cause a performance spike, it reveals the associativity of that cache level (e.g., 8-way).

-   **Takeaway**: This demonstrates a deep, first-principles understanding of the CPU memory hierarchy. It shows the ability to design novel experiments to reverse-engineer the critical performance characteristics of the underlying hardware, a crucial skill for writing optimal, cache-friendly code.

### **Chapter 4: Data-Oriented Design & Cache-Aware Algorithms**

-   **Concept**: This chapter moves from measuring hardware to applying that knowledge to optimize a real-world algorithm: matrix multiplication. A naive implementation (`for i, for j, for k`) can have terrible cache performance due to non-contiguous memory accesses.
-   **Methodology**: The experiment implements a classic optimization technique known as **cache-blocking** or **tiling**. The algorithm first reorganizes the source matrices into small, contiguous blocks (e.g., 32x32) that are sized to fit comfortably within a CPU cache level (e.g., L1). The computationally intensive multiplication is then performed one block at a time. Because all data for a given block operation is hot in the cache, the CPU's execution units are constantly fed with data, dramatically reducing stalls and increasing throughput. The code even includes a detailed software simulation of the cache to explicitly count the reduction in cache misses, proving *why* the tiled version is faster.
-   **Takeaway**: This showcases the ability to move from theory to practice. It demonstrates the core principle of Data-Oriented Design: arranging data to suit the hardware will yield far greater performance gains than just algorithmic changes alone.

### **Chapter 5: Measuring Raw Disk I/O Performance**

-   **Concept**: Moving beyond the CPU-memory boundary to analyze the performance of persistent storage. The goal is to measure the true hardware performance of a disk by cleverly bypassing the operating system's caches, which can mask true latency.
-   **Methodology**: This experiment uses two key Linux features to achieve its goal:
    1.  **`O_DIRECT`**: This flag on the `open()` system call instructs the kernel to bypass its "page cache" and perform I/O directly between the application's memory and the storage device. This is essential for measuring the hardware, not the OS cache.
    2.  **Asynchronous I/O (`aio`)**: Using `aio_read()` and `aio_write()` allows the program to initiate a large I/O operation and get control back immediately. This enables the program to perform fine-grained measurements *while the I/O is in flight*. For reads, it polls a zeroed buffer to see when data arrives. For writes, it continuously "paints" the source buffer with timestamps, then reads the file back to see which timestamp was physically persisted for each block.
-   **Takeaway**: This demonstrates a sophisticated understanding of the Linux I/O stack and the ability to design experiments that disentangle software latency (OS caching) from hardware latency (raw device speed). This is a critical skill for building reliable, high-throughput persistent systems like databases or message queues.

### **Chapter 6: End-to-End RPC Latency Analysis**

-   **Concept**: Building and analyzing a complete, networked client-server system. The goal is to deconstruct the total latency of a Remote Procedure Call (RPC) into its constituent parts, from the client application, across the network, to the server, and back.
-   **Methodology**: A multi-threaded, key-value store server is built using the standard Berkeley sockets API and `pthreads`. A holistic performance analysis is then performed using a combination of tools:
    -   **Application Tracing (`KUtrace`)**: To precisely measure time spent in the client and server user-space code. Commented-out trace points show exactly where to instrument the code to capture request reception, processing, and response transmission times.
    -   **Packet Capture (`Wireshark`)**: To analyze the underlying network protocol, measuring network transit times and the overhead of TCP itself.
-   **Takeaway**: This shows the ability to build and diagnose a distributed system, pinpointing sources of latency across machine boundaries and through multiple software layers (application, kernel, network).

### **Chapter 7: Systematic Analysis & Debugging of a Distributed System**

-   **Concept**: This chapter is a case study in the real-world performance engineering lifecycle: stress-testing, debugging, and analyzing a distributed system under realistic conditions.
-   **Methodology**: It takes the RPC system from Chapter 6 and subjects it to rigorous testing. The key skill demonstrated is **multi-source trace alignment**: creating a single, coherent timeline from the clock-independent traces of the client and server. This is essential for accurately understanding distributed transactions. The chapter also documents the pragmatic debugging of real-world issues like firewalls and cross-platform compatibility.
-   **Takeaway**: Proves the ability to complete the entire performance engineering lifecycle. It shows an ability to design and execute repeatable tests, diagnose complex cross-machine issues, and perform a sophisticated, multi-source analysis to understand system behavior under load.

### **Chapter 10: Probabilistic Performance Analysis & Tail Latency**

-   **Concept**: This chapter moves beyond deterministic measurement to the statistical modeling of system behavior, with a focus on rare, high-latency "tail" events, which are often more important than average-case performance.
-   **Methodology**: It applies **queueing theory** and the **Poisson distribution** to model the random arrival of requests at a server. It calculates the probability of a "request bunching" event (multiple requests arriving in a short window) that could overwhelm a serialized resource (like a disk) and lead to requests missing their deadlines.
-   **Takeaway**: This demonstrates the ability to apply rigorous mathematical models to reason about and predict system performance. It shows an understanding that for latency-critical systems, designing for the worst case (tail latency) is paramount. This is a highly sophisticated skill directly applicable to risk management in HFT and other domains.

### **Chapter 11: System Call Tracing & Profiling with Standard Linux Tools**

-   **Concept**: This chapter demonstrates proficiency with the industry-standard Linux performance analysis toolkit, showing an ability to look "under the hood" of a running program.
-   **Methodology**: The experiment uses two of the most critical Linux tools:
    -   **`perf`**: The de facto low-overhead statistical profiler, used to identify which functions are consuming the most CPU time ("hotspots").
    -   **`strace`**: A powerful tracing tool that intercepts and logs every system call a program makes to the kernel, revealing its I/O patterns and interaction with the OS.
    The experiment uses these tools to perform an **overhead analysis**, measuring the performance impact of adding instrumentation code to a benchmark.
-   **Takeaway**: Shows fluency in the broader ecosystem of Linux performance tools and a disciplined approach to measuring the cost of observability.

### **Chapter 15: Kernel Bypass Networking & Nanosecond-Scale Timestamping**

-   **Concept**: This is a demonstration of the techniques used at the bleeding edge of HFT to achieve the absolute minimum network latency, where the OS itself is treated as a performance bottleneck.
-   **Methodology**: The experiment builds an ultra-low-latency timestamping engine using a suite of highly specialized, HFT-grade tools:
    -   **DPDK (Data Plane Development Kit)**: A library to **bypass the kernel networking stack entirely** and allow a user-space application to read packets directly from the NIC hardware.
    -   **Lock-Free SPSC Ring Buffers**: A wait-free, single-producer, single-consumer queue for the most efficient possible data handoff between a dedicated network-receiving thread and a worker thread.
    -   **SIMD (AVX) Intrinsics**: CPU vector instructions to perform high-throughput computations on the received data.
-   **Takeaway**: This is the culmination of the hardware-focused chapters, demonstrating mastery of the specific tools and architectures required to operate in the nanosecond-latency regime. It shows an expert-level understanding that for ultimate performance, one must program directly against the hardware.

### **Chapter 23: Linux Kernel Scheduler Internals**

-   **Concept**: A theoretical exploration of the heart of the operating system: the Linux Kernel Scheduler. It analyzes how the OS decides which thread runs on which CPU core, for how long, and when to move it.
-   **Analysis**: The notes deconstruct the principles behind Linux's **Completely Fair Scheduler (CFS)**. It discusses key data structures (`task_struct`), CPU affinity, and the use of "virtual runtime" to ensure fairness. Most importantly, it identifies the fundamental trade-off the scheduler must constantly manage: **load balancing** (keeping all cores busy) vs. **cache locality** (keeping a task on one core where its data is hot in the cache).
-   **Takeaway**: This demonstrates a foundational understanding of how the OS manages CPU resources. This knowledge is a prerequisite for advanced performance tuning and for correctly interpreting the results of any profiling tool.

### **Chapter 24: Linux Page Cache & Read-Ahead Optimization**

-   **Concept**: An analysis of a key kernel I/O optimization: the speculative "read-ahead" algorithm.
-   **Analysis**: When the kernel detects a sequential file access pattern, it prefetches subsequent data into the page cache before the application requests it. The notes explore the performance impact of tuning the size of this read-ahead window, calculating the reduction in total I/O operations from using a larger prefetch size. It correctly connects this OS-level tuning to the physical block size of modern SSDs, but also notes the downsides of overly aggressive prefetching (cache pollution, wasted RAM).
-   **Takeaway**: This shows an understanding of the dynamic, predictive optimizations within the Linux I/O stack and the ability to reason about the interaction between OS tuning and underlying storage hardware.

### **Chapter 25: Kernel-Level Root Cause Analysis**

-   **Concept**: A case study in diagnosing a performance anomaly ("glitch") by analyzing a trace of kernel-level events across multiple cores.
-   **Analysis**: By examining a KUtrace visualization at a microsecond level, a mysterious I/O slowdown is root-caused. The cause is not a bug, but a **kernel-level interrupt storm**: a system-wide timer interrupt, delivered via an Inter-Processor Interrupt (IPI), coincides with the SSD's hardware interrupt on the same core. The CPU becomes contended servicing both interrupts and their deferred work (RCU processing), elongating the I/O latency.
-   **Takeaway**: This demonstrates an expert-level ability to interpret complex trace data and debug subtle, OS-induced jitter. It shows a deep knowledge of kernel internals like interrupt handling, RCU, and scheduling.

### **Chapter 29: Application-Level Load Balancing & Contention**

-   **Concept**: A case study analyzing a classic problem in parallel computing: load imbalance in a multi-queue, multi-threaded application.
-   **Analysis**: A load test against a queueing simulation reveals that a simple static partitioning of work (one queue per thread) leads to a systemic bottleneck. One thread becomes overwhelmed while others sit idle, artificially limiting system throughput. The analysis points toward the need for more dynamic solutions like **work-stealing**, where idle threads can proactively take work from busy threads.
-   **Takeaway**: This highlights an architectural understanding of parallel systems. It shows the ability to diagnose systemic bottlenecks and reason about the advanced scheduling algorithms required to achieve optimal scalability.

---

## KUtrace Usage Guide

### Building the Code
A `Makefile` is provided. Simply run `make` to build all executables into the `bin/` directory.

### Generating Traces
-   **Method 1 (System-Wide)**: Use the `kutrace_control` utility from your KUtrace installation to start (`goipc`) and `stop` a system-wide trace.
-   **Method 2 (In-Code)**: `#include "kutrace_lib.h"` and use `kutrace::goipc()`, `kutrace::addevent()`, and `kutrace::stop()` to instrument your C++ code directly.

### Visualizing Traces
Use the `postproc3.sh` script from your KUtrace installation to turn any `.trace` file into an interactive HTML visualization.
