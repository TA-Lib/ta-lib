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
 *     1. EXTERNAL ORACLE on the 252-bar TA_SREF corpus: goldens captured from
 *        pandas 2.3.3 (rolling(n, min_periods=n).max()/.min(),
 *        middle = 0.5*(DCL+DCU)).
 *     2. A SECOND, INDEPENDENT EXTERNAL ORACLE: ta4j-core 0.22.6's
 *        DonchianChannel{Upper,Middle,Lower}Indicator -- a different language
 *        and codebase, so agreement with leg 1 is not two wrappers over the
 *        same rolling() call. Every row differs from BOTH neighbouring bars,
 *        so a window off by one in either direction fails here.
 *     3. DIFFERENTIAL: the three outputs must equal MAX(high,N),
 *        MIDPRICE(N) and MIN(low,N) index-for-index -- same window, same
 *        lookback -- over N from 2 to nbBars.
 *     4. Deterministic edges: an all-flat window collapses the channel onto
 *        the flat price with no NaN; a period larger than the history yields
 *        zero output and TA_SUCCESS.
 *     5. In-place aliasing (each output over each input) and the
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

/* Goldens from a SECOND, independent oracle: ta4j-core 0.22.6 (Java)
 * DonchianChannel{Upper,Middle,Lower}Indicator over the same 252 bars, with
 * DoubleNum so the comparison is about the formula and not BigDecimal. A
 * different language and codebase from pandas, so leg 1 and leg 2 agreeing is
 * not two wrappers over the same rolling() call.
 *
 * Every row is DISCRIMINATING: its triple differs from both neighbouring
 * bars, so a window off by one in either direction fails here. N=50 has no
 * such bar on this corpus -- a 50-bar extremum rarely moves in one step --
 * which is why the rows stop at N=20. */
static const DonchianGolden donchianTa4jGold[] =
{
   {   2,   1, 94.939999999999998    , 92.844999999999999    , 90.75 },
   {   2,   2, 96.375                , 93.890000000000001    , 91.405000000000001 },
   {   2,   3, 96.375                , 94.9375               , 93.5 },
   {   3,   2, 96.375                , 93.5625               , 90.75 },
   {   3,   3, 96.375                , 93.890000000000001    , 91.405000000000001 },
   {   3,   4, 96.375                , 94.594999999999999    , 92.814999999999998 },
   {   5,   4, 96.375                , 93.5625               , 90.75 },
   {   5,   5, 96.375                , 93.890000000000001    , 91.405000000000001 },
   {   5,   6, 96.375                , 94.1875               , 92 },
   {   7,   6, 96.375                , 93.5625               , 90.75 },
   {   7,   7, 96.375                , 93.0625               , 89.75 },
   {   7,   8, 96.375                , 92.907499999999999    , 89.439999999999998 },
   {  10, 100, 121.75                , 116.125               , 110.5 },
   {  10, 175, 137.69                , 130.28                , 122.87 },
   {  10, 200, 122.75                , 113.31                , 103.87 },
   {  20,  21, 99.625                , 93.077500000000001    , 86.530000000000001 },
   {  20, 251, 122.12                , 113.31                , 104.5 },
};
#define NB_TA4J_GOLD ((int)(sizeof(donchianTa4jGold)/sizeof(donchianTa4jGold[0])))

/* Floors for the two oracle legs: 3 outputs x every row each one compared.
 * Asserted, not printed -- the legs report nothing on success, so a count that
 * fell to zero is the only way one could run without comparing anything. */
static int g_donchianPandasCmp;
static int g_donchianTa4jCmp;
static int g_donchianDiffCmp;

