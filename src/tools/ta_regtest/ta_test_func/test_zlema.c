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
 *  090426 MF,CC  First version. ZLEMA (Zero-Lag EMA, issue #347).
 */

/* Description:
 *
 *   Regression tests for ZLEMA, an EMA over the de-lagged series
 *   d[j] = 2*P[j] - P[j-lag] with lag = (n-1)/2 truncating.
 *
 *   ZLEMA is post-cutover, so the --codegen VALUE sweep skips it: that sweep
 *   diffs against the frozen ta_ref_serve, which predates the function. Every
 *   value below therefore has to come from here, from server_verify (which
 *   this file drives) or from --xlang-hash.
 *
 *   Coverage:
 *     (1) External oracles, two of them, at two periods -- the only leg that
 *         can catch a wrong FORMULA.
 *     (2) A bitwise differential against the shipped TA_EMA over a materialised
 *         de-lagged series. Proves the fusion, never the formula: both sides
 *         share the de-lag.
 *     (3) Period-1 identity on inputs built to break the naive recursion.
 *     (4) Deterministic edges: flat input, empty ranges, the lookback shape,
 *         and in-place aliasing (outReal == inReal) bitwise.
 *     (5) The unstable period is EMA's, and it moves the first output.
 *     (6) TA_MAType_ZLEMA dispatch parity, through MA and through BBANDS.
 *     (7) The generic startIdx/endIdx range sweep.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Global functions declarations.   ****/
/* None */

/**** Local declarations.              ****/
#define OUT_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* --- Oracle 1: pandas-ta-classic 0.6.52 (ta-lib-oracles/pandas_serve;
 * pandas 3.0.3 / numpy 2.5.1), ta.zlma(close, length=N, mamode="ema") on the
 * standard 252-bar close series.
 *
 * It corroborates the STRUCTURE independently -- lag, the de-lag, alpha, and
 * the whole post-warm-up curve -- but NOT the seed: pandas_ta_classic's
 * overlap/ema.py says in-source that its SMA seed was written to match TA-Lib's
 * EMA lookback behaviour for lagged inputs, so agreement there is by
 * construction. It is not calling us either way: zlma() passes no `talib=`
 * kwarg, so pandas-ta's own ewm path runs regardless of whether the talib
 * module is importable in the venv.
 *
 * Measured full-range agreement: max rel 5.7e-16 over 239 bars at period 10,
 * 1.5e-15 over 209 bars at period 30. The 1e-12 tolerance below is the PVO
 * precedent and leaves ~660x headroom at the tighter of the two.
 *
 * idx is the OUTPUT-array index; add the ORACLE_BEG below for the global bar. */
static const struct { int idx; double value; } zlemaPandas10[] =
{
   {   0,  94.4185             },
   {   7,  89.18387347894159   },
   {  22,  87.68868876054243   },
   {  37,  89.97653283175656   },
   {  64,  99.56766011033952   },
   {  87, 114.58403109683026   },
   { 112, 130.74571000861027   },
   { 137, 121.78172611073379   },
   { 162, 134.35563908057492   },
   { 187, 106.4477445208301    },
   { 212, 105.65542066577297   },
   { 238, 108.64427272744322   },
};
#define NB_ZLEMA_PANDAS10 (sizeof(zlemaPandas10)/sizeof(zlemaPandas10[0]))
#define ZLEMA_P10_PERIOD 10
#define ZLEMA_P10_BEG    13
#define ZLEMA_P10_NB     239

/* Period 30 is the shipped default, and the period at which the Tulip arm is
 * useless (see below), so the pandas arm carries it alone. */
static const struct { int idx; double value; } zlemaPandas30[] =
{
   {   0,  83.47700000000002   },
   {   7,  87.23894645913948   },
   {  32,  89.76469129270977   },
   {  57, 119.60187628580154   },
   {  82, 127.58051378758816   },
   { 107, 122.63183495348304   },
   { 132, 130.58827511141902   },
   { 157, 108.98615382445496   },
   { 182,  96.27201161913554   },
   { 208, 109.59533608422609   },
};
#define NB_ZLEMA_PANDAS30 (sizeof(zlemaPandas30)/sizeof(zlemaPandas30[0]))
#define ZLEMA_P30_PERIOD 30
#define ZLEMA_P30_BEG    43
#define ZLEMA_P30_NB     209

