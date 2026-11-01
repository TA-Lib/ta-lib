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
 *  110126 MF,CC  First version (issue #569).
 *
 */

/* Description:
 *     Test TA_SKEW, the adjusted Fisher-Pearson coefficient of skewness (G1).
 *
 *     WHAT EACH LEG CAN SEE, measured by mutating the generator input and
 *     regenerating; every leg named goes red.
 *
 *       sqrt(n) for sqrt(n-1) in the coefficient     jump, near-degenerate,
 *                                                    degenerate, oracle
 *       third moment's 2n*r^3 term dropped           jump, near-degenerate,
 *                                                    degenerate, range, oracle,
 *                                                    flat, history, non-finite
 *       no trigger, or no rebuild at all             oracle, flat, history,
 *                                                    non-finite
 *       trigger at 1e-4 of the peak                  oracle, flat, history
 *       trigger against the current sum, not peak    history
 *       no re-anchor after the mean rebuild          jump, flat
 *       no periodic rebuild, or every 256n           non-finite
 *       degenerate window left to 0/0                jump, degenerate, flat,
 *                                                    history
 *       guard on M2 rather than on the denominator   degenerate
 *       guard on a zero denominator, not subnormal   degenerate
 *       guard raised to 1e-290                       degenerate
 *       an absolute 1e-14 floor on M2                jump, near-degenerate,
 *                                                    degenerate
 *
 *     No leg sees re-anchoring on the window's newest value instead of the
 *     residue: it moves the last bits, not a value by 1e-9.
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
#define SKEW_CAP 1100
#define SKEW_FRESH_CAP 6000

/* Every success-case call on a vector of this file's own goes to the language
 * servers bit for bit, bar the one-bar recomputes and the non-finite leg. The
 * period<3 leg is not wrapped: server_verify() skips reject cases by design.
 */
#define SKEW_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "SKEW", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),                 \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "SKEW [period %d]: server_verify compared no server "        \
                    "despite live pipes\n", (int)(period) );                     \
            return TA_SKEW_VACUOUS;                                              \
         }                                                                       \
      }                                                                          \
   } while(0)

/* Distance used throughout: G1 crosses zero, where a relative distance is
 * unbounded. */
#define SKEW_DIST(got, want) (fabs( (got) - (want) ) / fmax( 1.0, fabs( want ) ))

/* Every value 2^20 + k ulp: the corpus is exact, and the reference is not
 * measuring its own error. */
static const double skewNearDegenerate[] = {
   1048576.0, 1048576.0000000002, 1048576.0, 1048576.0000000005,
   1048576.0000000002, 1048576.0000000007, 1048576.0, 1048576.0000000002,
   1048576.0000000005, 1048576.0, 1048576.0000000002, 1048576.0000000002,
   1048576.000000001, 1048576.0, 1048576.0000000005, 1048576.0000000002,
   1048576.0, 1048576.0000000007, 1048576.0000000002, 1048576.0000000005,
};
/* Values 2^20 + k*2^-20 in near-symmetric windows, a few nudged by one or two
 * ulps (2^-32): several windows have |G1| near 1e-4, where only the sign and
 * the last digits separate a right answer from cancellation. */
static const double skewNearSymmetric[] = {
   1048576.0, 1048576.0000038147, 1048576.0000009537, 1048576.0000028613,
   1048576.0000019073, 1048576.0000019073, 1048576.0000009537, 1048576.000002861,
   1048576.0, 1048576.0000038147, 1048576.000001907, 1048576.0000038147,
   1048576.0, 1048576.0000028613, 1048576.0000009537, 1048576.0000009537,
   1048576.000002861, 1048575.9999999995, 1048576.0000038147, 1048576.0000019073,
   1048576.0000019073, 1048576.000000954, 1048576.000002861, 1048576.0,
};
/* Each: G1 over the exact doubles, the moments rational and the square root
 * in 60-digit decimal, rounded to the nearest double. */
static const double skewNearDegenerate_8[] = {
   0.9354143466934853, 0.38643671323171835, 0.487832912554607,
   0.38643671323171835, 0.8622790552053495, 0.8081220356417687,
   1.5391347242530748, 1.139535129778294, 1.139535129778294,
   1.5391347242530748, 0.8081220356417687, 0.8081220356417687,
   0.47992216567876855,
};
static const double skewNearSymmetric_8[] = {
   -3.1064302399943125e-05, -3.1064302399943125e-05, -3.1064302399943125e-05,
   -0.30424446147092504, -0.1599314760026033, 7.61242136399239e-05,
   -0.2621114135529784, -0.2621114135529784, 0.2621744421700696,
   -0.2167497107942718, 0.21670325402206114, -0.0001300087334818807,
   0.21662131185139674, -0.000341836306793493, 0.6102027794333617,
   -0.0003419159601656909, -0.08665613073930979,
};
static const double skewNearSymmetric_5[] = {
   -7.718708054349819e-05, 0.4046218211536871, 0.5125385682448519,
   -0.512091906327951, -0.40479600891093653, 0.0,
   0.0001544080870347213, -1.0883466109733622, 0.0001220703115905053,
   -1.0884536019829927, 7.721723607226097e-05, 0.5183288276413062,
   0.16567680578286828, 0.16533982805752934, 0.518145356667609,
   -0.0003088161703895048, -0.5519921125320036, 0.5511078765702715,
   0.405065252219863, -0.40497025020603034,
};

