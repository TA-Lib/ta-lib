/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
 */

/* Description:
 *
 *   Test TA_DONCHIAN (Donchian Channels).
 *
 *   Every comparison in this file is BITWISE (==), never tolerance-based:
 *   the upper and lower bands are selections of unmodified input values and
 *   the middle is one add plus an exact power-of-two halving, so no
 *   floating-point divergence exists for a tolerance to absorb. A nonzero
 *   diff anywhere here is a real bug.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE at optInLag=0 on the 252-bar TA_SREF corpus:
 *        goldens captured from pandas 2.3.3 (rolling(n, min_periods=n)
 *        .max()/.min(), middle = 0.5*(DCL+DCU)). pandas' rolling window is
 *        inclusive, so the arm is valid ONLY at lag 0; the lag ruling itself
 *        is settled by the proposal, not by an oracle (none ships the lagged
 *        default), and the lag ARITHMETIC is what leg 2's L sweep pins.
 *     2. DIFFERENTIAL: DONCHIAN(N,L)[i] must equal MAX(high,N)[i],
 *        MIN(low,N)[i] and MIDPRICE(N)[i] with the output arrays aligned by
 *        construction -- DONCHIAN's first output sits at bar N-1+L reading
 *        the window ending at bar N-1, which is exactly MAX's first output.
 *        Swept over N x L including both edges (N=2, N=nbBars) so the shift
 *        itself is exercised, not just the L=0 identity.
 *     3. Deterministic edges: an all-flat window collapses the channel onto
 *        the flat price with no NaN; a lag larger than the history yields
 *        zero output and TA_SUCCESS.
 *     4. In-place aliasing (each output over each input) and the
 *        output-distinctness rejection (two outputs on one buffer).
 *
 *   Cross-language value coverage comes from server_verify below plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so
 *   the --codegen value comparison cannot run for it (same situation as KC).
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP 1024

/* Golden values from pandas 2.3.3 (CPython 3.10), computed as
 *   DCU = high.rolling(n, min_periods=n).max()
 *   DCL = low.rolling(n, min_periods=n).min()
 *   DCM = 0.5 * (DCL + DCU)
 * over TA_SREF_{high,low}_daily_ref_0_PRIV -- the same 252 bars the history
 * argument carries -- and printed at %.17g, which round-trips to the same
 * double. 0.5*(a+b) and (a+b)/2.0 are bit-identical (commutative add, exact
 * power-of-two scaling), so the pins are compared with ==.
 * Valid at optInLag=0 only: pandas' rolling window includes the current bar.
 * bar is the ABSOLUTE bar index; the output index is bar - begIdx. */
typedef struct { int period; int bar;
                 double upper; double middle; double lower; } DonchianGolden;

static const DonchianGolden donchianGold[] =
{
   { 20,  19,  99.625,  93.1875,  86.75 },
   { 20, 100, 123.0,   112.36,   101.72 },
   { 20, 251, 122.12,  113.31,   104.5  },
   {  5,   4,  96.375,  93.5625,  90.75 },
   {  5, 100, 118.315, 114.4075, 110.5  },
   {  5, 251, 110.75,  108.685,  106.62 },
};
#define NB_GOLD ((int)(sizeof(donchianGold)/sizeof(donchianGold[0])))

