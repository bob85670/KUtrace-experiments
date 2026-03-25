# KUtrace Experiments & Performance Engineering Showcase

This repository contains a set of C++ experiments and tools for use with KUtrace, a low-overhead tracing tool for Linux.

Beyond being a collection of tools, this repository serves as a detailed portfolio of my knowledge in low-level system performance, latency optimization, and mechanical sympathy. The experiments in the `Solution/` directory are designed to measure, analyze, and optimize software performance by understanding the behavior of the underlying hardware and software systems. Each chapter represents a deep dive into a specific concept, building a comprehensive picture of modern performance engineering. The overall report is in the `Final_report/` directory.

Some code is provided by Richard L. Sites in https://github.com/dicksites/KUtrace.

---

## Latency Table (Ch2–Ch6 + Ch7 predictions with local production setup)

| Chapter | Description | Latency number |
|---|---|---|
| Ch2 | `mystery1.cc` unoptimized (`-O0`) loop cost | **~4.3 cycles/iteration** |
| Ch2 | `mystery1.cc` optimized (`-O2`) loop removed | **~0.00 cycles/iteration** |
| Ch2 | 64-bit integer add measurement (`mystery1_64bit_int_add.cpp`, sample run) | **~0.22 cycles/iteration** (at 100 iters), **~0.00 cycles/iteration** (at 1e9 iters; “not meaningful”) |
| Ch2 | 64-bit integer multiply measurement (from `2.8.png`) | **~2.67 cycles/iteration** |
| Ch2 | 64-bit integer divide measurement (from `2.8.png`) | **~27.84 cycles/iteration** |
| Ch2 | Double add measurement (from `2.8b.png`) | **~50.00 cycles/iteration** |
| Ch2 | Double multiply measurement (from `2.8b.png`) | **~2209.40 cycles/iteration** |
| Ch2 | Double divide measurement (from `2.8b.png`) | **~86.80 cycles/iteration** |
| Ch2 | Double divide drift (from `2.9.png`) | **~13.22–14.23 cycles/iteration** |
| Ch3 | L2 access time | **10–20 cycles** |
| Ch3 | Main memory access time | **100–300 cycles** |
| Ch3 | `mystery2` stride 256 | **naive 22 cy/ld**, **linear 70 cy/ld**, **scrambled 188 cy/ld** |
| Ch3 | `mystery2` stride 128 scrambled | **158 cy/ld** |
| Ch3 | Cache-size test points (`mystery2` lgcount) | **[8] 103 cy/ld**, **[9] 83 cy/ld**, **[10] 85 cy/ld**, **[11] 69 cy/ld** |
| Ch3 | Average per-cache | **L1 3–4 cycles**, **L2 14–15 cycles**, **L3 50–60 cycles** |
| Ch4 | Matrix multiply times (from `4.0.png`) | **3.259 s** (SimpleMultiply), **4.451 s** (Columnwise), **1.073 s** (Transpose), **0.480 s** (TransposeFast), **0.351 s** (BlockMultiplyRemap), **0.254 s** (SimpleMultiplyOne) |
| Ch4 | BlockMultiplyRemap single-thread vs OpenMP (from `4.1.png`) | **0.346 s** (no OpenMP), **0.072 s** (OpenMP) |
| Ch5 | **SSD read** 40MB completion | **25.904 ms** |
| Ch5 | **SSD write** 40MB completion | **30.828 ms** |
| Ch5 | **SSD seek time to read soonest-delivered block** | **0.213 ms** (≈ **213 μs**) |
| Ch6 | Ping RPC (estimate vs reality, `Chapter6`) | **0.01 ms (est)**, **0.5 ms (real)** |
| Ch6 | 1MB write RPC (estimate vs reality, `Chapter6`) | **0.05 ms (est)**, **1.5 ms avg (real)** |
| Ch6 | 1MB read RPC (estimate vs reality, `Chapter6`) | **0.03 ms (est)**, **1.0 ms first (real)**, **0.3 ms avg later (real)** |
| Ch6 | Measured client4 ping average (from `6.png`) | **0.514 ms/RPC** |
| Ch6 | Measured client4 1MB write average (from `6.png`) | **1.450 ms/RPC** |
| Ch6 | Measured client4 1MB read average (from `6.png`) | **0.379 ms/RPC** |
| Ch7 (predict) | **Experiment 1 (RAM)**: 4 clients write 1MB values to RAM DB over **1 Gbit/s Ethernet** (small local switch + building uplink) | **~10–25 ms/RPC** typical, **p99 ~30–80 ms** |
| Ch7 (predict) | **Experiment 2 (disk)**: 4 clients write 1MB values to disk-backed store (SSD) | **~2–10 ms/RPC** typical, **p99 ~10–30 ms** |
| Ch7 (predict) | **Experiment 3 (disk + 8-byte response)** | **~1.5–8 ms/RPC** typical, **p99 ~8–25 ms** |
| Ch7 (predict) | **Experiment 3 + `O_DIRECT` + `O_NOATIME`** | **~2.5–15 ms/RPC** typical, **p99 ~15–50 ms** |

