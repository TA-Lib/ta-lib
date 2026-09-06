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
 *  090426 MF,CC  First version (issue #365).
 */

/* Description:
 *
 *   Test TA_KDJ (KDJ Stochastic).
 *
 *   Legs:
 *     1. EXTERNAL ORACLES, the formula gate. Two independently written
 *        implementations, in two languages, one per moving-average family:
 *        Tulip Indicators for the Wilder default and trading-signals for the
 *        simple-average family. See kdjOracle below. Plus cross-language
 *        replay of every configuration through server_verify.
 *     2. DELEGATION pin, bitwise: K and D must be TA_STOCH's own two outputs
 *        for the same arguments, and J exactly 3K - 2D recomputed here. Both
 *        sides are the same call, so this is a drift gate on the composition
 *        and a formula gate on the J line's coefficients and sign only.
 *     3. DECOMPOSITION differential, bitwise: the Wilder family rebuilt from
 *        TA_STOCHF's raw Fast-K fed through TA_RMA twice. This is the leg that
 *        proves ma()'s RMA arm and the lookback bookkeeping, and it is run at
 *        two unstable-period settings so both smoothing hops must shift.
 *     4. EDGES: a flat series is exactly 0.0 on all three lines; a
 *        small-magnitude series is NOT zeroed (issue #253); a range shorter
 *        than the lookback succeeds with no values; and J leaves [0,100] in
 *        both directions on the reference corpus, so a future clamp cannot
 *        land unnoticed.
 *     5. In-place aliasing, bitwise, every (output, price input) pair.
 *
 *   Legs 2, 3 and 5 are built from shipped primitives, so they prove the
 *   composition, never the formula; that is leg 1's job. The frozen
 *   ta_ref_serve predates this function, so the --codegen sweep value-compares
 *   nothing for it -- server_verify in leg 1 and --xlang-hash are the
 *   cross-language gates.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define KDJ_CAP    1200
#define KDJ_SYN_NB 300

/* Leg 1. Measured worst absolute disagreement over BOTH arms, all six
 * parameter tuples below and all three lines -- 8688 values -- is 2.27e-13,
 * and it lands on a J value near zero, where 3K and 2D nearly cancel. The pair
 * is therefore sized on the absolute term (12x headroom); the relative term is
 * headroom for the two Wilder/mean recurrences at full magnitude, where the
 * measured gap is ~2e-15 relative. A relative-only bound would redden a
 * correct implementation at a J zero crossing (issue #348), and J crosses zero
 * on this corpus. */
#define KDJ_ORACLE_ABS 2e-12
#define KDJ_ORACLE_REL 1e-13

typedef enum { KDJ_ARM_TULIP = 0, KDJ_ARM_TS = 1 } KdjArm;

/* bar is the ABSOLUTE bar index; the output index is bar - begIdx. */
typedef struct { KdjArm arm; int n; int m1; int m2; int bar;
                 double k; double d; double j; } KdjGolden;

/* Goldens captured on the 252-bar TA_SREF high/low/close series
 * (TA_SREF_{high,low,close}_daily_ref_0_PRIV) at %.17g, which round-trips to
 * the same double. Neither arm has ever seen this implementation, and neither
 * was re-derived here: both were EXECUTED, on 2026-09-04, by
 * ta-lib-oracles/capture_365_kdj.py.
 *
 *  KDJ_ARM_TULIP -- Tulip Indicators 0.9.2, pinned be18abb, in C. KDJ is not a
 *    Tulip function; the arm is `ti_stoch(n, kslow=1, dperiod=1)` for the raw
 *    stochastic, then `ti_wilders(m1)` and `ti_wilders(m2)`, then 3K-2D. This
 *    is the only arm with an INDEPENDENT WILDER KERNEL: wilders.c seeds with
 *    the simple average of the first `period` values exactly as TA_RMA does,
 *    then steps (x-v)*(1/n)+v where TA_RMA steps wAlpha*x + wBeta*v.
 *  KDJ_ARM_TS -- trading-signals 8.3.0, in TypeScript, on node v22.21.1.
 *    `StochasticOscillator` computes the whole indicator including an
 *    INDEPENDENTLY IMPLEMENTED J line (`stochJ: 3 * stochK - 2 * stochD`) and
 *    smooths with a mean that re-sums its stored window every bar. It is a
 *    simple-average implementation only, which is why the five-parameter shape
 *    matters: it reaches TA_KDJ at MAType SMA and nowhere else.
 *
 * GUARD DIVERGENCE, both arms. Tulip tests `kdiff == 0.0` and trading-signals
 * `divisor === 0`, both exact, where TA_STOCH scales the test to the window's
 * own extremes (issue #253). A window whose range is sub-epsilon but non-zero
 * would divide there and answer 0.0 here. No window in this corpus is close:
 * the smallest high-low range any window reaches is 2.63 at n=5 and 7.47 at
 * n=14. Leg 4 covers the flat window instead, where all three answer 0.
 *
 * Each configuration's widest-gap bar is included, so the frozen tolerance is
 * exercised rather than merely satisfied, along with J's corpus maximum
 * (112.98) and minimum (-15.99). */
static const KdjGolden kdjOracle[] =
{
   { KDJ_ARM_TULIP,  9, 3, 3,  12,     69.314524995033992,     59.903455280756042,     88.136664423589892 },
   { KDJ_ARM_TULIP,  9, 3, 3,  13,     49.478261806741756,     56.428390789417946,     35.578003841389375 },
   { KDJ_ARM_TULIP,  9, 3, 3,  44,     71.388731802979933,     55.722583679490306,      102.7210280499592 },
   { KDJ_ARM_TULIP,  9, 3, 3,  72,     25.009800015041925,     43.071757852255089,    -11.114115659384396 },
   { KDJ_ARM_TULIP,  9, 3, 3,  78,     79.989598207792042,     63.492750354891371,      112.9832939135934 },
   { KDJ_ARM_TULIP,  9, 3, 3, 132,     88.597232194067573,     88.415962924793533,     88.959770732615624 },
   { KDJ_ARM_TULIP,  9, 3, 3, 140,     16.175641010016999,     32.256071878797371,    -15.985220727543748 },
   { KDJ_ARM_TULIP,  9, 3, 3, 192,     25.232204242108416,     26.039007675266785,     23.618597375791673 },
   { KDJ_ARM_TULIP,  9, 3, 3, 251,     44.369807149365073,     49.884860101368481,     33.339701245358256 },
   { KDJ_ARM_TULIP, 14, 3, 3,  17,     24.191284788646097,     26.791540721181562,     18.990772923575165 },
   { KDJ_ARM_TULIP, 14, 3, 3,  18,       28.7488824157317,     27.443987952698276,     31.358671341798548 },
   { KDJ_ARM_TULIP, 14, 3, 3,  75,     50.381823405127932,     43.342213599513194,     64.461043016357408 },
   { KDJ_ARM_TULIP, 14, 3, 3, 134,     86.787696196365502,     89.206340652138167,     81.950407284820159 },
   { KDJ_ARM_TULIP, 14, 3, 3, 157,     64.222888024113047,     54.678644463103574,     83.311375146131994 },
   { KDJ_ARM_TULIP, 14, 3, 3, 193,     16.455163822769975,     18.290836818157654,     12.783817831994618 },
   { KDJ_ARM_TULIP, 14, 3, 3, 251,      37.57338066924499,     34.117756127947054,      44.48462975184087 },
   { KDJ_ARM_TULIP,  9, 5, 5,  16,     42.580724680217543,     51.323673809295521,     25.094826422061587 },
   { KDJ_ARM_TULIP,  9, 5, 5,  17,     38.095647705339083,     48.678068588504232,     16.930805939008792 },
   { KDJ_ARM_TULIP,  9, 5, 5,  44,     61.422136395505113,     49.060276645119124,     86.145855896277098 },
   { KDJ_ARM_TULIP,  9, 5, 5,  75,      46.47560987203849,     49.025109268669702,     41.376611078776065 },
   { KDJ_ARM_TULIP,  9, 5, 5, 134,     79.927988283945197,     81.579814187526679,     76.624336476782247 },
   { KDJ_ARM_TULIP,  9, 5, 5, 193,      24.16659005747541,     27.168517467024422,      18.16273523837738 },
   { KDJ_ARM_TULIP,  9, 5, 5, 251,      46.72860655830425,     48.996918929084799,     42.191981816743152 },
   { KDJ_ARM_TS,     9, 3, 3,  12,     87.343612054662387,     70.166936115099858,     121.69696393378743 },
   { KDJ_ARM_TS,     9, 3, 3,  13,     58.480491420590802,     72.693358032994453,     30.054758195783506 },
   { KDJ_ARM_TS,     9, 3, 3,  72,     13.930734994385297,     23.493165192884941,    -5.1941254026139916 },
   { KDJ_ARM_TS,     9, 3, 3, 132,     89.434327734587342,     92.929507162971973,     82.443968877818094 },
   { KDJ_ARM_TS,     9, 3, 3, 192,     32.192510125921444,     35.747942069911495,     25.081646237941342 },
   { KDJ_ARM_TS,     9, 3, 3, 251,     40.561032419294492,     57.358428385233104,     6.9662404874172665 },
   { KDJ_ARM_TS,    14, 3, 3,  17,     28.647249190938506,      30.51778937357734,     24.906168825660835 },
   { KDJ_ARM_TS,    14, 3, 3,  18,     25.566343042071196,     29.048543689320383,     18.601941747572823 },
   { KDJ_ARM_TS,    14, 3, 3,  75,     50.160733121626969,     29.288760404716459,     91.904678555447987 },
   { KDJ_ARM_TS,    14, 3, 3, 127,     91.207310419099358,     92.130835675951047,     89.360259905395964 },
   { KDJ_ARM_TS,    14, 3, 3, 134,     84.920245125843891,     88.423711038116309,     77.913313301299041 },
   { KDJ_ARM_TS,    14, 3, 3, 193,     14.856479858323249,     22.063687912749756,    0.44206374947023619 },
   { KDJ_ARM_TS,    14, 3, 3, 251,     39.300932136346454,     34.507152493531613,     48.888491421976127 },
};
#define NB_KDJ_ORACLE ((int)(sizeof(kdjOracle)/sizeof(KdjGolden)))

/* Leg 2/3 grid. (2,1,1) and (9,1,3) exercise the identity hop, where ma()
 * short-circuits a period of 1. */
typedef struct { int n; int m1; int m2; } KdjTuple;
static const KdjTuple kdjGrid[] =
{
   { 9, 3, 3 }, { 5, 3, 3 }, { 14, 3, 3 }, { 9, 5, 5 }, { 2, 1, 1 }, { 9, 1, 3 },
};
#define NB_KDJ_GRID ((int)(sizeof(kdjGrid)/sizeof(KdjTuple)))

static const TA_MAType kdjMaTypes[] = { TA_MAType_RMA, TA_MAType_SMA, TA_MAType_EMA };
#define NB_KDJ_MATYPE ((int)(sizeof(kdjMaTypes)/sizeof(TA_MAType)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_kdjOracleCmp;
static int g_kdjDelegCmp;
static int g_kdjDecompCmp;
static int g_kdjEdgeCmp;
static int g_kdjAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_kdj_oracle( const TA_History *history );
static ErrorNumber test_kdj_delegation( const TA_History *history );
static ErrorNumber test_kdj_decomposition( const TA_History *history );
static ErrorNumber test_kdj_edges( const TA_History *history );
static ErrorNumber test_kdj_aliasing( const TA_History *history );

/* A small-magnitude series quoted around 1e-6, built from an exact rule so
 * nothing is transported. It pins the OUTCOME issue #253 cares about: an
 * instrument quoted far below 1.0 is not zeroed.
 *
 * It cannot discriminate a fixed-band substitution, and NO choice of magnitude
 * would let it. KDJ delegates to TA_STOCH, whose guard (stoch.c) is
 * TA_IS_ZERO_SCALED(highest-lowest, |highest|+|lowest|) -- a RELATIVE dead-zone
 * of ~90 ULP, so the threshold shrinks with the quote and the fired/not-fired
 * ratio is invariant under rescaling. Quoting at 1e-13 was tried; still green,
 * necessarily. Exposing a scaled-vs-fixed difference requires mutating the
 * guard itself, which is a mutation test, not test data. Do not "fix" this leg
 * by shrinking the series. */
static void kdjBuildSmall( double *h, double *l, double *c, int nb )
{
   int i;
   for( i = 0; i < nb; i++ )
   {
      double mid = 1.0e-6 * ( 1.0 + 0.25 * (double)( ( ( i * 37 ) % 401 ) - 200 ) / 401.0 );
      h[i] = mid * 1.01;
      l[i] = mid * 0.99;
      c[i] = mid * ( 1.0 + 0.003 * (double)( ( i % 7 ) - 3 ) );
   }
}

/**** Global functions definitions. ****/
ErrorNumber test_func_kdj( TA_History *history )
{
   ErrorNumber err;

   g_kdjOracleCmp = g_kdjDelegCmp = g_kdjDecompCmp = 0;
   g_kdjEdgeCmp = g_kdjAliasCmp = 0;

   if( (int)history->nbBars > KDJ_CAP )
   {
      printf( "KDJ Fail: %d bars exceeds this file's buffers (%d)\n",
              (int)history->nbBars, KDJ_CAP );
      return TA_KDJ_VACUOUS;
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   err = test_kdj_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kdj_delegation( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kdj_decomposition( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kdj_edges( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kdj_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( (int)history->nbBars == 252
       && ( g_kdjOracleCmp != NB_KDJ_ORACLE * 3 || g_kdjDelegCmp != 13032
            || g_kdjDecompCmp != 7089 || g_kdjEdgeCmp != 2113
            || g_kdjAliasCmp != 39096 ) )
   {
      printf( "KDJ Fail: coverage counters (oracle %d, delegation %d, "
              "decomposition %d, edges %d, alias %d) are not what this file "
              "was written with (%d, 13032, 7089, 2113, 39096)\n",
              g_kdjOracleCmp, g_kdjDelegCmp, g_kdjDecompCmp, g_kdjEdgeCmp,
              g_kdjAliasCmp, NB_KDJ_ORACLE * 3 );
      return TA_KDJ_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) The frozen two-arm goldens, plus the cross-language replay. */
static ErrorNumber test_kdj_oracle( const TA_History *history )
{
   static TA_Real K[KDJ_CAP], D[KDJ_CAP], J[KDJ_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx = 0, nbElement = 0;
   int nbBars = (int)history->nbBars;
   int k, lastRow = -1;

   if( nbBars != 252 )
   {
      printf( "KDJ oracle skip: goldens were captured on the 252-bar corpus, "
              "got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   for( k = 0; k < NB_KDJ_ORACLE; k++ )
   {
      const KdjGolden *g = &kdjOracle[k];
      TA_MAType mt = ( g->arm == KDJ_ARM_TULIP ) ? TA_MAType_RMA : TA_MAType_SMA;
      const double *want[3];
      double got[3];
      int c;

      if( lastRow < 0 || g->arm != kdjOracle[lastRow].arm
          || g->n != kdjOracle[lastRow].n || g->m1 != kdjOracle[lastRow].m1
          || g->m2 != kdjOracle[lastRow].m2 )
      {
         lastRow = k;
         retCode = TA_KDJ( 0, nbBars-1, history->high, history->low,
                           history->close, g->n, g->m1, mt, g->m2, mt,
                           &begIdx, &nbElement, K, D, J );
         if( retCode != TA_SUCCESS
             || begIdx != TA_KDJ_Lookback( g->n, g->m1, mt, g->m2, mt )
             || nbElement != nbBars - begIdx )
         {
            printf( "KDJ oracle Fail [arm %d (%d,%d,%d)]: rc=%d (%d,%d)\n",
                    (int)g->arm, g->n, g->m1, g->m2, (int)retCode,
                    begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[5];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)g->n;
            optIn[1] = (double)g->m1;
            optIn[2] = (double)mt;
            optIn[3] = (double)g->m2;
            optIn[4] = (double)mt;
            e = server_verify( "KDJ", 0, nbBars-1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low,
                                                   history->close, NULL },
                               optIn, 5,
                               (const TA_Real*[]){ K, D, J, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            /* "No failure reported" and "nothing was compared" are the same
             * observation without this. */
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "KDJ oracle [arm %d (%d,%d,%d)]: compared no server "
                       "despite live pipes\n", (int)g->arm, g->n, g->m1, g->m2 );
               return TA_KDJ_VACUOUS;
            }
         }
      }

      /* Each row's bar is transcribed from the capture, and the index below is
       * unchecked arithmetic on it: a bar outside the output is a silent
       * out-of-bounds read, not a mismatch. */
      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "KDJ oracle Fail [arm %d (%d,%d,%d)]: golden bar %d is outside "
                 "the output [%d..%d]\n", (int)g->arm, g->n, g->m1, g->m2,
                 g->bar, begIdx, begIdx + nbElement - 1 );
         return TA_KDJ_VACUOUS;
      }

      got[0] = K[g->bar - begIdx];
      got[1] = D[g->bar - begIdx];
      got[2] = J[g->bar - begIdx];
      want[0] = &g->k; want[1] = &g->d; want[2] = &g->j;

      for( c = 0; c < 3; c++ )
      {
         double err;
         const char *mode;

         g_kdjOracleCmp++;
         if( !checkOracleValue( got[c], *want[c], KDJ_ORACLE_REL,
                                KDJ_ORACLE_ABS, &err, &mode ) )
         {
            printf( "KDJ oracle Fail [arm %d (%d,%d,%d)] bar %d line %c: got "
                    "%.17g expected %.17g (%s err %.3g, tol rel %g abs %g)\n",
                    (int)g->arm, g->n, g->m1, g->m2, g->bar, "KDJ"[c],
                    got[c], *want[c], mode, err, KDJ_ORACLE_REL,
                    KDJ_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) K and D are TA_STOCH's own outputs, bitwise, and J is exactly 3K - 2D.
 *
 * Both sides reach the same call, so what this pins is the J line's
 * coefficients and sign, the outBegIdx/outNBElement agreement, and a red light
 * the day the body is re-transcribed for speed.
 */
static ErrorNumber test_kdj_delegation( const TA_History *history )
{
   static TA_Real K[KDJ_CAP], D[KDJ_CAP], J[KDJ_CAP];
   static TA_Real sK[KDJ_CAP], sD[KDJ_CAP];
   TA_Integer begIdx, nbElement, sBegIdx, sNbElement;
   TA_RetCode rc1, rc2;
   int nbBars = (int)history->nbBars;
   int g, t, i;

   for( t = 0; t < NB_KDJ_MATYPE; t++ )
   {
      TA_MAType mt = kdjMaTypes[t];

      for( g = 0; g < NB_KDJ_GRID; g++ )
      {
         const KdjTuple *p = &kdjGrid[g];

         rc1 = TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                       p->n, p->m1, mt, p->m2, mt,
                       &begIdx, &nbElement, K, D, J );
         rc2 = TA_STOCH( 0, nbBars-1, history->high, history->low, history->close,
                         p->n, p->m1, mt, p->m2, mt,
                         &sBegIdx, &sNbElement, sK, sD );

         if( rc1 != rc2 || begIdx != sBegIdx || nbElement != sNbElement )
         {
            printf( "KDJ delegation Fail [mt=%d (%d,%d,%d)]: KDJ rc=%d (%d,%d) "
                    "vs STOCH rc=%d (%d,%d)\n", (int)mt, p->n, p->m1, p->m2,
                    (int)rc1, begIdx, nbElement, (int)rc2, sBegIdx, sNbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         if( rc1 != TA_SUCCESS || nbElement <= 0 )
         {
            printf( "KDJ delegation Fail [mt=%d (%d,%d,%d)]: no output to "
                    "compare (rc=%d, nb=%d)\n", (int)mt, p->n, p->m1, p->m2,
                    (int)rc1, nbElement );
            return TA_KDJ_VACUOUS;
         }

         for( i = 0; i < nbElement; i++ )
         {
            g_kdjDelegCmp += 3;
            if( memcmp( &K[i], &sK[i], sizeof(double) ) != 0 ||
                memcmp( &D[i], &sD[i], sizeof(double) ) != 0 )
            {
               printf( "KDJ delegation Fail [mt=%d (%d,%d,%d)] out %d: K %.17g "
                       "vs %.17g, D %.17g vs %.17g\n", (int)mt, p->n, p->m1,
                       p->m2, i, K[i], sK[i], D[i], sD[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            {
               double wantJ = 3.0 * sK[i] - 2.0 * sD[i];
               if( memcmp( &J[i], &wantJ, sizeof(double) ) != 0 )
               {
                  printf( "KDJ delegation Fail [mt=%d (%d,%d,%d)] out %d: J "
                          "%.17g expected %.17g\n", (int)mt, p->n, p->m1,
                          p->m2, i, J[i], wantJ );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) The Wilder family rebuilt from shipped primitives, bitwise.
 *
 * TA_STOCHF at a Fast-D period of 1 hands back the raw stochastic untouched;
 * feeding that through TA_RMA twice is the definition KDJ's default claims.
 * Run at two unstable-period settings, because a lookback that failed to carry
 * the setting through BOTH hops would still line up at zero.
 *
 * A period of 1 is excluded once the unstable period is non-zero: ma_lookback
 * answers 0 at period <= 1 for every type while rma_lookback(1) is the
 * unstable period, so the two sides legitimately disagree on outBegIdx there.
 */
static ErrorNumber test_kdj_decomposition( const TA_History *history )
{
   static TA_Real K[KDJ_CAP], D[KDJ_CAP], J[KDJ_CAP];
   static TA_Real fk[KDJ_CAP], fd[KDJ_CAP], r1[KDJ_CAP], r2[KDJ_CAP];
   static const int unstable[] = { 0, 5 };
   TA_Integer begIdx, nbElement, b1, n1, b2, n2, b3, n3;
   int nbBars = (int)history->nbBars;
   int u, g, i;

   for( u = 0; u < (int)(sizeof(unstable)/sizeof(int)); u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, (unsigned int)unstable[u] );

      for( g = 0; g < NB_KDJ_GRID; g++ )
      {
         const KdjTuple *p = &kdjGrid[g];

         if( unstable[u] != 0 && ( p->m1 == 1 || p->m2 == 1 ) )
            continue;

         if( TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                     p->n, p->m1, TA_MAType_RMA, p->m2, TA_MAType_RMA,
                     &begIdx, &nbElement, K, D, J ) != TA_SUCCESS ||
             TA_STOCHF( 0, nbBars-1, history->high, history->low, history->close,
                        p->n, 1, TA_MAType_SMA, &b1, &n1, fk, fd ) != TA_SUCCESS ||
             TA_RMA( 0, n1-1, fk, p->m1, &b2, &n2, r1 ) != TA_SUCCESS ||
             TA_RMA( 0, n2-1, r1, p->m2, &b3, &n3, r2 ) != TA_SUCCESS )
         {
            printf( "KDJ decomposition [unst=%d (%d,%d,%d)]: a leg failed\n",
                    unstable[u], p->n, p->m1, p->m2 );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         if( nbElement != n3 || begIdx != b1 + b2 + b3 || nbElement <= 0 )
         {
            printf( "KDJ decomposition Fail [unst=%d (%d,%d,%d)]: KDJ (%d,%d) "
                    "vs compose (%d,%d)\n", unstable[u], p->n, p->m1, p->m2,
                    begIdx, nbElement, b1 + b2 + b3, n3 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbElement; i++ )
         {
            double wantK = r1[b3 + i];
            double wantJ = 3.0 * wantK - 2.0 * r2[i];

            g_kdjDecompCmp += 3;
            if( memcmp( &K[i], &wantK, sizeof(double) ) != 0 ||
                memcmp( &D[i], &r2[i], sizeof(double) ) != 0 ||
                memcmp( &J[i], &wantJ, sizeof(double) ) != 0 )
            {
               printf( "KDJ decomposition Fail [unst=%d (%d,%d,%d)] out %d: "
                       "K %.17g/%.17g D %.17g/%.17g J %.17g/%.17g\n",
                       unstable[u], p->n, p->m1, p->m2, i,
                       K[i], wantK, D[i], r2[i], J[i], wantJ );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_RMA, 0 );
   return TA_TEST_PASS;
}

/* (4) Edges. */
static ErrorNumber test_kdj_edges( const TA_History *history )
{
   static TA_Real h[KDJ_SYN_NB], l[KDJ_SYN_NB], c[KDJ_SYN_NB];
   static TA_Real K[KDJ_CAP], D[KDJ_CAP], J[KDJ_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int i, nbAbove, nbBelow, nonZero;

   /* A whole flat series: every window's range is exactly zero, so the guard
    * fires on every bar and each smoothing hop carries the zero forward. Assert
    * the exact 0.0, not a near-zero. */
   for( i = 0; i < KDJ_SYN_NB; i++ )
      h[i] = l[i] = c[i] = 42.0;

   retCode = TA_KDJ( 0, KDJ_SYN_NB-1, h, l, c, 9, 3, TA_MAType_RMA, 3,
                     TA_MAType_RMA, &begIdx, &nbElement, K, D, J );
   if( retCode != TA_SUCCESS || nbElement != KDJ_SYN_NB - 12 )
   {
      printf( "KDJ flat Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbElement; i++ )
   {
      g_kdjEdgeCmp += 3;
      if( K[i] != 0.0 || D[i] != 0.0 || J[i] != 0.0 )
      {
         printf( "KDJ flat Fail out %d: K %.17g D %.17g J %.17g, expected an "
                 "exact 0 on all three\n", i, K[i], D[i], J[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* The same shape quoted around 1e-6, where the window range is small in
    * absolute terms but nowhere near flat. A guard put against a fixed band
    * would zero this whole output (issue #253). */
   kdjBuildSmall( h, l, c, KDJ_SYN_NB );
   retCode = TA_KDJ( 0, KDJ_SYN_NB-1, h, l, c, 9, 3, TA_MAType_RMA, 3,
                     TA_MAType_RMA, &begIdx, &nbElement, K, D, J );
   if( retCode != TA_SUCCESS || nbElement <= 0 )
   {
      printf( "KDJ small-magnitude Fail: rc=%d (%d,%d)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   nonZero = 0;
   for( i = 0; i < nbElement; i++ )
   {
      g_kdjEdgeCmp++;
      if( K[i] != 0.0 )
         nonZero++;
      if( !( K[i] >= 0.0 ) || K[i] > 100.0 )
      {
         printf( "KDJ small-magnitude Fail out %d: K %.17g is outside [0,100]\n",
                 i, K[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   if( nonZero != nbElement )
   {
      printf( "KDJ small-magnitude Fail: %d of %d K values are exactly 0 -- the "
              "flat-window guard is answering for a window that is not flat\n",
              nbElement - nonZero, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* The declared defaults ARE the indicator: 9/3/3 with both hops Wilder is
    * what every published KDJ means by the name, and nothing else in this file
    * calls TA_KDJ without spelling its parameters out. */
   if( nbBars == 252 )
   {
      static TA_Real dK[KDJ_CAP], dD[KDJ_CAP], dJ[KDJ_CAP];
      TA_Integer dBegIdx, dNbElement;

      if( TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                  TA_INTEGER_DEFAULT, TA_INTEGER_DEFAULT, TA_INTEGER_DEFAULT,
                  TA_INTEGER_DEFAULT, TA_INTEGER_DEFAULT,
                  &dBegIdx, &dNbElement, dK, dD, dJ ) != TA_SUCCESS ||
          TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                  9, 3, TA_MAType_RMA, 3, TA_MAType_RMA,
                  &begIdx, &nbElement, K, D, J ) != TA_SUCCESS )
      {
         printf( "KDJ defaults Fail: a call failed\n" );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( dBegIdx != begIdx || dNbElement != nbElement || nbElement <= 0 )
      {
         printf( "KDJ defaults Fail: (%d,%d) vs (%d,%d)\n",
                 dBegIdx, dNbElement, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_kdjEdgeCmp += 3;
         if( memcmp( &dK[i], &K[i], sizeof(double) ) != 0 ||
             memcmp( &dD[i], &D[i], sizeof(double) ) != 0 ||
             memcmp( &dJ[i], &J[i], sizeof(double) ) != 0 )
         {
            printf( "KDJ defaults Fail out %d: the declared defaults are not "
                    "9/3/3 with both hops Wilder (K %.17g/%.17g)\n",
                    i, dK[i], K[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* A range shorter than the lookback succeeds and produces nothing. */
   begIdx = 123; nbElement = 456;
   retCode = TA_KDJ( 0, 10, history->high, history->low, history->close,
                     9, 3, TA_MAType_RMA, 3, TA_MAType_RMA,
                     &begIdx, &nbElement, K, D, J );
   g_kdjEdgeCmp++;
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "KDJ short-range Fail: rc=%d (%d,%d), expected TA_SUCCESS (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* J is deliberately unbounded: a future clamp would be invisible to every
    * leg above, all of which compare against sources that do not clamp either. */
   if( nbBars == 252 )
   {
      retCode = TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                        9, 3, TA_MAType_RMA, 3, TA_MAType_RMA,
                        &begIdx, &nbElement, K, D, J );
      if( retCode != TA_SUCCESS || nbElement <= 0 )
      {
         printf( "KDJ unbounded-J Fail: rc=%d (%d,%d)\n",
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      nbAbove = nbBelow = 0;
      for( i = 0; i < nbElement; i++ )
      {
         g_kdjEdgeCmp++;
         if( J[i] > 100.0 ) nbAbove++;
         if( J[i] < 0.0 )   nbBelow++;
      }
      if( nbAbove == 0 || nbBelow == 0 )
      {
         printf( "KDJ unbounded-J Fail: %d bars above 100 and %d below 0; the "
                 "reference corpus has both\n", nbAbove, nbBelow );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, bitwise, every (output, price input) pair.
 *
 * TA_STOCH elects a caller buffer as its own K scratch when the buffer aliases
 * a price input, so the pairing is not symmetric and ASan cannot see a
 * silently wrong value here.
 */
static ErrorNumber test_kdj_aliasing( const TA_History *history )
{
   static TA_Real refK[KDJ_CAP], refD[KDJ_CAP], refJ[KDJ_CAP];
   static TA_Real out[3][KDJ_CAP];
   static TA_Real in[3][KDJ_CAP];
   TA_Integer begIdx, nbElement, aBegIdx, aNbElement;
   int nbBars = (int)history->nbBars;
   int g, oi, ii, i, s;

   for( g = 0; g < NB_KDJ_GRID; g++ )
   {
      const KdjTuple *p = &kdjGrid[g];

      if( TA_KDJ( 0, nbBars-1, history->high, history->low, history->close,
                  p->n, p->m1, TA_MAType_RMA, p->m2, TA_MAType_RMA,
                  &begIdx, &nbElement, refK, refD, refJ ) != TA_SUCCESS
          || nbElement <= 0 )
      {
         printf( "KDJ aliasing [(%d,%d,%d)]: the reference call failed\n",
                 p->n, p->m1, p->m2 );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( oi = 0; oi < 3; oi++ )
      {
         for( ii = 0; ii < 3; ii++ )
         {
            TA_Real *outs[3];
            const TA_Real *ins[3];

            for( i = 0; i < nbBars; i++ )
            {
               in[0][i] = history->high[i];
               in[1][i] = history->low[i];
               in[2][i] = history->close[i];
            }
            for( s = 0; s < 3; s++ )
            {
               ins[s] = in[s];
               outs[s] = out[s];
            }
            outs[oi] = in[ii];

            if( TA_KDJ( 0, nbBars-1, ins[0], ins[1], ins[2],
                        p->n, p->m1, TA_MAType_RMA, p->m2, TA_MAType_RMA,
                        &aBegIdx, &aNbElement,
                        outs[0], outs[1], outs[2] ) != TA_SUCCESS
                || aBegIdx != begIdx || aNbElement != nbElement )
            {
               printf( "KDJ aliasing Fail [(%d,%d,%d) out %d over in %d]: "
                       "(%d,%d) vs (%d,%d)\n", p->n, p->m1, p->m2, oi, ii,
                       aBegIdx, aNbElement, begIdx, nbElement );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            for( i = 0; i < nbElement; i++ )
            {
               const TA_Real *want[3];
               want[0] = &refK[i]; want[1] = &refD[i]; want[2] = &refJ[i];

               for( s = 0; s < 3; s++ )
               {
                  g_kdjAliasCmp++;
                  if( memcmp( &outs[s][i], want[s], sizeof(double) ) != 0 )
                  {
                     printf( "KDJ aliasing Fail [(%d,%d,%d) out %d over in %d] "
                             "line %c out %d: got %.17g expected %.17g\n",
                             p->n, p->m1, p->m2, oi, ii, "KDJ"[s], i,
                             outs[s][i], *want[s] );
                     return TA_TESTUTIL_TFRR_BAD_CALCULATION;
                  }
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}
