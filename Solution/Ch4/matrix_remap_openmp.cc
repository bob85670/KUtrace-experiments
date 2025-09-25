#include <stdio.h>
#include <string.h>
#include <sys/time.h>	// gettimeofday
#include <omp.h>
#include "../../basetypes.h"
#include "../../kutrace_lib.h"
#include "../../timecounters.h"

// g++ -O2 -fopenmp matrix_remap.cc ../../kutrace_lib.cc -o matrix_remap

#define TRACK_CACHES 0
#define HASHED_L3 0

static const int kRowsize = 1024;
static const int kColsize = kRowsize;
static const int kBlocksize = 8;

static const int kRemapsize = 32;
//static const int kRemapsize = 16;
//static const int kRemapsize = 32;
//static const int kRemapsize = 64;

////typedef unsigned long int uint64;
typedef void MulProc(const double* a, const double* b, double* c);

static double* aa = NULL;
static double* bb = NULL;
static double* cc = NULL;

static const int kL1LgSize = 15;
static const int kL1LgAssoc = 3;
static const int kL1LgLinesize = 6;
static const int kL1LgSetsize = kL1LgSize - kL1LgAssoc - kL1LgLinesize;
static const int kL1Setsize = 1 << kL1LgSetsize;
static const int kL1Assoc = 1 << kL1LgAssoc;
static const int kL1Assocmask = kL1Assoc - 1;
static const uint64 kL1Setmask = (1l << kL1LgSetsize) - 1;
static const uint64 kL1Tagmask = (1l << kL1LgLinesize) - 1;

static const int kL2LgSize = 18;
static const int kL2LgAssoc = 3;
static const int kL2LgLinesize = 6;
static const int kL2LgSetsize = kL2LgSize - kL2LgAssoc - kL2LgLinesize;
static const int kL2Setsize = 1 << kL2LgSetsize;
static const int kL2Assoc = 1 << kL2LgAssoc;
static const int kL2Assocmask = kL2Assoc - 1;
static const uint64 kL2Setmask = (1l << kL2LgSetsize) - 1;
static const uint64 kL2Tagmask = (1l << kL2LgLinesize) - 1;


static const int kL3LgSize = 21;
static const int kL3LgAssoc = 4;
static const int kL3LgLinesize = 6;
static const int kL3LgSetsize = kL3LgSize - kL3LgAssoc - kL3LgLinesize;
static const int kL3Setsize = 1 << kL3LgSetsize;
static const int kL3Assoc = 1 << kL3LgAssoc;
static const int kL3Assocmask = kL3Assoc - 1;
static const uint64 kL3Setmask = (1l << kL3LgSetsize) - 1;
static const uint64 kL3Tagmask = (1l << kL3LgLinesize) - 1;

static uint64 L1misses = 0;
static uint64 L2misses = 0;
static uint64 L3misses = 0;
static int L1rr = 0;
static int L2rr = 0;
static int L3rr = 0;

static uint64 L1tag[kL1Setsize * kL1Assoc];
static uint64 L2tag[kL2Setsize * kL2Assoc];
static uint64 L3tag[kL3Setsize * kL3Assoc];


void InitTags() {
  memset(L1tag, 0, kL1Setsize * kL1Assoc * sizeof(uint64));
  memset(L2tag, 0, kL2Setsize * kL2Assoc * sizeof(uint64));
  memset(L3tag, 0, kL3Setsize * kL3Assoc * sizeof(uint64));
  L1misses = L2misses = L3misses = 0;
}

#if TRACK_CACHES
bool L1(uint64 addr) {
  int set = ((addr >> kL1LgLinesize) & kL1Setmask) << kL1LgAssoc;
  uint64 tag = addr & ~kL1Tagmask;
  for (int i = 0; i < kL1Assoc; ++i) {
    if (L1tag[set + i] == tag) {return true;}
  }
  ++L1misses; 
  L1tag[set + L1rr] = tag;
  L1rr = (L1rr + 1) & kL1Assocmask;
  return false;
}

bool L2(uint64 addr) {
  int set = ((addr >> kL2LgLinesize) & kL2Setmask) << kL2LgAssoc;
  uint64 tag = addr & ~kL2Tagmask;
  for (int i = 0; i < kL2Assoc; ++i) {
    if (L2tag[set + i] == tag) {return true;}
  }
  ++L2misses; 
  L2tag[set + L2rr] = tag;
  L2rr = (L2rr + 1) & kL2Assocmask;
  return false;
}

