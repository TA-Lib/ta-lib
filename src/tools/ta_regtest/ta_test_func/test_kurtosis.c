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
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #72).
 *  092126 MF,CC  External-oracle goldens; rebuild and flat legs (issue #433).
 *
 */

/* Description:
 *     Test TA_KURTOSIS, sample-adjusted Fisher excess kurtosis (G2).
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generator input and
 *     regenerating: the first leg to go red and, with it skipped, the next.
 *     "Only" means skipping it leaves every leg green.
 *
 *       coefA denominator (n-1) -> n              leg 1, then 3
 *       residue correction dropped                leg 1, then 3
 *       no fourth-moment trigger, or at 1e-4      leg 1, then 7
 *       no trigger at all, period only            leg 1, then 7
 *       triggers against the current sums         leg 10, then 11
 *       no second-moment trigger, or at 1e-4      leg 15 only
 *       no re-anchor after the mean rebuild       leg 9, then 13
 *       re-anchor on the fourth moment alone      leg 9 only
 *       no periodic rebuild, or every 256n        leg 14 only
 *       degenerate window returns 0               leg 4, then 9
 *       var.c's triggers, rebuild every n/4       legs 9 to 13, each alone
 *
 *     No leg sees re-anchoring on the window's newest value instead of the
 *     residue, a fourth-moment trigger at 1e-3, or a period of n/4: each moves
 *     cost or the last bits, not a value by 1e-9.
 *
 *     Leg 1 carries several defects because the progression identity
 *     constrains the two coefficients against each other AND the shift's
 *     staleness at once. That is also why it is worth more than a golden: a
 *     golden only says "not what it was", this says "not the estimator".
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
#define KURT_CAP 1100
#define KURT_FRESH_CAP 6000

/* The legs below diff their own corpora against the language servers
 * bit-for-bit (issue #427). Without it every vector in this file is checked
 * against in-process C alone -- the arithmetic-progression identity, the
 * 60-digit goldens and the near-degenerate set are written specifically to
 * break this function, and a port that used the integer coefficient forms
 * (which overflow) or a different reseed period would satisfy every assertion
 * here while disagreeing with C.
 *
 * MEASURED that this compares rather than merely runs -- see the commit that
 * added it: handing the servers a different period than C used fails with
 * "SV FAIL [KURTOSIS] ... BITWISE mismatch vs in-process C".
 *
 * The period<4 leg is deliberately NOT wrapped: it asserts TA_BAD_PARAM, and
 * server_verify() skips reject cases by design, so the floor below would fire
 * on a leg that is working exactly as intended.
 */
#define KURT_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "KURTOSIS", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),             \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         /* "Returned PASS" and "compared nothing" are otherwise the same        \
          * observation. Every site below is a success case.  */                 \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "KURTOSIS oracle [period %d]: server_verify compared no "    \
                    "server despite live pipes\n", (int)(period) );              \
            return TA_KURTOSIS_VACUOUS;                                          \
         }                                                                       \
      }                                                                          \
   } while(0)

/* The exact-arithmetic identity leg 1 rests on.
 *
 * For n values in arithmetic progression the discrete-uniform population
 * kurtosis is g2 = -6(n^2+1)/(5(n^2-1)). Substituting it into
 * G2 = [(n-1)/((n-2)(n-3))] * [(n+1)g2 + 6] gives a numerator of
 * -6(n-2)(n-3) over a denominator of 5(n-2)(n-3): the n-dependence cancels
 * exactly and every progression, at every length and every spacing, has
 * excess kurtosis -6/5. Nothing external is consulted, and a wrong
 * coefficient moves the answer by O(1/n) -- far outside the tolerance.
 */
#define KURT_PROGRESSION (-1.2)

/* kurtBenign: 40 bars */
static const double kurtBenign[] = {
   100.0, 100.25, 100.5, 100.125,
   99.75, 99.5, 99.875, 100.375,
   101.0, 101.5, 101.25, 100.75,
   100.5, 100.25, 100.0, 99.5,
   99.0, 98.75, 99.25, 100.0,
   112.0, 100.5, 100.25, 100.0,
   99.875, 99.75, 99.625, 99.5,
   99.5, 99.5, 99.5, 99.5,
   99.5, 99.75, 100.25, 101.0,
   102.0, 103.5, 105.5, 108.0,
};
/* period 10: 31 values; scipy 1.15.3 differs from the 60-digit reference at bar 9 by 8.22e-14 relative */
static const double kurtBenign_g2_10[] = {
   0.6482205399083414, -0.8816072588500443, -1.107521252571008,
   -1.107521252571008, -1.0168714998943365, -0.7775047258979206,
   -0.7775047258979206, -0.22568255461919157, -1.0187074829931972,
   -1.2335393711367912, -0.8475573064056762, 9.256930592297806,
   9.341830620802492, 9.405005759562547, 9.446429672329058,
   9.459034555889948, 9.464346012906676, 9.56317970094897,
   9.760662166066723, 9.809810251124867, 9.788599860715935,
   0.027177349374474004, 0.4779993558054533, 0.2571428571428571,
   -0.3936640054357058, 5.241042805274473, 4.525870285221228,
   2.662284493719168, 1.9120786353315609, 1.2672326730673418,
   0.6783972260217559,
};
/* period 20: 21 values; scipy 1.15.3 differs from the 60-digit reference at bar 19 by 1.50e-14 relative */
static const double kurtBenign_g2_20[] = {
   -0.15125214370233347, 16.997367238867056, 16.956117509081388,
   16.997367238867056, 16.996536660159897, 17.014937702450045,
   17.079555019736922, 17.03231950337341, 16.975516979193134,
   17.19475491858422, 17.87812665102518, 18.457808808413514,
   18.746412883040506, 18.937243337549692, 19.044354102785295,
   18.97082202167872, 18.460266518744998, 17.234158037516167,
   14.45765249177646, 9.615968694353265, 4.676997147652225,
};

