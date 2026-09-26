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
 *  092526 MF,CC  First version (#449).
 */

/* Description:
 *
 *   Test TA_PERCENTB (Bollinger Bands %B): (x - lower) / (upper - lower) over
 *   the bands TA_BBANDS returns, and 0.5 where upper == lower.
 *
 *   Legs:
 *     1. Goldens: the exact value, from code that never runs TA-Lib, and
 *        Bollinger's own printed example.
 *     2. Composition: bitwise equal to that quotient over TA_BBANDS' outputs
 *        at every MA type, deviation pair, startIdx and unstable period, and
 *        TA_S_PERCENTB against TA_S_BBANDS. Only this leg sees a change that
 *        moves the last bit.
 *     3. The author's identities through TA_MA and TA_STDDEV, and the exact
 *        band crossings at a period of 2.
 *     4. The lookback tier against TA_BBANDS_Lookback, unstable periods too.
 *     5. In-place, across a periodic rebuild and a tile boundary.
 *     6. startIdx-anchored calls equal the series cut at startIdx - lookback.
 *     7. The parameter ranges and the defaults.
 *     8. Flat, alternating and sub-ulp windows, bar by bar.
 *     9. The stream against the batch, where kUp = -kDn turns a one-ulp band
 *        difference into a %B near 1e15.
 *
 *   WHAT EACH LEG CAN SEE, measured by mutating the base body of
 *   ta_codegen/input/percentb/percentb.c (the default, percentb.yaml) and
 *   regenerating: every leg that went red with test_abstract skipped, and
 *   test_abstract where it goes red first.
 *
 *     SMA window one bar short                     1 2 3 8 9
 *     general path's variance over n-1 bars        1 2 3 4 8 9
 *     width upper - middle                         1 2 3 8 9
 *     width 2*k*sd, not from the rounded bands     1 2 8 9
 *     guard dropped, SMA path                      1 2 3 8 9, test_abstract
 *     guard dropped, general path                  1 2 8 9
 *     guard on k*sd instead of the width           1 2 8
 *     guard |width| < 1e-14                        1 2
 *     guard width <= 0                             1 2 3 9
 *     0.0 at a zero width                          1 2 3 8 9
 *     sample variance, n/(n-1)                     1 2 3 8 9
 *     periodic rebuild one bar late                2 3 9
 *     kUp and kDn swapped                          1 2 3 9
 *     default period 21                            1 3 7
 *     lookback + 1                                 2 4 6 7 9, test_abstract
 *     lookback restated as n - 1                   2 3 4 8
 *     variance entered at startIdx                 1 2 3 4 5 8
 *     offset 0 on the general path                 2 3
 *     the fold ((x - M) + k*sd) / (2*k*sd)         2 9
 *     unfused upper band                           2 9
 *     middle band as maTotal * (1/n)               2 9
 *     equal-k branch dropped                       2
 *     x read from inReal in the SMA tile pass      5, test_abstract
 *     x copied after the variance, general path    5
 *
 *   No leg sees 255-bar tiles for 256: the tile only sets where the band
 *   pass splits the output.
 *
 *   SERVER_VERIFY: every golden call but the two 100500-bar ones, which do not
 *   fit server_verify's 256 KB request; leg 2 routes the 3000-bar walk once per
 *   MA type and branch and under each unstable period, leg 6 one anchored call
 *   per MA type and branch.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"
#include "ta_test_reference.h"

/**** Local declarations. ****/
#define PCTB_CAP         3100
#define PCTB_WALK_N      3000
#define PCTB_LONG_N      100500
#define PCTB_ULP         ( DBL_EPSILON / 2.0 )
#define PCTB_SV_MAX_BARS 16000
#define PCTB_NB_MATYPE   14

enum { PCTB_CORPUS, PCTB_NEGCORPUS, PCTB_TICK100, PCTB_BIG1E8, PCTB_FINE100, PCTB_PEG,
       PCTB_FLATPOS, PCTB_FLATNEG, PCTB_STEPFLAT, PCTB_ZERO, PCTB_ALT, PCTB_EPS, PCTB_LONG,
       PCTB_SUBULP, PCTB_HD, PCTB_WALK, PCTB_NEGWALK, PCTB_INTCORPUS, PCTB_NANWALK,
       PCTB_INFWALK, PCTB_NB_SERIES };

typedef struct { const char *name; const double *x; int nbBars; } PctbSeries;

static PctbSeries pctbSeries[PCTB_NB_SERIES];

static double pctbNeg[252], pctbTick[300], pctbBig[300], pctbFine[300];
static double pctbPeg[TA_TEST_REF_PEG_N], pctbPegWalk[TA_TEST_REF_PEG_N];
static double pctbFlatPos[60], pctbFlatNeg[60], pctbStep[80], pctbZero[60], pctbAlt[60];
static double pctbEps[200], pctbLong[PCTB_LONG_N], pctbSubUlp[100], pctbInt[252];
static double pctbWalk[PCTB_WALK_N], pctbNegWalk[PCTB_WALK_N];
static double pctbNanWalk[PCTB_WALK_N], pctbInfWalk[PCTB_WALK_N];

/* Bollinger Capital Management, "Bollinger Bands Tutorial", page 7 (Wayback
 * 20090203122641): Home Depot, 1/27/99 to 2/24/99, as printed. */
static const double pctbHd[20] = { 60.25, 60.0, 60.4063, 59.125, 57.625, 58.625, 57.625,
                                   55.875, 55.5, 54.25, 55.5313, 58.25, 56.875, 58.0313,
                                   58.0, 58.4063, 60.5, 61.5, 60.5313, 59.75 };

/* Leg 1. Captured by ta-lib-oracles/capture_449_percentb.py at commit
 * 395a0b6067644e01688691e23e8e3550f1813f49, which regenerates these rows
 * (its goldens.md) and never runs TA-Lib. `bar` is the ABSOLUTE bar index.
 *
 *   q     : kDn/K + ((x - M)/K) / sqrt(V), K = kUp + kDn and V the window's
 *           population variance, over the exact input doubles in rational
 *           arithmetic, proved the nearest double (Python 3.12.3); 0.5 where
 *           V = 0 or K = 0. M is the exact SMA, WMA, EMA or DEMA, seeded where
 *           TA_MA seeds. M and sigma are frozen as the doubles nearest the
 *           exact middle band and sqrt(V).
 *   ts    : trading-signals 8.3.0 PercentB (node v22.21.1). SMA, one multiplier,
 *           0.5 at a zero width.
 *   ta4j  : ta4j 0.22.6 PercentBIndicator, DoubleNum (OpenJDK 21.0.12.1). SMA,
 *           one multiplier.
 *   lean  : LEAN 2.5.18090 (commit 5b0c9975d3189efe4353e5071374b77ab9681bdb)
 *           BollingerBands(n, k, type).PercentB, System.Decimal.
 *
 * ts, ta4j and lean are provenance, not compared: the capture froze a value
 * only within its arm gate of q, or where the arm's own rule gives 0.5 at a
 * zero width. NAN: the arm does not compute that bar, or sits outside that
 * gate there. pandas-ta-classic 0.6.52 was compared and is not frozen: it adds
 * float epsilon to every width of a series once one width is 0.
 *
 * TOLERANCE, kind 0: |got - q| <= tol, with
 *   tol = eV*|q - kDn/K| + eM*|M|/(|K|*sigma) + own.
 * own is the literal quotient's own cancellation, 8u*(|x| + (1+|q|)*(|U|+|L|))
 * / |U - L| on the exact bands, rounded up. eV and eM are what the deviation's
 * and the middle band's errors do to %B: the group's largest relative distance
 * of TA_STDDEV(n, 1) from sigma and of TA_MA and TA_BBANDS' middle band from M,
 * floored at 1e-15 and failing above 1e-6. Where M is 0 the group's largest
 * absolute middle-band distance stands in for eM*|M|.
 * kind 1: V = 0 or K = 0, so the exact bands coincide. kind 2: V > 0, but
 * k*sigma is under half an ulp of M, so the ROUNDED bands coincide (the exact
 * %B is in the row's comment). Both are 0.5, bitwise.
 * Worst measured: 0.74 of tol, on long-sma-n100000.
 */
typedef struct { int bar, kind; double q, M, sigma, own, ts, ta4j, lean; } PctbRow;

typedef struct
{
   const char *tag;
   int series, startIdx, endIdx, n;
   double kUp, kDn;
   TA_MAType maType;
   unsigned int unstEma;
   int beg, nb;
   const PctbRow *rows;
   int nbRows;
} PctbGolden;

#define PCTB_ROWS(a) (a), (int)(sizeof(a)/sizeof((a)[0]))

/* bar, kind, q, M, sigma, own, ts, ta4j, lean */
static const PctbRow pctb_sma_n2[] =
{
   {      1, 0,                     0.75,       93.157499999999999,       1.6574999999999989, 5.7e-14,                     0.75,                     0.75,                     0.75 },
   {      2, 0,                     0.25,       94.594999999999999,      0.21999999999999886, 3.4e-13,                     0.25,                     0.25,                     0.25 },
   {      3, 0,                     0.75,       94.734999999999999,      0.35999999999999943, 2.7e-13,                     0.75,      0.75000000000000988,                     0.75 },
   {    100, 0,                     0.25,                 117.0625,                   1.0625, 8.6e-14,                     0.25,      0.24999999999998998,                     0.25 },
   {    101, 1,                      0.5,                    116.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {    102, 0,                     0.25,                    114.0,                      2.0, 4.5e-14,                     0.25,      0.24999999999999289,                     0.25 },
   {    131, 0,                     0.75,                  137.845,     0.034999999999996589, 4.0e-12,                     0.75,      0.74999999999959399,                     0.75 },
   {    200, 0,                     0.75,                   107.06,     0.060000000000002274, 1.8e-12,                     0.75,      0.74999999999976319,                     0.75 },
   {    213, 0,                     0.25,       90.905000000000001,      0.65500000000000114, 1.1e-13,                     0.25,       0.2499999999999783,                     0.25 },
   {    239, 0,                     0.25,       109.47499999999999,      0.22500000000000142, 3.8e-13,      0.25000000000000788,      0.24999999999994474,                     0.25 },
   {    243, 0,                     0.25,       109.59999999999999,      0.39999999999999858, 2.2e-13,      0.25000000000000444,      0.24999999999996891,                     0.25 },
   {    251, 0,                     0.25,                   108.31,      0.43999999999999773, 2.0e-13,                     0.25,      0.24999999999996769,                     0.25 },
};

static const PctbRow pctb_sma_n2_k13[] =
{
   {      1, 0,                      1.0,       93.157499999999999,       1.6574999999999989, 6.2e-14,                      NAN,                      NAN,                      NAN },
   {      2, 0,                      0.5,       94.594999999999999,      0.21999999999999886, 3.9e-13,                      NAN,                      NAN,                      NAN },
   {      3, 0,                      1.0,       94.734999999999999,      0.35999999999999943, 3.0e-13,                      NAN,                      NAN,                      NAN },
   {    100, 0,                      0.5,                 117.0625,                   1.0625, 9.7e-14,                      NAN,                      NAN,                      NAN },
   {    101, 1,                      0.5,                    116.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {    102, 0,                      0.5,                    114.0,                      2.0, 5.0e-14,                      NAN,                      NAN,                      NAN },
   {    131, 0,                      1.0,                  137.845,     0.034999999999996589, 4.4e-12,                      NAN,                      NAN,                      NAN },
   {    200, 0,                      1.0,                   107.06,     0.060000000000002274, 2.0e-12,                      NAN,                      NAN,                      NAN },
   {    239, 0,                      0.5,       109.47499999999999,      0.22500000000000142, 4.4e-13,                      NAN,                      NAN,                      NAN },
   {    251, 0,                      0.5,                   108.31,      0.43999999999999773, 2.2e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n2_k25_15[] =
{
   {      1, 0,                    0.625,       93.157499999999999,       1.6574999999999989, 5.4e-14,                      NAN,                      NAN,                      NAN },
   {      2, 0,                    0.125,       94.594999999999999,      0.21999999999999886, 3.2e-13,                      NAN,                      NAN,                      NAN },
   {      3, 0,                    0.625,       94.734999999999999,      0.35999999999999943, 2.5e-13,                      NAN,                      NAN,                      NAN },
   {    100, 0,                    0.125,                 117.0625,                   1.0625, 8.0e-14,                      NAN,                      NAN,                      NAN },
   {    101, 1,                      0.5,                    116.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {    102, 0,                    0.125,                    114.0,                      2.0, 4.2e-14,                      NAN,                      NAN,                      NAN },
   {    131, 0,                    0.625,                  137.845,     0.034999999999996589, 3.8e-12,                      NAN,                      NAN,                      NAN },
   {    177, 0,                    0.125,                   130.97,      0.96999999999999886, 9.8e-14,                      NAN,                      NAN,                      NAN },
   {    200, 0,                    0.625,                   107.06,     0.060000000000002274, 1.7e-12,                      NAN,                      NAN,                      NAN },
   {    251, 0,                    0.125,                   108.31,      0.43999999999999773, 1.8e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n5[] =
{
   {      4, 0,      0.47413751814102489,       93.912999999999997,        1.285646141051261, 6.5e-14,      0.47413751814102817,      0.47413751814102817,      0.47413751814102456 },
   {      5, 0,      0.54873987057769658,       94.537999999999997,      0.44624656861425743, 2.0e-13,      0.54873987057770623,      0.54873987057770623,      0.54873987057769624 },
   {      6, 0,     0.061340149179918257,       94.081000000000003,      0.88394230580960353, 7.4e-14,     0.061340149179921956,     0.061340149179921956,     0.061340149179918216 },
   {     10, 0,      0.94040389277885694,       92.837999999999994,       1.8659008548151754, 5.5e-14,      0.94040389277885639,      0.94040389277885639,      0.94040389277885816 },
   {     75, 0,      0.99307966536109349,       87.325999999999993,        5.031641282921508, 2.0e-14,      0.99307966536109404,      0.99307966536109471,      0.99307966536109327 },
   {    100, 0,      0.64918299020038406,                  114.357,       2.7533299838559127, 4.0e-14,      0.64918299020038406,      0.64918299020038273,      0.64918299020038417 },
   {    121, 0,      0.22374236390788738,                   122.91,      0.31673332631726736, 3.0e-13,      0.22374236390789362,      0.22374236390785998,      0.22374236390788621 },
   {    176, 0,      0.18716777109682045,                  133.476,       1.2274950101731588, 8.2e-14,      0.18716777109681434,      0.18716777109680857,      0.18716777109682101 },
   {    177, 0,      0.11540661252764806,       132.52600000000001,       1.6419939098547234, 5.8e-14,      0.11540661252764528,      0.11540661252763662,      0.11540661252764711 },
   {    180, 0,      0.30929462834371119,       128.91200000000001,       2.3491734716704054, 4.4e-14,      0.30929462834371002,      0.30929462834370092,      0.30929462834371108 },
   {    200, 0,      0.58304673141963914,                   106.81,      0.93320951559657861, 1.1e-13,      0.58304673141964281,      0.58304673141962371,      0.58304673141963859 },
   {    202, 0,    0.0006218403462698468,                  103.998,        6.507092745612284, 1.1e-14,   0.00062184034626966954,   0.00062184034626803163,   0.00062184034626960882 },
   {    251, 0,     0.091939955283464284,                  109.036,      0.71435565371878917, 1.1e-13,     0.091939955283460079,      0.09193995528345511,     0.091939955283463659 },
};

static const PctbRow pctb_sma_n5_k13[] =
{
   {      4, 0,      0.72413751814102489,       93.912999999999997,        1.285646141051261, 7.2e-14,                      NAN,                      NAN,                      NAN },
   {      5, 0,      0.79873987057769658,       94.537999999999997,      0.44624656861425743, 2.2e-13,                      NAN,                      NAN,                      NAN },
   {      6, 0,      0.31134014917991826,       94.081000000000003,      0.88394230580960353, 8.5e-14,                      NAN,                      NAN,                      NAN },
   {     75, 0,       1.2430796653610934,       87.325999999999993,        5.031641282921508, 2.1e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.89918299020038406,                  114.357,       2.7533299838559127, 4.4e-14,                      NAN,                      NAN,                      NAN },
   {    121, 0,      0.47374236390788738,                   122.91,      0.31673332631726736, 3.4e-13,                      NAN,                      NAN,                      NAN },
   {    136, 0,      0.26704228102576449,       134.53800000000001,        3.254943317478816, 3.2e-14,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.83304673141963914,                   106.81,      0.93320951559657861, 1.2e-13,                      NAN,                      NAN,                      NAN },
   {    202, 0,      0.25062184034626983,                  103.998,        6.507092745612284, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    251, 0,      0.34193995528346427,                  109.036,      0.71435565371878917, 1.3e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n5_k25_15[] =
{
   {      4, 0,      0.34913751814102489,       93.912999999999997,        1.285646141051261, 6.1e-14,                      NAN,                      NAN,                      NAN },
   {      5, 0,      0.42373987057769663,       94.537999999999997,      0.44624656861425743, 1.9e-13,                      NAN,                      NAN,                      NAN },
   {      6, 0,    -0.063659850820081743,       94.081000000000003,      0.88394230580960353, 7.4e-14,                      NAN,                      NAN,                      NAN },
   {     52, 0,    -0.093041836594882604,       88.724999999999994,       2.3742535669131883, 2.7e-14,                      NAN,                      NAN,                      NAN },
   {     75, 0,      0.86807966536109349,       87.325999999999993,        5.031641282921508, 2.0e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.52418299020038406,                  114.357,       2.7533299838559127, 3.8e-14,                      NAN,                      NAN,                      NAN },
   {    121, 0,     0.098742363907887376,                   122.91,      0.31673332631726736, 2.8e-13,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.45804673141963909,                   106.81,      0.93320951559657861, 1.0e-13,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.12437815965373016,                  103.998,        6.507092745612284, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    251, 0,    -0.033060044716535723,                  109.036,      0.71435565371878917, 1.1e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20[] =
{
   {     19, 0,      0.20901483689438513,       92.891000000000005,       2.5911974066056795, 2.7e-14,      0.20901483689438599,      0.20901483689438599,      0.20901483689438516 },
   {     20, 0,      0.10497215382837141,       92.734750000000005,       2.7591409328811025, 2.4e-14,       0.1049721538283722,       0.1049721538283722,      0.10497215382837104 },
   {     21, 0,     0.094400825593143223,       92.375249999999994,       2.9279213936682114, 2.2e-14,      0.09440082559314289,     0.094400825593144097,     0.094400825593143042 },
   {     45, 0,      0.92645605936132136,       86.896249999999995,       2.2591717923832175, 4.2e-14,      0.92645605936131814,      0.92645605936131969,      0.92645605936132081 },
   {     90, 0,       1.0173037777133378,       102.19750000000001,       10.053328864112624, 1.2e-14,       1.0173037777133378,       1.0173037777133374,       1.0173037777133391 },
   {    100, 0,      0.64476467283312766,       112.76949999999999,       5.5788818100045816, 2.0e-14,      0.64476467283312666,      0.64476467283312788,      0.64476467283312766 },
   {    139, 0,      0.19049785694523211,       131.20249999999999,        5.163534521042731, 1.9e-14,       0.1904978569452325,       0.1904978569452325,        0.190497856945232 },
   {    167, 0,      0.93806724364968652,                 123.5585,       2.1066971187145058, 6.4e-14,      0.93806724364968519,      0.93806724364968852,      0.93806724364968563 },
   {    200, 0,      0.15307836107723372,                 116.3365,       6.6416295251993684, 1.3e-14,      0.15307836107723494,      0.15307836107723494,      0.15307836107723363 },
   {    202, 0,     -0.18854402820424501,                  113.877,       8.3062952632325793, 9.7e-15,     -0.18854402820424454,     -0.18854402820424454,     -0.18854402820424498 },
   {    223, 0,        1.294567303403535,       95.155000000000001,       2.7640830305907955, 4.4e-14,       1.2945673034035345,       1.2945673034035372,       1.2945673034035341 },
   {    234, 0,       1.0062361752288422,                  102.486,       6.9799436960479841, 1.7e-14,       1.0062361752288418,        1.006236175228844,       1.0062361752288425 },
   {    251, 0,      0.27611909512545263,       110.57000000000001,       3.0149958540601682, 2.9e-14,      0.27611909512545441,      0.27611909512545563,      0.27611909512545252 },
};

static const PctbRow pctb_sma_n20_k13[] =
{
   {     19, 0,      0.45901483689438516,       92.891000000000005,       2.5911974066056795, 3.1e-14,                      NAN,                      NAN,                      NAN },
   {     20, 0,      0.35497215382837138,       92.734750000000005,       2.7591409328811025, 2.7e-14,                      NAN,                      NAN,                      NAN },
   {     21, 0,      0.34440082559314322,       92.375249999999994,       2.9279213936682114, 2.5e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.89476467283312766,       112.76949999999999,       5.5788818100045816, 2.1e-14,                      NAN,                      NAN,                      NAN },
   {    139, 0,      0.44049785694523214,       131.20249999999999,        5.163534521042731, 2.1e-14,                      NAN,                      NAN,                      NAN },
   {    167, 0,       1.1880672436496864,                 123.5585,       2.1066971187145058, 7.0e-14,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.40307836107723372,                 116.3365,       6.6416295251993684, 1.4e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     0.061455971795755002,                  113.877,       8.3062952632325793, 8.5e-15,                      NAN,                      NAN,                      NAN },
   {    223, 0,        1.544567303403535,       95.155000000000001,       2.7640830305907955, 4.7e-14,                      NAN,                      NAN,                      NAN },
   {    251, 0,      0.52611909512545263,       110.57000000000001,       3.0149958540601682, 3.3e-14,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_k25_15[] =
{
   {     19, 0,     0.084014836894385145,       92.891000000000005,       2.5911974066056795, 2.6e-14,                      NAN,                      NAN,                      NAN },
   {     20, 0,    -0.020027846171628592,       92.734750000000005,       2.7591409328811025, 2.3e-14,                      NAN,                      NAN,                      NAN },
   {     21, 0,    -0.030599174406856774,       92.375249999999994,       2.9279213936682114, 2.2e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.51976467283312766,       112.76949999999999,       5.5788818100045816, 1.9e-14,                      NAN,                      NAN,                      NAN },
   {    140, 0,     -0.02112050362943823,       131.19650000000001,       5.1729839309628618, 1.8e-14,                      NAN,                      NAN,                      NAN },
   {    167, 0,      0.81306724364968652,                 123.5585,       2.1066971187145058, 6.2e-14,                      NAN,                      NAN,                      NAN },
   {    200, 0,     0.028078361077233702,                 116.3365,       6.6416295251993684, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.31354402820424498,                  113.877,       8.3062952632325793, 1.1e-14,                      NAN,                      NAN,                      NAN },
   {    223, 0,        1.169567303403535,       95.155000000000001,       2.7640830305907955, 4.2e-14,                      NAN,                      NAN,                      NAN },
   {    251, 0,       0.1511190951254526,       110.57000000000001,       3.0149958540601682, 2.7e-14,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n50[] =
{
   {     49, 0,      0.57430580385550412,       89.326800000000006,       3.9472017632748391, 2.1e-14,      0.57430580385550456,      0.57430580385550456,        0.574305803855504 },
   {     50, 0,      0.48428261282091839,                  89.2774,       3.9351324297919121, 2.0e-14,      0.48428261282091928,      0.48428261282091928,      0.48428261282091828 },
   {     51, 0,      0.47779568260636734,       89.157399999999996,       3.8551061256468673, 2.1e-14,      0.47779568260636857,      0.47779568260636857,      0.47779568260636751 },
   {     53, 0,      0.15752586474406871,       88.723600000000005,       3.8131346737297385, 1.7e-14,      0.15752586474406899,       0.1575258647440699,      0.15752586474406882 },
   {     63, 0,      0.84763653389212223,       87.521199999999993,       2.8613218204179689, 3.3e-14,      0.84763653389211768,      0.84763653389212634,      0.84763653389212201 },
   {     74, 0,      0.35527694931148324,       87.690699999999993,       3.0242245303548478, 2.4e-14,      0.35527694931148157,      0.35527694931148862,      0.35527694931148351 },
   {     75, 0,       1.2279049576103744,       88.008200000000002,       3.1741094436077657, 3.5e-14,       1.2279049576103724,        1.227904957610378,       1.2279049576103735 },
   {     77, 0,       1.4623155040068525,       88.630700000000004,       4.2369939827665553, 2.9e-14,        1.462315504006853,       1.4623155040068556,       1.4623155040068516 },
   {     93, 0,       1.0312424194197467,       96.185500000000005,       10.854037232753534, 1.1e-14,       1.0312424194197469,       1.0312424194197476,       1.0312424194197485 },
   {    100, 0,      0.83101272686236249,       99.652500000000003,       12.346579657945759, 8.7e-15,      0.83101272686236238,      0.83101272686236349,      0.83101272686236138 },
   {    200, 0,     -0.02274911980063091,                   122.53,       7.3696919881362746, 1.1e-14,    -0.022749119800630914,    -0.022749119800631386,    -0.022749119800630636 },
   {    202, 0,     -0.36724024863884291,       121.63760000000001,       8.8319240395284204, 1.1e-14,     -0.36724024863884314,     -0.36724024863884314,      -0.3672402486388428 },
   {    251, 0,       0.6649180196893143,                 102.7762,       7.7217153249779935, 1.3e-14,      0.66491801968931474,      0.66491801968931341,       0.6649180196893143 },
};

static const PctbRow pctb_sma_n50_k13[] =
{
   {     49, 0,      0.82430580385550412,       89.326800000000006,       3.9472017632748391, 2.3e-14,                      NAN,                      NAN,                      NAN },
   {     50, 0,      0.73428261282091845,                  89.2774,       3.9351324297919121, 2.2e-14,                      NAN,                      NAN,                      NAN },
   {     51, 0,       0.7277956826063674,       89.157399999999996,       3.8551061256468673, 2.3e-14,                      NAN,                      NAN,                      NAN },
   {     75, 0,       1.4779049576103744,       88.008200000000002,       3.1741094436077657, 3.7e-14,                      NAN,                      NAN,                      NAN },
   {     77, 0,       1.7123155040068525,       88.630700000000004,       4.2369939827665553, 3.0e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,       1.0810127268623624,       99.652500000000003,       12.346579657945759, 8.7e-15,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.22725088019936909,                   122.53,       7.3696919881362746, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.11724024863884289,       121.63760000000001,       8.8319240395284204, 8.7e-15,                      NAN,                      NAN,                      NAN },
   {    204, 0,      0.10754732779594979,                  120.504,       10.364187570668527, 7.3e-15,                      NAN,                      NAN,                      NAN },
   {    251, 0,       0.9149180196893143,                 102.7762,       7.7217153249779935, 1.4e-14,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n50_k25_15[] =
{
   {     49, 0,      0.44930580385550412,       89.326800000000006,       3.9472017632748391, 2.0e-14,                      NAN,                      NAN,                      NAN },
   {     50, 0,      0.35928261282091839,                  89.2774,       3.9351324297919121, 2.0e-14,                      NAN,                      NAN,                      NAN },
   {     51, 0,      0.35279568260636734,       89.157399999999996,       3.8551061256468673, 2.0e-14,                      NAN,                      NAN,                      NAN },
   {     75, 0,       1.1029049576103744,       88.008200000000002,       3.1741094436077657, 3.4e-14,                      NAN,                      NAN,                      NAN },
   {     77, 0,       1.3373155040068525,       88.630700000000004,       4.2369939827665553, 2.8e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.70601272686236249,       99.652500000000003,       12.346579657945759, 8.6e-15,                      NAN,                      NAN,                      NAN },
   {    200, 0,     -0.14774911980063091,                   122.53,       7.3696919881362746, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.49224024863884291,       121.63760000000001,       8.8319240395284204, 1.2e-14,                      NAN,                      NAN,                      NAN },
   {    217, 0,     0.074928867332972326,       112.78020000000001,       14.813320963241159, 5.3e-15,                      NAN,                      NAN,                      NAN },
   {    251, 0,       0.5399180196893143,                 102.7762,       7.7217153249779935, 1.3e-14,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n200[] =
{
   {    199, 0,      0.47232249407945109,               108.900525,       17.166693103197687, 5.6e-15,      0.47232249407945159,      0.47232249407945159,      0.47232249407945109 },
   {    200, 0,      0.47286334530794222,       108.97862499999999,       17.122827233969716, 5.6e-15,      0.47286334530794305,      0.47286334530794305,      0.47286334530794216 },
   {    201, 0,      0.47017151048956607,       109.03955000000001,       17.093976542849823, 5.6e-15,      0.47017151048956668,      0.47017151048956668,      0.47017151048956601 },
   {    209, 0,      0.31893605783666623,       109.09610000000001,       17.046602228596761, 5.1e-15,      0.31893605783666679,       0.3189360578366669,      0.31893605783666668 },
   {    213, 0,      0.22508968981873032,                 109.0423,       17.089482736759471, 4.7e-15,      0.22508968981873034,      0.22508968981873101,      0.22508968981873076 },
   {    234, 0,      0.59542512534847891,               110.588525,       15.801590456323535, 6.6e-15,      0.59542512534847958,      0.59542512534848002,      0.59542512534847902 },
   {    235, 0,      0.61981508036442201,       110.74554999999999,       15.720996841724128, 6.8e-15,      0.61981508036442268,      0.61981508036442312,      0.61981508036442212 },
   {    244, 0,       0.4718504190575557,               111.787925,       14.813053553179877, 6.6e-15,      0.47185041905755704,      0.47185041905755676,       0.4718504190575557 },
   {    247, 0,      0.46032882284283194,       112.06382499999999,       14.581272638023576, 6.7e-15,      0.46032882284283327,      0.46032882284283305,      0.46032882284283205 },
   {    251, 0,      0.41969515532029861,                112.44425,       14.240267876254997, 6.7e-15,       0.4196951553202995,       0.4196951553202995,      0.41969515532029855 },
};

static const PctbRow pctb_sma_n200_k13[] =
{
   {    199, 0,      0.72232249407945115,               108.900525,       17.166693103197687, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.72286334530794227,       108.97862499999999,       17.122827233969716, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    201, 0,      0.72017151048956607,       109.03955000000001,       17.093976542849823, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    213, 0,      0.47508968981873029,                 109.0423,       17.089482736759471, 4.7e-15,                      NAN,                      NAN,                      NAN },
   {    226, 0,      0.67007991078796225,       109.74890000000001,       16.419213403509925, 5.7e-15,                      NAN,                      NAN,                      NAN },
   {    235, 0,      0.86981508036442201,       110.74554999999999,       15.720996841724128, 6.7e-15,                      NAN,                      NAN,                      NAN },
   {    250, 0,      0.68723005154269223,               112.348975,       14.333988988567523, 6.9e-15,                      NAN,                      NAN,                      NAN },
   {    251, 0,      0.66969515532029855,                112.44425,       14.240267876254997, 6.8e-15,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n200_k25_15[] =
{
   {    199, 0,      0.34732249407945109,               108.900525,       17.166693103197687, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.34786334530794222,       108.97862499999999,       17.122827233969716, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    201, 0,      0.34517151048956607,       109.03955000000001,       17.093976542849823, 5.5e-15,                      NAN,                      NAN,                      NAN },
   {    202, 0,      0.11166618479565678,       109.02267500000001,       17.110103184065693, 4.6e-15,                      NAN,                      NAN,                      NAN },
   {    213, 0,      0.10008968981873032,                 109.0423,       17.089482736759471, 4.6e-15,                      NAN,                      NAN,                      NAN },
   {    235, 0,      0.49481508036442196,       110.74554999999999,       15.720996841724128, 6.7e-15,                      NAN,                      NAN,                      NAN },
   {    251, 0,      0.29469515532029861,                112.44425,       14.240267876254997, 6.6e-15,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n2_k1[] =
{
   {      1, 0,                      1.0,       93.157499999999999,       1.6574999999999989, 1.3e-13,                      1.0,                      1.0,                      1.0 },
   {      2, 0,                      0.0,       94.594999999999999,      0.21999999999999886, 5.8e-13,                      0.0,                      0.0,                      0.0 },
   {      3, 0,                      1.0,       94.734999999999999,      0.35999999999999943, 5.9e-13,                      1.0,       1.0000000000000198,                      1.0 },
   {    100, 0,                      0.0,                 117.0625,                   1.0625, 1.5e-13,                      0.0,  -2.0062383127344007e-14,                      0.0 },
   {    101, 1,                      0.5,                    116.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {    102, 0,                      0.0,                    114.0,                      2.0, 7.6e-14,                      0.0,  -1.4210854715202004e-14,                      0.0 },
   {    131, 0,                      1.0,                  137.845,     0.034999999999996589, 8.8e-12,                      1.0,      0.99999999999918798,                      1.0 },
   {    200, 0,                      1.0,                   107.06,     0.060000000000002274, 4.0e-12,                      1.0,      0.99999999999952627,                      1.0 },
   {    213, 0,                      0.0,       90.905000000000001,      0.65500000000000114, 1.9e-13,                      0.0,  -4.3391922794509862e-14,                      0.0 },
   {    243, 0,                      0.0,       109.59999999999999,      0.39999999999999858, 3.7e-13,   1.7763568394002252e-14,  -5.3290705182006757e-14,                      0.0 },
   {    251, 0,                      0.0,                   108.31,      0.43999999999999773, 3.3e-13,                      0.0,  -6.4594794160009442e-14,                      0.0 },
};

static const PctbRow pctb_sma_n5_k1[] =
{
   {      4, 0,      0.44827503628204979,       93.912999999999997,        1.285646141051261, 1.3e-13,      0.44827503628205634,      0.44827503628205634,      0.44827503628204912 },
   {      5, 0,      0.59747974115539326,       94.537999999999997,      0.44624656861425743, 4.0e-13,      0.59747974115541247,      0.59747974115541247,       0.5974797411553926 },
   {      6, 0,     -0.37731970164016349,       94.081000000000003,      0.88394230580960353, 1.8e-13,     -0.37731970164016315,     -0.37731970164016315,     -0.37731970164016354 },
   {     10, 0,       1.3808077855577139,       92.837999999999994,       1.8659008548151754, 1.3e-13,       1.3808077855577094,       1.3808077855577094,       1.3808077855577163 },
   {     38, 0,     -0.16963053624029995,       86.245000000000005,       1.5344282322741589, 8.3e-14,     -0.16963053624030017,     -0.16963053624029556,     -0.16963053624029911 },
   {     75, 0,        1.486159330722187,       87.325999999999993,        5.031641282921508, 4.7e-14,       1.4861593307221865,       1.4861593307221881,       1.4861593307221865 },
   {    100, 0,      0.79836598040076812,                  114.357,       2.7533299838559127, 8.6e-14,      0.79836598040076878,      0.79836598040076467,      0.79836598040076823 },
   {    121, 0,    -0.052515272184225255,                   122.91,      0.31673332631726736, 5.4e-13,    -0.052515272184225151,    -0.052515272184292451,    -0.052515272184227593 },
   {    133, 0,     -0.40308578046767829,       137.32599999999999,      0.56251577755650406, 4.2e-13,     -0.40308578046763394,     -0.40308578046763394,     -0.40308578046767807 },
   {    194, 0,      0.05532274707538342,                  117.062,       3.1618437659062151, 5.1e-14,     0.055322747075385724,     0.055322747075369993,      0.05532274707538417 },
   {    200, 0,      0.66609346283927817,                   106.81,      0.93320951559657861, 2.3e-13,      0.66609346283928683,      0.66609346283924875,      0.66609346283927706 },
   {    202, 0,     -0.49875631930746028,                  103.998,        6.507092745612284, 2.8e-14,     -0.49875631930746067,     -0.49875631930746395,     -0.49875631930746078 },
   {    251, 0,     -0.31612008943307146,                  109.036,      0.71435565371878917, 2.5e-13,     -0.31612008943308795,     -0.31612008943309788,     -0.31612008943307268 },
};

static const PctbRow pctb_sma_n5_k25[] =
{
   {      4, 0,       0.4793100145128199,       93.912999999999997,        1.285646141051261, 5.2e-14,      0.47931001451282257,      0.47931001451282257,      0.47931001451281963 },
   {      5, 0,      0.53899189646215728,       94.537999999999997,      0.44624656861425743, 1.6e-13,      0.53899189646216472,      0.53899189646216472,      0.53899189646215706 },
   {      6, 0,      0.14907211934393461,       94.081000000000003,      0.88394230580960353, 6.3e-14,      0.14907211934393699,      0.14907211934393699,      0.14907211934393458 },
   {     10, 0,       0.8523231142230856,       92.837999999999994,       1.8659008548151754, 4.2e-14,      0.85232311422308482,      0.85232311422308482,       0.8523231142230866 },
   {     75, 0,      0.89446373228887477,       87.325999999999993,        5.031641282921508, 1.6e-14,      0.89446373228887488,      0.89446373228887543,      0.89446373228887455 },
   {    100, 0,      0.61934639216030718,                  114.357,       2.7533299838559127, 3.2e-14,      0.61934639216030729,      0.61934639216030629,      0.61934639216030729 },
   {    121, 0,       0.2789938911263099,                   122.91,      0.31673332631726736, 2.5e-13,       0.2789938911263139,      0.27899389112628697,      0.27899389112630896 },
   {    133, 0,       0.1387656878129287,       137.32599999999999,      0.56251577755650406, 1.5e-13,      0.13876568781294277,      0.13876568781294277,      0.13876568781292878 },
   {    156, 0,      0.77292535157884268,       124.73400000000001,       2.7597289721999867, 3.7e-14,      0.77292535157884146,      0.77292535157883735,      0.77292535157884257 },
   {    192, 0,      0.22103201385083102,                  119.036,       1.9113304266923594, 3.8e-14,      0.22103201385082949,      0.22103201385082205,      0.22103201385083077 },
   {    200, 0,      0.56643738513571129,                   106.81,      0.93320951559657861, 8.5e-14,      0.56643738513571451,       0.5664373851356993,      0.56643738513571085 },
   {    202, 0,      0.10049747227701587,                  103.998,        6.507092745612284, 8.8e-15,      0.10049747227701573,      0.10049747227701443,      0.10049747227701569 },
   {    251, 0,      0.17355196422677141,                  109.036,      0.71435565371878917, 9.1e-14,      0.17355196422676611,      0.17355196422676475,      0.17355196422677091 },
};

static const PctbRow pctb_sma_n20_k1[] =
{
   {     19, 0,     -0.08197032621122971,       92.891000000000005,       2.5911974066056795, 5.0e-14,    -0.081970326211229586,    -0.081970326211229586,    -0.081970326211229697 },
   {     20, 0,     -0.29005569234325718,       92.734750000000005,       2.7591409328811025, 5.3e-14,     -0.29005569234325762,     -0.29005569234325762,      -0.2900556923432579 },
   {     21, 0,     -0.31119834881371355,       92.375249999999994,       2.9279213936682114, 5.1e-14,     -0.31119834881371422,     -0.31119834881371178,     -0.31119834881371394 },
   {     43, 0,       1.2693149103014223,       86.047749999999994,       2.2047213401017367, 9.7e-14,       1.2693149103014154,       1.2693149103014187,       1.2693149103014216 },
   {     90, 0,       1.5346075554266758,       102.19750000000001,       10.053328864112624, 2.9e-14,       1.5346075554266749,       1.5346075554266743,        1.534607555426678 },
   {    100, 0,      0.78952934566625521,       112.76949999999999,       5.5788818100045816, 4.2e-14,      0.78952934566625288,       0.7895293456662561,      0.78952934566625532 },
   {    167, 0,        1.376134487299373,                 123.5585,       2.1066971187145058, 1.6e-13,       1.3761344872993704,        1.376134487299377,       1.3761344872993713 },
   {    200, 0,      -0.1938432778455326,                 116.3365,       6.6416295251993684, 2.6e-14,     -0.19384327784553101,     -0.19384327784553101,     -0.19384327784553271 },
   {    202, 0,     -0.87708805640848997,                  113.877,       8.3062952632325793, 2.8e-14,     -0.87708805640848808,     -0.87708805640848808,     -0.87708805640848997 },
   {    223, 0,         2.08913460680707,       95.155000000000001,       2.7640830305907955, 1.2e-13,       2.0891346068070691,       2.0891346068070744,       2.0891346068070682 },
   {    251, 0,     0.052238190250905203,       110.57000000000001,       3.0149958540601682, 5.1e-14,     0.052238190250909915,     0.052238190250912274,     0.052238190250905009 },
};

static const PctbRow pctb_sma_n20_k25[] =
{
   {     19, 0,      0.26721186951550813,       92.891000000000005,       2.5911974066056795, 2.3e-14,       0.2672118695155089,       0.2672118695155089,      0.26721186951550813 },
   {     20, 0,      0.18397772306269713,       92.734750000000005,       2.7591409328811025, 2.0e-14,      0.18397772306269791,      0.18397772306269791,      0.18397772306269683 },
   {     21, 0,      0.17552066047451459,       92.375249999999994,       2.9279213936682114, 1.9e-14,      0.17552066047451367,      0.17552066047451464,      0.17552066047451442 },
   {     90, 0,      0.91384302217067037,       102.19750000000001,       10.053328864112624, 9.1e-15,      0.91384302217067004,      0.91384302217067004,      0.91384302217067126 },
   {    100, 0,      0.61581173826650215,       112.76949999999999,       5.5788818100045816, 1.6e-14,      0.61581173826650126,      0.61581173826650226,      0.61581173826650215 },
   {    139, 0,      0.25239828555618571,       131.20249999999999,        5.163534521042731, 1.6e-14,      0.25239828555618632,      0.25239828555618632,       0.2523982855561856 },
   {    167, 0,      0.85045379491974915,                 123.5585,       2.1066971187145058, 5.0e-14,      0.85045379491974749,      0.85045379491975015,      0.85045379491974848 },
   {    200, 0,      0.22246268886178697,                 116.3365,       6.6416295251993684, 1.1e-14,      0.22246268886178783,      0.22246268886178783,      0.22246268886178691 },
   {    202, 0,    -0.050835222563396003,                  113.877,       8.3062952632325793, 7.1e-15,     -0.05083522256339558,     -0.05083522256339558,    -0.050835222563395982 },
   {    220, 0,      0.47922826468777108,       94.967500000000001,       3.3458928180681458, 2.0e-14,      0.47922826468777274,      0.47922826468777274,      0.47922826468777086 },
   {    223, 0,        1.135653842722828,       95.155000000000001,       2.7640830305907955, 3.3e-14,       1.1356538427228278,       1.1356538427228298,       1.1356538427228273 },
   {    228, 0,      0.74581556704276808,       97.757999999999996,       5.2331917602931384, 1.6e-14,      0.74581556704276841,      0.74581556704276952,      0.74581556704276808 },
   {    251, 0,      0.32089527610036206,       110.57000000000001,       3.0149958540601682, 2.4e-14,      0.32089527610036361,      0.32089527610036456,        0.320895276100362 },
};

static const PctbRow pctb_sma_n50_k1[] =
{
   {     49, 0,      0.64861160771100823,       89.326800000000006,       3.9472017632748391, 4.4e-14,      0.64861160771100945,      0.64861160771100945,      0.64861160771100812 },
   {     50, 0,      0.46856522564183684,                  89.2774,       3.9351324297919121, 4.0e-14,       0.4685652256418385,       0.4685652256418385,      0.46856522564183661 },
   {     51, 0,      0.45559136521273474,       89.157399999999996,       3.8551061256468673, 4.1e-14,      0.45559136521273713,      0.45559136521273713,      0.45559136521273502 },
   {     63, 0,       1.1952730677842445,       87.521199999999993,       2.8613218204179689, 7.4e-14,       1.1952730677842354,       1.1952730677842527,        1.195273067784244 },
   {     74, 0,       0.2105538986229665,       87.690699999999993,       3.0242245303548478, 4.4e-14,      0.21055389862296245,      0.21055389862297655,      0.21055389862296697 },
   {     75, 0,       1.9558099152207491,       88.008200000000002,       3.1741094436077657, 8.7e-14,       1.9558099152207482,       1.9558099152207593,       1.9558099152207469 },
   {     77, 0,        2.424631008013705,       88.630700000000004,       4.2369939827665553, 7.5e-14,        2.424631008013709,       2.4246310080137143,       2.4246310080137032 },
   {     96, 0,       1.2417565614504549,       97.711200000000005,       11.782032615809548, 2.1e-14,       1.2417565614504562,       1.2417565614504573,        1.241756561450458 },
   {    100, 0,        1.162025453724725,       99.652500000000003,       12.346579657945759, 2.0e-14,       1.1620254537247243,       1.1620254537247265,       1.1620254537247228 },
   {    200, 0,     -0.54549823960126187,                   122.53,       7.3696919881362746, 3.0e-14,     -0.54549823960126076,     -0.54549823960126376,     -0.54549823960126131 },
   {    202, 0,      -1.2344804972776857,       121.63760000000001,       8.8319240395284204, 3.2e-14,      -1.2344804972776855,      -1.2344804972776855,      -1.2344804972776857 },
   {    205, 0,     -0.61885861213288951,       119.86660000000001,       10.889043412531699, 2.0e-14,     -0.61885861213288818,     -0.61885861213288951,      -0.6188586121328894 },
   {    251, 0,       0.8298360393786286,                 102.7762,       7.7217153249779935, 2.8e-14,      0.82983603937862949,      0.82983603937862671,       0.8298360393786286 },
};

static const PctbRow pctb_sma_n50_k25[] =
{
   {     49, 0,      0.55944464308440323,       89.326800000000006,       3.9472017632748391, 1.7e-14,      0.55944464308440378,      0.55944464308440378,      0.55944464308440323 },
   {     50, 0,      0.48742609025673472,                  89.2774,       3.9351324297919121, 1.7e-14,      0.48742609025673544,      0.48742609025673544,      0.48742609025673461 },
   {     51, 0,      0.48223654608509392,       89.157399999999996,       3.8551061256468673, 1.7e-14,      0.48223654608509486,      0.48223654608509486,      0.48223654608509398 },
   {     63, 0,      0.77810922711369779,       87.521199999999993,       2.8613218204179689, 2.6e-14,      0.77810922711369412,      0.77810922711370112,      0.77810922711369768 },
   {     74, 0,      0.38422155944918662,       87.690699999999993,       3.0242245303548478, 2.0e-14,      0.38422155944918518,      0.38422155944919084,      0.38422155944918679 },
   {     75, 0,       1.0823239660882995,       88.008200000000002,       3.1741094436077657, 2.6e-14,       1.0823239660882982,       1.0823239660883026,       1.0823239660882988 },
   {     77, 0,       1.2698524032054821,       88.630700000000004,       4.2369939827665553, 2.2e-14,       1.2698524032054821,       1.2698524032054841,       1.2698524032054814 },
   {     96, 0,      0.79670262458018193,       97.711200000000005,       11.782032615809548, 7.1e-15,      0.79670262458018226,      0.79670262458018293,      0.79670262458018326 },
   {    100, 0,         0.76481018148989,       99.652500000000003,       12.346579657945759, 6.8e-15,         0.76481018148989,      0.76481018148989099,      0.76481018148988911 },
   {    200, 0,     0.081800704159495266,                   122.53,       7.3696919881362746, 9.0e-15,     0.081800704159495363,     0.081800704159495002,     0.081800704159495488 },
   {    202, 0,     -0.19379219891107433,       121.63760000000001,       8.8319240395284204, 7.7e-15,     -0.19379219891107458,     -0.19379219891107458,     -0.19379219891107424 },
   {    217, 0,      0.25994309386637787,       112.78020000000001,       14.813320963241159, 4.6e-15,      0.25994309386637832,      0.25994309386637759,      0.25994309386637854 },
   {    251, 0,      0.63193441575145148,                 102.7762,       7.7217153249779935, 1.1e-14,      0.63193441575145182,      0.63193441575145071,      0.63193441575145137 },
};

static const PctbRow pctb_sma_n5_k0[] =
{
   {      4, 1,                      0.5,       93.912999999999997,        1.285646141051261,     0.0,                      0.5,                      NAN,                      NAN },
   {      5, 1,                      0.5,       94.537999999999997,      0.44624656861425743,     0.0,                      0.5,                      NAN,                      NAN },
   {      6, 1,                      0.5,       94.081000000000003,      0.88394230580960353,     0.0,                      0.5,                      NAN,                      NAN },
   {    100, 1,                      0.5,                  114.357,       2.7533299838559127,     0.0,                      0.5,                      NAN,                      NAN },
   {    200, 1,                      0.5,                   106.81,      0.93320951559657861,     0.0,                      0.5,                      NAN,                      NAN },
   {    251, 1,                      0.5,                  109.036,      0.71435565371878917,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n50_k0[] =
{
   {     49, 1,                      0.5,       89.326800000000006,       3.9472017632748391,     0.0,                      0.5,                      NAN,                      NAN },
   {     50, 1,                      0.5,                  89.2774,       3.9351324297919121,     0.0,                      0.5,                      NAN,                      NAN },
   {     51, 1,                      0.5,       89.157399999999996,       3.8551061256468673,     0.0,                      0.5,                      NAN,                      NAN },
   {    100, 1,                      0.5,       99.652500000000003,       12.346579657945759,     0.0,                      0.5,                      NAN,                      NAN },
   {    200, 1,                      0.5,                   122.53,       7.3696919881362746,     0.0,                      0.5,                      NAN,                      NAN },
   {    251, 1,                      0.5,                 102.7762,       7.7217153249779935,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_k01_02[] =
{
   {     19, 0,      -3.2131355080748647,       92.891000000000005,       2.5911974066056795, 1.0e-12,                      NAN,                      NAN,                      NAN },
   {     20, 0,      -4.6003712822883811,       92.734750000000005,       2.7591409328811025, 1.3e-12,                      NAN,                      NAN,                      NAN },
   {     21, 0,      -4.7413223254247567,       92.375249999999994,       2.9279213936682114, 1.2e-12,                      NAN,                      NAN,                      NAN },
   {    100, 0,       2.5968623044417014,       112.76949999999999,       5.5788818100045816, 5.0e-13,                      NAN,                      NAN,                      NAN },
   {    167, 0,       6.5075632486624855,                 123.5585,       2.1066971187145058, 2.8e-12,                      NAN,                      NAN,                      NAN },
   {    200, 0,      -3.9589551856368836,                 116.3365,       6.6416295251993684, 5.7e-13,                      NAN,                      NAN,                      NAN },
   {    202, 0,      -8.5139203760565998,                  113.877,       8.3062952632325793, 8.1e-13,                      NAN,                      NAN,                      NAN },
   {    223, 0,         11.2608973787138,       95.155000000000001,       2.7640830305907955, 2.7e-12,                      NAN,                      NAN,                      NAN },
   {    251, 0,      -2.3184120649939652,       110.57000000000001,       3.0149958540601682, 8.3e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_km1_2[] =
{
   {     19, 0,      0.83605934757754052,       92.891000000000005,       2.5911974066056795, 1.5e-13,                      NAN,                      NAN,                      NAN },
   {     20, 0,      0.41988861531348565,       92.734750000000005,       2.7591409328811025, 1.1e-13,                      NAN,                      NAN,                      NAN },
   {     21, 0,      0.37760330237257289,       92.375249999999994,       2.9279213936682114, 1.1e-13,                      NAN,                      NAN,                      NAN },
   {     77, 0,       4.7529797172636581,       91.022499999999994,       5.0554313614171438, 1.9e-13,                      NAN,                      NAN,                      NAN },
   {    100, 0,       2.5790586913325106,       112.76949999999999,       5.5788818100045816, 1.4e-13,                      NAN,                      NAN,                      NAN },
   {    167, 0,       3.7522689745987461,                 123.5585,       2.1066971187145058, 5.4e-13,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.61231344430893486,                 116.3365,       6.6416295251993684, 6.1e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.75417611281698005,                  113.877,       8.3062952632325793, 4.8e-14,                      NAN,                      NAN,                      NAN },
   {    223, 0,       5.1782692136141399,       95.155000000000001,       2.7640830305907955, 4.0e-13,                      NAN,                      NAN,                      NAN },
   {    251, 0,       1.1044763805018105,       110.57000000000001,       3.0149958540601682, 1.7e-13,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_km2_m2[] =
{
   {     19, 0,      0.79098516310561484,       92.891000000000005,       2.5911974066056795, 3.7e-14,      0.79098516310561395,      0.79098516310561395,      0.79098516310561484 },
   {     20, 0,      0.89502784617162856,       92.734750000000005,       2.7591409328811025, 3.6e-14,      0.89502784617162778,      0.89502784617162778,        0.895027846171629 },
   {     21, 0,      0.90559917440685678,       92.375249999999994,       2.9279213936682114, 3.4e-14,      0.90559917440685711,      0.90559917440685589,        0.905599174406857 },
   {     45, 0,     0.073543940638678612,       86.896249999999995,       2.2591717923832175, 2.8e-14,     0.073543940638681887,     0.073543940638680319,     0.073543940638679195 },
   {     90, 0,    -0.017303777713337899,       102.19750000000001,       10.053328864112624, 7.4e-15,    -0.017303777713337815,    -0.017303777713337461,    -0.017303777713339071 },
   {    100, 0,      0.35523532716687239,       112.76949999999999,       5.5788818100045816, 1.7e-14,      0.35523532716687339,      0.35523532716687212,      0.35523532716687234 },
   {    123, 0,    -0.054765563563968653,                 119.9395,       4.1956912124225729, 2.1e-14,    -0.054765563563969756,    -0.054765563563969756,    -0.054765563563969138 },
   {    165, 0,      0.46458501346322056,       122.86150000000001,       2.3189335372105857, 4.7e-14,      0.46458501346322112,      0.46458501346321807,      0.46458501346322029 },
   {    200, 0,      0.84692163892276628,                 116.3365,       6.6416295251993684, 1.8e-14,      0.84692163892276506,      0.84692163892276506,       0.8469216389227664 },
   {    202, 0,        1.188544028204245,                  113.877,       8.3062952632325793, 1.6e-14,       1.1885440282042445,       1.1885440282042445,        1.188544028204245 },
   {    223, 0,     -0.29456730340353504,       95.155000000000001,       2.7640830305907955, 2.9e-14,      -0.2945673034035346,     -0.29456730340353721,     -0.29456730340353404 },
   {    234, 0,   -0.0062361752288423132,                  102.486,       6.9799436960479841, 1.1e-14,    -0.006236175228841863,   -0.0062361752288438987,   -0.0062361752288425318 },
   {    251, 0,      0.72388090487454737,       110.57000000000001,       3.0149958540601682, 3.7e-14,      0.72388090487454559,      0.72388090487454437,      0.72388090487454748 },
};

static const PctbRow pctb_sma_n20_km2_2[] =
{
   {     19, 1,                      0.5,       92.891000000000005,       2.5911974066056795,     0.0,                      NAN,                      NAN,                      NAN },
   {     20, 1,                      0.5,       92.734750000000005,       2.7591409328811025,     0.0,                      NAN,                      NAN,                      NAN },
   {     21, 1,                      0.5,       92.375249999999994,       2.9279213936682114,     0.0,                      NAN,                      NAN,                      NAN },
   {    100, 1,                      0.5,       112.76949999999999,       5.5788818100045816,     0.0,                      NAN,                      NAN,                      NAN },
   {    200, 1,                      0.5,                 116.3365,       6.6416295251993684,     0.0,                      NAN,                      NAN,                      NAN },
   {    251, 1,                      0.5,       110.57000000000001,       3.0149958540601682,     0.0,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_k0[] =
{
   {     19, 1,                      0.5,       92.891000000000005,       2.5911974066056795,     0.0,                      0.5,                      NAN,                      NAN },
   {     20, 1,                      0.5,       92.734750000000005,       2.7591409328811025,     0.0,                      0.5,                      NAN,                      NAN },
   {     21, 1,                      0.5,       92.375249999999994,       2.9279213936682114,     0.0,                      0.5,                      NAN,                      NAN },
   {    100, 1,                      0.5,       112.76949999999999,       5.5788818100045816,     0.0,                      0.5,                      NAN,                      NAN },
   {    200, 1,                      0.5,                 116.3365,       6.6416295251993684,     0.0,                      0.5,                      NAN,                      NAN },
   {    251, 1,                      0.5,       110.57000000000001,       3.0149958540601682,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_sma_n20_kmax[] =
{
   {     19, 0,                      0.5,       92.891000000000005,       2.5911974066056795, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     20, 0,                      0.5,       92.734750000000005,       2.7591409328811025, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     21, 0,                      0.5,       92.375249999999994,       2.9279213936682114, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     24, 0,                      0.5,       90.776749999999993,       4.0702227442119181, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    100, 0,                      0.5,       112.76949999999999,       5.5788818100045816, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    200, 0,                      0.5,                 116.3365,       6.6416295251993684, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    251, 0,                      0.5,       110.57000000000001,       3.0149958540601682, 1.4e-15,                      0.5,                      0.5,                      NAN },
};

static const PctbRow pctb_sma_n20_kmin[] =
{
   {     19, 0,                      0.5,       92.891000000000005,       2.5911974066056795, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     20, 0,                      0.5,       92.734750000000005,       2.7591409328811025, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     21, 0,                      0.5,       92.375249999999994,       2.9279213936682114, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {     24, 0,                      0.5,       90.776749999999993,       4.0702227442119181, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    100, 0,                      0.5,       112.76949999999999,       5.5788818100045816, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    200, 0,                      0.5,                 116.3365,       6.6416295251993684, 1.4e-15,                      0.5,                      0.5,                      NAN },
   {    251, 0,                      0.5,       110.57000000000001,       3.0149958540601682, 1.4e-15,                      0.5,                      0.5,                      NAN },
};

static const PctbRow pctb_sma_n20_s100[] =
{
   {    100, 0,      0.64476467283312766,       112.76949999999999,       5.5788818100045816, 2.0e-14,      0.64476467283312666,      0.64476467283312788,      0.64476467283312766 },
   {    101, 0,      0.62573121071164517,                113.33975,       5.2895577497083819, 2.1e-14,      0.62573121071164473,      0.62573121071164473,      0.62573121071164517 },
   {    102, 0,      0.41893913777720804,                 113.6335,       5.0378812758936666, 2.0e-14,      0.41893913777720815,      0.41893913777720815,      0.41893913777720809 },
   {    139, 0,      0.19049785694523211,       131.20249999999999,        5.163534521042731, 1.9e-14,       0.1904978569452325,       0.1904978569452325,        0.190497856945232 },
   {    167, 0,      0.93806724364968652,                 123.5585,       2.1066971187145058, 6.4e-14,      0.93806724364968519,      0.93806724364968852,      0.93806724364968563 },
   {    170, 0,       1.1370463136960398,       124.45100000000001,       2.9625004641349855, 5.0e-14,       1.1370463136960396,       1.1370463136960423,       1.1370463136960411 },
   {    200, 0,      0.15307836107723372,                 116.3365,       6.6416295251993684, 1.3e-14,      0.15307836107723494,      0.15307836107723494,      0.15307836107723363 },
   {    202, 0,     -0.18854402820424501,                  113.877,       8.3062952632325793, 9.7e-15,     -0.18854402820424454,     -0.18854402820424454,     -0.18854402820424498 },
   {    220, 0,      0.47403533085971383,       94.967500000000001,       3.3458928180681458, 2.5e-14,      0.47403533085971594,      0.47403533085971594,      0.47403533085971361 },
   {    223, 0,        1.294567303403535,       95.155000000000001,       2.7640830305907955, 4.4e-14,       1.2945673034035345,       1.2945673034035372,       1.2945673034035341 },
   {    234, 0,       1.0062361752288422,                  102.486,       6.9799436960479841, 1.7e-14,       1.0062361752288418,        1.006236175228844,       1.0062361752288425 },
   {    251, 0,      0.27611909512545263,       110.57000000000001,       3.0149958540601682, 2.9e-14,      0.27611909512545441,      0.27611909512545563,      0.27611909512545252 },
};

static const PctbRow pctb_sma_n20_s251[] =
{
   {    251, 0,      0.27611909512545263,       110.57000000000001,       3.0149958540601682, 2.9e-14,      0.27611909512545441,      0.27611909512545563,      0.27611909512545252 },
};

static const PctbRow pctb_ema_n5[] =
{
   {      4, 0,      0.47413751814102489,       93.912999999999997,        1.285646141051261, 6.5e-14,                      NAN,                      NAN,      0.47413751814102456 },
   {      5, 0,      0.76592174598712504,       94.150333333333336,      0.44624656861425743, 2.2e-13,                      NAN,                      NAN,      0.76592174598712459 },
   {      6, 0,      0.19448735083653318,        93.61022222222222,      0.88394230580960353, 8.0e-14,                      NAN,                      NAN,      0.19448735083653307 },
   {     62, 0,       1.0026808582654183,       89.391615856079582,       1.2823166535610457, 7.8e-14,                      NAN,                      NAN,       1.0026808582654168 },
   {    100, 0,      0.55575550063443324,       115.38594683335327,       2.7533299838559127, 3.9e-14,                      NAN,                      NAN,      0.55575550063443324 },
   {    121, 0,      0.49337324907425073,       122.56839565145356,      0.31673332631726736, 3.5e-13,                      NAN,                      NAN,      0.49337324907425062 },
   {    135, 0,      0.24234341489033184,       135.76424350322014,       1.1005380502281603, 9.6e-14,                      NAN,                      NAN,      0.24234341489033248 },
   {    200, 0,      0.26916976720385716,       107.98165187893095,      0.93320951559657861, 9.1e-14,                      NAN,                      NAN,      0.26916976720385644 },
   {    201, 0,     0.017661846918881063,       107.65443458595396,      0.33919905660246324, 2.2e-13,                      NAN,                      NAN,     0.017661846918880435 },
   {    251, 0,      0.21276739129251382,       108.69074495185036,      0.71435565371878917, 1.2e-13,                      NAN,                      NAN,      0.21276739129251329 },
};

static const PctbRow pctb_ema_n20[] =
{
   {     19, 0,      0.20901483689438513,       92.891000000000005,       2.5911974066056795, 2.7e-14,                      NAN,                      NAN,      0.20901483689438516 },
   {     20, 0,      0.12978469917824667,       92.460904761904757,       2.7591409328811025, 2.4e-14,                      NAN,                      NAN,      0.12978469917824634 },
   {     21, 0,      0.12641223795402834,       92.000342403628125,       2.9279213936682114, 2.3e-14,                      NAN,                      NAN,      0.12641223795402817 },
   {     77, 0,       1.1675160093887711,       91.441674527551939,       5.0554313614171438, 2.3e-14,                      NAN,                      NAN,       1.1675160093887718 },
   {     90, 0,       0.9568676950697218,       104.62783525629985,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.95686769506972291 },
   {    100, 0,      0.68923126139439517,       111.77720463169035,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,      0.68923126139439517 },
   {    143, 0,      0.35300745716708387,       128.24828755174929,       4.8952951902821944, 2.2e-14,                      NAN,                      NAN,      0.35300745716708382 },
   {    167, 0,      0.86613088503421509,       124.16469247770411,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86613088503421443 },
   {    200, 0,        0.204078559898006,       114.98160229488369,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407855989800594 },
   {    202, 0,     -0.13234818868996329,       112.00988305771658,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234818868996326 },
   {    251, 0,      0.44556700814803607,       108.52646097903106,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556700814803585 },
};

static const PctbRow pctb_ema_n20_s19_u0[] =
{
   {     19, 0,      0.20901483689438513,       92.891000000000005,       2.5911974066056795, 2.7e-14,                      NAN,                      NAN,      0.20901483689438516 },
   {     20, 0,      0.12978469917824667,       92.460904761904757,       2.7591409328811025, 2.4e-14,                      NAN,                      NAN,      0.12978469917824634 },
   {     21, 0,      0.12641223795402834,       92.000342403628125,       2.9279213936682114, 2.3e-14,                      NAN,                      NAN,      0.12641223795402817 },
   {     77, 0,       1.1675160093887711,       91.441674527551939,       5.0554313614171438, 2.3e-14,                      NAN,                      NAN,       1.1675160093887718 },
   {     90, 0,       0.9568676950697218,       104.62783525629985,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.95686769506972291 },
   {    100, 0,      0.68923126139439517,       111.77720463169035,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,      0.68923126139439517 },
   {    143, 0,      0.35300745716708387,       128.24828755174929,       4.8952951902821944, 2.2e-14,                      NAN,                      NAN,      0.35300745716708382 },
   {    167, 0,      0.86613088503421509,       124.16469247770411,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86613088503421443 },
   {    200, 0,        0.204078559898006,       114.98160229488369,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407855989800594 },
   {    202, 0,     -0.13234818868996329,       112.00988305771658,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234818868996326 },
   {    251, 0,      0.44556700814803607,       108.52646097903106,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556700814803585 },
};

static const PctbRow pctb_ema_n20_s20_u0[] =
{
   {     20, 0,      0.10497215382837141,       92.734750000000005,       2.7591409328811025, 2.4e-14,                      NAN,                      NAN,      0.10497215382837104 },
   {     21, 0,      0.10525689377668553,       92.248107142857137,       2.9279213936682114, 2.3e-14,                      NAN,                      NAN,      0.10525689377668535 },
   {     22, 0,   -0.0086933661945494487,       91.536858843537416,       3.3206934140326774, 1.9e-14,                      NAN,                      NAN,   -0.0086933661945491607 },
   {     77, 0,        1.167470912930177,       91.442586455756199,       5.0554313614171438, 2.3e-14,                      NAN,                      NAN,       1.1674709129301777 },
   {     90, 0,      0.95686152149444625,       104.62808351622989,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.95686152149444725 },
   {    100, 0,      0.68922717215497542,       111.77729588522401,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,      0.68922717215497542 },
   {    138, 0,      0.21630178866810112,       129.91119950257226,       5.3236143737126547, 1.9e-14,                      NAN,                      NAN,      0.21630178866810121 },
   {    167, 0,      0.86613087177902037,       124.16469258940283,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,       0.8661308717790196 },
   {    200, 0,      0.20407855974335753,       114.98160229899216,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407855974335748 },
   {    202, 0,     -0.13234818879118684,       112.00988306107975,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234818879118682 },
   {    251, 0,      0.44556700814596789,         108.526460979056,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556700814596767 },
};

static const PctbRow pctb_ema_n20_s100_u0[] =
{
   {    100, 0,      0.64476467283312766,       112.76949999999999,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,      0.64476467283312766 },
   {    101, 0,      0.63814166853053411,       113.07716666666667,       5.2895577497083819, 2.1e-14,                      NAN,                      NAN,      0.63814166853053422 },
   {    102, 0,      0.45163743885039825,       112.97457936507936,       5.0378812758936666, 2.0e-14,                      NAN,                      NAN,      0.45163743885039825 },
   {    138, 0,      0.21526273476093657,       129.93332559183324,       5.3236143737126547, 1.9e-14,                      NAN,                      NAN,      0.21526273476093663 },
   {    167, 0,      0.86598674740041903,       124.16590709505537,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86598674740041826 },
   {    170, 0,       1.0527978700255298,       125.44934421390614,       2.9625004641349855, 4.9e-14,                      NAN,                      NAN,       1.0527978700255309 },
   {    200, 0,      0.20407687824283097,       114.98164697060633,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407687824283091 },
   {    202, 0,     -0.13234928939978655,       112.00991962899974,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234928939978652 },
   {    224, 0,       1.0849469069247302,       98.762497255970828,       3.8924484582329413, 3.0e-14,                      NAN,                      NAN,       1.0849469069247302 },
   {    251, 0,      0.44556698565881092,       108.52646125025073,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,       0.4455669856588107 },
};

static const PctbRow pctb_ema_n20_s251_u0[] =
{
   {    251, 0,      0.27611909512545263,       110.57000000000001,       3.0149958540601682, 2.9e-14,                      NAN,                      NAN,      0.27611909512545252 },
};

static const PctbRow pctb_ema_n20_s0_u15[] =
{
   {     34, 0,       0.5528378321703693,       87.866215336241723,       2.8567819635912013, 2.9e-14,                      NAN,                      NAN,      0.55283783217036941 },
   {     35, 0,      0.41195505513608194,       87.771813875647268,        2.546466117485171, 3.0e-14,                      NAN,                      NAN,      0.41195505513608188 },
   {     36, 0,      0.41315813938521345,       87.680688744633244,       2.4921412856216638, 3.0e-14,                      NAN,                      NAN,      0.41315813938521351 },
   {     77, 0,       1.1675160093887711,       91.441674527551939,       5.0554313614171438, 2.3e-14,                      NAN,                      NAN,       1.1675160093887718 },
   {     90, 0,       0.9568676950697218,       104.62783525629985,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.95686769506972291 },
   {    100, 0,      0.68923126139439517,       111.77720463169035,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,      0.68923126139439517 },
   {    143, 0,      0.35300745716708387,       128.24828755174929,       4.8952951902821944, 2.2e-14,                      NAN,                      NAN,      0.35300745716708382 },
   {    167, 0,      0.86613088503421509,       124.16469247770411,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86613088503421443 },
   {    200, 0,        0.204078559898006,       114.98160229488369,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407855989800594 },
   {    202, 0,     -0.13234818868996329,       112.00988305771658,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234818868996326 },
   {    251, 0,      0.44556700814803607,       108.52646097903106,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556700814803585 },
};

static const PctbRow pctb_ema_n20_s100_u15[] =
{
   {    100, 0,      0.71126383481646349,       111.28553613932247,       5.5788818100045816, 2.0e-14,                      NAN,                      NAN,       0.7112638348164636 },
   {    101, 0,      0.70159848442683692,       111.73453269748224,       5.2895577497083819, 2.1e-14,                      NAN,                      NAN,      0.70159848442683703 },
   {    102, 0,       0.5119189342267384,       111.75981529772203,       5.0378812758936666, 2.0e-14,                      NAN,                      NAN,      0.51191893422673829 },
   {    140, 0,      0.21948231007354688,       128.80445400936145,       5.1729839309628618, 1.9e-14,                      NAN,                      NAN,      0.21948231007354677 },
   {    167, 0,       0.8662023032181202,       124.16409065177508,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86620230321811942 },
   {    170, 0,        1.052911398902423,       125.44799889650419,       2.9625004641349855, 4.9e-14,                      NAN,                      NAN,       1.0529113989024244 },
   {    200, 0,      0.20407939313465207,       114.98158015868724,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407939313465201 },
   {    202, 0,     -0.13234764330362309,       112.00986493715668,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234764330362306 },
   {    224, 0,       1.0849472954277812,       98.762491207058417,       3.8924484582329413, 3.0e-14,                      NAN,                      NAN,       1.0849472954277812 },
   {    251, 0,      0.44556701929113296,       108.52646084464548,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556701929113274 },
};

static const PctbRow pctb_ema_n20_s0_u100[] =
{
   {    119, 0,      0.77931729753928947,       118.44331940414776,       3.6845915309569932, 3.3e-14,                      NAN,                      NAN,      0.77931729753928958 },
   {    120, 0,      0.77620791952813406,       118.88871755613368,       3.8297982649220574, 3.2e-14,                      NAN,                      NAN,      0.77620791952813362 },
   {    121, 0,      0.71254397087545696,       119.23836350316857,       3.9069992001534888, 3.1e-14,                      NAN,                      NAN,      0.71254397087545684 },
   {    143, 0,      0.35300745716708387,       128.24828755174929,       4.8952951902821944, 2.2e-14,                      NAN,                      NAN,      0.35300745716708382 },
   {    167, 0,      0.86613088503421509,       124.16469247770411,       2.1066971187145058, 6.3e-14,                      NAN,                      NAN,      0.86613088503421443 },
   {    170, 0,        1.052873784393592,       125.44844462850367,       2.9625004641349855, 4.9e-14,                      NAN,                      NAN,       1.0528737843935934 },
   {    200, 0,        0.204078559898006,       114.98160229488369,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,      0.20407855989800594 },
   {    202, 0,     -0.13234818868996329,       112.00988305771658,       8.3062952632325793, 9.3e-15,                      NAN,                      NAN,     -0.13234818868996326 },
   {    224, 0,       1.0849471667085351,       98.762493211190545,       3.8924484582329413, 3.0e-14,                      NAN,                      NAN,       1.0849471667085351 },
   {    251, 0,      0.44556700814803607,       108.52646097903106,       3.0149958540601682, 3.2e-14,                      NAN,                      NAN,      0.44556700814803585 },
};

static const PctbRow pctb_ema_n20_k25_15[] =
{
   {     19, 0,     0.084014836894385145,       92.891000000000005,       2.5911974066056795, 2.6e-14,                      NAN,                      NAN,                      NAN },
   {     20, 0,    0.0047846991782466812,       92.460904761904757,       2.7591409328811025, 2.3e-14,                      NAN,                      NAN,                      NAN },
   {     21, 0,    0.0014122379540283372,       92.000342403628125,       2.9279213936682114, 2.1e-14,                      NAN,                      NAN,                      NAN },
   {     77, 0,       1.0425160093887711,       91.441674527551939,       5.0554313614171438, 2.2e-14,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.56423126139439517,       111.77720463169035,       5.5788818100045816, 1.9e-14,                      NAN,                      NAN,                      NAN },
   {    167, 0,      0.74113088503421509,       124.16469247770411,       2.1066971187145058, 6.0e-14,                      NAN,                      NAN,                      NAN },
   {    200, 0,     0.079078559898006009,       114.98160229488369,       6.6416295251993684, 1.3e-14,                      NAN,                      NAN,                      NAN },
   {    202, 0,     -0.25734818868996329,       112.00988305771658,       8.3062952632325793, 1.1e-14,                      NAN,                      NAN,                      NAN },
   {    206, 0,     0.043572809517527969,       106.08800258104742,       9.8724568750640795, 7.4e-15,                      NAN,                      NAN,                      NAN },
   {    251, 0,      0.32056700814803607,       108.52646097903106,       3.0149958540601682, 3.0e-14,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_wma_n20[] =
{
   {     19, 0,      0.26561442823074399,       92.304357142857143,       2.5911974066056795, 2.8e-14,                      NAN,                      NAN,      0.26561442823074399 },
   {     20, 0,      0.18293918379986793,       91.874261904761909,       2.7591409328811025, 2.5e-14,                      NAN,                      NAN,      0.18293918379986765 },
   {     21, 0,      0.17872950280052641,       91.387619047619054,       2.9279213936682114, 2.3e-14,                      NAN,                      NAN,      0.17872950280052627 },
   {     90, 0,      0.89327295808117013,       107.18519047619047,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.89327295808117102 },
   {    100, 0,      0.55189211922017078,                  114.842,       5.5788818100045816, 1.9e-14,                      NAN,                      NAN,      0.55189211922017078 },
   {    141, 0,      0.28125666720248654,       130.55604761904763,       4.9213472748831686, 2.1e-14,                      NAN,                      NAN,      0.28125666720248665 },
   {    167, 0,      0.90051975294800579,       123.87490476190476,       2.1066971187145058, 6.4e-14,                      NAN,                      NAN,      0.90051975294800501 },
   {    200, 0,      0.28199734693883072,       112.91157142857143,       6.6416295251993684, 1.4e-14,                      NAN,                      NAN,      0.28199734693883066 },
   {    202, 0,    -0.062705737260417227,                  109.696,       8.3062952632325793, 8.7e-15,                      NAN,                      NAN,    -0.062705737260417185 },
   {    223, 0,       1.2490730115865769,       95.658000000000001,       2.7640830305907955, 4.3e-14,                      NAN,                      NAN,       1.2490730115865758 },
   {    251, 0,      0.36832486039230333,                  109.458,       3.0149958540601682, 3.1e-14,                      NAN,                      NAN,      0.36832486039230317 },
};

static const PctbRow pctb_dema_n20[] =
{
   {     38, 0,       0.3905995450764122,       85.153018369866643,       2.2006726812499853, 3.3e-14,                      NAN,                      NAN,      0.39059954507641259 },
   {     39, 0,      0.39447008679533985,       84.752852623878354,       2.0796298348504236, 3.5e-14,                      NAN,                      NAN,      0.39447008679533957 },
   {     40, 0,       0.3846759472300153,       84.326613973544369,       2.0629130495491079, 3.5e-14,                      NAN,                      NAN,      0.38467594723001536 },
   {     90, 0,      0.75872178048712047,       112.59593942581688,       10.053328864112624, 1.2e-14,                      NAN,                      NAN,      0.75872178048712102 },
   {    100, 0,      0.39102317661185176,       118.43187527084889,       5.5788818100045816, 1.8e-14,                      NAN,                      NAN,      0.39102317661185171 },
   {    126, 0,      0.58220142810785391,       129.22464614277567,       5.3994008926917063, 2.3e-14,                      NAN,                      NAN,      0.58220142810785391 },
   {    138, 0,      0.02277593911338335,       134.03222748007008,       5.3236143737126547, 1.7e-14,                      NAN,                      NAN,     0.022775939113383614 },
   {    167, 0,      0.91141783336428384,       123.78306894345479,       2.1066971187145058, 6.4e-14,                      NAN,                      NAN,      0.91141783336428306 },
   {    200, 0,      0.44571104111837001,       108.56226860880227,       6.6416295251993684, 1.5e-14,                      NAN,                      NAN,      0.44571104111836995 },
   {    223, 0,       1.4351363927344647,       93.600821462219102,       2.7640830305907955, 4.5e-14,                      NAN,                      NAN,       1.4351363927344636 },
   {    251, 0,      0.32044203891355411,       110.03546603295653,       3.0149958540601682, 3.0e-14,                      NAN,                      NAN,      0.32044203891355394 },
};

static const PctbRow pctb_tick100_n20[] =
{
   {     19, 0,      0.72613350843332269,                 100.0055,    0.0049749371855356446, 2.0e-11,      0.72613350843351165,      0.72613350843351165,      0.72613350843332269 },
   {     20, 0,      0.70412414523193145,                  100.006,    0.0048989794855688624, 2.0e-11,      0.70412414523214717,      0.70412414523214717,      0.70412414523193134 },
   {     21, 0,      0.22361460080371667,                 100.0055,    0.0049749371855356446, 1.6e-11,      0.22361460080404119,      0.22361460080404119,       0.2236146008037167 },
   {     23, 0,                     0.25,                  100.005,     0.005000000000002558, 1.6e-11,      0.25000000000035527,      0.25000000000106581,                     0.25 },
   {    100, 0,      0.72613350843332269,                 100.0055,    0.0049749371855356446, 2.0e-11,      0.72613350843279756,      0.72613350843351165,      0.72613350843332269 },
   {    200, 0,                     0.75,                  100.005,     0.005000000000002558, 2.0e-11,      0.75000000000035527,      0.75000000000177636,                     0.75 },
   {    210, 0,       0.9330127018922193,                 100.0025,    0.0043301270189244085, 2.5e-11,       0.9330127018923996,      0.93301270189650198,      0.93301270189221963 },
   {    211, 0,      0.35566243270259357,                 100.0025,    0.0043301270189244085, 2.0e-11,      0.35566243270280695,      0.35566243270772974,      0.35566243270259346 },
   {    233, 0,      0.15930742806537657,                 100.0065,     0.004769696007087168, 1.6e-11,      0.15930742806621712,      0.15930742806547227,      0.15930742806537668 },
   {    299, 0,      0.27386649156667731,       100.00450000000001,    0.0049749371855356446, 1.6e-11,       0.2738664915672025,      0.27386649156791659,      0.27386649156667731 },
};

static const PctbRow pctb_tick100_n20_k13[] =
{
   {     19, 0,      0.97613350843332269,                 100.0055,    0.0049749371855356446, 2.3e-11,                      NAN,                      NAN,                      NAN },
   {     20, 0,      0.95412414523193145,                  100.006,    0.0048989794855688624, 2.3e-11,                      NAN,                      NAN,                      NAN },
   {     21, 0,       0.4736146008037167,                 100.0055,    0.0049749371855356446, 1.8e-11,                      NAN,                      NAN,                      NAN },
   {     23, 0,                      0.5,                  100.005,     0.005000000000002558, 1.8e-11,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.97613350843332269,                 100.0055,    0.0049749371855356446, 2.3e-11,                      NAN,                      NAN,                      NAN },
   {    200, 0,                      1.0,                  100.005,     0.005000000000002558, 2.3e-11,                      NAN,                      NAN,                      NAN },
   {    210, 0,       1.1830127018922194,                 100.0025,    0.0043301270189244085, 2.8e-11,                      NAN,                      NAN,                      NAN },
   {    233, 0,      0.40930742806537657,                 100.0065,     0.004769696007087168, 1.8e-11,                      NAN,                      NAN,                      NAN },
   {    299, 0,      0.52386649156667731,       100.00450000000001,    0.0049749371855356446, 1.9e-11,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_big1e8_n20[] =
{
   {     19, 0,      0.50000015584010649,       100000000.01000001,    0.0083665985199326241, 1.1e-05,      0.50000044525738552,      0.50000044525738552,                      0.5 },
   {     20, 0,      0.48446590599807682,           100000000.0105,    0.0080467366290010722, 1.1e-05,      0.48446595649134505,      0.48446641944787805,      0.48446575584996998 },
   {     21, 0,       0.2011927808778094,       100000000.01000001,    0.0083665985199326241, 9.1e-06,      0.20119311169014362,      0.20119311169014362,      0.20119284766640175 },
   {     67, 0,      0.10430793827937365,       100000000.01350001,    0.0085293588166542103, 8.4e-06,      0.10430768585706911,      0.10430812261801499,      0.10430799231162929 },
   {    100, 0,      0.51553439429812775,           100000000.0095,    0.0080467369530706594, 1.2e-05,      0.51553450646518795,      0.51553450646518795,      0.51553424415003002 },
   {    133, 0,      0.96468899364172578,       100000000.00650001,    0.0072629192646564986, 1.6e-05,      0.96468912499115211,      0.96468912499115211,      0.96468916380452174 },
   {    200, 0,      0.77469954224668625,           100000000.0105,    0.0086458063215201931, 1.2e-05,      0.77470010858137572,      0.77470010858137572,      0.77469959268396416 },
   {    213, 0,     0.044908788580885206,            100000000.013,     0.007141425686777652, 9.7e-06,     0.044909473701781728,     0.044909473701781728,     0.044908972690896817 },
   {    243, 0,      0.38375255144983361,            100000000.014,    0.0086023231260130527, 9.8e-06,      0.38375293612244754,      0.38375293612244754,      0.38375236125618079 },
   {    281, 0,      0.89240325258551834,           100000000.0095,    0.0066895429908685978, 1.6e-05,      0.89240356182234326,      0.89240356182234326,      0.89240342369476611 },
   {    299, 0,      0.27626072193420703,       100000000.00650001,    0.0072629192646564986, 1.1e-05,      0.27626119060783966,      0.27626119060783966,      0.27626077298300805 },
};

static const PctbRow pctb_big1e8_n20_k13[] =
{
   {     19, 0,      0.75000015584010649,       100000000.01000001,    0.0083665985199326241, 1.2e-05,                      NAN,                      NAN,                      NAN },
   {     20, 0,      0.73446590599807682,           100000000.0105,    0.0080467366290010722, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {     21, 0,       0.4511927808778094,       100000000.01000001,    0.0083665985199326241, 1.1e-05,                      NAN,                      NAN,                      NAN },
   {    100, 0,      0.76553439429812775,           100000000.0095,    0.0080467369530706594, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {    133, 0,       1.2146889936417258,       100000000.00650001,    0.0072629192646564986, 1.7e-05,                      NAN,                      NAN,                      NAN },
   {    200, 0,       1.0246995422466862,           100000000.0105,    0.0086458063215201931, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {    211, 0,      0.95554460695168097,       100000000.01449999,    0.0066895409304028944, 1.7e-05,                      NAN,                      NAN,                      NAN },
   {    213, 0,      0.29490878858088521,            100000000.013,     0.007141425686777652, 1.2e-05,                      NAN,                      NAN,                      NAN },
   {    281, 0,       1.1424032525855183,           100000000.0095,    0.0066895429908685978, 1.8e-05,                      NAN,                      NAN,                      NAN },
   {    299, 0,      0.52626072193420703,       100000000.00650001,    0.0072629192646564986, 1.3e-05,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_fine100_n20[] =
{
   {     19, 0,      0.77469964078464981,           100.0000000105,   8.6458090509825876e-09, 1.2e-05,      0.77469963978964307,      0.77469963978964307,      0.77469959268396416 },
   {     20, 0,      0.75314481312581516,            100.000000011,    8.888195312613152e-09, 1.2e-05,      0.75314493061806598,      0.75314493061806598,      0.75314477770835531 },
   {     21, 0,      0.19060087278210977,            100.000000011,    8.888195312613152e-09, 8.5e-06,       0.1906008624176794,       0.1906008624176794,      0.19060082724534347 },
   {     33, 0,      0.45877610656817436,       100.00000001150001,   9.0967036277423574e-09, 9.6e-06,        0.458776314258108,      0.45877709535738026,      0.45877627175530156 },
   {    100, 0,      0.77086822522944942,            100.000000011,   8.3066248068297299e-09, 1.3e-05,      0.77086815493552907,      0.77086901032800881,      0.77086816944298053 },
   {    138, 0,      0.17499448187678443,           100.0000000085,   6.5383480597440202e-09, 1.2e-05,      0.17499573458119022,      0.17499464785028326,      0.17499437701670414 },
   {    200, 0,      0.74913945518919456,       100.00000001149999,   8.5293620727346838e-09, 1.2e-05,      0.74913924596863879,      0.74914049555106998,      0.74913941224823344 },
   {    247, 0,       0.7041241855204956,       100.00000001399999,   7.3484709966766632e-09, 1.4e-05,      0.70412442407453069,      0.70412635792710343,      0.70412414523193168 },
   {    250, 0,     0.099108272774618297,            100.000000012,   7.4833161523230298e-09, 9.5e-06,     0.099108606956245973,      0.09910955645884903,      0.09910813713136328 },
   {    253, 0,      0.13706331828042889,       100.00000001149999,   7.9214908852202706e-09, 9.2e-06,      0.13706360760316849,      0.13706450458402064,      0.13706321821876319 },
   {    289, 0,      0.94232606774363836,            100.000000008,    6.782329496660508e-09, 1.6e-05,      0.94232646920781793,      0.94232594538871572,      0.94232586846469124 },
   {    294, 0,      0.94757198415038368,       100.00000000749999,   6.9821194547204734e-09, 1.6e-05,      0.94757216739293704,      0.94757165856269709,      0.94757179627464561 },
   {    299, 0,      0.80361539425449036,           100.0000000095,   8.6458088455238507e-09, 1.2e-05,      0.80361549948265476,      0.80361591040024194,      0.80361533928227613 },
};

static const PctbRow pctb_fine100_n20_k13[] =
{
   {     19, 0,       1.0246996407846498,           100.0000000105,   8.6458090509825876e-09, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {     20, 0,       1.0031448131258152,            100.000000011,    8.888195312613152e-09, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {     21, 0,       0.4406008727821098,            100.000000011,    8.888195312613152e-09, 9.7e-06,                      NAN,                      NAN,                      NAN },
   {    100, 0,       1.0208682252294494,            100.000000011,   8.3066248068297299e-09, 1.4e-05,                      NAN,                      NAN,                      NAN },
   {    200, 0,      0.99913945518919456,       100.00000001149999,   8.5293620727346838e-09, 1.4e-05,                      NAN,                      NAN,                      NAN },
   {    250, 0,      0.34910827277461831,            100.000000012,   7.4833161523230298e-09, 1.1e-05,                      NAN,                      NAN,                      NAN },
   {    287, 0,      0.42857153214912636,       100.00000000899999,   7.0000000716529545e-09, 1.3e-05,                      NAN,                      NAN,                      NAN },
   {    289, 0,       1.1923260677436385,            100.000000008,    6.782329496660508e-09, 1.8e-05,                      NAN,                      NAN,                      NAN },
   {    294, 0,       1.1975719841503836,       100.00000000749999,   6.9821194547204734e-09, 1.8e-05,                      NAN,                      NAN,                      NAN },
   {    299, 0,       1.0536153942544904,           100.0000000095,   8.6458088455238507e-09, 1.4e-05,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_peg_n20[] =
{
   {     19, 0,      0.70848943766993067,       1.0001432662393108,   0.00010391603879722283, 9.5e-12,      0.70848943767056749,      0.70848943767056749,      0.70848943766981709 },
   {     20, 0,      0.67519880838546653,       1.0001544905594362,   9.9880592390075537e-05, 9.7e-12,      0.67519880838607349,      0.67519880838607349,      0.67519880838547464 },
   {     21, 0,      0.64472712631857598,       1.0001654922759025,   9.4215325779816093e-05, 1.1e-11,      0.64472712631895335,      0.64472712631895335,      0.64472712631859208 },
   {    100, 0,      0.27845887117330859,       1.0001016674829954,   1.7277704957751641e-06, 4.6e-10,      0.27845887118423507,      0.27845887118423507,      0.27845887117804791 },
   {    161, 0,      0.27845913283658463,       1.0001000000080518,   8.3430272686125863e-12, 9.5e-05,      0.27845422970976885,      0.27845422970976885,      0.27846008282337131 },
   {    214, 2,                      0.5,       1.0001000000000004,   4.8393499691331258e-17,     0.0,                      NAN,                      NAN,                      NAN }, /* exact 0.44264606653235955 */
   {    215, 1,                      0.5,       1.0001000000000004,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {    216, 1,                      0.5,       1.0001000000000004,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {    540, 0,     -0.58972473588516838,       1.0000990909090914,   3.9626354032403547e-06, 2.4e-10,     -0.58972473585969332,     -0.58972473588771068,     -0.58972473588516727 },
   {    542, 0,     -0.44483546825354775,       1.0000917054845984,   2.3330597759234992e-05, 3.8e-11,     -0.44483546825087322,      -0.4448354682532526,     -0.44483546825384573 },
   {    558, 0,      0.85796800827352537,       0.9999496011062684,   9.0754990345182548e-05, 1.2e-11,      0.85796800827328468,      0.85796800827083808,       0.8579680082736052 },
   {    563, 0,        1.059337174497188,      0.99999511357100568,   0.00015404150169739521, 7.4e-12,       1.0593371744975255,        1.059337174496084,       1.0593371744972504 },
   {    694, 0,      0.27845947300265866,        1.000100000007945,   8.2320756714439737e-12, 9.6e-05,      0.27845660031288771,      0.27844311377245506,      0.27846205899512139 },
   {    747, 2,                      0.5,       1.0001000000000004,   4.8393499691331258e-17,     0.0,                      NAN,                      NAN,                      NAN }, /* exact 0.44264606653235955 */
   {   1000, 1,                      0.5,       1.0001000000000004,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {   1079, 1,                      0.5,       1.0001000000000004,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_flatpos_n20[] =
{
   {     19, 1,                      0.5,                      5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     20, 1,                      0.5,                      5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     21, 1,                      0.5,                      5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     59, 1,                      0.5,                      5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_flatpos_n20_k25_15[] =
{
   {     19, 1,                      0.5,                      5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     20, 1,                      0.5,                      5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     21, 1,                      0.5,                      5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     59, 1,                      0.5,                      5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_flatneg_n20[] =
{
   {     19, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     20, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     21, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     59, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_flatneg_ema_n20[] =
{
   {     19, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     20, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     21, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     59, 1,                      0.5,                     -5.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_zero_n20[] =
{
   {     19, 1,                      0.5,                      0.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     20, 1,                      0.5,                      0.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     21, 1,                      0.5,                      0.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     59, 1,                      0.5,                      0.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_stepflat_n20[] =
{
   {     19, 0,      0.20901483689438513,       92.891000000000005,       2.5911974066056795, 2.7e-14,      0.20901483689438599,      0.20901483689438599,      0.20901483689438516 },
   {     20, 0,      0.10497215382837141,       92.734750000000005,       2.7591409328811025, 2.4e-14,       0.1049721538283722,       0.1049721538283722,      0.10497215382837104 },
   {     21, 0,     0.094400825593143223,       92.375249999999994,       2.9279213936682114, 2.2e-14,      0.09440082559314289,     0.094400825593144097,     0.094400825593143042 },
   {     23, 0,    -0.051864985555983956,       91.290750000000003,       3.7557872926325313, 1.7e-14,    -0.051864985555983886,    -0.051864985555982936,    -0.051864985555984157 },
   {     33, 0,      0.64101941165507936,       87.297250000000005,       2.9654605354817991, 2.9e-14,       0.6410194116550787,      0.64101941165508014,      0.64101941165507936 },
   {     38, 0,      0.27603438975756872,       86.161500000000004,       2.2006726812499853, 3.1e-14,      0.27603438975756645,      0.27603438975756805,      0.27603438975756939 },
   {     43, 0,      0.30352169133702783,       85.447500000000005,       2.0008570038860847, 3.5e-14,      0.30352169133702511,      0.30352169133702689,      0.30352169133702728 },
   {     57, 0,      0.44264606653235955,       83.890749999999997,     0.068652658360765118, 1.1e-12,      0.44264606653236699,      0.44264606653236699,      0.44264606653235955 },
   {     58, 1,                      0.5,                   83.875,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     59, 1,                      0.5,                   83.875,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     79, 1,                      0.5,                   83.875,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_alt_n20[] =
{
   {     19, 0,                     0.25,                      0.0,                      1.0, 1.4e-15,                     0.25,                     0.25,                     0.25 },
   {     20, 0,                     0.75,                      0.0,                      1.0, 1.8e-15,                     0.75,                     0.75,                     0.75 },
   {     21, 0,                     0.25,                      0.0,                      1.0, 1.4e-15,                     0.25,                     0.25,                     0.25 },
   {     59, 0,                     0.25,                      0.0,                      1.0, 1.4e-15,                     0.25,                     0.25,                     0.25 },
};

static const PctbRow pctb_alt_n5[] =
{
   {      4, 0,      0.70412414523193145,      0.20000000000000001,       0.9797958971132712, 1.8e-15,      0.70412414523193145,      0.70412414523193145,      0.70412414523193156 },
   {      5, 0,      0.29587585476806849,     -0.20000000000000001,       0.9797958971132712, 1.4e-15,      0.29587585476806849,      0.29587585476806849,      0.29587585476806844 },
   {      6, 0,      0.70412414523193145,      0.20000000000000001,       0.9797958971132712, 1.8e-15,      0.70412414523193145,      0.70412414523193145,      0.70412414523193156 },
   {     59, 0,      0.29587585476806849,     -0.20000000000000001,       0.9797958971132712, 1.4e-15,      0.29587585476806849,      0.29587585476806849,      0.29587585476806844 },
};

static const PctbRow pctb_eps_n20[] =
{
   {     19, 0,      0.70412414523193145,   4.4408920985006264e-17,   2.1755839288168293e-16, 1.8e-15,      0.70412414523193145,      0.70412414523193145,                      NAN },
   {     20, 0,      0.70412414523193145,   4.4408920985006264e-17,   2.1755839288168293e-16, 1.8e-15,      0.70412414523193145,      0.70412414523193145,                      NAN },
   {     21, 0,      0.68344984642633566,    6.661338147750939e-17,   2.1181705310112557e-16, 1.8e-15,      0.68344984642633566,      0.68344984642633566,                      NAN },
   {     27, 0,      0.15930742806537657,    6.661338147750939e-17,   2.1181705310112557e-16, 1.3e-15,      0.15930742806537659,      0.15930742806537659,                      NAN },
   {     49, 0,                     0.75,                      0.0,   2.2204460492503131e-16, 1.8e-15,                     0.75,                     0.75,                      NAN },
   {     50, 0,      0.72613350843332269,   2.2204460492503132e-17,   2.2093159237770887e-16, 1.8e-15,      0.72613350843332281,      0.72613350843332281,                      NAN },
   {    100, 0,      0.31655015357366428,   -6.661338147750939e-17,   2.1181705310112557e-16, 1.5e-15,      0.31655015357366428,      0.31655015357366428,                      NAN },
   {    156, 0,                      1.0,  -1.3322676295501878e-16,   1.7763568394002506e-16, 2.1e-15,                      1.0,                      1.0,                      NAN },
   {    199, 0,                     0.25,                      0.0,   2.2204460492503131e-16, 1.4e-15,                     0.25,                     0.25,                      NAN },
};

static const PctbRow pctb_neg_n20[] =
{
   {     19, 0,      0.79098516310561484,      -92.891000000000005,       2.5911974066056795, 3.7e-14,      0.79098516310561395,      0.79098516310561395,      0.79098516310561484 },
   {     20, 0,      0.89502784617162856,      -92.734750000000005,       2.7591409328811025, 3.6e-14,      0.89502784617162778,      0.89502784617162778,        0.895027846171629 },
   {     21, 0,      0.90559917440685678,      -92.375249999999994,       2.9279213936682114, 3.4e-14,      0.90559917440685711,      0.90559917440685589,        0.905599174406857 },
   {     45, 0,     0.073543940638678612,      -86.896249999999995,       2.2591717923832175, 2.8e-14,     0.073543940638681887,     0.073543940638680319,     0.073543940638679195 },
   {     90, 0,    -0.017303777713337899,      -102.19750000000001,       10.053328864112624, 7.4e-15,    -0.017303777713337815,    -0.017303777713337461,    -0.017303777713339071 },
   {    100, 0,      0.35523532716687239,      -112.76949999999999,       5.5788818100045816, 1.7e-14,      0.35523532716687339,      0.35523532716687212,      0.35523532716687234 },
   {    123, 0,    -0.054765563563968653,                -119.9395,       4.1956912124225729, 2.1e-14,    -0.054765563563969756,    -0.054765563563969756,    -0.054765563563969138 },
   {    165, 0,      0.46458501346322056,      -122.86150000000001,       2.3189335372105857, 4.7e-14,      0.46458501346322112,      0.46458501346321807,      0.46458501346322029 },
   {    200, 0,      0.84692163892276628,                -116.3365,       6.6416295251993684, 1.8e-14,      0.84692163892276506,      0.84692163892276506,       0.8469216389227664 },
   {    202, 0,        1.188544028204245,                 -113.877,       8.3062952632325793, 1.6e-14,       1.1885440282042445,       1.1885440282042445,        1.188544028204245 },
   {    223, 0,     -0.29456730340353504,      -95.155000000000001,       2.7640830305907955, 2.9e-14,      -0.2945673034035346,     -0.29456730340353721,     -0.29456730340353404 },
   {    234, 0,   -0.0062361752288423132,                 -102.486,       6.9799436960479841, 1.1e-14,    -0.006236175228841863,   -0.0062361752288438987,   -0.0062361752288425318 },
   {    251, 0,      0.72388090487454737,      -110.57000000000001,       3.0149958540601682, 3.7e-14,      0.72388090487454559,      0.72388090487454437,      0.72388090487454748 },
};

static const PctbRow pctb_subulp_n20[] =
{
   {     19, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     20, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     21, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
   {     30, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      0.5,                      NAN,                      NAN }, /* exact 1.5897247358851685 */
   {     31, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      0.5,                      NAN,                      NAN }, /* exact 0.44264606653235955 */
   {     79, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      0.5,                      NAN,                      NAN }, /* exact 0.44264606653235955 */
   {     99, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      0.5,                      NAN,                      NAN },
};

static const PctbRow pctb_subulp_ema_n20[] =
{
   {     19, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     20, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     21, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
   {     30, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      NAN,                      NAN,                      NAN }, /* exact 1.5378330817953985 */
   {     31, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      NAN,                      NAN,                      NAN }, /* exact 0.40115875411472396 */
   {     79, 2,                      0.5,              100000000.0,   3.2476327892695914e-09,     0.0,                      NAN,                      NAN,                      NAN }, /* exact 0.4828759915403224 */
   {     99, 1,                      0.5,              100000000.0,                      0.0,     0.0,                      NAN,                      NAN,                      NAN },
};

static const PctbRow pctb_long_sma_n100000[] =
{
   {  99999, 0,      0.26042858870746688,       100.00235505943299,       11.555512259078105, 6.6e-15,      0.26042858870746599,      0.26042858870746599,      0.26042858870746688 },
   { 100000, 0,      0.86765817141172219,       100.00270559339523,       11.555496101772844, 9.5e-15,      0.86765817141171975,      0.86765817141171975,      0.86765817141172374 },
   { 100001, 0,      0.75007706813238995,       100.00269479494095,       11.555484795449328, 8.9e-15,      0.75007706813238884,      0.75007706813238884,      0.75007706813239061 },
   { 100057, 0,      0.14692671720293649,       100.00205549933911,       11.555061910439923, 6.1e-15,      0.14692671720293232,      0.14692671720293232,      0.14692671720293587 },
   { 100276, 0,      0.13557700929526695,       100.00002269001007,        11.55617727142655, 6.0e-15,      0.13557700929526514,      0.13557700929526514,      0.13557700929526537 },
   { 100336, 0,     0.067802890429229393,       100.00112377867698,       11.556313727674031, 5.7e-15,     0.067802890429231544,     0.067802890429231544,     0.067802890429228185 },
   { 100362, 0,     0.099530221527881005,       99.999616103315347,        11.55628831218397, 5.8e-15,     0.099530221527879811,     0.099530221527879811,     0.099530221527882184 },
   { 100477, 0,      0.93220105207318926,       99.999266808581353,       11.555410058228178, 9.8e-15,      0.93220105207318871,      0.93220105207318871,      0.93220105207318849 },
   { 100499, 0,      0.86330865268602752,       99.999108736228948,       11.555230880296529, 9.5e-15,      0.86330865268603107,      0.86330865268603107,      0.86330865268602841 },
};

static const PctbRow pctb_long_ema_n100000[] =
{
   {  99999, 0,      0.26042858870746688,       100.00235505943299,       11.555512259078105, 6.6e-15,                      NAN,                      NAN,      0.26042858870746688 },
   { 100000, 0,      0.86765840187662324,       100.00269494085018,       11.555496101772844, 9.5e-15,                      NAN,                      NAN,       0.8676584018766248 },
   { 100001, 0,      0.75007206348439603,       100.00292611947614,       11.555484795449328, 8.9e-15,                      NAN,                      NAN,       0.7500720634843967 },
   { 100095, 0,     0.071073614695532153,        100.0034084819428,       11.555764801948829, 5.7e-15,                      NAN,                      NAN,     0.071073614695531043 },
   { 100278, 0,      0.92955396866051765,       100.00153113608246,        11.55615886744615, 9.8e-15,                      NAN,                      NAN,      0.92955396866051576 },
   { 100336, 0,     0.067770436605573428,       100.00262396494831,       11.556313727674031, 5.7e-15,                      NAN,                      NAN,     0.067770436605572235 },
   { 100477, 0,      0.93212625295707741,       100.00272414641601,       11.555410058228178, 9.8e-15,                      NAN,                      NAN,      0.93212625295707663 },
   { 100499, 0,      0.86320400140736864,       100.00394581497623,       11.555230880296529, 9.5e-15,                      NAN,                      NAN,      0.86320400140736953 },
};

static const PctbRow pctb_bollinger_hd[] =
{
   {     19, 0,      0.68300336206623447,                58.332825,       1.9359958527525312, 3.0e-14,      0.68300336206623469,      0.68300336206623469,      0.68300336206623469 },
};

static const PctbGolden pctbGoldens[] =
{
   { "sma-n2",             PCTB_CORPUS,         0,    251,      2,         2.0,         2.0, TA_MAType_SMA,     0,      1,   251, PCTB_ROWS(pctb_sma_n2) },
   { "sma-n2-k13",         PCTB_CORPUS,         0,    251,      2,         1.0,         3.0, TA_MAType_SMA,     0,      1,   251, PCTB_ROWS(pctb_sma_n2_k13) },
   { "sma-n2-k25-15",      PCTB_CORPUS,         0,    251,      2,         2.5,         1.5, TA_MAType_SMA,     0,      1,   251, PCTB_ROWS(pctb_sma_n2_k25_15) },
   { "sma-n5",             PCTB_CORPUS,         0,    251,      5,         2.0,         2.0, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5) },
   { "sma-n5-k13",         PCTB_CORPUS,         0,    251,      5,         1.0,         3.0, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5_k13) },
   { "sma-n5-k25-15",      PCTB_CORPUS,         0,    251,      5,         2.5,         1.5, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5_k25_15) },
   { "sma-n20",            PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20) },
   { "sma-n20-k13",        PCTB_CORPUS,         0,    251,     20,         1.0,         3.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k13) },
   { "sma-n20-k25-15",     PCTB_CORPUS,         0,    251,     20,         2.5,         1.5, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k25_15) },
   { "sma-n50",            PCTB_CORPUS,         0,    251,     50,         2.0,         2.0, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50) },
   { "sma-n50-k13",        PCTB_CORPUS,         0,    251,     50,         1.0,         3.0, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50_k13) },
   { "sma-n50-k25-15",     PCTB_CORPUS,         0,    251,     50,         2.5,         1.5, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50_k25_15) },
   { "sma-n200",           PCTB_CORPUS,         0,    251,    200,         2.0,         2.0, TA_MAType_SMA,     0,    199,    53, PCTB_ROWS(pctb_sma_n200) },
   { "sma-n200-k13",       PCTB_CORPUS,         0,    251,    200,         1.0,         3.0, TA_MAType_SMA,     0,    199,    53, PCTB_ROWS(pctb_sma_n200_k13) },
   { "sma-n200-k25-15",    PCTB_CORPUS,         0,    251,    200,         2.5,         1.5, TA_MAType_SMA,     0,    199,    53, PCTB_ROWS(pctb_sma_n200_k25_15) },
   { "sma-n2-k1",          PCTB_CORPUS,         0,    251,      2,         1.0,         1.0, TA_MAType_SMA,     0,      1,   251, PCTB_ROWS(pctb_sma_n2_k1) },
   { "sma-n5-k1",          PCTB_CORPUS,         0,    251,      5,         1.0,         1.0, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5_k1) },
   { "sma-n5-k25",         PCTB_CORPUS,         0,    251,      5,         2.5,         2.5, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5_k25) },
   { "sma-n20-k1",         PCTB_CORPUS,         0,    251,     20,         1.0,         1.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k1) },
   { "sma-n20-k25",        PCTB_CORPUS,         0,    251,     20,         2.5,         2.5, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k25) },
   { "sma-n50-k1",         PCTB_CORPUS,         0,    251,     50,         1.0,         1.0, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50_k1) },
   { "sma-n50-k25",        PCTB_CORPUS,         0,    251,     50,         2.5,         2.5, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50_k25) },
   { "sma-n5-k0",          PCTB_CORPUS,         0,    251,      5,         0.0,         0.0, TA_MAType_SMA,     0,      4,   248, PCTB_ROWS(pctb_sma_n5_k0) },
   { "sma-n50-k0",         PCTB_CORPUS,         0,    251,     50,         0.0,         0.0, TA_MAType_SMA,     0,     49,   203, PCTB_ROWS(pctb_sma_n50_k0) },
   { "sma-n20-k01-02",     PCTB_CORPUS,         0,    251,     20,         0.1,         0.2, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k01_02) },
   { "sma-n20-km1-2",      PCTB_CORPUS,         0,    251,     20,        -1.0,         2.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_km1_2) },
   { "sma-n20-km2-m2",     PCTB_CORPUS,         0,    251,     20,        -2.0,        -2.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_km2_m2) },
   { "sma-n20-km2-2",      PCTB_CORPUS,         0,    251,     20,        -2.0,         2.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_km2_2) },
   { "sma-n20-k0",         PCTB_CORPUS,         0,    251,     20,         0.0,         0.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_k0) },
   { "sma-n20-kmax",       PCTB_CORPUS,         0,    251,     20, TA_REAL_MAX, TA_REAL_MAX, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_kmax) },
   { "sma-n20-kmin",       PCTB_CORPUS,         0,    251,     20, TA_REAL_MIN, TA_REAL_MIN, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_sma_n20_kmin) },
   { "sma-n20-s100",       PCTB_CORPUS,       100,    251,     20,         2.0,         2.0, TA_MAType_SMA,     0,    100,   152, PCTB_ROWS(pctb_sma_n20_s100) },
   { "sma-n20-s251",       PCTB_CORPUS,       251,    251,     20,         2.0,         2.0, TA_MAType_SMA,     0,    251,     1, PCTB_ROWS(pctb_sma_n20_s251) },
   { "ema-n5",             PCTB_CORPUS,         0,    251,      5,         2.0,         2.0, TA_MAType_EMA,     0,      4,   248, PCTB_ROWS(pctb_ema_n5) },
   { "ema-n20",            PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_EMA,     0,     19,   233, PCTB_ROWS(pctb_ema_n20) },
   { "ema-n20-s19-u0",     PCTB_CORPUS,        19,    251,     20,         2.0,         2.0, TA_MAType_EMA,     0,     19,   233, PCTB_ROWS(pctb_ema_n20_s19_u0) },
   { "ema-n20-s20-u0",     PCTB_CORPUS,        20,    251,     20,         2.0,         2.0, TA_MAType_EMA,     0,     20,   232, PCTB_ROWS(pctb_ema_n20_s20_u0) },
   { "ema-n20-s100-u0",    PCTB_CORPUS,       100,    251,     20,         2.0,         2.0, TA_MAType_EMA,     0,    100,   152, PCTB_ROWS(pctb_ema_n20_s100_u0) },
   { "ema-n20-s251-u0",    PCTB_CORPUS,       251,    251,     20,         2.0,         2.0, TA_MAType_EMA,     0,    251,     1, PCTB_ROWS(pctb_ema_n20_s251_u0) },
   { "ema-n20-s0-u15",     PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_EMA,    15,     34,   218, PCTB_ROWS(pctb_ema_n20_s0_u15) },
   { "ema-n20-s100-u15",   PCTB_CORPUS,       100,    251,     20,         2.0,         2.0, TA_MAType_EMA,    15,    100,   152, PCTB_ROWS(pctb_ema_n20_s100_u15) },
   { "ema-n20-s0-u100",    PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_EMA,   100,    119,   133, PCTB_ROWS(pctb_ema_n20_s0_u100) },
   { "ema-n20-k25-15",     PCTB_CORPUS,         0,    251,     20,         2.5,         1.5, TA_MAType_EMA,     0,     19,   233, PCTB_ROWS(pctb_ema_n20_k25_15) },
   { "wma-n20",            PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_WMA,     0,     19,   233, PCTB_ROWS(pctb_wma_n20) },
   { "dema-n20",           PCTB_CORPUS,         0,    251,     20,         2.0,         2.0, TA_MAType_DEMA,    0,     38,   214, PCTB_ROWS(pctb_dema_n20) },
   { "tick100-n20",        PCTB_TICK100,        0,    299,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_tick100_n20) },
   { "tick100-n20-k13",    PCTB_TICK100,        0,    299,     20,         1.0,         3.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_tick100_n20_k13) },
   { "big1e8-n20",         PCTB_BIG1E8,         0,    299,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_big1e8_n20) },
   { "big1e8-n20-k13",     PCTB_BIG1E8,         0,    299,     20,         1.0,         3.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_big1e8_n20_k13) },
   { "fine100-n20",        PCTB_FINE100,        0,    299,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_fine100_n20) },
   { "fine100-n20-k13",    PCTB_FINE100,        0,    299,     20,         1.0,         3.0, TA_MAType_SMA,     0,     19,   281, PCTB_ROWS(pctb_fine100_n20_k13) },
   { "peg-n20",            PCTB_PEG,            0,   1079,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,  1061, PCTB_ROWS(pctb_peg_n20) },
   { "flatpos-n20",        PCTB_FLATPOS,        0,     59,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    41, PCTB_ROWS(pctb_flatpos_n20) },
   { "flatpos-n20-k25-15", PCTB_FLATPOS,        0,     59,     20,         2.5,         1.5, TA_MAType_SMA,     0,     19,    41, PCTB_ROWS(pctb_flatpos_n20_k25_15) },
   { "flatneg-n20",        PCTB_FLATNEG,        0,     59,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    41, PCTB_ROWS(pctb_flatneg_n20) },
   { "flatneg-ema-n20",    PCTB_FLATNEG,        0,     59,     20,         2.0,         2.0, TA_MAType_EMA,     0,     19,    41, PCTB_ROWS(pctb_flatneg_ema_n20) },
   { "zero-n20",           PCTB_ZERO,           0,     59,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    41, PCTB_ROWS(pctb_zero_n20) },
   { "stepflat-n20",       PCTB_STEPFLAT,       0,     79,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    61, PCTB_ROWS(pctb_stepflat_n20) },
   { "alt-n20",            PCTB_ALT,            0,     59,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    41, PCTB_ROWS(pctb_alt_n20) },
   { "alt-n5",             PCTB_ALT,            0,     59,      5,         2.0,         2.0, TA_MAType_SMA,     0,      4,    56, PCTB_ROWS(pctb_alt_n5) },
   { "eps-n20",            PCTB_EPS,            0,    199,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   181, PCTB_ROWS(pctb_eps_n20) },
   { "neg-n20",            PCTB_NEGCORPUS,      0,    251,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,   233, PCTB_ROWS(pctb_neg_n20) },
   { "subulp-n20",         PCTB_SUBULP,         0,     99,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,    81, PCTB_ROWS(pctb_subulp_n20) },
   { "subulp-ema-n20",     PCTB_SUBULP,         0,     99,     20,         2.0,         2.0, TA_MAType_EMA,     0,     19,    81, PCTB_ROWS(pctb_subulp_ema_n20) },
   { "long-sma-n100000",   PCTB_LONG,           0, 100499, 100000,         2.0,         2.0, TA_MAType_SMA,     0,  99999,   501, PCTB_ROWS(pctb_long_sma_n100000) },
   { "long-ema-n100000",   PCTB_LONG,           0, 100499, 100000,         2.0,         2.0, TA_MAType_EMA,     0,  99999,   501, PCTB_ROWS(pctb_long_ema_n100000) },
   { "bollinger-hd",       PCTB_HD,             0,     19,     20,         2.0,         2.0, TA_MAType_SMA,     0,     19,     1, PCTB_ROWS(pctb_bollinger_hd) },
};

#define NB_PCTB_GOLDENS ((int)(sizeof(pctbGoldens)/sizeof(pctbGoldens[0])))

/* Leg 2's deviation pairs. (2.5,2.5) and (0.1,0.1) see the equal branch
 * dropped (2*sd is exact, so (2,2) cannot); (2.5,1.5) and (-1.5,1.5) an
 * unfused upper band; (0,0) and (-2,2) coincide on every bar; (-2,-2) is a
 * negative width. */
static const double pctbK[][2] = { {2.0,2.0}, {2.5,2.5}, {0.1,0.1}, {2.5,1.5}, {1.0,3.0},
                                   {2.0,1.0}, {0.1,0.2}, {-1.0,2.0}, {-2.0,2.0}, {0.0,0.0},
                                   {-1.5,1.5}, {-0.2,0.2}, {-2.0,-2.0} };
#define NB_PCTB_K ((int)(sizeof(pctbK)/sizeof(pctbK[0])))

/* One pair per branch of the band map: (2,2) and (2.5,1.5). */
static const int pctbKBranch[2] = { 0, 3 };

static const int pctbPeriods[] = { 2, 5, 20, 33, 34, 50, 200 };
#define NB_PCTB_PERIODS ((int)(sizeof(pctbPeriods)/sizeof(pctbPeriods[0])))

static const int pctbDiffSeries[] = { PCTB_CORPUS, PCTB_NEGCORPUS, PCTB_WALK, PCTB_NEGWALK,
                                      PCTB_ALT, PCTB_FLATPOS, PCTB_FLATNEG, PCTB_ZERO, PCTB_EPS,
                                      PCTB_PEG, PCTB_TICK100, PCTB_BIG1E8, PCTB_FINE100,
                                      PCTB_STEPFLAT, PCTB_SUBULP };
#define NB_PCTB_DIFF_SERIES ((int)(sizeof(pctbDiffSeries)/sizeof(pctbDiffSeries[0])))

/* An unstable id and an MA type whose lookback reads it. */
static const struct { TA_FuncUnstId id; TA_MAType maType; } pctbUnst[] = {
   { TA_FUNC_UNST_EMA,  TA_MAType_EMA   }, { TA_FUNC_UNST_EMA,  TA_MAType_DEMA  },
   { TA_FUNC_UNST_EMA,  TA_MAType_TEMA  }, { TA_FUNC_UNST_EMA,  TA_MAType_ZLEMA },
   { TA_FUNC_UNST_KAMA, TA_MAType_KAMA  }, { TA_FUNC_UNST_T3,   TA_MAType_T3    },
   { TA_FUNC_UNST_MAMA, TA_MAType_MAMA  }, { TA_FUNC_UNST_RMA,  TA_MAType_RMA   },
};
#define NB_PCTB_UNST ((int)(sizeof(pctbUnst)/sizeof(pctbUnst[0])))

static int g_pctbGoldenCmp;
static int g_pctbDiffCmp;
static int g_pctbDiffZero;
static int g_pctbNanCmp;
static int g_pctbFloatCmp;
static int g_pctbIdentCmp;
static int g_pctbCrossCmp;
static int g_pctbLookbackCmp;
static int g_pctbAliasCmp;
static int g_pctbAnchorCmp;
static int g_pctbParamCmp;
static int g_pctbDegenCmp;
static int g_pctbSubUlpCmp;
static int g_pctbStreamCmp;
static int g_pctbStreamHuge;

/**** Local functions declarations. ****/
static void        pctb_build_series( const TA_History *history );
static ErrorNumber test_pctb_all( void );
static ErrorNumber test_pctb_goldens( void );
static ErrorNumber test_pctb_composition( void );
static ErrorNumber test_pctb_identity( void );
static ErrorNumber test_pctb_lookback( void );
static ErrorNumber test_pctb_alias( void );
static ErrorNumber test_pctb_anchor( void );
static ErrorNumber test_pctb_params( void );
static ErrorNumber test_pctb_degenerate( void );
static ErrorNumber test_pctb_stream( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_percentb( TA_History *history )
{
   ErrorNumber err;

   if( history->nbBars != 252 )
   {
      printf( "PERCENTB Fail: the goldens were captured on the 252-bar corpus, got %u bars\n",
              history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   pctb_build_series( history );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   err = test_pctb_all();
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   return err;
}

/**** Local functions definitions. ****/
#define PCTB_LEG(call) do { err = (call); if( err != TA_TEST_PASS ) return err; } while(0)

static ErrorNumber test_pctb_all( void )
{
   static const struct { const char *leg; int want; const int *got; } cov[] = {
      { "golden exact",            616, &g_pctbGoldenCmp   },
      { "composition",        58592010, &g_pctbDiffCmp     },
      { "zero width",         20928178, &g_pctbDiffZero    },
      { "NaN and Inf input",     76868, &g_pctbNanCmp      },
      { "float tier",            25912, &g_pctbFloatCmp    },
      { "author's identity",    936174, &g_pctbIdentCmp    },
      { "band crossing",           502, &g_pctbCrossCmp    },
      { "lookback",               2530, &g_pctbLookbackCmp },
      { "in-place",             996400, &g_pctbAliasCmp    },
      { "anchor",               657436, &g_pctbAnchorCmp   },
      { "parameters",             1194, &g_pctbParamCmp    },
      { "degenerate",            78269, &g_pctbDegenCmp    },
      { "sub-ulp width",           490, &g_pctbSubUlpCmp   },
      { "stream",                47696, &g_pctbStreamCmp   },
      { "stream near 1e15",         49, &g_pctbStreamHuge  },
   };
   ErrorNumber err;
   unsigned int c;
   int bad = 0;

   g_pctbGoldenCmp = g_pctbDiffCmp = g_pctbDiffZero = g_pctbNanCmp = 0;
   g_pctbFloatCmp = g_pctbIdentCmp = g_pctbCrossCmp = g_pctbLookbackCmp = 0;
   g_pctbAliasCmp = g_pctbAnchorCmp = g_pctbParamCmp = g_pctbDegenCmp = 0;
   g_pctbSubUlpCmp = g_pctbStreamCmp = g_pctbStreamHuge = 0;

   PCTB_LEG( test_pctb_goldens() );
   PCTB_LEG( test_pctb_composition() );
   PCTB_LEG( test_pctb_identity() );
   PCTB_LEG( test_pctb_lookback() );
   PCTB_LEG( test_pctb_alias() );
   PCTB_LEG( test_pctb_anchor() );
   PCTB_LEG( test_pctb_params() );
   PCTB_LEG( test_pctb_degenerate() );
   PCTB_LEG( test_pctb_stream() );

   /* Literal: every leg is deterministic, and a leg that compared nothing
    * prints nothing either. */
   for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
   {
      if( *cov[c].got != cov[c].want )
      {
         printf( "PERCENTB Fail: the %s leg compared %d times, not the %d this file was "
                 "written with\n", cov[c].leg, *cov[c].got, cov[c].want );
         bad = 1;
      }
   }

   return bad ? TA_TESTUTIL_TFRR_BAD_CALCULATION : TA_TEST_PASS;
}

static void pctb_build_series( const TA_History *history )
{
   const double *c = history->close;
   int i;

   for( i = 0; i < 252; i++ )
   {
      pctbNeg[i] = -c[i];
      pctbInt[i] = floor( c[i] );
   }

   ta_test_ref_lcg_seed( 0x447Au );
   for( i = 0; i < 300; i++ )
      pctbTick[i] = ( 10000.0 + ( ta_test_ref_lcg_sym() >= 0.0 ? 1.0 : 0.0 ) ) / 100.0;
   ta_test_ref_lcg_seed( 0x447Bu );
   for( i = 0; i < 300; i++ )
      pctbBig[i] = ( 1.0e10 + (double)(int)( ( ta_test_ref_lcg_sym() + 1.0 ) * 1.5 ) ) / 100.0;
   ta_test_ref_lcg_seed( 0x447Cu );
   for( i = 0; i < 300; i++ )
      pctbFine[i] = ( 1.0e10 + (double)(int)( ( ta_test_ref_lcg_sym() + 1.0 ) * 1.5 ) ) / 1.0e8;
   ta_test_ref_lcg_seed( 0x447Du );
   for( i = 0; i < 200; i++ )
      pctbEps[i] = ( ta_test_ref_lcg_sym() >= 0.0 ? 1.0 : -1.0 ) * DBL_EPSILON;
   ta_test_ref_lcg_seed( 0x447Eu );
   for( i = 0; i < PCTB_LONG_N; i++ )
      pctbLong[i] = 100.0 + 20.0 * ta_test_ref_lcg_sym();
   ta_test_ref_peg_ema( pctbPeg, pctbPegWalk );

   for( i = 0; i < 60; i++ )
   {
      pctbFlatPos[i] = 5.0;
      pctbFlatNeg[i] = -5.0;
      pctbZero[i]    = 0.0;
      pctbAlt[i]     = ( i % 2 == 0 ) ? 1.0 : -1.0;
   }
   for( i = 0; i < 80; i++ )
      pctbStep[i] = c[ i < 40 ? i : 39 ];
   for( i = 0; i < 100; i++ )
      pctbSubUlp[i] = 1.0e8;
   pctbSubUlp[30] = pctbSubUlp[60] = nextafter( 1.0e8, INFINITY );

   /* Not in the capture: 1% steps, long enough to cross the SMA path's tile
    * and its 32n rebuild at every period leg 2 uses. */
   ta_test_ref_lcg_seed( 0x4491u );
   pctbWalk[0] = 100.0;
   for( i = 1; i < PCTB_WALK_N; i++ )
      pctbWalk[i] = pctbWalk[i-1] * ( 1.0 + 0.01 * ta_test_ref_lcg_sym() );
   for( i = 0; i < PCTB_WALK_N; i++ )
   {
      pctbNegWalk[i] = -pctbWalk[i];
      pctbNanWalk[i] = pctbWalk[i];
      pctbInfWalk[i] = pctbWalk[i];
   }
   pctbNanWalk[1500] = NAN;
   pctbInfWalk[1500] = INFINITY;

#define PCTB_SERIES(id,nm,arr,len) pctbSeries[id].name = nm; pctbSeries[id].x = arr; pctbSeries[id].nbBars = len
   PCTB_SERIES( PCTB_CORPUS,    "corpus",    c,           252 );
   PCTB_SERIES( PCTB_NEGCORPUS, "negcorpus", pctbNeg,     252 );
   PCTB_SERIES( PCTB_TICK100,   "tick100",   pctbTick,    300 );
   PCTB_SERIES( PCTB_BIG1E8,    "big1e8",    pctbBig,     300 );
   PCTB_SERIES( PCTB_FINE100,   "fine100",   pctbFine,    300 );
   PCTB_SERIES( PCTB_PEG,       "peg",       pctbPeg,     TA_TEST_REF_PEG_N );
   PCTB_SERIES( PCTB_FLATPOS,   "flatpos",   pctbFlatPos, 60 );
   PCTB_SERIES( PCTB_FLATNEG,   "flatneg",   pctbFlatNeg, 60 );
   PCTB_SERIES( PCTB_STEPFLAT,  "stepflat",  pctbStep,    80 );
   PCTB_SERIES( PCTB_ZERO,      "zero",      pctbZero,    60 );
   PCTB_SERIES( PCTB_ALT,       "alt",       pctbAlt,     60 );
   PCTB_SERIES( PCTB_EPS,       "eps",       pctbEps,     200 );
   PCTB_SERIES( PCTB_LONG,      "long",      pctbLong,    PCTB_LONG_N );
   PCTB_SERIES( PCTB_SUBULP,    "subulp",    pctbSubUlp,  100 );
   PCTB_SERIES( PCTB_HD,        "hd",        pctbHd,      20 );
   PCTB_SERIES( PCTB_WALK,      "walk",      pctbWalk,    PCTB_WALK_N );
   PCTB_SERIES( PCTB_NEGWALK,   "negwalk",   pctbNegWalk, PCTB_WALK_N );
   PCTB_SERIES( PCTB_INTCORPUS, "intcorpus", pctbInt,     252 );
   PCTB_SERIES( PCTB_NANWALK,   "nanwalk",   pctbNanWalk, PCTB_WALK_N );
   PCTB_SERIES( PCTB_INFWALK,   "infwalk",   pctbInfWalk, PCTB_WALK_N );
#undef PCTB_SERIES
}

static int pctb_same( double a, double b )
{
   return memcmp( &a, &b, sizeof(double) ) == 0;
}

static ErrorNumber pctb_route( const char *tag, int startIdx, int endIdx,
                               const double *x, int nbBars,
                               int n, double kUp, double kDn, TA_MAType maType,
                               TA_RetCode retCode, int beg, int nb, const double *out )
{
   double optIn[4];
   int before;
   ErrorNumber err;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)n;
   optIn[1] = kUp;
   optIn[2] = kDn;
   optIn[3] = (double)maType;
   before = server_verify_value_comparisons();
   err = server_verify( "PERCENTB", startIdx, endIdx, nbBars, retCode, beg, nb,
                        (const TA_Real*[]){ x, NULL }, optIn, 4,
                        (const TA_Real*[]){ out, NULL }, NULL );
   if( err != TA_TEST_PASS )
      return err;
   if( server_verify_value_comparisons() == before )
   {
      printf( "PERCENTB %s: server_verify compared no server despite live pipes\n", tag );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* (1) */
static ErrorNumber test_pctb_goldens( void )
{
   static double out[PCTB_CAP], sd[PCTB_CAP], ma[PCTB_CAP];
   static double up[PCTB_CAP], mid[PCTB_CAP], lo[PCTB_CAP];
   TA_RetCode retCode, rc;
   TA_Integer beg, nb, begS, nbS, begM, nbM, begB, nbB;
   ErrorNumber err;
   int k, r, a, b;

   for( k = 0; k < NB_PCTB_GOLDENS; k++ )
   {
      const PctbGolden *g = &pctbGoldens[k];
      const PctbSeries *s = &pctbSeries[g->series];
      const double K = g->kUp + g->kDn;
      double eV = 1e-15, eM = 1e-15, absM = 0.0, dM[2];

      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, g->unstEma );
      beg = nb = -1;
      retCode = TA_PERCENTB( g->startIdx, g->endIdx, s->x, g->n, g->kUp, g->kDn, g->maType,
                             &beg, &nb, out );
      if( retCode != TA_SUCCESS || beg != g->beg || nb != g->nb )
      {
         printf( "PERCENTB golden Fail [%s]: rc=%d (%d,%d), expected (%d,%d)\n",
                 g->tag, (int)retCode, beg, nb, g->beg, g->nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      rc = TA_STDDEV( beg, g->endIdx, s->x, g->n, 1.0, &begS, &nbS, sd );
      if( rc != TA_SUCCESS || begS != beg || nbS != nb )
      {
         printf( "PERCENTB golden Fail [%s]: TA_STDDEV rc=%d (%d,%d)\n", g->tag, (int)rc,
                 begS, nbS );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_MA( g->startIdx, g->endIdx, s->x, g->n, g->maType, &begM, &nbM, ma );
      if( rc != TA_SUCCESS || begM > beg || begM + nbM != g->endIdx + 1 )
      {
         printf( "PERCENTB golden Fail [%s]: TA_MA rc=%d (%d,%d)\n", g->tag, (int)rc,
                 begM, nbM );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_BBANDS( g->startIdx, g->endIdx, s->x, g->n, g->kUp, g->kDn, g->maType,
                      &begB, &nbB, up, mid, lo );
      if( rc != TA_SUCCESS || begB != beg || nbB != nb )
      {
         printf( "PERCENTB golden Fail [%s]: TA_BBANDS rc=%d (%d,%d)\n", g->tag, (int)rc,
                 begB, nbB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( r = 0; r < g->nbRows; r++ )
      {
         const PctbRow *row = &g->rows[r];

         b = row->bar;
         if( b < beg || b - beg >= nb )
         {
            printf( "PERCENTB golden Fail [%s]: golden bar %d is outside the output [%d..%d]\n",
                    g->tag, b, beg, beg + nb - 1 );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         if( row->kind == 0 && fabs( sd[b-beg] - row->sigma ) / row->sigma > eV )
            eV = fabs( sd[b-beg] - row->sigma ) / row->sigma;
         dM[0] = fabs( ma[b-begM] - row->M );
         dM[1] = fabs( mid[b-beg] - row->M );
         for( a = 0; a < 2; a++ )
         {
            if( dM[a] > absM )
               absM = dM[a];
            if( row->M != 0.0 && dM[a] / fabs( row->M ) > eM )
               eM = dM[a] / fabs( row->M );
         }
      }
      if( !( eV <= 1e-6 ) || !( eM <= 1e-6 ) )
      {
         printf( "PERCENTB golden Fail [%s]: TA_STDDEV is %.3g and the middle band %.3g "
                 "relative from the frozen sigma and M, over 1e-6\n", g->tag, eV, eM );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( r = 0; r < g->nbRows; r++ )
      {
         const PctbRow *row = &g->rows[r];
         const double got = out[row->bar - beg];

         g_pctbGoldenCmp++;
         if( row->kind == 0 )
         {
            const double tol = eV * fabs( row->q - g->kDn / K )
                               + ( row->M != 0.0 ? eM * fabs( row->M ) : absM )
                                 / ( fabs( K ) * row->sigma )
                               + row->own;

            if( !( fabs( got - row->q ) <= tol ) )
            {
               printf( "PERCENTB golden Fail [%s] at bar %d: got %.17g, exact %.17g "
                       "(capture_449_percentb.py; |diff| %.3g over %.3g)\n", g->tag,
                       row->bar, got, row->q, fabs( got - row->q ), tol );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
         else if( !pctb_same( got, row->q ) )
         {
            printf( "PERCENTB golden Fail [%s] at bar %d: got %.17g, the zero-width value %.17g "
                    "(capture_449_percentb.py, kind %d, bitwise)\n", g->tag, row->bar, got,
                    row->q, row->kind );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( s->nbBars <= PCTB_SV_MAX_BARS )
      {
         err = pctb_route( g->tag, g->startIdx, g->endIdx, s->x, s->nbBars, g->n, g->kUp,
                           g->kDn, g->maType, retCode, beg, nb, out );
         if( err != TA_TEST_PASS )
            return err;
      }
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   }

   /* Bollinger's worked example as he printed it, 0.6830 to 4 decimals, at
    * every default: his 20 bars, 2 deviations and simple middle band. A
    * sample deviation gives 0.6784. */
   retCode = TA_PERCENTB( 0, 19, pctbHd, TA_INTEGER_DEFAULT, TA_REAL_DEFAULT, TA_REAL_DEFAULT,
                          (TA_MAType)TA_INTEGER_DEFAULT, &beg, &nb, out );
   g_pctbGoldenCmp++;
   if( retCode != TA_SUCCESS || beg != 19 || nb != 1 || !( fabs( out[0] - 0.6830 ) <= 5e-5 ) )
   {
      printf( "PERCENTB golden Fail [bollinger-hd, defaults]: rc=%d (%d,%d) %.17g, Bollinger "
              "printed 0.6830\n", (int)retCode, beg, nb, nb == 1 ? out[0] : NAN );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* TA_PERCENTB against (x - L) / (U - L) over TA_BBANDS' outputs, and 0.5
 * where U - L is 0, bitwise; NaN where the composition is NaN. */
static ErrorNumber pctb_vs_bbands( const char *tag, const PctbSeries *s,
                                   int startIdx, int endIdx, int n,
                                   double kUp, double kDn, TA_MAType maType )
{
   static double up[PCTB_CAP], mid[PCTB_CAP], lo[PCTB_CAP], got[PCTB_CAP];
   TA_RetCode rcB, rcP;
   TA_Integer begB, nbB, begP, nbP;
   double den, want;
   int i;

   rcB = TA_BBANDS( startIdx, endIdx, s->x, n, kUp, kDn, maType, &begB, &nbB, up, mid, lo );
   begP = nbP = -1;
   rcP = TA_PERCENTB( startIdx, endIdx, s->x, n, kUp, kDn, maType, &begP, &nbP, got );
   if( rcB != TA_SUCCESS || rcP != rcB || begP != begB || nbP != nbB )
   {
      printf( "PERCENTB %s Fail [%s n=%d k=(%g,%g) matype=%d start=%d end=%d]: PERCENTB rc=%d "
              "(%d,%d), BBANDS rc=%d (%d,%d)\n", tag, s->name, n, kUp, kDn, (int)maType,
              startIdx, endIdx, (int)rcP, begP, nbP, (int)rcB, begB, nbB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbB; i++ )
   {
      den  = up[i] - lo[i];
      want = ( den == 0.0 ) ? 0.5 : ( s->x[begB + i] - lo[i] ) / den;
      if( den == 0.0 )
         g_pctbDiffZero++;
      if( isnan( want ) )
         g_pctbNanCmp++;
      g_pctbDiffCmp++;
      if( !pctb_same( got[i], want ) && !( isnan( got[i] ) && isnan( want ) ) )
      {
         printf( "PERCENTB %s Fail [%s n=%d k=(%g,%g) matype=%d start=%d end=%d] at bar %d: "
                 "%.17g, TA_BBANDS gives %.17g (x %.17g, U %.17g, L %.17g)\n", tag, s->name, n,
                 kUp, kDn, (int)maType, startIdx, endIdx, begB + i, got[i], want,
                 s->x[begB + i], up[i], lo[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* The 3000-bar walk crosses the SMA path's tiles and a 32n rebuild. MAMA
 * sends Java down the element-by-element compare, which takes at most 512
 * outputs. */
static ErrorNumber pctb_route_walk( const char *tag, TA_MAType maType )
{
   static double out[PCTB_CAP];
   const PctbSeries *s = &pctbSeries[PCTB_WALK];
   const int endIdx = maType == TA_MAType_MAMA ? 540 : PCTB_WALK_N-1;
   TA_Integer beg, nb;
   TA_RetCode rc;
   ErrorNumber err;
   int m;

   if( !server_verify_active() )
      return TA_TEST_PASS;
   for( m = 0; m < 2; m++ )
   {
      const double kUp = pctbK[pctbKBranch[m]][0], kDn = pctbK[pctbKBranch[m]][1];

      rc = TA_PERCENTB( 0, endIdx, s->x, 20, kUp, kDn, maType, &beg, &nb, out );
      err = pctb_route( tag, 0, endIdx, s->x, PCTB_WALK_N, 20, kUp, kDn, maType, rc, beg, nb,
                        out );
      if( err != TA_TEST_PASS )
         return err;
   }
   return TA_TEST_PASS;
}

static int pctb_starts( int n, int lookback, int last, int *starts )
{
   const int cand[8] = { 0, 1, n-1, n, lookback, lookback+1, 300, last };
   int k, j, m = 0;

   for( k = 0; k < 8; k++ )
   {
      if( cand[k] < 0 || cand[k] > last )
         continue;
      for( j = 0; j < m && starts[j] != cand[k]; j++ )
         ;
      if( j == m )
         starts[m++] = cand[k];
   }
   return m;
}

/* (2) The decisive leg. Every tolerance-based leg passes the algebraic fold,
 * an unfused upper band and a multiply by 1/n for the middle band's divide. */
static ErrorNumber test_pctb_composition( void )
{
   static const int counts[] = { 1, 255, 256, 257, 511, 512, 513 };
   static float xf[252];
   const PctbSeries *s;
   ErrorNumber err;
   int d, mt, k, p, t, u, m, n, lookback, last, nbStarts, starts[8];

   for( d = 0; d < NB_PCTB_DIFF_SERIES; d++ )
   {
      s = &pctbSeries[pctbDiffSeries[d]];
      last = s->nbBars - 1;
      for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
      for( k = 0; k < NB_PCTB_K; k++ )
      for( p = 0; p < NB_PCTB_PERIODS; p++ )
      {
         n = pctbPeriods[p];
         lookback = TA_BBANDS_Lookback( n, pctbK[k][0], pctbK[k][1], (TA_MAType)mt );
         nbStarts = pctb_starts( n, lookback, last, starts );
         for( t = 0; t < nbStarts; t++ )
         {
            err = pctb_vs_bbands( "composition", s, starts[t], last, n,
                                  pctbK[k][0], pctbK[k][1], (TA_MAType)mt );
            if( err != TA_TEST_PASS )
               return err;
         }
      }
   }

   /* The SMA path's 256-bar tile, on each side of one and two boundaries. */
   s = &pctbSeries[PCTB_WALK];
   for( p = 0; p < 2; p++ )
   for( t = 0; t < 2; t++ )
   for( m = 0; m < (int)(sizeof(counts)/sizeof(counts[0])); m++ )
   {
      int startIdx = t == 0 ? 0 : 300;
      int first = startIdx < 19 ? 19 : startIdx;

      k = pctbKBranch[p];
      err = pctb_vs_bbands( "tile", s, startIdx, first + counts[m] - 1, 20,
                            pctbK[k][0], pctbK[k][1], TA_MAType_SMA );
      if( err != TA_TEST_PASS )
         return err;
   }

   /* startIdx 0 below a warmed-up lookback is where the variance must be
    * entered at the moving average's begIdx, not at startIdx. */
   for( u = 0; u < NB_PCTB_UNST; u++ )
   {
      static const unsigned int periods[] = { 3, 40 };
      static const int ns[] = { 5, 20, 34 };
      int up, dd, np;

      for( up = 0; up < 2; up++ )
      {
         TA_SetUnstablePeriod( pctbUnst[u].id, periods[up] );
         for( dd = 0; dd < 2; dd++ )
         for( np = 0; np < 3; np++ )
         for( m = 0; m < 2; m++ )
         {
            k = pctbKBranch[m];
            s = &pctbSeries[dd == 0 ? PCTB_CORPUS : PCTB_WALK];
            last = s->nbBars - 1;
            n = ns[np];
            lookback = TA_BBANDS_Lookback( n, pctbK[k][0], pctbK[k][1], pctbUnst[u].maType );
            nbStarts = pctb_starts( n, lookback, last, starts );
            for( t = 0; t < nbStarts; t++ )
            {
               err = pctb_vs_bbands( "unstable composition", s, starts[t], last, n,
                                     pctbK[k][0], pctbK[k][1], pctbUnst[u].maType );
               if( err != TA_TEST_PASS )
                  return err;
            }
         }

         if( up == 1 )
         {
            err = pctb_route_walk( "walk, unstable 40", pctbUnst[u].maType );
            if( err != TA_TEST_PASS )
               return err;
         }
         TA_SetUnstablePeriod( pctbUnst[u].id, 0 );
      }
   }

   /* The period at its maximum. */
   s = &pctbSeries[PCTB_LONG];
   for( t = 0; t < 3; t++ )
   {
      static const int longStarts[] = { 0, 99999, 100250 };

      err = pctb_vs_bbands( "n=100000", s, longStarts[t], PCTB_LONG_N-1, 100000, 2.0, 2.0,
                            TA_MAType_SMA );
      if( err != TA_TEST_PASS )
         return err;
      err = pctb_vs_bbands( "n=100000", s, longStarts[t], PCTB_LONG_N-1, 100000, 2.5, 1.5,
                            TA_MAType_EMA );
      if( err != TA_TEST_PASS )
         return err;
   }

   /* Outside the contract, batch only: a NaN or +Inf bar reaches the output
    * as the composition carries it. */
   for( d = 0; d < 2; d++ )
   for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
   for( m = 0; m < 2; m++ )
   for( t = 0; t < 2; t++ )
   {
      s = &pctbSeries[d == 0 ? PCTB_NANWALK : PCTB_INFWALK];
      k = pctbKBranch[m];
      err = pctb_vs_bbands( "non-finite", s, t == 0 ? 0 : 1600, PCTB_WALK_N-1, 20,
                            pctbK[k][0], pctbK[k][1], (TA_MAType)mt );
      if( err != TA_TEST_PASS )
         return err;
   }

   for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
   {
      err = pctb_route_walk( "walk", (TA_MAType)mt );
      if( err != TA_TEST_PASS )
         return err;
   }

   /* The single-precision tier against its own TA_S_BBANDS. */
   for( k = 0; k < 252; k++ )
      xf[k] = (float)pctbSeries[PCTB_CORPUS].x[k];
   for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
   for( m = 0; m < 2; m++ )
   for( p = 1; p <= 2; p++ )
   for( t = 0; t < 2; t++ )
   {
      static double up[PCTB_CAP], mid[PCTB_CAP], lo[PCTB_CAP], got[PCTB_CAP];
      TA_Integer begB, nbB, begP, nbP;
      TA_RetCode rcB, rcP;
      double den, want;
      int i, startIdx;

      k = pctbKBranch[m];
      n = pctbPeriods[p];
      lookback = TA_BBANDS_Lookback( n, pctbK[k][0], pctbK[k][1], (TA_MAType)mt );
      startIdx = t == 0 ? 0 : lookback + 1;
      rcB = TA_S_BBANDS( startIdx, 251, xf, n, pctbK[k][0], pctbK[k][1], (TA_MAType)mt,
                         &begB, &nbB, up, mid, lo );
      begP = nbP = -1;
      rcP = TA_S_PERCENTB( startIdx, 251, xf, n, pctbK[k][0], pctbK[k][1], (TA_MAType)mt,
                           &begP, &nbP, got );
      if( rcB != TA_SUCCESS || rcP != rcB || begP != begB || nbP != nbB )
      {
         printf( "PERCENTB float Fail [n=%d k=(%g,%g) matype=%d start=%d]: TA_S_PERCENTB rc=%d "
                 "(%d,%d), TA_S_BBANDS rc=%d (%d,%d)\n", n, pctbK[k][0], pctbK[k][1], mt,
                 startIdx, (int)rcP, begP, nbP, (int)rcB, begB, nbB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbB; i++ )
      {
         den  = up[i] - lo[i];
         want = ( den == 0.0 ) ? 0.5 : ( (double)xf[begB + i] - lo[i] ) / den;
         g_pctbFloatCmp++;
         if( !pctb_same( got[i], want ) )
         {
            printf( "PERCENTB float Fail [n=%d k=(%g,%g) matype=%d start=%d] at bar %d: %.17g, "
                    "TA_S_BBANDS gives %.17g\n", n, pctbK[k][0], pctbK[k][1], mt, startIdx,
                    begB + i, got[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3a) The author's form rewritten: %B = ((x - M) + kDn*sd) / ((kUp+kDn)*sd),
 * at every MA type, with M from TA_MA and sd from TA_STDDEV; at SMA and
 * kUp == kDn == k it reads 0.5 + z/(2k). TA_STDDEV enters at PERCENTB's
 * outBegIdx, where its deviation takes its rolling-sum anchor; TA_MA enters
 * at startIdx, as the middle band does (a recursive MA entered later is
 * another series). TA_BBANDS gives the bands the bound is scaled by, nothing
 * else: the literal quotient cancels about u*(|x| + (1+|q|)*(|U|+|L|)) / |U-L|
 * absolute, and the rewritten one rounds a few u relative. Worst measured:
 * 0.15 of the bound. */
#define PCTB_IDENT_ABS 4.0
#define PCTB_IDENT_REL 8.0
static ErrorNumber test_pctb_identity( void )
{
   static const double kPairs[][2] = { {2.0,2.0}, {1.0,3.0}, {2.0,1.0}, {2.5,1.5}, {0.1,0.2},
                                       {-1.0,2.0}, {-2.0,-2.0} };
   static const int ns[] = { 5, 20, 50 };
   static double got[PCTB_CAP], up[PCTB_CAP], mid[PCTB_CAP], lo[PCTB_CAP];
   static double sd[PCTB_CAP], ma[PCTB_CAP];
   TA_Integer beg, nb, begB, nbB, begS, nbS, begM, nbM;
   TA_RetCode rc;
   int d, mt, k, p, i, last, n;
   double kUp, kDn, x, fold, bound;

   for( d = 0; d < 2; d++ )
   {
      const PctbSeries *s = &pctbSeries[d == 0 ? PCTB_CORPUS : PCTB_WALK];

      last = s->nbBars - 1;
      for( mt = -1; mt < PCTB_NB_MATYPE; mt++ )
      for( k = 0; k < (int)(sizeof(kPairs)/sizeof(kPairs[0])); k++ )
      for( p = 0; p < 3; p++ )
      {
         TA_MAType maType = mt < 0 ? TA_MAType_SMA : (TA_MAType)mt;

         n   = ns[p];
         kUp = kPairs[k][0];
         kDn = kPairs[k][1];

         /* mt == -1: the defaults, all four resolved by the prologue. */
         if( mt < 0 && ( k != 0 || n != 20 ) )
            continue;
         if( mt < 0 )
            rc = TA_PERCENTB( 0, last, s->x, TA_INTEGER_DEFAULT, TA_REAL_DEFAULT,
                              TA_REAL_DEFAULT, (TA_MAType)TA_INTEGER_DEFAULT, &beg, &nb, got );
         else
            rc = TA_PERCENTB( 0, last, s->x, n, kUp, kDn, maType, &beg, &nb, got );
         if( rc != TA_SUCCESS )
         {
            printf( "PERCENTB identity Fail [%s n=%d matype=%d]: rc=%d\n", s->name, n, mt,
                    (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( nb == 0 )
            continue;

         rc = TA_BBANDS( 0, last, s->x, n, kUp, kDn, maType, &begB, &nbB, up, mid, lo );
         if( rc != TA_SUCCESS || begB != beg || nbB != nb )
         {
            printf( "PERCENTB identity Fail [%s n=%d matype=%d]: BBANDS rc=%d (%d,%d), "
                    "PERCENTB (%d,%d)\n", s->name, n, mt, (int)rc, begB, nbB, beg, nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         rc = TA_STDDEV( beg, last, s->x, n, 1.0, &begS, &nbS, sd );
         if( rc != TA_SUCCESS || begS != beg || nbS != nb )
         {
            printf( "PERCENTB identity Fail [%s n=%d matype=%d]: STDDEV rc=%d (%d,%d)\n",
                    s->name, n, mt, (int)rc, begS, nbS );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         rc = TA_MA( 0, last, s->x, n, maType, &begM, &nbM, ma );
         if( rc != TA_SUCCESS || begM > beg || begM + nbM != last + 1 )
         {
            printf( "PERCENTB identity Fail [%s n=%d matype=%d]: MA rc=%d (%d,%d)\n",
                    s->name, n, mt, (int)rc, begM, nbM );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nb; i++ )
         {
            if( up[i] == lo[i] )
               continue;
            x     = s->x[beg + i];
            fold  = ( ( x - ma[beg + i - begM] ) + kDn * sd[i] ) / ( ( kUp + kDn ) * sd[i] );
            bound = PCTB_ULP * ( PCTB_IDENT_ABS * ( fabs( x ) + ( 1.0 + fabs( fold ) )
                                                    * ( fabs( up[i] ) + fabs( lo[i] ) ) )
                                 / fabs( up[i] - lo[i] )
                                 + PCTB_IDENT_REL * fabs( fold ) );
            g_pctbIdentCmp++;
            if( !( fabs( got[i] - fold ) <= bound ) )
            {
               printf( "PERCENTB identity Fail [%s n=%d k=(%g,%g) matype=%d] at bar %d: %.17g, "
                       "((x - TA_MA) + kDn*TA_STDDEV) / ((kUp+kDn)*TA_STDDEV) gives %.17g "
                       "(|diff| %.3g over %.3g)\n", s->name, n, kUp, kDn, mt, beg + i, got[i],
                       fold, fabs( got[i] - fold ), bound );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   /* (3b) "At 100 we are at the upper band, at 0 we are at the lower band",
    * exactly: over two integer closes a and b the bands are exact, and at
    * k = (1,1) they are max(a,b) and min(a,b). So %B is 1 on an up bar, 0 on a
    * down bar and 0.5 on an unchanged one; at k = (1,3) a down bar sits halfway
    * between the bands. */
   {
      const PctbSeries *s = &pctbSeries[PCTB_INTCORPUS];
      int j;

      for( j = 0; j < 2; j++ )
      {
         const double kDn = j == 0 ? 1.0 : 3.0;
         double want;

         rc = TA_PERCENTB( 0, 251, s->x, 2, 1.0, kDn, TA_MAType_SMA, &beg, &nb, got );
         if( rc != TA_SUCCESS || beg != 1 || nb != 251 )
         {
            printf( "PERCENTB band crossing Fail [k=(1,%g)]: rc=%d (%d,%d)\n", kDn, (int)rc,
                    beg, nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nb; i++ )
         {
            const double a = s->x[i], b = s->x[i + 1];

            want = b > a ? 1.0 : ( b == a || j == 1 ) ? 0.5 : 0.0;
            g_pctbCrossCmp++;
            if( !pctb_same( got[i], want ) )
            {
               printf( "PERCENTB band crossing Fail [k=(1,%g)] at bar %d: %.17g, the bars "
                       "%g then %g put it exactly at %g\n", kDn, i + 1, got[i], a, b, want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) No value comparison sees a lookback that disagrees with the batch call
 * only in the count a caller sizes its buffer by. */
static ErrorNumber test_pctb_lookback( void )
{
   static const int ns[] = { 2, 20, 33, 34, 100, 100000 };
   static const unsigned int us[] = { 0, 1, 7, 40 };
   static const TA_FuncUnstId ids[] = { TA_FUNC_UNST_EMA, TA_FUNC_UNST_KAMA, TA_FUNC_UNST_MAMA,
                                        TA_FUNC_UNST_T3, TA_FUNC_UNST_RMA };
   static const struct { int n; TA_MAType maType; unsigned int unstEma; int want; } known[] = {
      {      2, TA_MAType_SMA,       0,     1 }, {     20, TA_MAType_SMA,       0,    19 },
      { 100000, TA_MAType_SMA,       0, 99999 }, {     20, TA_MAType_EMA,      15,    34 },
      {     20, TA_MAType_EMA,     100,   119 }, {     20, TA_MAType_DEMA,      0,    38 },
      {     33, TA_MAType_MAMA,      0,    32 }, {     34, TA_MAType_MAMA,      0,    33 },
      {     20, TA_MAType_DISABLED,  0,    19 }, {     20, TA_MAType_DEFAULT,   0,    19 },
   };
   static double out[PCTB_CAP];
   const PctbSeries *s = &pctbSeries[PCTB_WALK];
   TA_Integer beg, nb;
   TA_RetCode rc;
   int d, j, mt, p, lb, lbB;
   unsigned int k;

   for( k = 0; k < sizeof(known)/sizeof(known[0]); k++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, known[k].unstEma );
      lb = TA_PERCENTB_Lookback( known[k].n, 2.0, 2.0, known[k].maType );
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
      g_pctbLookbackCmp++;
      if( lb != known[k].want )
      {
         printf( "PERCENTB lookback Fail [n=%d matype=%d unstable %u]: %d, expected %d\n",
                 known[k].n, (int)known[k].maType, known[k].unstEma, lb, known[k].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( d = 0; d < (int)(sizeof(ids)/sizeof(ids[0])); d++ )
   for( j = 0; j < (int)(sizeof(us)/sizeof(us[0])); j++ )
   {
      TA_SetUnstablePeriod( ids[d], us[j] );
      for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
      {
         for( p = 0; p < (int)(sizeof(ns)/sizeof(ns[0])); p++ )
         {
            lb  = TA_PERCENTB_Lookback( ns[p], 2.5, 1.5, (TA_MAType)mt );
            lbB = TA_BBANDS_Lookback( ns[p], 2.5, 1.5, (TA_MAType)mt );
            g_pctbLookbackCmp++;
            if( lb != lbB || lb < 0 )
            {
               printf( "PERCENTB lookback Fail [n=%d matype=%d unstable id %d = %u]: %d, "
                       "TA_BBANDS_Lookback %d\n", ns[p], mt, (int)ids[d], us[j], lb, lbB );
               TA_SetUnstablePeriod( ids[d], 0 );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* The first output lands on the lookback bar, and a range one bar
          * shorter is empty. */
         lb = TA_PERCENTB_Lookback( 20, 2.5, 1.5, (TA_MAType)mt );
         rc = TA_PERCENTB( 0, PCTB_WALK_N-1, s->x, 20, 2.5, 1.5, (TA_MAType)mt, &beg, &nb, out );
         g_pctbLookbackCmp++;
         if( rc != TA_SUCCESS || beg != lb || nb != PCTB_WALK_N - lb )
         {
            printf( "PERCENTB lookback Fail [n=20 matype=%d unstable id %d = %u]: rc=%d (%d,%d), "
                    "lookback %d\n", mt, (int)ids[d], us[j], (int)rc, beg, nb, lb );
            TA_SetUnstablePeriod( ids[d], 0 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         beg = nb = -1;
         rc = TA_PERCENTB( 0, lb - 1, s->x, 20, 2.5, 1.5, (TA_MAType)mt, &beg, &nb, out );
         g_pctbLookbackCmp++;
         if( rc != TA_SUCCESS || beg != 0 || nb != 0 )
         {
            printf( "PERCENTB lookback Fail [n=20 matype=%d unstable id %d = %u]: [0,%d] gave "
                    "rc=%d (%d,%d), expected TA_SUCCESS (0,0)\n", mt, (int)ids[d], us[j],
                    lb - 1, (int)rc, beg, nb );
            TA_SetUnstablePeriod( ids[d], 0 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         rc = TA_PERCENTB( 0, lb, s->x, 20, 2.5, 1.5, (TA_MAType)mt, &beg, &nb, out );
         g_pctbLookbackCmp++;
         if( rc != TA_SUCCESS || beg != lb || nb != 1 )
         {
            printf( "PERCENTB lookback Fail [n=20 matype=%d unstable id %d = %u]: [0,%d] gave "
                    "rc=%d (%d,%d), expected (%d,1)\n", mt, (int)ids[d], us[j], lb,
                    (int)rc, beg, nb, lb );
            TA_SetUnstablePeriod( ids[d], 0 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
      }
      TA_SetUnstablePeriod( ids[d], 0 );
   }

   return TA_TEST_PASS;
}

/* (5) outReal == inReal. At startIdx n-1 the first output slot is the first
 * bar a rebuild re-reads, and every tile's variances land on the x slots of
 * the same tile; 3000 bars take every period here past a 32n rebuild and a
 * tile boundary inside the aliased run, and the peg series collapses the
 * variance and forces rebuilds that the walk does not. */
static ErrorNumber test_pctb_alias( void )
{
   static const int ns[] = { 5, 20, 50 };
   static double clean[PCTB_CAP], alias[PCTB_CAP];
   TA_Integer beg, nb, beg2, nb2;
   TA_RetCode rc;
   int d, mt, p, k, t, i, n, startIdx;

   for( d = 0; d < 2; d++ )
   {
      const PctbSeries *s = &pctbSeries[d == 0 ? PCTB_WALK : PCTB_PEG];
      int nbBars = s->nbBars;

      for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
      for( p = 0; p < 3; p++ )
      for( k = 0; k < 2; k++ )
      for( t = 0; t < 4; t++ )
      {
         double kUp = pctbK[pctbKBranch[k]][0], kDn = pctbK[pctbKBranch[k]][1];

         if( d == 1 && mt != TA_MAType_SMA )
            continue;
         n = ns[p];
         startIdx = t == 0 ? 0 : t == 1 ? n-1 : t == 2 ? n : 300;

         rc = TA_PERCENTB( startIdx, nbBars-1, s->x, n, kUp, kDn, (TA_MAType)mt, &beg, &nb,
                           clean );
         if( rc != TA_SUCCESS )
         {
            printf( "PERCENTB in-place Fail [%s n=%d matype=%d start=%d]: rc=%d\n", s->name, n,
                    mt, startIdx, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         memcpy( alias, s->x, (size_t)nbBars * sizeof(double) );
         rc = TA_PERCENTB( startIdx, nbBars-1, alias, n, kUp, kDn, (TA_MAType)mt, &beg2, &nb2,
                           alias );
         if( rc != TA_SUCCESS || beg2 != beg || nb2 != nb )
         {
            printf( "PERCENTB in-place Fail [%s n=%d matype=%d start=%d]: rc=%d (%d,%d) vs "
                    "(%d,%d)\n", s->name, n, mt, startIdx, (int)rc, beg2, nb2, beg, nb );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nb; i++ )
         {
            g_pctbAliasCmp++;
            if( !pctb_same( clean[i], alias[i] ) )
            {
               printf( "PERCENTB in-place Fail [%s n=%d k=(%g,%g) matype=%d start=%d] at bar "
                       "%d: separate %.17g, in-place %.17g\n", s->name, n, kUp, kDn, mt,
                       startIdx, beg + i, clean[i], alias[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) A call reads nothing before startIdx - lookback and keys every rolling
 * sum on its own start, so from startIdx s it is, bit for bit, the call from
 * the lookback bar on the series cut at s - lookback. */
static ErrorNumber test_pctb_anchor( void )
{
   static const int ns[] = { 5, 20 };
   static double got[PCTB_CAP], cut[PCTB_CAP];
   TA_Integer beg, nb, cutBeg, cutNb;
   TA_RetCode rc;
   ErrorNumber err;
   int d, mt, p, m, pass, i, n, lb, s0, e, a, last;
   double kUp, kDn;

   for( d = 0; d < 2; d++ )
   {
      const PctbSeries *s = &pctbSeries[d == 0 ? PCTB_CORPUS : PCTB_WALK];

      last = d == 0 ? s->nbBars - 1 : 699;
      for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
      for( m = 0; m < 2; m++ )
      for( p = 0; p < 2; p++ )
      {
         n   = ns[p];
         kUp = pctbK[pctbKBranch[m]][0];
         kDn = pctbK[pctbKBranch[m]][1];
         lb  = TA_BBANDS_Lookback( n, kUp, kDn, (TA_MAType)mt );
         for( s0 = lb; s0 <= last; s0 = ( s0 == last ) ? last + 1 : ( s0 + 23 < last ? s0 + 23 : last ) )
         for( pass = 0; pass < 2; pass++ )
         {
            e = pass == 0 ? s0 : last;
            if( pass == 1 && e == s0 )
               continue;
            a = s0 - lb;

            rc = TA_PERCENTB( s0, e, s->x, n, kUp, kDn, (TA_MAType)mt, &beg, &nb, got );
            if( rc != TA_SUCCESS || beg != s0 || nb != e - s0 + 1 )
            {
               printf( "PERCENTB anchor Fail [%s n=%d matype=%d] [%d,%d]: rc=%d (%d,%d)\n",
                       s->name, n, mt, s0, e, (int)rc, beg, nb );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            rc = TA_PERCENTB( lb, e - a, s->x + a, n, kUp, kDn, (TA_MAType)mt, &cutBeg, &cutNb,
                              cut );
            if( rc != TA_SUCCESS || cutBeg != lb || cutNb != nb )
            {
               printf( "PERCENTB anchor Fail [%s n=%d matype=%d] [%d,%d]: the series cut at %d "
                       "gave rc=%d (%d,%d)\n", s->name, n, mt, s0, e, a, (int)rc, cutBeg, cutNb );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            for( i = 0; i < nb; i++ )
            {
               g_pctbAnchorCmp++;
               if( !pctb_same( got[i], cut[i] ) )
               {
                  printf( "PERCENTB anchor Fail [%s n=%d k=(%g,%g) matype=%d] [%d,%d] at bar %d: "
                          "%.17g, the series cut at %d gives %.17g\n", s->name, n, kUp, kDn, mt,
                          s0, e, s0 + i, got[i], a, cut[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }

         if( d == 0 && p == 1 )
         {
            s0 = lb + 23;
            rc = TA_PERCENTB( s0, last, s->x, n, kUp, kDn, (TA_MAType)mt, &beg, &nb, got );
            err = pctb_route( "anchored", s0, last, s->x, s->nbBars, n, kUp, kDn,
                              (TA_MAType)mt, rc, beg, nb, got );
            if( err != TA_TEST_PASS )
               return err;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) The declared ranges are ABI, and each default resolves to the value
 * the YAML declares: 20, 2, 2, SMA. */
static ErrorNumber test_pctb_params( void )
{
   static double out[PCTB_CAP], want[PCTB_CAP];
   const PctbSeries *s = &pctbSeries[PCTB_CORPUS];
   const double kOver  = nextafter( TA_REAL_MAX, INFINITY );
   const double kUnder = nextafter( TA_REAL_MIN, -INFINITY );
   const struct { int n; double kUp, kDn; int maType; int ok; } v[] = {
      {     -1,      2.0,      2.0,   0, 0 }, {      0,      2.0,      2.0,   0, 0 },
      {      1,      2.0,      2.0,   0, 0 }, { 100001,      2.0,      2.0,   0, 0 },
      {     20,    kOver,      2.0,   0, 0 }, {     20,      2.0,    kOver,   0, 0 },
      {     20,   kUnder,      2.0,   0, 0 }, {     20,      2.0,   kUnder,   0, 0 },
      {     20, INFINITY,      2.0,   0, 0 }, {     20,      2.0, INFINITY,   0, 0 },
      {     20,-INFINITY,      2.0,   0, 0 }, {     20,      2.0,-INFINITY,   0, 0 },
      {     20,      NAN,      2.0,   0, 0 }, {     20,      2.0,      NAN,   0, 0 },
      {     20,      2.0,      2.0,  -1, 0 }, {     20,      2.0,      2.0,  14, 0 },
      {     20,      2.0,      2.0, 100, 0 },
      {      2,      2.0,      2.0,   0, 1 }, { 100000,      2.0,      2.0,   0, 1 },
      {     20, TA_REAL_MAX, TA_REAL_MIN, 1, 1 }, {     20, TA_REAL_MIN, TA_REAL_MAX, 1, 1 },
      {     20,      2.0,      2.0,  13, 1 },
   };
   const int nbV = (int)(sizeof(v)/sizeof(v[0]));
   TA_Integer beg, nb, begW, nbW;
   TA_RetCode rc;
   int i, j, lb;

   for( i = 0; i < nbV; i++ )
   {
      lb = TA_PERCENTB_Lookback( v[i].n, v[i].kUp, v[i].kDn, (TA_MAType)v[i].maType );
      rc = TA_PERCENTB( 0, 251, s->x, v[i].n, v[i].kUp, v[i].kDn, (TA_MAType)v[i].maType,
                        &beg, &nb, out );
      g_pctbParamCmp++;
      if( v[i].ok ? ( rc != TA_SUCCESS || lb < 0 ) : ( rc != TA_BAD_PARAM || lb != -1 ) )
      {
         printf( "PERCENTB parameter Fail [n=%d k=(%.17g,%.17g) matype=%d]: rc=%d, lookback %d, "
                 "expected %s\n", v[i].n, v[i].kUp, v[i].kDn, v[i].maType, (int)rc, lb,
                 v[i].ok ? "TA_SUCCESS" : "TA_BAD_PARAM and -1" );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      if( server_verify_active() && isfinite( v[i].kUp ) && isfinite( v[i].kDn ) )
      {
         const double optIn[4] = { (double)v[i].n, v[i].kUp, v[i].kDn, (double)v[i].maType };
         ErrorNumber err = server_verify_lookback_parity( "PERCENTB", 0, 251, 252,
                                                          (const TA_Real*[]){ s->x, NULL },
                                                          optIn, 4 );
         if( err != TA_TEST_PASS )
            return err;
      }
   }

   rc = TA_PERCENTB( -1, 251, s->x, 20, 2.0, 2.0, TA_MAType_SMA, &beg, &nb, out );
   g_pctbParamCmp++;
   if( rc != TA_OUT_OF_RANGE_START_INDEX )
   {
      printf( "PERCENTB parameter Fail: startIdx -1 gave rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   rc = TA_PERCENTB( 30, 29, s->x, 20, 2.0, 2.0, TA_MAType_SMA, &beg, &nb, out );
   g_pctbParamCmp++;
   if( rc != TA_OUT_OF_RANGE_END_INDEX )
   {
      printf( "PERCENTB parameter Fail: endIdx < startIdx gave rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* Each default alone, against the explicit value, bitwise. */
   for( j = 0; j < 5; j++ )
   {
      int    n   = j == 0 ? TA_INTEGER_DEFAULT : 20;
      double kUp = j == 1 ? TA_REAL_DEFAULT : 2.5;
      double kDn = j == 2 ? TA_REAL_DEFAULT : 1.5;
      int    mat = j == 3 ? TA_INTEGER_DEFAULT : j == 4 ? TA_MAType_DEFAULT : TA_MAType_SMA;
      double eUp = j == 1 ? 2.0 : 2.5;
      double eDn = j == 2 ? 2.0 : 1.5;

      rc = TA_PERCENTB( 0, 251, s->x, 20, eUp, eDn, TA_MAType_SMA, &begW, &nbW, want );
      if( rc != TA_SUCCESS )
      {
         printf( "PERCENTB default Fail [%d]: the explicit call gave rc=%d\n", j, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      rc = TA_PERCENTB( 0, 251, s->x, n, kUp, kDn, (TA_MAType)mat, &beg, &nb, out );
      lb = TA_PERCENTB_Lookback( n, kUp, kDn, (TA_MAType)mat );
      g_pctbParamCmp++;
      if( rc != TA_SUCCESS || beg != begW || nb != nbW || lb != 19 )
      {
         printf( "PERCENTB default Fail [%d]: rc=%d (%d,%d) lookback %d, the explicit call gives "
                 "(%d,%d) and 19\n", j, (int)rc, beg, nb, lb, begW, nbW );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
      {
         g_pctbParamCmp++;
         if( !pctb_same( out[i], want[i] ) )
         {
            printf( "PERCENTB default Fail [%d] at bar %d: %.17g, the explicit call gives %.17g\n",
                    j, beg + i, out[i], want[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) A flat window has sigma exactly 0, so both bands are the middle band,
 * fused or not: 0.5 at every MA type and deviation pair. An even window of
 * alternating +-1 has M = 0 and sigma = 1, so at k = 2 the bands are +-2 and
 * %B is exactly 0.75 or 0.25. The sub-ulp series has sigma > 0 on the windows
 * that hold its odd bar, but 2*sigma is under half an ulp of 1e8, so the
 * rounded bands still coincide: 0.5, which a guard on sigma instead of on the
 * width misses. */
static ErrorNumber test_pctb_degenerate( void )
{
   static const int ids[3] = { PCTB_FLATPOS, PCTB_FLATNEG, PCTB_ZERO };
   static const int ns[] = { 2, 4, 20 };
   static double out[PCTB_CAP], sd[PCTB_CAP], up[PCTB_CAP], mid[PCTB_CAP], lo[PCTB_CAP];
   TA_Integer beg, nb, begS, nbS, begB, nbB;
   TA_RetCode rc;
   int c, mt, p, k, i, lb, wantNb;
   double want;

   for( c = 0; c < 4; c++ )
   for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
   for( p = 0; p < 3; p++ )
   for( k = 0; k < NB_PCTB_K; k++ )
   {
      const PctbSeries *s = &pctbSeries[c < 3 ? ids[c] : PCTB_ALT];
      const double kUp = c < 3 ? pctbK[k][0] : 2.0;
      const double kDn = c < 3 ? pctbK[k][1] : 2.0;

      if( c == 3 && ( mt != TA_MAType_SMA || k != 0 ) )
         continue;
      lb = TA_BBANDS_Lookback( ns[p], kUp, kDn, (TA_MAType)mt );
      wantNb = lb < s->nbBars ? s->nbBars - lb : 0;
      rc = TA_PERCENTB( 0, s->nbBars-1, s->x, ns[p], kUp, kDn, (TA_MAType)mt, &beg, &nb, out );
      if( rc != TA_SUCCESS || nb != wantNb || ( nb > 0 && beg != lb ) )
      {
         printf( "PERCENTB degenerate Fail [%s n=%d matype=%d]: rc=%d (%d,%d), lookback %d\n",
                 s->name, ns[p], mt, (int)rc, beg, nb, lb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
      {
         want = c < 3 ? 0.5 : s->x[beg + i] > 0.0 ? 0.75 : 0.25;
         g_pctbDegenCmp++;
         if( !pctb_same( out[i], want ) )
         {
            printf( "PERCENTB degenerate Fail [%s n=%d k=(%g,%g) matype=%d] at bar %d: %.17g, "
                    "expected exactly %g\n", s->name, ns[p], kUp, kDn, mt, beg + i, out[i],
                    want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   for( mt = 0; mt < PCTB_NB_MATYPE; mt++ )
   {
      const PctbSeries *s = &pctbSeries[PCTB_SUBULP];

      rc = TA_PERCENTB( 0, 99, s->x, 20, 2.0, 2.0, (TA_MAType)mt, &beg, &nb, out );
      if( rc != TA_SUCCESS )
      {
         printf( "PERCENTB sub-ulp Fail [matype=%d]: rc=%d\n", mt, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( nb == 0 )
         continue;
      rc = TA_STDDEV( beg, 99, s->x, 20, 1.0, &begS, &nbS, sd );
      if( rc != TA_SUCCESS || begS != beg || nbS != nb )
      {
         printf( "PERCENTB sub-ulp Fail [matype=%d]: STDDEV rc=%d (%d,%d)\n", mt, (int)rc,
                 begS, nbS );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_BBANDS( 0, 99, s->x, 20, 2.0, 2.0, (TA_MAType)mt, &begB, &nbB, up, mid, lo );
      if( rc != TA_SUCCESS || begB != beg || nbB != nb )
      {
         printf( "PERCENTB sub-ulp Fail [matype=%d]: BBANDS rc=%d (%d,%d)\n", mt, (int)rc,
                 begB, nbB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
      {
         if( sd[i] > 0.0 && up[i] == lo[i] )
            g_pctbSubUlpCmp++;
         g_pctbDegenCmp++;
         if( !pctb_same( out[i], 0.5 ) )
         {
            printf( "PERCENTB sub-ulp Fail [matype=%d] at bar %d: %.17g, expected exactly 0.5 "
                    "(sigma %.3g, U %.17g, L %.17g)\n", mt, beg + i, out[i], sd[i], up[i],
                    lo[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (9) Open, Peek then Update on every bar, and OpenAndFill, bitwise equal to
 * the batch. At kUp = -kDn the bands cancel in exact arithmetic, and where
 * the fused upper band and the unfused lower band round an ulp apart the
 * batch gives %B near 1e15: the stream must round each band the same way. */
static ErrorNumber test_pctb_stream( void )
{
   static const double kPairs[][2] = { {-1.5,1.5}, {-0.2,0.2}, {2.5,1.5}, {2.0,2.0} };
   static const TA_MAType types[] = { TA_MAType_SMA, TA_MAType_EMA };
   static double batch[PCTB_CAP], fill[PCTB_CAP];
   const PctbSeries *s = &pctbSeries[PCTB_WALK];
   const int last = PCTB_WALK_N - 1;
   TA_PERCENTB_Stream *st;
   TA_Integer beg, nb, begF, nbF;
   TA_RetCode rc;
   double v, pv;
   int t, k, b, i;

   for( t = 0; t < 2; t++ )
   for( k = 0; k < (int)(sizeof(kPairs)/sizeof(kPairs[0])); k++ )
   {
      const double kUp = kPairs[k][0], kDn = kPairs[k][1];

      rc = TA_PERCENTB( 0, last, s->x, 20, kUp, kDn, types[t], &beg, &nb, batch );
      if( rc != TA_SUCCESS || nb != last - beg + 1 )
      {
         printf( "PERCENTB stream Fail [k=(%g,%g) matype=%d]: batch rc=%d (%d,%d)\n", kUp, kDn,
                 (int)types[t], (int)rc, beg, nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
         if( fabs( batch[i] ) > 1.0e6 )
            g_pctbStreamHuge++;

      st = NULL;
      rc = TA_PERCENTB_Open( &st, s->x, beg + 1, 20, kUp, kDn, types[t], &v );
      g_pctbStreamCmp++;
      if( rc != TA_SUCCESS || !pctb_same( v, batch[0] ) )
      {
         printf( "PERCENTB stream Fail [k=(%g,%g) matype=%d]: Open rc=%d %.17g, batch %.17g\n",
                 kUp, kDn, (int)types[t], (int)rc, v, batch[0] );
         if( st ) TA_PERCENTB_Close( st );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( b = beg + 1; b <= last; b++ )
      {
         rc = TA_PERCENTB_Peek( st, s->x[b], &pv );
         if( rc == TA_SUCCESS )
            rc = TA_PERCENTB_Update( st, s->x[b], &v );
         g_pctbStreamCmp++;
         if( rc != TA_SUCCESS || !pctb_same( pv, batch[b - beg] ) || !pctb_same( v, batch[b - beg] ) )
         {
            printf( "PERCENTB stream Fail [k=(%g,%g) matype=%d] at bar %d: rc=%d Peek %.17g "
                    "Update %.17g, batch %.17g\n", kUp, kDn, (int)types[t], b, (int)rc, pv, v,
                    batch[b - beg] );
            TA_PERCENTB_Close( st );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      TA_PERCENTB_Close( st );

      st = NULL;
      rc = TA_PERCENTB_OpenAndFill( &st, s->x, last + 1, 20, kUp, kDn, types[t], &begF, &nbF,
                                    fill );
      if( st ) TA_PERCENTB_Close( st );
      if( rc != TA_SUCCESS || begF != beg || nbF != nb )
      {
         printf( "PERCENTB stream Fail [k=(%g,%g) matype=%d]: OpenAndFill rc=%d (%d,%d), batch "
                 "(%d,%d)\n", kUp, kDn, (int)types[t], (int)rc, begF, nbF, beg, nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
      {
         g_pctbStreamCmp++;
         if( !pctb_same( fill[i], batch[i] ) )
         {
            printf( "PERCENTB stream Fail [k=(%g,%g) matype=%d] at bar %d: OpenAndFill %.17g, "
                    "batch %.17g\n", kUp, kDn, (int)types[t], beg + i, fill[i], batch[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}
