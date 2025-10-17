#include <stdint.h>
#include <stdio.h>
#include <time.h>		// time()
#include "../../timecounters.h"

static const int kIterations = 1000;

int main (int argc, const char** argv) { 
  uint64_t product = 1;
  uint64_t quotient = 100000000;

  time_t t = time(NULL);	// A number that the compiler does not know
  uint64_t incr = t & 255;		// Unknown increment 0..255, now 64-bit

  int64_t startcy = GetCycles();
  for (int i = 1; i < kIterations; ++i) {
    // product *= incr;
    quotient /= incr;
  }
  int64_t elapsed = GetCycles() - startcy;
  double felapsed = elapsed;

  // fprintf(stdout, "multiplication: \n");
  // fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
  //         kIterations, elapsed, felapsed / kIterations);
  
  // fprintf(stdout, "%lu %lu\n", t, product);

  fprintf(stdout, "division: \n");
  fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
          kIterations, elapsed, felapsed / kIterations);
  
  fprintf(stdout, "%lu %lu\n", t, quotient);

  return 0;
}

// To prevent extremes of overflow and underflow
// #include <stdint.h>
// #include <stdio.h>
// #include <time.h>   // time()
// #include "../../timecounters.h"

// static const int kIterations = 10; // smaller values

// int main (int argc, const char** argv) { 
//   double sum = 0.0, product = 0.0001, quotient = 100000000.0;

//   time_t t = time(NULL);
//   volatile double incr = (double)(t & 100);   // a smaller incr to prevent product for exponential growth, and cast to double to ensure fp arithmetics

//   // sum
//   int64_t startcy_add = GetCycles();
//   for (int i = 0; i < kIterations; ++i) {
//     sum += incr;
//   }
//   int64_t elapsed_sum = GetCycles() - startcy_add;
//   double felapsed_sum = elapsed_sum;

//   // multiple
//   int64_t startcy_mul = GetCycles();
//   for (int i = 1; i < kIterations; ++i) {
//     product *= incr;
//   }
//   int64_t elapsed_mul = GetCycles() - startcy_mul;
//   double felapsed_mul = elapsed_mul;

//   // division
//   int64_t startcy_div = GetCycles();
//   for (int i = 0; i < kIterations; ++i) {
//     quotient /= incr;
//   }
//   int64_t elapsed_div = GetCycles() - startcy_div;
//   double felapsed_div = elapsed_div;

//   fprintf(stdout, "addition: \n");
//   fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
//           kIterations, elapsed_sum, felapsed_sum / kIterations);
//   fprintf(stdout, "%lu %f\n", t, sum);

//   fprintf(stdout, "multiplication: \n");
//   fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
//           kIterations, elapsed_mul, felapsed_mul / kIterations);
//   fprintf(stdout, "%lu %.10f\n", t, product);

//   fprintf(stdout, "division: \n");
//   fprintf(stdout, "%d iterations, %lu cycles, %4.2f cycles/iteration\n", 
//           kIterations, elapsed_div, felapsed_div / kIterations);
//   fprintf(stdout, "%lu %.20f\n", t, quotient); // increased precision

//   return 0;
// }