/* kurtNearDegenerate: 20 bars */
static const double kurtNearDegenerate[] = {
   1048576.0, 1048576.0000000002, 1048576.0, 1048576.0000000005,
   1048576.0000000002, 1048576.0000000007, 1048576.0, 1048576.0000000002,
   1048576.0000000005, 1048576.0, 1048576.0000000002, 1048576.0000000002,
   1048576.000000001, 1048576.0, 1048576.0000000005, 1048576.0000000002,
   1048576.0, 1048576.0000000007, 1048576.0000000002, 1048576.0000000005,
};
/* period 8: 13 values; scipy 1.15.3 differs from the 60-digit reference at bar 7 by 3.75e+00 relative */
static const double kurtNearDegenerate_g2_8[] = {
   0.35, -0.448, -0.9886927196984725,
   -0.448, 0.8404628099173553, -0.22857142857142856,
   2.5706475633895747, 1.6517673130193906, 1.6517673130193906,
   2.5706475633895747, -0.22857142857142856, -0.22857142857142856,
   -0.5644996347699051,
};

/* Leg 8. Captured on the 252-bar TA_SREF close series at %.17g, by
 * ta-lib-oracles/capture_433_kurtosis.py. `bar` is the ABSOLUTE bar index.
 *
 *   exact : G2 in rational arithmetic over the exact doubles, rounded to the
 *           nearest double.
 *   scipy : scipy 1.18.1 `scipy.stats.kurtosis(window, fisher=True,
 *           bias=False)` (numpy 2.5.1).
 *   pta   : pandas-ta-classic 0.6.52 `ta.kurtosis(close, length=n)` (pandas
 *           3.0.3), with talib blocked from import.
 *
 * Per period: the first three output bars, bars 100, 200 and 251, both
 * extremes, and the bars where this body, scipy, pandas-ta-classic and pandas
 * `rolling(n).kurt()` each sit furthest from exact. The pandas rolling arm is
 * checked but not frozen: its own online moments sit 1e-5 from exact at
 * period 4.
 *
 * The distance is |got - want| / max(1, |want|): G2 crosses zero, where a
 * relative distance is unbounded. Against the exact column this body's worst
 * over every bar at every period from 4 to 252 is 8.1e-13.
 */
#define KURT_EXACT_TOL  2e-12
#define KURT_ORACLE_TOL 1e-11

typedef struct { int period; int bar; double exact; double scipy; double pta; } KurtGolden;

static const KurtGolden kurtGoldens[] =
{
   /* period 4: outBegIdx 3, outNBElement 249 */
   {   4,   3,        3.348875844038774,
                      3.3488758440386235,       3.3488758440386235 },
   {   4,   4,     -0.71843862529093439,
                    -0.71843862529125069,     -0.71843862529125069 },
   {   4,   5,      0.56347346247207131,
                     0.56347346247207319,      0.56347346247207319 },
   {   4, 100,      -3.7033543758329346,
                     -3.7033543758329426,      -3.7033543758329426 },
   {   4, 114,      -5.7476122115659516,
                     -5.7476122115659498,      -5.7476122115659498 },
   {   4, 120,      0.36565212879794012,
                     0.36565212879794018,      0.36565212879794018 },
   {   4, 197,     -0.56890240512588353,
                    -0.56890240512588264,     -0.56890240512588264 },
   {   4, 200,       3.6064721095405785,
                      3.6064721095405794,       3.6064721095405794 },
   {   4, 202,       3.9990100363168248,
                      3.9990100363168217,       3.9990100363168217 },
   {   4, 251,        1.028156465867746,
                      1.0281564658677453,       1.0281564658677453 },
   /* period 10: outBegIdx 9, outNBElement 243 */
   {  10,   9,     -0.60119690269995296,
                    -0.60119690269992843,     -0.60119690269992843 },
   {  10,  10,      0.39803823580233955,
                     0.39803823580238173,      0.39803823580238173 },
   {  10,  11,     0.023758462902016296,
                    0.023758462902017108,     0.023758462902017996 },
   {  10,  85,      0.81119612986511525,
                     0.81119612986517842,      0.81119612986517797 },
   {  10, 100,    -0.055310821016923167,
                   -0.055310821016939737,    -0.055310821016940181 },
   {  10, 177,      0.61267978173619531,
                     0.61267978173622284,       0.6126797817362224 },
   {  10, 200,     -0.98238016614270829,
                    -0.98238016614272139,     -0.98238016614272139 },
   {  10, 206,      -2.3915251638991162,
                     -2.3915251638991157,      -2.3915251638991162 },
   {  10, 210,       5.1286464453190543,
                      5.1286464453190774,       5.1286464453190783 },
   {  10, 250,     -0.56755481247728123,
                     -0.5675548124772849,     -0.56755481247728534 },
   {  10, 251,      -1.2409111071262529,
                     -1.2409111071262697,      -1.2409111071262697 },
   /* period 30: outBegIdx 29, outNBElement 223 */
   {  30,  29,     -0.55281054811689501,
                    -0.55281054811689057,     -0.55281054811689101 },
   {  30,  30,     -0.75644168783549159,
                    -0.75644168783548871,     -0.75644168783548915 },
   {  30,  31,     -0.70561016782388253,
                    -0.70561016782388331,     -0.70561016782388286 },
   {  30,  41,       2.2710484962427251,
                      2.2710484962427273,       2.2710484962427286 },
   {  30,  71,     -0.28224102734279205,
                    -0.28224102734277245,     -0.28224102734277201 },
   {  30, 100,      0.14725020229761809,
                     0.14725020229762054,      0.14725020229762054 },
   {  30, 151,      -1.2055508090951743,
                     -1.2055508090951719,      -1.2055508090951728 },
   {  30, 167,      0.15702293133497761,
                     0.15702293133497847,      0.15702293133497847 },
   {  30, 200,     -0.97703398286946219,
                    -0.97703398286946008,     -0.97703398286946053 },
   {  30, 213,      -1.5428796808595964,
                     -1.5428796808595955,      -1.5428796808595955 },
   {  30, 251,      0.99467634781466474,
                     0.99467634781466696,      0.99467634781466652 },
   /* period 100: outBegIdx 99, outNBElement 153 */
   { 100,  99,      0.42179091594997609,
                     0.42179091594997109,      0.42179091594997109 },
   { 100, 100,      0.21462593355687948,
                     0.21462593355687787,      0.21462593355687787 },
   { 100, 101,     0.034508591159079588,
                    0.034508591159079227,     0.034508591159079227 },
   { 100, 107,     -0.75204168259206006,
                    -0.75204168259207105,      -0.7520416825920706 },
   { 100, 122,      -1.5506531070513621,
                     -1.5506531070513641,      -1.5506531070513636 },
   { 100, 174,     -0.17753679290639901,
                    -0.17753679290640001,     -0.17753679290640045 },
   { 100, 189,     -0.42721841462360305,
                    -0.42721841462360377,     -0.42721841462360333 },
   { 100, 200,    -0.053104354202882023,
                   -0.053104354202880621,    -0.053104354202880177 },
   { 100, 204,       2.2380243180325019,
                      2.2380243180325152,       2.2380243180325148 },
   { 100, 251,      -1.1147080989277434,
                     -1.1147080989277425,      -1.1147080989277427 },
};
#define NB_KURT_GOLDEN ((int)(sizeof(kurtGoldens)/sizeof(KurtGolden)))

