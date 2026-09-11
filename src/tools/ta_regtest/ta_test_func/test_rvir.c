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
 *  091126 KL     First version (proposal-drafts issue #68).
 *
 */

/* Description:
 *     Test TA_RVIR, the 1995 refined Relative Volatility Index: TA_RVI over the
 *     highs and again over the lows, averaged.
 *
 *     WHAT EACH LEG CAN AND CANNOT SEE. The function is implemented by calling
 *     TA_RVI twice, so a leg written against TA_RVI is structurally true and
 *     cannot fail for an arithmetic reason -- it pins the API contract for a
 *     later rewrite, nothing more. Leg 1 is therefore the one that carries the
 *     arithmetic: it rebuilds both legs from TA_STDDEV and TA_RMA, so it holds
 *     even if TA_RVI and TA_RVIR are both wrong in the same way.
 *
 *     The discriminating power of each leg was measured by mutation rather than
 *     assumed, each mutation applied to the generator input and regenerated:
 *
 *       averaging dropped, the high leg returned alone
 *                          -> leg 1 RED, leg 2 RED, leg 4 RED, leg 3 green
 *       the two TA_RVI calls swapped
 *                          -> every leg green, and correctly so: TA_RVI's write
 *                             index trails its read index, so neither order can
 *                             corrupt an aliased input. The order is not what
 *                             makes leg 6 pass, which is why leg 5 reaches for
 *                             the scratch buffer's extent instead.
 *       lookback raised by one
 *                          -> leg 3 RED, leg 1 green. Leg 1 anchors its
 *                             reference at the reported outBegIdx, so it
 *                             follows a shifted window instead of catching it.
 *       every scratch allocation one element short
 *                          -> leg 5 RED (heap overflow on the last bar)
 *
 *     Leg 3 and leg 1 are the pair that covers the warm-up: neither sees what
 *     the other does.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

/**** Local declarations. ****/
#define RVIR_CAP 1100

/* optInStdDevPeriod >= 2: a one-bar deviation window is identically zero.
 * optInTimePeriod == 1 is the no-memory case both leg 3 and leg 4 need. */
static const int rvirSdPeriods[]   = { 2, 3, 10, 14, 30 };
static const int rvirTimePeriods[] = { 1, 2, 14, 30 };
#define NB_RVIR_SD   ((int)(sizeof(rvirSdPeriods)/sizeof(int)))
#define NB_RVIR_TIME ((int)(sizeof(rvirTimePeriods)/sizeof(int)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_rvirDiffCmp;
static int g_rvirCompCmp;
static int g_rvirDegenCmp;
static int g_rvirTieCmp;
static int g_rvirEdgeCmp;
static int g_rvirAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber rvir_reference_leg( const TA_Real *in, int nbBars, int startIdx,
                                       int period, int sdPeriod, int unst,
                                       TA_Real *ref, int *nbRef, const char *tag );
static ErrorNumber test_rvir_differential( const TA_History *history );
static ErrorNumber test_rvir_composite( const TA_History *history );
static ErrorNumber test_rvir_degenerate( const TA_History *history );
static ErrorNumber test_rvir_tie( const TA_History *history );
static ErrorNumber test_rvir_edges( void );
static ErrorNumber test_rvir_aliasing( const TA_History *history );
static ErrorNumber test_rvir_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_rvir( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_rvirDiffCmp = g_rvirCompCmp = g_rvirDegenCmp = 0;
   g_rvirTieCmp = g_rvirEdgeCmp = g_rvirAliasCmp = 0;

   err = test_rvir_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_composite( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_degenerate( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_tie( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_rvir_range( history );
   if( err != TA_TEST_PASS )
      return err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( history->nbBars == 252
       && ( g_rvirDiffCmp != 39757 || g_rvirCompCmp != 4609
            || g_rvirDegenCmp != 13707 || g_rvirTieCmp != 243
            || g_rvirEdgeCmp != 106592 || g_rvirAliasCmp != 9218 ) )
   {
      printf( "RVIR Fail: coverage counters (diff %d, composite %d, degenerate "
              "%d, tie %d, edges %d, alias %d) are not what this file was "
              "written with (39757, 4609, 13707, 243, 106592, 9218)\n",
              g_rvirDiffCmp, g_rvirCompCmp, g_rvirDegenCmp, g_rvirTieCmp,
              g_rvirEdgeCmp, g_rvirAliasCmp );
      return TA_RVIR_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* One leg of the reference, rebuilt from TA_STDDEV and two TA_RMA legs exactly
 * as test_rvi.c's differential does, including the four anchoring details that
 * each move the result at the 1e-13 level if dropped. Returns the leg's values
 * from `startIdx` onward.
 */
static ErrorNumber rvir_reference_leg( const TA_Real *in, int nbBars, int startIdx,
                                       int period, int sdPeriod, int unst,
                                       TA_Real *ref, int *nbRef, const char *tag )
{
   static TA_Real sigma[RVIR_CAP], up[RVIR_CAP], dn[RVIR_CAP];
   static TA_Real refUp[RVIR_CAP], refDn[RVIR_CAP];
   TA_Integer begSd, nbSd, begU, nbU, begD, nbD;
   TA_RetCode retCode;
   int k, sdStart;

   sdStart = startIdx - (period-1) - unst;

   retCode = TA_STDDEV( sdStart, nbBars-1, in, sdPeriod, 1.0, &begSd, &nbSd, sigma );
   if( retCode != TA_SUCCESS || begSd != sdStart )
   {
      printf( "RVIR differential [%s N=%d SD=%d u=%d]: TA_STDDEV rc=%d beg %d "
              "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
              begSd, sdStart );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* The only hand-written step: TA-Lib ships no vector sign gate. A compare
    * and a select add no arithmetic, so the reference stays free of novel
    * numerics. */
   for( k = 0; k < nbSd; k++ )
   {
      int bar = begSd + k;
      up[k] = in[bar] > in[bar-1] ? sigma[k] : 0.0;
      dn[k] = in[bar] < in[bar-1] ? sigma[k] : 0.0;
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, (unsigned int)unst );
   retCode = TA_RMA( 0, nbSd-1, up, period, &begU, &nbU, refUp );
   if( retCode == TA_SUCCESS )
      retCode = TA_RMA( 0, nbSd-1, dn, period, &begD, &nbD, refDn );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   if( retCode != TA_SUCCESS || begU != (period-1)+unst || nbD != nbU )
   {
      printf( "RVIR differential [%s N=%d SD=%d u=%d]: TA_RMA rc=%d beg %d "
              "expected %d\n", tag, period, sdPeriod, unst, (int)retCode,
              begU, (period-1)+unst );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( k = 0; k < nbU; k++ )
   {
      double total = refUp[k] + refDn[k];
      ref[k] = total == 0.0 ? 50.0 : 100.0*(refUp[k]/total);
   }
   *nbRef = nbU;

   return TA_TEST_PASS;
}

/* (1) TA_RVIR against a compose over shipped primitives, BIT-EXACT.
 *
 * Independent of TA_RVI: both legs are rebuilt from TA_STDDEV and TA_RMA, so a
 * shared defect in TA_RVI and TA_RVIR cannot hide here. The average is spelled
 * `0.5*(a+b)` because scaling by one half is exact -- `(a+b)/2` and
 * `0.5*a + 0.5*b` are the same double, which is why the body under test is free
 * to use any of them.
 */
static ErrorNumber test_rvir_differential( const TA_History *history )
{
   static TA_Real refHigh[RVIR_CAP], refLow[RVIR_CAP], out[RVIR_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, unst, anchor, startIdx, lookbackTotal;
   int sdPeriod, period, nbHigh, nbLow;
   ErrorNumber err;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( unst = 0; unst <= 4; unst += 2 )
   for( anchor = 0; anchor <= 40; anchor += 20 )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, (unsigned int)unst );
      lookbackTotal = TA_RVIR_Lookback( period, sdPeriod );
      startIdx = anchor < lookbackTotal ? lookbackTotal : anchor;
      if( startIdx > nbBars-1 )
         continue;

      err = rvir_reference_leg( history->high, nbBars, startIdx, period,
                                sdPeriod, unst, refHigh, &nbHigh, "high" );
      if( err != TA_TEST_PASS )
         goto done;
      err = rvir_reference_leg( history->low, nbBars, startIdx, period,
                                sdPeriod, unst, refLow, &nbLow, "low" );
      if( err != TA_TEST_PASS )
         goto done;

      retCode = TA_RVIR( startIdx, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != startIdx
          || nbElement != nbHigh || nbHigh != nbLow )
      {
         printf( "RVIR differential Fail [N=%d SD=%d u=%d s=%d]: rc=%d (%d,%d) "
                 "expected (%d,%d)\n", period, sdPeriod, unst, startIdx,
                 (int)retCode, begIdx, nbElement, startIdx, nbHigh );
         err = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }

      for( k = 0; k < nbElement; k++ )
      {
         double want = 0.5 * ( refHigh[k] + refLow[k] );

         g_rvirDiffCmp++;
         if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR differential Fail [N=%d SD=%d u=%d s=%d] at bar %d: "
                    "got %.17g expected %.17g -- the compose over TA_STDDEV and "
                    "TA_RMA is bit-exact, so any difference is a change in the "
                    "arithmetic or its order\n", period, sdPeriod, unst,
                    startIdx, startIdx+k, out[k], want );
            err = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }
   }

   err = TA_TEST_PASS;

done:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   return err;
}

/* (2) The published contract: this function is the average of TA_RVI over the
 * highs and TA_RVI over the lows, bit for bit.
 *
 * STRUCTURALLY TRUE against the current body, which calls TA_RVI twice, so it
 * cannot fail for an arithmetic reason. It is here for the rewrite that fuses
 * the two legs into one pass -- the shape the proposal asked for, which the
 * streaming analyzer rejects today because it carries two window cursors. A
 * fused body would have to reproduce this equality, and nothing else in this
 * file would notice if it drifted.
 */
static ErrorNumber test_rvir_composite( const TA_History *history )
{
   static TA_Real legHigh[RVIR_CAP], legLow[RVIR_CAP], out[RVIR_CAP];
   TA_Integer begIdx, nbElement, begH, nbH, begL, nbL;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, sdPeriod, period;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVIR composite Fail [N=%d SD=%d]: rc=%d\n",
                 period, sdPeriod, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_RVI( begIdx, nbBars-1, history->high, period, sdPeriod,
                        &begH, &nbH, legHigh );
      if( retCode == TA_SUCCESS )
         retCode = TA_RVI( begIdx, nbBars-1, history->low, period, sdPeriod,
                           &begL, &nbL, legLow );
      if( retCode != TA_SUCCESS || begH != begIdx || begL != begIdx
          || nbH != nbElement || nbL != nbElement )
      {
         printf( "RVIR composite Fail [N=%d SD=%d]: legs rc=%d (%d,%d)/(%d,%d) "
                 "against (%d,%d)\n", period, sdPeriod, (int)retCode,
                 begH, nbH, begL, nbL, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < nbElement; k++ )
      {
         double want = 0.5 * ( legHigh[k] + legLow[k] );

         g_rvirCompCmp++;
         if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR composite Fail [N=%d SD=%d] at bar %d: got %.17g "
                    "expected %.17g\n", period, sdPeriod, begIdx+k,
                    out[k], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) A series whose high equals its low at every bar: the two legs are the
 * same computation, so this function must return exactly TA_RVI of it.
 *
 * This is the leg that sees the warm-up. It compares outBegIdx as well as the
 * values, and it does so against a function whose lookback is established, so a
 * lookback that drifts by one bar is caught here and nowhere else -- leg 1
 * anchors its reference at whatever outBegIdx this function reports and would
 * follow the drift instead of failing on it.
 */
static ErrorNumber test_rvir_degenerate( const TA_History *history )
{
   static TA_Real out[RVIR_CAP], want[RVIR_CAP];
   TA_Integer begIdx, nbElement, begWant, nbWant;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int a, b, k, unst, sdPeriod, period;
   ErrorNumber err;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( unst = 0; unst <= 4; unst += 2 )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];

      TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, (unsigned int)unst );

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->high,
                         period, sdPeriod, &begIdx, &nbElement, out );
      if( retCode == TA_SUCCESS )
         retCode = TA_RVI( 0, nbBars-1, history->high, period, sdPeriod,
                           &begWant, &nbWant, want );
      if( retCode != TA_SUCCESS || begIdx != begWant || nbElement != nbWant )
      {
         printf( "RVIR degenerate Fail [N=%d SD=%d u=%d]: rc=%d (%d,%d) against "
                 "TA_RVI (%d,%d) -- with high == low the two legs are the same "
                 "computation, so the warm-up must match too\n",
                 period, sdPeriod, unst, (int)retCode, begIdx, nbElement,
                 begWant, nbWant );
         err = TA_TESTUTIL_TFRR_BAD_BEGIDX;
         goto done;
      }

      for( k = 0; k < nbElement; k++ )
      {
         g_rvirDegenCmp++;
         if( memcmp( &want[k], &out[k], sizeof(double) ) != 0 )
         {
            printf( "RVIR degenerate Fail [N=%d SD=%d u=%d] at bar %d: %.17g "
                    "against TA_RVI's %.17g\n", period, sdPeriod, unst,
                    begIdx+k, out[k], want[k] );
            err = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            goto done;
         }
      }
   }

   err = TA_TEST_PASS;

done:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RVI, 0 );
   return err;
}

/* (4) The tie rule, made observable.
 *
 * At optInTimePeriod == 1 the smoothing has no memory, so each leg is decided
 * by the sign of its own bar alone and takes one of {0, 50, 100}. The average
 * of two such legs takes {0, 25, 50, 75, 100}, and the quarter-points are the
 * bars where the high and the low disagree. A tie routed to the down bucket
 * instead of to neither would erase them: both legs would be in {0, 100} and
 * the set would collapse to {0, 50, 100}.
 */
static ErrorNumber test_rvir_tie( const TA_History *history )
{
   static TA_Real out[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int i, nbQuarter = 0, nbHalf = 0, nbEnd = 0;

   if( nbBars != 252 )
      return TA_TEST_PASS;

   retCode = TA_RVIR( 0, nbBars-1, history->high, history->low, 1, 10,
                      &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 9 || nbElement != 243 )
   {
      printf( "RVIR tie Fail: rc=%d (%d,%d) expected (9,243)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbElement; i++ )
   {
      g_rvirTieCmp++;
      if( out[i] == 25.0 || out[i] == 75.0 )
         nbQuarter++;
      else if( out[i] == 50.0 )
         nbHalf++;
      else if( out[i] == 0.0 || out[i] == 100.0 )
         nbEnd++;
      else
      {
         printf( "RVIR tie Fail at bar %d: %.17g is not one of "
                 "{0, 25, 50, 75, 100}. With no smoothing memory each leg is "
                 "decided by its own bar's sign, so any other value means a leg "
                 "carries state it should not\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( nbQuarter == 0 )
   {
      printf( "RVIR tie Fail: no bar landed on 25 or 75, so this leg saw no bar "
              "where the high and the low disagree and would pass against a "
              "single-leg implementation\n" );
      return TA_RVIR_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (5) Flat input, the two shape edges, and the scratch buffer's extent.
 *
 * A flat window has zero deviation in both legs, so each resolves to 50 through
 * its own zero-total guard and the average is 50 -- without the guard every
 * value below is NaN, and NaN fails the equality.
 *
 * The single-bar range is the one that sizes the scratch buffer to exactly one
 * element. Allocating one short is invisible to every other leg and to a plain
 * run; it was confirmed to abort this leg under AddressSanitizer.
 */
static ErrorNumber test_rvir_edges( void )
{
   static TA_Real high[300], low[300], out[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int period, sdPeriod, i, lookbackTotal;

   for( i = 0; i < 300; i++ )
   {
      high[i] = 42.0;
      low[i]  = 42.0;
   }

   for( sdPeriod = 2; sdPeriod <= 20; sdPeriod++ )
   for( period = 1; period <= 20; period++ )
   {
      lookbackTotal = TA_RVIR_Lookback( period, sdPeriod );
      retCode = TA_RVIR( 0, 299, high, low, period, sdPeriod,
                         &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != lookbackTotal
          || nbElement != 300 - lookbackTotal )
      {
         printf( "RVIR flat Fail [N=%d SD=%d]: rc=%d (%d,%d)\n",
                 period, sdPeriod, (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rvirEdgeCmp++;
         if( isnan( out[i] ) || out[i] != 50.0 )
         {
            printf( "RVIR flat Fail [N=%d SD=%d] out %d: %.17g, expected exactly "
                    "50.0 (NaN => a leg's zero-total guard is missing)\n",
                    period, sdPeriod, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* One bar of output, requested at the first index that has one: the scratch
    * buffer is exactly one element wide here. */
   lookbackTotal = TA_RVIR_Lookback( 14, 10 );
   retCode = TA_RVIR( lookbackTotal, lookbackTotal, high, low, 14, 10,
                      &begIdx, &nbElement, out );
   g_rvirEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != lookbackTotal || nbElement != 1 )
   {
      printf( "RVIR edge Fail: single-bar range gave rc=%d (%d,%d), expected "
              "(%d,1)\n", (int)retCode, begIdx, nbElement, lookbackTotal );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* A range shorter than the lookback produces nothing, successfully, and
    * allocates nothing on the way. */
   retCode = TA_RVIR( 0, lookbackTotal-1, high, low, 14, 10,
                      &begIdx, &nbElement, out );
   g_rvirEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "RVIR edge Fail: short range gave rc=%d (%d,%d), expected (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   return TA_TEST_PASS;
}

/* (6) In-place aliasing, outReal over each input in turn, bitwise.
 *
 * Two cases rather than one: the two inputs are consumed by different calls at
 * different points, so aliasing one is not evidence about the other.
 */
static ErrorNumber test_rvir_aliasing( const TA_History *history )
{
   static TA_Real clean[RVIR_CAP], alias[RVIR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int nbBars = (int)history->nbBars;
   int a, b, i, which, sdPeriod, period;
   const TA_Real *aliased;

   for( a = 0; a < NB_RVIR_SD; a++ )
   for( b = 0; b < NB_RVIR_TIME; b++ )
   for( which = 0; which < 2; which++ )
   {
      sdPeriod = rvirSdPeriods[a];
      period   = rvirTimePeriods[b];
      aliased  = which == 0 ? history->high : history->low;

      retCode = TA_RVIR( 0, nbBars-1, history->high, history->low,
                         period, sdPeriod, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "RVIR alias Fail [N=%d SD=%d %s]: rc=%d\n", period, sdPeriod,
                 which == 0 ? "high" : "low", (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = aliased[i];
      if( which == 0 )
         retCode = TA_RVIR( 0, nbBars-1, alias, history->low, period, sdPeriod,
                            &begIdx2, &nbElement2, alias );
      else
         retCode = TA_RVIR( 0, nbBars-1, history->high, alias, period, sdPeriod,
                            &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "RVIR alias Fail [N=%d SD=%d %s]: rc=%d shape (%d,%d) vs "
                 "(%d,%d)\n", period, sdPeriod, which == 0 ? "high" : "low",
                 (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_rvirAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "RVIR alias Fail [N=%d SD=%d %s] out %d: separate %.17g, "
                    "in-place %.17g -- a store landed under a read the same bar "
                    "still needed\n", period, sdPeriod,
                    which == 0 ? "high" : "low", i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_CONVERGING for TA_RVI's
 * reason, inherited through both legs: the smoothed legs are IIR recurrences
 * seeded at startIdx - lookback, so an earlier start moves a value by a
 * residual the unstable period bounds. */
typedef struct { int period; int sdPeriod; const TA_Real *high; const TA_Real *low; } RvirRangeParam;

static TA_RetCode rvirRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   RvirRangeParam *p = (RvirRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_RVIR_Lookback( p->period, p->sdPeriod );
   return TA_RVIR( startIdx, endIdx, p->high, p->low, p->period, p->sdPeriod,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_rvir_range( const TA_History *history )
{
   RvirRangeParam param;

   param.period   = 14;
   param.sdPeriod = 10;
   param.high     = history->high;
   param.low      = history->low;

   return doRangeTestEx( rvirRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_RVI,
                         (void *)&param, 1, 0 );
}