bool L3(uint64 addr) {
#if HASHED_L3
  int set = (((addr >> kL3LgLinesize) ^ (addr >> kL3LgSize)) & kL3Setmask) << kL3LgAssoc;
#else
  int set = ((addr >> kL3LgLinesize) & kL3Setmask) << kL3LgAssoc;
#endif
  uint64 tag = addr & ~kL3Tagmask;
  for (int i = 0; i < kL3Assoc; ++i) {
    if (L3tag[set + i] == tag) {return true;}
  }
  ++L3misses; 
  L3tag[set + L3rr] = tag;
  L3rr = (L3rr + 1) & kL3Assocmask;
  return false;
}

void L123(uint64 addr) {
  L1(addr);
  L2(addr);
  L3(addr);
}

#else

bool L1(uint64 addr) {return false;}
bool L2(uint64 addr) {return false;}
bool L3(uint64 addr) {return false;}
void L123(uint64 addr) {}
#endif

// Give simple values near 1.0 to each element of arr
void SimpleInit(double* arr) {
  for (int row = 0; row < kRowsize; ++row) {
    for (int col = 0; col < kColsize; ++col) {
      arr[row * kRowsize + col] = 1.0 + (row * kRowsize + col) / 1000000.0;
    }
  }
}

// Zero arr
void ZeroInit(double* arr) {
  for (int row = 0; row < kRowsize; ++row) {
    for (int col = 0; col < kColsize; ++col) {
      arr[row * kRowsize + col] = 0.0;
    }
  }
}

// Sum all the elements of arr -- used for simple sameness check
double SimpleSum(double* arr) {
  double sum = 0.0;
  for (int row = 0; row < kRowsize; ++row) {
    for (int col = 0; col < kColsize; ++col) {
      sum += arr[row * kRowsize + col];
    }
  }
  return sum;
}

// Test two arrays for equality
bool EqualArray(const double* arr1, const double* arr2) {
  for (int k = 0; k < kRowsize * kColsize; ++k) {
    if (arr1[k] != arr2[k]) {return false;}
  }
  return true;
}

void TimeMe(const char* label, MulProc f, const double* a, const double* b, double* c) {
  InitTags();
  int64 start_usec = GetUsec();
  f(a, b, c);  
  int64 stop_usec = GetUsec();
  double duration_usec = stop_usec - start_usec;
  fprintf(stdout, "%s\t%5.3f seconds, sum=%18.9f\n", label, duration_usec/1000000.0, SimpleSum(c)); 
  fprintf(stdout, "Misses L1/L2/L3 %10lld %10lld %10lld\n", L1misses, L2misses, L3misses);
}

inline
double VectorSum4(const double* aptr, const double* bptr, int count, int rowsize) {
  const double* aptr2 = aptr;
  const double* bptr2 = bptr;
  double sum0 = 0.0;
  double sum1 = 0.0;
  double sum2 = 0.0;
  double sum3 = 0.0;
  for (int k = 0; k < count; k += 4) {
    sum0 += aptr2[0] * bptr2[0 * rowsize];
    sum1 += aptr2[1] * bptr2[1 * rowsize];
    sum2 += aptr2[2] * bptr2[2 * rowsize];
    sum3 += aptr2[3] * bptr2[3 * rowsize];
L1((uint64)&aptr2[0]);
L2((uint64)&aptr2[0]);
L3((uint64)&aptr2[0]);
L1((uint64)&bptr2[0 * rowsize]);
L2((uint64)&bptr2[0 * rowsize]);
L3((uint64)&bptr2[0 * rowsize]);
L1((uint64)&aptr2[1]);
L2((uint64)&aptr2[1]);
L3((uint64)&aptr2[1]);
L1((uint64)&bptr2[1 * rowsize]);
L2((uint64)&bptr2[1 * rowsize]);
L3((uint64)&bptr2[1 * rowsize]);
L1((uint64)&aptr2[2]);
L2((uint64)&aptr2[2]);
L3((uint64)&aptr2[2]);
L1((uint64)&bptr2[2 * rowsize]);
L2((uint64)&bptr2[2 * rowsize]);
L3((uint64)&bptr2[2 * rowsize]);
L1((uint64)&aptr2[3]);
L2((uint64)&aptr2[3]);
L3((uint64)&aptr2[3]);
L1((uint64)&bptr2[3 * rowsize]);
L2((uint64)&bptr2[3 * rowsize]);
L3((uint64)&bptr2[3 * rowsize]);
    aptr2 += 4;
    bptr2 += 4 * rowsize;
  }
  return (sum0 + sum1 + sum2 + sum3);
}

