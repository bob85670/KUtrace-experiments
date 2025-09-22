#include <stdint.h>
#include <stdio.h>
#include <time.h>		// time()
#include "../../timecounters.h"

static const int kIterations = 1000 * 1000000;

int main (int argc, const char** argv) { 
  uint64_t sum = 0;

  time_t t = time(NULL);	// A number that the compiler does not know
  uint64_t incr = t & 255;		// Unknown increment 0..255, now 64-bit

  int64_t startcy = GetCycles();
  for (int i = 0; i < kIterations; ++i) {
    sum += incr;
  }
  int64_t elapsed = GetCycles() - startcy;
  double felapsed = elapsed;

  fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
          kIterations, elapsed, felapsed / kIterations);
  
  // fprintf(stdout, "%lu %lu\n", t, sum);	// Make sum live

  return 0;
}
