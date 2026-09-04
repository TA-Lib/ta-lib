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
 *  090426 MF,CC  First version (issue #346).
 */

/* Description:
 *
 *   Test TA_VHF (Vertical Horizontal Filter).
 *
 *   Legs:
 *     1. DIFFERENTIAL against a compose over TA_MAX / TA_MIN / TA_SUM. Only
 *        the per-bar |change| is hand-written -- TA-Lib ships no vector ABS --
 *        so the window pair, the anchor and the divide all come from shipped
 *        primitives. NOT bit-exact, and deliberately so: TA_SUM carries a
 *        running accumulator where TA_VHF re-sums the window every bar. The
 *        two agree to 1.0e-14 relative / 2.6e-15 absolute, measured over 1.7M
 *        values across six corpora and every period from 2 to 100; a wrong
 *        formula misses by whole percent.
 *     2. EXTERNAL ORACLES: three independent implementations of VHF, in three
 *        languages, agreeing BIT-FOR-BIT with each other and with TA_VHF over
 *        all 709 values of the frozen corpus. See vhfOracle below.
 *     3. Two PUBLISHED vectors, reproduced at their printed precision. One is
 *        a table from a book printed decades before this file.
 *     4. Exact-arithmetic edges, on inputs whose every intermediate is an exact
 *        binary fraction: a flat-lead-in ramp is exactly 1.0 at its first bar
 *        and exactly (N-1)/N after it -- the second value is what pins the
 *        window ASYMMETRY, since co-terminal windows would report 1.0 forever.
 *        An all-flat window is exactly 0.0 and never NaN.
 *     5. In-place aliasing (outReal == inReal), bitwise, over both corpora and
 *        every period. The window is re-scanned from the input on every bar,
 *        so this is the leg that would catch a store landing under a later
 *        read.
 *     6. The startIdx/endIdx range sweep, in the EXACT class: every bar is
 *        recomputed from its own window, so no range may move a value at all.
 *
 *   Cross-language value coverage comes from server_verify in leg 2 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as RMA).
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
#define VHF_CAP    3100
#define VHF_GD_NB  1000
#define VHF_SYN_NB 2000

/* Leg 1. Measured worst 1.03e-14 relative / 2.55e-15 absolute, over 1.7M values
 * spanning six corpora -- the three swept below plus three magnitude extremes
 * (1e6 with a 1e-4 oscillation, 1e8 quantised to 1e-3, a flat-noise 9000) --
 * and every period from 2 to 100. Two decimal orders of headroom on each. */
#define VHF_DIFF_REL 1e-12
#define VHF_DIFF_ABS 1e-13

/* Leg 2. Every oracle value below is BIT-IDENTICAL to ours on this platform,
 * so the tolerance is headroom for cross-platform rounding of the window sum,
 * not for a measured gap. Relative only: VHF is bounded in [0,1] and the
 * frozen rows bottom out at 0.158, so an absolute companion would never be the
 * binding term. The one place VHF reaches zero -- the exactly flat window -- is
 * answered exactly and gated by leg 4 instead. */
#define VHF_ORACLE_REL 1e-12

/* An upper bound is a real property of VHF, not an implementation detail: the
 * extrema are two points the summed changes connect, so num <= den exactly.
 * The slack is for the FP inequality only -- num is one correctly-rounded
 * subtraction while den is a rounded sum, so a window at the bound can land a
 * few ulp above 1. */
#define VHF_BOUND_SLACK 8e-16

typedef struct { int period; int bar; double want; } VhfGolden;

/* Goldens captured by ta-lib-oracles/capture_346_vhf.py on the 252-bar
 * TA_SREF close series (TA_SREF_close_daily_ref_0_PRIV), at %.17g, which
 * round-trips to the same double. `bar` is the ABSOLUTE bar index; the output
 * index is bar - begIdx.
 *
 * THREE independent implementations, in three languages, each driven on this
 * exact series (2026-09-04) and each agreeing BIT-FOR-BIT with the other two
 * over all 709 values (0 disagreements; the capture script refuses to print a
 * table when that count is non-zero):
 *   1. Tulip Indicators 0.9.2, pinned be18abb -- C, `ti_vhf`. Ships its own
 *      committed golden vectors for VHF, replayed by leg 3.
 *   2. pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1) -- Python,
 *      `ta.vhf`. Sums the window fresh, where Tulip rolls an accumulator, so
 *      the two agreeing is not one summation order checked twice.
 *   3. trading-signals 8.3.0 -- TypeScript, `ts.VHF`.
 * They diverge only where this corpus does not go: on an all-flat input the
 * three answer NaN, +Inf and 0.0 respectively (measured, period 5), against
 * TA_VHF's 0.0 (issue #112). No window in this corpus is flat.
 * The lowest-valued bar of each period is included: it is where a relative
 * tolerance is least forgiving. */
static const VhfGolden vhfOracle[] =
{
   {   5,   5,    0.19819140919366987 },
   {   5,   6,    0.47368421052631604 },
   {   5,  66,    0.31821797931583151 },
   {   5, 128,    0.41379310344827586 },
   {   5, 190,    0.35774410774410786 },
   {   5, 206,    0.19446845289541931 },
   {   5, 251,    0.61980830670926534 },
   {  14,  14,     0.2941678035470669 },
   {  14,  15,    0.31002875629043863 },
   {  14,  73,    0.55192150449713817 },
   {  14, 133,    0.74695270599707464 },
   {  14, 192,    0.38328690807799443 },
   {  14, 250,    0.17848970251716254 },
   {  14, 251,    0.22301644031451054 },
   {  28,  28,    0.28895638235045984 },
   {  28,  29,    0.30539456085599648 },
   {  28,  84,    0.44960474308300402 },
   {  28, 140,    0.35904201339557551 },
   {  28, 163,    0.15791848398255387 },
   {  28, 196,    0.41681748573019634 },
   {  28, 251,    0.31557122123159842 },
};
#define NB_VHF_ORACLE ((int)(sizeof(vhfOracle)/sizeof(VhfGolden)))

/* Leg 3. Neither vector is ours and neither was recomputed here: both are
 * upstream Tulip's own committed expected output, and the second of those is
 * transcribed from a book table. They pin the anchor and the window pair
 * against sources that have never seen this implementation. */
typedef struct
{
   const char   *source;
   int           nbIn;
   const double *in;
   int           nbOut;
   const double *out;
   int           decimals;
} VhfBookVector;

/* tests/untest.txt:450 of Tulip Indicators 0.9.2 (`vhf 5`), 3 decimals. */
static const double vhfTulipVecIn[15] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const double vhfTulipVecOut[10] =
   { 0.720, 0.232, 0.432, 0.553, 0.640, 0.796, 0.625, 0.771, 0.947, 0.576 };

/* Steven B. Achelis, "Technical Analysis from A to Z", page 354, transcribed
 * into Tulip's tests/atoz.txt:260 (`vhf 5`, `#page 354`), 4 decimals. Its last
 * value is a hard 1: that window's oldest change is zero, so the numerator and
 * the denominator are the same distance. */
static const double vhfAtozVecIn[9] =
   { 44.3125, 44.125, 45.0625, 44.8125, 44.8125, 45.25, 46.125, 47.375, 49 };
static const double vhfAtozVecOut[4] =
   { 0.6207, 0.525, 0.9111, 1 };

static const VhfBookVector vhfBookVectors[] =
{
   { "Tulip Indicators 0.9.2 tests/untest.txt:450", 15, vhfTulipVecIn, 10, vhfTulipVecOut, 3 },
   { "Achelis, TA from A to Z p.354 (atoz.txt:260)",  9, vhfAtozVecIn,   4, vhfAtozVecOut,  4 },
};
#define NB_VHF_BOOK ((int)(sizeof(vhfBookVectors)/sizeof(VhfBookVector)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_vhfDiffCmp;
static int g_vhfOracleCmp;
static int g_vhfBookCmp;
static int g_vhfEdgeCmp;
static int g_vhfAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_vhf_differential( const char *tag, const TA_Real *in,
                                          int nbBars, int maxPeriod );
static ErrorNumber test_vhf_oracle( const TA_History *history );
static ErrorNumber test_vhf_published( void );
static ErrorNumber test_vhf_edges( void );
static ErrorNumber test_vhf_aliasing( const char *tag, const TA_Real *in,
                                      int nbBars, int maxPeriod );
static ErrorNumber test_vhf_range( const TA_Real *in );

/* A small-magnitude oscillating series, built from an exact rule so nothing is
 * transported. Its closes sit near 1e-5 with 30% swings, which is where the
 * running accumulator of TA_SUM and the per-bar re-sum of TA_VHF disagree the
 * most (leg 1's tolerance is measured on it, not on the price corpora, where
 * the two are bit-identical up to period 52). */
static void vhfBuildSmall( double *dest, int nb )
{
   int i;
   for( i = 0; i < nb; i++ )
      dest[i] = 1.0e-5 * ( 1.0 + 0.3 * (double)( ( ( i * 37 ) % 401 ) - 200 ) / 401.0 );
}

/**** Global functions definitions. ****/
ErrorNumber test_func_vhf( TA_History *history )
{
   static TA_Real small[VHF_SYN_NB];
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   /* VHF has no unstable period; a leftover global setting must not reach it,
    * and the range sweep below asserts the same thing from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_vhfDiffCmp = g_vhfOracleCmp = g_vhfBookCmp = 0;
   g_vhfEdgeCmp = g_vhfAliasCmp = 0;

   vhfBuildSmall( small, VHF_SYN_NB );

   err = test_vhf_differential( "TA_SREF close", history->close, nbBars, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_differential( "gData close", gDataClose, VHF_GD_NB, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_differential( "small-magnitude", small, VHF_SYN_NB, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_published();
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_aliasing( "TA_SREF close", history->close, nbBars, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_aliasing( "small-magnitude", small, VHF_SYN_NB, 60 );
   if( err != TA_TEST_PASS )
      return err;

   err = test_vhf_range( history->close );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_vhfDiffCmp != 186381 || g_vhfOracleCmp != NB_VHF_ORACLE
            || g_vhfBookCmp != 14 || g_vhfEdgeCmp != 9243
            || g_vhfAliasCmp != 129210 ) )
   {
      printf( "VHF Fail: coverage counters (diff %d, oracle %d, published %d, "
              "edges %d, alias %d) are not what this file was written with "
              "(186381, %d, 14, 9243, 129210)\n",
              g_vhfDiffCmp, g_vhfOracleCmp, g_vhfBookCmp, g_vhfEdgeCmp,
              g_vhfAliasCmp, NB_VHF_ORACLE );
      return TA_VHF_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) TA_VHF against a compose over shipped primitives.
 *
 * The reference re-derives the whole formula from TA_MAX, TA_MIN and TA_SUM,
 * with one hand-written line for the per-bar |change| because TA-Lib ships no
 * vector ABS. TA_SUM's window is anchored one bar EARLIER than TA_MAX's, which
 * is the asymmetry that makes the lookback optInTimePeriod rather than
 * optInTimePeriod-1; getting that wrong here would show up as a whole-percent
 * miss, not a rounding one.
 *
 * The [0,1] bound is checked in the same pass, on every value.
 */
static ErrorNumber test_vhf_differential( const char *tag, const TA_Real *in,
                                          int nbBars, int maxPeriod )
{
   static TA_Real chg[VHF_CAP], hi[VHF_CAP], lo[VHF_CAP], sum[VHF_CAP], out[VHF_CAP];
   TA_Integer begHi, nbHi, begLo, nbLo, begSum, nbSum, begIdx, nbElement;
   TA_RetCode retCode;
   int period, i, t;

   for( i = 0; i + 1 < nbBars; i++ )
      chg[i] = fabs( in[i+1] - in[i] );

   for( period = 2; period <= maxPeriod; period++ )
   {
      if( TA_MAX( 0, nbBars-1, in, period, &begHi, &nbHi, hi ) != TA_SUCCESS ||
          TA_MIN( 0, nbBars-1, in, period, &begLo, &nbLo, lo ) != TA_SUCCESS ||
          TA_SUM( 0, nbBars-2, chg, period, &begSum, &nbSum, sum ) != TA_SUCCESS )
      {
         printf( "VHF differential [%s N=%d]: reference compose failed\n", tag, period );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_VHF( 0, nbBars-1, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != nbBars - period )
      {
         printf( "VHF differential Fail [%s N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 tag, period, (int)retCode, begIdx, nbElement,
                 period, nbBars - period );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( t = period; t < nbBars; t++ )
      {
         double num  = hi[t-(period-1)] - lo[t-(period-1)];
         double den  = sum[t-period];
         double want = ( den > 0.0 ) ? num / den : 0.0;
         double got  = out[t-begIdx];
         double err;
         const char *mode;

         g_vhfDiffCmp++;
         if( !checkOracleValue( got, want, VHF_DIFF_REL, VHF_DIFF_ABS, &err, &mode ) )
         {
            printf( "VHF differential Fail [%s N=%d] at bar %d: got %.17g "
                    "expected %.17g (%s err %.3g, tol rel %g abs %g)\n",
                    tag, period, t, got, want, mode, err,
                    VHF_DIFF_REL, VHF_DIFF_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( !( got >= 0.0 ) || got > 1.0 + VHF_BOUND_SLACK )
         {
            printf( "VHF bound Fail [%s N=%d] at bar %d: %.17g is outside "
                    "[0,1]; the extrema are two points the summed changes "
                    "connect, so the ratio cannot exceed 1\n",
                    tag, period, t, got );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) The frozen three-oracle goldens, plus the cross-language replay. */
static ErrorNumber test_vhf_oracle( const TA_History *history )
{
   static TA_Real out[VHF_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "VHF oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_VHF_ORACLE; k++ )
   {
      double got, err;
      const char *mode;

      if( vhfOracle[k].period != lastPeriod )
      {
         lastPeriod = vhfOracle[k].period;
         retCode = TA_VHF( 0, nbBars-1, history->close, lastPeriod,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod
             || nbElement != nbBars - lastPeriod )
         {
            printf( "VHF oracle Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    lastPeriod, nbBars - lastPeriod );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "VHF", 0, nbBars-1, nbBars,
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
               printf( "VHF oracle [N=%d]: compared no server despite live "
                       "pipes\n", lastPeriod );
               return TA_VHF_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( vhfOracle[k].bar < begIdx || vhfOracle[k].bar - begIdx >= nbElement )
      {
         printf( "VHF oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", vhfOracle[k].period, vhfOracle[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_VHF_VACUOUS;
      }

      got = out[vhfOracle[k].bar - begIdx];
      g_vhfOracleCmp++;
      if( !checkOracleValue( got, vhfOracle[k].want, VHF_ORACLE_REL, 0.0,
                             &err, &mode ) )
      {
         printf( "VHF oracle Fail [N=%d] at bar %d: got %.17g expected %.17g "
                 "(%s err %.3g, tol rel %g)\n",
                 vhfOracle[k].period, vhfOracle[k].bar, got,
                 vhfOracle[k].want, mode, err, VHF_ORACLE_REL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The two published vectors, at their printed precision. */
static ErrorNumber test_vhf_published( void )
{
   static TA_Real out[VHF_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int v, i;

   for( v = 0; v < NB_VHF_BOOK; v++ )
   {
      const VhfBookVector *bv = &vhfBookVectors[v];
      double quantum = pow( 10.0, (double)bv->decimals );

      retCode = TA_VHF( 0, bv->nbIn-1, bv->in, 5, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != bv->nbOut )
      {
         printf( "VHF published Fail [%s]: rc=%d (%d,%d) expected (5,%d)\n",
                 bv->source, (int)retCode, begIdx, nbElement, bv->nbOut );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double rounded = floor( out[i] * quantum + 0.5 ) / quantum;

         g_vhfBookCmp++;
         if( rounded != bv->out[i] )
         {
            printf( "VHF published Fail [%s] out %d: got %.10f -> %.*f, "
                    "expected %.*f\n",
                    bv->source, i, out[i], bv->decimals, rounded,
                    bv->decimals, bv->out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Exact-arithmetic edges.
 *
 * Every close below is an exact binary fraction and every change is exactly
 * 0.5, so each partial sum is exact and the two assertions are equalities, not
 * tolerances.
 *
 *   - A ramp with ONE repeated close at its head. The first output's window
 *     opens on that repeat, so its oldest change is zero and the numerator and
 *     the denominator are the same distance: exactly 1.0.
 *   - Every later output spans N equal changes across N+1 closes but only N-1
 *     of them between its extrema: exactly (N-1)/N. This is the assertion that
 *     pins the window ASYMMETRY -- co-terminal windows would report 1.0 here
 *     too, and leg 3's book vector alone would not tell the two apart.
 *   - An all-flat input has no vertical and no horizontal movement: exactly
 *     0.0. Non-vacuous by construction -- the denominator is exactly zero, so
 *     without the guard the divide yields NaN, and NaN fails both comparisons
 *     below.
 */
static ErrorNumber test_vhf_edges( void )
{
   static TA_Real in[128], out[VHF_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, i, sign;

   for( sign = 1; sign >= -1; sign -= 2 )
   {
      /* in[0] == in[1], then a ramp in exact halves. */
      in[0] = 64.0;
      for( i = 1; i < 100; i++ )
         in[i] = 64.0 + (double)sign * 0.5 * (double)( i - 1 );

      for( period = 2; period <= 40; period++ )
      {
         double want = (double)( period - 1 ) / (double)period;

         retCode = TA_VHF( 0, 99, in, period, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
         {
            printf( "VHF ramp Fail [N=%d sign=%d]: rc=%d (%d,%d)\n",
                    period, sign, (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         g_vhfEdgeCmp++;
         if( out[0] != 1.0 )
         {
            printf( "VHF ramp Fail [N=%d sign=%d]: first output %.17g, expected "
                    "exactly 1.0 -- that window's oldest change is zero, so the "
                    "range and the path are the same distance\n",
                    period, sign, out[0] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 1; i < nbElement; i++ )
         {
            g_vhfEdgeCmp++;
            if( out[i] != want )
            {
               printf( "VHF ramp Fail [N=%d sign=%d] out %d: %.17g, expected "
                       "exactly %.17g. N equal changes span N+1 closes but only "
                       "N-1 of them lie between the extrema; a co-terminal "
                       "window would report 1.0 here\n",
                       period, sign, i, out[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   for( i = 0; i < 100; i++ )
      in[i] = 42.0;
   for( period = 2; period <= 40; period++ )
   {
      retCode = TA_VHF( 0, 99, in, period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period || nbElement != 100 - period )
      {
         printf( "VHF flat Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_vhfEdgeCmp++;
         if( isnan( out[i] ) || out[i] != 0.0 )
         {
            printf( "VHF flat Fail [N=%d] out %d: %.17g, expected exactly 0.0 "
                    "(NaN => the zero-denominator guard is missing)\n",
                    period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inReal, bitwise.
 *
 * Each bar re-reads optInTimePeriod+1 input values, the oldest of which sits at
 * exactly the slot the FIRST output was written to when startIdx is at the
 * lookback. That one bar of clearance is the whole invariant, and it holds only
 * because the entire window is read before the store.
 */
static ErrorNumber test_vhf_aliasing( const char *tag, const TA_Real *in,
                                      int nbBars, int maxPeriod )
{
   static TA_Real clean[VHF_CAP], alias[VHF_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int period, i;

   for( period = 2; period <= maxPeriod; period++ )
   {
      retCode = TA_VHF( 0, nbBars-1, in, period, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "VHF alias Fail [%s N=%d]: rc=%d\n", tag, period, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = in[i];
      retCode = TA_VHF( 0, nbBars-1, alias, period, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "VHF alias Fail [%s N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 tag, period, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_vhfAliasCmp++;
         if( clean[i] != alias[i] )
         {
            printf( "VHF alias Fail [%s N=%d] out %d: separate %.17g, in-place "
                    "%.17g -- a store landed under a read the next bar still "
                    "needed\n", tag, period, i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The startIdx/endIdx range sweep. TA_STABLE_EXACT: every bar is
 * recomputed from its own window with no carried state, so no range may move
 * a value by even one ulp. No unstable-period id (matching its abstract
 * metadata, which doRangeTestEx cross-checks against the stability class). */
typedef struct { int period; const TA_Real *in; } VhfRangeParam;

static TA_RetCode vhfRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   VhfRangeParam *p = (VhfRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_VHF_Lookback( p->period );
   return TA_VHF( startIdx, endIdx, p->in, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_vhf_range( const TA_Real *in )
{
   VhfRangeParam param;

   param.period = 28;
   param.in     = in;

   return doRangeTestEx( vhfRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
