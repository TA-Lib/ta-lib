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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  100626 KL,CC First version (proposal FISHER, #485).
 */

/* Description:
 *
 *   Test TA_FISHER (Ehlers' Fisher Transform, #485).
 *
 *   The cross-language gates compare every language against this library, so
 *   none of them can catch a wrong formula. What constrains it here is TWO
 *   independent readings of the author: a 50-digit transcription of his
 *   EasyLanguage listing (John F. Ehlers, "Using The Fisher Transform",
 *   Stocks & Commodities V.20:11, November 2002, pp.40-42, Figure 4), and
 *   Tulip Indicators' own published regression vector, which is a separate
 *   implementation of the same listing.
 *
 *   Legs:
 *     1. GOLDEN A, the committed corpus at the default period, clamp-free.
 *        Held at 1e-12*max(1,|v|). Also the leg that drives server_verify.
 *     2. GOLDEN B, the corpus at period 20, where v reaches 0.99172 and
 *        0.99456 and the clamp fires. This is the leg that tells the three
 *        readings of the clamp apart: LEAN's .999 threshold misses bars 131
 *        and 132 by 1.06 and 1.70, and the form that clamps for the transform
 *        WITHOUT feeding the clamped value back misses bar 132 by 1.17. A
 *        tolerance cannot absorb any of those.
 *     3. TULIP, the vector published at tests/untest.txt:189-193, period 5
 *        over 15 bars. An independent implementation, held at 1e-3 absolute
 *        because that vector prints three decimals. Its first trigger is
 *        0.000, which is the same seed rule this file asserts in leg 1.
 *     4. LOOKBACK AND THE UNSTABLE PERIOD. The lookback is n-1 plus whatever
 *        TA_SetUnstablePeriod holds, and the first trigger is the 0 seed only
 *        when nothing was discarded: with U > 0 it is the computed value of
 *        the bar before the first output, not a seed.
 *     5. FLAT WINDOW. A constant series has no channel; the ruling is the
 *        neutral position, so every output is exactly 0 rather than a
 *        division by its own zero range.
 *     6. ALIASING. Each output buffer over each input in turn.
 *     7. RANGE INDEPENDENCE, swept via doRangeTestEx.
 *
 *   NOT verified here: LEAN's spy_with_fisher.txt (external data this box does
 *   not have) and the ta-lib-oracles LEAN arm.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define FISH_NB_BAR 252
#define FISH_TOL    1e-12      /* times max(1, |want|) */

#define FISH_GOLDEN_CMP   30   /* 8 rows of A plus 7 of B, two outputs each */
#define FISH_TULIP_CMP    22   /* 11 bars, two outputs */
/* 5 unstable settings x 4 periods, each checking _Lookback and then the
 * call's own begIdx plus the seed rule: 5*4*2. */
#define FISH_LOOKBACK_CMP 40
/* 252 bars less the default period's 9-bar lookback. */
#define FISH_FLAT_CMP    243
#define FISH_ALIAS_CMP     4

static int g_fishGoldenCmp;
static int g_fishTulipCmp;
static int g_fishLookbackCmp;
static int g_fishFlatCmp;
static int g_fishAliasCmp;

typedef struct { int period; int bar; double fisher; double trigger; } FishGolden;

/* A: the committed corpus at the default period, clamp-free. From a 50-digit
 * evaluation of the listing; the shipped binary64 form reproduces every row
 * to 4.4e-16 on this host.
 *
 * B: the same corpus at period 20, where the clamp fires on bars 131 and 132.
 */
static const FishGolden fishGolden[] =
{
   { 10,   9, -0.2226615347277912,  0.0                  },
   { 10,  10, -0.0510874254334018, -0.2226615347277912   },
   { 10,  11,  0.3632435660223233, -0.0510874254334018   },
   { 10,  50,  1.400346290517107,   1.717352256019719    },
   { 10, 100, -0.1816990006930449,  0.0560026095809204   },
   { 10, 150, -1.451401155387904,  -1.955975502199476    },
   { 10, 200, -3.019372570680587,  -3.102703206073857    },
   { 10, 251,  0.6170529429148078,  1.09819925110516     },
   { 20,  19, -0.1868523206496722,  0.0                  },
   { 20, 130,  4.694477305846587,   4.30901158771376     },
   { 20, 131,  6.147439820173494,   4.694477305846587    },
   { 20, 132,  6.873921077336947,   6.147439820173494    },
   { 20, 133,  5.316033133597328,   6.873921077336947    },
   { 20, 142, -1.162513924310997,  -0.9907375315616144   },
   { 20, 251, -0.5588166723173358, -0.3533512470061788   }
};
#define FISH_NB_GOLDEN ((int)(sizeof(fishGolden)/sizeof(fishGolden[0])))

/* Tulip Indicators, tests/untest.txt:189-193, `fisher 5` over 15 bars. */
static const double tulipHigh[15] =
{ 82.15,81.89,83.03,83.30,83.85,83.90,83.33,84.30,84.84,85.00,85.90,86.58,86.98,88.00,87.87 };
static const double tulipLow[15] =
{ 81.29,80.64,81.31,82.65,83.07,83.11,82.49,82.30,84.15,84.11,84.03,85.39,85.76,87.17,87.01 };
static const double tulipFisher[11] =
{ 0.343,0.791,0.825,0.806,1.066,1.439,1.851,2.275,2.698,3.117,3.185 };
static const double tulipTrigger[11] =
{ 0.000,0.343,0.791,0.825,0.806,1.066,1.439,1.851,2.275,2.698,3.117 };

static ErrorNumber test_fish_golden  ( const TA_History *history );
static ErrorNumber test_fish_tulip   ( void );
static ErrorNumber test_fish_lookback( const TA_History *history );
static ErrorNumber test_fish_flat    ( void );
static ErrorNumber test_fish_aliasing( const TA_History *history );
static ErrorNumber test_fish_range   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_fisher( TA_History *history )
{
   ErrorNumber retValue;

   g_fishGoldenCmp = 0;
   g_fishTulipCmp = 0;
   g_fishLookbackCmp = 0;
   g_fishFlatCmp = 0;
   g_fishAliasCmp = 0;

   if( history->nbBars != FISH_NB_BAR )
   {
      printf( "Fail: TA_FISHER expects the %d-bar corpus, got %d\n",
              FISH_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_fish_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fish_tulip();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fish_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fish_flat();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fish_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fish_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_fishGoldenCmp   != FISH_GOLDEN_CMP
    || g_fishTulipCmp    != FISH_TULIP_CMP
    || g_fishLookbackCmp != FISH_LOOKBACK_CMP
    || g_fishFlatCmp     != FISH_FLAT_CMP
    || g_fishAliasCmp    != FISH_ALIAS_CMP )
   {
      printf( "Fail: TA_FISHER comparison counts (golden %d, tulip %d, lookback %d, "
              "flat %d, alias %d) are not what this file asserts (%d, %d, %d, %d, %d)\n",
              g_fishGoldenCmp, g_fishTulipCmp, g_fishLookbackCmp, g_fishFlatCmp,
              g_fishAliasCmp,
              FISH_GOLDEN_CMP, FISH_TULIP_CMP, FISH_LOOKBACK_CMP, FISH_FLAT_CMP,
              FISH_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

static int fishClose( double got, double want, double tol )
{
   double scale = ( fabs(want) > 1.0 ) ? fabs(want) : 1.0;
   return fabs( got - want ) <= tol * scale;
}

/* (1)+(2) The two golden tables, and the leg that reaches the servers. */
static ErrorNumber test_fish_golden( const TA_History *history )
{
   static TA_Real outF[FISH_NB_BAR], outT[FISH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, lastPeriod;

   lastPeriod = -1;

   for( k = 0; k < FISH_NB_GOLDEN; k++ )
   {
      const FishGolden *g = &fishGolden[k];
      double gotF, gotT;

      rc = TA_FISHER( 0, (int)history->nbBars - 1, history->high, history->low,
                      g->period, &begIdx, &nbElement, outF, outT );
      if( rc != TA_SUCCESS || begIdx != g->period - 1 )
      {
         printf( "Fail: TA_FISHER golden rc=%d begIdx=%d (n=%d, want %d)\n",
                 (int)rc, (int)begIdx, g->period, g->period - 1 );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* Once per period, send the same call through every language server.
       * Without this the group runs under --codegen having compared nothing
       * against any server, which --codegen reports as a failure.
       */
      if( g->period != lastPeriod && server_verify_active() )
      {
         const double opt[1] = { (double)g->period };
         ErrorNumber e = server_verify( "FISHER", 0,
                            (int)history->nbBars - 1, (int)history->nbBars,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->high, history->low, NULL },
                            opt, 1,
                            (const TA_Real*[]){ outF, outT, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
         lastPeriod = g->period;
      }

      gotF = outF[g->bar - begIdx];
      gotT = outT[g->bar - begIdx];

      if( !fishClose( gotF, g->fisher, FISH_TOL ) )
      {
         printf( "Fail: TA_FISHER n=%d bar %d outFisher %.17g, expected %.17g\n",
                 g->period, g->bar, gotF, g->fisher );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishGoldenCmp++;

      /* The seed row is an EXACT zero, not a small number. */
      if( g->trigger == 0.0 )
      {
         if( gotT != 0.0 )
         {
            printf( "Fail: TA_FISHER n=%d bar %d outTrigger %.17g, expected "
                    "exactly 0\n", g->period, g->bar, gotT );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      else if( !fishClose( gotT, g->trigger, FISH_TOL ) )
      {
         printf( "Fail: TA_FISHER n=%d bar %d outTrigger %.17g, expected %.17g\n",
                 g->period, g->bar, gotT, g->trigger );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishGoldenCmp++;
   }

   return TA_TEST_PASS;
}

/* (3) Tulip's published vector: a second implementation of the same listing. */
static ErrorNumber test_fish_tulip( void )
{
   TA_Real outF[15], outT[15];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   rc = TA_FISHER( 0, 14, tulipHigh, tulipLow, 5, &begIdx, &nbElement, outF, outT );
   if( rc != TA_SUCCESS || begIdx != 4 || nbElement != 11 )
   {
      printf( "Fail: TA_FISHER tulip rc=%d range %d/%d (want 4/11)\n",
              (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < 11; i++ )
   {
      /* Absolute, and 1e-3: the vector prints three decimals, so that is the
       * whole resolution this oracle has. It is not a statement about the
       * agreement, which measures 4.0e-4 worst here against values printed to
       * the same place.
       */
      if( fabs( outF[i] - tulipFisher[i] ) > 1e-3 )
      {
         printf( "Fail: TA_FISHER tulip bar %d outFisher %.17g, vector %.3f\n",
                 i + 4, outF[i], tulipFisher[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishTulipCmp++;

      if( fabs( outT[i] - tulipTrigger[i] ) > 1e-3 )
      {
         printf( "Fail: TA_FISHER tulip bar %d outTrigger %.17g, vector %.3f\n",
                 i + 4, outT[i], tulipTrigger[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishTulipCmp++;
   }

   return TA_TEST_PASS;
}

/* (4) The lookback moves with the unstable period, and the seed is only
 * exposed when nothing was discarded. */
static ErrorNumber test_fish_lookback( const TA_History *history )
{
   static const int periods[] = { 2, 10, 20, 100 };
   static const unsigned int unst[] = { 0, 1, 7, 30, 0 };
   static TA_Real outF[FISH_NB_BAR], outT[FISH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int p, u, want;

   for( u = 0; u < (int)(sizeof(unst)/sizeof(unst[0])); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, unst[u] );

      for( p = 0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
      {
         want = periods[p] - 1 + (int)unst[u];

         if( TA_FISHER_Lookback( periods[p] ) != want )
         {
            printf( "Fail: TA_FISHER_Lookback(%d) = %d at U=%u, expected %d\n",
                    periods[p], TA_FISHER_Lookback( periods[p] ), unst[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_fishLookbackCmp++;

         rc = TA_FISHER( 0, (int)history->nbBars - 1, history->high, history->low,
                         periods[p], &begIdx, &nbElement, outF, outT );
         if( rc != TA_SUCCESS || begIdx != want )
         {
            printf( "Fail: TA_FISHER rc=%d begIdx=%d at n=%d U=%u, expected %d\n",
                    (int)rc, (int)begIdx, periods[p], unst[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* The seed is only ever exposed at U = 0. Discard even one bar and
          * the first trigger is a computed value, so a zero there would mean
          * the recursion was restarted rather than warmed.
          */
         if( unst[u] == 0 )
         {
            if( outT[0] != 0.0 )
            {
               printf( "Fail: TA_FISHER first trigger %.17g at U=0, expected "
                       "exactly 0 (n=%d)\n", outT[0], periods[p] );
               TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, 0 );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         else if( outT[0] == 0.0 )
         {
            printf( "Fail: TA_FISHER first trigger is the 0 seed at U=%u (n=%d); "
                    "the warm-up did not carry\n", unst[u], periods[p] );
            TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_fishLookbackCmp++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_FISHER, 0 );
   return TA_TEST_PASS;
}

/* (5) A flat series has no channel: the neutral position, so exactly 0. */
static ErrorNumber test_fish_flat( void )
{
   static TA_Real h[FISH_NB_BAR], l[FISH_NB_BAR];
   static TA_Real outF[FISH_NB_BAR], outT[FISH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < FISH_NB_BAR; i++ )
   {
      h[i] = 100.0;
      l[i] = 100.0;
   }

   rc = TA_FISHER( 0, FISH_NB_BAR-1, h, l, 10, &begIdx, &nbElement, outF, outT );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_FISHER flat rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      /* Exactly zero, not small: r = 0.5 makes the bar's contribution
       * identically zero, so both recursions stay at their zero seed. A
       * division by the window's own zero range would be a NaN here, and
       * Tulip's 0.001 floor would read -7.6 -- maximally oversold on a market
       * that has not moved.
       */
      if( outF[i] != 0.0 || outT[i] != 0.0 )
      {
         printf( "Fail: TA_FISHER flat bar %d: fisher %.17g trigger %.17g, "
                 "expected exactly 0\n", (int)begIdx + i, outF[i], outT[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishFlatCmp++;
   }

   return TA_TEST_PASS;
}

/* (6) Each output over each input in turn. */
static ErrorNumber test_fish_aliasing( const TA_History *history )
{
   static TA_Real refF[FISH_NB_BAR], refT[FISH_NB_BAR];
   static TA_Real workH[FISH_NB_BAR], workL[FISH_NB_BAR], other[FISH_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int nb, i, which;

   nb = (int)history->nbBars;

   rc = TA_FISHER( 0, nb-1, history->high, history->low, 10,
                   &begIdx, &nbElement, refF, refT );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_FISHER aliasing baseline failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( which = 0; which < 2; which++ )
   {
      for( i = 0; i < nb; i++ )
      {
         workH[i] = history->high[i];
         workL[i] = history->low[i];
      }

      /* outFisher over the chosen input, then outTrigger over it. */
      rc = TA_FISHER( 0, nb-1, workH, workL, 10, &begIdx2, &nbElement2,
                      ( which == 0 ) ? workH : workL,
                      other );
      if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement
          || memcmp( refF, ( which == 0 ) ? workH : workL,
                     (size_t)nbElement * sizeof(double) ) != 0
          || memcmp( refT, other, (size_t)nbElement * sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_FISHER aliasing outFisher over %s\n",
                 ( which == 0 ) ? "inHigh" : "inLow" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishAliasCmp++;

      for( i = 0; i < nb; i++ )
      {
         workH[i] = history->high[i];
         workL[i] = history->low[i];
      }

      rc = TA_FISHER( 0, nb-1, workH, workL, 10, &begIdx2, &nbElement2,
                      other,
                      ( which == 0 ) ? workH : workL );
      if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement
          || memcmp( refT, ( which == 0 ) ? workH : workL,
                     (size_t)nbElement * sizeof(double) ) != 0
          || memcmp( refF, other, (size_t)nbElement * sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_FISHER aliasing outTrigger over %s\n",
                 ( which == 0 ) ? "inHigh" : "inLow" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_fishAliasCmp++;
   }

   return TA_TEST_PASS;
}

/* (7) A sub-range equals the same slice of a full run, within the convergence
 * the unstable period bounds. */
static TA_RetCode fishRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real other[FISH_NB_BAR];
   TA_RetCode rc;

   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_FISHER_Lookback( 10 );

   if( outputNb == 0 )
      rc = TA_FISHER( startIdx, endIdx, h->high, h->low, 10,
                      outBegIdx, outNbElement, outputBuffer, other );
   else
      rc = TA_FISHER( startIdx, endIdx, h->high, h->low, 10,
                      outBegIdx, outNbElement, other, outputBuffer );

   return rc;
}

static ErrorNumber test_fish_range( const TA_History *history )
{
   return doRangeTestEx( fishRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_FISHER,
                         (void *)history, 2, 0 );
}
