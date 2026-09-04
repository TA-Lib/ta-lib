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
 *  090426 MF,CC  First version (issue #348).
 */

/* Description:
 *
 *   Test TA_RMA (Wilder's Smoothed Moving Average).
 *
 *   Legs:
 *     1. DIFFERENTIAL, memcmp-exact: TA_RMA over TA_TRANGE IS TA_ATR. Both
 *        carry the same seed window, the same beta-first coefficient pair and
 *        the same fused step, so nothing is left for a tolerance to absorb.
 *        This is the only gate in the tree that sees the coefficient SPELLING,
 *        the fusion, and the fused operand ORDER -- the last of which no static
 *        gate catches. Periods 1 and 2 are vacuous for the spelling (the two
 *        coefficient orders give identical doubles there); the leg is never
 *        narrowed to them.
 *     2. EXTERNAL ORACLE, form 1: pandas-ta-classic 0.6.52 `ta.rma`, which is
 *        SMA-seeded at bar N-1 and steps `alpha*x + (1-alpha)*prev` with
 *        alpha = 1/N -- our recurrence, at the canonical alpha rather than
 *        ours, so the arm is independent of the spelling decision.
 *     3. EXTERNAL ORACLE, form 2: Tulip Indicators 0.9.2 `wilders`, the same
 *        average written `prev + (x-prev)*(1/N)`. A different arithmetic form
 *        AND a different codebase from leg 2, so the two agreeing is not one
 *        formula checked twice.
 *     4. Two PUBLISHED vectors, reproduced at their printed precision.
 *     5. Period 1 is the input, bitwise; period 2 is pinned alongside it.
 *     6. All-flat input returns the constant exactly and never NaN (#112).
 *     7. In-place aliasing, outReal == inReal, bitwise.
 *     8. The startIdx/endIdx range sweep, in the CONVERGING class.
 *
 *   Legs 2 and 3 are compared with checkOracleValue -- an absolute term
 *   ALONGSIDE the relative one, not instead of it. That is load-bearing here
 *   and measured, not defensive: on the sign-crossing series both oracles pass
 *   within ~1e-15 absolute while the RELATIVE error at the bar nearest a zero
 *   crossing reaches 1.1e-12 (pandas) and 2.2e-12 (Tulip). A relative-only
 *   1e-12 would be red on a correct implementation.
 *
 *   Cross-language value coverage comes from server_verify below plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as KC).
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
extern double gDataHigh[];
extern double gDataLow[];
extern double gDataClose[];

/**** Local declarations. ****/
#define RMA_CAP  1024
#define RMA_GD_NB 1000

/* pandas is the same recurrence at the canonical alpha; the residual is its
 * pairwise-summed seed against our sequential one, plus its unfused step. Two
 * decimal orders of headroom over the measured worst (6.6e-16 relative on the
 * price corpus, 1.1e-15 absolute on the sign-crossing one). */
#define RMA_PANDAS_REL 1e-12
#define RMA_PANDAS_ABS 1e-12
/* Tulip is a different arithmetic form, so it lands closer to us than pandas
 * does, not further: measured 1.6e-16 relative and 1.8e-15 absolute. */
#define RMA_TULIP_REL  1e-14
#define RMA_TULIP_ABS  1e-13

typedef struct { int period; int bar; double want; } RmaGolden;

/* A sign-crossing series in exact eighths, built from the rule
 *    x[i] = ((i*37) mod 401 - 200) / 8
 * so the oracle and this file construct the identical doubles rather than
 * transporting a table. 37 and 401 are coprime and 401 is prime, so the
 * residues are a permutation: the series spans [-25, +25.125], crosses zero
 * often, and its RMA passes within 2e-4 of zero -- which is what makes the
 * absolute companion above measurable rather than notional. */
static void rmaBuildCrossing( double *dest, int nb )
{
   int i;
   for( i = 0; i < nb; i++ )
      dest[i] = (double)( ( ( i * 37 ) % 401 ) - 200 ) / 8.0;
}

/* Goldens captured by ta-lib-oracles/pandas_serve/capture_348_rma.py against
 * pandas-ta-classic 0.6.52 / pandas 3.0.3 / numpy 2.5.1 (CPython 3.12), at
 * %.17g which round-trips to the same double. `bar` is the ABSOLUTE bar index;
 * the output index is bar - begIdx. */
static const RmaGolden rmaPandasClose[] =
{
   {   2,   1,        93.157499999999999 },
   {   2,   2,        93.766249999999999 },
   {   2,  50,        89.753681891184328 },
   {   2, 150,          122.382756893379 },
   {   2, 251,        108.43841071045082 },
   {   5,   4,        93.913000000000011 },
   {   5,   5,        94.055400000000006 },
   {   5,  50,        89.449404360488245 },
   {   5, 150,        123.21853561837347 },
   {   5, 251,        108.94720710690687 },
   {  14,  13,        93.857500000000002 },
   {  14,  14,        93.653392857142862 },
   {  14,  50,        88.541947809318273 },
   {  14, 150,        125.26530097547894 },
   {  14, 251,        108.01489148034045 },
   {  30,  29,        90.426333333333346 },
   {  30,  30,        90.253788888888906 },
   {  30,  50,        89.220004227722384 },
   {  30, 150,        121.33794198613596 },
   {  30, 251,        108.26343017251227 },
};

static const RmaGolden rmaPandasCross[] =
{
   {   2,   1,                  -22.6875 },
   {   2,   2,                 -19.21875 },
   {   2,  28,      0.016792791429907084 },
   {   2,  50,          1.51679286761112 },
   {   2, 150,        12.548998021590284 },
   {   2, 251,       -9.0876282364435745 },
   {   5,   4,                    -15.75 },
   {   5,   5,       -12.975000000000001 },
   {   5,  50,       -1.2505041233389222 },
   {   5, 150,        4.6393391289997012 },
   {   5, 202,   -0.00019917518156553626 },
   {   5, 251,      -0.40535548266954402 },
   {  14,  13,       -5.6785714285714288 },
   {  14,  14,       -6.0140306122448983 },
   {  14,  50,       -1.0137235372549149 },
   {  14, 150,         1.326119112497842 },
   {  14, 175,      0.013146456743032386 },
   {  14, 251,       0.50960581079171319 },
   {  30,  29,       -3.0499999999999998 },
   {  30,  30,       -2.4983333333333331 },
   {  30,  50,       -1.6469231405840379 },
   {  30,  72,     0.0021537358922822103 },
   {  30, 150,       0.74514745909028934 },
   {  30, 251,       0.25077744797727564 },
};

/* Goldens from a SECOND oracle, on the SAME (period, bar) grid: Tulip
 * Indicators 0.9.2 (git be18abb) `wilders`, driven through ta_tulip_serve with
 * the lossless hex-of-IEEE-bits input transport. Tulip writes the step as
 * `prev + (x-prev)*per` with `per = 1/N` a reciprocal MULTIPLY, an
 * algebraically identical average reached by different arithmetic -- so
 * agreeing with leg 2 on the same bars is two implementations, not two
 * wrappers over one. */
static const RmaGolden rmaTulipClose[] =
{
   {   2,   1,        93.157499999999999 },
   {   2,   2,        93.766249999999999 },
   {   2,  50,        89.753681891184328 },
   {   2, 150,          122.382756893379 },
   {   2, 251,        108.43841071045082 },
   {   5,   4,        93.912999999999982 },
   {   5,   5,        94.055399999999992 },
   {   5,  50,        89.449404360488217 },
   {   5, 150,        123.21853561837344 },
   {   5, 251,        108.94720710690683 },
   {  14,  13,        93.857499999999987 },
   {  14,  14,        93.653392857142848 },
   {  14,  50,         88.54194780931823 },
   {  14, 150,        125.26530097547888 },
   {  14, 251,        108.01489148034038 },
   {  30,  29,        90.426333333333332 },
   {  30,  30,        90.253788888888892 },
   {  30,  50,        89.220004227722342 },
   {  30, 150,        121.33794198613595 },
   {  30, 251,        108.26343017251223 },
};

static const RmaGolden rmaTulipCross[] =
{
   {   2,   1,                  -22.6875 },
   {   2,   2,                 -19.21875 },
   {   2,  28,      0.016792791429907036 },
   {   2,  50,        1.5167928676111204 },
   {   2, 150,        12.548998021590286 },
   {   2, 251,       -9.0876282364435763 },
   {   5,   4,                    -15.75 },
   {   5,   5,                   -12.975 },
   {   5,  50,       -1.2505041233389211 },
   {   5, 150,        4.6393391289997012 },
   {   5, 202,   -0.00019917518156531422 },
   {   5, 251,      -0.40535548266954446 },
   {  14,  13,       -5.6785714285714288 },
   {  14,  14,       -6.0140306122448983 },
   {  14,  50,       -1.0137235372549145 },
   {  14, 150,         1.326119112497842 },
   {  14, 175,      0.013146456743032449 },
   {  14, 251,       0.50960581079171385 },
   {  30,  29,       -3.0499999999999998 },
   {  30,  30,       -2.4983333333333331 },
   {  30,  50,       -1.6469231405840383 },
   {  30,  72,      0.002153735892282237 },
   {  30, 150,       0.74514745909028901 },
   {  30, 251,       0.25077744797727597 },
};

#define NB_PANDAS_CLOSE ((int)(sizeof(rmaPandasClose)/sizeof(RmaGolden)))
#define NB_PANDAS_CROSS ((int)(sizeof(rmaPandasCross)/sizeof(RmaGolden)))
#define NB_TULIP_CLOSE  ((int)(sizeof(rmaTulipClose)/sizeof(RmaGolden)))
#define NB_TULIP_CROSS  ((int)(sizeof(rmaTulipCross)/sizeof(RmaGolden)))

/* Coverage counters. Every leg reports nothing on success, so a count that
 * reached zero is the only way one could run without comparing anything. */
static int g_rmaDiffCmp;
static int g_rmaOracleCmp;
static int g_rmaBookCmp;
static int g_rmaPeriod1Cmp;
static int g_rmaAliasCmp;

/* (1) DIFFERENTIAL, memcmp-exact: TA_RMA(TA_TRANGE(h,l,c), N) IS TA_ATR(N).
 *
 * TRANGE's first output is bar 1, so RMA over it begins at bar N-1 of the
 * true-range array == bar N of the price array, which is exactly where ATR
 * begins. Same count, same bars, and no arithmetic left over: the two bodies
 * carry the same seed sum, the same (wBeta, wAlpha) pair derived in the same
 * order, and the same fused step.
 *
 * Both unstable periods are pinned to 0 because the leg only holds when they
 * agree -- RMA's and ATR's are separate knobs over the same recursion.
 */
static ErrorNumber test_rma_differential( const char *tag,
                                          const TA_Real *high,
                                          const TA_Real *low,
                                          const TA_Real *close,
                                          int nbBars )
{
   static const int periods[] = { 1, 2, 3, 14, 30, 100 };
   static TA_Real tr[RMA_CAP], atr[RMA_CAP], rma[RMA_CAP];
   TA_RetCode retCode;
   TA_Integer trBeg, trNb, atrBeg, atrNb, rmaBeg, rmaNb;
   int pi, i;

   if( nbBars > RMA_CAP )
      nbBars = RMA_CAP;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );

   retCode = TA_TRANGE( 0, nbBars - 1, high, low, close, &trBeg, &trNb, tr );
   if( retCode != TA_SUCCESS || trBeg != 1 )
   {
      printf( "RMA differential Fail [%s]: TRANGE rc=%d beg=%d\n",
              tag, (int)retCode, trBeg );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( pi = 0; pi < (int)(sizeof(periods)/sizeof(periods[0])); pi++ )
   {
      int N = periods[pi];

      if( N + 1 > nbBars )
         continue;

      retCode = TA_ATR( 0, nbBars - 1, high, low, close, N,
                        &atrBeg, &atrNb, atr );
      if( retCode != TA_SUCCESS )
      {
         printf( "RMA differential Fail [%s N=%d]: ATR rc=%d\n",
                 tag, N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_RMA( 0, trNb - 1, tr, N, &rmaBeg, &rmaNb, rma );
      if( retCode != TA_SUCCESS )
      {
         printf( "RMA differential Fail [%s N=%d]: RMA rc=%d\n",
                 tag, N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* ATR anchors at bar N of the prices; RMA at bar N-1 of the true
       * ranges, which is the same bar. Both must therefore report the same
       * element count, and the outputs align index-for-index. */
      if( atrNb != rmaNb || atrBeg != rmaBeg + 1 )
      {
         printf( "RMA differential Fail [%s N=%d]: shape ATR(%d,%d) RMA(%d,%d)\n",
                 tag, N, atrBeg, atrNb, rmaBeg, rmaNb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      if( memcmp( atr, rma, (size_t)atrNb * sizeof(TA_Real) ) != 0 )
      {
         for( i = 0; i < atrNb; i++ )
            if( atr[i] != rma[i] )
            {
               printf( "RMA differential Fail [%s N=%d] out %d: "
                       "RMA(TRANGE)=%.17g ATR=%.17g\n",
                       tag, N, i, rma[i], atr[i] );
               break;
            }
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_rmaDiffCmp += atrNb;
   }

   return TA_TEST_PASS;
}

/* (2)+(3) The two external oracles, on one series.
 *
 * Only the pandas legs ask the language servers. The Tulip table is the same
 * (period, bar) grid, so a second pass would replay identical calls. */
static ErrorNumber test_rma_oracle( const char *oracle, const char *series,
                                    const TA_Real *in, int nbBars,
                                    const RmaGolden *gold, int nbGold,
                                    double relTol, double absTol,
                                    int verifyServers )
{
   static TA_Real out[RMA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int k, lastPeriod = -1;

   begIdx = 0; nbElement = 0;

   for( k = 0; k < nbGold; k++ )
   {
      double got, err;
      const char *mode;

      if( gold[k].period != lastPeriod )
      {
         lastPeriod = gold[k].period;
         retCode = TA_RMA( 0, nbBars - 1, in, lastPeriod,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod - 1
             || nbElement != nbBars - ( lastPeriod - 1 ) )
         {
            printf( "RMA %s/%s Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    oracle, series, lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod - 1, nbBars - ( lastPeriod - 1 ) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( verifyServers && server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "RMA", 0, nbBars - 1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ in, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "RMA %s/%s [N=%d]: compared no server despite live "
                       "pipes\n", oracle, series, lastPeriod );
               return TA_RMA_ORACLE_VACUOUS;
            }
         }
      }

      /* Every row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( gold[k].bar < begIdx || gold[k].bar - begIdx >= nbElement )
      {
         printf( "RMA %s/%s Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", oracle, series, gold[k].period, gold[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_RMA_ORACLE_VACUOUS;
      }

      got = out[gold[k].bar - begIdx];
      g_rmaOracleCmp++;
      if( !checkOracleValue( got, gold[k].want, relTol, absTol, &err, &mode ) )
      {
         printf( "RMA %s/%s Fail [N=%d] at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol rel %g abs %g)\n",
                 oracle, series, gold[k].period, gold[k].bar, got, gold[k].want,
                 mode, err, relTol, absTol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The two PUBLISHED vectors, at their printed precision.
 *
 * Neither is ours and neither was recomputed here: the first is upstream
 * Tulip's own committed expected output, the second is a table printed in a
 * book two decades before this file. They pin the seed window and the anchor
 * against a source that has no idea how we spell the recurrence.
 */
typedef struct
{
   const char   *source;
   int           nbIn;
   const double *in;
   int           nbOut;
   const double *out;
   int           decimals;
} RmaBookVector;

/* tests/untest.txt:483 of Tulip Indicators 0.9.2 (`wilders 5`), upstream's own
 * committed expected output, printed at 3 decimals. */
static const double rmaTulipVecIn[15] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const double rmaTulipVecOut[11] =
   { 82.426, 82.571, 82.625, 82.898, 83.228, 83.455, 83.870,
     84.404, 84.901, 85.475, 85.838 };

/* Steven B. Achelis, "Technical Analysis from A to Z", page 366, transcribed
 * into Tulip's tests/atoz.txt:296 (`wilders 5`, `#page 366`), 4 decimals. */
static const double rmaAtozVecIn[12] =
   { 62.125, 61.125, 62.3438, 65.3125, 63.9688, 63.4375,
     63, 63.7812, 63.4062, 63.4062, 62.4375, 61.8438 };
static const double rmaAtozVecOut[8] =
   { 62.975, 63.0675, 63.054, 63.1995, 63.2408, 63.2739, 63.1066, 62.8540 };

static const RmaBookVector rmaBookVectors[] =
{
   { "Tulip Indicators 0.9.2 tests/untest.txt:483", 15, rmaTulipVecIn, 11, rmaTulipVecOut, 3 },
   { "Achelis, TA from A to Z p.366 (atoz.txt:296)", 12, rmaAtozVecIn,  8, rmaAtozVecOut,  4 },
};
#define NB_BOOK_VECTORS ((int)(sizeof(rmaBookVectors)/sizeof(RmaBookVector)))

static ErrorNumber test_rma_published( void )
{
   static TA_Real out[RMA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int v, i;

   for( v = 0; v < NB_BOOK_VECTORS; v++ )
   {
      const RmaBookVector *bv = &rmaBookVectors[v];
      double quantum = pow( 10.0, (double)bv->decimals );

      retCode = TA_RMA( 0, bv->nbIn - 1, bv->in, 5, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 4 || nbElement != bv->nbOut )
      {
         printf( "RMA published Fail [%s]: rc=%d (%d,%d) expected (4,%d)\n",
                 bv->source, (int)retCode, begIdx, nbElement, bv->nbOut );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double rounded = floor( out[i] * quantum + 0.5 ) / quantum;

         g_rmaBookCmp++;
         if( rounded != bv->out[i] )
         {
            printf( "RMA published Fail [%s] out %d: got %.10f -> %.*f, "
                    "expected %.*f\n",
                    bv->source, i, out[i], bv->decimals, rounded,
                    bv->decimals, bv->out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) Period 1 is the input, bitwise, and period 2 is pinned alongside it.
 *
 * test_period_boundary.c already sweeps every function carrying
 * TA_FUNC_FLG_PERIOD1_IDENTITY, but all three of its series are strictly
 * positive two-decimal prices. RMA's documented input is an arbitrary
 * oscillator, so the copy is re-checked here on a series that crosses zero.
 * Known hole, shared with SMA and TRIMA and not fixed here: a -0.0 input comes
 * back +0.0, because the seed sum starts from a literal 0.0.
 *
 * Period 2 is the smallest period at which the recurrence actually runs; its
 * coefficients are the same doubles under either spelling, so this pins the
 * shape (anchor, count, seed) and not the spelling.
 */
static ErrorNumber test_rma_period_one( const TA_Real *in, int nbBars )
{
   static TA_Real out[RMA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   if( nbBars > RMA_CAP )
      nbBars = RMA_CAP;

   retCode = TA_RMA( 0, nbBars - 1, in, 1, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != nbBars )
   {
      printf( "RMA period-1 Fail: rc=%d (%d,%d) expected (0,%d)\n",
              (int)retCode, begIdx, nbElement, nbBars );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( TA_RMA_Lookback( 1 ) != 0 )
   {
      printf( "RMA period-1 Fail: lookback %d, expected 0\n",
              TA_RMA_Lookback( 1 ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
   {
      g_rmaPeriod1Cmp++;
      if( out[i] != in[i] )
      {
         printf( "RMA period-1 Fail at %d: got %.17g, input %.17g\n",
                 i, out[i], in[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Period 2: the average of the two most recent bars at the anchor, then
    * halves thereafter. Both are exact in binary. */
   retCode = TA_RMA( 0, nbBars - 1, in, 2, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != nbBars - 1 )
   {
      printf( "RMA period-2 Fail: rc=%d (%d,%d) expected (1,%d)\n",
              (int)retCode, begIdx, nbElement, nbBars - 1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   {
      double prev = ( in[0] + in[1] ) / 2.0;

      for( i = 0; i < nbElement; i++ )
      {
         g_rmaPeriod1Cmp++;
         if( out[i] != prev )
         {
            printf( "RMA period-2 Fail at %d: got %.17g expected %.17g\n",
                    i, out[i], prev );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( i + 1 < nbElement )
            prev = 0.5 * in[i + 2] + 0.5 * prev;
      }
   }

   return TA_TEST_PASS;
}

/* (6) A flat input returns the constant exactly, and never NaN (#112).
 *
 * There is no data division anywhere in the body -- the only divide is the
 * seed's `/ period`, with period >= 1 -- so no NaN or Inf path exists to
 * begin with. This pins that, and pins that the fused step reproduces a
 * constant exactly rather than drifting off it. */
static ErrorNumber test_rma_flat( void )
{
   static TA_Real flat[64];
   static TA_Real out[RMA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, pi;
   static const int periods[] = { 1, 2, 3, 14, 64 };

   for( i = 0; i < 64; i++ )
      flat[i] = 100.0;

   for( pi = 0; pi < (int)(sizeof(periods)/sizeof(periods[0])); pi++ )
   {
      int N = periods[pi];

      retCode = TA_RMA( 0, 63, flat, N, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != N - 1 || nbElement != 64 - (N-1) )
      {
         printf( "RMA flat Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 N, (int)retCode, begIdx, nbElement, N - 1, 64 - (N-1) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         if( out[i] != 100.0 )
         {
            printf( "RMA flat Fail [N=%d] at %d: %.17g != 100.0\n",
                    N, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* A period longer than the history: zero output, TA_SUCCESS. */
   retCode = TA_RMA( 0, 63, flat, 65, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 0 || begIdx != 0 )
   {
      printf( "RMA oversize-period Fail: rc=%d (%d,%d) expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/* (7) In-place aliasing, outReal == inReal, bitwise.
 *
 * Safe by construction: the value written at output index k is the bar
 * startIdx+k, which the body has already read, and every remaining read is at
 * a strictly greater bar. */
static ErrorNumber test_rma_aliasing( const TA_Real *in, int nbBars )
{
   static TA_Real ref[RMA_CAP], buf[RMA_CAP];
   static const int periods[] = { 1, 2, 14, 30 };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, aBeg, aNb;
   int pi, i;

   if( nbBars > RMA_CAP )
      nbBars = RMA_CAP;

   for( pi = 0; pi < (int)(sizeof(periods)/sizeof(periods[0])); pi++ )
   {
      int N = periods[pi];

      retCode = TA_RMA( 0, nbBars - 1, in, N, &begIdx, &nbElement, ref );
      if( retCode != TA_SUCCESS )
      {
         printf( "RMA aliasing Fail [N=%d]: reference rc=%d\n", N, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      memcpy( buf, in, (size_t)nbBars * sizeof(TA_Real) );
      retCode = TA_RMA( 0, nbBars - 1, buf, N, &aBeg, &aNb, buf );
      if( retCode != TA_SUCCESS || aBeg != begIdx || aNb != nbElement )
      {
         printf( "RMA aliasing Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 N, (int)retCode, aBeg, aNb, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rmaAliasCmp++;
         if( buf[i] != ref[i] )
         {
            printf( "RMA aliasing Fail [N=%d] out %d: %.17g != %.17g\n",
                    N, i, buf[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. RMA is an IIR recursion whose output
 * depends on how far back it started, so it is TA_STABLE_CONVERGING with its
 * own unstable id: the residual between two anchors decays by a factor of
 * (1 - 1/period) per bar, which is the envelope that class assumes. */
typedef struct { int period; const TA_Real *in; } RmaRangeParam;

static TA_RetCode rmaRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   RmaRangeParam *p = (RmaRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_RMA_Lookback( p->period );
   return TA_RMA( startIdx, endIdx, p->in, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rma_range( const TA_Real *in )
{
   RmaRangeParam param;

   param.period = 14;
   param.in     = in;

   return doRangeTestEx( rmaRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_RMA,
                         (void *)&param, 1, 0 );
}

/**** Global functions definitions.   ****/
ErrorNumber test_func_rma( TA_History *history )
{
   static TA_Real crossing[RMA_CAP];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_rmaDiffCmp = g_rmaOracleCmp = g_rmaBookCmp = 0;
   g_rmaPeriod1Cmp = g_rmaAliasCmp = 0;

   rmaBuildCrossing( crossing, nbBars > RMA_CAP ? RMA_CAP : nbBars );

   err = test_rma_differential( "TA_SREF", history->high, history->low,
                                history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rma_differential( "gData", gDataHigh, gDataLow, gDataClose,
                                RMA_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   /* The goldens were captured on the 252-bar corpus; a different history
    * makes them meaningless rather than wrong, so say so and skip. */
   if( nbBars == 252 )
   {
      err = test_rma_oracle( "pandas-ta-classic 0.6.52", "close",
                             history->close, nbBars,
                             rmaPandasClose, NB_PANDAS_CLOSE,
                             RMA_PANDAS_REL, RMA_PANDAS_ABS, 1 );
      if( err != TA_TEST_PASS )
         return err;

      err = test_rma_oracle( "pandas-ta-classic 0.6.52", "sign-crossing",
                             crossing, nbBars,
                             rmaPandasCross, NB_PANDAS_CROSS,
                             RMA_PANDAS_REL, RMA_PANDAS_ABS, 1 );
      if( err != TA_TEST_PASS )
         return err;

      err = test_rma_oracle( "Tulip Indicators 0.9.2", "close",
                             history->close, nbBars,
                             rmaTulipClose, NB_TULIP_CLOSE,
                             RMA_TULIP_REL, RMA_TULIP_ABS, 0 );
      if( err != TA_TEST_PASS )
         return err;

      err = test_rma_oracle( "Tulip Indicators 0.9.2", "sign-crossing",
                             crossing, nbBars,
                             rmaTulipCross, NB_TULIP_CROSS,
                             RMA_TULIP_REL, RMA_TULIP_ABS, 0 );
      if( err != TA_TEST_PASS )
         return err;
   }
   else
   {
      printf( "RMA oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
   }

   err = test_rma_published();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rma_period_one( crossing, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rma_flat();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rma_aliasing( history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rma_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Every leg above is silent on success, so a count that reached zero is the
    * only remaining way one could run without comparing anything. LITERAL
    * counts rather than floors, because on the shipped 252-bar corpus every
    * one of them is deterministic: 7212 differential values (the six periods
    * over 252 bars and over gData's 1000), 88 oracle rows, 19 published
    * values, 503 period-1/2 copies and 965 aliased values. */
   if( nbBars == 252
       && ( g_rmaDiffCmp != 7212 || g_rmaOracleCmp != 88 || g_rmaBookCmp != 19
            || g_rmaPeriod1Cmp != 503 || g_rmaAliasCmp != 965 ) )
   {
      printf( "RMA Fail: coverage counters (diff %d, oracle %d, published %d, "
              "period1 %d, alias %d) are not what this file was written with "
              "(7212, 88, 19, 503, 965)\n",
              g_rmaDiffCmp, g_rmaOracleCmp, g_rmaBookCmp,
              g_rmaPeriod1Cmp, g_rmaAliasCmp );
      return TA_RMA_ORACLE_VACUOUS;
   }

   return TA_TEST_PASS;
}
