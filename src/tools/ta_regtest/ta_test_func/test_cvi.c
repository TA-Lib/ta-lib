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
 *  090426 MF,CC  First version (issue #358).
 */

/* Description:
 *
 *   Test TA_CVI (Chaikin's Volatility).
 *
 *   Legs:
 *     1. DIFFERENTIAL against a compose over TA_SUB / TA_EMA / TA_ROCP, at
 *        memcmp exactness, over an optInTimePeriod x optInROCPeriod grid, two
 *        corpora, three anchors and two EMA unstable periods. This is the
 *        PRIMARY gate and the ONLY one that reaches
 *        optInROCPeriod != optInTimePeriod: every published implementation of
 *        CVI collapses the author's two lengths into one, so no external
 *        oracle exists for that regime. The TA_EMA call is anchored at
 *        startIdx - optInROCPeriod so both paths seed their recursion on the
 *        same bar; anchoring it anywhere else compares two different EMAs and
 *        the leg means nothing.
 *     2. EXTERNAL ORACLES: frozen rows from two independent implementations,
 *        replayed cross-language by server_verify. See cviOracle below.
 *     3. Exact-arithmetic edges at optInTimePeriod 3, where the smoothing
 *        factor is exactly 1/2 and every intermediate is a binary fraction:
 *        a geometric decay whose steady value is 100*(2^-m - 1) and so names
 *        the lag outright, and a flat lead-in where the lagged average is
 *        exactly zero with a non-zero numerator over it -- unguarded that is
 *        +-Inf, not merely NaN -- and an all-flat sweep where it is 0/0.
 *     4. Parameter rejection, pinning the declared ranges: an EMA period of 1
 *        is refused, which is what makes an explicit no-smoothing arm
 *        unnecessary here (ema.c carries one because its range admits 1).
 *     5. In-place aliasing, over inHigh and over inLow, bitwise.
 *     6. The startIdx/endIdx range sweep, CONVERGING against TA_FUNC_UNST_EMA
 *        -- CVI has no unstable period of its own but inherits EMA's through
 *        the seed, which is also the UNSTABLE_MAP row in test_codegen.c.
 *
 *   Cross-language value coverage comes from server_verify in leg 2 plus the
 *   --xlang-hash sweep; the frozen ta_ref_serve predates this function, so the
 *   --codegen value comparison cannot run for it (same situation as VHF).
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
#define CVI_CAP     3100
#define CVI_GD_NB   1000
#define CVI_MAX_N   20
#define CVI_MAX_M   20

/* Leg 2. Measured agreement of TA_CVI with the rows below is 4.6e-15 relative
 * at worst, and 8.8e-15 over every bar of the corpus whose |CVI| >= 10 at any
 * period from 2 to 40 -- so this is two decimal orders of headroom for
 * cross-platform rounding, not a measured gap. The absolute floor never
 * governs: every frozen row has |value| >= 10, where the relative term is
 * 1e-11. It is here so that adding a row near a zero crossing later cannot
 * silently turn the check into a spurious failure. CVI is a difference of two
 * nearly equal EMAs divided by one of them, so relative error there is
 * amplified without bound -- measured 1.8e-11 at a bar where |CVI| is 1.7e-3.
 */
#define CVI_ORACLE_REL 1e-12
#define CVI_ORACLE_ABS 1e-12

typedef struct { int period; int bar; double want; const char *src; } CviGolden;

/* Goldens captured by ta-lib-oracles/capture_358_cvi.py on the 252-bar TA_SREF
 * high/low series, at %.17g, which round-trips to the same double. `period` is
 * BOTH lengths (see below), `bar` is the ABSOLUTE bar index; the output index
 * is bar - begIdx.
 *
 * THREE independent implementations were driven on this exact series
 * (2026-09-04), and all three place the first output at bar 2*period-1,
 * confirming the lookback and the lag:
 *   1. pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1) -- Python,
 *      `ta.cvi`. Seeds the EMA with the average of the first `period` spreads,
 *      as TA-Lib does, but spells the step alpha*x + (1-alpha)*prev and the
 *      ratio (100*(a-b))/b, so it is close rather than bit-identical.
 *   2. Tulip Indicators 0.9.2, pinned be18abb -- C, `ti_cvi`.
 *   3. trading-signals 8.3.0 -- TypeScript, `ts.CVI`.
 *
 * Rows tagged "tulip" come from arm 2, the rest from arm 1. DO NOT move a
 * tulip row below bar 200 and DO NOT "fix" the seed toward it: arms 2 and 3
 * seed the EMA from the first RAW spread, and that error decays only
 * geometrically. Measured against arm 1 at period 10 it is 1.1e-01 at bar 19,
 * 3.0e-04 at bar 40, 3.2e-11 at bar 120 and 3.9e-16 only by bar 220; at period
 * 20 it is still 2.7e-10 at bar 220, which is why no period-20 tulip row
 * appears here.
 *
 * All three arms expose ONE length driving both the average and the lag, so
 * every row is optInROCPeriod == optInTimePeriod. Nothing external reaches the
 * two-length regime; leg 1 is what covers it.
 *
 * Rows are picked at fixed fractions of the output, walked forward to the
 * first bar with |CVI| >= 10 -- away from the zero crossings where a relative
 * tolerance stops meaning anything.
 */
static const CviGolden cviOracle[] =
{
   {   5,  10,      30.701973068571505, "pandas" },
   {   5,  69,     -16.341345605249501, "pandas" },
   {   5, 134,     -15.251732177830609, "pandas" },
   {   5, 192,      16.163394342965436, "pandas" },
   {   5, 251,     -10.949893057533124, "tulip"  },
   {  10,  24,     -13.173401413075892, "pandas" },
   {  10,  77,      65.814635360909833, "pandas" },
   {  10, 136,      14.784662183201126, "pandas" },
   {  10, 194,     -13.457467647868086, "pandas" },
   {  10, 251,     -43.716570243931827, "tulip"  },
   {  14,  27,      14.959797363727267, "pandas" },
   {  14,  83,      35.260809029845646, "pandas" },
   {  14, 148,      24.307487614216186, "pandas" },
   {  14, 201,      14.290070182393253, "pandas" },
   {  14, 251,     -39.788754933958579, "tulip"  },
   {  20,  40,     -10.612717758682335, "pandas" },
   {  20,  92,      32.219268118518038, "pandas" },
   {  20, 148,      11.893893605355673, "pandas" },
   {  20, 198,       14.07452895802758, "pandas" },
};
#define NB_CVI_ORACLE ((int)(sizeof(cviOracle)/sizeof(CviGolden)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
/* Literal expected counts, from the run this file was written against. */
#define CVI_DIFF_EXPECTED   2293680
#define CVI_EDGE_EXPECTED   12957
#define CVI_ALIAS_EXPECTED  920360

static int g_cviDiffCmp;
static int g_cviOracleCmp;
static int g_cviEdgeCmp;
static int g_cviAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_cvi_differential( const char *tag, const TA_Real *high,
                                          const TA_Real *low, int nbBars );
static ErrorNumber test_cvi_oracle( const TA_History *history );
static ErrorNumber test_cvi_edges( void );
static ErrorNumber test_cvi_param_reject( const TA_History *history );
static ErrorNumber test_cvi_aliasing( const char *tag, const TA_Real *high,
                                      const TA_Real *low, int nbBars );
static ErrorNumber test_cvi_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_cvi( TA_History *history )
{
   ErrorNumber err;
   int nbBars = (int)history->nbBars;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_cviDiffCmp = g_cviOracleCmp = g_cviEdgeCmp = g_cviAliasCmp = 0;

   err = test_cvi_differential( "TA_SREF", history->high, history->low, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_differential( "gData", gDataHigh, gDataLow, CVI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_edges();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_param_reject( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_aliasing( "TA_SREF", history->high, history->low, nbBars );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_aliasing( "gData", gDataHigh, gDataLow, CVI_GD_NB );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cvi_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( nbBars == 252
       && ( g_cviDiffCmp != CVI_DIFF_EXPECTED || g_cviOracleCmp != NB_CVI_ORACLE
            || g_cviEdgeCmp != CVI_EDGE_EXPECTED
            || g_cviAliasCmp != CVI_ALIAS_EXPECTED ) )
   {
      printf( "CVI Fail: coverage counters (diff %d, oracle %d, edges %d, "
              "alias %d) are not what this file was written with "
              "(%d, %d, %d, %d)\n",
              g_cviDiffCmp, g_cviOracleCmp, g_cviEdgeCmp, g_cviAliasCmp,
              CVI_DIFF_EXPECTED, NB_CVI_ORACLE, CVI_EDGE_EXPECTED,
              CVI_ALIAS_EXPECTED );
      return TA_CVI_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) TA_CVI against TA_SUB -> TA_EMA -> TA_ROCP, memcmp-exact.
 *
 * The fused body keeps TA_EMA's seed accumulation and recursion and TA_ROCP's
 * ratio and zero guard verbatim, so the two paths are the same arithmetic in
 * the same order and the only admissible difference is none at all.
 *
 * Anchoring is the whole subtlety. The EMA seeds at
 * startIdx - lookbackTotal in the fused body, so the reference EMA must be
 * asked for a range starting at startIdx - optInROCPeriod, whose own clamp
 * lands on that same bar. Ask for it at 0 instead and the two seed on
 * different bars for every startIdx above the lookback; the values then differ
 * by a converging amount and the leg silently degrades into a tolerance
 * argument.
 */
static ErrorNumber test_cvi_differential( const char *tag, const TA_Real *high,
                                          const TA_Real *low, int nbBars )
{
   static TA_Real spread[CVI_CAP], emaBuf[CVI_CAP], rocBuf[CVI_CAP], out[CVI_CAP];
   TA_Integer begS, nbS, begE, nbE, begR, nbR, begIdx, nbElement;
   TA_RetCode retCode;
   int n, m, a, k, unst, startIdx, lookbackTotal;
   static const int UNST[2] = { 0, 4 };

   if( TA_SUB( 0, nbBars-1, high, low, &begS, &nbS, spread ) != TA_SUCCESS
       || begS != 0 || nbS != nbBars )
   {
      printf( "CVI differential [%s]: TA_SUB reference failed\n", tag );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( unst = 0; unst < 2; unst++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (unsigned int)UNST[unst] );

      for( n = 2; n <= CVI_MAX_N; n++ )
      {
         for( m = 1; m <= CVI_MAX_M; m++ )
         {
            lookbackTotal = TA_CVI_Lookback( n, m );

            for( a = 0; a < 3; a++ )
            {
               startIdx = ( a == 0 ) ? 0
                        : ( a == 1 ) ? lookbackTotal + 11
                                     : nbBars / 2;
               if( startIdx < lookbackTotal )
                  startIdx = lookbackTotal;
               if( startIdx > nbBars-1 )
                  continue;

               if( TA_EMA( startIdx-m, nbBars-1, spread, n,
                           &begE, &nbE, emaBuf ) != TA_SUCCESS
                   || begE != startIdx-m )
               {
                  printf( "CVI differential [%s N=%d M=%d K=%d start=%d]: "
                          "reference TA_EMA anchored at %d, expected %d\n",
                          tag, n, m, UNST[unst], startIdx, begE, startIdx-m );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }
               if( TA_ROCP( 0, nbE-1, emaBuf, m, &begR, &nbR, rocBuf ) != TA_SUCCESS
                   || begR != m || nbR != nbE-m )
               {
                  printf( "CVI differential [%s N=%d M=%d K=%d start=%d]: "
                          "reference TA_ROCP (%d,%d), expected (%d,%d)\n",
                          tag, n, m, UNST[unst], startIdx, begR, nbR, m, nbE-m );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }

               retCode = TA_CVI( startIdx, nbBars-1, high, low, n, m,
                                 &begIdx, &nbElement, out );
               if( retCode != TA_SUCCESS || begIdx != startIdx || nbElement != nbR )
               {
                  printf( "CVI differential Fail [%s N=%d M=%d K=%d start=%d]: "
                          "rc=%d (%d,%d) expected (%d,%d)\n",
                          tag, n, m, UNST[unst], startIdx, (int)retCode,
                          begIdx, nbElement, startIdx, nbR );
                  return TA_TESTUTIL_TFRR_BAD_BEGIDX;
               }

               for( k = 0; k < nbElement; k++ )
               {
                  double want = 100.0 * rocBuf[k];

                  g_cviDiffCmp++;
                  if( memcmp( &want, &out[k], sizeof(double) ) != 0 )
                  {
                     printf( "CVI differential Fail [%s N=%d M=%d K=%d start=%d] "
                             "out %d: got %.17g expected %.17g -- the compose is "
                             "the same arithmetic in the same order, so this is "
                             "exact or it is wrong\n",
                             tag, n, m, UNST[unst], startIdx, k, out[k], want );
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

/* (2) The frozen oracle rows, plus the cross-language replay. */
static ErrorNumber test_cvi_oracle( const TA_History *history )
{
   static TA_Real out[CVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "CVI oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_CVI_ORACLE; k++ )
   {
      double got, err;
      const char *mode;

      if( cviOracle[k].period != lastPeriod )
      {
         lastPeriod = cviOracle[k].period;
         retCode = TA_CVI( 0, nbBars-1, history->high, history->low,
                           lastPeriod, lastPeriod, &begIdx, &nbElement, out );
         /* All three arms put the first value at 2*period-1 when the two
          * lengths are equal. That is the only structural claim they can
          * make jointly, and it is worth more than any single value. */
         if( retCode != TA_SUCCESS || begIdx != 2*lastPeriod - 1
             || nbElement != nbBars - begIdx )
         {
            printf( "CVI oracle Fail [N=M=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, begIdx, nbElement,
                    2*lastPeriod - 1, nbBars - (2*lastPeriod - 1) );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[2];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            optIn[1] = (double)lastPeriod;
            e = server_verify( "CVI", 0, nbBars-1, nbBars,
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
               printf( "CVI oracle [N=M=%d]: compared no server despite live "
                       "pipes\n", lastPeriod );
               return TA_CVI_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture script, and the
       * index below is unchecked arithmetic on it: a bar above the anchor is a
       * silent out-of-bounds read, not a mismatch. */
      if( cviOracle[k].bar < begIdx || cviOracle[k].bar - begIdx >= nbElement )
      {
         printf( "CVI oracle Fail [N=M=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", cviOracle[k].period, cviOracle[k].bar,
                 begIdx, begIdx + nbElement - 1 );
         return TA_CVI_VACUOUS;
      }

      got = out[cviOracle[k].bar - begIdx];
      g_cviOracleCmp++;
      if( !checkOracleValue( got, cviOracle[k].want, CVI_ORACLE_REL,
                             CVI_ORACLE_ABS, &err, &mode ) )
      {
         printf( "CVI oracle Fail [%s N=M=%d] at bar %d: got %.17g expected "
                 "%.17g (%s err %.3g, tol rel %g abs %g)\n",
                 cviOracle[k].src, cviOracle[k].period, cviOracle[k].bar, got,
                 cviOracle[k].want, mode, err, CVI_ORACLE_REL, CVI_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) Exact-arithmetic edges.
 *
 * At optInTimePeriod 3 the smoothing factor is exactly 1/2, so with a spread
 * series of binary fractions every EMA value, every difference and every
 * quotient below is exact and the assertions are equalities, not tolerances.
 *
 *   - DECAY. The spread is 8 for ten bars, then 0 forever. The seed is the
 *     average of the first three, so E is 8 through bar 9 and then halves:
 *     E(t) = 2^(12-t). The steady output is therefore 100*(2^-m - 1), a value
 *     that names the lag outright -- -75 at m=2, -87.5 at m=3, -93.75 at m=4.
 *     An off-by-one ring, which is the failure this indicator invites, lands
 *     on the neighbouring value and cannot pass.
 *   - FLAT LEAD-IN. The spread is 0 for fifty bars, then 8. Bars 50 to 52 have
 *     a NON-ZERO numerator over an exactly zero lagged average: unguarded that
 *     is +Inf, a louder break than the 0/0 the all-flat case gives. Bar 53 is
 *     exactly 87.5, which pins the seed convention -- an EMA seeded from the
 *     first raw spread reaches a different value there.
 *   - ALL FLAT. high == low throughout, at every period pair: exactly 0.0 and
 *     never NaN.
 */
static ErrorNumber test_cvi_edges( void )
{
   static TA_Real high[512], low[512], out[CVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, m, n, t, nb;

   /* -- DECAY -- */
   nb = 220;
   for( i = 0; i < nb; i++ )
   {
      high[i] = 100.0;
      low[i]  = ( i < 10 ) ? 92.0 : 100.0;
   }

   for( m = 2; m <= 8; m++ )
   {
      double steady = 100.0 * ( ( 1.0 - ldexp( 1.0, m ) ) / ldexp( 1.0, m ) );

      retCode = TA_CVI( 0, nb-1, high, low, 3, m, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 2+m || nbElement != nb - (2+m) )
      {
         printf( "CVI decay Fail [M=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                 m, (int)retCode, begIdx, nbElement, 2+m, nb - (2+m) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( t = begIdx; t <= 200; t++ )
      {
         double want;

         if( t <= 9 )
            want = 0.0;
         else if( t <= 9+m )
            want = 100.0 * ( ( ldexp( 1.0, 12-t ) - 8.0 ) / 8.0 );
         else
            want = steady;

         g_cviEdgeCmp++;
         if( memcmp( &want, &out[t-begIdx], sizeof(double) ) != 0 )
         {
            printf( "CVI decay Fail [M=%d] bar %d: got %.17g, expected exactly "
                    "%.17g. The spread halves every bar, so the steady value is "
                    "100*(2^-M - 1) and it names the lag: a ring off by one "
                    "reports the neighbouring M\n",
                    m, t, out[t-begIdx], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* -- FLAT LEAD-IN -- */
   nb = 100;
   for( i = 0; i < nb; i++ )
   {
      high[i] = 100.0;
      low[i]  = ( i < 50 ) ? 100.0 : 92.0;
   }

   retCode = TA_CVI( 0, nb-1, high, low, 3, 3, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != nb-5 )
   {
      printf( "CVI flat lead-in Fail: rc=%d (%d,%d) expected (5,%d)\n",
              (int)retCode, begIdx, nbElement, nb-5 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( t = 5; t <= 53; t++ )
   {
      double want = ( t <= 52 ) ? 0.0 : 87.5;

      g_cviEdgeCmp++;
      if( memcmp( &want, &out[t-begIdx], sizeof(double) ) != 0 )
      {
         printf( "CVI flat lead-in Fail bar %d: got %.17g, expected exactly "
                 "%.17g (bars 50..52 divide a non-zero numerator by an exactly "
                 "zero lagged average -- +Inf without the guard; bar 53 pins "
                 "the averaged seed)\n", t, out[t-begIdx], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* -- ALL FLAT -- */
   nb = 100;
   for( i = 0; i < nb; i++ )
      high[i] = low[i] = 42.0;

   for( n = 2; n <= 12; n++ )
   {
      for( m = 1; m <= 12; m++ )
      {
         retCode = TA_CVI( 0, nb-1, high, low, n, m, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != n-1+m )
         {
            printf( "CVI all-flat Fail [N=%d M=%d]: rc=%d (%d,%d)\n",
                    n, m, (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_cviEdgeCmp++;
            if( isnan( out[i] ) || out[i] != 0.0 )
            {
               printf( "CVI all-flat Fail [N=%d M=%d] out %d: %.17g, expected "
                       "exactly 0.0 (NaN => the zero-denominator guard is "
                       "missing)\n", n, m, i, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Parameter rejection.
 *
 * The declared EMA range starts at 2, and that is load-bearing rather than
 * cosmetic: at a period of 1 the recursion reduces to (x-prev)+prev, which is
 * NOT a copy of x once consecutive values leave a factor of two of each other.
 * ema.c carries an explicit copy arm because its own range admits 1; CVI's
 * does not, so it needs none -- and this leg is what keeps the two facts tied
 * together.
 */
static ErrorNumber test_cvi_param_reject( const TA_History *history )
{
   static TA_Real out[CVI_CAP];
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k;
   static const int BAD[][2] = { {1,10}, {0,10}, {-2,10}, {10,0}, {10,-1},
                                 {100001,10}, {10,100001} };

   for( k = 0; k < (int)(sizeof(BAD)/sizeof(BAD[0])); k++ )
   {
      if( TA_CVI( 0, nbBars-1, history->high, history->low,
                  BAD[k][0], BAD[k][1], &begIdx, &nbElement, out ) != TA_BAD_PARAM )
      {
         printf( "CVI param Fail: (N=%d,M=%d) was accepted; the EMA period "
                 "starts at 2 and the ROC period at 1\n",
                 BAD[k][0], BAD[k][1] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( TA_CVI_Lookback( BAD[k][0], BAD[k][1] ) != -1 )
      {
         printf( "CVI param Fail: TA_CVI_Lookback(%d,%d) answered a number for "
                 "a pair the call rejects\n", BAD[k][0], BAD[k][1] );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   if( TA_CVI( 0, nbBars-1, history->high, history->low, 2, 1,
               &begIdx, &nbElement, out ) != TA_SUCCESS || begIdx != 2 )
   {
      printf( "CVI param Fail: the smallest legal pair (2,1) was rejected\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, bitwise, over each price component in turn.
 *
 * Safe by construction -- the lookback clamp puts every store at least
 * optInROCPeriod bars behind the bar being read -- but it is the leg that
 * would catch a store landing under a later read, and VHF's caught one.
 */
static ErrorNumber test_cvi_aliasing( const char *tag, const TA_Real *high,
                                      const TA_Real *low, int nbBars )
{
   static TA_Real clean[CVI_CAP], aliasHigh[CVI_CAP], aliasLow[CVI_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int n, m, i;

   for( n = 2; n <= CVI_MAX_N; n++ )
   {
      for( m = 1; m <= CVI_MAX_M; m++ )
      {
         retCode = TA_CVI( 0, nbBars-1, high, low, n, m,
                           &begIdx, &nbElement, clean );
         if( retCode != TA_SUCCESS )
         {
            printf( "CVI alias Fail [%s N=%d M=%d]: rc=%d\n",
                    tag, n, m, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbBars; i++ )
         {
            aliasHigh[i] = high[i];
            aliasLow[i]  = low[i];
         }

         retCode = TA_CVI( 0, nbBars-1, aliasHigh, aliasLow, n, m,
                           &begIdx2, &nbElement2, aliasHigh );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "CVI alias Fail [%s N=%d M=%d over inHigh]: rc=%d shape "
                    "(%d,%d) vs (%d,%d)\n", tag, n, m, (int)retCode,
                    begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_cviAliasCmp++;
            if( memcmp( &clean[i], &aliasHigh[i], sizeof(double) ) != 0 )
            {
               printf( "CVI alias Fail [%s N=%d M=%d over inHigh] out %d: "
                       "separate %.17g, in-place %.17g -- a store landed under "
                       "a read a later bar still needed\n",
                       tag, n, m, i, clean[i], aliasHigh[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         for( i = 0; i < nbBars; i++ )
         {
            aliasHigh[i] = high[i];
            aliasLow[i]  = low[i];
         }

         retCode = TA_CVI( 0, nbBars-1, aliasHigh, aliasLow, n, m,
                           &begIdx2, &nbElement2, aliasLow );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "CVI alias Fail [%s N=%d M=%d over inLow]: rc=%d shape "
                    "(%d,%d) vs (%d,%d)\n", tag, n, m, (int)retCode,
                    begIdx2, nbElement2, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_cviAliasCmp++;
            if( memcmp( &clean[i], &aliasLow[i], sizeof(double) ) != 0 )
            {
               printf( "CVI alias Fail [%s N=%d M=%d over inLow] out %d: "
                       "separate %.17g, in-place %.17g\n",
                       tag, n, m, i, clean[i], aliasLow[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The startIdx/endIdx range sweep. CVI seeds an EMA at its own lookback,
 * so it converges across startIdx rather than matching: TA_STABLE_CONVERGING
 * with TA_FUNC_UNST_EMA, the id CVI is also mapped to in test_codegen.c's
 * UNSTABLE_MAP. doRangeTestEx cross-checks the pair -- CONVERGING with
 * TA_TEST_UNST_NONE is a stability mismatch. */
typedef struct { int period; int rocPeriod; const TA_Real *high; const TA_Real *low; } CviRangeParam;

static TA_RetCode cviRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   CviRangeParam *p = (CviRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CVI_Lookback( p->period, p->rocPeriod );
   return TA_CVI( startIdx, endIdx, p->high, p->low, p->period, p->rocPeriod,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_cvi_range( const TA_History *history )
{
   CviRangeParam param;

   param.period    = 10;
   param.rocPeriod = 14;
   param.high      = history->high;
   param.low       = history->low;

   return doRangeTestEx( cviRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_EMA,
                         (void *)&param, 1, 0 );
}