#define ZLEMA_ORACLE_TOL 1e-12
#define ZLEMA_ORACLE_ABS 1e-12

/* --- Oracle 2: Tulip Indicators 0.9.2 ti_zlema (ta-lib-oracles/tulip_serve,
 * vendored at be18abb), same 252-bar close series, period 10.
 *
 * Fully independent of ours in all three places the references disagree: it
 * spells the de-lag c + (c - l) rather than 2c - l, seeds from the single raw
 * price input[lag-1], and emits its first value at bar lag-1 instead of at a
 * full EMA lookback. Its warm-up is therefore not comparable to ours at ANY
 * tolerance -- it converges to us rather than rounding to us. Measured, the two
 * become BIT-IDENTICAL at bar 179 and stay so for the remaining 73 bars; the
 * three spots below sit inside that run, and none of them repeats a pandas row.
 *
 * Period 10, not the default 30: at period 30 Tulip has still not converged by
 * the last bar of a 252-bar corpus (measured 1.6e-8 at bar 251), so there is no
 * bar on this corpus where it could corroborate anything.
 *
 * idx is the GLOBAL bar index. Never query this oracle below period 3 --
 * ti_zlema reads input[lag-1], which is input[-1] when lag is 0. */
static const struct { int bar; double value; } zlemaTulip10[] =
{
   { 190, 119.85022761490855 },
   { 210,  95.750772869804237 },
   { 235, 117.53789362762379 },
};
#define NB_ZLEMA_TULIP10 (sizeof(zlemaTulip10)/sizeof(zlemaTulip10[0]))

/* Comparison counters. These groups print nothing on success, so a count that
 * reached zero is the only remaining way one of them can run without comparing
 * anything. */
static int g_zlemaOracleCmp   = 0;
static int g_zlemaDiffCloseCmp = 0;
static int g_zlemaDiffHostileCmp  = 0;

/* Periods the differential sweeps. 1 and 2 make lag 0 (the de-lagged series is
 * the input itself); odd/even pairs straddle the truncation. */
static const int zlemaDiffGrid[] = { 1, 2, 3, 4, 5, 7, 10, 14, 20, 30, 50, 100 };
#define NB_ZLEMA_DIFF_GRID (sizeof(zlemaDiffGrid)/sizeof(zlemaDiffGrid[0]))

