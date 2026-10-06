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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100626 KL,CC  First version (proposal DOSC, #479).
 */

/* Description:
 *
 *   Test TA_DOSC (Constance Brown's Derivative Oscillator, #479).
 *
 *   --codegen, --xlang-hash and server_verify compare every language against
 *   this library, so none of them can catch a wrong formula. Two things do.
 *
 *   The COMPOSITION leg is the primary one: TA_DOSC must land on the same
 *   BITS as TA_RSI followed by two TA_EMA calls and a TA_SMA, entered
 *   L1 + L2 + Lg bars early, with the smoothed line read at [k + Lg] against
 *   the average at [k]. That +Lg offset is the trap -- a gate indexing the
 *   smoothed line at [k] fails a correct implementation. The leg sweeps the
 *   two inherited unstable periods INDEPENDENTLY: moved together, a stage
 *   reading the wrong callee's lookback would still line up.
 *
 *   The GOLDEN leg holds the formula, from a 60-digit evaluation over the
 *   committed corpus. MEASURED: the library reproduces those 32 rows to
 *   8.5e-14 absolute, which is the Wilder recursion's own accumulation
 *   amplified by the cancellation in DS - SMA(DS); the tolerance is 1e-12.
 *
 *   The DEGENERATE leg pins what TA_DOSC inherits from TA_RSI when no gain
 *   and no loss has been seen. Note that a flat run alone cannot arbitrate
 *   that: DOSC is a difference, so a CONSTANT RSI gives exactly 0 whatever
 *   the constant is. It is the transient after the flat run ends that
 *   separates them, by up to 24.5 (MEASURED, bar 104), and that is what this
 *   leg pins.
 *
 *   Every comparison count is pinned, so a leg that stops comparing fails.
 *
 *   SERVER_VERIFY: the golden leg, once per parameter set.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define DOSC_NB_BAR 252

/* 4 parameter sets x 8 rows. */
#define DOSC_GOLDEN_CMP      32
/* 4 sets x 5 unstable settings x 3 startIdx values (the lookback, one past
 * it, and 40 past it), each comparing every output bar of a 252-bar corpus:
 * sum over all of them of (252 - startIdx), skipping any startIdx past bar
 * 250. Computed from the parameter table, not read back from a run. */
#define DOSC_COMPOSE_CMP  12677
/* 4 sets x 5 unstable settings, _Lookback and the call's own begIdx. */
#define DOSC_LOOKBACK_CMP    40
/* 72 bars of exact zero inside the flat run, then 15 transient rows. */
#define DOSC_DEGEN_CMP       87
/* outReal aliased onto inReal. */
#define DOSC_ALIAS_CMP        1

static int g_doscGoldenCmp;
static int g_doscComposeCmp;
static int g_doscLookbackCmp;
static int g_doscDegenCmp;
static int g_doscAliasCmp;

typedef struct
{
   int    t;
   int    f;
   int    s;
   int    g;
   int    bar;
   double value;
} DoscGolden;

/* From a 60-digit evaluation of the #479 formula over the committed corpus,
 * rounded once to 17 significant digits. The 2/2/2/2 set is the minimum-period
 * edge. The first rows of each set sit on the seeds, where a first-value EMA
 * seeding differs; by bar 100 that difference is gone.
 */
static const DoscGolden doscGolden[] =
{
   { 14, 5, 3, 9,  28,  -0.32493339246440395 },
   { 14, 5, 3, 9,  29,   1.1294804858305276  },
   { 14, 5, 3, 9,  30,   1.8411745168139129  },
   { 14, 5, 3, 9,  33,   3.3947072651925123  },
   { 14, 5, 3, 9,  40,  -2.3604690875888581  },
   { 14, 5, 3, 9, 100,  -6.7257040101749217  },
   { 14, 5, 3, 9, 180,  -4.8983277148123285  },
   { 14, 5, 3, 9, 251,  -1.3052378861986969  },
   {  7, 2, 2, 3,  11,   6.3911107376778942  },
   {  7, 2, 2, 3,  12,   4.9871202912720856  },
   {  7, 2, 2, 3,  13,  -6.9909842984477359  },
   {  7, 2, 2, 3,  16,  -2.9991129829473309  },
   {  7, 2, 2, 3,  23,  -4.6347218582406544  },
   {  7, 2, 2, 3, 100,   3.4948832639583238  },
   {  7, 2, 2, 3, 180,  -0.5742141628081755  },
   {  7, 2, 2, 3, 251,  -2.8784371609126027  },
   { 21, 9, 5, 13, 45,   2.1398319394716627  },
   { 21, 9, 5, 13, 46,   2.6849728905192842  },
   { 21, 9, 5, 13, 47,   2.9293385772958467  },
   { 21, 9, 5, 13, 50,   3.1363117509310663  },
   { 21, 9, 5, 13, 57,  -2.0959620671606891  },
   { 21, 9, 5, 13,100,  -1.8542800921709932  },
   { 21, 9, 5, 13,180,   0.73800983568677381 },
   { 21, 9, 5, 13,251,  -1.167076396328979   },
   {  2, 2, 2, 2,   5,  -3.4694854393017427  },
   {  2, 2, 2, 2,   6, -10.488256616386783   },
   {  2, 2, 2, 2,   7,  -4.5134274384189839  },
   {  2, 2, 2, 2,  10,  11.422684030342726   },
   {  2, 2, 2, 2,  17,  -2.5478249156729516  },
   {  2, 2, 2, 2, 100,   5.5705910166985504  },
   {  2, 2, 2, 2, 180,   1.1696093963642138  },
   {  2, 2, 2, 2, 251,  -7.2429724561874922  }
};

#define DOSC_NB_GOLDEN ((int)(sizeof(doscGolden)/sizeof(doscGolden[0])))

/* MEASURED: the library reproduces the rows above to 8.549e-14 absolute. */
#define DOSC_GOLDEN_TOL 1e-12

/* The flat-then-move run: bars 0 to 99 flat at 100.0, the committed closes
 * from bar 100 on. The transient rows below are a 60-digit evaluation under
 * TA_RSI's own 50.0 (#480). Under the 0.0 this card was written against they
 * read 14.81, 30.25, 38.13, ... -- a gap of up to 24.5 -- so these rows
 * arbitrate the inherited value, which the flat run itself cannot.
 */
#define DOSC_FLAT_TO 99

typedef struct { int bar; double value; } DoscRow;

static const DoscRow doscTransient[] =
{
   { 100,  7.4074074074074074 },
   { 101, 15.123456790123457  },
   { 102, 17.400517234869938  },
   { 103, 17.294263178997475  },
   { 104, 15.323978160349752  },
   { 105, 13.133320857460436  },
   { 106, 11.276874735326397  },
   { 107,  7.283203155787624  },
   { 108,  2.9282261775362377 },
   { 109, -1.071150851727855  },
   { 110, -4.0567494106766029 },
   { 111, -5.6531288705528659 },
   { 112, -6.1620498569382107 },
   { 113, -5.0366027101733319 },
   { 114, -3.6506662312682621 }
};

#define DOSC_NB_TRANSIENT ((int)(sizeof(doscTransient)/sizeof(doscTransient[0])))

static ErrorNumber test_dosc_golden  ( const TA_History *history );
static ErrorNumber test_dosc_compose ( const TA_History *history );
static ErrorNumber test_dosc_lookback( const TA_History *history );
static ErrorNumber test_dosc_degen   ( const TA_History *history );
static ErrorNumber test_dosc_aliasing( const TA_History *history );
static ErrorNumber test_dosc_range   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_dosc( TA_History *history )
{
   ErrorNumber retValue;

   g_doscGoldenCmp = 0;
   g_doscComposeCmp = 0;
   g_doscLookbackCmp = 0;
   g_doscDegenCmp = 0;
   g_doscAliasCmp = 0;

   if( history->nbBars != DOSC_NB_BAR )
   {
      printf( "Fail: TA_DOSC expects the %d-bar corpus, got %d\n",
              DOSC_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   retValue = test_dosc_golden( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_dosc_compose( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_dosc_lookback( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_dosc_degen( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_dosc_aliasing( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_dosc_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   if( g_doscGoldenCmp   != DOSC_GOLDEN_CMP
    || g_doscComposeCmp  != DOSC_COMPOSE_CMP
    || g_doscLookbackCmp != DOSC_LOOKBACK_CMP
    || g_doscDegenCmp    != DOSC_DEGEN_CMP
    || g_doscAliasCmp    != DOSC_ALIAS_CMP )
   {
      printf( "Fail: TA_DOSC comparison counts (golden %d, compose %d, "
              "lookback %d, degenerate %d, alias %d) are not what this file "
              "asserts (%d, %d, %d, %d, %d)\n",
              g_doscGoldenCmp, g_doscComposeCmp, g_doscLookbackCmp,
              g_doscDegenCmp, g_doscAliasCmp,
              DOSC_GOLDEN_CMP, DOSC_COMPOSE_CMP, DOSC_LOOKBACK_CMP,
              DOSC_DEGEN_CMP, DOSC_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* (1) GOLDEN: the formula, from 60 digits over the committed corpus. */
static ErrorNumber test_dosc_golden( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   static TA_Real out[DOSC_NB_BAR];
   int k, lastT = -1, lastF = -1;

   for( k = 0; k < DOSC_NB_GOLDEN; k++ )
   {
      const DoscGolden *gd = &doscGolden[k];
      double got, err;
      int want = gd->t + (gd->f - 1) + (gd->s - 1) + (gd->g - 1);

      rc = TA_DOSC( 0, (int)history->nbBars - 1, history->close,
                    gd->t, gd->f, gd->s, gd->g,
                    &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_DOSC golden rc=%d (%d/%d/%d/%d)\n",
                 (int)rc, gd->t, gd->f, gd->s, gd->g );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != want || gd->bar < begIdx )
      {
         printf( "Fail: TA_DOSC golden range: begIdx=%d (want %d), bar %d\n",
                 (int)begIdx, want, gd->bar );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      got = out[gd->bar - begIdx];
      err = fabs( got - gd->value );
      if( !( err <= DOSC_GOLDEN_TOL ) )
      {
         printf( "Fail: TA_DOSC golden bar %d (%d/%d/%d/%d): %.17g, expected "
                 "%.17g (abs %.3g, tol %.1e)\n",
                 gd->bar, gd->t, gd->f, gd->s, gd->g, got, gd->value,
                 err, DOSC_GOLDEN_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_doscGoldenCmp++;

      if( (gd->t != lastT || gd->f != lastF) && server_verify_active() )
      {
         const double opt[4] = { (double)gd->t, (double)gd->f,
                                 (double)gd->s, (double)gd->g };
         ErrorNumber e;

         e = server_verify( "DOSC", 0, (int)history->nbBars - 1,
                            (int)history->nbBars,
                            rc, begIdx, nbElement,
                            (const TA_Real*[]){ history->close, NULL },
                            opt, 4,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS ) return e;
         lastT = gd->t;
         lastF = gd->f;
      }
   }

   return TA_TEST_PASS;
}

/* (2) COMPOSITION, bit-exact: TA_RSI then TA_EMA then TA_EMA then TA_SMA.
 *
 * The two inherited unstable periods are swept INDEPENDENTLY. Moving them
 * together would leave a stage that reads the wrong callee's lookback lined up
 * with the chain, because the chain would shift by the same amount.
 */
static ErrorNumber test_dosc_compose( const TA_History *history )
{
   static const int tSet[] = { 14,  7, 21,  2 };
   static const int fSet[] = {  5,  2,  9,  2 };
   static const int sSet[] = {  3,  2,  5,  2 };
   static const int gSet[] = {  9,  3, 13,  2 };
   /* (unstable RSI, unstable EMA) pairs: zero, each alone, and both. */
   static const int uRsiSet[] = { 0, 3, 0, 2, 5 };
   static const int uEmaSet[] = { 0, 0, 4, 2, 1 };
   static TA_Real r[DOSC_NB_BAR], e1[DOSC_NB_BAR], e2[DOSC_NB_BAR];
   static TA_Real sg[DOSC_NB_BAR], mine[DOSC_NB_BAR];
   TA_RetCode rc;
   TA_Integer bR, nR, b1, n1, b2, n2, bS, nS, bD, nD;
   int set, u, j, k, a, startIdx, L1, L2, Lg, total;

   for( set = 0; set < 4; set++ )
   {
      for( u = 0; u < 5; u++ )
      {
         if( TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, uRsiSet[u] ) != TA_SUCCESS
          || TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uEmaSet[u] ) != TA_SUCCESS )
         {
            printf( "Fail: TA_DOSC compose could not set an unstable period\n" );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         L1 = TA_EMA_Lookback( fSet[set] );
         L2 = TA_EMA_Lookback( sSet[set] );
         Lg = TA_SMA_Lookback( gSet[set] );
         total = TA_DOSC_Lookback( tSet[set], fSet[set], sSet[set], gSet[set] );

         if( total != TA_RSI_Lookback( tSet[set] ) + L1 + L2 + Lg )
         {
            printf( "Fail: TA_DOSC_Lookback(%d,%d,%d,%d)=%d is not the sum of "
                    "its callees\n", tSet[set], fSet[set], sSet[set],
                    gSet[set], total );
            goto restore_bad;
         }

         for( j = 0; j < 3; j++ )
         {
            startIdx = total + (j == 0 ? 0 : (j == 1 ? 1 : 40));
            if( startIdx > DOSC_NB_BAR - 2 ) continue;
            a = startIdx - (L1 + L2 + Lg);

            rc = TA_DOSC( startIdx, DOSC_NB_BAR - 1, history->close,
                          tSet[set], fSet[set], sSet[set], gSet[set],
                          &bD, &nD, mine );
            if( rc != TA_SUCCESS ) goto badrc;
            if( bD != startIdx )
            {
               printf( "Fail: TA_DOSC compose begIdx=%d, expected %d\n",
                       (int)bD, startIdx );
               goto restore_bad;
            }

            rc = TA_RSI( a, DOSC_NB_BAR - 1, history->close, tSet[set],
                         &bR, &nR, r );
            if( rc != TA_SUCCESS ) goto badrc;
            if( bR != a )
            {
               printf( "Fail: TA_DOSC compose RSI begIdx=%d, expected %d\n",
                       (int)bR, a );
               goto restore_bad;
            }

            rc = TA_EMA( 0, (int)nR - 1, r, fSet[set], &b1, &n1, e1 );
            if( rc != TA_SUCCESS ) goto badrc;
            rc = TA_EMA( 0, (int)n1 - 1, e1, sSet[set], &b2, &n2, e2 );
            if( rc != TA_SUCCESS ) goto badrc;
            rc = TA_SMA( 0, (int)n2 - 1, e2, gSet[set], &bS, &nS, sg );
            if( rc != TA_SUCCESS ) goto badrc;

            if( b1 != L1 || b2 != L2 || bS != Lg || (int)nS != (int)nD )
            {
               printf( "Fail: TA_DOSC compose stage offsets %d/%d/%d (want "
                       "%d/%d/%d), %d values against %d\n",
                       (int)b1, (int)b2, (int)bS, L1, L2, Lg,
                       (int)nS, (int)nD );
               goto restore_bad;
            }

            for( k = 0; k < (int)nD; k++ )
            {
               /* The +Lg is the point: the smoothed line at the bar the
                * average ENDS on, not at the average's own index. */
               double want = e2[k + Lg] - sg[k];
               if( mine[k] != want )
               {
                  printf( "Fail: TA_DOSC compose %d/%d/%d/%d unstRSI=%d "
                          "unstEMA=%d start=%d bar %d: %.17g, chain %.17g\n",
                          tSet[set], fSet[set], sSet[set], gSet[set],
                          uRsiSet[u], uEmaSet[u], startIdx, startIdx + k,
                          mine[k], want );
                  goto restore_bad;
               }
               g_doscComposeCmp++;
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;

badrc:
   printf( "Fail: TA_DOSC compose sub-call rc=%d\n", (int)rc );
restore_bad:
   TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TESTUTIL_TFRR_BAD_CALCULATION;
}

/* (3) LOOKBACK: 28 at the defaults, +k under unst(RSI), +2k under unst(EMA).
 * The factor of two is the one easy to get wrong. */
static ErrorNumber test_dosc_lookback( const TA_History *history )
{
   static const int tSet[] = { 14,  7, 21,  2 };
   static const int fSet[] = {  5,  2,  9,  2 };
   static const int sSet[] = {  3,  2,  5,  2 };
   static const int gSet[] = {  9,  3, 13,  2 };
   static const int uRsiSet[] = { 0, 3, 0, 2, 5 };
   static const int uEmaSet[] = { 0, 0, 4, 2, 1 };
   static TA_Real out[DOSC_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int set, u, want, got;

   for( u = 0; u < 5; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, uRsiSet[u] );
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, uEmaSet[u] );

      for( set = 0; set < 4; set++ )
      {
         want = tSet[set] + uRsiSet[u]
              + (fSet[set] - 1 + uEmaSet[u])
              + (sSet[set] - 1 + uEmaSet[u])
              + (gSet[set] - 1);
         got = TA_DOSC_Lookback( tSet[set], fSet[set], sSet[set], gSet[set] );
         if( got != want )
         {
            printf( "Fail: TA_DOSC_Lookback(%d,%d,%d,%d) = %d at unstRSI=%d "
                    "unstEMA=%d, expected %d\n",
                    tSet[set], fSet[set], sSet[set], gSet[set], got,
                    uRsiSet[u], uEmaSet[u], want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_doscLookbackCmp++;

         rc = TA_DOSC( 0, (int)history->nbBars - 1, history->close,
                       tSet[set], fSet[set], sSet[set], gSet[set],
                       &begIdx, &nbElement, out );
         if( rc != TA_SUCCESS || begIdx != want )
         {
            printf( "Fail: TA_DOSC(%d,%d,%d,%d) at unstRSI=%d unstEMA=%d "
                    "answered rc=%d begIdx=%d, expected %d\n",
                    tSet[set], fSet[set], sSet[set], gSet[set],
                    uRsiSet[u], uEmaSet[u], (int)rc, (int)begIdx, want );
            TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );
            TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         g_doscLookbackCmp++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (4) DEGENERATE: what DOSC inherits from TA_RSI when no change has been seen.
 *
 * A flat run alone cannot arbitrate it -- a constant RSI makes DOSC exactly 0
 * whatever the constant is -- so this leg also pins the transient after the
 * flat run ends, where the two conventions part by up to 24.5.
 */
static ErrorNumber test_dosc_degen( const TA_History *history )
{
   static double in[DOSC_NB_BAR];
   static TA_Real out[DOSC_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, k;

   for( i = 0; i <= DOSC_FLAT_TO; i++ )
      in[i] = 100.0;
   for( i = DOSC_FLAT_TO + 1; i < DOSC_NB_BAR; i++ )
      in[i] = history->close[i];

   rc = TA_DOSC( 0, DOSC_NB_BAR - 1, in, 14, 5, 3, 9,
                 &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != 28 )
   {
      printf( "Fail: TA_DOSC degenerate rc=%d begIdx=%d\n",
              (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* Inside the flat run: exactly zero, not nearly. */
   for( i = 28; i <= DOSC_FLAT_TO; i++ )
   {
      if( out[i - begIdx] != 0.0 )
      {
         printf( "Fail: TA_DOSC flat bar %d: %.17g, expected exactly 0\n",
                 i, out[i - begIdx] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_doscDegenCmp++;
   }

   /* The transient: this is the part that pins the inherited value. */
   for( k = 0; k < DOSC_NB_TRANSIENT; k++ )
   {
      double got = out[doscTransient[k].bar - begIdx];
      double err = fabs( got - doscTransient[k].value );
      if( !( err <= DOSC_GOLDEN_TOL ) )
      {
         printf( "Fail: TA_DOSC transient bar %d: %.17g, expected %.17g "
                 "(abs %.3g, tol %.1e)\n",
                 doscTransient[k].bar, got, doscTransient[k].value,
                 err, DOSC_GOLDEN_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_doscDegenCmp++;
   }

   return TA_TEST_PASS;
}

/* (5) outReal may be inReal. */
static ErrorNumber test_dosc_aliasing( const TA_History *history )
{
   static TA_Real ref[DOSC_NB_BAR], work[DOSC_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int i, nb;

   nb = (int)history->nbBars;

   rc = TA_DOSC( 0, nb-1, history->close, 14, 5, 3, 9,
                 &begIdx, &nbElement, ref );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: TA_DOSC aliasing: the baseline call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nb; i++ )
      work[i] = history->close[i];

   rc = TA_DOSC( 0, nb-1, work, 14, 5, 3, 9, &begIdx2, &nbElement2, work );
   if( rc != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
   {
      printf( "Fail: TA_DOSC aliased onto inReal answered rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < (int)nbElement; i++ )
   {
      if( work[i] != ref[i] )
      {
         printf( "Fail: TA_DOSC aliased onto inReal differs at bar %d: "
                 "%.17g vs %.17g\n", (int)begIdx + i, work[i], ref[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   g_doscAliasCmp++;

   return TA_TEST_PASS;
}

static TA_RetCode doscRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_DOSC_Lookback( 14, 5, 3, 9 );
   return TA_DOSC( startIdx, endIdx, h->close, 14, 5, 3, 9,
                   outBegIdx, outNbElement, outputBuffer );
}

/* (6) Range sweep over BOTH inherited ids. doRangeTestEx takes one; a function
 * recursive through callees carrying different ids needs the whole set swept
 * together, or the leg whose id is left at zero never warms. */
static ErrorNumber test_dosc_range( const TA_History *history )
{
   static const TA_FuncUnstId ids[2] = { TA_FUNC_UNST_RSI, TA_FUNC_UNST_EMA };

   return doRangeTestMulti( doscRangeTestFunction,
                            TA_STABLE_CONVERGING, ids, 2,
                            (void *)history, 1, 0 );
}
