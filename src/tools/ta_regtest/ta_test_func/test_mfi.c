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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082326 MF,CC Created. Issue #244.
 */

/* Description:
 *
 *   MFI is a RATIO of positive money flow to total money flow, so its value
 *   cannot depend on the SIZE of that money flow. Issue #244: it did -- the
 *   emitted value was zeroed whenever the window's money flow summed below a
 *   literal 1.0, which any instrument reaches if it is quoted small enough
 *   (a low price, small volumes, or a BTC-quoted pair).
 *
 *   The lineage-bound gates cannot catch that class on their own: ta_ref_serve
 *   is our own frozen pre-cutover source and --fuzz-064 is our own last
 *   release, and BOTH carry the defect. They pin what we did, not what is
 *   correct. So the value legs here are EXTERNAL.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE, two independent implementations (Tulip Indicators in
 *        C, pandas-ta-classic in Python), at an absolute tolerance.
 *     2. SCALE INVARIANCE, exact. Scaling volume by a power of two is exact in
 *        binary, so every money flow, both sums and their ratio scale exactly
 *        -- the output must be BIT-IDENTICAL. This is the direct #244
 *        detector, and it needs no oracle at all. The sweep spans both sides
 *        of the old threshold (see the k list).
 *     3. The oracle values again at 2^-60, where the money flow is ~1e-9 of
 *        natural -- far under the old guard. Leg 2 proves we are
 *        self-consistent; this proves the value we are consistent AT is the
 *        one an outside implementation computes.
 *     4. Range: the output is a 0-100 oscillator and must never leave it. The
 *        sums drift a few ulp as the window slides and a sum whose true value
 *        is near zero can drift negative, which used to emit values outside
 *        the documented range.
 *     5. The empty-window ruling (0/0 -> 0.0), which NEITHER oracle covers:
 *        both divide unguarded and produce NaN there. It is ours by
 *        convention, so it is pinned here rather than borrowed.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

/**** Local declarations. ****/
#define OUT_CAP 300          /* > nbBars */
#define MFI_NB_BARS 252

/* --- LEG 1 + 3 provenance -------------------------------------------------
 * Captured by running BOTH oracles on this suite's 252-bar history, over the
 * lossless hex-of-IEEE-bits transport (issue #115) so each saw byte-identical
 * doubles. Neither number below was computed here.
 *
 *   Tulip Indicators 0.9.2 (C, LGPL), build 1645649572, pinned at
 *   be18abb13e075ba866898dcc7cb52399603302a6, via ta-lib-oracles/tulip_serve,
 *   method TA_MFI.
 *
 *   pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1), via
 *   ta-lib-oracles/pandas_serve, method TA_MFI.
 *
 * Both are genuinely separate evaluations, and each differs from us -- and
 * from the other -- in a way that makes agreement meaningful rather than
 * circular:
 *
 *   - Neither has ANY threshold guard on the ratio. Tulip emits
 *     up/(up+down)*100 unconditionally; pandas emits 100*psum/(psum+nsum).
 *     That is precisely why they can arbitrate #244.
 *   - Tulip's typical price is (h+l+c)*(1.0/3.0) against our (h+l+c)/3.0 --
 *     a different rounding of the same quantity.
 *   - pandas sums with a FRESH-WINDOW .rolling(length).sum() rather than an
 *     add/subtract accumulator, so unlike Tulip and unlike us it carries no
 *     residue drift at all. Where the two oracles disagree with each other
 *     (period 2, index 2: 7.2e-15 against exactly 0) that is the drift, and
 *     both sit inside the bound below.
 *   - pandas emits one extra leading value, a partial window over `length-1`
 *     real bars plus a phantom zero from its NaN first diff. Its index
 *     `idx+1` is our `idx`; the capture aligned them, these are our indices.
 *
 * Worst deviation measured over ALL 710 output values x 3 periods x both
 * volume scales: 7.11e-14 against Tulip, 4.82e-14 against pandas. The bound
 * below carries ~28x headroom.
 *
 * ABSOLUTE, not relative: MFI is a bounded 0-100 oscillator that legitimately
 * reaches exactly 0 and exactly 100, so a relative bound is undefined at the
 * ends and meaningless near them (index 2 of period 2 is 7.2e-15 -- relative
 * error there is 100%, absolute error is nothing).
 */
#define MFI_ORACLE_TOL 2e-12

