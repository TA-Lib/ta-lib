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
 *  090426 MF,CC  First version (issue #359).
 */

/* Description:
 *
 *   Test TA_MASSI (Mass Index).
 *
 *   Legs:
 *     1. DIFFERENTIAL against a compose over TA_EMA -> TA_EMA -> per-element
 *        ratio -> TA_SUM, at memcmp exactness, over a fast x slow grid, two
 *        corpora, three anchors and three EMA unstable periods. The grid
 *        includes pairs with fast > slow, which is where pandas-ta-classic
 *        swaps the two and TA_MASSI must not.
 *
 *        Two things make this leg evidence rather than decoration. The
 *        reference is ANCHORED at startIdx - lookback, because TA_EMA re-seeds
 *        at startIdx - its own lookback and an unanchored reference is a
 *        DIFFERENT function once startIdx clears the lookback -- leg 2 proves
 *        the grid separates the two. And the grid is crossed with an
 *        UNSTABLE-PERIOD sweep, because each stage boundary here is the callee
 *        LOOKBACK, not (fast-1); the two coincide exactly at unstable period 0,
 *        which is where every other gate runs.
 *     2. ANCHORING NON-VACUITY. Asserts at least one grid cell where the
 *        anchored and unanchored references disagree.
 *     3. EXTERNAL ORACLES: frozen rows from three independent implementations,
 *        replayed cross-language by server_verify. See massiOracle below.
 *     4. Exact-arithmetic edges, all equalities rather than tolerances:
 *        an all-flat market (exactly optInSlowPeriod, and NaN without the
 *        zero guard), a constant non-zero range (also exactly
 *        optInSlowPeriod, reached through the divide rather than the guard),
 *        and SCALE INVARIANCE -- the same varying series shifted down by
 *        2^-100 must give bit-identical output, which is what an absolute
 *        epsilon band on the guard would destroy (issue #253).
 *     5. Parameter rejection, pinning the declared ranges, plus the lookback
 *        asymmetry that makes the two periods non-interchangeable.
 *     6. In-place aliasing, over inHigh and over inLow, bitwise.
 *     7. The startIdx/endIdx range sweep, CONVERGING against TA_FUNC_UNST_EMA
 *        -- MASSI has no unstable period of its own but inherits EMA's TWICE,
 *        which is also the UNSTABLE_MAP row in test_codegen.c.
 *
 *   Cross-language value coverage comes from server_verify in leg 3 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as CVI).
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

/**** Local declarations. ****/
#define MASSI_CAP    3100
#define MASSI_GD_NB  1000

/* Leg 3. Measured worst agreement of TA_MASSI with the rows below, on this
 * platform: 9.0e-16 relative against pandas over EVERY bar of all four
 * parameter pairs, and 2.5e-15 against the two raw-seed arms over every bar at
 * or past 175. So three decimal orders of headroom, for cross-platform
 * rounding of the two recursions rather than for a measured gap. MASSI is a
 * sum of optInSlowPeriod ratios each near 1, so it never approaches zero and
 * the relative term is always the binding one; the absolute floor is here only
 * so that a row added near zero later cannot silently turn this into a
 * spurious failure. */
#define MASSI_ORACLE_REL 1e-12
#define MASSI_ORACLE_ABS 1e-12

typedef struct { int fast; int slow; int bar; double want; const char *src; } MassiGolden;

/* Goldens captured by ta-lib-oracles/capture_359_massi.py on the 252-bar
 * TA_SREF high/low series, at %.17g, which round-trips to the same double.
 * `bar` is the ABSOLUTE bar index; the output index is bar - begIdx.
 *
 * THREE independent implementations were driven on this exact series
 * (2026-09-04), and all three place the first output at 2*(fast-1)+slow-1,
 * which is the lookback:
 *   1. pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1) -- Python,
 *      `ta.massi`. The only arm that exposes BOTH periods, and the only one
 *      that seeds its EMA the way TA-Lib does (overlap/ema.py defaults
 *      sma=True). It spells the step (1-k)*prev + k*x rather than
 *      ((x-prev)*k)+prev, so it is close rather than bit-identical.
 *   2. Tulip Indicators 0.9.2, pinned be18abb -- C, `ti_mass`.
 *   3. trading-signals 8.3.0 -- TypeScript, `ts.MassIndex`. It is the arm that
 *      corroborates the flat-market answer: `double === 0 ? 1 : single/double`.
 *      Tulip divides unguarded and emits NaN; pandas guards differently, via
 *      the non_zero_range() nudge described below.
 *
 * DO NOT move a tulip or trading-signals row below bar 175. Both hard-code the
 * EMA period at 9 (which is why they appear only on the fast == 9 pairs) and
 * both seed from the first RAW high-low range, so their warm-up is a different
 * convention, not a rounding difference: measured against arm 1 at 9/25 the
 * gap is 7.2e-03 at bar 40, 2.0e-06 at bar 80, 3.7e-12 at bar 140, 4.9e-14 at
 * bar 160 and only 7.2e-16 by bar 180. The published Achelis p.182 vector is
 * seeded that way too and corroborates structure only.
 *
 * pandas swaps fast and slow when slow < fast (volatility/massi.py:26), so
 * every pair here keeps slow > fast; the no-swap ruling is pinned by leg 1's
 * fast > slow cells and by leg 5 instead. Its non_zero_range() also nudges the
 * WHOLE high-low series by 2.2e-16 if ANY bar is flat -- this corpus has none
 * (min high-low is 1.0), which is why no flat-bar row appears here.
 */
static const MassiGolden massiOracle[] =
{
   {  9,  25,  40,      24.624023433455974, "pandas"          },
   {  9,  25,  41,      24.459982721026051, "pandas"          },
   {  9,  25,  93,      26.565153522353782, "pandas"          },
   {  9,  25, 146,      25.186880067585125, "pandas"          },
   {  9,  25, 199,      25.918563721685704, "pandas"          },
   {  9,  25, 251,      24.170501466329661, "pandas"          },
   {  9,  25, 175,      24.308435505865219, "tulip"           },
   {  9,  25, 175,      24.308435505865226, "trading-signals" },
   {  9,  25, 200,      25.942788420389675, "tulip"           },
   {  9,  25, 200,      25.942788420389686, "trading-signals" },
   {  9,  25, 251,      24.170501466329654, "tulip"           },
   {  9,  25, 251,      24.170501466329661, "trading-signals" },
   {  9,  10,  25,      10.058214763147717, "pandas"          },
   {  9,  10,  26,      9.9625189756095835, "pandas"          },
   {  9,  10,  81,      11.314064551303744, "pandas"          },
   {  9,  10, 138,      10.277273333764514, "pandas"          },
   {  9,  10, 195,      9.9107062098171443, "pandas"          },
   {  9,  10, 251,      8.2728021595564947, "pandas"          },
   {  9,  10, 175,      9.8268029413541012, "tulip"           },
   {  9,  10, 175,      9.8268029413541029, "trading-signals" },
   {  9,  10, 200,      10.133421105256309, "tulip"           },
   {  9,  10, 200,      10.133421105256307, "trading-signals" },
   {  9,  10, 251,      8.2728021595564929, "tulip"           },
   {  9,  10, 251,      8.2728021595564947, "trading-signals" },
   {  5,  14,  21,      14.168873942954701, "pandas"          },
   {  5,  14,  22,      14.077787564125472, "pandas"          },
   {  5,  14,  78,       14.87490859308568, "pandas"          },
   {  5,  14, 136,      14.455482691175655, "pandas"          },
   {  5,  14, 194,      13.930580422802295, "pandas"          },
   {  5,  14, 251,      12.414269220196172, "pandas"          },
   {  3,  30,  33,      29.934966341440543, "pandas"          },
   {  3,  30,  34,      29.836340160478606, "pandas"          },
   {  3,  30,  87,      30.120644547127185, "pandas"          },
   {  3,  30, 142,      29.770592759372533, "pandas"          },
   {  3,  30, 197,      30.182844262063217, "pandas"          },
   {  3,  30, 251,      29.598644980276021, "pandas"          },
};
#define NB_MASSI_ORACLE ((int)(sizeof(massiOracle)/sizeof(MassiGolden)))

/* Leg 1 grid. The fast list reaches past every slow value so the fast > slow
 * regime is covered, and both start at the smallest legal period. */
static const int massiFast[] = { 2, 3, 4, 5, 7, 9, 12, 16, 25 };
static const int massiSlow[] = { 2, 3, 5, 9, 14, 20, 25, 30, 40 };
#define NB_MASSI_FAST ((int)(sizeof(massiFast)/sizeof(int)))
#define NB_MASSI_SLOW ((int)(sizeof(massiSlow)/sizeof(int)))

/* 0 is the shipped default; the non-zero values are the only ones that
 * separate a lookback-anchored stage boundary from a (fast-1) one. */
static const int massiUnst[] = { 0, 1, 4 };
#define NB_MASSI_UNST ((int)(sizeof(massiUnst)/sizeof(int)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing.
 * LITERAL expected counts, from the run this file was written against. */
#define MASSI_DIFF_EXPECTED   721008
#define MASSI_ANCHOR_EXPECTED 162
#define MASSI_EDGE_EXPECTED   68728
#define MASSI_ALIAS_EXPECTED  192492

static int g_massiDiffCmp;
static int g_massiOracleCmp;
static int g_massiAnchorSep;
static int g_massiEdgeCmp;
static int g_massiAliasCmp;

/**** Local functions declarations. ****/
static int massi_build_reference( const TA_Real *high, const TA_Real *low,
                                  int base, int endIdx, int fast, int slow,
                                  TA_Real *dest, int *refBeg, int *refNb );
static ErrorNumber test_massi_differential( const char *tag, const TA_Real *high,
                                            const TA_Real *low, int nbBars );
static ErrorNumber test_massi_anchoring( const TA_Real *high, const TA_Real *low,
                                         int nbBars );
static ErrorNumber test_massi_oracle( const TA_History *history );
static ErrorNumber test_massi_edges( void );
static ErrorNumber test_massi_param_reject( const TA_History *history );
static ErrorNumber test_massi_aliasing( const char *tag, const TA_Real *high,
                                        const TA_Real *low, int nbBars );
static ErrorNumber test_massi_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_massi( TA_History *history )
{
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_massiDiffCmp = g_massiOracleCmp = g_massiAnchorSep = 0;
   g_massiEdgeCmp = g_massiAliasCmp = 0;

   err = test_massi_differential( "TA_SREF", history->high, history->low, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_differential( "gData", gDataHigh, gDataLow, MASSI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_anchoring( history->high, history->low, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_param_reject( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_aliasing( "TA_SREF", history->high, history->low, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_aliasing( "gData", gDataHigh, gDataLow, MASSI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_massi_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_massiDiffCmp != MASSI_DIFF_EXPECTED
            || g_massiOracleCmp != NB_MASSI_ORACLE
            || g_massiAnchorSep != MASSI_ANCHOR_EXPECTED
            || g_massiEdgeCmp != MASSI_EDGE_EXPECTED
            || g_massiAliasCmp != MASSI_ALIAS_EXPECTED ) )
   {
      printf( "MASSI Fail: coverage counters (diff %d, oracle %d, anchor %d, "
              "edges %d, alias %d) are not what this file was written with "
              "(%d, %d, %d, %d, %d)\n",
              g_massiDiffCmp, g_massiOracleCmp, g_massiAnchorSep,
              g_massiEdgeCmp, g_massiAliasCmp,
              MASSI_DIFF_EXPECTED, NB_MASSI_ORACLE, MASSI_ANCHOR_EXPECTED,
              MASSI_EDGE_EXPECTED, MASSI_ALIAS_EXPECTED );
      return TA_MASSI_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Rebuild MASSI from shipped primitives only: TA_EMA of the high-low range,
 * TA_EMA of that, their per-element ratio, TA_SUM of it.
 *
 * `base` is the first input bar the caller is allowed to consume. Passing
 * (refStart - lookback) anchors the reference exactly where TA_MASSI anchors;
 * passing 0 builds the UNANCHORED variant leg 2 contrasts against.
 *
 * Returns 0 when the range is too short to produce anything.
 */
static int massi_build_reference( const TA_Real *high, const TA_Real *low,
                                  int base, int endIdx, int fast, int slow,
                                  TA_Real *dest, int *refBeg, int *refNb )
{
   static TA_Real hl[MASSI_CAP], e1[MASSI_CAP], e2[MASSI_CAP], ratio[MASSI_CAP];
   TA_Integer begE1, nbE1, begE2, nbE2, begS, nbS;
   int n = endIdx - base + 1, i;

   for( i = 0; i < n; i++ )
      hl[i] = high[base+i] - low[base+i];

   if( TA_EMA( 0, n-1, hl, fast, &begE1, &nbE1, e1 ) != TA_SUCCESS || nbE1 <= 0 )
      return 0;
   if( TA_EMA( 0, nbE1-1, e1, fast, &begE2, &nbE2, e2 ) != TA_SUCCESS || nbE2 <= 0 )
      return 0;
   for( i = 0; i < nbE2; i++ )
      ratio[i] = ( e2[i] == 0.0 ) ? 1.0 : e1[begE2+i] / e2[i];
   if( TA_SUM( 0, nbE2-1, ratio, slow, &begS, &nbS, dest ) != TA_SUCCESS || nbS <= 0 )
      return 0;

   *refBeg = base + (int)begE1 + (int)begE2 + (int)begS;
   *refNb  = (int)nbS;
   return 1;
}

/* (1) TA_MASSI against that compose, memcmp-exact.
 *
 * The fused body keeps TA_EMA's seed accumulation and recursion and TA_SUM's
 * add-publish-subtract order verbatim, so the two paths are the same
 * arithmetic in the same order and the only admissible difference is none at
 * all. A reordered accumulation shows up here at ~1e-16 and a wrong formula at
 * whole percent; both fail.
 */
static ErrorNumber test_massi_differential( const char *tag, const TA_Real *high,
                                            const TA_Real *low, int nbBars )
{
   static TA_Real ref[MASSI_CAP], out[MASSI_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int f, s, a, u, k, startIdx, lookbackTotal, refStart, refBeg, refNb;

   for( u = 0; u < NB_MASSI_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (unsigned int)massiUnst[u] );

      for( f = 0; f < NB_MASSI_FAST; f++ )
      {
         for( s = 0; s < NB_MASSI_SLOW; s++ )
         {
            int fast = massiFast[f], slow = massiSlow[s];

            lookbackTotal = TA_MASSI_Lookback( fast, slow );

            for( a = 0; a < 3; a++ )
            {
               startIdx = ( a == 0 ) ? 0
                        : ( a == 1 ) ? lookbackTotal + 11
                                     : nbBars / 2;
               refStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;
               if( refStart > nbBars-1 )
                  continue;

               retCode = TA_MASSI( startIdx, nbBars-1, high, low, fast, slow,
                                   &begIdx, &nbElement, out );
               if( retCode != TA_SUCCESS || begIdx != refStart
                   || nbElement != nbBars - refStart )
               {
                  printf( "MASSI differential Fail [%s F=%d S=%d K=%d start=%d]: "
                          "rc=%d (%d,%d) expected (%d,%d)\n",
                          tag, fast, slow, massiUnst[u], startIdx, (int)retCode,
                          begIdx, nbElement, refStart, nbBars - refStart );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }

               if( !massi_build_reference( high, low, refStart - lookbackTotal,
                                           nbBars-1, fast, slow,
                                           ref, &refBeg, &refNb ) )
               {
                  printf( "MASSI differential Fail [%s F=%d S=%d K=%d start=%d]: "
                          "reference compose produced nothing\n",
                          tag, fast, slow, massiUnst[u], startIdx );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_RETCODE;
               }
               if( refBeg != begIdx || refNb != nbElement )
               {
                  printf( "MASSI differential Fail [%s F=%d S=%d K=%d start=%d]: "
                          "reference range (%d,%d) vs (%d,%d)\n",
                          tag, fast, slow, massiUnst[u], startIdx,
                          refBeg, refNb, begIdx, nbElement );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }

               for( k = 0; k < nbElement; k++ )
               {
                  g_massiDiffCmp++;
                  if( memcmp( &ref[k], &out[k], sizeof(double) ) != 0 )
                  {
                     printf( "MASSI differential Fail [%s F=%d S=%d K=%d start=%d] "
                             "out %d: got %.17g expected %.17g -- the compose is "
                             "the same arithmetic in the same order, so this is "
                             "exact or it is wrong\n",
                             tag, fast, slow, massiUnst[u], startIdx, k,
                             out[k], ref[k] );
                     TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                     return TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  }
               }
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (2) ANCHORING NON-VACUITY: the grid must contain a cell where the anchored
 * reference and an unanchored one (always from bar 0) disagree. Without such a
 * cell, leg 1 would pass against a reference that ignores startIdx. */
static ErrorNumber test_massi_anchoring( const TA_Real *high, const TA_Real *low,
                                         int nbBars )
{
   static TA_Real anchored[MASSI_CAP], unanchored[MASSI_CAP];
   int f, s, a, startIdx, lookbackTotal, refStart;
   int aBeg, aNb, bBeg, bNb, i, nbCells = 0;

   for( f = 0; f < NB_MASSI_FAST; f++ )
   {
      for( s = 0; s < NB_MASSI_SLOW; s++ )
      {
         int fast = massiFast[f], slow = massiSlow[s];

         lookbackTotal = TA_MASSI_Lookback( fast, slow );

         for( a = 0; a < 2; a++ )
         {
            startIdx = ( a == 0 ) ? lookbackTotal + 11 : nbBars / 2;
            refStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;
            if( refStart > nbBars-1 )
               continue;
            if( !massi_build_reference( high, low, refStart - lookbackTotal,
                                        nbBars-1, fast, slow,
                                        anchored, &aBeg, &aNb ) )
               continue;
            if( !massi_build_reference( high, low, 0, nbBars-1, fast, slow,
                                        unanchored, &bBeg, &bNb ) )
               continue;

            nbCells++;
            i = aBeg - bBeg;
            if( i >= 0 && i < bNb && anchored[0] != unanchored[i] )
               g_massiAnchorSep++;
         }
      }
   }

   if( nbCells == 0 )
   {
      printf( "MASSI anchoring Fail: no cell built a reference at all\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( g_massiAnchorSep == 0 )
   {
      printf( "MASSI anchoring Fail: no cell separates the anchored reference "
              "from the unanchored one (%d cell(s)); leg 1 would then pass "
              "against a reference that ignores startIdx\n", nbCells );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/* (3) The frozen oracle rows, plus the cross-language replay. */
static ErrorNumber test_massi_oracle( const TA_History *history )
{
   static TA_Real out[MASSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastFast = -1, lastSlow = -1;

   if( nbBars != 252 )
   {
      printf( "MASSI oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_MASSI_ORACLE; k++ )
   {
      double got, err;
      const char *mode;

      if( massiOracle[k].fast != lastFast || massiOracle[k].slow != lastSlow )
      {
         int wantBeg;

         lastFast = massiOracle[k].fast;
         lastSlow = massiOracle[k].slow;
         wantBeg  = 2*(lastFast-1) + lastSlow - 1;

         retCode = TA_MASSI( 0, nbBars-1, history->high, history->low,
                             lastFast, lastSlow, &begIdx, &nbElement, out );
         /* All three arms put the first value at 2*(fast-1)+slow-1. That is
          * the one structural claim they make jointly, and it is worth more
          * than any single value of theirs. */
         if( retCode != TA_SUCCESS || begIdx != wantBeg
             || nbElement != nbBars - wantBeg )
         {
            printf( "MASSI oracle Fail [F=%d S=%d]: rc=%d (%d,%d) expected "
                    "(%d,%d)\n", lastFast, lastSlow, (int)retCode,
                    begIdx, nbElement, wantBeg, nbBars - wantBeg );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastFast;
            optIn[1] = (double)lastSlow;
            e = server_verify( "MASSI", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               optIn, 2,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "MASSI oracle [F=%d S=%d]: compared no server despite "
                       "live pipes\n", lastFast, lastSlow );
               return TA_MASSI_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( massiOracle[k].bar < begIdx
          || massiOracle[k].bar - begIdx >= nbElement )
      {
         printf( "MASSI oracle Fail [F=%d S=%d]: golden bar %d is outside the "
                 "output [%d..%d]\n", massiOracle[k].fast, massiOracle[k].slow,
                 massiOracle[k].bar, begIdx, begIdx + nbElement - 1 );
         return TA_MASSI_VACUOUS;
      }

      got = out[massiOracle[k].bar - begIdx];
      g_massiOracleCmp++;
      if( !checkOracleValue( got, massiOracle[k].want, MASSI_ORACLE_REL,
                             MASSI_ORACLE_ABS, &err, &mode ) )
      {
         printf( "MASSI oracle Fail [%s F=%d S=%d] at bar %d: got %.17g "
                 "expected %.17g (%s err %.3g, tol rel %g abs %g)\n",
                 massiOracle[k].src, massiOracle[k].fast, massiOracle[k].slow,
                 massiOracle[k].bar, got, massiOracle[k].want, mode, err,
                 MASSI_ORACLE_REL, MASSI_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) Exact-arithmetic edges. Every assertion here is an equality.
 *
 *   - ALL FLAT. high == low on every bar, so both averages are exactly zero
 *     and every ratio is the guarded 1.0: the sum is exactly optInSlowPeriod.
 *     Non-vacuous by construction -- without the guard the divide is 0/0 and
 *     every value is NaN, which fails the equality.
 *   - CONSTANT NON-ZERO RANGE. high - low is exactly 8 on every bar, so both
 *     averages equal 8 and the ratio reaches 1.0 through the DIVIDE rather
 *     than through the guard. Same answer, different path: it is what says the
 *     flat-market result is the continuous limit and not an artifact.
 *   - SCALE INVARIANCE. A varying range, and the same range shifted down by
 *     2^-100. Every operation on the path is exactly equivariant under a
 *     power-of-two scale and the ratio cancels it outright, so the two outputs
 *     must be BIT-IDENTICAL. They are not if the zero test is an absolute
 *     epsilon band: at 1e-30 every range is "zero" to TA_IS_ZERO, every ratio
 *     collapses to 1.0 and the scaled series reports optInSlowPeriod
 *     everywhere (issue #253). The sweep therefore also asserts that the
 *     scaled output is NOWHERE equal to optInSlowPeriod.
 */
static ErrorNumber test_massi_edges( void )
{
   static TA_Real high[256], low[256], high2[256], low2[256];
   static TA_Real out[MASSI_CAP], out2[MASSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int i, f, s, nb;

   /* -- ALL FLAT, then CONSTANT NON-ZERO RANGE -- */
   nb = 120;
   for( i = 0; i < nb; i++ )
   {
      high[i]  = 42.0;  low[i]  = 42.0;
      high2[i] = 100.0; low2[i] = 92.0;
   }

   for( f = 2; f <= 12; f++ )
   {
      for( s = 2; s <= 12; s++ )
      {
         int wantBeg = 2*(f-1) + s - 1;
         double want = (double)s;

         retCode = TA_MASSI( 0, nb-1, high, low, f, s,
                             &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != wantBeg
             || nbElement != nb - wantBeg )
         {
            printf( "MASSI all-flat Fail [F=%d S=%d]: rc=%d (%d,%d) expected "
                    "(%d,%d)\n", f, s, (int)retCode, begIdx, nbElement,
                    wantBeg, nb - wantBeg );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         retCode = TA_MASSI( 0, nb-1, high2, low2, f, s,
                             &begIdx2, &nbElement2, out2 );
         if( retCode != TA_SUCCESS || begIdx2 != wantBeg
             || nbElement2 != nb - wantBeg )
         {
            printf( "MASSI const-range Fail [F=%d S=%d]: rc=%d (%d,%d)\n",
                    f, s, (int)retCode, begIdx2, nbElement2 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            g_massiEdgeCmp++;
            if( memcmp( &want, &out[i], sizeof(double) ) != 0 )
            {
               printf( "MASSI all-flat Fail [F=%d S=%d] out %d: %.17g, expected "
                       "exactly %.17g (NaN => the zero-denominator guard is "
                       "missing; 0 => it answers with an oscillator's neutral "
                       "instead of the ratio's continuous limit)\n",
                       f, s, i, out[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_massiEdgeCmp++;
            if( memcmp( &want, &out2[i], sizeof(double) ) != 0 )
            {
               printf( "MASSI const-range Fail [F=%d S=%d] out %d: %.17g, "
                       "expected exactly %.17g -- a constant non-zero range "
                       "reaches 1.0 through the divide, not the guard\n",
                       f, s, i, out2[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   /* -- SCALE INVARIANCE -- */
   nb = 200;
   for( i = 0; i < nb; i++ )
   {
      double hl = 1.0 + 0.5 * (double)( i % 3 );

      low[i]   = 0.0;  high[i]  = hl;
      low2[i]  = 0.0;  high2[i] = ldexp( hl, -100 );
   }

   for( f = 2; f <= 12; f++ )
   {
      for( s = 2; s <= 12; s++ )
      {
         retCode = TA_MASSI( 0, nb-1, high, low, f, s,
                             &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "MASSI scale Fail [F=%d S=%d]: rc=%d\n", f, s, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         retCode = TA_MASSI( 0, nb-1, high2, low2, f, s,
                             &begIdx2, &nbElement2, out2 );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx
             || nbElement2 != nbElement )
         {
            printf( "MASSI scale Fail [F=%d S=%d]: rc=%d shape (%d,%d) vs "
                    "(%d,%d)\n", f, s, (int)retCode, begIdx2, nbElement2,
                    begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            double flat = (double)s;

            g_massiEdgeCmp++;
            if( memcmp( &out[i], &out2[i], sizeof(double) ) != 0 )
            {
               printf( "MASSI scale Fail [F=%d S=%d] out %d: %.17g at unit "
                       "scale, %.17g at 2^-100. The ratio cancels a power-of-"
                       "two scale exactly, so an absolute epsilon band on the "
                       "zero test is the only thing that can separate them\n",
                       f, s, i, out[i], out2[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_massiEdgeCmp++;
            if( memcmp( &flat, &out2[i], sizeof(double) ) == 0 )
            {
               printf( "MASSI scale Fail [F=%d S=%d] out %d: the scaled series "
                       "reports exactly %.17g, the flat-market answer, so this "
                       "sweep is comparing two collapsed outputs and proves "
                       "nothing\n", f, s, i, flat );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) Parameter rejection, and the lookback asymmetry.
 *
 * Both declared ranges start at 2. For the EMA period that is load-bearing
 * rather than cosmetic: at 1 the recursion reduces to (x-prev)+prev, which is
 * NOT a copy of x once consecutive values leave a factor of two of each other,
 * so a period-1 MASSI would silently stop matching a composed TA_EMA -- and it
 * would still read green on this corpus, whose ranges never move that far in
 * one bar.
 *
 * The lookback asymmetry is what says the two periods are not
 * interchangeable: pandas-ta-classic swaps them when slow < fast, and a MASSI
 * that did the same would answer the same number for both orders.
 */
static ErrorNumber test_massi_param_reject( const TA_History *history )
{
   static TA_Real out[MASSI_CAP];
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k;
   static const int BAD[][2] = { {1,25}, {0,25}, {-2,25}, {9,1}, {9,0},
                                 {9,-1}, {100001,25}, {9,100001} };

   for( k = 0; k < (int)(sizeof(BAD)/sizeof(BAD[0])); k++ )
   {
      if( TA_MASSI( 0, nbBars-1, history->high, history->low,
                    BAD[k][0], BAD[k][1],
                    &begIdx, &nbElement, out ) != TA_BAD_PARAM )
      {
         printf( "MASSI param Fail: (F=%d,S=%d) was accepted; both periods "
                 "start at 2\n", BAD[k][0], BAD[k][1] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( TA_MASSI_Lookback( BAD[k][0], BAD[k][1] ) != -1 )
      {
         printf( "MASSI param Fail: TA_MASSI_Lookback(%d,%d) answered a number "
                 "for a pair the call rejects\n", BAD[k][0], BAD[k][1] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   if( TA_MASSI( 0, nbBars-1, history->high, history->low, 2, 2,
                 &begIdx, &nbElement, out ) != TA_SUCCESS || begIdx != 3 )
   {
      printf( "MASSI param Fail: the smallest legal pair (2,2) was rejected or "
              "anchored wrong\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   if( TA_MASSI_Lookback( 25, 3 ) == TA_MASSI_Lookback( 3, 25 ) )
   {
      printf( "MASSI param Fail: the lookback is symmetric in its two periods, "
              "so they are interchangeable -- the EMA period must enter twice "
              "and the summation period once\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (6) In-place aliasing, bitwise, over each price component in turn.
 *
 * Safe by construction -- the lookback clamp puts every store at least
 * lookbackTotal bars behind the bar being read -- but it is the leg that would
 * catch a store landing under a later read, and VHF's caught one.
 */
static ErrorNumber test_massi_aliasing( const char *tag, const TA_Real *high,
                                        const TA_Real *low, int nbBars )
{
   static TA_Real clean[MASSI_CAP], aliasHigh[MASSI_CAP], aliasLow[MASSI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int f, s, i;

   for( f = 0; f < NB_MASSI_FAST; f++ )
   {
      for( s = 0; s < NB_MASSI_SLOW; s++ )
      {
         int fast = massiFast[f], slow = massiSlow[s];

         retCode = TA_MASSI( 0, nbBars-1, high, low, fast, slow,
                             &begIdx, &nbElement, clean );
         if( retCode != TA_SUCCESS )
         {
            printf( "MASSI alias Fail [%s F=%d S=%d]: rc=%d\n",
                    tag, fast, slow, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbBars; i++ )
         {
            aliasHigh[i] = high[i];
            aliasLow[i]  = low[i];
         }
         retCode = TA_MASSI( 0, nbBars-1, aliasHigh, aliasLow, fast, slow,
                             &begIdx2, &nbElement2, aliasHigh );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx
             || nbElement2 != nbElement )
         {
            printf( "MASSI alias Fail [%s F=%d S=%d over inHigh]: rc=%d shape "
                    "(%d,%d) vs (%d,%d)\n", tag, fast, slow, (int)retCode,
                    begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_massiAliasCmp++;
            if( memcmp( &clean[i], &aliasHigh[i], sizeof(double) ) != 0 )
            {
               printf( "MASSI alias Fail [%s F=%d S=%d over inHigh] out %d: "
                       "separate %.17g, in-place %.17g -- a store landed under "
                       "a read a later bar still needed\n",
                       tag, fast, slow, i, clean[i], aliasHigh[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         for( i = 0; i < nbBars; i++ )
         {
            aliasHigh[i] = high[i];
            aliasLow[i]  = low[i];
         }
         retCode = TA_MASSI( 0, nbBars-1, aliasHigh, aliasLow, fast, slow,
                             &begIdx2, &nbElement2, aliasLow );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx
             || nbElement2 != nbElement )
         {
            printf( "MASSI alias Fail [%s F=%d S=%d over inLow]: rc=%d shape "
                    "(%d,%d) vs (%d,%d)\n", tag, fast, slow, (int)retCode,
                    begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_massiAliasCmp++;
            if( memcmp( &clean[i], &aliasLow[i], sizeof(double) ) != 0 )
            {
               printf( "MASSI alias Fail [%s F=%d S=%d over inLow] out %d: "
                       "separate %.17g, in-place %.17g\n",
                       tag, fast, slow, i, clean[i], aliasLow[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. MASSI seeds two stacked EMA at its own
 * lookback, so it converges across startIdx rather than matching:
 * TA_STABLE_CONVERGING with TA_FUNC_UNST_EMA, the id MASSI is also mapped to
 * in test_codegen.c's UNSTABLE_MAP. doRangeTestEx cross-checks the pair --
 * CONVERGING with TA_TEST_UNST_NONE is a stability mismatch. */
typedef struct { int fast; int slow; const TA_Real *high; const TA_Real *low; } MassiRangeParam;

static TA_RetCode massiRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                          TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                          TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                          TA_Integer *lookback, void *opaqueData,
                                          unsigned int outputNb, unsigned int *isOutputInteger )
{
   MassiRangeParam *p = (MassiRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_MASSI_Lookback( p->fast, p->slow );
   return TA_MASSI( startIdx, endIdx, p->high, p->low, p->fast, p->slow,
                    outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_massi_range( const TA_History *history )
{
   MassiRangeParam param;

   param.fast = 9;
   param.slow = 25;
   param.high = history->high;
   param.low  = history->low;

   return doRangeTestEx( massiRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)&param, 1, 0 );
}
