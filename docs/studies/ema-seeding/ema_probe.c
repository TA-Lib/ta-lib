/* Pins the study's model of TA_EMA's seeding against the shipped library.
 *
 * Build (from the repo root, after `scripts/build.py`):
 *   cc -I include -o /tmp/ema_probe docs/studies/ema-seeding/ema_probe.c \
 *      cmake-build/libta-lib.a -lm
 *
 * Prints three facts the write-up relies on:
 *   1. TA_EMA's first output IS the plain p-bar SMA ending at startIdx.
 *   2. TA_EMA re-seeds at startIdx-lookback, so its output is start-dependent.
 *   3. The study's Python `ema_block_S` reproduces TA_EMA bit-for-bit.
 */
#include <stdio.h>
#include <math.h>
#include "ta_libc.h"

extern double gDataClose[];

int main(void)
{
   double out[10000], out2[10000];
   int beg, n, i, p = 25, start = 400;
   double sma = 0.0;

   TA_Initialize();

   /* 1. first output == SMA of the p bars ending at startIdx */
   TA_EMA(start, start, gDataClose, p, &beg, &n, out);
   for (i = start - p + 1; i <= start; i++) sma += gDataClose[i];
   sma /= p;
   printf("1. TA_EMA(%d,%d) first out = %.17g\n", start, start, out[0]);
   printf("   plain %d-bar SMA        = %.17g   %s\n", p, sma,
          out[0] == sma ? "BIT-IDENTICAL" : "differ");

   /* 2. same bar, whole-array call */
   TA_EMA(0, 500, gDataClose, p, &beg, &n, out2);
   printf("2. TA_EMA(0,500) at bar %d = %.17g   (beg=%d)\n",
          start, out2[start - beg], beg);
   printf("   -> start-dependent: %s, gap %.6g\n",
          out2[start - beg] == out[0] ? "no" : "YES",
          fabs(out2[start - beg] - out[0]));

   /* 3. dump a short run for the Python side to compare against */
   TA_EMA(0, 60, gDataClose, p, &beg, &n, out);
   printf("3. TA_EMA(0,60,p=%d) beg=%d n=%d\n", p, beg, n);
   for (i = 0; i < n; i++) printf("   %d %.17g\n", beg + i, out[i]);

   TA_Shutdown();
   return 0;
}