/* Leg 7. Captured on the 252-bar TA_SREF close series at %.17g, by
 * ta-lib-oracles/capture_569_skew.py. `bar` is the ABSOLUTE bar index.
 *
 *   exact : G1 over the exact doubles, the moments rational and the square
 *           root in 60-digit decimal, rounded to the nearest double.
 *   scipy : scipy 1.18.1 `scipy.stats.skew(window, bias=False)` (numpy 2.5.1).
 *   pta   : pandas-ta-classic 0.6.52 `ta.skew(close, length=n)` (pandas
 *           3.0.3), with talib blocked from import.
 *
 * Per period: the first three output bars, bars 100, 200 and 251, both
 * extremes, the bar nearest zero, and the bars where this body, scipy,
 * pandas-ta-classic and pandas `rolling(n).skew()` each sit furthest from
 * exact. The pandas rolling arm is checked but not frozen: its online moments
 * sit 6.5e-9 from exact at period 5.
 *
 * Against the exact column this body's worst over every bar at periods 3, 5,
 * 14, 30, 100 and 252 is 1.1e-12.
 */
#define SKEW_EXACT_TOL  3e-12
#define SKEW_ORACLE_TOL 1e-11

typedef struct { int period; int bar; double exact; double scipy; double pta; } SkewGolden;

static const SkewGolden skewGoldens[] =
{
   /* period 3: outBegIdx 2, outNBElement 250 */
   {   3,   2,      -1.6163957097256014,
                     -1.6163957097256016,      -1.6163957097256019 },
   {   3,   3,     -0.64696859492150738,
                    -0.64696859492127268,      -0.6469685949212729 },
   {   3,   4,      0.28360253809470787,
                     0.28360253809464314,      0.28360253809464325 },
   {   3,  46,                        0,
                  3.7082460869259747e-13,   3.7082460869259752e-13 },
   {   3, 100,      -1.1667973167570633,
                     -1.1667973167570962,      -1.1667973167570966 },
   {   3, 101,       1.7320508075688772,
                      1.7320508075689114,       1.7320508075689118 },
   {   3, 102,      -1.7320508075688772,
                     -1.7320508075688954,      -1.7320508075688956 },
   {   3, 120,     -0.99229202717938436,
                    -0.99229202717938425,     -0.99229202717938436 },
   {   3, 197,      0.62596588390901686,
                     0.62596588390899954,      0.62596588390899965 },
   {   3, 200,       1.6066834278056294,
                      1.6066834278057196,         1.60668342780572 },
   {   3, 251,      -1.3928215515127658,
                     -1.3928215515128373,      -1.3928215515128377 },
   /* period 5: outBegIdx 4, outNBElement 248 */
   {   5,   4,      -1.6230429897856744,
                     -1.6230429897856156,      -1.6230429897856156 },
   {   5,   5,     -0.82242062587638987,
                    -0.82242062587621911,       -0.822420625876219 },
   {   5,   6,      -1.0918516763627111,
                     -1.0918516763626536,      -1.0918516763626533 },
   {   5, 100,     -0.14314775098534746,
                    -0.14314775098534743,     -0.14314775098534746 },
   {   5, 121,      0.19586107447648438,
                      0.1958610744765647,      0.19586107447656476 },
   {   5, 133,      -1.2824649351123254,
                     -1.2824649351120545,      -1.2824649351120543 },
   {   5, 150,     -0.49880353944389139,
                    -0.49880353944387895,      -0.4988035394438789 },
   {   5, 200,      -1.5131933477324833,
                     -1.5131933477324147,      -1.5131933477324149 },
   {   5, 201,       2.1367638193200218,
                      2.1367638193199463,       2.1367638193199463 },
   {   5, 202,      -2.2219579772645699,
                     -2.2219579772645726,      -2.2219579772645721 },
   {   5, 229,    0.0016654890159781739,
                    0.001665489016055698,    0.0016654890160556978 },
   {   5, 251,     -0.63875340277159465,
                     -0.6387534027716838,     -0.63875340277168369 },
   /* period 14: outBegIdx 13, outNBElement 239 */
   {  14,  13,       0.1577529197394438,
                     0.15775291973944092,      0.15775291973944092 },
   {  14,  14,      0.14298208982664967,
                     0.14298208982663563,       0.1429820898266356 },
   {  14,  15,      0.33090529759748527,
                     0.33090529759746545,       0.3309052975974654 },
   {  14,  87,      -2.1231971534345391,
                     -2.1231971534345413,      -2.1231971534345409 },
   {  14, 100,    -0.059795773198311773,
                   -0.059795773198311766,    -0.059795773198311759 },
   {  14, 109,      0.10004470458355855,
                     0.10004470458357961,      0.10004470458357959 },
   {  14, 119,  -0.00072007507800529486,
                 -0.00072007507801139165,  -0.00072007507801139154 },
   {  14, 200,     0.020927957254860366,
                    0.020927957254862774,     0.020927957254862774 },
   {  14, 248,       2.3445294468071345,
                      2.3445294468071309,       2.3445294468071309 },
   {  14, 250,      -1.2244865886161067,
                     -1.2244865886160399,      -1.2244865886160394 },
   {  14, 251,     -0.90093945486700233,
                    -0.90093945486690796,     -0.90093945486690785 },
   /* period 30: outBegIdx 29, outNBElement 223 */
   {  30,  29,     -0.27181007995356488,
                    -0.27181007995357476,     -0.27181007995357476 },
   {  30,  30,     -0.16283632202069856,
                    -0.16283632202071144,     -0.16283632202071147 },
   {  30,  31,    -0.036307297791766516,
                   -0.036307297791761013,    -0.036307297791761027 },
   {  30,  59,    -0.040271163810786922,
                   -0.040271163810825017,     -0.04027116381082501 },
   {  30,  78,       1.3106244077645539,
                      1.3106244077645552,        1.310624407764555 },
   {  30, 100,     -0.84611986573040154,
                    -0.84611986573040565,     -0.84611986573040576 },
   {  30, 102,     -0.97676247293666496,
                    -0.97676247293665963,     -0.97676247293665963 },
   {  30, 161,       1.1197770937021221,
                      1.1197770937021227,       1.1197770937021227 },
   {  30, 167,     0.083564404731802905,
                    0.083564404731804251,     0.083564404731804237 },
   {  30, 200,      -0.2719739161255183,
                    -0.27197391612552035,     -0.27197391612552041 },
   {  30, 212,   -0.0006245207340034903,
                 -0.00062452073400480967,  -0.00062452073400480967 },
   {  30, 251,      0.19424792343393449,
                     0.19424792343394948,       0.1942479234339495 },
   /* period 100: outBegIdx 99, outNBElement 153 */
   { 100,  99,        1.196031212664336,
                      1.1960312126643327,       1.1960312126643327 },
   { 100, 100,       1.1413661154551835,
                      1.1413661154551831,       1.1413661154551829 },
   { 100, 101,       1.0956341231294102,
                      1.0956341231294102,       1.0956341231294102 },
   { 100, 140,    0.0014234360740460856,
                   0.0014234360740451794,     0.001423436074045179 },
   { 100, 189,      0.27704166354701942,
                     0.27704166354702242,      0.27704166354702237 },
   { 100, 192,      0.27842217032584449,
                     0.27842217032582833,      0.27842217032582839 },
   { 100, 195,      0.18561207135693789,
                     0.18561207135693486,      0.18561207135693492 },
   { 100, 200,     -0.20486511742918015,
                    -0.20486511742919156,     -0.20486511742919158 },
   { 100, 207,      -1.2009799774012961,
                     -1.2009799774013012,      -1.2009799774013012 },
   { 100, 251,    -0.081939271676535821,
                   -0.081939271676539277,    -0.081939271676539277 },
};
#define NB_SKEW_GOLDEN ((int)(sizeof(skewGoldens)/sizeof(SkewGolden)))

