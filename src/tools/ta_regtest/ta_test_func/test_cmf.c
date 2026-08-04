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
 *  072126 MF,CC  First version (issue #134).
 */

/* Description:
 *
 *   Test TA_CMF (Chaikin Money Flow).
 *
 *   CMF is not covered by the --codegen sweep: that sweep diffs against the
 *   frozen ta_ref_serve, which predates this function. Coverage therefore comes
 *   from this file plus server_verify / --xlang-hash.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE (formula correctness) at four periods, absolute
 *        tolerance. Plus in-place aliasing and cross-language bitwise.
 *     2. SELF REGRESSION, bitwise. Catches a future reassociation of the money
 *        flow volume expression, which the tolerance legs would wave through.
 *     3. Deterministic edges that no fuzz shape reaches.
 *     4. Aliasing of the output over each of the four inputs.
 *     5. The generic start/end range sweep.
 *     6. The single-precision entry point, which no other
 *        test in the tree reaches.
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

/* CMF is a zero-crossing oscillator, so a RELATIVE tolerance is the wrong tool:
 * near a centerline crossing the true value passes through ~1e-5 and a relative
 * bound explodes while the absolute error stays at the last bit. Measured worst
 * relative error across these legs is 7.6e-13 -- only 1.3x under a 1e-12
 * relative bound. The absolute error over the same data is at most 2e-15, so an
 * absolute 1e-13 carries ~50x headroom. Do not convert this to a relative tol.
 *
 * The golden values below were captured from two independent implementations
 * that agree with each other to <=2.2e-16 on this data:
 *   1. Tulip Indicators `cmf` (C)                 -- ta-lib-oracles/tulip_serve
 *   2. pandas-ta-classic 0.6.52 `cmf` (Python)    -- ta-lib-oracles/pandas_serve
 * Both trace to Marc Chaikin's published derivation. Spot indices were chosen
 * where |CMF| is largest, i.e. deliberately away from centerline crossings.
 */
#define CMF_ORACLE_TOL 1e-13

typedef struct { int period; int begIdx; int nbElement; } CmfShape;

static const CmfShape cmfShape[] =
{
   { 20, 19, 233 },
   { 21, 20, 232 },
   { 14, 13, 239 },
   {  2,  1, 251 },
   { 100, 99, 153 },   /* > the 50-element static CIRCBUF: exercises the heap path */
};
#define NB_CMF_SHAPE (sizeof(cmfShape)/sizeof(cmfShape[0]))

/* idx is the OUTPUT-array index (0 == global bar `begIdx` for that period). */
typedef struct { int period; int idx; double value; } CmfGolden;

static const CmfGolden cmfOracle[] =
{
   { 20,  86,  0.27488370739394669 },
   { 20, 177, -0.27466797094768686 },
   { 20, 214,  0.36645256949023364 },
   { 20, 215,  0.29510957870722815 },
   { 20, 216,  0.30335574043351538 },

   { 21,  86,  0.28310612850294853 },
   { 21, 176, -0.29635727216658597 },
   { 21, 213,  0.27720692501637051 },
   { 21, 214,  0.32496064144415715 },
   { 21, 215,  0.28358661822021536 },

   { 14, 214,  0.36678858153545052 },
   { 14, 218,  0.33555476247639643 },
   { 14, 220,  0.41524830694319675 },
   { 14, 221,  0.34069264314693642 },
   { 14, 222,  0.34156300098442721 },

   {  2,  26,  0.96885943781186668 },
   {  2,  52, -0.85797575836126816 },
   {  2, 124,  0.88290699597279243 },
   {  2, 195, -0.98682115273522464 },

   /* Period 100 is deliberately above the CIRCBUF static size (50), so these
    * four are the only golden values that exercise the TA_Malloc/TA_Free path. */
   { 100,  97, -0.090913955460523499 },
   { 100, 102, -0.087859635348609119 },
   { 100, 114, -0.095337355152823267 },
   { 100, 116, -0.086854593763862481 },
};
#define NB_CMF_ORACLE (sizeof(cmfOracle)/sizeof(cmfOracle[0]))

/* Our own output, frozen BITWISE. The oracle legs above run at 1e-13 and would
 * not notice the money flow volume expression being re-associated (all orders
 * agree to ~1e-16). This leg would. If it ever fails while the oracle legs pass,
 * the arithmetic was reordered -- decide deliberately, do not just re-capture.
 * 17 significant digits round-trips a double exactly. */
static const CmfGolden cmfSelf[] =
{
   { 20,   0,  0.050071067777419445 },
   { 20,   1,  0.044514975886278806 },
   { 20, 116,  0.0024762946305121832 },
   { 20, 232,  0.029836595668103968  },
};
#define NB_CMF_SELF (sizeof(cmfSelf)/sizeof(cmfSelf[0]))

/**** Local functions declarations. ****/
static ErrorNumber test_cmf_oracle   ( const TA_History *history );
static ErrorNumber test_cmf_self     ( const TA_History *history );
static ErrorNumber test_cmf_edges    ( void );
static ErrorNumber test_cmf_aliasing ( const TA_History *history );
static ErrorNumber test_cmf_range    ( const TA_History *history );
static ErrorNumber test_cmf_single   ( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_cmf( TA_History *history )
{
   ErrorNumber retValue;

   /* CMF has no unstable period; make sure a leftover global setting from an
    * earlier test cannot influence it (it must not). */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_cmf_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmf_self( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmf_edges();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmf_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmf_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_cmf_single( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) External-oracle formula check at four periods, plus cross-language. */
static ErrorNumber test_cmf_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int s, k;

   for( s = 0; s < NB_CMF_SHAPE; s++ )
   {
      int period = cmfShape[s].period;

      retCode = TA_CMF( 0, (int)history->nbBars - 1,
                        history->high, history->low, history->close, history->volume,
                        period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CMF oracle Fail (period %d): retCode = %d\n", period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != cmfShape[s].begIdx || nbElement != cmfShape[s].nbElement )
      {
         printf( "CMF oracle Fail (period %d): shape got (%d,%d) expected (%d,%d)\n",
                 period, begIdx, nbElement, cmfShape[s].begIdx, cmfShape[s].nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* CMF is a ratio of a signed sum to the volume that produced it, so it
       * cannot leave [-1,+1] by more than rounding. Guards against a divisor
       * mix-up that a handful of spot values could miss. */
      for( k = 0; k < (unsigned int)nbElement; k++ )
      {
         if( isnan( out[k] ) || fabs( out[k] ) > 1.0 + 1e-12 )
         {
            printf( "CMF range Fail (period %d): out[%u] = %.17g outside [-1,+1]\n",
                    period, k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      for( k = 0; k < NB_CMF_ORACLE; k++ )
      {
         double want, got, err;

         if( cmfOracle[k].period != period )
            continue;

         want = cmfOracle[k].value;
         got  = out[cmfOracle[k].idx];
         err  = fabs( got - want );

         if( isnan( got ) || err > CMF_ORACLE_TOL )  /* NaN > tol is false -> guard explicitly */
         {
            printf( "CMF oracle Fail (period %d) at out[%d]: got %.17g expected %.17g (abs=%.3e > %.3e)\n",
                    period, cmfOracle[k].idx, got, want, err, CMF_ORACLE_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      /* Cross-language: CMF must be bit-identical on every language server. */
      if( server_verify_active() )
      {
         double optIn[1];
         ErrorNumber e;

         optIn[0] = (double)period;
         e = server_verify( "CMF", 0, (int)history->nbBars - 1, history->nbBars,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low,
                                                history->close, history->volume, NULL },
                            optIn, 1,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (2) Self regression, bitwise. See the cmfSelf comment. */
static ErrorNumber test_cmf_self( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int k;

   retCode = TA_CMF( 0, (int)history->nbBars - 1,
                     history->high, history->low, history->close, history->volume,
                     20, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "CMF self Fail: retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < NB_CMF_SELF; k++ )
   {
      double want = cmfSelf[k].value;
      double got  = out[cmfSelf[k].idx];

      if( memcmp( &got, &want, sizeof(double) ) != 0 )
      {
         printf( "CMF self Fail at out[%d]: got %.17g expected %.17g (bitwise)\n"
                 "  The money flow volume arithmetic changed. The external-oracle\n"
                 "  legs still pass, so this is a reassociation, not a formula error.\n",
                 cmfSelf[k].idx, got, want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) Deterministic edges. None of these are reachable by the fuzz shapes,
 * which generate well-formed bars with volume >= 1000. */
static ErrorNumber test_cmf_edges( void )
{
   static TA_Real high[32], low[32], close[32], volume[32];
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   /* --- Zero volume over the whole window -> 0.0, not NaN (issue #112).
    * Non-vacuous: without the sumVol guard this is 0.0/0.0 == NaN. --- */
   for( i = 0; i < 20; i++ )
   {
      high[i]   = 100.0 + i;
      low[i]    =  99.0 + i;
      close[i]  =  99.5 + i;
      volume[i] =   0.0;
   }
   retCode = TA_CMF( 0, 19, high, low, close, volume, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 4 || nbElement != 16 )
   {
      printf( "CMF zero-volume Fail: rc=%d shape (%d,%d) expected (4,16)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( isnan( out[i] ) || out[i] != 0.0 )
      {
         printf( "CMF zero-volume Fail: out[%d] = %.17g (expected exactly 0.0; NaN => guard missing)\n",
                 i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- Flat bars (high == low) contribute exactly 0 money flow, but their
    * volume still counts in the divisor. Window of 2: one flat bar carrying
    * volume 300 and one bar closing at its high carrying volume 100, so
    * CMF == 100/400 == 0.25 exactly. Without the `tmp > 0.0` guard the flat bar
    * divides by zero and the result is NaN or inf. --- */
   {
      static const TA_Real fh[3] = { 50.0, 50.0, 60.0 };
      static const TA_Real fl[3] = { 50.0, 50.0, 40.0 };
      static const TA_Real fc[3] = { 50.0, 50.0, 60.0 };
      static const TA_Real fv[3] = { 300.0, 300.0, 100.0 };

      retCode = TA_CMF( 0, 2, fh, fl, fc, fv, 2, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 2 )
      {
         printf( "CMF flat-bar Fail: rc=%d shape (%d,%d) expected (1,2)\n",
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( isnan( out[0] ) || out[0] != 0.0 )
      {
         printf( "CMF flat-bar Fail: out[0] = %.17g (two flat bars => exactly 0.0)\n", out[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( isnan( out[1] ) || out[1] != 0.25 )
      {
         printf( "CMF flat-bar Fail: out[1] = %.17g (expected exactly 0.25)\n", out[1] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* --- Malformed bar (low > high) also contributes 0 rather than dividing by
    * a negative range. Same shape as above with the range inverted. --- */
   {
      static const TA_Real mh[3] = { 40.0, 40.0, 60.0 };
      static const TA_Real ml[3] = { 50.0, 50.0, 40.0 };
      static const TA_Real mc[3] = { 45.0, 45.0, 60.0 };
      static const TA_Real mv[3] = { 300.0, 300.0, 100.0 };

      retCode = TA_CMF( 0, 2, mh, ml, mc, mv, 2, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 2 )
      {
         printf( "CMF malformed Fail: rc=%d shape (%d,%d) expected (1,2)\n",
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( isnan( out[1] ) || out[1] != 0.25 )
      {
         printf( "CMF malformed Fail: out[1] = %.17g (expected exactly 0.25)\n", out[1] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* --- Close at the high on every bar pins CMF to exactly +1, close at the
    * low pins it to exactly -1. Proves the multiplier's sign and scale. --- */
   for( i = 0; i < 12; i++ )
   {
      high[i]   = 100.0 + i;
      low[i]    =  90.0 + i;
      close[i]  = high[i];
      volume[i] = 1000.0 + 10.0 * i;
   }
   retCode = TA_CMF( 0, 11, high, low, close, volume, 4, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 3 || nbElement != 9 )
   {
      printf( "CMF close-at-high Fail: rc=%d shape (%d,%d) expected (3,9)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != 1.0 )
      {
         printf( "CMF close-at-high Fail: out[%d] = %.17g (expected exactly 1.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   for( i = 0; i < 12; i++ )
      close[i] = low[i];
   retCode = TA_CMF( 0, 11, high, low, close, volume, 4, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 3 || nbElement != 9 )
   {
      printf( "CMF close-at-low Fail: rc=%d shape (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( out[i] != -1.0 )
      {
         printf( "CMF close-at-low Fail: out[%d] = %.17g (expected exactly -1.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- Close at the midpoint contributes exactly nothing. --- */
   for( i = 0; i < 12; i++ )
      close[i] = ( high[i] + low[i] ) / 2.0;
   retCode = TA_CMF( 0, 11, high, low, close, volume, 4, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "CMF close-at-mid Fail: rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
      if( isnan( out[i] ) || out[i] != 0.0 )
      {
         printf( "CMF close-at-mid Fail: out[%d] = %.17g (expected exactly 0.0)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* --- Whole-series window: period == number of bars gives exactly one
    * output, at the last bar. --- */
   for( i = 0; i < 12; i++ )
      close[i] = high[i];
   retCode = TA_CMF( 0, 11, high, low, close, volume, 12, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 11 || nbElement != 1 )
   {
      printf( "CMF full-window Fail: rc=%d shape (%d,%d) expected (11,1)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( out[0] != 1.0 )
   {
      printf( "CMF full-window Fail: out[0] = %.17g (expected exactly 1.0)\n", out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* --- Not enough bars for a single window: success with an empty range. --- */
   retCode = TA_CMF( 0, 11, high, low, close, volume, 13, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 0 )
   {
      printf( "CMF short-input Fail: rc=%d nbElement=%d (expected success, 0)\n",
              (int)retCode, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) outReal may alias any of the four inputs. The circular buffer carries the
 * trailing volume precisely so that no input element is re-read after the output
 * has overwritten it; without that, aliasing inVolume corrupts the divisor at
 * startIdx == lookbackTotal. */
static ErrorNumber test_cmf_aliasing( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];
   const TA_Real *src[4];
   const char *name[4] = { "inHigh", "inLow", "inClose", "inVolume" };
   int which, i, nb;

   nb = (int)history->nbBars;

   retCode = TA_CMF( 0, nb - 1,
                     history->high, history->low, history->close, history->volume,
                     20, &begIdx, &nbElement, ref );
   if( retCode != TA_SUCCESS )
   {
      printf( "CMF aliasing Fail: baseline retCode = %d\n", (int)retCode );
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

      retCode = TA_CMF( 0, nb - 1, in[0], in[1], in[2], in[3],
                        20, &begIdx2, &nbElement2, work );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "CMF aliasing Fail (%s): rc=%d shape (%d,%d) vs (%d,%d)\n",
                 name[which], (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( memcmp( ref, work, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
      {
         for( i = 0; i < nbElement; i++ )
            if( ref[i] != work[i] )
            {
               printf( "CMF aliasing Fail (%s): bit mismatch at out[%d] separate=%.17g aliased=%.17g\n",
                       name[which], i, ref[i], work[i] );
               break;
            }
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) Generic startIdx/endIdx range sweep (self-coherency + lookback). CMF is a
 * finite moving-window ratio with no unstable period: TA_STABLE_EPSILON, no
 * unstId (matching its abstract metadata). */
typedef struct
{
   int period;
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;
   const TA_Real *volume;
} CmfRangeParam;

static TA_RetCode cmfRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   CmfRangeParam *p = (CmfRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CMF_Lookback( p->period );
   return TA_CMF( startIdx, endIdx, p->high, p->low, p->close, p->volume,
                  p->period, outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_cmf_range( const TA_History *history )
{
   CmfRangeParam param;

   param.period = 20;
   param.high   = history->high;
   param.low    = history->low;
   param.close  = history->close;
   param.volume = history->volume;

   return doRangeTestEx( cmfRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (6) Single-precision entry point, over the full period grid (including the
 * heap CIRCBUF path) and a startIdx > lookback sub-range.
 *
 * TA_S_CMF takes float inputs but must compute in double throughout (PR #33),
 * so on the SAME widened values it must be bit-identical to TA_CMF. Nothing
 * else covers it: test_s_overflow.c is limited to the vector-arithmetic
 * operators, and the --codegen float leg skips any function the frozen
 * ta_ref_serve predates -- which is every function added after the cutover.
 * A single-point check here was demonstrably weak: a sabotaged TA_S_CMF that
 * doubled its output on the period>50 branch passed the entire suite. */
static ErrorNumber test_cmf_single( const TA_History *history )
{
   static TA_Real  outD[OUT_CAP], outS[OUT_CAP];
   static TA_Real  dHigh[OUT_CAP], dLow[OUT_CAP], dClose[OUT_CAP], dVolume[OUT_CAP];
   static float    fHigh[OUT_CAP], fLow[OUT_CAP], fClose[OUT_CAP], fVolume[OUT_CAP];
   TA_RetCode rcD, rcS;
   TA_Integer begD, nbD, begS, nbS;
   unsigned int sh;
   int i, nb, pass;

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

   /* pass 0: full range.  pass 1: a startIdx strictly above the lookback. */
   for( pass = 0; pass < 2; pass++ )
   {
      for( sh = 0; sh < NB_CMF_SHAPE; sh++ )
      {
         int period = cmfShape[sh].period;
         int startIdx = ( pass == 0 ) ? 0 : period + 7;

         if( startIdx > nb - 1 )
            continue;

         rcD = TA_CMF  ( startIdx, nb - 1, dHigh, dLow, dClose, dVolume,
                         period, &begD, &nbD, outD );
         rcS = TA_S_CMF( startIdx, nb - 1, fHigh, fLow, fClose, fVolume,
                         period, &begS, &nbS, outS );

         if( rcD != rcS || begD != begS || nbD != nbS )
         {
            printf( "CMF single Fail [period %d start %d]: rc D=%d S=%d shape D(%d,%d) S(%d,%d)\n",
                    period, startIdx, (int)rcD, (int)rcS, begD, nbD, begS, nbS );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( rcD != TA_SUCCESS )
            continue;

         for( i = 0; i < nbD; i++ )
            if( memcmp( &outD[i], &outS[i], sizeof(TA_Real) ) != 0 )
            {
               printf( "CMF single Fail [period %d start %d] at out[%d]: double %.17g vs TA_S_ %.17g "
                       "(TA_S_ must compute in double -- see PR #33)\n",
                       period, startIdx, i, outD[i], outS[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }

      }
   }

   /* The zero-volume guard exists in both emitted variants, but test_cmf_edges
    * reaches only the double one. Drive the same window through the
    * single-precision entry point too. */
   {
      static float  zh[20], zl[20], zc[20], zv[20];
      static TA_Real zout[OUT_CAP];
      TA_Integer zbeg, znb;

      for( i = 0; i < 20; i++ )
      {
         zh[i] = 100.0f + (float)i;
         zl[i] =  99.0f + (float)i;
         zc[i] =  99.5f + (float)i;
         zv[i] =   0.0f;
      }
      if( TA_S_CMF( 0, 19, zh, zl, zc, zv, 5, &zbeg, &znb, zout ) != TA_SUCCESS )
      {
         printf( "CMF single zero-volume Fail: bad retCode\n" );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < znb; i++ )
         if( isnan( zout[i] ) || zout[i] != 0.0 )
         {
            printf( "CMF single zero-volume Fail: out[%d] = %.17g (expected exactly 0.0)\n",
                    i, zout[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   return TA_TEST_PASS;
}
