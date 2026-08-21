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
 *  082126 MF,CC  First version (issue #237).
 */

/* Description:
 *
 *   Test TA_VWAP (Volume Weighted Average Price).
 *
 *   VWAP is not covered by the --codegen sweep: that sweep diffs against the
 *   frozen ta_ref_serve, which predates this function. Coverage therefore comes
 *   from this file plus server_verify / --xlang-hash.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE (formula correctness) against pandas-ta-classic, at a
 *        relative tolerance, on two ranges. Plus the convex-combination bound
 *        over EVERY value, and cross-language bitwise.
 *     2. EXTERNAL ORACLE, BITWISE, against trading-signals. Doubles as the
 *        reassociation detector the tolerance leg cannot be.
 *     3. Deterministic edges: the zero-volume rulings and the exactness
 *        identities, none of which any oracle covers or any fuzz shape reaches.
 *     4. Aliasing of the output over each of the four inputs.
 *     5. The generic start/end range sweep.
 *     6. The single-precision entry point, which no other test in the tree
 *        reaches.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* The two ranges every value leg runs. VWAP carries `path_dependent`: the sums
 * are seeded at startIdx and never converge, so the value at a given bar is a
 * different number depending on where the caller started. Shape 1 exists to
 * pin that -- a body that ignored startIdx and always accumulated from bar 0
 * would pass shape 0 alone. */
typedef struct { int startIdx; int begIdx; int nbElement; } VwapShape;

static const VwapShape vwapShape[] =
{
   {   0,   0, 252 },
   { 100, 100, 152 },
};
#define NB_VWAP_SHAPE (sizeof(vwapShape)/sizeof(vwapShape[0]))

/* `shape` indexes vwapShape[]; `idx` is the OUTPUT-array index within it. */
typedef struct { int shape; int idx; double value; } VwapGolden;

/* --- LEG 1 provenance -----------------------------------------------------
 * pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1, Python 3.12.3), via
 * ta-lib-oracles/pandas_serve, method TA_VWAP, on this file's 252-bar series.
 *
 * pandas_ta_classic/overlap/vwap.py is session-anchored -- it groups by
 * `index.to_period(anchor)` -- so the capture gives it a synthetic 1-minute
 * DatetimeIndex, which puts all 252 bars inside one anchor bucket and reduces
 * its expression to the `tpv.cumsum() / volume.cumsum()` its own docstring
 * states. The index is a grouping key; no value depends on it.
 *
 * It is a genuinely separate evaluation, not a re-rounding of ours: numpy's
 * cumsum accumulates the whole product array at once, so 192 of the 252 values
 * differ from ours in the last bits. Worst relative deviation across both
 * shapes is 4.0e-16 at these spots (5.2e-16 over all 404 values), so the 1e-14
 * bound below carries ~25x headroom. Relative rather than absolute: VWAP is a
 * strictly positive price with no zero crossing, so relative is the meaningful
 * axis, and an absolute bound would be scale-dependent.
 */
#define VWAP_ORACLE_TOL 1e-14

static const VwapGolden vwapOracle[] =
{
   /* shape 0 -- startIdx 0 */
   { 0,   0, 91.833333333333329 },
   { 0,   1, 92.868395583796413 },
   { 0,   2, 93.605543220336926 },
   { 0,  41, 89.217099769203131 },
   { 0, 100, 94.879303248880532 },
   { 0, 130, 101.2371759075165 },
   { 0, 176, 108.24576785745043 },
   { 0, 210, 107.89661267744582 },
   { 0, 251, 107.0775504357283 },
   /* shape 1 -- startIdx 100 */
   { 1,   0, 115.16333333333334 },
   { 1,   1, 115.34551890050594 },
   { 1,  25, 119.76555694639723 },
   { 1,  75, 124.89162643202312 },
   { 1, 120, 114.25365308116316 },
   { 1, 151, 112.89053997317323 },
};
#define NB_VWAP_ORACLE (sizeof(vwapOracle)/sizeof(vwapOracle[0]))

/* --- LEG 2 provenance -----------------------------------------------------
 * trading-signals 8.3.0 (TypeScript, MIT), via ta-lib-oracles/
 * trading_signals_serve/capture.mjs, on the same series.
 *
 * `dist/trend/VWAP/VWAP.js` is cumulative from the first candle fed, with no
 * session anchor and no window at all, so unlike the pandas arm nothing has to
 * be neutralised to reach TA-Lib's definition; shape 1 is captured by feeding
 * it from bar 100.
 *
 * These values are BIT-IDENTICAL to ours on all 404 outputs of both shapes,
 * which is why this leg can be compared with memcmp. That is the expected
 * result, not a warning sign: both implementations run the same sequential
 * recurrence (`sum += typicalPrice * volume`) over the same doubles, and every
 * operation involved -- add, multiply, divide -- is correctly rounded by
 * IEEE-754, so agreeing to the bit is what two correct evaluations of the same
 * order MUST do. (Contrast the AC arm, whose SMA re-sums its window and
 * therefore must NOT match bitwise.) The FMA contract is what makes it hold:
 * `-ffp-contract=off` plus the product kept in its own statement in
 * ta_codegen/input/vwap/vwap.c.
 *
 * If this leg fails while leg 1 passes, the arithmetic was reassociated --
 * decide that deliberately, do not just re-capture. 17 significant digits
 * round-trips a double exactly.
 *
 * Measured reach, so nobody over-trusts it: replacing `sumPV / sumV` with
 * `sumPV * (1.0 / sumV)` fails HERE and passes leg 1, which is exactly the
 * split the two legs are for. Reordering the typical price to
 * `(close + high + low)` does NOT fail, and that is a property of the data,
 * not a hole in the leg: this series carries two-decimal prices near 100, so
 * every partial sum of three of them is exact and no addition order can
 * differ. An addition-order change in the typical price is invisible on this
 * corpus to any leg here.
 */
static const VwapGolden vwapBitwise[] =
{
   /* shape 0 -- startIdx 0 */
   { 0,   0, 91.833333333333329 },
   { 0,   1, 92.868395583796413 },
   { 0,   2, 93.605543220336926 },
   { 0,  41, 89.217099769203145 },
   { 0, 100, 94.879303248880504 },
   { 0, 130, 101.23717590751647 },
   { 0, 176, 108.24576785745043 },
   { 0, 210, 107.89661267744586 },
   { 0, 251, 107.07755043572831 },
   /* shape 1 -- startIdx 100 */
   { 1,   0, 115.16333333333334 },
   { 1,   1, 115.34551890050594 },
   { 1,  25, 119.76555694639723 },
   { 1,  75, 124.89162643202307 },
   { 1, 120, 114.25365308116315 },
   { 1, 151, 112.89053997317322 },
};
#define NB_VWAP_BITWISE (sizeof(vwapBitwise)/sizeof(vwapBitwise[0]))

/**** Local functions declarations. ****/
static ErrorNumber test_vwap_oracle   ( const TA_History *history );
static ErrorNumber test_vwap_bitwise  ( const TA_History *history );
static ErrorNumber test_vwap_edges    ( void );
static ErrorNumber test_vwap_aliasing ( const TA_History *history );
static ErrorNumber test_vwap_range    ( const TA_History *history );
static ErrorNumber test_vwap_single   ( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_vwap( TA_History *history )
{
   ErrorNumber retValue;

   /* VWAP has no unstable period; make sure a leftover global setting from an
    * earlier test cannot influence it (it must not). */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_vwap_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_vwap_bitwise( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_vwap_edges();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_vwap_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_vwap_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_vwap_single( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) External-oracle formula check on both ranges, plus a structural bound
 * over every value, plus cross-language. */
static ErrorNumber test_vwap_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int s, k;

   for( s = 0; s < NB_VWAP_SHAPE; s++ )
   {
      int startIdx = vwapShape[s].startIdx;
      int i;
      double lo, hi;

      retCode = TA_VWAP( startIdx, (int)history->nbBars - 1,
                         history->high, history->low, history->close, history->volume,
                         &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "VWAP oracle Fail (startIdx %d): retCode = %d\n", startIdx, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != vwapShape[s].begIdx || nbElement != vwapShape[s].nbElement )
      {
         printf( "VWAP oracle Fail (startIdx %d): shape got (%d,%d) expected (%d,%d)\n",
                 startIdx, begIdx, nbElement, vwapShape[s].begIdx, vwapShape[s].nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* VWAP is a convex combination of the typical prices of the bars it has
       * consumed -- non-negative weights summing to one -- so it can never
       * leave [min TP, max TP] over those bars, by more than rounding. It
       * holds on every one of the 404 values where the spot checks below see
       * 15 of them, and it fails on anything that is not a weighted mean of
       * those typical prices: a wrong divisor, a dropped division, a sign
       * error, the wrong price component.
       *
       * What it deliberately does NOT catch is a DIFFERENT set of
       * non-negative weights -- an unweighted mean is a convex combination
       * too, and sits inside the same bound. That half is pinned exactly by
       * the weighting case in leg 3.
       *
       * The bound is recomputed cumulatively so it tightens as it goes: a
       * whole-series min/max would be far weaker at the start.
       *
       * The 1e-9 slack is headroom, not a fudge factor tuned to pass: measured
       * over both shapes, the worst excursion outside [lo,hi] is exactly 0.0,
       * and the accumulated rounding on 252 terms bounds it near 5e-12 anyway.
       */
      lo =  TA_REAL_MAX;
      hi = -TA_REAL_MAX;
      for( i = 0; i < nbElement; i++ )
      {
         double tp = ( history->high [startIdx+i] +
                       history->low  [startIdx+i] +
                       history->close[startIdx+i] ) / 3.0;
         if( tp < lo ) lo = tp;
         if( tp > hi ) hi = tp;

         if( isnan( out[i] ) || out[i] < lo - 1e-9 || out[i] > hi + 1e-9 )
         {
            printf( "VWAP bound Fail (startIdx %d) at out[%d]: %.17g outside [%.17g, %.17g]\n",
                    startIdx, i, out[i], lo, hi );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      for( k = 0; k < NB_VWAP_ORACLE; k++ )
      {
         double want, got, err;

         if( vwapOracle[k].shape != (int)s )
            continue;

         want = vwapOracle[k].value;
         got  = out[vwapOracle[k].idx];
         err  = fabs( got - want ) / fabs( want );

         if( isnan( got ) || err > VWAP_ORACLE_TOL )  /* NaN > tol is false -> guard explicitly */
         {
            printf( "VWAP oracle Fail (startIdx %d) at out[%d]: got %.17g expected %.17g (rel=%.3e > %.3e)\n",
                    startIdx, vwapOracle[k].idx, got, want, err, VWAP_ORACLE_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      /* Cross-language: VWAP must be bit-identical on every language server. */
      if( server_verify_active() )
      {
         ErrorNumber e;

         e = server_verify( "VWAP", startIdx, (int)history->nbBars - 1, history->nbBars,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low,
                                                history->close, history->volume, NULL },
                            NULL, 0,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (2) External oracle, bitwise. See the vwapBitwise comment. */
static ErrorNumber test_vwap_bitwise( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int s, k;

   for( s = 0; s < NB_VWAP_SHAPE; s++ )
   {
      int startIdx = vwapShape[s].startIdx;

      retCode = TA_VWAP( startIdx, (int)history->nbBars - 1,
                         history->high, history->low, history->close, history->volume,
                         &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "VWAP bitwise Fail (startIdx %d): retCode = %d\n", startIdx, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( k = 0; k < NB_VWAP_BITWISE; k++ )
      {
         double want, got;

         if( vwapBitwise[k].shape != (int)s )
            continue;

         want = vwapBitwise[k].value;
         got  = out[vwapBitwise[k].idx];

         if( memcmp( &got, &want, sizeof(double) ) != 0 )
         {
            printf( "VWAP bitwise Fail (startIdx %d) at out[%d]: got %.17g expected %.17g (bitwise)\n"
                    "  The accumulation was reassociated (or an FMA contracted the product).\n"
                    "  The tolerance leg still passes, so this is not a formula error.\n",
                    startIdx, vwapBitwise[k].idx, got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Deterministic edges.
 *
 * Every case here is exact: the typical prices are chosen so (high+low+close)/3
 * is an integer, and the volumes so both running sums stay exactly
 * representable. So these compare with == and not a tolerance, and a formula
 * that is merely close fails.
 *
 * None of it is reachable by the fuzz shapes, which generate well-formed bars
 * with volume >= 1000, and none of it is covered by either oracle: pandas
 * emits NaN where TA-Lib carries forward, and trading-signals emits no value
 * at all for a zero-volume bar. */
static ErrorNumber test_vwap_edges( void )
{
   static TA_Real high[32], low[32], close[32], volume[32];
   static TA_Real out[OUT_CAP], out2[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   /* Every bar carries the same typical price, exactly 100. A weighted mean of
    * one repeated value is that value whatever the weights are, so this is
    * exact regardless of the volumes -- and it fails if the divisor is not the
    * same volume that multiplied the price. */
   for( i = 0; i < 12; i++ )
   {
      high[i]   = 101.0;
      low[i]    =  99.0;
      close[i]  = 100.0;
      volume[i] = 1000.0 + 500.0 * i;
   }
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 12 )
   {
      printf( "VWAP constant-price Fail: rc=%d shape (%d,%d) expected (0,12)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != 100.0 )
      {
         printf( "VWAP constant-price Fail: out[%d] = %.17g (expected exactly 100.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* The weighting is real, and it is by volume. Two bars, typical prices 100
    * and 200, volumes 1000 and 3000: (100*1000 + 200*3000)/4000 == 175 exactly.
    * An UNWEIGHTED mean of the same two bars would be 150, and weighting by
    * anything else would land elsewhere again. */
   {
      static const TA_Real wh[2] = { 101.0, 201.0 };
      static const TA_Real wl[2] = {  99.0, 199.0 };
      static const TA_Real wc[2] = { 100.0, 200.0 };
      static const TA_Real wv[2] = { 1000.0, 3000.0 };

      retCode = TA_VWAP( 0, 1, wh, wl, wc, wv, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 2 )
      {
         printf( "VWAP weighting Fail: rc=%d shape (%d,%d) expected (0,2)\n",
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( out[0] != 100.0 )
      {
         printf( "VWAP weighting Fail: out[0] = %.17g (one bar => its own typical price, 100.0)\n",
                 out[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( out[1] != 175.0 )
      {
         printf( "VWAP weighting Fail: out[1] = %.17g (expected exactly 175.0; 150.0 would mean "
                 "the mean is unweighted)\n", out[1] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* A leading run of untraded bars: nothing has been weighted yet, so the
    * value carried forward is the 0.0 seed, and the first bar that trades
    * reports its own typical price. Without the guard the first five are
    * 0.0/0.0 == NaN (issue #112). */
   for( i = 0; i < 12; i++ )
   {
      high[i]   = 101.0 + i;
      low[i]    =  99.0 + i;
      close[i]  = 100.0 + i;
      volume[i] = ( i < 5 ) ? 0.0 : 1000.0;
   }
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 12 )
   {
      printf( "VWAP zero-prefix Fail: rc=%d shape (%d,%d) expected (0,12)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < 5; i++ )
      if( isnan( out[i] ) || out[i] != 0.0 )
      {
         printf( "VWAP zero-prefix Fail: out[%d] = %.17g (expected exactly 0.0; NaN => guard missing)\n",
                 i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   if( out[5] != 105.0 )
   {
      printf( "VWAP zero-prefix Fail: out[5] = %.17g (first traded bar => its typical price, 105.0)\n",
              out[5] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* No volume anywhere: every value is the 0.0 seed, and above all every
    * value is FINITE. VWAP does not carry TA_FUNC_FLG_NAN_INF_OUT, so
    * test_abstract.c holds it to this over its all-zero dataset too. */
   for( i = 0; i < 12; i++ )
      volume[i] = 0.0;
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 12 )
   {
      printf( "VWAP no-volume Fail: rc=%d nbElement=%d expected (success,12)\n",
              (int)retCode, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( !isfinite( out[i] ) || out[i] != 0.0 )
      {
         printf( "VWAP no-volume Fail: out[%d] = %.17g (expected exactly 0.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* A negative cumulative volume cannot weight anything either. The guard is
    * `sumV > 0.0` and not `!= 0.0` precisely so this stays out of a
    * price-scale output; with `!= 0.0` these would be real, negatively
    * weighted numbers instead of the carried-forward seed. */
   for( i = 0; i < 12; i++ )
      volume[i] = -1000.0;
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 12 )
   {
      printf( "VWAP negative-volume Fail: rc=%d nbElement=%d expected (success,12)\n",
              (int)retCode, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( !isfinite( out[i] ) || out[i] != 0.0 )
      {
         printf( "VWAP negative-volume Fail: out[%d] = %.17g (expected exactly 0.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* An untraded bar AFTER volume has traded needs no arm at all: it changes
    * neither sum, so it repeats the previous value bit for bit. This is the
    * documented behaviour, and it is what distinguishes the shipped guard --
    * which tests the cumulative volume -- from one that tested the bar's own
    * volume and reset or skipped on it. */
   for( i = 0; i < 12; i++ )
   {
      high[i]   = 101.0 + i;
      low[i]    =  99.0 + i;
      close[i]  = 100.0 + i;
      volume[i] = ( i == 3 || i == 4 ) ? 0.0 : 1000.0 + 100.0 * i;
   }
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 12 )
   {
      printf( "VWAP zero-gap Fail: rc=%d nbElement=%d expected (success,12)\n",
              (int)retCode, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 3; i <= 4; i++ )
      if( memcmp( &out[i], &out[2], sizeof(double) ) != 0 )
      {
         printf( "VWAP zero-gap Fail: out[%d] = %.17g, expected the bar-2 value %.17g bit for bit\n",
                 i, out[i], out[2] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   if( out[5] == out[4] )
   {
      printf( "VWAP zero-gap Fail: out[5] did not move off the repeated value -- "
              "the traded bar after the gap was ignored\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* A bar that cannot be weighted is left out of the average ENTIRELY and
    * repeats the previous value -- and then the average RESUMES.
    *
    * "Skipped, not absorbed" is the whole point, and each expectation below
    * pins a different half of it:
    *
    *   out[3] == out[2] bitwise   -- the bad bar produced no new value.
    *   out[4] == exactly 101.75   -- which is (100+101+102+104)*1000/4000,
    *                                 the average over bars {0,1,2,4}. So the
    *                                 bad bar contributed neither its price
    *                                 term NOR its volume: had the volume
    *                                 alone reached the divisor, this would be
    *                                 407000/5000 == 81.4.
    *   out[11] != out[3]          -- the line is still moving ten bars later.
    *
    * Letting the bad bar into the running sums instead would freeze the
    * output at one stale value for EVERY remaining bar however clean it was,
    * because a cumulative sum has no trailing term to subtract it back out.
    * Silent, permanent, and a plausible-looking price the whole way -- which
    * is exactly what this leg exists to prevent regressing to.
    *
    * Three ways for a bar to be unusable, all reaching the same guard: a
    * non-finite volume, an infinite volume, and a non-finite PRICE with a
    * perfectly good volume. The last one is what proves the two sums are
    * committed together rather than independently.
    */
   {
      static const double badVol[3] = { (double)NAN, (double)INFINITY, -(double)INFINITY };
      const char *badName[4] = { "NaN volume", "+Inf volume", "-Inf volume", "NaN close" };
      int which;

      for( which = 0; which < 4; which++ )
      {
         for( i = 0; i < 12; i++ )
         {
            high[i]   = 101.0 + i;
            low[i]    =  99.0 + i;
            close[i]  = 100.0 + i;
            volume[i] = 1000.0;
         }
         if( which < 3 )
            volume[3] = badVol[which];
         else
            close[3] = (double)NAN;   /* price unusable, volume perfectly good */

         retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || nbElement != 12 )
         {
            printf( "VWAP skip-bar Fail (%s): rc=%d nbElement=%d expected (success,12)\n",
                    badName[which], (int)retCode, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
            if( !isfinite( out[i] ) )
            {
               printf( "VWAP skip-bar Fail (%s): out[%d] = %.17g is not finite\n",
                       badName[which], i, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         if( memcmp( &out[3], &out[2], sizeof(double) ) != 0 )
         {
            printf( "VWAP skip-bar Fail (%s): out[3] = %.17g, expected the bar-2 value "
                    "%.17g bit for bit\n", badName[which], out[3], out[2] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( out[4] != 101.75 )
         {
            printf( "VWAP skip-bar Fail (%s): out[4] = %.17g, expected exactly 101.75 "
                    "(the average over bars 0,1,2,4). 81.4 would mean the bad bar's "
                    "volume still reached the divisor; a frozen %.17g would mean the "
                    "sums were poisoned.\n", badName[which], out[4], out[3] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( out[11] == out[3] )
         {
            printf( "VWAP skip-bar Fail (%s): out[11] is still the frozen %.17g -- the "
                    "average never resumed\n", badName[which], out[11] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* Cross-language, on the DOCTORED arrays. The guard is generated into
          * all four backends, but every other cross-language leg in this file
          * feeds the clean 252-bar history, --codegen skips VWAP (no frozen
          * baseline), and no fuzz shape emits a non-finite bar -- so without
          * this call the skip has C-only coverage. An emitter that gated
          * `sumPV +=` and left `sumV += volume` unconditional in one language
          * would bias every later value there and ship green.
          *
          * The inputs travel as hex-of-IEEE-bits, so the NaN and the
          * infinities arrive bit-exact rather than through a %g round-trip. */
         if( server_verify_active() )
         {
            ErrorNumber e;

            e = server_verify( "VWAP", 0, 11, 12,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ high, low, close, volume, NULL },
                               NULL, 0,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
            {
               printf( "VWAP skip-bar Fail (%s): cross-language mismatch on the "
                       "doctored bar\n", badName[which] );
               return e;
            }
         }
      }
   }

   /* Path dependence, stated directly rather than only implied by the two
    * golden shapes: the same bar computed from a later startIdx is a
    * different number, because the sums are seeded at startIdx and never
    * converge. This is what TA_FUNC_FLG_PATH_DEP declares. */
   for( i = 0; i < 12; i++ )
   {
      high[i]   = 101.0 + i;
      low[i]    =  99.0 + i;
      close[i]  = 100.0 + i;
      volume[i] = 1000.0 + 100.0 * i;
   }
   retCode = TA_VWAP( 0, 11, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "VWAP path-dep Fail: rc=%d on the full range\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   retCode = TA_VWAP( 6, 11, high, low, close, volume, &begIdx, &nbElement, out2 );
   if( retCode != TA_SUCCESS || begIdx != 6 || nbElement != 6 )
   {
      printf( "VWAP path-dep Fail: rc=%d shape (%d,%d) expected (6,6)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( out2[0] != 106.0 )
   {
      printf( "VWAP path-dep Fail: out2[0] = %.17g (a range starting at bar 6 sees one bar, "
              "so its typical price, 106.0)\n", out2[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( out2[0] == out[6] )
   {
      printf( "VWAP path-dep Fail: bar 6 is %.17g from either startIdx -- the accumulation "
              "ignored startIdx\n", out2[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* One bar is a complete answer: lookback is 0, so the first bar of any
    * range reports its own typical price. */
   if( TA_VWAP_Lookback() != 0 )
   {
      printf( "VWAP lookback Fail: %d (expected 0)\n", TA_VWAP_Lookback() );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   retCode = TA_VWAP( 0, 0, high, low, close, volume, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 1 || out[0] != 100.0 )
   {
      printf( "VWAP single-bar Fail: rc=%d shape (%d,%d) out[0]=%.17g expected (0,1) 100.0\n",
              (int)retCode, begIdx, nbElement, out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) outReal may alias any of the four inputs. Every input value for a bar is
 * read into a scalar before that bar's output is written, and the output index
 * never runs ahead of the input index, so no input element is re-read after
 * the output has overwritten it. */
static ErrorNumber test_vwap_aliasing( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];
   const TA_Real *src[4];
   const char *name[4] = { "inHigh", "inLow", "inClose", "inVolume" };
   int which, i, nb;

   nb = (int)history->nbBars;

   retCode = TA_VWAP( 0, nb - 1,
                      history->high, history->low, history->close, history->volume,
                      &begIdx, &nbElement, ref );
   if( retCode != TA_SUCCESS )
   {
      printf( "VWAP aliasing Fail: baseline retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->close;
   src[3] = history->volume;

   for( which = 0; which < 4; which++ )
   {
      const TA_Real *in[4];

      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];

      for( i = 0; i < 4; i++ )
         in[i] = ( i == which ) ? work : src[i];

      retCode = TA_VWAP( 0, nb - 1, in[0], in[1], in[2], in[3],
                         &begIdx2, &nbElement2, work );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "VWAP aliasing Fail (%s): rc=%d shape (%d,%d) vs (%d,%d)\n",
                 name[which], (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( memcmp( ref, work, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
      {
         for( i = 0; i < nbElement; i++ )
            if( ref[i] != work[i] )
            {
               printf( "VWAP aliasing Fail (%s): bit mismatch at out[%d] separate=%.17g aliased=%.17g\n",
                       name[which], i, ref[i], work[i] );
               break;
            }
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Generic startIdx/endIdx range sweep (self-coherency + lookback).
 * TA_STABLE_SKIP because VWAP carries `path_dependent`: the sums are seeded at
 * startIdx and never converge, so comparing a bar's value across ranges is
 * meaningless -- leg 3 asserts the opposite of convergence directly. What this
 * sweep still checks is that every range returns a coherent
 * (retCode, outBegIdx, outNBElement) consistent with the lookback. */
typedef struct
{
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;
   const TA_Real *volume;
} VwapRangeParam;

static TA_RetCode vwapRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   VwapRangeParam *p = (VwapRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_VWAP_Lookback();
   return TA_VWAP( startIdx, endIdx, p->high, p->low, p->close, p->volume,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_vwap_range( const TA_History *history )
{
   VwapRangeParam param;

   param.high   = history->high;
   param.low    = history->low;
   param.close  = history->close;
   param.volume = history->volume;

   return doRangeTestEx( vwapRangeTestFunction,
                         TA_STABLE_SKIP, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (6) Single-precision entry point.
 *
 * TA_S_VWAP takes float inputs but must compute in double throughout (PR #33),
 * so on the SAME widened values it must be bit-identical to TA_VWAP.
 *
 * This is not the only cover: test_variants.c sweeps the whole
 * TA_VariantTable unfiltered, so VWAP's entry there already gets the same
 * widened-input memcmp across that gate's ranges and regimes. What this leg
 * adds is the real 252-bar history at a startIdx of 100, which that sweep does
 * not run -- and with a cumulative accumulator, a float entry point that
 * ignored startIdx would still agree with the double one at startIdx 0. */
static ErrorNumber test_vwap_single( const TA_History *history )
{
   static TA_Real  outD[OUT_CAP], outS[OUT_CAP];
   static TA_Real  dHigh[OUT_CAP], dLow[OUT_CAP], dClose[OUT_CAP], dVolume[OUT_CAP];
   static float    fHigh[OUT_CAP], fLow[OUT_CAP], fClose[OUT_CAP], fVolume[OUT_CAP];
   TA_RetCode rcD, rcS;
   TA_Integer begD, nbD, begS, nbS;
   unsigned int s;
   int i, nb;

   nb = (int)history->nbBars;
   if( nb > OUT_CAP )
      nb = OUT_CAP;

   for( i = 0; i < nb; i++ )
   {
      fHigh[i]   = (float)history->high[i];
      fLow[i]    = (float)history->low[i];
      fClose[i]  = (float)history->close[i];
      fVolume[i] = (float)history->volume[i];
      dHigh[i]   = (TA_Real)fHigh[i];
      dLow[i]    = (TA_Real)fLow[i];
      dClose[i]  = (TA_Real)fClose[i];
      dVolume[i] = (TA_Real)fVolume[i];
   }

   for( s = 0; s < NB_VWAP_SHAPE; s++ )
   {
      int startIdx = vwapShape[s].startIdx;

      rcD = TA_VWAP  ( startIdx, nb - 1, dHigh, dLow, dClose, dVolume, &begD, &nbD, outD );
      rcS = TA_S_VWAP( startIdx, nb - 1, fHigh, fLow, fClose, fVolume, &begS, &nbS, outS );

      if( rcD != TA_SUCCESS || rcS != TA_SUCCESS || begD != begS || nbD != nbS )
      {
         printf( "VWAP single Fail (startIdx %d): rcD=%d rcS=%d shape (%d,%d) vs (%d,%d)\n",
                 startIdx, (int)rcD, (int)rcS, begD, nbD, begS, nbS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbD; i++ )
         if( memcmp( &outD[i], &outS[i], sizeof(TA_Real) ) != 0 )
         {
            printf( "VWAP single Fail (startIdx %d) at out[%d]: double=%.17g float=%.17g "
                    "(must be bit-identical on widened inputs)\n",
                    startIdx, i, outD[i], outS[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   return TA_TEST_PASS;
}