/* Microsoft's KURT documentation: =KURT(3,4,5,2,3,4,5,6,4,7) is -0.151799637,
 * published to nine decimals. The exact value is -0.15179963720841422. */
static const double kurtExcel[] = { 3.0, 4.0, 5.0, 2.0, 3.0, 4.0, 5.0, 6.0, 4.0, 7.0 };
#define KURT_EXCEL (-0.151799637)

/* Legs 9 to 14 hold every window of a call to the same window computed with
 * no history (startIdx == endIdx). This body's worst across them is 4.1e-11,
 * at period 2000, where the full run itself sits 3.7e-11 from exact. With
 * var.c's triggers and a rebuild every n/4, the decay onto the shift, an
 * outlier's departure, volatility regimes and a near-flat window err by 1e-6
 * to 3e2.
 */
#define KURT_FRESH_TOL 1e-9

/* Tolerances, from measurement rather than habit. The worst relative deviation
 * of this implementation from the 60-digit reference is 9.96e-14 on the benign
 * corpus and 1.59e-15 on the near-degenerate one, so 1e-12 leaves an order of
 * headroom on both and still fails anything that changes the estimator. For
 * scale: scipy 1.15.3 misses the near-degenerate window by 375% (1.6625
 * against 0.35) and warns while doing it, which is what makes that leg worth
 * having rather than a restatement of the benign one.
 */
#define KURT_GOLDEN_TOL 1e-12

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_kurtProgCmp;
static int g_kurtProgSkip;
static int g_kurtScaleCmp;
static int g_kurtGoldenCmp;
static int g_kurtNearCmp;
static int g_kurtNanCmp;
static int g_kurtAliasCmp;
static int g_kurtOracleCmp;
static int g_kurtFlatCmp;
static int g_kurtHistCmp;
static int g_kurtHistFlatCmp;
static int g_kurtDepartCmp;
static int g_kurtRegimeCmp;
static int g_kurtUlpCmp;
static int g_kurtBadNanCmp;
static int g_kurtCleanCmp;
static int g_kurtCalmCmp;
static int g_kurtStarveCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_kurt_progression( void );
static ErrorNumber test_kurt_scale_invariance( const TA_History *history );
static ErrorNumber test_kurt_golden( void );
static ErrorNumber test_kurt_degenerate( void );
static ErrorNumber test_kurt_aliasing( const TA_History *history );
static ErrorNumber test_kurt_contract( const TA_History *history );
static ErrorNumber test_kurt_range( const TA_History *history );
static ErrorNumber test_kurt_oracle( const TA_History *history );
static ErrorNumber test_kurt_flat( void );
static ErrorNumber test_kurt_history( void );
static ErrorNumber test_kurt_departure( void );
static ErrorNumber test_kurt_regimes( void );
static ErrorNumber test_kurt_ulp( void );
static ErrorNumber test_kurt_nonfinite( void );
static ErrorNumber test_kurt_starve( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_kurtosis( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_kurtProgCmp = g_kurtProgSkip = g_kurtScaleCmp = g_kurtGoldenCmp = 0;
   g_kurtNearCmp = g_kurtNanCmp = g_kurtAliasCmp = 0;
   g_kurtOracleCmp = g_kurtFlatCmp = g_kurtHistCmp = g_kurtHistFlatCmp = 0;
   g_kurtDepartCmp = g_kurtRegimeCmp = g_kurtUlpCmp = 0;
   g_kurtBadNanCmp = g_kurtCleanCmp = g_kurtCalmCmp = g_kurtStarveCmp = 0;

   err = test_kurt_progression();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_scale_invariance( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_golden();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_degenerate();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_contract( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_range( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_flat();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_history();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_departure();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_regimes();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_ulp();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_nonfinite();
   if( err != TA_TEST_PASS )
      return err;

   err = test_kurt_starve();
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( history->nbBars == 252 )
   {
      /* One source for each count: a second copy in the message can disagree
       * with the one the gate tests, and only a mutation would show it.  */
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "progression",        102420, &g_kurtProgCmp     },
         { "progression skipped",    36, &g_kurtProgSkip    },
         { "scale",                5775, &g_kurtScaleCmp    },
         { "golden",                 52, &g_kurtGoldenCmp   },
         { "near-degenerate",        13, &g_kurtNearCmp     },
         { "NaN",                   571, &g_kurtNanCmp      },
         { "alias",                1155, &g_kurtAliasCmp    },
         { "oracle",                 43, &g_kurtOracleCmp   },
         { "flat",                62322, &g_kurtFlatCmp     },
         { "history",              1368, &g_kurtHistCmp     },
         { "history flat",         2604, &g_kurtHistFlatCmp },
         { "departure",           11469, &g_kurtDepartCmp   },
         { "regimes",             22644, &g_kurtRegimeCmp   },
         { "one ulp",              4001, &g_kurtUlpCmp      },
         { "non-finite window",     195, &g_kurtBadNanCmp   },
         { "clean window",         7303, &g_kurtCleanCmp    },
         { "periodic",             1981, &g_kurtCalmCmp     },
         { "second moment",       10887, &g_kurtStarveCmp   },
      };
      unsigned int c;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "KURTOSIS Fail: the %s leg compared %d times, not the %d "
                    "this file was written with\n",
                    cov[c].leg, *cov[c].got, cov[c].want );
            return TA_KURTOSIS_VACUOUS;
         }
      }
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) Every arithmetic progression has excess kurtosis exactly -6/5.
 *
 * THE PREMISE IS CHECKED PER SERIES. 1e-4 is not a double, so base + 1e-4*i
 * lands on unequal differences at every base here and the series is not a
 * progression at all; asserting -1.2 on it would be testing the spacing's
 * representability. Such a combination is skipped and counted rather than
 * silently expected to hold. 2^-13 is the small spacing that stays exact, and
 * at base 3.1e10 it spans 600 bars in about 2e4 ulps.
 *
 * Measured worst |G2 + 1.2| is 1.1e-15. A wrong coefficient misses by O(1/n)
 * and needs nothing this tight; the 1e-12 is for a weakened rebuild trigger.
 */