typedef struct { int period; int idx; double tulip; double pandas; } MfiGolden;

static const MfiGolden mfiOracle[] =
{
   /* period, idx,        Tulip 0.9.2,              pandas-ta-classic 0.6.52 */
   {  2,   0, 100                   , 100                    },
   {  2,   1, 53.489986681886073    , 53.489986681886066     },
   {  2,   2, 7.2063042819866937e-15, 0                      },
   {  2,  62, 100                   , 100                    },
   {  2, 125, 100                   , 100                    },
   {  2, 187, 40.157772208583658    , 40.157772208583673     },
   {  2, 248, 0                     , 0                      },
   {  2, 249, 0                     , 0                      },
   { 14,   0, 42.892339191984448    , 42.892339191984433     },
   { 14,   1, 45.607156851091489    , 45.607156851091482     },
   { 14,   2, 38.87879395159338     , 38.878793951593373     },
   { 14,  59, 35.632094488668137    , 35.63209448866813      },
   { 14, 119, 78.780045284268795    , 78.780045284268795     },
   { 14, 178, 26.565090844618716    , 26.565090844618741     },
   { 14, 236, 46.95018919963556     , 46.950189199635574     },
   { 14, 237, 53.199678850628629    , 53.199678850628658     },
   { 30,   0, 37.58090766368931     , 37.580907663689317     },
   { 30,   1, 37.250835604283047    , 37.250835604283047     },
   { 30,   2, 34.822199591052311    , 34.822199591052311     },
   { 30,  55, 64.949673296628035    , 64.949673296628063     },
   { 30, 111, 58.813945030830297    , 58.813945030830297     },
   { 30, 166, 35.614676987139099    , 35.614676987139092     },
   { 30, 220, 63.988299916573098    , 63.988299916573098     },
   { 30, 221, 65.129156440474773    , 65.129156440474773     },
};
#define NB_MFI_ORACLE (sizeof(mfiOracle)/sizeof(mfiOracle[0]))

/* The periods the golden table covers. */
static const int mfiPeriod[] = { 2, 14, 30 };
#define NB_MFI_PERIOD (sizeof(mfiPeriod)/sizeof(mfiPeriod[0]))

/* Volume scale exponents for leg 2. Natural money flow over this history is
 * ~5e9 for a 14-bar window, so the old `sum < 1.0` guard began firing around
 * 2^-33: k=10/20/30 are the CONTROL arm (the guard never fired there, so they
 * passed before this fix too) and k=40..80 are the arm that caught it. A
 * sweep that only used large k would not show that it is the THRESHOLD that
 * matters rather than small numbers in general. */
