/* ta_bench.c — Benchmark the 19 Accelerate-optimized TA functions.
 *
 * Usage: ta_bench [runs]
 *   runs = number of repetitions per function/size (default 50)
 *
 * Outputs CSV to stdout:
 *   accel,function,size,run,us
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ta_libc.h"

#ifdef WIN32
   #include "windows.h"
   typedef LARGE_INTEGER bench_time_t;
   static LARGE_INTEGER bench_qpf;
   static void bench_init(void) { QueryPerformanceFrequency(&bench_qpf); }
   static bench_time_t bench_now(void) { bench_time_t t; QueryPerformanceCounter(&t); return t; }
   static double bench_us(bench_time_t a, bench_time_t b) {
      return (double)(b.QuadPart - a.QuadPart) / (double)bench_qpf.QuadPart * 1e6;
   }
#elif defined(__APPLE__)
   #include <mach/mach_time.h>
   typedef uint64_t bench_time_t;
   static mach_timebase_info_data_t bench_tbi;
   static void bench_init(void) { mach_timebase_info(&bench_tbi); }
   static bench_time_t bench_now(void) { return mach_absolute_time(); }
   static double bench_us(bench_time_t a, bench_time_t b) {
      return (double)(b - a) * (double)bench_tbi.numer / (double)bench_tbi.denom / 1000.0;
   }
#else
   #include <time.h>
   typedef clock_t bench_time_t;
   static void bench_init(void) {}
   static bench_time_t bench_now(void) { return clock(); }
   static double bench_us(bench_time_t a, bench_time_t b) {
      return (double)(b - a) / (double)CLOCKS_PER_SEC * 1e6;
   }
#endif

static const int SIZES[] = { 100, 1000, 10000, 100000, 1000000 };
#define NUM_SIZES (sizeof(SIZES) / sizeof(SIZES[0]))
#define WARMUP_ITERS 10

static void fill(double *buf, int n, double lo, double hi)
{
   const double scale = (hi - lo) / (double)RAND_MAX;
   for (int i = 0; i < n; i++)
      buf[i] = lo + (double)rand() * scale;
}

typedef TA_RetCode (*fn_1in)(int, int, const double[], int*, int*, double[]);
typedef TA_RetCode (*fn_2in)(int, int, const double[], const double[], int*, int*, double[]);

typedef struct {
   const char *name;
   int nInputs;
   union { fn_1in f1; fn_2in f2; };
   double lo, hi;
} BenchFunc;

int main(int argc, char **argv)
{
   const int runs = (argc > 1) ? atoi(argv[1]) : 50;
   if (runs <= 0) { fprintf(stderr, "usage: ta_bench [runs]\n"); return 1; }

   bench_init();

   const int maxN = SIZES[NUM_SIZES - 1];
   double *in0 = malloc(sizeof(double) * maxN);
   double *in1 = malloc(sizeof(double) * maxN);
   double *out = malloc(sizeof(double) * maxN);

   const BenchFunc funcs[] = {
      { "ACOS",  1, { .f1 = TA_ACOS  }, -1.0,   1.0   },
      { "ASIN",  1, { .f1 = TA_ASIN  }, -1.0,   1.0   },
      { "ATAN",  1, { .f1 = TA_ATAN  }, -100.0, 100.0  },
      { "CEIL",  1, { .f1 = TA_CEIL  },  0.0,  1000.0  },
      { "COS",   1, { .f1 = TA_COS   }, -3.15,  3.15   },
      { "COSH",  1, { .f1 = TA_COSH  }, -3.0,   3.0    },
      { "EXP",   1, { .f1 = TA_EXP   }, -5.0,   5.0    },
      { "FLOOR", 1, { .f1 = TA_FLOOR },  0.0,  1000.0  },
      { "LN",    1, { .f1 = TA_LN    },  0.01, 1000.0  },
      { "LOG10", 1, { .f1 = TA_LOG10 },  0.01, 1000.0  },
      { "SIN",   1, { .f1 = TA_SIN   }, -3.15,  3.15   },
      { "SINH",  1, { .f1 = TA_SINH  }, -3.0,   3.0    },
      { "SQRT",  1, { .f1 = TA_SQRT  },  0.0,  1000.0  },
      { "TAN",   1, { .f1 = TA_TAN   }, -1.5,   1.5    },
      { "TANH",  1, { .f1 = TA_TANH  }, -3.0,   3.0    },
      { "ADD",   2, { .f2 = TA_ADD   },  0.0,  1000.0  },
      { "SUB",   2, { .f2 = TA_SUB   },  0.0,  1000.0  },
      { "MULT",  2, { .f2 = TA_MULT  },  0.0,   100.0  },
      { "DIV",   2, { .f2 = TA_DIV   },  0.01,  100.0  },
   };
   const int nFuncs = sizeof(funcs) / sizeof(funcs[0]);

#ifdef TA_USE_ACCELERATE
   const char *accel = "ON";
#else
   const char *accel = "OFF";
#endif

   printf("accel,function,size,run,us\n");

   for (int f = 0; f < nFuncs; f++) {
      const BenchFunc *fn = &funcs[f];
      for (int s = 0; s < (int)NUM_SIZES; s++) {
         const int n = SIZES[s];
         int begIdx, nbElem;

         /* Fresh random data per size */
         srand(42 + f * 1000 + s);
         fill(in0, n, fn->lo, fn->hi);
         fill(in1, n, fn->lo, fn->hi);

         /* Warmup */
         for (int w = 0; w < WARMUP_ITERS; w++) {
            if (fn->nInputs == 1)
               fn->f1(0, n-1, in0, &begIdx, &nbElem, out);
            else
               fn->f2(0, n-1, in0, in1, &begIdx, &nbElem, out);
         }

         /* Timed runs */
         for (int r = 0; r < runs; r++) {
            bench_time_t t0, t1;
            if (fn->nInputs == 1) {
               t0 = bench_now();
               fn->f1(0, n-1, in0, &begIdx, &nbElem, out);
               t1 = bench_now();
            } else {
               t0 = bench_now();
               fn->f2(0, n-1, in0, in1, &begIdx, &nbElem, out);
               t1 = bench_now();
            }
            printf("%s,%s,%d,%d,%.4f\n", accel, fn->name, n, r, bench_us(t0, t1));
         }
      }
   }

   free(in0);
   free(in1);
   free(out);
   return 0;
}