---

## KUtrace Usage Guide

There are two primary ways to generate traces for your applications: using the `kutrace_control` command-line tool for system-wide tracing, or by instrumenting your C++ code directly with the KUtrace API.

## Building the Code

A `Makefile` is provided to build all the experiment binaries. Simply run `make` from the root directory of the project:

```bash
make
```

This will compile all executables and place them in the `bin/` directory.

To clean up all compiled files, you can run:
```bash
make clean
```

---

## Method 1: System-Wide Tracing with `kutrace_control`

This method allows you to trace all activity on the system between two points in time. This is useful for getting a high-level overview of system performance.

1.  **Start Tracing**: Navigate to the `postproc` directory within your main KUtrace installation and run the control utility.

    ```bash
    # Path to your KUtrace installation
    $ cd /path/to/KUtrace/postproc
    $ ./kutrace_control
    ```

2.  **Select Trace Type**: At the prompt, type one of the following commands and press Enter:
    *   `go`: Starts a basic trace.
    *   `goipc`: Starts a trace that also includes instructions-per-cycle for each timespan.
    *   `gollc`: Starts a trace that also includes last-level cache misses for each timespan.
    *   `goipcllc`: Starts a trace that includes both IPC and LLC metrics.

3.  **Stop Tracing**: Run your workload. When you are ready to stop tracing, type `stop` at the `kutrace_control` prompt and press Enter.

    This will stop the trace and write a raw binary trace file with a name like `ku_20240709_152922_hostname_pid.trace`.

---

## Method 2: Instrumenting Code with the KUtrace C++ API

This method gives you fine-grained control over what you trace by adding tracing calls directly into your C++ source code. This is ideal for detailed performance analysis of specific code paths.

The `src/queuetest.cc` file provides a detailed example of this usage.

### API Usage Overview

1.  **Include the Library**: Add the KUtrace library header to your C++ file.

    ```cpp
    #include "kutrace_lib.h"
    ```

2.  **Initialize Tracing**: In your `main` function, start the tracing process. `goipc` is a good default to get instruction counts.

    ```cpp
    int main (int argc, const char** argv) {
      // Start self-tracing when the program begins
      kutrace::goipc(argv[0]);
      
      // ... your program logic ...
    }
    ```

3.  **Add Events and Markers**: Instrument your code by adding events, names, and markers at points of interest.

    ```cpp
    // Log a specific event with a type and value
    // e.g., tracking an RPC request ID
    kutrace::addevent(KUTRACE_RPCIDREQ, request_id);

    // Associate a human-readable name with an ID
    kutrace::addname(KUTRACE_METHODNAME, request_id, "MyMethod");

    // Mark a specific point in the code with a simple label
    kutrace::mark_a("ProcessingStarted");
    ```

    The marking functions (`mark_a`, `mark_b`, `mark_c`, `mark_d`) use different letters to write to different trace buffers, which can help reduce contention on multi-core systems.

4.  **Stop Tracing**: Before your program exits, stop the trace and write the data to a file.

    ```cpp
    int main (int argc, const char** argv) {
      // ... start tracing and run logic ...

      // Stop tracing and create the trace file
      char namebuf[256];
      kutrace::stop(kutrace::MakeTraceFileName("my_trace_prefix", namebuf));

      return 0;
    }
    ```

---

## Visualizing the Trace

Once you have a `.trace` file (from either method), you can generate an interactive HTML visualization.

1.  **Run the Post-Processing Script**: Use the `postproc3.sh` script (from your KUtrace installation's `postproc` directory) to process the raw trace file.

    ```bash
    # Usage: ./postproc3.sh <trace_file> <"Caption for the chart">
    $ /path/to/KUtrace/postproc/postproc3.sh ku_20240709_152922_hostname_pid.trace "My Application Trace"
    ```

2.  **View the Results**: The script will produce a `.json` file and an `.html` file. Open the `.html` file in your web browser to view the interactive trace visualization.


