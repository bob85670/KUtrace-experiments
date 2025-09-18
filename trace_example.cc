// trace_example.cc
#include <iostream>
#include <unistd.h>  // for sleep
#include "kutrace_lib.h"  // KUtrace user-mode library header

void myFunction(int iterations) {
    kutrace::mark_a("func");  // Insert marker: start of function
    for (int i = 0; i < iterations; ++i) {
        std::cout << "Iteration " << i << std::endl;  // Triggers syscalls for tracing
    }
    kutrace::mark_a("func");  // End marker (same label for simplicity)
}

int main(int argc, char** argv) {
    char fname[256];
    kutrace::MakeTraceFileName("ku", fname);  // Generate default trace filename

    // Initialize and start tracing
    kutrace::DoReset(0);           // Reset buffer
    kutrace::DoInit(argv[0]);      // Initialize with program name
    kutrace::DoOn();               // Turn tracing on

    // Insert markers and simulate work
    kutrace::mark_a("start");        // Custom marker for start
    myFunction(5);                 // Function with loop (triggers events)
    sleep(2);                      // Allow time for kernel-user transitions
    kutrace::mark_a("end");          // Custom marker for end

    // Stop and dump trace
    kutrace::DoOff();              // Turn tracing off
    sleep(1);                      // Wait for pending traces to finish
    kutrace::DoFlush();            // Flush remaining buffer
    kutrace::DoDump(fname);        // Dump to file
    kutrace::DoQuit();             // Clean up

    std::cout << "Trace file generated: " << fname << std::endl;
    return 0;
}