/* (1) pandas oracle, bitwise. */
static ErrorNumber test_donchian_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int k, idx, lastPeriod = -1;

   g_donchianPandasCmp = 0;

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
                                lastPeriod,
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
            double optIn[1];
            ErrorNumber e;

            optIn[0] = (double)lastPeriod;
            e = server_verify( "DONCHIAN", 0, nbBars - 1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ up, mid, lo, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }

      idx = donchianGold[k].bar - begIdx;
      g_donchianPandasCmp += 3;
      if( up[idx]  != donchianGold[k].upper
          || mid[idx] != donchianGold[k].middle
          || lo[idx]  != donchianGold[k].lower )
      {
         printf( "DONCHIAN oracle Fail [pandas 2.3.3 N=%d] at bar %d: "
                 "got (%.17g,%.17g,%.17g) expected (%.17g,%.17g,%.17g)\n",
                 donchianGold[k].period, donchianGold[k].bar,
                 up[idx], mid[idx], lo[idx],
                 donchianGold[k].upper, donchianGold[k].middle, donchianGold[k].lower );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( g_donchianPandasCmp < 3 * NB_GOLD )
   {
      printf( "DONCHIAN oracle Fail: %d comparisons, expected %d\n",
              g_donchianPandasCmp, 3 * NB_GOLD );
      return TA_DONCHIAN_ORACLE_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (2) ta4j oracle -- a second, independent implementation. */
static ErrorNumber test_donchian_ta4j( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int k, idx, lastPeriod = -1;

   g_donchianTa4jCmp = 0;

   if( nbBars != 252 )
   {
      printf( "DONCHIAN ta4j skip: goldens were captured on the 252-bar corpus, got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;
   for( k = 0; k < NB_TA4J_GOLD; k++ )
   {
      if( donchianTa4jGold[k].period != lastPeriod )
      {
         lastPeriod = donchianTa4jGold[k].period;
         retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                                lastPeriod,
                                &begIdx, &nbElement, up, mid, lo );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod - 1
             || nbElement != nbBars - (lastPeriod - 1) )
         {
            printf( "DONCHIAN ta4j Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod - 1, nbBars - (lastPeriod - 1) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
      }

      idx = donchianTa4jGold[k].bar - begIdx;
      g_donchianTa4jCmp += 3;
      if( up[idx]  != donchianTa4jGold[k].upper
          || mid[idx] != donchianTa4jGold[k].middle
          || lo[idx]  != donchianTa4jGold[k].lower )
      {
         printf( "DONCHIAN ta4j Fail [ta4j-core 0.22.6 N=%d] at bar %d: "
                 "got (%.17g,%.17g,%.17g) expected (%.17g,%.17g,%.17g)\n",
                 donchianTa4jGold[k].period, donchianTa4jGold[k].bar,
                 up[idx], mid[idx], lo[idx],
                 donchianTa4jGold[k].upper, donchianTa4jGold[k].middle, donchianTa4jGold[k].lower );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( g_donchianTa4jCmp < 3 * NB_TA4J_GOLD )
   {
      printf( "DONCHIAN ta4j Fail: %d comparisons, expected %d\n",
              g_donchianTa4jCmp, 3 * NB_TA4J_GOLD );
      return TA_DONCHIAN_ORACLE_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (3) DIFFERENTIAL, bitwise: the three outputs ARE MAX/MIDPRICE/MIN over the
 * same window. Same lookback (N-1), same first output bar, so they align
 * index-for-index and element counts must be equal.
 */
static ErrorNumber test_donchian_differential( const TA_History *history )
{
   static const int periods[] = { 2, 3, 5, 20, 50, 252 };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   static TA_Real refMax[OUT_CAP], refMin[OUT_CAP], refMid[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int pi, i, N;

   g_donchianDiffCmp = 0;

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

      retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                             N, &begIdx, &nbElement, up, mid, lo );
      if( retCode != TA_SUCCESS )
      {
         printf( "DONCHIAN differential Fail [N=%d]: rc=%d\n", N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( nbElement != refNb || begIdx != refBeg )
      {
         printf( "DONCHIAN differential Fail [N=%d]: shape (%d,%d) expected (%d,%d)\n",
                 N, begIdx, nbElement, refBeg, refNb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( up[i] != refMax[i] || lo[i] != refMin[i] || mid[i] != refMid[i] )
         {
            printf( "DONCHIAN differential Fail [N=%d] out %d: "
                    "(%.17g,%.17g,%.17g) != (MAX,MIDPRICE,MIN) (%.17g,%.17g,%.17g)\n",
                    N, i, up[i], mid[i], lo[i],
                    refMax[i], refMid[i], refMin[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_donchianDiffCmp += 3;
      }
   }

   /* The identity is the only in-tree check that the window is the right one;
    * a sweep that compared nothing would leave it unproven. */
   if( g_donchianDiffCmp < 3 * 100 )
   {
      printf( "DONCHIAN differential Fail: %d comparisons, expected >= %d\n",
              g_donchianDiffCmp, 3 * 100 );
      return TA_DONCHIAN_ORACLE_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (4) Deterministic edges. */
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
   retCode = TA_DONCHIAN( 0, 31, flatHigh, flatLow, 5,
                          &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS || begIdx != 4 || nbElement != 28 )
   {
      printf( "DONCHIAN flat Fail: rc=%d (%d,%d) expected (4,28)\n",
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

   /* A period longer than the history: zero output, TA_SUCCESS. And the
    * boundary either side of it -- exactly-fits emits one element. */
   retCode = TA_DONCHIAN( 0, 31, flatHigh, flatLow, 32,
                          &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS || nbElement != 1 || begIdx != 31 )
   {
      printf( "DONCHIAN exact-fit period Fail: rc=%d (%d,%d) expected (31,1)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   retCode = TA_DONCHIAN( 0, 31, flatHigh, flatLow, 33,
                          &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS || nbElement != 0 || begIdx != 0 )
   {
      printf( "DONCHIAN oversize-period Fail: rc=%d (%d,%d) expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing -- each output over each input in turn must match the
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
   int i, which, N = 20;

   if( nbBars > OUT_CAP )
      nbBars = OUT_CAP;

   retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                          N, &begIdx, &nbElement, up, mid, lo );
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

      retCode = TA_DONCHIAN( 0, nbBars - 1, inA, inB, N,
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

   /* BOTH inputs aliased at once -- the six cases above only ever have one
    * live alias, so nothing there exercises two simultaneous in-place writes.
    * Safe by the same argument (every read of both arrays precedes both
    * writes, and outIdx never passes trailingIdx), which is what this pins. */
   memcpy( inA, history->high, nbBars * sizeof(TA_Real) );
   memcpy( inB, history->low,  nbBars * sizeof(TA_Real) );
   retCode = TA_DONCHIAN( 0, nbBars - 1, inA, inB, N, &aBeg, &aNb, inA, o1, inB );
   if( retCode != TA_SUCCESS || aBeg != begIdx || aNb != nbElement )
   {
      printf( "DONCHIAN dual-alias Fail: rc=%d (%d,%d) expected (%d,%d)\n",
              (int)retCode, aBeg, aNb, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( inA[i] != up[i] || o1[i] != mid[i] || inB[i] != lo[i] )
      {
         printf( "DONCHIAN dual-alias Fail out %d: (%.17g,%.17g,%.17g) != (%.17g,%.17g,%.17g)\n",
                 i, inA[i], o1[i], inB[i], up[i], mid[i], lo[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Two outputs on one buffer must be rejected (issue #108). */
   retCode = TA_DONCHIAN( 0, nbBars - 1, history->high, history->low,
                          N, &aBeg, &aNb, o1, o1, o2 );
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

   err = test_donchian_ta4j( history );
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