static ErrorNumber test_kurt_progression( void )
{
   static double in[600], out[600];
   static const double spacings[] = { 1.0, 0.25, -3.0, 1e-4, 7.5,
                                      0.0001220703125 };
   static const double bases[]    = { 0.0, 100.0, -50.0, 31498938283.0 };
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int s, b, n, i, uneven;
   double d0, di;

   for( s = 0; s < 6; s++ )
   for( b = 0; b < 4; b++ )
   for( n = 4; n <= 60; n += 7 )
   {
      for( i = 0; i < 600; i++ )
         in[i] = bases[b] + spacings[s] * (double)i;

      uneven = 0;
      d0 = in[1] - in[0];
      for( i = 2; i < 600; i++ )
      {
         di = in[i] - in[i-1];
         if( memcmp( &d0, &di, sizeof(double) ) != 0 ) { uneven = 1; break; }
      }
      if( uneven )
      {
         g_kurtProgSkip++;
         continue;
      }

      retCode = TA_KURTOSIS( 0, 599, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || nbElement != 600-(n-1) )
      {
         printf( "KURTOSIS progression Fail [base=%g spacing=%g N=%d]: rc=%d "
                 "(%d,%d)\n", bases[b], spacings[s], n, (int)retCode,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      KURT_SERVER_VERIFY( 0, 599, 600, retCode, begIdx, nbElement, in, n, out );

      for( i = 0; i < nbElement; i++ )
      {
         g_kurtProgCmp++;
         if( !(fabs(out[i] - KURT_PROGRESSION) <= 1e-12) )
         {
            printf( "KURTOSIS progression Fail [base=%g spacing=%g N=%d] bar %d: "
                    "%.17g, expected %.17g. Every progression has this value at "
                    "every n -- the n-dependence cancels -- so a miss is the "
                    "estimator, not the corpus\n",
                    bases[b], spacings[s], n, begIdx+i, out[i], KURT_PROGRESSION );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) Scaling by a power of two is BITWISE invisible.
 *
 * G2 is a standardised moment, so it is scale-free in exact arithmetic. In
 * floating point that only survives if the scaling is exact: a power of two
 * shifts the exponent and leaves every mantissa alone, so each deviation, each
 * running sum and the final quotient are the same numbers scaled by the same
 * power, and the k^4 in the numerator cancels the k^4 in the denominator
 * exactly. Any other multiplier rounds and the equality is approximate.
 *
 * THE EXPONENT RANGE IS LOAD-BEARING. This leg exists to catch an absolute
 * threshold anywhere in the body -- the #243 mistake, where a fixed epsilon is
 * a cliff at a price level rather than a noise floor. At 2^-8 a corpus around
 * 100 still has a second moment near 1e-1, so a 1e-14 floor is nowhere near it
 * and the leg passes over the defect. MEASURED: with such a floor inserted,
 * 2^-8 is green and 2^-40 is red. The span has to reach the floor to see it.
 */
static ErrorNumber test_kurt_scale_invariance( const TA_History *history )
{
   static double scaled[KURT_CAP], base[KURT_CAP], out[KURT_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i, e;
   double k;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = 4; n <= 40; n += 9 )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, base );
      if( retCode != TA_SUCCESS )
      {
         printf( "KURTOSIS scale Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, base );

      for( e = -40; e <= 40; e += 20 )
      {
         k = ldexp( 1.0, e );
         for( i = 0; i < nbBars; i++ )
            scaled[i] = history->close[i] * k;

         retCode = TA_KURTOSIS( 0, nbBars-1, scaled, n,
                                &begIdx2, &nbElement2, out );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
         {
            printf( "KURTOSIS scale Fail [N=%d k=2^%d]: rc=%d (%d,%d) vs "
                    "(%d,%d)\n", n, e, (int)retCode, begIdx2, nbElement2,
                    begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbElement; i++ )
         {
            g_kurtScaleCmp++;
            if( memcmp( &base[i], &out[i], sizeof(double) ) != 0 )
            {
               printf( "KURTOSIS scale Fail [N=%d k=2^%d] bar %d: %.17g against "
                       "%.17g -- a power of two must be bitwise invisible to a "
                       "standardised moment\n",
                       n, e, begIdx+i, out[i], base[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Frozen goldens, computed in 60-digit decimal over the exact binary value
 * of each input (never over its shortest round-trip decimal, which sits ~3e-15
 * away and is the size of what is being measured).
 */
static ErrorNumber kurt_vs_golden( const char *tag, const double *x, int nbBars,
                                   int period, const double *golden, int nbGolden,
                                   int *counter )
{
   static double out[KURT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;
   double err;

   retCode = TA_KURTOSIS( 0, nbBars-1, x, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != nbGolden
       || begIdx != period-1 )
   {
      printf( "KURTOSIS golden[%s] Fail: rc=%d (%d,%d), expected (%d,%d)\n",
              tag, (int)retCode, begIdx, nbElement, period-1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                       x, period, out );

   for( i = 0; i < nbGolden; i++ )
   {
      (*counter)++;
      err = fabs( out[i] - golden[i] );
      if( golden[i] != 0.0 )
         err /= fabs( golden[i] );
      if( !(err <= KURT_GOLDEN_TOL) )
      {
         printf( "KURTOSIS golden[%s] Fail at bar %d: %.17g against %.17g, "
                 "relative %.3e over %.3e\n",
                 tag, begIdx+i, out[i], golden[i], err, KURT_GOLDEN_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_kurt_golden( void )
{
   ErrorNumber err;

   err = kurt_vs_golden( "benign p=10", kurtBenign, 40, 10,
                         kurtBenign_g2_10, 31, &g_kurtGoldenCmp );
   if( err != TA_TEST_PASS )
      return err;

   err = kurt_vs_golden( "benign p=20", kurtBenign, 40, 20,
                         kurtBenign_g2_20, 21, &g_kurtGoldenCmp );
   if( err != TA_TEST_PASS )
      return err;

   /* The near-degenerate window: every value is base + k*ulp at base 2^20, so
    * the corpus is exact and the reference is not measuring its own error.
    * NON-VACUOUS, and by a wide margin -- scipy 1.15.3 answers 1.6624999999999996
    * on the first window where the reference says 0.34999999999999998, a 375%
    * miss, and raises RuntimeWarning: Precision loss occurred while doing it.
    * This implementation lands 1.59e-15 from the reference there.
    */
   return kurt_vs_golden( "near-degenerate p=8", kurtNearDegenerate, 20, 8,
                          kurtNearDegenerate_g2_8, 13, &g_kurtNearCmp );
}

/* (4) A window with no spread returns NaN, not a number. A guard that
 * substitutes one fails it. Leg 9 covers the levels whose window mean does not
 * round back to the level.
 */
static ErrorNumber test_kurt_degenerate( void )
{
   static double in[600], out[600];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   for( i = 0; i < 600; i++ )
      in[i] = 42.0;

   retCode = TA_KURTOSIS( 0, 599, in, 30, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 571 )
   {
      printf( "KURTOSIS degenerate Fail: rc=%d (%d,%d), expected (29,571)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   KURT_SERVER_VERIFY( 0, 599, 600, retCode, begIdx, nbElement, in, 30, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_kurtNanCmp++;
      if( !isnan( out[i] ) )
      {
         printf( "KURTOSIS degenerate Fail at bar %d: %.17g, expected NaN. A "
                 "point mass has no excess kurtosis and no defensible neutral "
                 "-- 0 asserts normality, -1.2 asserts uniformity\n",
                 begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) In-place aliasing, outReal == inReal, bitwise. The rebuild re-reads the
 * window after this bar's store, so the store must not land under a read the
 * next bar still needs.
 */
static ErrorNumber test_kurt_aliasing( const TA_History *history )
{
   static double clean[KURT_CAP], alias[KURT_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = 4; n <= 40; n += 9 )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "KURTOSIS alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      /* The non-aliased call only: in-place behaviour is a C-side memory
       * property, not something the servers are asked to reproduce. */
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, clean );

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_KURTOSIS( 0, nbBars-1, alias, n,
                             &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "KURTOSIS alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_kurtAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "KURTOSIS alias Fail [N=%d] bar %d: separate %.17g, "
                    "in-place %.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) The (n-2)(n-3) denominators are why the range starts at 4, and the
 * contract is what enforces it -- there is no runtime branch for a short
 * window, so this leg is the only thing standing between a caller and a
 * division by zero.
 */
static ErrorNumber test_kurt_contract( const TA_History *history )
{
   static double out[KURT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, lookback;

   if( nbBars > KURT_CAP )
      nbBars = KURT_CAP;

   for( n = -2; n <= 3; n++ )
   {
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, out );
      if( retCode != TA_BAD_PARAM )
      {
         printf( "KURTOSIS contract Fail: period %d gave rc=%d, expected "
                 "TA_BAD_PARAM -- (n-2)(n-3) is zero or negative below 4\n",
                 n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   for( n = 4; n <= 40; n += 9 )
   {
      lookback = TA_KURTOSIS_Lookback( n );
      if( lookback != n-1 )
      {
         printf( "KURTOSIS contract Fail: lookback(%d) = %d, expected %d\n",
                 n, lookback, n-1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      retCode = TA_KURTOSIS( 0, nbBars-1, history->close, n,
                             &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != nbBars-(n-1) )
      {
         printf( "KURTOSIS contract Fail [N=%d]: rc=%d (%d,%d), expected "
                 "(%d,%d)\n", n, (int)retCode, begIdx, nbElement,
                 n-1, nbBars-(n-1) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          history->close, n, out );
   }

   return TA_TEST_PASS;
}

/* (7) The startIdx/endIdx range sweep. TA_STABLE_EPSILON, not EXACT: the window
 * is finite, but it is carried in a running accumulator whose shift -- and
 * whose rebuild phase -- are both seeded from the call's own startIdx. Two
 * calls covering the same bar from different starts therefore rebuild on
 * different bars and land a few ulp apart, which is exactly what this class
 * describes. No unstable period: the rebuild re-derives the shift from the
 * window itself and carries nothing in from before startIdx.
 */
typedef struct { int period; const TA_Real *in; } KurtRangeParam;

static TA_RetCode kurtRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   KurtRangeParam *p = (KurtRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_KURTOSIS_Lookback( p->period );
   return TA_KURTOSIS( startIdx, endIdx, p->in, p->period,
                       outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_kurt_range( const TA_History *history )
{
   KurtRangeParam param;

   param.period = 30;
   param.in     = history->close;

   return doRangeTestEx( kurtRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (8) The frozen external-oracle goldens and Excel's published example, plus
 * the cross-language replay. */
static ErrorNumber test_kurt_oracle( const TA_History *history )
{
   static TA_Real out[KURT_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode retCode;
   int k, lastPeriod = -1;
   const char *which;
   double got, want, tol;

   retCode = TA_KURTOSIS( 0, 9, kurtExcel, 10, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 9 || nbElement != 1 )
   {
      printf( "KURTOSIS Excel Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx,
              nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   g_kurtOracleCmp++;
   if( !(fabs( out[0] - KURT_EXCEL ) <= 5e-10) )
   {
      printf( "KURTOSIS Excel Fail: KURT(3,4,5,2,3,4,5,6,4,7) is %.17g, "
              "Microsoft publishes %.9f\n", out[0], KURT_EXCEL );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( history->nbBars != 252 )
      return TA_TEST_PASS;

   for( k = 0; k < NB_KURT_GOLDEN; k++ )
   {
      const KurtGolden *g = &kurtGoldens[k];

      if( g->period != lastPeriod )
      {
         lastPeriod = g->period;
         retCode = TA_KURTOSIS( 0, 251, history->close, lastPeriod,
                                &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod-1
             || nbElement != 252-begIdx )
         {
            printf( "KURTOSIS oracle Fail [N=%d]: rc=%d (%d,%d)\n", lastPeriod,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         KURT_SERVER_VERIFY( 0, 251, 252, retCode, begIdx, nbElement,
                             history->close, lastPeriod, out );
      }

      /* A golden's bar is hand-transcribed and indexes `out` unchecked. */
      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "KURTOSIS oracle Fail [N=%d]: golden bar %d is outside the "
                 "output [%d..%d]\n", g->period, g->bar, begIdx,
                 begIdx + nbElement - 1 );
         return TA_KURTOSIS_VACUOUS;
      }

      got = out[g->bar - begIdx];
      g_kurtOracleCmp++;
      which = NULL;
      if( !(fabs( got - g->exact ) <= KURT_EXACT_TOL * fmax( 1.0, fabs( g->exact ) )) )
      {
         which = "exact";
         want  = g->exact;
         tol   = KURT_EXACT_TOL;
      }
      else if( !(fabs( got - g->scipy ) <= KURT_ORACLE_TOL * fmax( 1.0, fabs( g->scipy ) )) )
      {
         which = "scipy";
         want  = g->scipy;
         tol   = KURT_ORACLE_TOL;
      }
      else if( !(fabs( got - g->pta ) <= KURT_ORACLE_TOL * fmax( 1.0, fabs( g->pta ) )) )
      {
         which = "pandas-ta-classic";
         want  = g->pta;
         tol   = KURT_ORACLE_TOL;
      }
      if( which )
      {
         printf( "KURTOSIS oracle Fail [N=%d] at bar %d: got %.17g, %s expects "
                 "%.17g (distance %.3e over %.0e)\n", g->period, g->bar, got,
                 which, want, fabs( got - want ) / fmax( 1.0, fabs( want ) ), tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Every window of one call against the same window computed with no history,
 * and a flat window must be NaN from both. */
static ErrorNumber kurt_vs_fresh( const char *tag, const double *in, int nbBars,
                                  int period, int toServers,
                                  int *counter, int *flatCounter )
{
   static TA_Real out[KURT_FRESH_CAP];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   int i, j, bar, flat;

   retCode = TA_KURTOSIS( 0, nbBars-1, in, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period-1
       || nbElement != nbBars-period+1 )
   {
      printf( "KURTOSIS %s Fail [N=%d]: rc=%d (%d,%d)\n", tag, period,
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( toServers )
      KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                          in, period, out );

   for( i = 0; i < nbElement; i++ )
   {
      bar  = begIdx + i;
      flat = 1;
      for( j = bar-period+1; j <= bar && flat; j++ )
         if( in[j] != in[bar] )
            flat = 0;

      if( flat )
      {
         if( !flatCounter )
         {
            printf( "KURTOSIS %s Fail [N=%d]: bar %d is a flat window this leg "
                    "was not written to reach\n", tag, period, bar );
            return TA_KURTOSIS_VACUOUS;
         }
         (*flatCounter)++;
         if( !isnan( out[i] ) )
         {
            printf( "KURTOSIS %s Fail [N=%d] bar %d: %.17g on a flat window at "
                    "%.17g, expected NaN\n", tag, period, bar, out[i], in[bar] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }

      retCode = TA_KURTOSIS( bar, bar, in, period, &begOne, &nbOne, &one );
      if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
      {
         printf( "KURTOSIS %s Fail [N=%d]: one-bar call at %d rc=%d (%d,%d)\n",
                 tag, period, bar, (int)retCode, begOne, nbOne );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      (*counter)++;
      if( !(fabs( out[i] - one ) <= KURT_FRESH_TOL * fmax( 1.0, fabs( one ) )) )
      {
         printf( "KURTOSIS %s Fail [N=%d] bar %d: %.17g against %.17g with no "
                 "history, distance %.3e over %.0e\n", tag, period, bar,
                 out[i], one, fabs( out[i] - one ) / fmax( 1.0, fabs( one ) ),
                 KURT_FRESH_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static unsigned int kurt_lcg( unsigned int *seed )
{
   *seed = (*seed * 1103515245u + 12345u) & 0x7fffffffu;
   return *seed >> 8;
}

/* (9) A flat stretch after a walk is NaN at every level, including levels whose
 * window mean does not round back to the level. Anchored on that mean, every
 * deviation is the same rounding residue and the moments are its cancellation:
 * a finite value, not 0/0. 49, 98 and 103 are periods where n * (1/n) != 1 in
 * double, 2000 is long enough for the sum's own rounding to reach many ulps,
 * and 4 is the shortest period. All but 2000 run past the periodic rebuild.
 * At 3.7e-67 the residue's fourth power underflows while its square does not,
 * so only the second moment can tell that the anchor missed.
 */
#define KURT_FLAT_BARS 6000
static ErrorNumber test_kurt_flat( void )
{
   static const double levels[] = { 0.1, 100.0, 100.37, 3.3, 31498938283.17,
                                    3.7e-67 };
   static const int periods[]   = { 4, 49, 98, 103, 2000 };
   static TA_Real in[KURT_FLAT_BARS];
   ErrorNumber err;
   unsigned int seed;
   int lv, pr, n, i, nbBars, leadIn;

   for( lv = 0; lv < 6; lv++ )
   for( pr = 0; pr < 5; pr++ )
   {
      n = periods[pr];
      nbBars = 35*n;
      if( nbBars > KURT_FLAT_BARS )
         nbBars = KURT_FLAT_BARS;

      seed = 99u;
      for( i = 0; i < n; i++ )
         in[i] = levels[lv] * (1.0 + 1e-3*((double)(kurt_lcg( &seed ) % 2001u) - 1000.0)*1e-3);
      for( ; i < nbBars; i++ )
         in[i] = levels[lv];

      leadIn = 0;
      err = kurt_vs_fresh( "flat", in, nbBars, n, 1, &leadIn, &g_kurtFlatCmp );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (10) History. After a flat lead-in, the series jumps away and decays
 * geometrically back onto its first value, which is the shift until a rebuild,
 * so once the spread has gone the running sums hold nothing but the rounding
 * left by the values that departed. A trigger that compares against the
 * current sums rather than the largest held since the last rebuild misses it.
 */
static ErrorNumber test_kurt_history( void )
{
   static const double levels[] = { 100.37, 0.1, 1.0, 3.3 };
   static const double ratios[] = { 0.1, 0.05, 0.2 };
   static const int periods[]   = { 4, 5, 8, 12, 20 };
   static TA_Real in[75];
   ErrorNumber err;
   int lv, rt, pr, j;
   double dev;

   for( lv = 0; lv < 4; lv++ )
   for( rt = 0; rt < 3; rt++ )
   {
      for( j = 0; j < 20; j++ )
         in[j] = levels[lv];
      dev = 0.5*levels[lv];
      for( j = 1; j <= 15; j++ )
      {
         dev *= ratios[rt];
         in[19+j] = (j*7) % 3 ? levels[lv] + dev : levels[lv] - dev;
      }
      for( j = 35; j < 75; j++ )
         in[j] = levels[lv];

      for( pr = 0; pr < 5; pr++ )
      {
         err = kurt_vs_fresh( "history", in, 75, periods[pr], 1,
                              &g_kurtHistCmp, &g_kurtHistFlatCmp );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   return TA_TEST_PASS;
}

/* (11) An outlier leaving the window. Its fourth power dominated the sums, so
 * what survives is mostly its rounding; a rebuild anchored on a mean that
 * still includes it leaves the shift far from every remaining value.
 */
#define KURT_DEPART_BARS 3000
static ErrorNumber test_kurt_departure( void )
{
   static const int periods[] = { 5, 30, 100, 400 };
   static TA_Real in[KURT_DEPART_BARS];
   ErrorNumber err;
   unsigned int seed;
   int pr, i;
   double v;

   seed = 12345u;
   v    = 100.0;
   for( i = 0; i < KURT_DEPART_BARS; i++ )
   {
      v += ((double)(kurt_lcg( &seed ) % 2001u) - 1000.0) * 5e-4;
      in[i] = v;
   }
   for( i = 100; i < KURT_DEPART_BARS; i += 200 )
      in[i] = 1e5;

   for( pr = 0; pr < 4; pr++ )
   {
      err = kurt_vs_fresh( "departure", in, KURT_DEPART_BARS, periods[pr], 1,
                           &g_kurtDepartCmp, NULL );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (12) Volatility regimes 500x apart, 500 bars each. Entering a calm regime
 * the fourth moment falls by orders of magnitude inside one window, and the
 * shift anchored in the volatile one is many calm sigma stale.
 */
#define KURT_REGIME_BARS 6000
static ErrorNumber test_kurt_regimes( void )
{
   static const int periods[] = { 10, 50, 300, 1000 };
   static TA_Real in[KURT_REGIME_BARS];
   ErrorNumber err;
   unsigned int seed;
   int pr, i;
   double v;

   seed = 4242u;
   v    = 100.0;
   for( i = 0; i < KURT_REGIME_BARS; i++ )
   {
      v += ((double)(kurt_lcg( &seed ) % 2001u) - 1000.0)
           * ((i/500) % 2 ? 5e-3 : 1e-5);
      in[i] = v;
   }

   for( pr = 0; pr < 4; pr++ )
   {
      err = kurt_vs_fresh( "regimes", in, KURT_REGIME_BARS, periods[pr], 1,
                           &g_kurtRegimeCmp, NULL );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (13) A long window of one level with a few bars a few ulps off. Its spread
 * is under the rounding of the window's own left-to-right mean, so a rebuild
 * anchored there is stale on arrival and fires again on every bar. The first
 * bar sits 0.1% off the level, so its departure forces that rebuild.
 */
#define KURT_ULP_PERIOD 2000
#define KURT_ULP_BARS   6000
static ErrorNumber test_kurt_ulp( void )
{
   static TA_Real in[KURT_ULP_BARS];
   unsigned int seed, r;
   int i;
   double ulp;

   ulp  = nextafter( 100.37, 200.0 ) - 100.37;
   seed = 777u;
   for( i = 0; i < KURT_ULP_BARS; i++ )
   {
      r = kurt_lcg( &seed );
      in[i] = r % 20u == 0 ? 100.37 + (double)((int)(r % 7u) - 3)*ulp : 100.37;
   }
   in[0] = 100.37 * 1.001;

   return kurt_vs_fresh( "one ulp", in, KURT_ULP_BARS, KURT_ULP_PERIOD, 1,
                         &g_kurtUlpCmp, NULL );
}

/* (14) A non-finite input poisons the running sums, and the periodic rebuild
 * is the only way out: every trigger comparison is false on NaN. A window
 * holding the value is NaN, the output may stay NaN for up to 32n bars after
 * it leaves, and from then on every window must match a no-history recompute
 * again. Not sent to the servers: a NaN's bits are no contract, and those an
 * fma makes from an Inf differ between JVMs. The servers get a finite control
 * instead: on stationary noise at period 20 no trigger fires, so the periodic
 * rebuild alone moves the output bits they compare.
 */
#define KURT_BAD_BARS 4600
#define KURT_CALM_BARS 2000
static ErrorNumber test_kurt_nonfinite( void )
{
   static const int periods[] = { 5, 20, 40 };
   static const int badAt[]   = { 100, 1600, 3100 };
   static TA_Real in[KURT_BAD_BARS], out[KURT_BAD_BARS], calm[KURT_CALM_BARS];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   ErrorNumber err;
   unsigned int seed;
   int pr, n, i, k, bar, held, graceEnd;
   double v;

   seed = 31337u;
   for( i = 0; i < KURT_CALM_BARS; i++ )
      calm[i] = 100.0 + 1e-2*(((double)(kurt_lcg( &seed ) % 2001u) - 1000.0)/1000.0);
   err = kurt_vs_fresh( "periodic", calm, KURT_CALM_BARS, 20, 1, &g_kurtCalmCmp,
                        NULL );
   if( err != TA_TEST_PASS )
      return err;

   seed = 2718u;
   v    = 100.0;
   for( i = 0; i < KURT_BAD_BARS; i++ )
   {
      v += ((double)(kurt_lcg( &seed ) % 2001u) - 1000.0) * 5e-4;
      in[i] = v;
   }
   in[badAt[0]] = NAN;
   in[badAt[1]] = INFINITY;
   in[badAt[2]] = -INFINITY;

   for( pr = 0; pr < 3; pr++ )
   {
      n = periods[pr];
      retCode = TA_KURTOSIS( 0, KURT_BAD_BARS-1, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1
          || nbElement != KURT_BAD_BARS-n+1 )
      {
         printf( "KURTOSIS non-finite Fail [N=%d]: rc=%d (%d,%d)\n", n,
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         bar = begIdx + i;
         held = 0;
         graceEnd = -1;
         for( k = 0; k < 3; k++ )
         {
            if( badAt[k] > bar-n && badAt[k] <= bar )
               held = 1;
            else if( badAt[k] <= bar-n )
               graceEnd = badAt[k] + n + 32*n;
         }

         if( held )
         {
            g_kurtBadNanCmp++;
            if( !isnan( out[i] ) )
            {
               printf( "KURTOSIS non-finite Fail [N=%d] bar %d: %.17g on a "
                       "window holding a non-finite input, expected NaN\n",
                       n, bar, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            continue;
         }

         retCode = TA_KURTOSIS( bar, bar, in, n, &begOne, &nbOne, &one );
         if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
         {
            printf( "KURTOSIS non-finite Fail [N=%d]: one-bar call at %d rc=%d "
                    "(%d,%d)\n", n, bar, (int)retCode, begOne, nbOne );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         /* Inside the grace window a value is checked once it is back, but
          * not counted: when it comes back depends on the rebuild schedule. */
         if( bar < graceEnd )
         {
            if( isnan( out[i] ) )
               continue;
         }
         else
            g_kurtCleanCmp++;
         if( !(fabs( out[i] - one ) <= KURT_FRESH_TOL * fmax( 1.0, fabs( one ) )) )
         {
            printf( "KURTOSIS non-finite Fail [N=%d] bar %d: %.17g against %.17g "
                    "with no history", n, bar, out[i], one );
            if( graceEnd >= 0 )
               printf( ", %d bars after the last non-finite input left the "
                       "window", bar - (graceEnd - 32*n) );
            printf( "\n" );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (15) The second moment's own trigger. A window of constant |deviation| sets
 * peak4 = peak2^2/n; one jump among equal values then keeps the fourth moment
 * above 1% of its peak while the second falls under 1% of its own, and a
 * fourth-moment trigger alone lets the second moment's rounding through,
 * squared. Every window holding the jump among n-1 zeros has G2 exactly n. At
 * n = 10000 this body is within 4.1e-13*n of it, and 1.1e-10*n without the
 * second-moment trigger. n = 1000 takes the servers down the same rebuild,
 * where the two differ only in the last bits; the larger vector is far past
 * their request buffer.
 */
typedef struct { int n; double jump; int jumpAt; int toServers; } KurtStarve;
#define KURT_STARVE_BARS (4*10000 + 1)
static ErrorNumber test_kurt_starve( void )
{
   static const KurtStarve cases[] = {
      {  1000, 2.5,  2990,                          1 },
      { 10000, 5.75, 3*10000 - 1 - (10000/100 + 2), 0 },
   };
   static TA_Real in[KURT_STARVE_BARS], out[KURT_STARVE_BARS];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int c, n, nbBars, i, bar;

   for( c = 0; c < 2; c++ )
   {
      n = cases[c].n;
      nbBars = 4*n + 1;
      in[0] = 0.0;
      for( i = 1; i <= n; i++ )
         in[i] = i % 2 ? 1e-3 : -1e-3;
      for( ; i <= 2*n; i++ )
         in[i] = i % 2 ? 1.0 : -1.0;
      for( ; i < nbBars; i++ )
         in[i] = 0.0;
      in[cases[c].jumpAt] = cases[c].jump;

      retCode = TA_KURTOSIS( 0, nbBars-1, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != nbBars-n+1 )
      {
         printf( "KURTOSIS second-moment Fail [N=%d]: rc=%d (%d,%d)\n", n,
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      if( cases[c].toServers )
         KURT_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                             in, n, out );

      for( bar = 3*n; bar < cases[c].jumpAt + n; bar++ )
      {
         g_kurtStarveCmp++;
         if( !(fabs( out[bar-begIdx] - (double)n ) <= 1e-11 * n) )
         {
            printf( "KURTOSIS second-moment Fail [N=%d] at bar %d: %.17g, exact "
                    "%d\n", n, bar, out[bar-begIdx], n );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}
