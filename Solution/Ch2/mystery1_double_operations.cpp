#include <stdint.h>
#include <stdio.h>
#include <time.h>		// time()
#include "../../timecounters.h"

static const int kIterations = 1000000;

int main (int argc, const char** argv) { 
  double product = 1.0, quotient = 1.0;

  time_t t = time(NULL);	// A number that the compiler does not know
  volatile uint64_t incr = t & 255;		// Unknown increment 0..255, now 64-bit

  // multiple
  int64_t startcy_mul = GetCycles();
  for (int i = 0; i < kIterations; ++i) {
    product *= incr;
    if (i % 10000 == 0) {
      int64_t elapsed_mul = GetCycles() - startcy_mul;
      double felapsed_mul = elapsed_mul;

      fprintf(stdout, "multiplication: \n");
      fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
              i, elapsed_mul, felapsed_mul / kIterations);
      fprintf(stdout, "%lu %f\n", t, product);

      int64_t startcy_mul = GetCycles();
    }
  }
  int64_t elapsed_mul = GetCycles() - startcy_mul;
  
  // division
  int64_t startcy_div = GetCycles();
  for (int i = 0; i < kIterations; ++i) {
    quotient /= incr;
    if (i % 10000 == 0) {
      int64_t elapsed_div = GetCycles() - startcy_div;
      double felapsed_div = elapsed_div;

      fprintf(stdout, "division: \n");
      fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
              i, elapsed_div, felapsed_div / kIterations);
      fprintf(stdout, "%lu %f\n", t, quotient);

      int64_t startcy_div = GetCycles();
    }
  }
  int64_t elapsed_div = GetCycles() - startcy_div;
  

  return 0;
}