static const int mfiScaleExp[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
#define NB_MFI_SCALE (sizeof(mfiScaleExp)/sizeof(mfiScaleExp[0]))

/**** Local functions declarations. ****/
static ErrorNumber test_mfi_oracle     ( const TA_History *history );
static ErrorNumber test_mfi_scale      ( const TA_History *history );
static ErrorNumber test_mfi_range      ( const TA_History *history );
static ErrorNumber test_mfi_empty      ( void );

/* Fill `dst` with the history volume scaled by 2^-exp (exact in binary). */
static void mfi_scale_volume( const TA_History *history, int exp, TA_Real *dst )
{
   int i;
   double f = ldexp( 1.0, -exp );
   for( i = 0; i < (int)history->nbBars; i++ )
      dst[i] = history->volume[i] * f;
}

/**** Global functions definitions. ****/
ErrorNumber test_func_mfi( TA_History *history )
{
   ErrorNumber retValue;

   /* MFI has no unstable period (issues #4/#14); make sure a leftover global
    * setting from an earlier test cannot influence it (it must not). */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_mfi_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_mfi_scale( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_mfi_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_mfi_empty();
   if( retValue != TA_TEST_PASS ) return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* LEG 1 + LEG 3: both external oracles, at natural volume and at 2^-60. */
static ErrorNumber test_mfi_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   static TA_Real vol[OUT_CAP];
   unsigned int g;
   int pass, checks = 0;

   for( pass = 0; pass < 2; pass++ )
   {
      const TA_Real *v;
      const char *tag;

      if( pass == 0 ) { v = history->volume; tag = "natural volume"; }
      else
      {
         /* ~1e-9 of natural: the money flow lands far under the retired
          * `< 1.0` guard, where the shipped library emitted 0. */
         mfi_scale_volume( history, 60, vol );
         v = vol; tag = "volume x 2^-60";
      }

      for( g = 0; g < NB_MFI_ORACLE; g++ )
      {
         int period = mfiOracle[g].period;
         int idx    = mfiOracle[g].idx;

         retCode = TA_MFI( 0, (int)history->nbBars - 1,
                           history->high, history->low, history->close, v,
                           period, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "MFI oracle Fail (%s, period %d): retCode = %d\n",
                    tag, period, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( begIdx != period || nbElement != (int)history->nbBars - period )
         {
            printf( "MFI oracle Fail (%s, period %d): shape got (%d,%d) expected (%d,%d)\n",
                    tag, period, begIdx, nbElement,
                    period, (int)history->nbBars - period );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         if( idx >= nbElement )
         {
            printf( "MFI oracle Fail: golden idx %d beyond nbElement %d\n", idx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( fabs( out[idx] - mfiOracle[g].tulip ) > MFI_ORACLE_TOL )
         {
            printf( "MFI Tulip oracle Fail (%s, period %d, idx %d): got %.17g expected %.17g (d=%.3g)\n",
                    tag, period, idx, out[idx], mfiOracle[g].tulip,
                    fabs( out[idx] - mfiOracle[g].tulip ) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( fabs( out[idx] - mfiOracle[g].pandas ) > MFI_ORACLE_TOL )
         {
            printf( "MFI pandas-ta oracle Fail (%s, period %d, idx %d): got %.17g expected %.17g (d=%.3g)\n",
                    tag, period, idx, out[idx], mfiOracle[g].pandas,
                    fabs( out[idx] - mfiOracle[g].pandas ) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         checks += 2;
      }
   }

   printf( "  MFI external oracles: %d comparison(s) vs Tulip 0.9.2 + pandas-ta-classic 0.6.52,"
           " at natural volume and 2^-60 (issue #244)\n", checks );
   return TA_TEST_PASS;
}

/* LEG 2: exact scale invariance. This is the #244 regression detector. */
static ErrorNumber test_mfi_scale( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, refBegIdx, refNbElement;
   static TA_Real ref[OUT_CAP];
   static TA_Real out[OUT_CAP];
   static TA_Real vol[OUT_CAP];
   unsigned int p, s;
   int k, compares = 0;

   for( p = 0; p < NB_MFI_PERIOD; p++ )
   {
      int period = mfiPeriod[p];

      retCode = TA_MFI( 0, (int)history->nbBars - 1,
                        history->high, history->low, history->close, history->volume,
                        period, &refBegIdx, &refNbElement, ref );
      if( retCode != TA_SUCCESS )
      {
         printf( "MFI scale Fail (period %d): reference retCode = %d\n", period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( s = 0; s < NB_MFI_SCALE; s++ )
      {
         mfi_scale_volume( history, mfiScaleExp[s], vol );

         retCode = TA_MFI( 0, (int)history->nbBars - 1,
                           history->high, history->low, history->close, vol,
                           period, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "MFI scale Fail (period %d, 2^-%d): retCode = %d\n",
                    period, mfiScaleExp[s], (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( begIdx != refBegIdx || nbElement != refNbElement )
         {
            printf( "MFI scale Fail (period %d, 2^-%d): shape got (%d,%d) expected (%d,%d)\n",
                    period, mfiScaleExp[s], begIdx, nbElement, refBegIdx, refNbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* BIT-identical, not "close": scaling volume by a power of two is
          * exact, so every money flow, both sums and their ratio scale
          * exactly and the ratio's scale factor cancels. Any difference at
          * all is a magnitude-dependent decision inside the body. */
         for( k = 0; k < nbElement; k++ )
         {
            if( memcmp( &out[k], &ref[k], sizeof(TA_Real) ) != 0 )
            {
               printf( "MFI scale Fail (period %d, volume x 2^-%d): out[%d] = %.17g,"
                       " natural volume gives %.17g. MFI is a ratio -- scaling every"
                       " volume by a power of two must not change it (issue #244).\n",
                       period, mfiScaleExp[s], k, out[k], ref[k] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            compares++;
         }
      }
   }

   printf( "  MFI scale invariance: %d bit-exact comparison(s) over %d volume scales"
           " spanning the retired 1.0 threshold (issue #244)\n",
           compares, (int)NB_MFI_SCALE );
   return TA_TEST_PASS;
}

/* LEG 4: a 0-100 oscillator must stay in 0-100, at every period. */
static ErrorNumber test_mfi_range( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   int period, k, checked = 0;

   for( period = 2; period <= 60; period++ )
   {
      retCode = TA_MFI( 0, (int)history->nbBars - 1,
                        history->high, history->low, history->close, history->volume,
                        period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "MFI range Fail (period %d): retCode = %d\n", period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( k = 0; k < nbElement; k++ )
      {
         if( isnan( out[k] ) || out[k] < 0.0 || out[k] > 100.0 )
         {
            printf( "MFI range Fail (period %d): out[%d] = %.17g outside [0,100]\n",
                    period, k, out[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         checked++;
      }
   }

   printf( "  MFI range: %d value(s) over periods 2..60, all within [0,100]\n", checked );
   return TA_TEST_PASS;
}

/* LEG 5: the empty window. Neither oracle covers this -- both divide
 * unguarded and return NaN -- so the 0.0 ruling is pinned here.
 *
 * Two ways a bar contributes nothing: the typical price does not move, or it
 * moves but carries no volume. A window of either is 0/0. The running sums
 * cannot detect that by themselves (add-then-subtract leaves residue of
 * arbitrary sign), which is why the guard cannot simply be a smaller
 * threshold. Both shapes follow a busy stretch, so the sums DO hold residue
 * by the time the window empties.
 *
 * The phase sweep is what makes this a detector rather than a coin flip. The
 * residue's SIGN depends on the preceding series, and a guard that merely
 * tests the total against zero catches the empty window only when the residue
 * happens to land <= 0 -- measured at roughly half of phases. One phase would
 * therefore pass such a guard half the time. Over 24 phases x 2 shapes it
 * does not. */
static ErrorNumber test_mfi_empty( void )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real h[MFI_NB_BARS], l[MFI_NB_BARS], c[MFI_NB_BARS], v[MFI_NB_BARS];
   static TA_Real out[OUT_CAP];
   const int period = 14;
   int shape, i, k, ph;
   int checked = 0;

   for( shape = 0; shape < 2; shape++ )
   for( ph = 0; ph < 24; ph++ )
   {
      double phase = ph * 0.0313;
      /* A busy stretch, so the accumulators carry real magnitude (and then
       * real residue) into the dead window that follows. */
      for( i = 0; i < MFI_NB_BARS - 40; i++ )
      {
         double t = 1000.0 * ( 1.0 + 0.3 * sin( i * 0.7 + phase ) );
         h[i] = t * 1.01; l[i] = t * 0.99; c[i] = t; v[i] = 1.0e6 + ( i % 97 );
      }
      /* Then 40 bars that put nothing into the window. */
      for( ; i < MFI_NB_BARS; i++ )
      {
         if( shape == 0 )
         {
            /* Flat: typical price never changes. */
            double t = 1000.0 * ( 1.0 + 0.3 * sin( ( MFI_NB_BARS - 41 ) * 0.7 + phase ) );
            h[i] = t * 1.01; l[i] = t * 0.99; c[i] = t; v[i] = 1234.0;
         }
         else
         {
            /* Halted: the quote still drifts, but nothing trades. */
            double t = 1000.0 * ( 1.0 + 0.3 * sin( i * 0.7 + phase ) );
            h[i] = t * 1.01; l[i] = t * 0.99; c[i] = t; v[i] = 0.0;
         }
      }

      retCode = TA_MFI( 0, MFI_NB_BARS - 1, h, l, c, v,
                        period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "MFI empty-window Fail (shape %d, phase %d): retCode = %d\n", shape, ph, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* The last bar's window is entirely inside the dead stretch. */
      k = nbElement - 1;
      if( out[k] != 0.0 )
      {
         printf( "MFI empty-window Fail (shape %d, phase %d): out[%d] = %.17g, expected"
                 " exactly 0.0. A window with no money flow at all is 0/0; TA-Lib"
                 " reports 0. A non-zero here is the accumulators' rounding residue"
                 " divided by itself.\n",
                 shape, ph, k, out[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      checked++;
   }

   printf( "  MFI empty window: %d case(s) -- flat price and zero volume, each over 24"
           " price phases -- report 0.0 rather than accumulator residue\n", checked );
   return TA_TEST_PASS;
}