//
//==============================================================================

// Copy an NxN subarray to linear addresses, spreading across all L1 cache sets
// 8x8   => 64*8 bytes = 512 bytes or 8 sequential cache lines
// 16x16 => 256*8  = 2048 bytes or 32 sequential cache lines
// 32x32 => 1024*8 = 8192 bytes or 128 sequential cache lines (two lines per set in i3 L1 cache)
void Remap(const double* x, double* xprime) {
  int k = 0;
  for (int row = 0; row < kRemapsize; ++row) {
    for (int col = 0; col < kRemapsize; col += 4) {
      xprime[k + 0] = x[row * kRowsize + col + 0];
      xprime[k + 1] = x[row * kRowsize + col + 1];
      xprime[k + 2] = x[row * kRowsize + col + 2];
      xprime[k + 3] = x[row * kRowsize + col + 3];
L1((uint64)&xprime[k + 0]);
L1((uint64)&xprime[k + 1]);
L1((uint64)&xprime[k + 2]);
L1((uint64)&xprime[k + 3]);
L1((uint64)&x[row * kRowsize + col + 0]);
L1((uint64)&x[row * kRowsize + col + 1]);
L1((uint64)&x[row * kRowsize + col + 2]);
L1((uint64)&x[row * kRowsize + col + 3]);

L2((uint64)&xprime[k + 0]);
L2((uint64)&xprime[k + 1]);
L2((uint64)&xprime[k + 2]);
L2((uint64)&xprime[k + 3]);
L2((uint64)&x[row * kRowsize + col + 0]);
L2((uint64)&x[row * kRowsize + col + 1]);
L2((uint64)&x[row * kRowsize + col + 2]);
L2((uint64)&x[row * kRowsize + col + 3]);

L3((uint64)&xprime[k + 0]);
L3((uint64)&xprime[k + 1]);
L3((uint64)&xprime[k + 2]);
L3((uint64)&xprime[k + 3]);
L3((uint64)&x[row * kRowsize + col + 0]);
L3((uint64)&x[row * kRowsize + col + 1]);
L3((uint64)&x[row * kRowsize + col + 2]);
L3((uint64)&x[row * kRowsize + col + 3]);

      k += 4;
    }
  }
}

// Copy all NxN subarrays to linear addresses
void RemapAll(const double* x, double* xprime) {
  int k = 0;
  for (int row = 0; row < kRowsize; row += kRemapsize) {
    for (int col = 0; col < kColsize; col += kRemapsize) {
      Remap(&x[row * kRowsize + col], &xprime[k]);
      k += (kRemapsize * kRemapsize);
    }
  }
}

// Copy an NxN subarray from linear addresses
void UnRemap(const double* xprime, double* x) {
  int k = 0;
  for (int row = 0; row < kRemapsize; ++row) {
    for (int col = 0; col < kRemapsize; col += 4) {
      x[row * kRowsize + col + 0] = xprime[k + 0];
      x[row * kRowsize + col + 1] = xprime[k + 1];
      x[row * kRowsize + col + 2] = xprime[k + 2];
      x[row * kRowsize + col + 3] = xprime[k + 3];
L1((uint64)&x[row * kRowsize + col + 0]);
L1((uint64)&x[row * kRowsize + col + 1]);
L1((uint64)&x[row * kRowsize + col + 2]);
L1((uint64)&x[row * kRowsize + col + 3]);
L1((uint64)&xprime[k + 0]);
L1((uint64)&xprime[k + 1]);
L1((uint64)&xprime[k + 2]);
L1((uint64)&xprime[k + 3]);

L2((uint64)&x[row * kRowsize + col + 0]);
L2((uint64)&x[row * kRowsize + col + 1]);
L2((uint64)&x[row * kRowsize + col + 2]);
L2((uint64)&x[row * kRowsize + col + 3]);
L2((uint64)&xprime[k + 0]);
L2((uint64)&xprime[k + 1]);
L2((uint64)&xprime[k + 2]);
L2((uint64)&xprime[k + 3]);

L3((uint64)&x[row * kRowsize + col + 0]);
L3((uint64)&x[row * kRowsize + col + 1]);
L3((uint64)&x[row * kRowsize + col + 2]);
L3((uint64)&x[row * kRowsize + col + 3]);
L3((uint64)&xprime[k + 0]);
L3((uint64)&xprime[k + 1]);
L3((uint64)&xprime[k + 2]);
L3((uint64)&xprime[k + 3]);

      k += 4;
    }
  }
}