/* (1) pandas oracle at optInLag=0, bitwise. */
static ErrorNumber test_donchian_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int k, idx, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "DONCHIAN oracle skip: goldens were captured on the 252-bar corpus, got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;
   for( k = 0; k < NB_GOLD; k++ )
   {
      if( donchianGold[k].period != lastPeriod )
      {
         lastPeriod = donchianGold[k].period;
         retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                                lastPeriod, 0,
                                &begIdx, &nbElement, up, mid, lo );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod - 1
             || nbElement != nbBars - (lastPeriod - 1) )
         {
            printf( "DONCHIAN oracle Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod - 1, nbBars - (lastPeriod - 1) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* Cross-language: bit-identical on every language server. */
         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;

            optIn[0] = (double)lastPeriod;
            optIn[1] = 0.0;
            e = server_verify( "DONCHIAN", 0, nbBars - 1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ up, mid, lo, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }

      idx = donchianGold[k].bar - begIdx;
      if( up[idx]  != donchianGold[k].upper
          || mid[idx] != donchianGold[k].middle
          || lo[idx]  != donchianGold[k].lower )
      {
         printf( "DONCHIAN oracle Fail [pandas 2.3.3 N=%d lag=0] at bar %d: "
                 "got (%.17g,%.17g,%.17g) expected (%.17g,%.17g,%.17g)\n",
                 donchianGold[k].period, donchianGold[k].bar,
                 up[idx], mid[idx], lo[idx],
                 donchianGold[k].upper, donchianGold[k].middle, donchianGold[k].lower );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (2) DIFFERENTIAL, bitwise: the three outputs ARE MAX/MIN/MIDPRICE of the
 * window ending optInLag bars back. Alignment is by construction:
 * DONCHIAN(N,L) puts its first output at bar N-1+L, whose window ends at bar
 * N-1 -- exactly where MAX/MIN/MIDPRICE(N) put their first output. So
 * don[i] == ref[i] index-for-index and donNb == refNb - L.
 */
static ErrorNumber test_donchian_differential( const TA_History *history )
{
   static const int periods[] = { 2, 3, 5, 20, 50, 252 };
   static const int lags[]    = { 0, 1, 2 };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   static TA_Real refMax[OUT_CAP], refMin[OUT_CAP], refMid[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int pi, li, i, N, L, expectNb;

   for( pi = 0; pi < (int)(sizeof(periods)/sizeof(periods[0])); pi++ )
   {
      N = periods[pi];
      if( N > nbBars )
         continue;

      retCode = TA_MAX( 0, nbBars - 1, history->high, N, &refBeg, &refNb, refMax );
      if( retCode != TA_SUCCESS )
      {
         printf( "DONCHIAN differential Fail: MAX(%d) rc=%d\n", N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      retCode = TA_MIN( 0, nbBars - 1, history->low, N, &refBeg, &refNb, refMin );
      if( retCode != TA_SUCCESS )
      {
         printf( "DONCHIAN differential Fail: MIN(%d) rc=%d\n", N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      retCode = TA_MIDPRICE( 0, nbBars - 1, history->high, history->low, N,
                             &refBeg, &refNb, refMid );
      if( retCode != TA_SUCCESS )
      {
         printf( "DONCHIAN differential Fail: MIDPRICE(%d) rc=%d\n", N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( li = 0; li < (int)(sizeof(lags)/sizeof(lags[0])); li++ )
      {
         L = lags[li];
         retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                                N, L, &begIdx, &nbElement, up, mid, lo );
         if( retCode != TA_SUCCESS )
         {
            printf( "DONCHIAN differential Fail [N=%d L=%d]: rc=%d\n",
                    N, L, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         expectNb = refNb - L;
         if( expectNb < 0 )
            expectNb = 0;
         if( nbElement != expectNb
             || ( nbElement > 0 && begIdx != N - 1 + L ) )
         {
            printf( "DONCHIAN differential Fail [N=%d L=%d]: shape (%d,%d) expected (%d,%d)\n",
                    N, L, begIdx, nbElement, N - 1 + L, expectNb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( up[i] != refMax[i] || lo[i] != refMin[i] || mid[i] != refMid[i] )
            {
               printf( "DONCHIAN differential Fail [N=%d L=%d] out %d: "
                       "(%.17g,%.17g,%.17g) != (MAX,MIDPRICE,MIN) (%.17g,%.17g,%.17g)\n",
                       N, L, i, up[i], mid[i], lo[i],
                       refMax[i], refMid[i], refMin[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Deterministic edges. */
static ErrorNumber test_donchian_edges( void )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real flatHigh[32], flatLow[32];
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   int i;

   /* All-flat: the channel collapses onto the price, bitwise, no NaN --
    * the only division is by the literal 2.0 (issue #112 by construction). */
   for( i = 0; i < 32; i++ )
   {
      flatHigh[i] = 100.0;
      flatLow[i]  = 100.0;
   }
   retCode = TA_DONCHIAN( 0, 31, flatHigh, flatLow, 5, 1,
                          &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 27 )
   {
      printf( "DONCHIAN flat Fail: rc=%d (%d,%d) expected (5,27)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( up[i] != 100.0 || mid[i] != 100.0 || lo[i] != 100.0 )
      {
         printf( "DONCHIAN flat Fail at out %d: (%.17g,%.17g,%.17g) != 100.0\n",
                 i, up[i], mid[i], lo[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* A lag pushing the lookback past the history: zero output, TA_SUCCESS. */
   retCode = TA_DONCHIAN( 0, 31, flatHigh, flatLow, 5, 1000,
                          &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS || nbElement != 0 || begIdx != 0 )
   {
      printf( "DONCHIAN oversize-lag Fail: rc=%d (%d,%d) expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/* (4) In-place aliasing -- each output over each input in turn must match the
 * separate-buffer run -- and the output-distinctness rejection (issue #108).
 * Aliasing holds by the same argument as MIDPRICE: every position written
 * (outIdx) sits at or below trailingIdx, the oldest position later read.
 */
static ErrorNumber test_donchian_aliasing( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, aBeg, aNb;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   static TA_Real inA[OUT_CAP], inB[OUT_CAP];
   static TA_Real o1[OUT_CAP], o2[OUT_CAP];
   TA_Real *outs[3];
   int nbBars = (int)history->nbBars;
   int i, which, N = 20, L = 1;

   if( nbBars > OUT_CAP )
      nbBars = OUT_CAP;

   retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                          N, L, &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS )
   {
      printf( "DONCHIAN aliasing Fail: reference run rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* Alias each of the three outputs over inHigh, then over inLow. */
   for( which = 0; which < 6; which++ )
   {
      TA_Real *aliased;

      memcpy( inA, history->high, nbBars * sizeof(TA_Real) );
      memcpy( inB, history->low,  nbBars * sizeof(TA_Real) );
      aliased = ( which < 3 ) ? inA : inB;
      outs[0] = o1; outs[1] = o2; outs[2] = aliased;
      /* Rotate which logical output lands on the aliased buffer. */
      { TA_Real *t = outs[which % 3]; outs[which % 3] = outs[2]; outs[2] = t; }

      retCode = TA_DONCHIAN( 0, nbBars - 1, inA, inB, N, L,
                             &aBeg, &aNb, outs[0], outs[1], outs[2] );
      if( retCode != TA_SUCCESS || aBeg != begIdx || aNb != nbElement )
      {
         printf( "DONCHIAN aliasing Fail [case %d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 which, (int)retCode, aBeg, aNb, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         if( outs[0][i] != up[i] || outs[1][i] != mid[i] || outs[2][i] != lo[i] )
         {
            printf( "DONCHIAN aliasing Fail [case %d] out %d: "
                    "(%.17g,%.17g,%.17g) != (%.17g,%.17g,%.17g)\n",
                    which, i, outs[0][i], outs[1][i], outs[2][i],
                    up[i], mid[i], lo[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* Two outputs on one buffer must be rejected (issue #108). */
   retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                          N, L, &aBeg, &aNb, o1, o1, o2 );
   if( retCode != TA_BAD_PARAM )
   {
      printf( "DONCHIAN aliasing Fail: shared output buffer accepted (rc=%d)\n",
              (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/**** Global functions definitions.   ****/
ErrorNumber test_func_donchian( TA_History *history )
{
   ErrorNumber err;

   err = test_donchian_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_donchian_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_donchian_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_donchian_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   return TA_TEST_PASS;
}