/* Microsoft's SKEW documentation: =SKEW(3,4,5,2,3,4,5,6,4,7) is 0.359543,
 * published to six decimals. The exact value is 0.35954307140679714. */
static const double skewExcel[] = { 3.0, 4.0, 5.0, 2.0, 3.0, 4.0, 5.0, 6.0, 4.0, 7.0 };
#define SKEW_EXCEL 0.359543

/* Legs 8 to 10 hold every window of a call to the same window computed with
 * no history (startIdx == endIdx). This body's worst across them is 2.9e-10,
 * at period 2000 as a walk leaves the window.
 */
#define SKEW_FRESH_TOL 1e-9

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_skewJumpCmp;
static int g_skewJumpFlatCmp;
static int g_skewJumpSkip;
static int g_skewNearCmp;
static int g_skewZeroCmp;
static int g_skewTinyCmp;
static int g_skewAliasCmp;
static int g_skewOracleCmp;
static int g_skewFlatCmp;
static int g_skewFlatLeadCmp;
static int g_skewHistCmp;
static int g_skewHistFlatCmp;
static int g_skewBadNanCmp;
static int g_skewCleanCmp;
static int g_skewCalmCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_skew_jump( void );
static ErrorNumber test_skew_near( void );
static ErrorNumber test_skew_degenerate( void );
static ErrorNumber test_skew_aliasing( const TA_History *history );
static ErrorNumber test_skew_contract( const TA_History *history );
static ErrorNumber test_skew_range( const TA_History *history );
static ErrorNumber test_skew_oracle( const TA_History *history );
static ErrorNumber test_skew_flat( void );
static ErrorNumber test_skew_history( void );
static ErrorNumber test_skew_nonfinite( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_skew( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_skewJumpCmp = g_skewJumpFlatCmp = g_skewJumpSkip = 0;
   g_skewNearCmp = g_skewZeroCmp = g_skewTinyCmp = g_skewAliasCmp = 0;
   g_skewOracleCmp = 0;
   g_skewFlatCmp = g_skewFlatLeadCmp = g_skewBadNanCmp = g_skewCleanCmp = 0;
   g_skewCalmCmp = g_skewHistCmp = g_skewHistFlatCmp = 0;

#define SKEW_RUN(call) do { err = (call); if( err != TA_TEST_PASS ) return err; } while(0)
   SKEW_RUN( test_skew_jump() );
   SKEW_RUN( test_skew_near() );
   SKEW_RUN( test_skew_degenerate() );
   SKEW_RUN( test_skew_aliasing( history ) );
   SKEW_RUN( test_skew_contract( history ) );
   SKEW_RUN( test_skew_range( history ) );
   SKEW_RUN( test_skew_oracle( history ) );
   SKEW_RUN( test_skew_flat() );
   SKEW_RUN( test_skew_history() );
   SKEW_RUN( test_skew_nonfinite() );
#undef SKEW_RUN

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( history->nbBars == 252 )
   {
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "jump",                 12397, &g_skewJumpCmp     },
         { "jump flat",            12364, &g_skewJumpFlatCmp },
         { "jump skipped",             7, &g_skewJumpSkip    },
         { "near",                    50, &g_skewNearCmp     },
         { "zero",                   571, &g_skewZeroCmp     },
         { "tiny spread",              6, &g_skewTinyCmp     },
         { "alias",                 1160, &g_skewAliasCmp    },
         { "oracle",                  57, &g_skewOracleCmp   },
         { "flat",                 62124, &g_skewFlatCmp     },
         { "flat lead-in",         13518, &g_skewFlatLeadCmp },
         { "history",               1356, &g_skewHistCmp     },
         { "history flat",          2628, &g_skewHistFlatCmp },
         { "non-finite window",      195, &g_skewBadNanCmp   },
         { "clean window",          7303, &g_skewCleanCmp    },
         { "periodic",              1981, &g_skewCalmCmp     },
      };
      unsigned int c;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "SKEW Fail: the %s leg compared %d times, not the %d this "
                    "file was written with\n", cov[c].leg, *cov[c].got,
                    cov[c].want );
            return TA_SKEW_VACUOUS;
         }
      }
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) One value apart from n-1 equal ones has G1 = sqrt(n) exactly, above the
 * rest, and -sqrt(n) below, at every level and every gap: the window's shape
 * is fixed and the n-dependence of the moments cancels against the
 * coefficient's. g1 and b1 give other values, so this pins the estimator, and
 * the sign, which a symmetric window cannot. Jumps 2n bars apart put every
 * window either around exactly one jump or on the flat level, which must be
 * +0.0 after each jump leaves. The one-ulp and 1e-13 gaps are near-degenerate
 * windows with a known answer: scipy 1.18.1 gives -1.0623 at 31498938283.17 x
 * 29 plus the next double, and 2.8409 on 5.0 x 7 plus 5.0 + 1e-13, where the
 * answers are +sqrt(30) and +sqrt(8). A gap that rounds back to the level is
 * skipped and counted.
 */
