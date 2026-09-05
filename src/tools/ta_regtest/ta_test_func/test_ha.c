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
 *  090526 MF,CC  First version (issue #373).
 */

/* Description:
 *
 *   Test TA_HA (Heikin-Ashi Candles).
 *
 *   Legs:
 *     1. TWO EXTERNAL ORACLES on the 252-bar TA_SREF OHLC, at tolerance ZERO.
 *        Every operation is an addition or a division by an exact power of
 *        two, in a fixed order, so a correct implementation reproduces the
 *        oracles' doubles bit for bit on any IEEE-754 platform -- there is no
 *        rounding for a tolerance to absorb. Replayed on every language server.
 *     2. The same two oracles on a corpus whose bar 0 has O+C != H+L. That is the
 *        ONLY input that can see a seed regression: where O+C == H+L, the
 *        published (O+C)/2 seed and ta4j's raw-bar-0 convention agree from bar
 *        1 onward, and TA_SREF bar 0 happens to satisfy it exactly.
 *     3. HA_close against TA_AVGPRICE, which is the same four-price average in
 *        a DIFFERENT summation order. Close at 1e-13, and asserted NOT
 *        bit-identical everywhere: the operand order is the bit-exactness
 *        contract with every external implementation, so unifying the two
 *        would silently trade it away.
 *     4. Exact edges: a flat bar reproduces its own price in all four outputs,
 *        and no output can leave [low, high] of its own bar.
 *     5. In-place aliasing, all 4 outputs x 4 inputs, bitwise. Each bar's four
 *        prices are read into locals before any store, and at startIdx 0 the
 *        store lands on the very slot they came from.
 *     6. The unstable period. HA is the corpus's first function whose lookback
 *        is ONLY the unstable period. A request anchored at bar 0 seeds at bar
 *        0 whatever the setting, so raising it only withholds warm-up bars:
 *        the values that remain must be bit-identical to the unfiltered run.
 *     7. Signed zeros. HA is the first function whose emitted VALUE is a
 *        three-way extremum, and the four backends' max/min primitives
 *        disagree on -0.0 -- so the extremum is spelled with plain
 *        comparisons and this leg holds all four languages to the same bytes.
 *     8. The startIdx/endIdx range sweep, TA_STABLE_CONVERGING: HA_open is
 *        recursive and its seed's influence halves every bar.
 *
 *   Cross-language value coverage is server_verify in legs 1, 2 and 7 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"
#include "../../ta_alloc_check.h"

/**** External variables declarations. ****/
extern double gDataOpen[];
extern double gDataHigh[];
extern double gDataLow[];
extern double gDataClose[];

/**** Local declarations. ****/
#define HA_CAP      1024
#define HA_GD_NB    1000
#define HA_SEED_NB    64
#define HA_ZERO_NB     8

/* Leg 3. TA_AVGPRICE is ((H+L)+C)+O where HA_close is ((O+H)+L)+C: the same
 * quantity, one rounding apart. Measured over the two corpora swept below, the
 * worst gap is 2.9e-14 absolute / 3.3e-16 relative, at prices near 100. The
 * relative term is what binds; the absolute companion is there so the leg does
 * not become vacuous on a corpus that crosses zero. */
#define HA_AVG_REL 1e-13
#define HA_AVG_ABS 1e-12

typedef struct { int bar; double open; double high; double low; double close; } HaGolden;

/* Captured by running pandas-ta-classic 0.6.52 `ta.ha()` (pandas 3.0.3, numpy
 * 2.5.1) on the 252-bar TA_SREF OHLC (TA_SREF_{open,high,low,close}_daily_ref_0
 * _PRIV), 2026-09-05, printed at %.17g so every literal round-trips to the same
 * double. `bar` is the ABSOLUTE bar index.
 *
 * A SECOND implementation, in a different library and reverse-engineered from a
 * different vendor's semantics, was driven over the same bars on the same day
 * and agrees BIT FOR BIT on all 1008 values: PyneCore 6.8.14
 * (`pynecore.core.security_process._heikinashi_step`, TradingView Pine v6.8's
 * `ticker.heikinashi()` bar transform). It is non-circular by construction --
 * that package declares no dependencies at all, so no TA-Lib binding can be
 * anywhere beneath it. The two also agree bit for bit over the seed corpus
 * below, all 256 values.
 *
 * TOLERANCE ZERO, and that is a property of the indicator rather than a boast:
 * the only operations are additions and divisions by 2 and 4, so every value is
 * the correctly-rounded result of a fixed expression and two conforming
 * implementations cannot differ by a bit.
 *
 * The rows cover both operands that can win the extremum on this corpus: bars
 * 2, 7, 17, 50 and 137 are where HA_open displaces the raw high or low. The
 * synthetic close never wins, and cannot -- it is the mean of the same bar's
 * four prices, so it always lies between that bar's low and high. */
static const HaGolden haSrefOracle[] =
{
   {   0,                       92,                    93.25,                    90.75,                       92 },
   {   1,                       92,       94.939999999999998,       91.405000000000001,       93.165000000000006 },
   {   2,        92.58250000000001,                   96.375,        92.58250000000001,       95.038749999999993 },
   {   3,       93.810625000000002,       96.189999999999998,                     93.5,       94.688749999999999 },
   {   7,       93.996992187499998,       93.996992187499998,                    89.75,       91.930000000000007 },
   {  17,       91.648668937683112,       91.648668937683112,       88.780000000000001,       89.632499999999993 },
   {  50,       90.574635830279576,       90.574635830279576,                       89,       89.444999999999993 },
   { 100,        113.9245584532806,                   116.87,                   112.62,                  115.545 },
   { 137,       132.94134950057648,       132.94134950057648,                   126.87,                   128.28 },
   { 199,       107.68744846790369,                      109,                   104.69,                 107.1725 },
   { 250,       109.59657997536647,                    110.5,                   108.56,                  109.375 },
   { 251,       109.48578998768323,                    109.5,                   106.62,                  108.295 },
};
#define NB_HA_SREF ((int)(sizeof(haSrefOracle)/sizeof(HaGolden)))

/* Leg 2, same two oracles and the same capture date, on the corpus
 * haBuildSeed() constructs below. Nothing is transported: the rule is stated in that function
 * and reproduced verbatim in the capture script, so both sides build the same
 * doubles rather than reading a table.
 *
 * Bar 0 is 100.0 / 102.0 / 98.0 / 99.0, so O+C is 199 and H+L is 200. That
 * single inequality is what makes this leg a seed test: the published seed puts
 * HA_open[0] at 99.5 where ta4j's raw-bar-0 convention puts it at 100.0, and
 * the two then differ on every later bar. On TA_SREF they would differ at bar 0
 * and nowhere else. */
static const HaGolden haSeedOracle[] =
{
   {   0,                     99.5,                      102,                       98,                    99.75 },
   {   1,                   99.625,                   103.75,                     99.5,                 101.8125 },
   {   2,                100.71875,                    105.5,                100.71875,                 103.3125 },
   {   3,               102.015625,                   107.25,               102.015625,                 104.8125 },
   {   4,              103.4140625,                      109,              103.4140625,                 106.3125 },
   {   5,             104.86328125,             104.86328125,                   98.125,                100.53125 },
   {   8,          102.83837890625,                  107.125,                  102.625,                 104.8125 },
   {  13,       102.97346496582031,                  107.875,                   102.75,                105.03125 },
   {  21,       103.15480357408524,                  104.125,                      100,                102.03125 },
   {  34,       104.67228681073902,                      110,       104.67228681073902,                 107.3125 },
   {  55,       106.26900576819745,       106.26900576819745,                   99.375,                   101.75 },
   {  63,       104.22639260065702,                  108.375,                      104,                106.03125 },
};
#define NB_HA_SEED ((int)(sizeof(haSeedOracle)/sizeof(HaGolden)))

/* Leg 7. The sign bit of each output on the eight signed-zero bars built by
 * haBuildZeros(). No external oracle can arbitrate this -- measured, pandas'
 * numpy reduction answers a third thing again -- so what is pinned here is the
 * ONE tie rule all four of our backends can share: the extremum starts at the
 * raw high (or low) and moves only on a STRICTLY greater (or lesser) candidate,
 * which is why the two-argument max/min builtins are not used. C's macros
 * return their second operand on a tie; Rust, Java and .NET return the negative
 * zero; --xlang-hash compares raw bytes. */
static const int haZeroSignBits[HA_ZERO_NB][4] =
{
   { 1, 0, 1, 0 },
   { 0, 1, 0, 0 },
   { 0, 1, 1, 0 },
   { 0, 1, 1, 1 },
   { 0, 0, 0, 0 },
   { 0, 0, 0, 0 },
   { 0, 1, 0, 0 },
   { 0, 0, 1, 0 },
};

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_haOracleCmp;
static int g_haAvgCmp;
static int g_haEdgeCmp;
static int g_haAliasCmp;
static int g_haUnstCmp;
static int g_haZeroCmp;

/**** Local functions declarations. ****/
static void haBuildSeed( double *o, double *h, double *l, double *c, int nb );
static void haBuildZeros( double *o, double *h, double *l, double *c );
static ErrorNumber haCheckOracle( const char *tag,
                                  const double *o, const double *h,
                                  const double *l, const double *c, int nbBars,
                                  const HaGolden *golden, int nbGolden );
static ErrorNumber test_ha_avgprice( const char *tag, const double *o,
                                     const double *h, const double *l,
                                     const double *c, int nbBars );
static ErrorNumber test_ha_edges( void );
static ErrorNumber test_ha_aliasing( const char *tag, const double *o,
                                     const double *h, const double *l,
                                     const double *c, int nbBars );
static ErrorNumber test_ha_unstable( const TA_History *history );
static ErrorNumber test_ha_zeros( void );
static ErrorNumber test_ha_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_ha( TA_History *history )
{
   static double seedO[HA_SEED_NB], seedH[HA_SEED_NB];
   static double seedL[HA_SEED_NB], seedC[HA_SEED_NB];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_haOracleCmp = g_haAvgCmp = g_haEdgeCmp = 0;
   g_haAliasCmp = g_haUnstCmp = g_haZeroCmp = 0;

   haBuildSeed( seedO, seedH, seedL, seedC, HA_SEED_NB );

   if( nbBars == 252 )
   {
      err = haCheckOracle( "TA_SREF OHLC", history->open, history->high,
                           history->low, history->close, nbBars,
                           haSrefOracle, NB_HA_SREF );
      if( err != TA_TEST_PASS )
         return err;
   }

   err = haCheckOracle( "exact-eighths seed corpus", seedO, seedH, seedL, seedC,
                        HA_SEED_NB, haSeedOracle, NB_HA_SEED );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_avgprice( "TA_SREF OHLC", history->open, history->high,
                           history->low, history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_avgprice( "gData OHLC", gDataOpen, gDataHigh, gDataLow,
                           gDataClose, HA_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_aliasing( "TA_SREF OHLC", history->open, history->high,
                           history->low, history->close, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_aliasing( "gData OHLC", gDataOpen, gDataHigh, gDataLow,
                           gDataClose, HA_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_unstable( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_zeros();
   if( err != TA_TEST_PASS )
      return err;

   err = test_ha_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every leg
    * above is deterministic. */
   if( nbBars == 252
       && ( g_haOracleCmp != (NB_HA_SREF + NB_HA_SEED) * 4
            || g_haAvgCmp != 1252 || g_haEdgeCmp != 1280
            || g_haAliasCmp != 80128 || g_haUnstCmp != 37040
            || g_haZeroCmp != HA_ZERO_NB * 4 ) )
   {
      printf( "HA Fail: coverage counters (oracle %d, avgprice %d, edges %d, "
              "alias %d, unstable %d, zeros %d) are not what this file was "
              "written with (%d, 1252, 1280, 80128, 37040, %d)\n",
              g_haOracleCmp, g_haAvgCmp, g_haEdgeCmp, g_haAliasCmp,
              g_haUnstCmp, g_haZeroCmp,
              (NB_HA_SREF + NB_HA_SEED) * 4, HA_ZERO_NB * 4 );
      return TA_HA_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Every price is a multiple of 1/8 near 100, so the corpus is stated by this
 * rule rather than transported, and the capture script carries the same six
 * lines. Bar 0 is the point of the whole thing: O+C != H+L. */
static void haBuildSeed( double *o, double *h, double *l, double *c, int nb )
{
   int i;

   for( i = 0; i < nb; i++ )
   {
      double b = 100.0 + 0.125 * (double)( ( i * 13 ) % 64 );

      o[i] = b;
      c[i] = b + 0.25 * (double)( ( ( i * 7 ) % 9 ) - 4 );
      h[i] = b + 2.0 + 0.125 * (double)( i % 7 );
      l[i] = b - 2.0 - 0.125 * (double)( i % 5 );
   }
}

/* Eight bars covering every mix of +-0.0 that can reach the extremum: the
 * carried pair is a signed zero exactly when its own operands all are, so the
 * recursion propagates the sign as well as the ties. */
static void haBuildZeros( double *o, double *h, double *l, double *c )
{
   static const int neg[HA_ZERO_NB][4] =
   {
      { 1, 0, 1, 1 }, { 1, 1, 0, 1 }, { 0, 1, 1, 0 }, { 1, 1, 1, 1 },
      { 0, 0, 0, 0 }, { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 1, 0, 1, 0 },
   };
   int i;

   for( i = 0; i < HA_ZERO_NB; i++ )
   {
      o[i] = neg[i][0] ? -0.0 : 0.0;
      h[i] = neg[i][1] ? -0.0 : 0.0;
      l[i] = neg[i][2] ? -0.0 : 0.0;
      c[i] = neg[i][3] ? -0.0 : 0.0;
   }
}

/* (1) and (2). The frozen pandas rows, compared with `!=` rather than a
 * tolerance, plus the cross-language replay of the same call. */
static ErrorNumber haCheckOracle( const char *tag,
                                  const double *o, const double *h,
                                  const double *l, const double *c, int nbBars,
                                  const HaGolden *golden, int nbGolden )
{
   static TA_Real outO[HA_CAP], outH[HA_CAP], outL[HA_CAP], outC[HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int k, f;

   retCode = TA_HA( 0, nbBars-1, o, h, l, c, &begIdx, &nbElement,
                    outO, outH, outL, outC );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != nbBars )
   {
      printf( "HA oracle Fail [%s]: rc=%d (%d,%d) expected (0,%d)\n",
              tag, (int)retCode, begIdx, nbElement, nbBars );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( server_verify_active() )
   {
      ErrorNumber e;
      int cmpBefore = server_verify_comparisons();

      e = server_verify( "HA", 0, nbBars-1, nbBars,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ o, h, l, c, NULL },
                         NULL, 0,
                         (const TA_Real*[]){ outO, outH, outL, outC, NULL },
                         NULL );
      if( e != TA_TEST_PASS )
         return e;
      /* "No failure reported" and "nothing was compared" are the same
       * observation without this. */
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "HA oracle [%s]: compared no server despite live pipes\n", tag );
         return TA_HA_VACUOUS;
      }
   }

   for( k = 0; k < nbGolden; k++ )
   {
      const double *got[4];
      double want[4];

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar past the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( golden[k].bar < begIdx || golden[k].bar - begIdx >= nbElement )
      {
         printf( "HA oracle Fail [%s]: golden bar %d is outside the output "
                 "[%d..%d]\n", tag, golden[k].bar, begIdx,
                 begIdx + nbElement - 1 );
         return TA_HA_VACUOUS;
      }

      got[0] = &outO[golden[k].bar - begIdx];
      got[1] = &outH[golden[k].bar - begIdx];
      got[2] = &outL[golden[k].bar - begIdx];
      got[3] = &outC[golden[k].bar - begIdx];
      want[0] = golden[k].open;
      want[1] = golden[k].high;
      want[2] = golden[k].low;
      want[3] = golden[k].close;

      for( f = 0; f < 4; f++ )
      {
         g_haOracleCmp++;
         if( *got[f] != want[f] )
         {
            static const char *names[4] = { "open", "high", "low", "close" };
            printf( "HA oracle Fail [%s] bar %d HA_%s: got %.17g expected "
                    "%.17g -- this comparison carries NO tolerance, because "
                    "every operation is an add or a divide by a power of two\n",
                    tag, golden[k].bar, names[f], *got[f], want[f] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) HA_close against TA_AVGPRICE.
 *
 * Same quantity, different summation order. The two must stay close, and must
 * NOT stay identical: the published order is what makes leg 1 a zero-tolerance
 * comparison against every external implementation, and rewriting HA to reuse
 * AVGPRICE's order would trade that away without moving any of the tolerances
 * in this file.
 */
static ErrorNumber test_ha_avgprice( const char *tag, const double *o,
                                     const double *h, const double *l,
                                     const double *c, int nbBars )
{
   static TA_Real outO[HA_CAP], outH[HA_CAP], outL[HA_CAP], outC[HA_CAP];
   static TA_Real avg[HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begAvg, nbAvg;
   int i, nbDifferent = 0;

   if( TA_AVGPRICE( 0, nbBars-1, o, h, l, c, &begAvg, &nbAvg, avg ) != TA_SUCCESS )
   {
      printf( "HA avgprice Fail [%s]: reference call failed\n", tag );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_HA( 0, nbBars-1, o, h, l, c, &begIdx, &nbElement,
                    outO, outH, outL, outC );
   if( retCode != TA_SUCCESS || begIdx != begAvg || nbElement != nbAvg )
   {
      printf( "HA avgprice Fail [%s]: rc=%d (%d,%d) vs AVGPRICE (%d,%d)\n",
              tag, (int)retCode, begIdx, nbElement, begAvg, nbAvg );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbElement; i++ )
   {
      double err;
      const char *mode;

      g_haAvgCmp++;
      if( !checkOracleValue( outC[i], avg[i], HA_AVG_REL, HA_AVG_ABS,
                             &err, &mode ) )
      {
         printf( "HA avgprice Fail [%s] out %d: HA_close %.17g vs AVGPRICE "
                 "%.17g (%s err %.3g, tol rel %g abs %g)\n",
                 tag, i, outC[i], avg[i], mode, err, HA_AVG_REL, HA_AVG_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( outC[i] != avg[i] )
         nbDifferent++;
   }

   if( nbDifferent == 0 )
   {
      printf( "HA avgprice Fail [%s]: HA_close is bit-identical to TA_AVGPRICE "
              "on all %d bars. The summation orders differ ((O+H)+L+C versus "
              "(H+L)+C+O), so one of them has been changed and leg 1's "
              "zero-tolerance oracle no longer means what it says\n",
              tag, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) Exact edges.
 *
 *   - A flat bar (O == H == L == C) has nothing to average and nothing to
 *     clamp, so all four outputs are exactly that price, forever. Exact, not
 *     approximate: the average of four identical doubles and the midpoint of
 *     two identical doubles are both that double.
 *   - On arbitrary bars, the synthetic candle must lie inside the raw one:
 *     HA_low <= HA_open, HA_close <= HA_high, and both extremes must contain
 *     the raw high/low respectively. Those are properties of the definition,
 *     not of the arithmetic, so they hold at every bar of every corpus.
 */
static ErrorNumber test_ha_edges( void )
{
   static TA_Real o[256], h[256], l[256], c[256];
   static TA_Real outO[HA_CAP], outH[HA_CAP], outL[HA_CAP], outC[HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, k;
   static const double flat[4] = { 0.0, 1.0, 1234.5678, 1.0e-7 };

   for( k = 0; k < 4; k++ )
   {
      for( i = 0; i < 256; i++ )
         o[i] = h[i] = l[i] = c[i] = flat[k];

      retCode = TA_HA( 0, 255, o, h, l, c, &begIdx, &nbElement,
                       outO, outH, outL, outC );
      if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 256 )
      {
         printf( "HA flat Fail [%g]: rc=%d (%d,%d)\n",
                 flat[k], (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_haEdgeCmp++;
         if( outO[i] != flat[k] || outH[i] != flat[k]
             || outL[i] != flat[k] || outC[i] != flat[k] )
         {
            printf( "HA flat Fail [%g] out %d: %.17g/%.17g/%.17g/%.17g, "
                    "expected the flat price in all four\n",
                    flat[k], i, outO[i], outH[i], outL[i], outC[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* A ramp with a widening range, so every bar has a distinct high and low. */
   for( i = 0; i < 256; i++ )
   {
      o[i] = 50.0 + 0.25 * (double)i;
      c[i] = 50.0 + 0.25 * (double)i + 0.125 * (double)( i % 9 );
      h[i] = 55.0 + 0.25 * (double)i + 0.5 * (double)( i % 5 );
      l[i] = 45.0 + 0.25 * (double)i - 0.5 * (double)( i % 3 );
   }

   retCode = TA_HA( 0, 255, o, h, l, c, &begIdx, &nbElement,
                    outO, outH, outL, outC );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 256 )
   {
      printf( "HA containment Fail: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      g_haEdgeCmp++;
      if( outH[i] < h[i] || outL[i] > l[i]
          || outH[i] < outO[i] || outH[i] < outC[i]
          || outL[i] > outO[i] || outL[i] > outC[i] )
      {
         printf( "HA containment Fail out %d: candle (%.17g,%.17g,%.17g,%.17g) "
                 "does not contain its own open/close or the raw high %.17g / "
                 "low %.17g\n",
                 i, outO[i], outH[i], outL[i], outC[i], h[i], l[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, every output over every input, bitwise. */
static ErrorNumber test_ha_aliasing( const char *tag, const double *o,
                                     const double *h, const double *l,
                                     const double *c, int nbBars )
{
   static TA_Real cleanO[HA_CAP], cleanH[HA_CAP], cleanL[HA_CAP], cleanC[HA_CAP];
   static TA_Real inBuf[4][HA_CAP];
   static TA_Real spare[4][HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   const TA_Real *clean[4];
   int inNb, outNb, i, f;

   retCode = TA_HA( 0, nbBars-1, o, h, l, c, &begIdx, &nbElement,
                    cleanO, cleanH, cleanL, cleanC );
   if( retCode != TA_SUCCESS )
   {
      printf( "HA alias Fail [%s]: reference call rc=%d\n", tag, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   clean[0] = cleanO; clean[1] = cleanH; clean[2] = cleanL; clean[3] = cleanC;

   for( outNb = 0; outNb < 4; outNb++ )
   {
      for( inNb = 0; inNb < 4; inNb++ )
      {
         TA_Real *outs[4];
         const TA_Real *ins[4];

         for( i = 0; i < nbBars; i++ )
         {
            inBuf[0][i] = o[i]; inBuf[1][i] = h[i];
            inBuf[2][i] = l[i]; inBuf[3][i] = c[i];
         }
         for( f = 0; f < 4; f++ )
         {
            ins[f] = inBuf[f];
            outs[f] = spare[f];
         }
         outs[outNb] = inBuf[inNb];

         retCode = TA_HA( 0, nbBars-1, ins[0], ins[1], ins[2], ins[3],
                          &begIdx2, &nbElement2,
                          outs[0], outs[1], outs[2], outs[3] );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx
             || nbElement2 != nbElement )
         {
            printf( "HA alias Fail [%s] out %d over in %d: rc=%d (%d,%d) vs "
                    "(%d,%d)\n", tag, outNb, inNb, (int)retCode,
                    begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         for( f = 0; f < 4; f++ )
         {
            for( i = 0; i < nbElement; i++ )
            {
               g_haAliasCmp++;
               if( outs[f][i] != clean[f][i] )
               {
                  printf( "HA alias Fail [%s] out %d over in %d, output %d "
                          "index %d: separate %.17g, in-place %.17g -- a store "
                          "landed under a read the same bar still needed\n",
                          tag, outNb, inNb, f, i, clean[f][i], outs[f][i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The unstable period.
 *
 * HA's lookback is the unstable period and nothing else, which no other
 * function in the corpus has. A request anchored at bar 0 therefore seeds at
 * bar 0 for every setting -- the setting only decides how many warm-up candles
 * are withheld -- so every value that survives must be bit-identical to the
 * value the setting-0 call put at the same absolute bar. Anything else means
 * the seed moved with the setting.
 */
static ErrorNumber test_ha_unstable( const TA_History *history )
{
   static TA_Real baseO[HA_CAP], baseH[HA_CAP], baseL[HA_CAP], baseC[HA_CAP];
   static TA_Real outO[HA_CAP], outH[HA_CAP], outL[HA_CAP], outC[HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begBase, nbBase;
   int nbBars = (int)history->nbBars;
   unsigned int k;
   int i;

   TA_SetUnstablePeriod( TA_FUNC_UNST_HA, 0 );
   retCode = TA_HA( 0, nbBars-1, history->open, history->high, history->low,
                    history->close, &begBase, &nbBase,
                    baseO, baseH, baseL, baseC );
   if( retCode != TA_SUCCESS || begBase != 0 || nbBase != nbBars )
   {
      printf( "HA unstable Fail: baseline rc=%d (%d,%d)\n",
              (int)retCode, begBase, nbBase );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( k = 1; k <= 40; k++ )
   {
      const TA_Real *base[4];
      const TA_Real *got[4];

      TA_SetUnstablePeriod( TA_FUNC_UNST_HA, k );
      if( TA_HA_Lookback() != (int)k )
      {
         printf( "HA unstable Fail [k=%u]: lookback %d\n", k, TA_HA_Lookback() );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      retCode = TA_HA( 0, nbBars-1, history->open, history->high, history->low,
                       history->close, &begIdx, &nbElement,
                       outO, outH, outL, outC );
      if( retCode != TA_SUCCESS || begIdx != (int)k
          || nbElement != nbBars - (int)k )
      {
         printf( "HA unstable Fail [k=%u]: rc=%d (%d,%d) expected (%u,%d)\n",
                 k, (int)retCode, begIdx, nbElement, k, nbBars - (int)k );
         TA_SetUnstablePeriod( TA_FUNC_UNST_HA, 0 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      base[0] = baseO; base[1] = baseH; base[2] = baseL; base[3] = baseC;
      got[0] = outO; got[1] = outH; got[2] = outL; got[3] = outC;
      for( i = 0; i < nbElement; i++ )
      {
         int f;
         for( f = 0; f < 4; f++ )
         {
            g_haUnstCmp++;
            if( got[f][i] != base[f][i + (int)k] )
            {
               printf( "HA unstable Fail [k=%u] output %d bar %d: %.17g vs the "
                       "unfiltered run's %.17g -- withholding warm-up candles "
                       "must not move the ones that remain\n",
                       k, f, i + (int)k, got[f][i], base[f][i + (int)k] );
               TA_SetUnstablePeriod( TA_FUNC_UNST_HA, 0 );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_HA, 0 );
   return TA_TEST_PASS;
}

/* (7) Signed zeros, and the one tie rule the four backends can share. */
static ErrorNumber test_ha_zeros( void )
{
   static TA_Real o[HA_ZERO_NB], h[HA_ZERO_NB], l[HA_ZERO_NB], c[HA_ZERO_NB];
   static TA_Real outO[HA_CAP], outH[HA_CAP], outL[HA_CAP], outC[HA_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, f;

   haBuildZeros( o, h, l, c );

   retCode = TA_HA( 0, HA_ZERO_NB-1, o, h, l, c, &begIdx, &nbElement,
                    outO, outH, outL, outC );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != HA_ZERO_NB )
   {
      printf( "HA zeros Fail: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( server_verify_active() )
   {
      ErrorNumber e;
      int cmpBefore = server_verify_comparisons();

      e = server_verify( "HA", 0, HA_ZERO_NB-1, HA_ZERO_NB,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ o, h, l, c, NULL },
                         NULL, 0,
                         (const TA_Real*[]){ outO, outH, outL, outC, NULL },
                         NULL );
      if( e != TA_TEST_PASS )
         return e;
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "HA zeros: compared no server despite live pipes\n" );
         return TA_HA_VACUOUS;
      }
   }

   for( i = 0; i < nbElement; i++ )
   {
      const TA_Real *got[4];

      got[0] = &outO[i]; got[1] = &outH[i];
      got[2] = &outL[i]; got[3] = &outC[i];

      for( f = 0; f < 4; f++ )
      {
         g_haZeroCmp++;
         if( *got[f] != 0.0 )
         {
            printf( "HA zeros Fail bar %d output %d: %.17g, expected a zero\n",
                    i, f, *got[f] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( ( signbit( *got[f] ) ? 1 : 0 ) != haZeroSignBits[i][f] )
         {
            printf( "HA zeros Fail bar %d output %d: sign bit %d, expected %d. "
                    "The extremum must keep the raw high/low unless a STRICTLY "
                    "greater/lesser candidate appears; a two-argument max/min "
                    "builtin would answer this differently in C than in Rust, "
                    "Java and .NET\n",
                    i, f, signbit( *got[f] ) ? 1 : 0, haZeroSignBits[i][f] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. TA_STABLE_CONVERGING: haOpen carries the
 * previous candle, and HA_high/HA_low are the extremum of the raw bar WITH
 * haOpen and haClose, so all three depend on where the recursion started --
 * measured, they agree bit for bit 54 bars past the call's own start. Only
 * HA_close is a function of the current bar alone and exact at every bar. */
typedef struct { const TA_History *history; } HaRangeParam;

static TA_RetCode haRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                       TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                       TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                       TA_Integer *lookback, void *opaqueData,
                                       unsigned int outputNb, unsigned int *isOutputInteger )
{
   HaRangeParam *p = (HaRangeParam *)opaqueData;
   TA_RetCode retCode;
   TA_Real *outs[4];
   TA_Real *dummy;
   int f;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   dummy = TA_Malloc( 3 * (endIdx-startIdx+1) * sizeof(TA_Real) );
   TA_TOOL_CHECK_ALLOC(dummy);

   for( f = 0; f < 4; f++ )
      outs[f] = NULL;
   {
      int d = 0;
      for( f = 0; f < 4; f++ )
      {
         if( (unsigned int)f == outputNb )
            outs[f] = outputBuffer;
         else
            outs[f] = &dummy[ (d++) * (endIdx-startIdx+1) ];
      }
   }

   *lookback = TA_HA_Lookback();
   retCode = TA_HA( startIdx, endIdx,
                    p->history->open, p->history->high,
                    p->history->low, p->history->close,
                    outBegIdx, outNbElement,
                    outs[0], outs[1], outs[2], outs[3] );

   TA_Free( dummy );
   return retCode;
}

static ErrorNumber test_ha_range( const TA_History *history )
{
   HaRangeParam param;

   param.history = history;

   return doRangeTestEx( haRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_HA,
                         (void *)&param, 4, 0 );
}