/**** Local functions declarations. ****/
static ErrorNumber test_zlema_oracle( const TA_History *history );
static ErrorNumber test_zlema_differential( const TA_History *history );
static ErrorNumber test_zlema_period_one( const TA_History *history );
static ErrorNumber test_zlema_edges( void );
static ErrorNumber test_zlema_unstable( const TA_History *history );
static ErrorNumber test_zlema_matype( const TA_History *history );
static ErrorNumber test_zlema_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_zlema( TA_History *history )
{
   ErrorNumber retValue;

   /* DO_TEST resets the compatibility mode between groups but NOT the unstable
    * periods, and earlier groups set them. Every leg below except (5) assumes
    * TA_FUNC_UNST_EMA is 0. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_zlema_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_differential( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_period_one( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_edges();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_unstable( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_matype( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_zlema_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_zlemaOracleCmp < (int)(NB_ZLEMA_PANDAS10 + NB_ZLEMA_PANDAS30 + NB_ZLEMA_TULIP10) )
   {
      printf( "ZLEMA oracle Fail: %d comparisons, expected %d\n",
              g_zlemaOracleCmp,
              (int)(NB_ZLEMA_PANDAS10 + NB_ZLEMA_PANDAS30 + NB_ZLEMA_TULIP10) );
      return TA_ZLEMA_VACUOUS;
   }
   /* Both corpora, each with its own floor: the close series carries the bulk
    * of the comparisons, so one combined floor would still be met with the
    * `hostile` corpus -- the only gate on the de-lag spelling -- entirely gone.
    * Measured 7959 and 3561 over the three unstable settings. */
   if( g_zlemaDiffCloseCmp < 7900 || g_zlemaDiffHostileCmp < 3500 )
   {
      printf( "ZLEMA differential Fail: %d close + %d hostile comparisons, "
              "expected >= 7900 and >= 3500\n",
              g_zlemaDiffCloseCmp, g_zlemaDiffHostileCmp );
      return TA_ZLEMA_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Compare one output against an oracle value at ZLEMA_ORACLE_TOL, counting the
 * comparison. Returns 0 on match. */
static int zlemaOracleMiss( const char *arm, int period, int idx,
                            double got, double want )
{
   double err = 0.0;
   const char *mode = "rel";

   g_zlemaOracleCmp++;
   if( !checkOracleValue( got, want, ZLEMA_ORACLE_TOL, ZLEMA_ORACLE_ABS, &err, &mode ) )
   {
      printf( "ZLEMA %s oracle Fail [period %d] at out[%d]: got %.17g expected %.17g "
              "(%s=%.3e > rel %.3e / abs %.3e)\n",
              arm, period, idx, got, want, mode, err,
              ZLEMA_ORACLE_TOL, ZLEMA_ORACLE_ABS );
      return 1;
   }
   return 0;
}

/* (1) External oracles + cross-language bitwise. */
static ErrorNumber test_zlema_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int k;
   int endIdx = (int)history->nbBars - 1;

   /* --- pandas arm, period 10 --- */
   retCode = TA_ZLEMA( 0, endIdx, history->close, ZLEMA_P10_PERIOD,
                       &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "ZLEMA oracle Fail [period %d]: retCode = %d\n",
              ZLEMA_P10_PERIOD, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != ZLEMA_P10_BEG || nbElement != ZLEMA_P10_NB )
   {
      printf( "ZLEMA oracle Fail [period %d]: shape got (%d,%d) expected (%d,%d)\n",
              ZLEMA_P10_PERIOD, begIdx, nbElement, ZLEMA_P10_BEG, ZLEMA_P10_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( k = 0; k < NB_ZLEMA_PANDAS10; k++ )
      if( zlemaOracleMiss( "pandas", ZLEMA_P10_PERIOD, zlemaPandas10[k].idx,
                           out[zlemaPandas10[k].idx], zlemaPandas10[k].value ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;

   /* --- Tulip arm, same call, late bars only --- */
   for( k = 0; k < NB_ZLEMA_TULIP10; k++ )
   {
      int idx = zlemaTulip10[k].bar - begIdx;
      if( idx < 0 || idx >= nbElement )
      {
         printf( "ZLEMA tulip oracle Fail: bar %d outside (%d,%d)\n",
                 zlemaTulip10[k].bar, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      if( zlemaOracleMiss( "tulip", ZLEMA_P10_PERIOD, idx,
                           out[idx], zlemaTulip10[k].value ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Cross-language: ZLEMA must be bit-identical on every language server. */
   if( server_verify_active() )
   {
      ErrorNumber e = server_verify( "ZLEMA", 0, endIdx, history->nbBars,
                                     retCode, begIdx, nbElement,
                                     (const TA_Real*[]){ history->close, NULL },
                                     (double[]){ (double)ZLEMA_P10_PERIOD }, 1,
                                     (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
   }

   /* --- pandas arm, period 30 (the shipped default) --- */
   retCode = TA_ZLEMA( 0, endIdx, history->close, ZLEMA_P30_PERIOD,
                       &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "ZLEMA oracle Fail [period %d]: retCode = %d\n",
              ZLEMA_P30_PERIOD, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != ZLEMA_P30_BEG || nbElement != ZLEMA_P30_NB )
   {
      printf( "ZLEMA oracle Fail [period %d]: shape got (%d,%d) expected (%d,%d)\n",
              ZLEMA_P30_PERIOD, begIdx, nbElement, ZLEMA_P30_BEG, ZLEMA_P30_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( k = 0; k < NB_ZLEMA_PANDAS30; k++ )
      if( zlemaOracleMiss( "pandas", ZLEMA_P30_PERIOD, zlemaPandas30[k].idx,
                           out[zlemaPandas30[k].idx], zlemaPandas30[k].value ) )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;

   if( server_verify_active() )
   {
      ErrorNumber e = server_verify( "ZLEMA", 0, endIdx, history->nbBars,
                                     retCode, begIdx, nbElement,
                                     (const TA_Real*[]){ history->close, NULL },
                                     (double[]){ (double)ZLEMA_P30_PERIOD }, 1,
                                     (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* (2) DIFFERENTIAL, bitwise: ZLEMA == the shipped TA_EMA over a materialised
 * de-lagged series, at zero tolerance.
 *
 * This is the strongest gate available and it is bit-exact only because of
 * three choices in zlema.c: the de-lag is 2.0*c - l (one rounding), the seed
 * sum accumulates sequentially from 0.0, and the recursion is
 * ((v - prevMA)*k) + prevMA -- the same three the C emitter then fuses into
 * fma(v - prevMA, k, prevMA), exactly as it fuses TA_EMA's. If this leg ever
 * needs a tolerance the implementation has drifted from the composition; do not
 * paper over it with an epsilon.
 *
 * It proves the fusion, NEVER the formula: both sides carry the same de-lag.
 * That is what leg (1) is for.
 *
 * TWO corpora, and the second is not optional. `2.0*c - l` and `c + (c - l)`
 * are algebraically equal and differ only by the second form's extra rounding,
 * which is one ULP OF THE LARGER PRICE -- invisible in the result until the
 * de-lagged value itself loses significance. Two conditions do that, and the
 * `hostile` series below carries both: operands decades apart, and operands
 * near a 2:1 ratio, where 2c - l cancels (measured 5e-12 relative at
 * c = 128.0, l = 256.0). The reference close series meets neither, so the two
 * spellings are bit-identical on it at every lag and that corpus alone CANNOT
 * see the de-lag spelling -- this leg passes with the spelling changed.
 * Measured, `hostile` separates them at 7 of the 12 grid periods.
 *
 * Cutting the grid to the insensitive periods, or dropping the corpus, would
 * leave the de-lag spelling ungated with a single combined count still met,
 * which is why the two corpora are counted separately below.
 *
 * Also sweeps a non-zero unstable period, the one axis that can see the seam
 * between the seed loop and the warm-up loop move. */
static ErrorNumber test_zlema_differential( const TA_History *history )
{
   static TA_Real outZL[OUT_CAP];
   static TA_Real outEMA[OUT_CAP];
   static TA_Real delagged[OUT_CAP];
   static TA_Real hostile[128];
   const int unstGrid[] = { 0, 1, 5 };
   unsigned int g, u, c;
   int savedK = (int)TA_GetUnstablePeriod( TA_FUNC_UNST_EMA );
   ErrorNumber result = TA_TEST_PASS;
   int i;

   /* Odd bars cycle three magnitudes nine decades apart; even bars ride a
    * geometric decay whose half-life sweeps 3, 10, 17, ... bars, so some lag
    * in the grid always lands near a 2:1 ratio. The sine term keeps the ratio
    * off an exact power of two, where both spellings would cancel to the same
    * exact zero and separate nothing. */
   for( i = 0; i < 128; i++ )
      hostile[i] = ( i % 2 )
                 ? ( ( i % 3 == 0 ) ? 1e-3 : ( ( i % 3 == 1 ) ? 1e6 : 3.14159 ) )
                 : 256.0 * pow( 0.5, i / ( 3.0 + ( i / 16 ) * 7.0 ) )
                         * ( 1.0 + 0.0031 * sin( i * 1.7 ) );

   for( u = 0; u < sizeof(unstGrid)/sizeof(unstGrid[0]) && result == TA_TEST_PASS; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, unstGrid[u] );

      for( c = 0; c < 2 && result == TA_TEST_PASS; c++ )
      {
         const TA_Real *src = c ? hostile : history->close;
         const char    *tag = c ? "hostile" : "close";
         int nbBars = c ? 128 : (int)history->nbBars;

         for( g = 0; g < NB_ZLEMA_DIFF_GRID; g++ )
         {
            int period = zlemaDiffGrid[g];
            int lag    = (period - 1) / 2;
            TA_RetCode rcZ, rcE;
            TA_Integer begZ, nbZ, begE, nbE;

            rcZ = TA_ZLEMA( 0, nbBars - 1, src, period, &begZ, &nbZ, outZL );

            /* Test-only reference: materialise d[j] = 2*P[j] - P[j-lag] for
             * j = lag..endIdx, then hand it to the shipped TA_EMA. No new
             * numerical logic -- TA_EMA is already pinned by its own hardcoded
             * values and by the bitwise cross-language gate. */
            for( i = lag; i < nbBars; i++ )
               delagged[i-lag] = 2.0 * src[i] - src[i-lag];

            rcE = TA_EMA( 0, nbBars - 1 - lag, delagged, period, &begE, &nbE, outEMA );

            if( rcZ != TA_SUCCESS || rcE != TA_SUCCESS )
            {
               printf( "ZLEMA differential Fail [%s period %d unst %d]: retCode "
                       "ZLEMA=%d EMA=%d\n",
                       tag, period, unstGrid[u], (int)rcZ, (int)rcE );
               result = TA_TESTUTIL_TFRR_BAD_RETCODE;
               break;
            }

            /* Ranges first: a differential that compares out[i] without pinning
             * both outBegIdx compares different bars and passes. When nothing is
             * produced both sides report begIdx 0, so only the count is meaningful. */
            if( nbE != nbZ || (nbZ > 0 && begE + lag != begZ) )
            {
               printf( "ZLEMA differential Fail [%s period %d unst %d]: range "
                       "ZLEMA(%d,%d) vs compose(%d,%d)\n",
                       tag, period, unstGrid[u], (int)begZ, (int)nbZ,
                       (int)(begE + lag), (int)nbE );
               result = TA_TESTUTIL_TFRR_BAD_BEGIDX;
               break;
            }

            for( i = 0; i < nbZ; i++ )
            {
               if( c ) g_zlemaDiffHostileCmp++; else g_zlemaDiffCloseCmp++;
               if( memcmp( &outZL[i], &outEMA[i], sizeof(double) ) != 0 )
               {
                  printf( "ZLEMA differential Fail [%s period %d unst %d] at out[%d]: "
                          "fused %.17g != compose %.17g (must be BIT-exact)\n",
                          tag, period, unstGrid[u], i, outZL[i], outEMA[i] );
                  result = TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  break;
               }
            }
            if( result != TA_TEST_PASS )
               break;
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
   return result;
}

/* (3) Period 1 is a bit-exact copy of the input -- the promise ZLEMA declares
 * with `period1_identity`, and the one zlema.c needs an explicit arm for.
 *
 * The fused recursion at period 1 is (x - prev) + prev, which returns x only
 * while x - prev is exactly representable. The reference close series never
 * breaks it; the two synthetic series below do, which is why they are here and
 * not just the reference one. */
static ErrorNumber test_zlema_period_one( const TA_History *history )
{
   static TA_Real in[64];
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, s, n;
   const int unstGrid[] = { 0, 3 };
   unsigned int u;
   int savedK = (int)TA_GetUnstablePeriod( TA_FUNC_UNST_EMA );

   for( u = 0; u < sizeof(unstGrid)/sizeof(unstGrid[0]); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, unstGrid[u] );

      for( s = 0; s < 3; s++ )
      {
         const TA_Real *src;
         int len;

         if( s == 0 )
         {
            src = history->close;
            len = (int)history->nbBars;
         }
         else if( s == 1 )
         {
            /* Consecutive values more than a factor of two apart: x - prev
             * rounds, and the naive recursion loses the low bits. */
            for( i = 0; i < 32; i++ )
               in[i] = ( i % 2 ) ? 90.11 + i : 1.23 + 0.01 * i;
            src = in;
            len = 32;
         }
         else
         {
            /* Three magnitudes decades apart in the same window. */
            for( i = 0; i < 32; i++ )
               in[i] = ( i % 3 == 0 ) ? 1e-3 : ( ( i % 3 == 1 ) ? 1e6 : 3.14159 );
            src = in;
            len = 32;
         }

         retCode = TA_ZLEMA( 0, len - 1, src, 1, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "ZLEMA period-1 Fail [series %d unst %d]: retCode = %d\n",
                    s, unstGrid[u], (int)retCode );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         /* Lookback at period 1 is the unstable period alone: lag is 0 and
          * ema_lookback(1) is 0. */
         if( begIdx != unstGrid[u] || nbElement != len - unstGrid[u] )
         {
            printf( "ZLEMA period-1 Fail [series %d unst %d]: shape (%d,%d) expected (%d,%d)\n",
                    s, unstGrid[u], begIdx, nbElement, unstGrid[u], len - unstGrid[u] );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( n = 0; n < nbElement; n++ )
         {
            if( memcmp( &out[n], &src[begIdx+n], sizeof(double) ) != 0 )
            {
               printf( "ZLEMA period-1 Fail [series %d unst %d] at out[%d]: "
                       "got %.17g expected the input %.17g (must be BIT-exact)\n",
                       s, unstGrid[u], n, out[n], src[begIdx+n] );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
   return TA_TEST_PASS;
}

/* (4) Deterministic edges on synthetic inputs. */
static ErrorNumber test_zlema_edges( void )
{
   static TA_Real in[64];
   static TA_Real out[OUT_CAP];
   static TA_Real inplace[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int i, lb;

   /* --- (a) flat input: the de-lag of a constant is that same constant
    * doubled minus itself, so every d[] is the constant and the EMA of a
    * constant is the constant. Exactly, not approximately. --- */
   for( i = 0; i < 40; i++ )
      in[i] = 42.125;
   retCode = TA_ZLEMA( 0, 39, in, 14, &begIdx, &nbElement, out );
   lb = TA_ZLEMA_Lookback( 14 );
   if( retCode != TA_SUCCESS || begIdx != lb || nbElement != 40 - lb )
   {
      printf( "ZLEMA flat Fail: rc=%d shape (%d,%d) expected (%d,%d)\n",
              (int)retCode, begIdx, nbElement, lb, 40 - lb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != 42.125 )
      {
         printf( "ZLEMA flat Fail at out[%d]: got %.17g expected 42.125 exactly\n",
                 i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- (b) the published lookback: lag + period - 1, lag truncating. Pinned
    * for the periods where the truncation is visible (an even period lags half
    * a bar less than the rounded convention some references use). --- */
   {
      static const struct { int period; int lookback; } lbGrid[] =
      {
         { 1, 0 }, { 2, 1 }, { 3, 3 }, { 4, 4 }, { 5, 6 },
         { 6, 7 }, { 7, 9 }, { 10, 13 }, { 30, 43 },
      };
      unsigned int k;
      for( k = 0; k < sizeof(lbGrid)/sizeof(lbGrid[0]); k++ )
         if( TA_ZLEMA_Lookback( lbGrid[k].period ) != lbGrid[k].lookback )
         {
            printf( "ZLEMA lookback Fail [period %d]: got %d expected %d\n",
                    lbGrid[k].period, TA_ZLEMA_Lookback( lbGrid[k].period ),
                    lbGrid[k].lookback );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   /* --- (c) a range shorter than the lookback produces nothing, and says so
    * with TA_SUCCESS rather than an error. --- */
   for( i = 0; i < 40; i++ )
      in[i] = 100.0 + i;
   retCode = TA_ZLEMA( 0, 5, in, 30, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "ZLEMA short-range Fail: rc=%d shape (%d,%d) expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* --- (d) a startIdx already past the lookback is honoured verbatim. --- */
   retCode = TA_ZLEMA( 30, 39, in, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 30 || nbElement != 10 )
   {
      printf( "ZLEMA anchored-range Fail: rc=%d shape (%d,%d) expected (30,10)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* --- (e) in-place (outReal == inReal) must be bit-for-bit identical to the
    * separate-buffer result. The write at output index k happens after both of
    * bar k's reads, and the trailing read never reaches a slot already written
    * -- test that, do not argue it. --- */
   retCode = TA_ZLEMA( 0, 39, in, 7, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "ZLEMA in-place Fail: baseline rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < 40; i++ )
      inplace[i] = in[i];
   retCode = TA_ZLEMA( 0, 39, inplace, 7, &begIdx2, &nbElement2, inplace );
   if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
   {
      printf( "ZLEMA in-place Fail: rc=%d shape (%d,%d) vs (%d,%d)\n",
              (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( memcmp( out, inplace, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
   {
      for( i = 0; i < nbElement; i++ )
         if( out[i] != inplace[i] )
         {
            printf( "ZLEMA in-place Fail: bit mismatch at out[%d] separate=%.17g inplace=%.17g\n",
                    i, out[i], inplace[i] );
            break;
         }
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (5) ZLEMA owns no unstable period; it borrows EMA's through ema_lookback.
 * So it must NOT advertise TA_FUNC_FLG_UNST_PER, and TA_FUNC_UNST_EMA must
 * still move both its lookback and its first output. A function that claimed
 * the flag, or one that ignored the setting, would both pass a value check. */
static ErrorNumber test_zlema_unstable( const TA_History *history )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   int savedK = (int)TA_GetUnstablePeriod( TA_FUNC_UNST_EMA );
   int k, endIdx = (int)history->nbBars - 1;
   ErrorNumber result = TA_TEST_PASS;

   if( TA_GetFuncHandle( "ZLEMA", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "ZLEMA unstable Fail: cannot get func handle/info\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   if( funcInfo->flags & TA_FUNC_FLG_UNST_PER )
   {
      printf( "ZLEMA unstable Fail: ZLEMA advertises TA_FUNC_FLG_UNST_PER, but it "
              "owns no TA_FUNC_UNST_ id -- it borrows EMA's\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( k = 0; k <= 5 && result == TA_TEST_PASS; k++ )
   {
      int expectLb;

      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, k );
      expectLb = TA_ZLEMA_Lookback( 10 );

      /* Non-vacuity: the lookback has to actually move with k, or the rest of
       * this loop is measuring nothing. */
      if( expectLb != 13 + k )
      {
         printf( "ZLEMA unstable Fail [k=%d]: lookback %d, expected %d -- the "
                 "EMA unstable period does not reach ZLEMA\n", k, expectLb, 13 + k );
         result = TA_TESTUTIL_TFRR_BAD_CALCULATION;
         break;
      }

      retCode = TA_ZLEMA( 0, endIdx, history->close, 10, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != expectLb ||
          nbElement != endIdx - expectLb + 1 )
      {
         printf( "ZLEMA unstable Fail [k=%d]: rc=%d shape (%d,%d) expected (%d,%d)\n",
                 k, (int)retCode, begIdx, nbElement, expectLb, endIdx - expectLb + 1 );
         result = TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   /* Restore what was there, not 0: this runs inside a suite whose other groups
    * set the same global. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, savedK );
   return result;
}

/* (6) TA_MAType_ZLEMA is member 12, appended after the two pseudo-members. It
 * must dispatch to exactly TA_ZLEMA, bit-for-bit, everywhere an optInMAType is
 * accepted -- the HMA precedent (#139). Three arms, because they reach the
 * dispatch through three different frames. */
static ErrorNumber test_zlema_matype( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rcM, rcZ;
   TA_Integer begM, nbM, begZ, nbZ;
   static TA_Real outMA[OUT_CAP];
   static TA_Real outZL[OUT_CAP];
   static TA_Real outUpper[OUT_CAP];
   static TA_Real outMiddle[OUT_CAP];
   static TA_Real outLower[OUT_CAP];

   nbBars = (int)history->nbBars;

   /* (a) dispatch parity across the period grid. The lookback arm is asserted
    * separately: ma_lookback's switch has its own default (0), and a ZLEMA arm
    * lost from it leaves every value comparison below green -- MA's
    * nothing-to-produce guard just stops firing and the callee clamps for
    * itself. Corpus-wide that is caught by the lookback/call parity sweep over
    * MACDEXT's MAType; this keeps --function=ZLEMA sufficient on its own. */
   for( g = 0; g < NB_ZLEMA_DIFF_GRID; g++ )
   {
      int period = zlemaDiffGrid[g];
      /* Period 1 is excluded: ma_lookback answers 0 there for every MAType,
       * ahead of the switch, which arm (b) covers instead. */
      int lbM = TA_MA_Lookback( period, TA_MAType_ZLEMA );
      int lbZ = TA_ZLEMA_Lookback( period );

      if( period > 1 && lbM != lbZ )
      {
         printf( "ZLEMA matype Fail [period %d]: TA_MA_Lookback %d != TA_ZLEMA_Lookback %d\n",
                 period, lbM, lbZ );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      rcM = TA_MA( 0, nbBars - 1, history->close, period, TA_MAType_ZLEMA,
                   &begM, &nbM, outMA );
      rcZ = TA_ZLEMA( 0, nbBars - 1, history->close, period, &begZ, &nbZ, outZL );

      if( rcM != TA_SUCCESS || rcZ != TA_SUCCESS )
      {
         printf( "ZLEMA matype Fail [period %d]: retCode MA=%d ZLEMA=%d\n",
                 period, (int)rcM, (int)rcZ );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begM != begZ || nbM != nbZ )
      {
         printf( "ZLEMA matype Fail [period %d]: range MA(%d,%d) ZLEMA(%d,%d)\n",
                 period, (int)begM, (int)nbM, (int)begZ, (int)nbZ );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbM; i++ )
         if( memcmp( &outMA[i], &outZL[i], sizeof(double) ) != 0 )
         {
            printf( "ZLEMA matype Fail [period %d] at out[%d]: "
                    "MA %.17g != ZLEMA %.17g (must be BIT-exact)\n",
                    period, i, outMA[i], outZL[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   /* (b) MA(1, ZLEMA): ma.c's identity path runs before the dispatch, so this
    * reaches the copy without entering zlema() at all -- and lands on the same
    * values zlema()'s own period-1 arm produces, which leg (3) pins. */
   rcM = TA_MA( 0, nbBars - 1, history->close, 1, TA_MAType_ZLEMA,
                &begM, &nbM, outMA );
   if( rcM != TA_SUCCESS || begM != 0 || nbM != nbBars )
   {
      printf( "ZLEMA matype Fail: MA(1,ZLEMA) rc=%d beg=%d nb=%d expected 0/%d\n",
              (int)rcM, (int)begM, (int)nbM, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nbBars; i++ )
      if( memcmp( &outMA[i], &history->close[i], sizeof(double) ) != 0 )
      {
         printf( "ZLEMA matype Fail: MA(1,ZLEMA) at out[%d] not an input copy\n", i );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* (c) BBANDS(20, 2, 2, ZLEMA): the middle band IS MA(close, 20, ZLEMA), and
    * BBANDS reaches the dispatch through its own frame rather than MA's. */
   rcM = TA_BBANDS( 0, nbBars - 1, history->close, 20, 2.0, 2.0, TA_MAType_ZLEMA,
                    &begM, &nbM, outUpper, outMiddle, outLower );
   rcZ = TA_ZLEMA( 0, nbBars - 1, history->close, 20, &begZ, &nbZ, outZL );
   if( rcM != TA_SUCCESS || rcZ != TA_SUCCESS || begM != begZ || nbM != nbZ )
   {
      printf( "ZLEMA matype Fail: BBANDS(ZLEMA) rc=%d range(%d,%d) vs ZLEMA rc=%d (%d,%d)\n",
              (int)rcM, (int)begM, (int)nbM, (int)rcZ, (int)begZ, (int)nbZ );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbM; i++ )
      if( memcmp( &outMiddle[i], &outZL[i], sizeof(double) ) != 0 )
      {
         printf( "ZLEMA matype Fail: BBANDS(ZLEMA) middle[%d] %.17g != ZLEMA %.17g\n",
                 i, outMiddle[i], outZL[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   return TA_TEST_PASS;
}

/* (7) Generic startIdx/endIdx range sweep. ZLEMA is an IIR seeded at its own
 * lookback, so it converges rather than matching exactly across startIdx:
 * TA_STABLE_CONVERGING with TA_FUNC_UNST_EMA, which is also the id ZLEMA is
 * mapped to in test_codegen.c's UNSTABLE_MAP. The pair is cross-checked --
 * CONVERGING with TA_TEST_UNST_NONE is a stability mismatch. */
typedef struct { int period; const TA_Real *close; } ZlemaRangeParam;

static TA_RetCode zlemaRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                          TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                          TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                          TA_Integer *lookback, void *opaqueData,
                                          unsigned int outputNb, unsigned int *isOutputInteger )
{
   ZlemaRangeParam *p = (ZlemaRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_ZLEMA_Lookback( p->period );
   return TA_ZLEMA( startIdx, endIdx, p->close, p->period,
                    outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_zlema_range( const TA_History *history )
{
   ZlemaRangeParam param;
   param.period = 20;
   param.close  = history->close;

   return doRangeTestEx( zlemaRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)&param, 1, 0 );
}