static ErrorNumber test_skew_jump( void )
{
   static double in[12*103], out[12*103];
   static const double levels[]  = { 100.37, 31498938283.17, 5.0, -50.0 };
   static const int periods[]    = { 3, 4, 7, 8, 30, 49, 103 };
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int lv, g, pr, n, nbBars, i, j, bar, jumps;
   double c, up, down, want;

   for( lv = 0; lv < 4; lv++ )
   for( g = 0; g < 3; g++ )
   for( pr = 0; pr < 7; pr++ )
   {
      c = levels[lv];
      if( g == 0 )
      {
         up   = c + 1.0;
         down = c - 1.0;
      }
      else if( g == 1 )
      {
         up   = nextafter( c, INFINITY );
         down = nextafter( c, -INFINITY );
      }
      else
      {
         up   = c + 1e-13;
         down = c - 1e-13;
      }
      if( up == c || down == c )
      {
         g_skewJumpSkip++;
         continue;
      }

      n = periods[pr];
      nbBars = 12*n;
      for( i = 0; i < nbBars; i++ )
         in[i] = c;
      /* The first jump lands inside the first window, so a call opens on one. */
      for( i = n/2, j = 0; i < nbBars; i += 2*n, j++ )
         in[i] = j % 2 ? down : up;

      retCode = TA_SKEW( 0, nbBars-1, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != nbBars-n+1 )
      {
         printf( "SKEW jump Fail [level=%.17g N=%d]: rc=%d (%d,%d)\n", c, n,
                 (int)retCode, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      SKEW_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement, in, n, out );

      for( i = 0; i < nbElement; i++ )
      {
         bar = begIdx + i;
         jumps = 0;
         want = 0.0;
         for( j = bar-n+1; j <= bar; j++ )
         {
            if( in[j] != c )
            {
               jumps++;
               want = in[j] > c ? sqrt( (double)n ) : -sqrt( (double)n );
            }
         }
         if( jumps > 1 )
         {
            printf( "SKEW jump Fail [N=%d]: bar %d holds %d jumps\n", n, bar, jumps );
            return TA_SKEW_VACUOUS;
         }
         if( jumps == 0 )
         {
            g_skewJumpFlatCmp++;
            if( out[i] != 0.0 || signbit( out[i] ) )
            {
               printf( "SKEW jump Fail [level=%.17g N=%d] bar %d: %.17g on a flat "
                       "window, expected +0\n", c, n, bar, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            continue;
         }
         g_skewJumpCmp++;
         if( !(fabs( out[i] - want ) <= 1e-12 * fabs( want )) )
         {
            printf( "SKEW jump Fail [level=%.17g gap %d N=%d] bar %d: %.17g, "
                    "expected %.17g. One value apart from n-1 equal ones is "
                    "+-sqrt(n) at any level and gap: a miss is the estimator or "
                    "the sign, not the corpus\n", c, g, n, bar, out[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) The near-degenerate and near-symmetric goldens. Where the exact value is
 * 0 the result must be exactly 0, which the distance alone would pass at
 * either sign. */
static ErrorNumber skew_vs_exact( const char *tag, const double *x, int nbBars,
                                  int period, const double *golden, int nbGolden )
{
   static double out[SKEW_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_SKEW( 0, nbBars-1, x, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != nbGolden || begIdx != period-1 )
   {
      printf( "SKEW %s Fail: rc=%d (%d,%d), expected (%d,%d)\n", tag,
              (int)retCode, begIdx, nbElement, period-1, nbGolden );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   SKEW_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement, x, period, out );

   for( i = 0; i < nbGolden; i++ )
   {
      g_skewNearCmp++;
      if( !(SKEW_DIST( out[i], golden[i] ) <= 1e-12)
          || (golden[i] > 0.0) != (out[i] > 0.0)
          || (golden[i] < 0.0) != (out[i] < 0.0) )
      {
         printf( "SKEW %s Fail at bar %d: %.17g against exact %.17g\n", tag,
                 begIdx+i, out[i], golden[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_skew_near( void )
{
   ErrorNumber err;

   err = skew_vs_exact( "near-degenerate p=8", skewNearDegenerate,
                        (int)(sizeof(skewNearDegenerate)/sizeof(double)), 8,
                        skewNearDegenerate_8,
                        (int)(sizeof(skewNearDegenerate_8)/sizeof(double)) );
   if( err != TA_TEST_PASS )
      return err;

   err = skew_vs_exact( "near-symmetric p=8", skewNearSymmetric,
                        (int)(sizeof(skewNearSymmetric)/sizeof(double)), 8,
                        skewNearSymmetric_8,
                        (int)(sizeof(skewNearSymmetric_8)/sizeof(double)) );
   if( err != TA_TEST_PASS )
      return err;

   return skew_vs_exact( "near-symmetric p=5", skewNearSymmetric,
                         (int)(sizeof(skewNearSymmetric)/sizeof(double)), 5,
                         skewNearSymmetric_5,
                         (int)(sizeof(skewNearSymmetric_5)/sizeof(double)) );
}

/* (3) A series constant from bar 0 is +0.0 on every bar: a point mass is
 * symmetric. 0/0 would be NaN, so this pins the branch. Leg 8 covers the
 * levels whose window mean does not round back to the level. */
static ErrorNumber test_skew_degenerate( void )
{
   static double in[600], out[600];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   for( i = 0; i < 600; i++ )
      in[i] = 42.0;

   retCode = TA_SKEW( 0, 599, in, 30, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 29 || nbElement != 571 )
   {
      printf( "SKEW degenerate Fail: rc=%d (%d,%d), expected (29,571)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   SKEW_SERVER_VERIFY( 0, 599, 600, retCode, begIdx, nbElement, in, 30, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_skewZeroCmp++;
      if( out[i] != 0.0 || signbit( out[i] ) )
      {
         printf( "SKEW degenerate Fail at bar %d: %.17g, expected +0. A window "
                 "of equal values is symmetric\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Spreads small enough that the denominator underflows, to 0 or into the
    * subnormals, while the second moment does not. The output must be finite
    * (this function declares no NaN or Inf) and within the +-sqrt(n) every
    * window obeys: in the subnormals the cubes are mostly rounding. The last
    * window sits just above the smallest normal denominator and is still
    * sqrt(3) exactly, so the bound cannot be raised into valid values. */
   {
      static const double a = 5.3065825762526225e-109;
      static double tiny[5][5];
      static const int len[5] = { 3, 3, 5, 3, 3 };
      int k;

      tiny[0][0] = 0.0; tiny[0][1] = 0.0; tiny[0][2] = 1e-110;
      tiny[1][0] = 1e-100; tiny[1][1] = 1e-100; tiny[1][2] = nextafter( 1e-100, 1.0 );
      tiny[2][0] = -a; tiny[2][1] = a; tiny[2][2] = a; tiny[2][3] = -a; tiny[2][4] = -a;
      tiny[3][0] = 0.0; tiny[3][1] = 0.0; tiny[3][2] = 2.3e-108;
      tiny[4][0] = 0.0; tiny[4][1] = 0.0; tiny[4][2] = 5e-103;

      for( k = 0; k < 5; k++ )
      {
         retCode = TA_SKEW( 0, len[k]-1, tiny[k], len[k], &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != len[k]-1 || nbElement != 1 )
         {
            printf( "SKEW tiny spread Fail [%d]: rc=%d (%d,%d)\n", k,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         SKEW_SERVER_VERIFY( 0, len[k]-1, len[k], retCode, begIdx, nbElement,
                             tiny[k], len[k], out );
         g_skewTinyCmp++;
         if( !isfinite( out[0] ) || !(fabs( out[0] ) <= sqrt( (double)len[k] ) * (1.0 + 1e-9)) )
         {
            printf( "SKEW tiny spread Fail [%d]: %.17g on finite inputs, |G1| "
                    "<= sqrt(%d) on every window\n", k, out[0], len[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_skewTinyCmp++;
      if( !(fabs( out[0] - sqrt( 3.0 ) ) <= 1e-12 * sqrt( 3.0 )) )
      {
         printf( "SKEW tiny spread Fail: %.17g just above the smallest normal "
                 "denominator, expected sqrt(3)\n", out[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) In-place aliasing, outReal == inReal, bitwise. The rebuild re-reads the
 * window after this bar's store, so the store must not land under a read the
 * next bar still needs.
 */
static ErrorNumber test_skew_aliasing( const TA_History *history )
{
   static double clean[SKEW_CAP], alias[SKEW_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > SKEW_CAP )
      nbBars = SKEW_CAP;

   for( n = 3; n <= 39; n += 9 )
   {
      retCode = TA_SKEW( 0, nbBars-1, history->close, n, &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "SKEW alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_SKEW( 0, nbBars-1, alias, n, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "SKEW alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_skewAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "SKEW alias Fail [N=%d] bar %d: separate %.17g, in-place "
                    "%.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) The n-2 denominator is why the range starts at 3, and the contract is
 * what enforces it: there is no runtime branch for a short window. */
static ErrorNumber test_skew_contract( const TA_History *history )
{
   static double out[SKEW_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, lookback;

   if( nbBars > SKEW_CAP )
      nbBars = SKEW_CAP;

   for( n = -2; n <= 2; n++ )
   {
      retCode = TA_SKEW( 0, nbBars-1, history->close, n, &begIdx, &nbElement, out );
      if( retCode != TA_BAD_PARAM )
      {
         printf( "SKEW contract Fail: period %d gave rc=%d, expected "
                 "TA_BAD_PARAM -- n-2 is zero or negative below 3\n",
                 n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
   }

   retCode = TA_SKEW( 0, nbBars-1, history->close, 100001, &begIdx, &nbElement, out );
   lookback = TA_SKEW_Lookback( 100001 );
   if( retCode != TA_BAD_PARAM || lookback != -1 )
   {
      printf( "SKEW contract Fail: period 100001 gave rc=%d, lookback %d, "
              "expected TA_BAD_PARAM and -1\n", (int)retCode, lookback );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   retCode = TA_SKEW( 0, nbBars-1, history->close, 100000, &begIdx, &nbElement, out );
   lookback = TA_SKEW_Lookback( 100000 );
   if( retCode != TA_SUCCESS || lookback != 99999 )
   {
      printf( "SKEW contract Fail: period 100000 gave rc=%d, lookback %d, "
              "expected TA_SUCCESS and 99999\n", (int)retCode, lookback );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( n = 3; n <= 39; n += 9 )
   {
      lookback = TA_SKEW_Lookback( n );
      if( lookback != n-1 )
      {
         printf( "SKEW contract Fail: lookback(%d) = %d, expected %d\n",
                 n, lookback, n-1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      retCode = TA_SKEW( 0, nbBars-1, history->close, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != nbBars-(n-1) )
      {
         printf( "SKEW contract Fail [N=%d]: rc=%d (%d,%d), expected (%d,%d)\n",
                 n, (int)retCode, begIdx, nbElement, n-1, nbBars-(n-1) );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   return TA_TEST_PASS;
}

/* (6) The startIdx/endIdx range sweep. TA_STABLE_EPSILON: the shift and the
 * rebuild phase are seeded from each call's own startIdx, so two calls
 * covering one bar differ by up to about 5e-12 on this corpus. */
typedef struct { int period; const TA_Real *in; } SkewRangeParam;

static TA_RetCode skewRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   SkewRangeParam *p = (SkewRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_SKEW_Lookback( p->period );
   return TA_SKEW( startIdx, endIdx, p->in, p->period,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_skew_range( const TA_History *history )
{
   SkewRangeParam param;

   param.period = 30;
   param.in     = history->close;

   return doRangeTestEx( skewRangeTestFunction, TA_STABLE_EPSILON,
                         TA_TEST_UNST_NONE, (void *)&param, 1, 0 );
}

/* (7) The frozen external-oracle goldens and Excel's published example. */
static ErrorNumber test_skew_oracle( const TA_History *history )
{
   static TA_Real out[SKEW_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode retCode;
   int k, lastPeriod = -1;
   const char *which;
   double got, want, tol;

   retCode = TA_SKEW( 0, 9, skewExcel, 10, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 9 || nbElement != 1 )
   {
      printf( "SKEW Excel Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   SKEW_SERVER_VERIFY( 0, 9, 10, retCode, begIdx, nbElement, skewExcel, 10, out );
   g_skewOracleCmp++;
   if( !(fabs( out[0] - SKEW_EXCEL ) <= 5e-7) )
   {
      printf( "SKEW Excel Fail: SKEW(3,4,5,2,3,4,5,6,4,7) is %.17g, Microsoft "
              "publishes %.6f\n", out[0], SKEW_EXCEL );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( history->nbBars != 252 )
      return TA_TEST_PASS;

   for( k = 0; k < NB_SKEW_GOLDEN; k++ )
   {
      const SkewGolden *g = &skewGoldens[k];

      if( g->period != lastPeriod )
      {
         lastPeriod = g->period;
         retCode = TA_SKEW( 0, 251, history->close, lastPeriod,
                            &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod-1
             || nbElement != 252-begIdx )
         {
            printf( "SKEW oracle Fail [N=%d]: rc=%d (%d,%d)\n", lastPeriod,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
      }

      /* A golden's bar is hand-transcribed and indexes `out` unchecked. */
      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "SKEW oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", g->period, g->bar, begIdx, begIdx + nbElement - 1 );
         return TA_SKEW_VACUOUS;
      }

      got = out[g->bar - begIdx];
      g_skewOracleCmp++;
      which = NULL;
      if( !(SKEW_DIST( got, g->exact ) <= SKEW_EXACT_TOL) )
      {
         which = "exact";
         want  = g->exact;
         tol   = SKEW_EXACT_TOL;
      }
      else if( !(SKEW_DIST( got, g->scipy ) <= SKEW_ORACLE_TOL) )
      {
         which = "scipy";
         want  = g->scipy;
         tol   = SKEW_ORACLE_TOL;
      }
      else if( !(SKEW_DIST( got, g->pta ) <= SKEW_ORACLE_TOL) )
      {
         which = "pandas-ta-classic";
         want  = g->pta;
         tol   = SKEW_ORACLE_TOL;
      }
      if( which )
      {
         printf( "SKEW oracle Fail [N=%d] at bar %d: got %.17g, %s expects "
                 "%.17g (distance %.3e over %.0e)\n", g->period, g->bar, got,
                 which, want, SKEW_DIST( got, want ), tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Every window of one call against the same window computed with no history,
 * and a flat window must be +0.0 from both. */
static ErrorNumber skew_vs_fresh( const char *tag, const double *in, int nbBars,
                                  int period, int *counter, int *flatCounter )
{
   static TA_Real out[SKEW_FRESH_CAP];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   int i, j, bar, flat;

   retCode = TA_SKEW( 0, nbBars-1, in, period, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != period-1
       || nbElement != nbBars-period+1 )
   {
      printf( "SKEW %s Fail [N=%d]: rc=%d (%d,%d)\n", tag, period,
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   SKEW_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
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
            printf( "SKEW %s Fail [N=%d]: bar %d is a flat window this leg was "
                    "not written to reach\n", tag, period, bar );
            return TA_SKEW_VACUOUS;
         }
         (*flatCounter)++;
         if( out[i] != 0.0 || signbit( out[i] ) )
         {
            printf( "SKEW %s Fail [N=%d] bar %d: %.17g on a flat window at "
                    "%.17g, expected +0\n", tag, period, bar, out[i], in[bar] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }

      retCode = TA_SKEW( bar, bar, in, period, &begOne, &nbOne, &one );
      if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
      {
         printf( "SKEW %s Fail [N=%d]: one-bar call at %d rc=%d (%d,%d)\n",
                 tag, period, bar, (int)retCode, begOne, nbOne );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      (*counter)++;
      if( !(SKEW_DIST( out[i], one ) <= SKEW_FRESH_TOL) )
      {
         printf( "SKEW %s Fail [N=%d] bar %d: %.17g against %.17g with no "
                 "history, distance %.3e over %.0e\n", tag, period, bar,
                 out[i], one, SKEW_DIST( out[i], one ), SKEW_FRESH_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static unsigned int skew_lcg( unsigned int *seed )
{
   *seed = (*seed * 1103515245u + 12345u) & 0x7fffffffu;
   return *seed >> 8;
}

/* (8) A flat stretch after a walk is +0.0 at every level, including levels
 * whose window mean does not round back to the level. Anchored on that mean,
 * every deviation is the same rounding residue, and the second moment is its
 * cancellation: a value that is not exactly 0, so the branch would not fire.
 * 49, 98 and 103 are periods where n * (1/n) != 1 in double, 2000 is long
 * enough for the sum's own rounding to reach many ulps, and 3 is the shortest
 * period. All but 2000 run past the periodic rebuild.
 */
#define SKEW_FLAT_BARS 6000
static ErrorNumber test_skew_flat( void )
{
   static const double levels[] = { 0.1, 100.0, 100.37, 3.3, 31498938283.17,
                                    3.7e-67 };
   static const int periods[]   = { 3, 49, 98, 103, 2000 };
   static TA_Real in[SKEW_FLAT_BARS];
   ErrorNumber err;
   unsigned int seed;
   int lv, pr, n, i, nbBars;

   for( lv = 0; lv < 6; lv++ )
   for( pr = 0; pr < 5; pr++ )
   {
      n = periods[pr];
      nbBars = 35*n;
      if( nbBars > SKEW_FLAT_BARS )
         nbBars = SKEW_FLAT_BARS;

      seed = 99u;
      for( i = 0; i < n; i++ )
         in[i] = levels[lv] * (1.0 + 1e-3*((double)(skew_lcg( &seed ) % 2001u) - 1000.0)*1e-3);
      for( ; i < nbBars; i++ )
         in[i] = levels[lv];

      err = skew_vs_fresh( "flat", in, nbBars, n, &g_skewFlatLeadCmp,
                           &g_skewFlatCmp );
      if( err != TA_TEST_PASS )
         return err;
   }

   return TA_TEST_PASS;
}

/* (9) History. After a flat lead-in, the series jumps away and decays
 * geometrically back onto its first value, which is the shift until a rebuild,
 * so once the spread has gone the running sums hold nothing but the rounding
 * left by the values that departed. A trigger that compares against the
 * current sums rather than the largest held since the last rebuild misses it.
 */
static ErrorNumber test_skew_history( void )
{
   static const double levels[] = { 100.37, 0.1, 1.0, 3.3 };
   static const double ratios[] = { 0.1, 0.05, 0.2 };
   static const int periods[]   = { 3, 5, 8, 12, 20 };
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
         err = skew_vs_fresh( "history", in, 75, periods[pr],
                              &g_skewHistCmp, &g_skewHistFlatCmp );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   return TA_TEST_PASS;
}

/* (10) A non-finite input poisons the running sums, and the periodic rebuild
 * is the only way out: every trigger comparison is false on NaN. A window
 * holding the value is NaN, the output may stay NaN for up to 32n bars after
 * it leaves, and from then on every window must match a no-history recompute
 * again. Not sent to the servers: a NaN's bits are no contract. The servers
 * get a finite control instead: on stationary noise at period 20 the trigger
 * never fires, so the periodic rebuild alone moves the output bits they
 * compare.
 */
#define SKEW_BAD_BARS 4600
#define SKEW_CALM_BARS 2000
static ErrorNumber test_skew_nonfinite( void )
{
   static const int periods[] = { 5, 20, 40 };
   static const int badAt[]   = { 100, 1600, 3100 };
   static TA_Real in[SKEW_BAD_BARS], out[SKEW_BAD_BARS], calm[SKEW_CALM_BARS];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   ErrorNumber err;
   unsigned int seed;
   int pr, n, i, k, bar, held, graceEnd;
   double v;

   seed = 31337u;
   for( i = 0; i < SKEW_CALM_BARS; i++ )
      calm[i] = 100.0 + 1e-2*(((double)(skew_lcg( &seed ) % 2001u) - 1000.0)/1000.0);
   err = skew_vs_fresh( "periodic", calm, SKEW_CALM_BARS, 20, &g_skewCalmCmp, NULL );
   if( err != TA_TEST_PASS )
      return err;

   seed = 2718u;
   v    = 100.0;
   for( i = 0; i < SKEW_BAD_BARS; i++ )
   {
      v += ((double)(skew_lcg( &seed ) % 2001u) - 1000.0) * 5e-4;
      in[i] = v;
   }
   in[badAt[0]] = NAN;
   in[badAt[1]] = INFINITY;
   in[badAt[2]] = -INFINITY;

   for( pr = 0; pr < 3; pr++ )
   {
      n = periods[pr];
      retCode = TA_SKEW( 0, SKEW_BAD_BARS-1, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1 || nbElement != SKEW_BAD_BARS-n+1 )
      {
         printf( "SKEW non-finite Fail [N=%d]: rc=%d (%d,%d)\n", n,
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
            g_skewBadNanCmp++;
            if( !isnan( out[i] ) )
            {
               printf( "SKEW non-finite Fail [N=%d] bar %d: %.17g on a window "
                       "holding a non-finite input, expected NaN\n", n, bar, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            continue;
         }

         retCode = TA_SKEW( bar, bar, in, n, &begOne, &nbOne, &one );
         if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
         {
            printf( "SKEW non-finite Fail [N=%d]: one-bar call at %d rc=%d "
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
            g_skewCleanCmp++;
         if( !(SKEW_DIST( out[i], one ) <= SKEW_FRESH_TOL) )
         {
            printf( "SKEW non-finite Fail [N=%d] bar %d: %.17g against %.17g "
                    "with no history\n", n, bar, out[i], one );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}
