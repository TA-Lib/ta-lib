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
 *  090526 MF,CC  First version (issue #371).
 */

/* Description:
 *
 *   Test TA_FRACTAL (Williams Fractal / bounded pivot detector).
 *
 *   The outputs are INTEGER flags, so every comparison below is exact
 *   equality; there is no tolerance anywhere in this file and nothing to
 *   justify. What has to be justified instead is the FIRING SET, and the
 *   goldens are frozen confirmation-bar index lists produced by executing two
 *   unrelated libraries.
 *
 *   Legs:
 *     1. EXTERNAL ORACLES on the 252-bar reference corpus, eight (L,R) pairs.
 *        See fractalCorpus below for the provenance and for which pairs are
 *        double-armed.
 *     2. EXTERNAL ORACLES on a 44-bar synthetic built to be tie-heavy. The
 *        corpus is thin exactly where this function's contested decisions
 *        live: a plateau that the strict rule must reject, an outside bar
 *        that fires BOTH flags, and an (L,R) pair whose mirror gives a
 *        different answer.
 *     3. DEGENERATE inputs -- all-flat and a monotone ramp -- must emit zero
 *        everywhere. All-flat is the case that separates the strict rule this
 *        function implements from a non-strict one, and both oracles were run
 *        on these two series and returned nothing.
 *     4. SHAPE: the lookback identity, the exactly-one-output range, and the
 *        below-lookback range that succeeds with nothing.
 *     5. The startIdx/endIdx range sweep, in the EXACT class.
 *
 *   There is no differential leg: nothing in the library decomposes into a
 *   CENTRED window test, and no composition over shipped primitives would be
 *   independent of this file's own arithmetic anyway.
 *
 *   Cross-language value coverage comes from server_verify in legs 1-2 plus
 *   the --xlang-hash sweep; the frozen ta_ref_serve predates this function, so
 *   the --codegen value comparison cannot run for it (same situation as VHF).
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"

/**** External variables declarations. ****/
extern TA_Real TA_SREF_high_daily_ref_0_PRIV[];
extern TA_Real TA_SREF_low_daily_ref_0_PRIV[];

/**** Local declarations. ****/
#define FRACTAL_CAP 512
#define FRACTAL_SYN_NB 44
#define FRACTAL_DEG_NB 60

/* Which oracle arms produced a row. trading-signals is SYMMETRIC ONLY --
 * SwingHigh/SwingLow take a single `lookback` and require 2*lookback+1 inputs
 * -- so every L != R row is single-armed off ta4j, and saying so here stops
 * the two named libraries from implying two arms on rows only one can reach. */
#define FX_BOTH "ta4j 0.22.6 + trading-signals 8.3.0"
#define FX_TA4J "ta4j 0.22.6 (only arm reaching L != R)"

typedef struct
{
   int         optInLeftBars;
   int         optInRightBars;
   const int  *swingHigh;      /* ABSOLUTE confirmation-bar indices */
   int         nbSwingHigh;
   const int  *swingLow;
   int         nbSwingLow;
   const char *source;
} FractalGolden;

/* ------------------------------------------------------------------------ *
 * Leg 1 goldens: the 252-bar reference series (TA_SREF_{high,low}_daily_ref_0_PRIV).
 *
 * Captured 2026-09-05 by running, on those exact doubles:
 *   1. ta4j 0.22.6 (Java, MIT, Maven Central org.ta4j:ta4j-core) --
 *      FractalHighIndicator(series, L, R) / FractalLowIndicator(series, L, R),
 *      reading the Boolean at every bar. ta4j defines that Boolean as "the bar
 *      (index - followingBars) is a confirmed fractal", which is the same
 *      confirmation-bar anchor TA_FRACTAL uses, and its getCountOfUnstableBars()
 *      returned L+R at every pair -- an independent confirmation of the lookback.
 *      It is the only arm whose constructor takes the two arms separately.
 *   2. trading-signals 8.3.0 (TypeScript, MIT) -- SwingHigh/SwingLow, whose
 *      window is 2*lookback+1 and whose pivot is rejected on `>= pivot` in
 *      either direction. Symmetric only, so it arms the first four rows.
 *
 * The two agree on EVERY symmetric row, index list for index list, and the
 * price trading-signals emits at each firing bar equals high/low[bar - L] --
 * the pivot the flag is about, not the bar it is reported on.
 *
 * A list is the whole answer, not a sample: every output bar NOT named here is
 * required to be 0, which is what makes a function stuck at a constant fail.
 * ------------------------------------------------------------------------ */
static const int fxH_1_1[] =
   { 3, 7, 12, 17, 20, 26, 28, 30, 35, 40, 45, 47, 50, 53, 61, 69, 79,
     84, 91, 94, 100, 107, 109, 115, 118, 121, 124, 127, 132, 136, 143,
     145, 150, 158, 161, 164, 168, 174, 177, 183, 185, 191, 195, 200,
     202, 206, 209, 218, 220, 226, 228, 237, 239, 243, 247, 249 };
static const int fxL_1_1[] =
   { 5, 9, 15, 18, 26, 31, 39, 41, 49, 55, 62, 65, 74, 85, 90, 93, 99,
     104, 108, 111, 120, 122, 128, 134, 137, 140, 149, 152, 160, 163,
     166, 169, 179, 184, 189, 194, 199, 203, 205, 207, 213, 219, 222,
     227, 230, 239, 241, 244, 246, 248 };
static const int fxH_2_2[] =
   { 4, 13, 18, 21, 29, 36, 48, 51, 80, 85, 92, 101, 108, 119, 122, 133,
     137, 144, 151, 159, 165, 175, 192, 203, 207, 210, 219, 227, 238,
     244, 250 };
static const int fxL_2_2[] =
   { 10, 16, 19, 27, 32, 42, 50, 56, 63, 75, 91, 94, 100, 105, 112, 121,
     141, 150, 153, 164, 180, 185, 190, 200, 204, 214, 223, 228, 231, 242 };
static const int fxH_3_3[] =
   { 14, 19, 30, 37, 49, 81, 93, 102, 109, 120, 134, 138, 145, 152, 160,
     176, 193, 204, 211, 220, 228, 239, 245, 251 };
static const int fxL_3_3[] =
   { 11, 17, 28, 43, 57, 76, 101, 106, 113, 122, 142, 151, 165, 181, 191,
     201, 205, 215, 224, 243 };
static const int fxH_5_5[] =
   { 16, 32, 39, 51, 95, 111, 122, 136, 154, 162, 178, 206, 213, 230, 241 };
static const int fxL_5_5[] =
   { 19, 30, 45, 59, 78, 108, 144, 153, 167, 207, 217, 226, 245 };
static const int fxH_2_5[] =
   { 7, 16, 21, 24, 32, 39, 51, 54, 95, 104, 111, 122, 136, 140, 147,
     154, 162, 178, 195, 206, 213, 230, 241, 247 };
static const int fxL_2_5[] =
   { 19, 30, 35, 45, 59, 66, 78, 94, 108, 115, 124, 144, 153, 156, 167,
     207, 217, 226, 231, 234, 245 };
static const int fxH_5_2[] =
   { 13, 29, 36, 48, 80, 85, 92, 108, 119, 133, 151, 159, 165, 175, 203,
     210, 219, 227, 238, 250 };
static const int fxL_5_2[] =
   { 10, 16, 27, 42, 56, 75, 100, 105, 141, 150, 164, 180, 185, 190, 200,
     204, 214, 223, 242 };
static const int fxH_1_10[] =
   { 21, 26, 29, 56, 59, 100, 103, 141, 145, 152, 167, 183, 186, 192,
     194, 200, 204, 211, 218, 246, 248 };
static const int fxL_1_10[] =
   { 35, 50, 64, 83, 94, 99, 113, 120, 129, 131, 158, 161, 172, 175, 212,
     222, 231, 236, 239, 250 };
static const int fxH_10_1[] =
   { 12, 35, 45, 47, 61, 79, 84, 91, 107, 115, 118, 124, 127, 132, 158,
     174, 226, 237 };
static const int fxL_10_1[] =
   { 15, 26, 39, 41, 55, 74, 104, 137, 140, 149, 179, 184, 189, 194, 199,
     203 };

/* (2,5) and (5,2) are each other's mirror and their lists differ: an anchor
 * that swapped the two arms would be invisible at the 2/2 default. */
static const FractalGolden fractalCorpus[] =
{
   {  1,  1, fxH_1_1,  56, fxL_1_1,  50, FX_BOTH },
   {  2,  2, fxH_2_2,  31, fxL_2_2,  30, FX_BOTH },
   {  3,  3, fxH_3_3,  24, fxL_3_3,  20, FX_BOTH },
   {  5,  5, fxH_5_5,  15, fxL_5_5,  13, FX_BOTH },
   {  2,  5, fxH_2_5,  24, fxL_2_5,  21, FX_TA4J },
   {  5,  2, fxH_5_2,  20, fxL_5_2,  19, FX_TA4J },
   {  1, 10, fxH_1_10, 21, fxL_1_10, 20, FX_TA4J },
   { 10,  1, fxH_10_1, 18, fxL_10_1, 16, FX_TA4J },
};
#define NB_FRACTAL_CORPUS ((int)(sizeof(fractalCorpus)/sizeof(FractalGolden)))

/* ------------------------------------------------------------------------ *
 * Leg 2 goldens: a synthetic series, same two oracles, captured the same day.
 *
 * The corpus cannot reach three of this function's decisions. Every value here
 * is an exact binary fraction on a 0.5 grid, so ties are exact ties:
 *   - eight flat bars, then a pattern that repeats highs two bars apart, so a
 *     candidate tied with a bar on its LEFT appears. Both oracles reject it;
 *     TradingView's Pine runtime would not, which is the divergence fractal.md
 *     documents.
 *   - one outside bar (index 20, high 56.0 / low 34.0) that is simultaneously
 *     a swing high and a swing low. That is what justifies two flags over one
 *     signed value, and the 252-bar corpus contains no such bar at 2/2 or at
 *     any wider pair.
 *   - a trailing ramp, where no bar dominates its own right arm.
 * ------------------------------------------------------------------------ */
static const TA_Real fractalSynHigh[FRACTAL_SYN_NB] =
{
   50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0,
   51.0, 52.5, 51.0, 53.0, 51.5, 50.0, 51.5, 50.0,
   52.0, 50.5, 52.0, 50.5, 56.0, 51.0, 52.5, 51.0,
   53.0, 51.5, 50.0, 51.5, 50.0, 52.0, 50.5, 52.0,
   48.0, 48.5, 49.0, 49.5, 50.0, 50.5, 51.0, 51.5,
   52.0, 52.5, 53.0, 53.5
};
static const TA_Real fractalSynLow[FRACTAL_SYN_NB] =
{
   40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0, 40.0,
   38.5, 40.0, 38.5, 40.0, 39.0, 37.5, 39.0, 38.0,
   39.5, 38.0, 39.5, 38.5, 34.0, 38.5, 40.0, 39.0,
   37.5, 39.0, 38.0, 39.5, 38.0, 39.5, 38.5, 40.0,
   42.0, 41.5, 41.0, 40.5, 40.0, 39.5, 39.0, 38.5,
   38.0, 37.5, 37.0, 36.5
};

static const int fsH_1_1[] = { 10, 12, 15, 17, 19, 21, 23, 25, 28, 30, 32 };
static const int fsL_1_1[] = { 9, 11, 14, 16, 18, 21, 25, 27, 29, 31 };
static const int fsH_2_2[] = { 13, 22, 26 };
static const int fsL_2_2[] = { 15, 22, 26 };
static const int fsH_2_1[] = { 10, 12, 17, 21, 25, 30 };
static const int fsL_2_1[] = { 9, 14, 21, 25 };
static const int fsH_1_2[] = { 13, 22, 26, 33 };
static const int fsL_1_2[] = { 12, 15, 19, 22, 26, 30, 32 };

static const FractalGolden fractalSynthetic[] =
{
   { 1, 1, fsH_1_1, 11, fsL_1_1, 10, FX_BOTH },
   { 2, 2, fsH_2_2,  3, fsL_2_2,  3, FX_BOTH },
   { 2, 1, fsH_2_1,  6, fsL_2_1,  4, FX_TA4J },
   { 1, 2, fsH_1_2,  4, fsL_1_2,  7, FX_TA4J },
};
#define NB_FRACTAL_SYN ((int)(sizeof(fractalSynthetic)/sizeof(FractalGolden)))

/* The bars where both flags fire at once, read off the lists above. Asserted
 * by name because it is the property the two-output shape exists for, and it
 * would otherwise be one silent element of a 3916-comparison sweep. */
typedef struct { int optInLeftBars; int optInRightBars; int bar; } FractalBothFire;
static const FractalBothFire fractalBothFire[] =
{
   { 1, 1, 21 }, { 1, 1, 25 }, { 2, 2, 22 }, { 2, 2, 26 },
};
#define NB_FRACTAL_BOTH ((int)(sizeof(fractalBothFire)/sizeof(FractalBothFire)))

/* Leg 3. Both oracles were run on these two shapes at 1/1, 2/2, 3/3 and 5/5
 * (ta4j also at 2/5 and 5/2) and emitted NOTHING on either. */
static const int fractalDegPairs[][2] =
   { {1,1}, {2,2}, {3,3}, {5,5}, {2,5}, {5,2} };
#define NB_FRACTAL_DEG_PAIRS ((int)(sizeof(fractalDegPairs)/sizeof(fractalDegPairs[0])))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_fractalCorpusCmp;
static int g_fractalSynCmp;
static int g_fractalDegCmp;
static int g_fractalBothCmp;
static int g_fractalShapeCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_fractal_goldens( const char *tag,
                                         const TA_Real *inHigh,
                                         const TA_Real *inLow,
                                         int nbBars,
                                         const FractalGolden *rows,
                                         int nbRows,
                                         int *counter );
static ErrorNumber test_fractal_both_fire( void );
static ErrorNumber test_fractal_degenerate( void );
static ErrorNumber test_fractal_shape( void );
static ErrorNumber test_fractal_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_fractal( TA_History *history )
{
   ErrorNumber err;

   /* FRACTAL has no unstable period; a leftover global setting must not reach
    * it, and the range sweep below asserts the same thing from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_fractalCorpusCmp = g_fractalSynCmp = g_fractalDegCmp = 0;
   g_fractalBothCmp = g_fractalShapeCmp = 0;

   if( history->nbBars == 252 )
   {
      err = test_fractal_goldens( "TA_SREF 252-bar corpus",
                                  TA_SREF_high_daily_ref_0_PRIV,
                                  TA_SREF_low_daily_ref_0_PRIV,
                                  252, fractalCorpus, NB_FRACTAL_CORPUS,
                                  &g_fractalCorpusCmp );
      if( err != TA_TEST_PASS )
         return err;
   }
   else
   {
      printf( "FRACTAL corpus skip: goldens were captured on the 252-bar "
              "series, got %d\n", (int)history->nbBars );
   }

   err = test_fractal_goldens( "tie-heavy synthetic", fractalSynHigh,
                               fractalSynLow, FRACTAL_SYN_NB,
                               fractalSynthetic, NB_FRACTAL_SYN,
                               &g_fractalSynCmp );
   if( err != TA_TEST_PASS )
      return err;

   err = test_fractal_both_fire();
   if( err != TA_TEST_PASS )
      return err;

   err = test_fractal_degenerate();
   if( err != TA_TEST_PASS )
      return err;

   err = test_fractal_shape();
   if( err != TA_TEST_PASS )
      return err;

   err = test_fractal_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: every leg above is deterministic on
    * the shipped corpus. */
   if( history->nbBars == 252
       && ( g_fractalCorpusCmp != 3916 || g_fractalSynCmp != 328
            || g_fractalDegCmp != 1296 || g_fractalBothCmp != 8
            || g_fractalShapeCmp != 432 ) )
   {
      printf( "FRACTAL Fail: coverage counters (corpus %d, synthetic %d, "
              "degenerate %d, both-fire %d, shape %d) are not what this file "
              "was written with (3916, 328, 1296, 8, 432)\n",
              g_fractalCorpusCmp, g_fractalSynCmp, g_fractalDegCmp,
              g_fractalBothCmp, g_fractalShapeCmp );
      return TA_FRACTAL_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) and (2): a frozen index list replayed as a full expected output.
 *
 * The list is expanded to one expected flag per output bar, so a bar the
 * oracles did NOT name is asserted to be 0. Integer outputs: exact equality,
 * no tolerance. */
static ErrorNumber test_fractal_goldens( const char *tag,
                                         const TA_Real *inHigh,
                                         const TA_Real *inLow,
                                         int nbBars,
                                         const FractalGolden *rows,
                                         int nbRows,
                                         int *counter )
{
   static TA_Integer outHigh[FRACTAL_CAP], outLow[FRACTAL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int r, i, k, lookback;

   for( r = 0; r < nbRows; r++ )
   {
      const FractalGolden *g = &rows[r];

      lookback = TA_FRACTAL_Lookback( g->optInLeftBars, g->optInRightBars );
      if( lookback != g->optInLeftBars + g->optInRightBars )
      {
         printf( "FRACTAL Fail [%s L=%d R=%d]: lookback %d, expected %d\n",
                 tag, g->optInLeftBars, g->optInRightBars, lookback,
                 g->optInLeftBars + g->optInRightBars );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      retCode = TA_FRACTAL( 0, nbBars-1, inHigh, inLow,
                            g->optInLeftBars, g->optInRightBars,
                            &begIdx, &nbElement, outHigh, outLow );
      if( retCode != TA_SUCCESS || begIdx != lookback
          || nbElement != nbBars - lookback )
      {
         printf( "FRACTAL Fail [%s L=%d R=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 tag, g->optInLeftBars, g->optInRightBars, (int)retCode,
                 begIdx, nbElement, lookback, nbBars - lookback );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      if( server_verify_active() )
      {
         double optIn[2];
         ErrorNumber e;
         int cmpBefore = server_verify_comparisons();

         optIn[0] = (double)g->optInLeftBars;
         optIn[1] = (double)g->optInRightBars;
         e = server_verify( "FRACTAL", 0, nbBars-1, nbBars,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ inHigh, inLow, NULL },
                            optIn, 2,
                            NULL,
                            (const TA_Integer*[]){ outHigh, outLow, NULL } );
         if( e != TA_TEST_PASS )
            return e;
         /* "No failure reported" and "nothing was compared" are the same
          * observation without this. */
         if( server_verify_comparisons() == cmpBefore )
         {
            printf( "FRACTAL [%s L=%d R=%d]: compared no server despite live "
                    "pipes\n", tag, g->optInLeftBars, g->optInRightBars );
            return TA_FRACTAL_VACUOUS;
         }
      }

      /* Both lists must land inside the output, or the expansion below would
       * quietly agree with a shorter answer than the oracle gave. */
      for( k = 0; k < g->nbSwingHigh; k++ )
      {
         if( g->swingHigh[k] < begIdx || g->swingHigh[k] >= begIdx + nbElement )
         {
            printf( "FRACTAL Fail [%s L=%d R=%d]: golden swing-high bar %d is "
                    "outside the output [%d..%d]\n", tag, g->optInLeftBars,
                    g->optInRightBars, g->swingHigh[k], begIdx,
                    begIdx + nbElement - 1 );
            return TA_FRACTAL_VACUOUS;
         }
      }
      for( k = 0; k < g->nbSwingLow; k++ )
      {
         if( g->swingLow[k] < begIdx || g->swingLow[k] >= begIdx + nbElement )
         {
            printf( "FRACTAL Fail [%s L=%d R=%d]: golden swing-low bar %d is "
                    "outside the output [%d..%d]\n", tag, g->optInLeftBars,
                    g->optInRightBars, g->swingLow[k], begIdx,
                    begIdx + nbElement - 1 );
            return TA_FRACTAL_VACUOUS;
         }
      }

      for( i = 0, k = 0; i < nbElement; i++ )
      {
         int want = 0;
         if( k < g->nbSwingHigh && g->swingHigh[k] == begIdx + i )
         {
            want = 100;
            k++;
         }
         (*counter)++;
         if( outHigh[i] != want )
         {
            printf( "FRACTAL Fail [%s L=%d R=%d] swing high at bar %d: got %d, "
                    "expected %d (%s)\n", tag, g->optInLeftBars,
                    g->optInRightBars, begIdx + i, outHigh[i], want, g->source );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      for( i = 0, k = 0; i < nbElement; i++ )
      {
         int want = 0;
         if( k < g->nbSwingLow && g->swingLow[k] == begIdx + i )
         {
            want = 100;
            k++;
         }
         (*counter)++;
         if( outLow[i] != want )
         {
            printf( "FRACTAL Fail [%s L=%d R=%d] swing low at bar %d: got %d, "
                    "expected %d (%s)\n", tag, g->optInLeftBars,
                    g->optInRightBars, begIdx + i, outLow[i], want, g->source );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* The outside bar: one input bar carrying both flags at once. A single signed
 * output could not express it. */
static ErrorNumber test_fractal_both_fire( void )
{
   static TA_Integer outHigh[FRACTAL_CAP], outLow[FRACTAL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int b;

   for( b = 0; b < NB_FRACTAL_BOTH; b++ )
   {
      const FractalBothFire *f = &fractalBothFire[b];
      int at;

      retCode = TA_FRACTAL( 0, FRACTAL_SYN_NB-1, fractalSynHigh, fractalSynLow,
                            f->optInLeftBars, f->optInRightBars,
                            &begIdx, &nbElement, outHigh, outLow );
      if( retCode != TA_SUCCESS || f->bar < begIdx
          || f->bar - begIdx >= nbElement )
      {
         printf( "FRACTAL both-fire Fail [L=%d R=%d]: rc=%d, bar %d outside "
                 "(%d,%d)\n", f->optInLeftBars, f->optInRightBars,
                 (int)retCode, f->bar, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      at = f->bar - begIdx;
      g_fractalBothCmp += 2;
      if( outHigh[at] != 100 || outLow[at] != 100 )
      {
         printf( "FRACTAL both-fire Fail [L=%d R=%d] at bar %d: (%d,%d), "
                 "expected (100,100) -- an outside bar is a swing high and a "
                 "swing low at once\n", f->optInLeftBars, f->optInRightBars,
                 f->bar, outHigh[at], outLow[at] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) Degenerate inputs.
 *
 * All-flat is the case that separates this function's strict rule from a
 * non-strict one: with equality admitted on either arm, every bar of a flat
 * series is a pivot. Both oracles were run on both series and emitted nothing.
 */
static ErrorNumber test_fractal_degenerate( void )
{
   static TA_Real high[FRACTAL_DEG_NB], low[FRACTAL_DEG_NB];
   static TA_Integer outHigh[FRACTAL_CAP], outLow[FRACTAL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int shape, p, i;

   for( shape = 0; shape < 2; shape++ )
   {
      const char *tag = shape == 0 ? "all-flat" : "monotone ramp";

      for( i = 0; i < FRACTAL_DEG_NB; i++ )
      {
         high[i] = shape == 0 ? 50.0 : 48.0 + 0.25 * (double)i;
         low[i]  = high[i] - 2.0;
      }

      for( p = 0; p < NB_FRACTAL_DEG_PAIRS; p++ )
      {
         int L = fractalDegPairs[p][0];
         int R = fractalDegPairs[p][1];

         retCode = TA_FRACTAL( 0, FRACTAL_DEG_NB-1, high, low, L, R,
                               &begIdx, &nbElement, outHigh, outLow );
         if( retCode != TA_SUCCESS || begIdx != L + R
             || nbElement != FRACTAL_DEG_NB - (L + R) )
         {
            printf( "FRACTAL %s Fail [L=%d R=%d]: rc=%d (%d,%d)\n",
                    tag, L, R, (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            g_fractalDegCmp += 2;
            if( outHigh[i] != 0 || outLow[i] != 0 )
            {
               printf( "FRACTAL %s Fail [L=%d R=%d] at bar %d: (%d,%d), "
                       "expected (0,0) -- a bar tied with, or below, any bar "
                       "of its window is not a pivot\n",
                       tag, L, R, begIdx + i, outHigh[i], outLow[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Shape: the lookback identity over a wide sweep, the range that produces
 * exactly one output, and the range shorter than the lookback. */
static ErrorNumber test_fractal_shape( void )
{
   static TA_Real high[FRACTAL_DEG_NB], low[FRACTAL_DEG_NB];
   static TA_Integer outHigh[FRACTAL_CAP], outLow[FRACTAL_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int L, R, i, lookback;

   for( i = 0; i < FRACTAL_DEG_NB; i++ )
   {
      high[i] = 50.0 + (double)( ( i * 7 ) % 13 ) * 0.5;
      low[i]  = 40.0 - (double)( ( i * 5 ) % 11 ) * 0.5;
   }

   for( L = 1; L <= 12; L++ )
   {
      for( R = 1; R <= 12; R++ )
      {
         lookback = TA_FRACTAL_Lookback( L, R );
         g_fractalShapeCmp++;
         if( lookback != L + R )
         {
            printf( "FRACTAL lookback Fail [L=%d R=%d]: %d, expected %d\n",
                    L, R, lookback, L + R );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* Exactly enough bars for one verdict. */
         outHigh[0] = outLow[0] = -1;
         retCode = TA_FRACTAL( 0, lookback, high, low, L, R,
                               &begIdx, &nbElement, outHigh, outLow );
         g_fractalShapeCmp++;
         if( retCode != TA_SUCCESS || begIdx != lookback || nbElement != 1
             || outHigh[0] < 0 || outLow[0] < 0 )
         {
            printf( "FRACTAL one-output Fail [L=%d R=%d]: rc=%d (%d,%d) "
                    "flags (%d,%d)\n", L, R, (int)retCode, begIdx, nbElement,
                    outHigh[0], outLow[0] );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* One bar short of the lookback: success, nothing produced. */
         begIdx = -1; nbElement = -1;
         retCode = TA_FRACTAL( 0, lookback-1, high, low, L, R,
                               &begIdx, &nbElement, outHigh, outLow );
         g_fractalShapeCmp++;
         if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
         {
            printf( "FRACTAL short-range Fail [L=%d R=%d]: rc=%d (%d,%d), "
                    "expected SUCCESS (0,0)\n", L, R, (int)retCode, begIdx,
                    nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) The startIdx/endIdx range sweep. TA_STABLE_EXACT: every bar is decided
 * from its own window with no carried state, so no range may move a flag. No
 * unstable-period id, matching its abstract metadata, which doRangeTestEx
 * cross-checks against the stability class. */
typedef struct
{
   int optInLeftBars;
   int optInRightBars;
   const TA_Real *high;
   const TA_Real *low;
} FractalRangeParam;

static TA_RetCode fractalRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                            TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                            TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                            TA_Integer *lookback, void *opaqueData,
                                            unsigned int outputNb, unsigned int *isOutputInteger )
{
   FractalRangeParam *p = (FractalRangeParam *)opaqueData;
   TA_RetCode retCode;
   TA_Integer *out1;
   TA_Integer *out2;
   TA_Integer *spare;

   (void)outputBuffer;
   *isOutputInteger = 1;

   spare = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Integer) );
   if( !spare )
      return TA_ALLOC_ERR;

   if( outputNb == 0 )
   {
      out1 = outputBufferInt;
      out2 = spare;
   }
   else
   {
      out1 = spare;
      out2 = outputBufferInt;
   }

   *lookback = TA_FRACTAL_Lookback( p->optInLeftBars, p->optInRightBars );
   retCode = TA_FRACTAL( startIdx, endIdx, p->high, p->low,
                         p->optInLeftBars, p->optInRightBars,
                         outBegIdx, outNbElement, out1, out2 );

   TA_Free( spare );

   return retCode;
}

static ErrorNumber test_fractal_range( const TA_History *history )
{
   FractalRangeParam param;

   /* Asymmetric on purpose: the sweep then also covers the case where the
    * reported bar and the bar being judged are a different distance apart on
    * each side of the window. */
   param.optInLeftBars  = 2;
   param.optInRightBars = 5;
   param.high           = history->high;
   param.low            = history->low;

   return doRangeTestEx( fractalRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 2, 0 );
}