// Copy all NxN subarrays from linear addresses
void UnRemapAll(const double* xprime, double* x) {
  int k = 0;
  for (int row = 0; row < kRowsize; row += kRemapsize) {
    for (int col = 0; col < kColsize; col += kRemapsize) {
      UnRemap(&xprime[k], &x[row * kRowsize + col]);
      k += (kRemapsize * kRemapsize);
    }
  }
}

void BlockMultiplyRemap(const double* a, const double* b, double* c) {
    RemapAll(a, aa);
    RemapAll(b, bb);

#pragma omp parallel for collapse(2) schedule(dynamic)
    for (int row = 0; row < kRowsize; row += kRemapsize) {
        for (int col = 0; col < kColsize; col += kRemapsize) {
            double* ccptr = &cc[(row * kRowsize) + (col * kRemapsize)];
            for (int k = 0; k < kRowsize; k += kRemapsize) {
                const double* aaptr = &aa[(row * kRowsize) + (k * kRemapsize)];
                const double* bbptr = &bb[(k * kRowsize) + (col * kRemapsize)];
                int kk = 0;
                for (int subrow = 0; subrow < kRemapsize; ++subrow) {
                    for (int subcol = 0; subcol < kRemapsize; ++subcol) {
                        ccptr[kk] += VectorSum4(&aaptr[subrow * kRemapsize + 0],
                                                &bbptr[0 * kRemapsize + subcol],
                                                kRemapsize, kRemapsize);
                        ++kk;
                    }
                }
            }
        }
    }

    RemapAll(cc, c);
}

double* PageAlign(double* p) {
  double* p_local = p + 511;
  *reinterpret_cast<uint64*>(&p_local) &= ~0xfff;
////  fprintf(stdout, "%016llx %016llx\n", (uint64)p, (uint64)p_local);
  return p_local;
}

int main(int argc, const char** argv) {
  // omp_set_num_threads(4);
kutrace::mark_a("alloc");
  double* abase = new double[kRowsize * kColsize + 512];
  double* bbase = new double[kRowsize * kColsize + 512];
  double* cbase = new double[kRowsize * kColsize + 512];
  double* a = PageAlign(abase);
  double* b = PageAlign(bbase);
  double* c = PageAlign(cbase);
  double* aabase = new double[kRowsize * kColsize + 512];
  double* bbbase = new double[kRowsize * kColsize + 512];
  double* ccbase = new double[kRowsize * kColsize + 512];
  aa = PageAlign(aabase);
  bb = PageAlign(bbbase);
  cc = PageAlign(ccbase);

kutrace::mark_a("init");
SimpleInit(a);
SimpleInit(b);
InitTags();

  // Test remap
kutrace::mark_a("remap");
  RemapAll(a, aa);
  UnRemapAll(aa, c);
  fprintf(stdout, "a  sum=%18.9f\n", SimpleSum(a)); 
  fprintf(stdout, "aa sum=%18.9f\n", SimpleSum(aa)); 
  fprintf(stdout, "c  sum=%18.9f\n", SimpleSum(c)); 
  fprintf(stdout, "%s\n", EqualArray(a, c) ? "Equal" : "Not equal"); 
  fprintf(stdout, "Remap Misses L1/L2/L3 %10lld %10lld %10lld\n", L1misses, L2misses, L3misses);
  InitTags();

kutrace::mark_a("simpr");
  TimeMe("BlockMultiplyRemap        ", BlockMultiplyRemap, a, b, c);
  ZeroInit(c);

  delete[] ccbase; 
  delete[] bbbase; 
  delete[] aabase; 
  delete[] cbase; 
  delete[] bbase; 
  delete[] abase; 
  return 0;
}