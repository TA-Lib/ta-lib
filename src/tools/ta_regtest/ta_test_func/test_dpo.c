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
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090426 MF,CC  First version (issue #363).
 */

/* Description:
 *
 *   Test TA_DPO (Detrended Price Oscillator): inReal[i-t] - SMA(inReal,n)[i],
 *   with t = n/2+1 and the value emitted at the bar whose average produced it.
 *
 *   Legs:
 *     1. BIT-EXACT differential against the shipped TA_SMA. TA_DPO fuses SMA's
 *        rolling window instead of calling it, so the compose is a memcmp, not
 *        a tolerance argument -- but only when TA_SMA is anchored at TA_DPO's
 *        own outBegIdx. SMA seeds its running sum at startIdx-(n-1), so a
 *        different anchor reaches the same window through a different
 *        add/subtract order and the last bits move. Observable only at n == 2,
 *        where DPO's lookback (2) exceeds SMA's (1), which is why the grid
 *        below carries n == 2 and sweeps startIdx.
 *     2. EXTERNAL ORACLES: two independent implementations, in two languages,
 *        frozen one column each. See dpoOracle below.
 *     3. Two PUBLISHED vectors, reproduced at their printed precision. One is
 *        a table from a book printed decades before this file.
 *     4. Exact-arithmetic edges. On a ramp of exact halves every intermediate
 *        is an exact binary fraction and DPO collapses to a CONSTANT that
 *        encodes t, so the displacement is pinned by an equality rather than a
 *        tolerance -- including the period pairs where t collapses (n=2/n=3
 *        both give 2, n=20/n=21 both give 11). An all-flat window is
 *        exactly 0.0, and the lookback's max() is asserted on both of its arms.
 *     5. In-place aliasing (outReal == inReal), bitwise. Two cursors read the
 *        input behind the write cursor and either can reach it exactly, so
 *        this is the leg that catches a store hoisted above a read. ASan
 *        cannot: the clobbering read stays in bounds.
 *     6. The startIdx/endIdx range sweep, in the EPSILON class: the window is
 *        a running accumulator, so a different anchor may move the last bits.
 *
 *   Cross-language value coverage comes from server_verify in leg 2 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as VHF).
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External variables declarations. ****/
extern double gDataClose[];

/**** Local declarations. ****/
#define DPO_CAP    1100
#define DPO_GD_NB  1000

/* Leg 2. The binding term is the ABSOLUTE one and that is not a rounded-up
 * relative bound: DPO crosses zero (three frozen rows below are pure
 * cancellation residue, 1e-14 and smaller), so a relative gate is meaningless
 * exactly where the series is most interesting. Measured worst gap of TA_DPO
 * against the frozen rows: 8.53e-14 absolute to Tulip, 5.68e-14 to pandas,
 * over all four periods and every one of the 964 output values. An order of
 * headroom. The relative companion only widens the bound on the large values,
 * where it is not the term deciding anything. */
#define DPO_ORACLE_REL 1e-12
#define DPO_ORACLE_ABS 1e-12

typedef struct { int period; int bar; double tulip; double pandas; } DpoGolden;

/* Goldens captured by ta-lib-oracles/capture_363_dpo.py on the 252-bar TA_SREF
 * close series (TA_SREF_close_daily_ref_0_PRIV), at %.17g, which round-trips to
 * the same double. `bar` is the ABSOLUTE bar index; the output index is
 * bar - begIdx.
 *
 * TWO independent implementations, in two languages, each driven on this exact
 * series (2026-09-04) and each frozen in its own column, so a row is a
 * comparison against two libraries rather than against one plus a claim about
 * the other:
 *   1. Tulip Indicators 0.9.2, pinned be18abb -- C, `ti_dpo`. Ships its own
 *      committed golden vectors for DPO, replayed by leg 3. It multiplies by a
 *      precomputed 1.0/period where TA_SMA divides, so it is near-bit-exact,
 *      not bitwise.
 *   2. pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1) -- Python,
 *      `ta.dpo` at centered=True, with its display shift undone. Its rolling
 *      mean is pairwise where TA_SMA accumulates sequentially, so the two
 *      oracles agreeing is not one summation order checked twice.
 * They agree with each other to 1.28e-13 absolute over these four periods.
 *
 * Periods carry both parities of the period and the collapse integer division
 * produces: t is 2, 3, 11 and 11 here, so 20 and 21 share one displacement. Period < 3 is deliberately absent -- ti_dpo returns period-1 from
 * ti_dpo_start while reading input[period-1-back], so it reads input[-1] at
 * n <= 2 (indicators/dpo.c:29,52). That upstream out-of-bounds is precisely
 * what the max() in TA_DPO_Lookback avoids, and leg 4 covers n == 2 instead.
 * The bar nearest zero of each period is included: it is where the absolute
 * term of the tolerance is the one deciding. */
static const DpoGolden dpoOracle[] =
{
   {   3,   2,     -2.0633333333333326,     -2.0633333333333326 },
   {   3,   3,     0.05333333333334167,     0.05333333333334167 },
   {   3,  46,  2.8421709430404007e-14,  1.4210854715202004e-14 },
   {   3,  64,    -0.27000000000003865,    -0.27000000000001023 },
   {   3,  76,     -8.4150000000000205,     -8.4150000000000063 },
   {   3, 127,     0.23000000000001819,     0.22999999999998977 },
   {   3, 189,      1.4599999999999653,      1.4599999999999937 },
   {   3, 203,       9.686666666666639,      9.6866666666666674 },
   {   3, 251,     0.45999999999997954,     0.45999999999999375 },
   {   4,   3,      -2.446249999999992,      -2.446249999999992 },
   {   4,   4,     0.29875000000001251,     0.29874999999999829 },
   {   4,  65,    -0.58500000000000796,    -0.58500000000000796 },
   {   4,  77,     -11.061250000000001,     -11.061250000000001 },
   {   4, 112,    0.017499999999984084,    0.017499999999984084 },
   {   4, 127,     -0.7650000000000432,    -0.76500000000001478 },
   {   4, 189,     0.38999999999990109,     0.39000000000001478 },
   {   4, 204,      10.547499999999886,      10.547499999999985 },
   {   4, 251,     0.95249999999992951,     0.95250000000001478 },
   {  20,  19,     -2.5760000000000076,     -2.5759999999999934 },
   {  20,  20,    -0.26475000000000648,    -0.26474999999999227 },
   {  20,  28,   0.0072499999999990905,   0.0072499999999990905 },
   {  20,  77,      2.1324999999999932,      2.1324999999999932 },
   {  20,  83,     -12.088000000000008,     -12.087999999999994 },
   {  20, 101,      9.6602499999999907,      9.6602499999999907 },
   {  20, 135,     0.41300000000001091,     0.41300000000001091 },
   {  20, 193,     -1.9339999999999975,     -1.9339999999999975 },
   {  20, 251,     -3.5699999999999648,     -3.5700000000000074 },
   {  21,  20,    -0.20595238095236823,    -0.20595238095238244 },
   {  21,  21,      3.6335714285714431,      3.6335714285714289 },
   {  21,  78,   -0.015714285714281573,   -0.015714285714295784 },
   {  21,  83,      -11.89928571428571,      -11.89928571428571 },
   {  21, 101,      10.076666666666696,      10.076666666666654 },
   {  21, 136,      1.7742857142857531,      1.7742857142857247 },
   {  21, 160,  -0.0080952380951941905,   -0.008095238095236823 },
   {  21, 194,      1.5271428571428913,      1.5271428571428629 },
   {  21, 251,      -3.317619047618976,      -3.317619047619047 },
};
#define NB_DPO_ORACLE ((int)(sizeof(dpoOracle)/sizeof(DpoGolden)))

/* Leg 3. Neither vector is ours and neither was recomputed here: both are
 * upstream Tulip's own committed expected output, and the second is
 * transcribed from a book table. They pin the anchor and the displacement
 * against sources that have never seen this implementation. The per-vector
 * `decimals` is the file's PRINT precision, and the tolerance is half of its
 * last digit -- the published numbers carry no more information than that, so
 * a tighter bound would be testing the transcription. */
typedef struct
{
   const char   *source;
   int           period;
   int           nbIn;
   const double *in;
   int           nbOut;
   const double *out;
   int           decimals;
} DpoBookVector;

/* tests/untest.txt:162 of Tulip Indicators 0.9.2 (`dpo 5`), 3 decimals. */
static const double dpoTulipVecIn[15] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const double dpoTulipVecOut[11] =
   { -1.366, 0.132, -0.094, 0.292, -0.478, -0.938, -0.264, -0.444,
     -1.214, -0.688, -0.264 };

/* Steven B. Achelis, "Technical Analysis from A to Z", page 119, transcribed
 * into Tulip's tests/atoz.txt:95 (`dpo 6`, `#page 119`), 4 decimals. Two of
 * its nine values land exactly on the half-digit boundary, which is what makes
 * this a round-trip claim rather than an error budget. */
static const double dpoAtozVecIn[14] =
   { 21.6562, 21.625, 21.5312, 22, 21.5, 21.9375, 22.8438,
     23.0625, 22.8125, 23.4375, 19.9375, 20.4688, 20.0625, 19.75 };
static const double dpoAtozVecOut[9] =
   { -0.0833, -0.3751, -0.1458, -0.8594, -0.6615, 0.5053, 0.9687,
     1.1823, 2.3594 };

static const DpoBookVector dpoBookVectors[] =
{
   { "Tulip Indicators 0.9.2 tests/untest.txt:162", 5, 15, dpoTulipVecIn, 11, dpoTulipVecOut, 3 },
   { "Achelis, TA from A to Z p.119 (atoz.txt:95)", 6, 14, dpoAtozVecIn,   9, dpoAtozVecOut,  4 },
};
#define NB_DPO_BOOK ((int)(sizeof(dpoBookVectors)/sizeof(DpoBookVector)))

/* Legs 1 and 5. Both arms of the lookback's max() (n == 2 takes t, everything
 * above takes n-1), both parities of t, and a period longer than the warm-up
 * the startIdx sweep reaches. */
static const int dpoPeriods[] = { 2, 3, 4, 5, 6, 20, 21, 100 };
#define NB_DPO_PERIODS ((int)(sizeof(dpoPeriods)/sizeof(int)))
#define DPO_MAX_START 34

/* Leg 4. On in[i] = 64 + i/2 every window sum, its quotient and the final
 * subtraction are exact binary fractions, and DPO collapses to the constant
 * (n-1)/4 - t/2 for every bar. These are those constants, written out rather
 * than recomputed from t, so the leg tests the displacement instead of
 * restating it. A displacement wrong by one bar moves each by 0.5. */
static const struct { int period; double want; } dpoRamp[] =
{
   {   2, -0.75 }, {   3, -0.5  }, {   4, -0.75 }, {   5, -0.5  },
   {   6, -0.75 }, {  20, -0.75 }, {  21, -0.5  }, {  40, -0.75 },
};
#define NB_DPO_RAMP ((int)(sizeof(dpoRamp)/sizeof(dpoRamp[0])))

/* Leg 4 again: both arms of max(n-1, n/2+1), asserted as numbers. n == 2 is
 * the only period where the displacement wins; n == 3 and n == 4 are the ties. */
static const struct { int period; int lookback; } dpoLookback[] =
{
   { 2, 2 }, { 3, 2 }, { 4, 3 }, { 5, 4 }, { 6, 5 },
   { 20, 19 }, { 21, 20 }, { 100, 99 },
};
#define NB_DPO_LOOKBACK ((int)(sizeof(dpoLookback)/sizeof(dpoLookback[0])))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. The
 * three sweeps below are fixed grids over fixed-length corpora, so their totals
 * are constants: a drop means the sweep stopped reaching bars. */
#define DPO_DIFF_CHECKS  334426
#define DPO_EDGE_CHECKS  8536
#define DPO_ALIAS_CHECKS 70326

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_dpoDiffCmp;
static int g_dpoOracleCmp;
static int g_dpoBookCmp;
static int g_dpoEdgeCmp;
static int g_dpoAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_dpo_differential( const char *tag, const TA_Real *in, int nbBars );
static ErrorNumber test_dpo_oracle( const TA_History *history );
static ErrorNumber test_dpo_published( void );
static ErrorNumber test_dpo_edges( void );
static ErrorNumber test_dpo_aliasing( const char *tag, const TA_Real *in, int nbBars );
static ErrorNumber test_dpo_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_dpo( TA_History *history )
{
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   /* DPO has no unstable period; a leftover global setting must not reach it,
    * and the range sweep below asserts the same thing from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_dpoDiffCmp = g_dpoOracleCmp = g_dpoBookCmp = 0;
   g_dpoEdgeCmp = g_dpoAliasCmp = 0;

   err = test_dpo_differential( "TA_SREF close", history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_differential( "gData close", gDataClose, DPO_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_published();
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_aliasing( "TA_SREF close", history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_aliasing( "gData close", gDataClose, DPO_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_dpo_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_dpoDiffCmp != DPO_DIFF_CHECKS || g_dpoOracleCmp != 2 * NB_DPO_ORACLE
            || g_dpoBookCmp != 20 || g_dpoEdgeCmp != DPO_EDGE_CHECKS
            || g_dpoAliasCmp != DPO_ALIAS_CHECKS ) )
   {
      printf( "DPO Fail: coverage counters (diff %d, oracle %d, published %d, "
              "edges %d, alias %d) are not what this file was written with "
              "(%d, %d, 20, %d, %d)\n",
              g_dpoDiffCmp, g_dpoOracleCmp, g_dpoBookCmp, g_dpoEdgeCmp,
              g_dpoAliasCmp, DPO_DIFF_CHECKS, 2 * NB_DPO_ORACLE,
              DPO_EDGE_CHECKS, DPO_ALIAS_CHECKS );
      return TA_DPO_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) TA_DPO against a compose over the shipped TA_SMA, bit-for-bit.
 *
 * TA_SMA is called at TA_DPO's own outBegIdx, never at 0: its running sum is
 * seeded at startIdx-(n-1), so two anchors reach the same window through
 * different add/subtract sequences and cancellation makes the result differ in
 * the last bits. That is invisible above n == 2 on this corpus, and n == 2 is
 * exactly where DPO's lookback exceeds SMA's -- so a lazy grid would report a
 * green memcmp that proves nothing about the anchor.
 */
static ErrorNumber test_dpo_differential( const char *tag, const TA_Real *in, int nbBars )
{
   static TA_Real fused[DPO_CAP], composed[DPO_CAP], sma[DPO_CAP];
   TA_Integer beg1, nb1, smaBeg, smaNb;
   TA_RetCode retCode;
   int p, period, startIdx, begIdx, lookback, disp, i;

   for( p = 0; p < NB_DPO_PERIODS; p++ )
   {
      period = dpoPeriods[p];
      lookback = TA_DPO_Lookback( period );
      disp = period / 2 + 1;

      for( startIdx = 0; startIdx <= DPO_MAX_START; startIdx++ )
      {
         retCode = TA_DPO( startIdx, nbBars-1, in, period, &beg1, &nb1, fused );
         if( retCode != TA_SUCCESS )
         {
            printf( "DPO differential Fail [%s N=%d start=%d]: rc=%d\n",
                    tag, period, startIdx, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         begIdx = startIdx < lookback ? lookback : startIdx;
         if( beg1 != begIdx || nb1 != nbBars - begIdx )
         {
            printf( "DPO differential Fail [%s N=%d start=%d]: shape (%d,%d) "
                    "expected (%d,%d)\n", tag, period, startIdx, beg1, nb1,
                    begIdx, nbBars - begIdx );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         retCode = TA_SMA( begIdx, nbBars-1, in, period, &smaBeg, &smaNb, sma );
         if( retCode != TA_SUCCESS || smaBeg != begIdx || smaNb != nb1 )
         {
            printf( "DPO differential Fail [%s N=%d start=%d]: TA_SMA rc=%d "
                    "(%d,%d)\n", tag, period, startIdx, (int)retCode, smaBeg, smaNb );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = 0; i < nb1; i++ )
            composed[i] = in[begIdx + i - disp] - sma[i];

         g_dpoDiffCmp += nb1;
         if( memcmp( fused, composed, (size_t)nb1 * sizeof(TA_Real) ) != 0 )
         {
            for( i = 0; i < nb1; i++ )
               if( fused[i] != composed[i] )
               {
                  printf( "DPO differential Fail [%s N=%d start=%d] out %d: "
                          "fused %.17g, composed-over-TA_SMA %.17g -- the fused "
                          "window must be bit-identical to TA_SMA anchored at "
                          "the same bar\n",
                          tag, period, startIdx, i, fused[i], composed[i] );
                  break;
               }
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) The frozen two-oracle goldens, plus the cross-language replay. */
static ErrorNumber test_dpo_oracle( const TA_History *history )
{
   static TA_Real out[DPO_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, o, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "DPO oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_DPO_ORACLE; k++ )
   {
      double got;

      if( dpoOracle[k].period != lastPeriod )
      {
         lastPeriod = dpoOracle[k].period;
         retCode = TA_DPO( 0, nbBars-1, history->close, lastPeriod,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod - 1
             || nbElement != nbBars - (lastPeriod - 1) )
         {
            printf( "DPO oracle Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod - 1, nbBars - (lastPeriod - 1) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "DPO", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->close, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "DPO oracle [N=%d]: compared no server despite live "
                       "pipes\n", lastPeriod );
               return TA_DPO_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( dpoOracle[k].bar < begIdx || dpoOracle[k].bar - begIdx >= nbElement )
      {
         printf( "DPO oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", dpoOracle[k].period, dpoOracle[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_DPO_VACUOUS;
      }

      got = out[dpoOracle[k].bar - begIdx];
      for( o = 0; o < 2; o++ )
      {
         double want = o ? dpoOracle[k].pandas : dpoOracle[k].tulip;
         double err;
         const char *mode;

         g_dpoOracleCmp++;
         if( !checkOracleValue( got, want, DPO_ORACLE_REL, DPO_ORACLE_ABS,
                                &err, &mode ) )
         {
            printf( "DPO oracle Fail [N=%d] at bar %d vs %s: got %.17g expected "
                    "%.17g (%s err %.3g, tol rel %g abs %g)\n",
                    dpoOracle[k].period, dpoOracle[k].bar,
                    o ? "pandas-ta-classic" : "tulip", got, want, mode, err,
                    DPO_ORACLE_REL, DPO_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) The two published vectors, at their printed precision. */
static ErrorNumber test_dpo_published( void )
{
   static TA_Real out[DPO_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int v, i;

   for( v = 0; v < NB_DPO_BOOK; v++ )
   {
      const DpoBookVector *bv = &dpoBookVectors[v];
      double tol = 0.5 * pow( 10.0, -(double)bv->decimals );

      retCode = TA_DPO( 0, bv->nbIn-1, bv->in, bv->period,
                        &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != bv->period - 1
          || nbElement != bv->nbOut )
      {
         printf( "DPO published Fail [%s]: rc=%d (%d,%d) expected (%d,%d)\n",
                 bv->source, (int)retCode, begIdx, nbElement,
                 bv->period - 1, bv->nbOut );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double diff = fabs( out[i] - bv->out[i] );

         g_dpoBookCmp++;
         if( isnan( out[i] ) || diff > tol )
         {
            printf( "DPO published Fail [%s] out %d: got %.10f, expected %.*f "
                    "(|diff| %.3e > %.1e, half of the last printed digit)\n",
                    bv->source, i, out[i], bv->decimals, bv->out[i], diff, tol );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Exact-arithmetic edges, the lookback's two arms, and the degenerate
 * ranges.
 *
 *   - A ramp of exact halves. Every window sum, its quotient by the period and
 *     the final subtraction are exact binary fractions, so DPO is a CONSTANT
 *     there and the assertion is an equality. That constant is a function of
 *     the displacement alone once the period is fixed, which makes this the leg
 *     that pins t -- including at n=2/n=3 and n=20/n=21, pairs where integer
 *     division collapses two periods onto one t.
 *   - An all-flat input detrends to exactly 0.0, never a residue.
 *   - The lookback on both arms of its max(), as numbers.
 */
static ErrorNumber test_dpo_edges( void )
{
   static TA_Real in[256], out[DPO_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int k, i, period, lookback;

   for( k = 0; k < NB_DPO_LOOKBACK; k++ )
   {
      g_dpoEdgeCmp++;
      if( TA_DPO_Lookback( dpoLookback[k].period ) != dpoLookback[k].lookback )
      {
         printf( "DPO lookback Fail [N=%d]: got %d, expected %d -- max(n-1, "
                 "n/2+1), whose displacement arm only wins at n == 2\n",
                 dpoLookback[k].period,
                 TA_DPO_Lookback( dpoLookback[k].period ),
                 dpoLookback[k].lookback );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( i = 0; i < 200; i++ )
      in[i] = 64.0 + 0.5 * (double)i;

   for( k = 0; k < NB_DPO_RAMP; k++ )
   {
      period = dpoRamp[k].period;
      lookback = TA_DPO_Lookback( period );

      retCode = TA_DPO( 0, 199, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != lookback
          || nbElement != 200 - lookback )
      {
         printf( "DPO ramp Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement,
                 lookback, 200 - lookback );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_dpoEdgeCmp++;
         if( out[i] != dpoRamp[k].want )
         {
            printf( "DPO ramp Fail [N=%d] out %d: %.17g, expected exactly "
                    "%.17g. Every intermediate here is an exact binary "
                    "fraction, so a difference is the displacement, not "
                    "rounding\n", period, i, out[i], dpoRamp[k].want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( i = 0; i < 200; i++ )
      in[i] = 42.0;
   for( period = 2; period <= 40; period++ )
   {
      lookback = TA_DPO_Lookback( period );
      retCode = TA_DPO( 0, 199, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != lookback
          || nbElement != 200 - lookback )
      {
         printf( "DPO flat Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_dpoEdgeCmp++;
         if( out[i] != 0.0 )
         {
            printf( "DPO flat Fail [N=%d] out %d: %.17g, expected exactly 0.0 "
                    "-- a flat series is its own average\n",
                    period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* Fewer bars than the lookback: TA_SUCCESS with (0,0), never a stale
    * outBegIdx. n == 2 is the arm where the displacement sets the lookback. */
   begIdx = 12345; nbElement = 12345;
   retCode = TA_DPO( 0, 1, in, 2, &begIdx, &nbElement, out );
   g_dpoEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "DPO short-range Fail [N=2]: rc=%d (%d,%d), expected TA_SUCCESS "
              "(0,0)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Exactly the lookback, on both arms of the max(): one value each. */
   for( period = 2; period <= 3; period++ )
   {
      lookback = TA_DPO_Lookback( period );
      retCode = TA_DPO( lookback, lookback, in, period,
                        &begIdx, &nbElement, out );
      g_dpoEdgeCmp++;
      if( retCode != TA_SUCCESS || begIdx != lookback || nbElement != 1 )
      {
         printf( "DPO one-value Fail [N=%d]: rc=%d (%d,%d), expected "
                 "TA_SUCCESS (%d,1)\n",
                 period, (int)retCode, begIdx, nbElement, lookback );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inReal, bitwise.
 *
 * Both the trailing window cursor and the displaced one read behind the write
 * cursor, and either can land exactly on it: the displaced one whenever
 * startIdx equals the displacement (n == 2, 3 and 4), the trailing one whenever
 * startIdx sits at the lookback. That is the whole invariant, and it holds only
 * because both reads precede the store.
 */
static ErrorNumber test_dpo_aliasing( const char *tag, const TA_Real *in, int nbBars )
{
   static TA_Real clean[DPO_CAP], alias[DPO_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int period, i;

   for( period = 2; period <= 60; period++ )
   {
      retCode = TA_DPO( 0, nbBars-1, in, period, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "DPO alias Fail [%s N=%d]: rc=%d\n", tag, period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_DPO( 0, nbBars-1, alias, period, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "DPO alias Fail [%s N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 tag, period, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_dpoAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "DPO alias Fail [%s N=%d] out %d: separate %.17g, in-place "
                    "%.17g -- a store landed under a read the same bar still "
                    "needed\n", tag, period, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The startIdx/endIdx range sweep. TA_STABLE_EPSILON, the class of a
 * finite window carried by a running accumulator: re-anchoring reaches the same
 * window through a different add/subtract order, so a value may move in its
 * last bits. No unstable-period id (matching its abstract metadata, which
 * doRangeTestEx cross-checks against the stability class). */
typedef struct { int period; const TA_Real *in; } DpoRangeParam;

static TA_RetCode dpoRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   DpoRangeParam *p = (DpoRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_DPO_Lookback( p->period );
   return TA_DPO( startIdx, endIdx, p->in, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_dpo_range( const TA_Real *in )
{
   DpoRangeParam param;

   param.period = 20;
   param.in     = in;

   return doRangeTestEx( dpoRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
