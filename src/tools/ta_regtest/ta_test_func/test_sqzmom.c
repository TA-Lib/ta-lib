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
 *  100826 KL,CC  Creation (#488).
 *  101126 MF,CC  Factor order, sub-ranges, short series, range test (#488).
 *  101126 MF,CC  pandas and ta4j goldens (#488).
 */

/* Description:
 *
 *   Test TA_SQZMOM.
 *
 *   Every leg of SQZMOM is a shipped function, so this file adds no numerical
 *   logic: it composes the card's formula out of the SAME public functions and
 *   requires the two to agree BIT FOR BIT. The reference deliberately calls
 *   TA_BBANDS, the function the card's formula names, while the implementation
 *   takes the MA + STDDEV pair instead -- so leg 1 tests that substitution
 *   rather than assuming it.
 *
 *   Legs:
 *     1. COMPOSITION. The formula through TA_BBANDS, TA_SMA, TA_TRANGE,
 *        TA_MIDPRICE and TA_LINEARREG, over five parameter triples. Bit for
 *        bit on both outputs. Also the leg that drives server_verify.
 *     2. THE ORDINAL AT ITS BOUNDARY. Leg 1's factors put every bar far from a
 *        Keltner edge, so the ordinal comparison there has almost no
 *        discrimination: scaling the band by 1+1e-7 moves not one bar. This
 *        leg sets the normal factor to a value read off the data itself --
 *        (BBU - KCM)/BAND at a chosen bar -- so that bar sits ON the edge, and
 *        asserts both that some bar really is within a few ulps of it (or the
 *        leg proved nothing) and that the two still agree. Tuned this way the
 *        comparison detects a relative band error of 6.2e-15.
 *     3. LOOKBACK. TA_SQZMOM_Lookback against the max of the four chains, and
 *        the call's own begIdx against it, at the triples the card measured
 *        against pandas, ta4j and trading-signals: 38 at (20,20), 26 at
 *        (20,14), 38 at (14,20).
 *     4. NESTING. The three channels share a centre and BAND >= 0, so they
 *        nest. Every bar reporting 3 must also satisfy the normal and wide
 *        tests, every 2 the wide test, and -1 must fail the wide test on both
 *        sides. A level the nesting forbids is a mis-ordered ladder.
 *     5. FLAT. A constant series has BAND == 0 and BBU == BBL == KCM, so no
 *        test is strictly satisfied on either side and every bar reads 0.
 *     6. ALIASING. Each output over each input in turn. The abstract layer
 *        caught this one: the body must read every bar input before it writes
 *        a caller buffer, which is why TA_LINEARREG runs last.
 *     7. EDGES. A period below its range and factors out of order are
 *        rejected; nbdev == 0 collapses the bands onto the middle band, which
 *        is inside every channel of positive width, so every bar reads 3; a
 *        series one bar short gives no output and the next bar gives one.
 *     8. SUB-RANGE. The composition again, entered above the lookback and
 *        ended before the last bar. Four legs keep running sums, so the
 *        reference starts each of them where the implementation does.
 *     9. RANGE INDEPENDENCE, swept via doRangeTestEx.
 *    10. ORACLES. Legs 1 to 9 hold the implementation to a composition of
 *        TA-Lib's own functions, which a wrong formula passes. This leg holds
 *        it to two other libraries over the same 252 bars: pandas-ta-classic
 *        0.6.52 (squeeze, mamode sma, lazybear, the level folded from one call
 *        per factor) at 20/20, 14/20 and 20/14, and ta4j 0.22.6
 *        (SqueezeProIndicator) at 20/20. Both descend from LazyBear's script,
 *        so they check the transcription of that script, not each other.
 *        Captured by ta-lib-oracles capture_488_sqzmom.py at b6dd468.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define SQZ_NB_BAR 252

/* A leg that compares nothing passes silently without these counts.
 * 11 cases; the bars each produces are 214, 226, 214, 250, 203, 233, 233, 212,
 * 54, 214 and 214, which is 2267, and each bar compares both outputs. */
#define SQZ_COMPOSE_CMP   4534
#define SQZ_BOUNDARY_CMP   642   /* 3 tuned runs x 214 bars */
/* One bar per tuned run lands on the edge -- the bar the factor was read from.
 * The floor is what stops the leg passing when the tuning lands nowhere near
 * one, which would make it leg 1 again. */
#define SQZ_BOUNDARY_EDGE    3
#define SQZ_LOOKBACK_CMP    23   /* 11 cases x (lookback, begIdx), and the unstable-period check */
#define SQZ_NEST_CMP      2267   /* the same 2267 bars, one check each */
#define SQZ_FLAT_CMP       214
#define SQZ_ALIAS_CMP        6   /* 2 outputs x 3 inputs */
#define SQZ_EDGE_CMP       229   /* 3 return codes, the 214 bars of the nbDev=0 run, 8 on factor order, 4 on short series */
#define SQZ_RANGE_CMP      590   /* 152 and 143 bars, both outputs */
#define SQZ_ORACLE_CMP    1736   /* 214, 214, 226 and 214 bars, both outputs */

static int g_sqzComposeCmp;
static int g_sqzBoundaryCmp;
static int g_sqzBoundaryEdge;
static int g_sqzLookbackCmp;
static int g_sqzNestCmp;
static int g_sqzFlatCmp;
static int g_sqzAliasCmp;
static int g_sqzEdgeCmp;
static int g_sqzRangeCmp;
static int g_sqzOracleCmp;

typedef struct
{
   int    bbPeriod;
   double nbDev;
   int    kcPeriod;
   double wide, normal, narrow;
   int    wantLookback;      /* -1: do not check */
} SqzCase;

/* The three the card measured against pandas / ta4j / trading-signals, plus the
 * shortest periods the ranges allow and a pair where the band leg dominates. */
static const SqzCase sqzCases[] =
{
   { 20, 2.0, 20, 2.0, 1.5, 1.0, 38 },
   { 20, 2.0, 14, 2.0, 1.5, 1.0, 26 },
   { 14, 2.0, 20, 2.0, 1.5, 1.0, 38 },
   {  2, 2.0,  2, 2.0, 1.5, 1.0,  2 },
   { 50, 2.5,  9, 3.0, 1.5, 0.5, 49 },
   { 20, 2.0,  3, 2.0, 1.5, 1.0, 19 },
   { 20, 2.0,  4, 2.0, 1.5, 1.0, 19 },
   { 20, 2.0, 21, 2.0, 1.5, 1.0, 40 },
   { 20, 2.0,100, 2.0, 1.5, 1.0,198 },
   { 20, 2.0, 20, 1.5, 1.5, 1.0, 38 },   /* wide == normal: the classic squeeze */
   { 20, 2.0, 20, 2.0, 1.5, 1.5, 38 },   /* normal == narrow */
};
#define SQZ_NB_CASE ((int)(sizeof(sqzCases)/sizeof(sqzCases[0])))

/* Scratch for the reference composition. */
static TA_Real refBBU[SQZ_NB_BAR], refBBM[SQZ_NB_BAR], refBBL[SQZ_NB_BAR];
static TA_Real refKCM[SQZ_NB_BAR], refBand[SQZ_NB_BAR], refStage[SQZ_NB_BAR];
static TA_Real refDev[SQZ_NB_BAR], refMom[SQZ_NB_BAR];
static TA_Integer refSqz[SQZ_NB_BAR];
static TA_Real gotMom[SQZ_NB_BAR];
static TA_Integer gotSqz[SQZ_NB_BAR];

/* The oracle captures, from each one's first bar to bar 251. A level is one
 * character: m is -1. ta4j has no -1: its NONE is both -1 and 0. */
static const TA_Real sqzOracleMomP2020[214] =
{
   0.1854678571428492, 0.45701249999999494, 0.5141232142857115,
   0.8894089285714228, 1.6433321428571404, 2.169808928571425,
   2.77982678571428, 2.935019642857139, 3.247308928571425,
   3.4619124999999986, 3.718099999999998, 3.7674428571428558,
   3.365214285714284, 3.0642625000000008, 1.9315375000000028,
   1.0271982142857161, 0.06486607142857437, -0.5636357142857107,
   -0.9547642857142822, -1.409514285714283, -1.4328553571428526,
   -1.4390821428571399, -1.6707410714285702, -1.7651624999999957,
   -0.8795464285714245, -0.10758214285713885, 1.0477303571428611,
   2.1522357142857165, 3.1723464285714313, 3.6577767857142867,
   3.991385714285715, 4.236219642857142, 4.195105357142856,
   3.4711160714285687, 2.022644642857139, 0.8246678571428534,
   -0.2564071428571464, 0.6338999999999972, 1.7157892857142816,
   3.457087499999996, 5.276566071428569, 6.24359821428571,
   6.946476785714285, 7.852482142857139, 9.179705357142858,
   10.234014285714286, 11.29322321428572, 11.966123214285716,
   13.18004821428572, 14.080567857142865, 14.811378571428579,
   15.512798214285716, 17.29534107142858, 17.64205714285715,
   17.20223214285715, 16.524248214285713, 14.663623214285714,
   13.239689285714292, 11.435098214285723, 9.433001785714296,
   7.358305357142856, 6.426732142857157, 5.037023214285709,
   3.7790107142857217, 1.9525089285714223, 0.5434750000000008,
   -0.9347089285714318, -1.9739035714285826, -1.8868821428571465,
   -2.479844642857149, -2.807133928571435, -3.2996142857142896,
   -2.93944464285714, -2.442528571428573, -1.8012678571428573,
   -0.12736428571428604, 1.0028107142857126, 2.075039285714285,
   3.41519464285714, 3.933455357142854, 4.09774642857143,
   4.798128571428573, 5.345285714285717, 5.675248214285719,
   5.88479642857143, 6.638958928571434, 7.367823214285714,
   8.326073214285715, 9.166575000000003, 9.815344642857141,
   10.428164285714281, 11.125787499999994, 11.436267857142854,
   11.542639285714275, 11.129826785714279, 10.824116071428557,
   10.304146428571425, 9.34992857142856, 7.487841071428563,
   5.66577857142857, 2.9167714285714164, 0.4205678571428493,
   -2.2233928571428656, -4.192378571428579, -5.48181785714287,
   -6.924425000000005, -7.972810714285718, -9.289664285714295,
   -10.846667857142856, -12.090971428571425, -12.066671428571428,
   -11.465485714285713, -10.873299999999997, -10.549517857142856,
   -9.369617857142854, -8.48768928571428, -7.012096428571429,
   -4.553603571428562, -2.406178571428569, -1.1287571428571361,
   -0.5629321428571386, -0.21981428571428374, 0.3732321428571428,
   0.7119071428571413, 1.3017285714285673, 1.689971428571429,
   2.2501500000000014, 2.2694642857142844, 2.232592857142854,
   2.446821428571427, 2.6347464285714297, 3.3094178571428614,
   4.1584678571428615, 4.211250000000005, 5.037907142857149,
   5.44962142857143, 5.409253571428577, 5.997432142857143,
   6.4562750000000015, 6.093739285714283, 4.749953571428571,
   4.17453214285714, 3.246774999999995, 1.7479821428571327,
   -0.28501071428571834, -1.5750642857142942, -3.072503571428574,
   -4.468739285714289, -6.264289285714287, -7.453603571428576,
   -9.04053928571429, -9.75004285714286, -9.635317857142853,
   -10.032271428571427, -10.396667857142855, -10.707199999999997,
   -10.805117857142857, -11.26888928571428, -12.069492857142855,
   -12.29374642857142, -12.489910714285713, -12.127517857142848,
   -11.708817857142845, -11.323875000000001, -13.290378571428569,
   -14.061921428571425, -14.635571428571415, -14.525110714285717,
   -14.974132142857142, -14.74174642857143, -13.944810714285705,
   -13.106692857142864, -12.216803571428565, -11.305385714285718,
   -10.886099999999997, -10.668435714285712, -9.43966428571428,
   -8.485039285714281, -7.192503571428576, -6.10877857142857,
   -4.753685714285707, -3.776857142857134, -2.6132357142857163,
   -0.9432571428571404, 0.19374285714285655, 2.1419142857142877,
   4.378739285714287, 6.0725892857142885, 6.952450000000001,
   7.84942142857143, 8.693417857142856, 9.019285714285722,
   9.07696071428572, 9.33481428571429, 10.212425000000003,
   11.176607142857147, 12.064585714285712, 12.823196428571428,
   12.63107142857143, 11.254503571428573, 9.965996428571428,
   8.291667857142862, 6.136796428571424, 4.527507142857138,
   3.1487857142857054, 1.8945678571428521, 1.2019928571428533,
   0.016724999999993884, -1.1222500000000046, -1.931821428571432,
   -2.746339285714294, -3.7803642857142954, -4.834017857142861,
   -5.857271428571432
};
static const char sqzOracleLvlP2020[] =
   "222222111111111mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm11122"
   "2222221111mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm121222222222222211mmmmmmmmmm"
   "m111mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm1221mmmmmmmmmmmmmmmmmmmmmmmm1111";
static const TA_Real sqzOracleMomP1420[214] =
{
   0.1854678571428492, 0.45701249999999494, 0.5141232142857115,
   0.8894089285714228, 1.6433321428571404, 2.169808928571425,
   2.77982678571428, 2.935019642857139, 3.247308928571425,
   3.4619124999999986, 3.718099999999998, 3.7674428571428558,
   3.365214285714284, 3.0642625000000008, 1.9315375000000028,
   1.0271982142857161, 0.06486607142857437, -0.5636357142857107,
   -0.9547642857142822, -1.409514285714283, -1.4328553571428526,
   -1.4390821428571399, -1.6707410714285702, -1.7651624999999957,
   -0.8795464285714245, -0.10758214285713885, 1.0477303571428611,
   2.1522357142857165, 3.1723464285714313, 3.6577767857142867,
   3.991385714285715, 4.236219642857142, 4.195105357142856,
   3.4711160714285687, 2.022644642857139, 0.8246678571428534,
   -0.2564071428571464, 0.6338999999999972, 1.7157892857142816,
   3.457087499999996, 5.276566071428569, 6.24359821428571,
   6.946476785714285, 7.852482142857139, 9.179705357142858,
   10.234014285714286, 11.29322321428572, 11.966123214285716,
   13.18004821428572, 14.080567857142865, 14.811378571428579,
   15.512798214285716, 17.29534107142858, 17.64205714285715,
   17.20223214285715, 16.524248214285713, 14.663623214285714,
   13.239689285714292, 11.435098214285723, 9.433001785714296,
   7.358305357142856, 6.426732142857157, 5.037023214285709,
   3.7790107142857217, 1.9525089285714223, 0.5434750000000008,
   -0.9347089285714318, -1.9739035714285826, -1.8868821428571465,
   -2.479844642857149, -2.807133928571435, -3.2996142857142896,
   -2.93944464285714, -2.442528571428573, -1.8012678571428573,
   -0.12736428571428604, 1.0028107142857126, 2.075039285714285,
   3.41519464285714, 3.933455357142854, 4.09774642857143,
   4.798128571428573, 5.345285714285717, 5.675248214285719,
   5.88479642857143, 6.638958928571434, 7.367823214285714,
   8.326073214285715, 9.166575000000003, 9.815344642857141,
   10.428164285714281, 11.125787499999994, 11.436267857142854,
   11.542639285714275, 11.129826785714279, 10.824116071428557,
   10.304146428571425, 9.34992857142856, 7.487841071428563,
   5.66577857142857, 2.9167714285714164, 0.4205678571428493,
   -2.2233928571428656, -4.192378571428579, -5.48181785714287,
   -6.924425000000005, -7.972810714285718, -9.289664285714295,
   -10.846667857142856, -12.090971428571425, -12.066671428571428,
   -11.465485714285713, -10.873299999999997, -10.549517857142856,
   -9.369617857142854, -8.48768928571428, -7.012096428571429,
   -4.553603571428562, -2.406178571428569, -1.1287571428571361,
   -0.5629321428571386, -0.21981428571428374, 0.3732321428571428,
   0.7119071428571413, 1.3017285714285673, 1.689971428571429,
   2.2501500000000014, 2.2694642857142844, 2.232592857142854,
   2.446821428571427, 2.6347464285714297, 3.3094178571428614,
   4.1584678571428615, 4.211250000000005, 5.037907142857149,
   5.44962142857143, 5.409253571428577, 5.997432142857143,
   6.4562750000000015, 6.093739285714283, 4.749953571428571,
   4.17453214285714, 3.246774999999995, 1.7479821428571327,
   -0.28501071428571834, -1.5750642857142942, -3.072503571428574,
   -4.468739285714289, -6.264289285714287, -7.453603571428576,
   -9.04053928571429, -9.75004285714286, -9.635317857142853,
   -10.032271428571427, -10.396667857142855, -10.707199999999997,
   -10.805117857142857, -11.26888928571428, -12.069492857142855,
   -12.29374642857142, -12.489910714285713, -12.127517857142848,
   -11.708817857142845, -11.323875000000001, -13.290378571428569,
   -14.061921428571425, -14.635571428571415, -14.525110714285717,
   -14.974132142857142, -14.74174642857143, -13.944810714285705,
   -13.106692857142864, -12.216803571428565, -11.305385714285718,
   -10.886099999999997, -10.668435714285712, -9.43966428571428,
   -8.485039285714281, -7.192503571428576, -6.10877857142857,
   -4.753685714285707, -3.776857142857134, -2.6132357142857163,
   -0.9432571428571404, 0.19374285714285655, 2.1419142857142877,
   4.378739285714287, 6.0725892857142885, 6.952450000000001,
   7.84942142857143, 8.693417857142856, 9.019285714285722,
   9.07696071428572, 9.33481428571429, 10.212425000000003,
   11.176607142857147, 12.064585714285712, 12.823196428571428,
   12.63107142857143, 11.254503571428573, 9.965996428571428,
   8.291667857142862, 6.136796428571424, 4.527507142857138,
   3.1487857142857054, 1.8945678571428521, 1.2019928571428533,
   0.016724999999993884, -1.1222500000000046, -1.931821428571432,
   -2.746339285714294, -3.7803642857142954, -4.834017857142861,
   -5.857271428571432
};
static const char sqzOracleLvlP1420[] =
   "2222221110mmmm0000mmmm0000mmmm000000mmmmmmmmmmmmm000000mmmmmm00000122222"
   "12222111100000000000mmmm00000mmmmmmmm0000011111111222222222211mmmm000000"
   "000mm00000000000mmm0mmmmmm0000000111222221mmmmmmmmmmmm0000000000012333";
static const TA_Real sqzOracleMomP2014[226] =
{
   -6.515586734693871, -4.889964285714277, -3.8970612244897875,
   -2.654617346938772, -2.0527551020408144, -0.9721173469387754,
   0.03695408163264968, 1.7363877551020357, 2.9597091836734597,
   3.504178571428562, 3.5320969387754992, 2.6911989795918236,
   1.6315765306122336, 0.11873979591835769, -1.0481530612244978,
   -0.9455867346938822, -0.23534693877551344, 0.41732653061224034,
   1.0943979591836694, 1.7794642857142824, 2.3189387755101993,
   2.6325969387755075, 3.430653061224489, 3.8292755102040834,
   3.7702397959183704, 3.3331326530612277, 1.5806071428571475,
   -0.37787755102040377, -2.5460051020408105, -3.788520408163259,
   -4.18185204081632, -4.2440102040816265, -3.3301683673469356,
   -2.310454081632652, -1.2559897959183677, -0.5125306122449,
   1.371556122448979, 2.901923469387752, 4.557015306122448,
   6.044984693877546, 6.579683673469386, 6.3148928571428495,
   5.149035714285711, 3.888653061224488, 2.415484693877552,
   0.08368877551020804, -1.9704948979591803, -3.308836734693873,
   -4.224693877551015, -2.5522244897959157, -0.20526530612244676,
   2.961346938775511, 6.1087857142857125, 8.076285714285717,
   9.748433673469384, 11.52037755102041, 13.013020408163257,
   13.890642857142858, 14.183397959183676, 13.107132653061228,
   12.304622448979586, 11.045938775510205, 8.770423469387763,
   7.839239795918364, 8.659704081632658, 8.88597448979593,
   8.872326530612249, 8.422872448979593, 7.462117346938777,
   6.2319030612244966, 4.885913265306128, 2.781760204081632,
   0.5450510204081613, 0.08417857142856988, -0.604545918367343,
   -1.2148571428571415, -3.0551530612244875, -4.15582653061224,
   -4.152683673469391, -3.3634081632653086, -1.4773520408163265,
   -0.4997091836734666, 0.5024234693877568, 0.8589642857142907,
   0.8422346938775542, 0.6751326530612253, 0.2858928571428588,
   1.9653265306122445, 2.993244897959185, 3.980061224489797,
   4.932857142857145, 5.046479591836739, 4.681306122448979,
   4.482102040816327, 4.9321989795918375, 4.691056122448982,
   4.930795918367349, 5.638193877551028, 6.206321428571436,
   6.7669999999999995, 6.280153061224489, 6.668520408163264,
   6.930469387755101, 7.650418367346942, 8.538316326530618,
   8.85513265306123, 8.642040816326535, 7.839632653061241,
   6.90604081632654, 5.265357142857146, 2.109204081632659,
   -0.2664081632653019, -3.283846938775513, -5.399734693877557,
   -7.920418367346938, -8.953438775510207, -9.033755102040818,
   -9.238826530612238, -8.898724489795912, -8.939591836734689,
   -9.05921428571428, -9.024081632653052, -7.312571428571421,
   -5.07036734693877, -3.8473571428571423, -3.246224489795913,
   -2.22848979591837, -1.784265306122447, -1.1310102040816297,
   0.7824387755102045, 3.057234693877552, 3.4817857142857154,
   3.628714285714291, 2.9765408163265388, 2.6117448979591886,
   1.2818775510204097, 0.5112653061224497, 0.17466326530612464,
   0.014510204081631575, -0.8205816326530619, -0.8911938775510226,
   -0.6183265306122492, -0.5310510204081622, 0.8479693877551073,
   2.9873979591836752, 3.9464795918367326, 5.432153061224488,
   6.398642857142861, 6.740469387755104, 6.7693367346938755,
   6.119163265306127, 4.8379183673469335, 2.468540816326529,
   1.2162244897959131, -0.7646938775510232, -2.7707755102040856,
   -5.400163265306121, -6.491581632653055, -7.451112244897956,
   -8.122897959183675, -8.818755102040818, -8.615693877551015,
   -8.860214285714282, -8.031755102040814, -6.412908163265303,
   -5.56971428571429, -5.854857142857142, -5.7007653061224515,
   -5.389102040816323, -6.166306122448979, -7.919112244897957,
   -8.386673469387755, -8.408265306122452, -8.268061224489795,
   -8.330806122448978, -8.049142857142858, -10.903122448979591,
   -12.112857142857145, -12.359030612244895, -11.750397959183676,
   -11.58727551020408, -10.78378571428572, -8.6205918367347,
   -7.1663775510204175, -6.673357142857157, -5.984938775510209,
   -5.676775510204078, -5.525469387755105, -4.185367346938791,
   -1.9130408163265304, -0.6697142857142868, 0.034357142857142975,
   0.6963061224489842, 0.9955306122449041, 1.0302857142857205,
   0.9350510204081699, 2.3756428571428643, 4.599775510204087,
   6.952377551020412, 8.197744897959186, 8.222775510204087,
   7.744867346938779, 7.062989795918371, 6.268653061224494,
   5.847142857142864, 5.518163265306122, 6.562265306122445,
   7.755948979591835, 8.459551020408156, 8.76175510204081,
   7.559846938775514, 5.540132653061225, 4.3187244897959145,
   2.8990204081632616, 0.8204489795918253, -0.48018367346939694,
   -1.644857142857152, -3.1629285714285764, -4.4302142857142925,
   -6.007112244897964, -6.405489795918367, -5.742530612244897,
   -4.725377551020404, -3.4541530612244884, -2.214275510204077,
   -1.846306122448973
};
static const char sqzOracleLvlP2014[] =
   "0mmmm000011121111111111000000mmmmm00000mmmm00000010mmmmmmmmmmmmmmmmmmmmm"
   "mm000000111122222100000000mmmmmmmmmmmmmmmmmmmmmmmmmmmm000112212222212221"
   "110mmmmm000000110000000000mmmmmmmmmmmmmmmmmmmmmmmmm221mmmmmmmmmmmmmmmmm0"
   "0000000000";
static const TA_Real sqzOracleMomJ2020[214] =
{
   0.18546785714285363, 0.4570124999999958, 0.5141232142857142,
   0.8894089285714273, 1.6433321428571404, 2.169808928571424,
   2.779826785714281, 2.9350196428571405, 3.2473089285714263,
   3.4619124999999933, 3.7180999999999944, 3.7674428571428518,
   3.3652142857142806, 3.0642624999999963, 1.9315374999999935,
   1.0271982142857081, 0.06486607142856649, -0.5636357142857173,
   -0.9547642857142886, -1.4095142857142893, -1.4328553571428593,
   -1.4390821428571456, -1.6707410714285729, -1.7651625000000006,
   -0.8795464285714267, -0.10758214285714018, 1.0477303571428607,
   2.152235714285718, 3.172346428571432, 3.6577767857142884,
   3.991385714285716, 4.236219642857144, 4.195105357142857,
   3.471116071428572, 2.0226446428571414, 0.824667857142855,
   -0.25640714285714683, 0.633899999999997, 1.7157892857142811,
   3.4570874999999948, 5.276566071428565, 6.243598214285711,
   6.946476785714285, 7.852482142857138, 9.179705357142858,
   10.23401428571428, 11.29322321428571, 11.966123214285709,
   13.18004821428572, 14.080567857142853, 14.811378571428563,
   15.51279821428571, 17.295341071428567, 17.642057142857134,
   17.202232142857127, 16.524248214285706, 14.663623214285705,
   13.239689285714276, 11.435098214285704, 9.433001785714271,
   7.35830535714285, 6.426732142857134, 5.0370232142857105,
   3.779010714285704, 1.9525089285714188, 0.5434750000000008,
   -0.934708928571439, -1.973903571428579, -1.8868821428571607,
   -2.4798446428571452, -2.8071339285714174, -3.2996142857142843,
   -2.93944464285714, -2.442528571428568, -1.8012678571428538,
   -0.1273642857142825, 1.0028107142857188, 2.0750392857142894,
   3.4151946428571485, 3.933455357142865, 4.097746428571437,
   4.798128571428581, 5.345285714285723, 5.675248214285723,
   5.884796428571441, 6.638958928571441, 7.367823214285728,
   8.32607321428572, 9.166575000000016, 9.815344642857156,
   10.428164285714296, 11.125787500000015, 11.436267857142873,
   11.542639285714301, 11.1298267857143, 10.824116071428584,
   10.304146428571443, 9.349928571428585, 7.4878410714285835,
   5.665778571428582, 2.916771428571444, 0.4205678571428706,
   -2.2233928571428407, -4.192378571428549, -5.4818178571428575,
   -6.924424999999985, -7.972810714285714, -9.28966428571428,
   -10.846667857142847, -12.090971428571436, -12.066671428571425,
   -11.465485714285705, -10.8733, -10.54951785714286,
   -9.369617857142856, -8.48768928571429, -7.012096428571429,
   -4.553603571428569, -2.406178571428562, -1.1287571428571397,
   -0.5629321428571217, -0.2198142857142784, 0.3732321428571481,
   0.7119071428571573, 1.3017285714285833, 1.6899714285714396,
   2.250150000000019, 2.2694642857142924, 2.232592857142876,
   2.4468214285714325, 2.6347464285714466, 3.3094178571428756,
   4.158467857142881, 4.211250000000021, 5.037907142857165,
   5.449621428571447, 5.409253571428586, 5.99743214285715,
   6.456275000000005, 6.0937392857142925, 4.749953571428577,
   4.174532142857142, 3.246774999999996, 1.7479821428571345,
   -0.28501071428571834, -1.575064285714305, -3.0725035714285696,
   -4.4687392857142925, -6.264289285714284, -7.4536035714286015,
   -9.040539285714289, -9.750042857142887, -9.635317857142866,
   -10.03227142857142, -10.396667857142887, -10.7072,
   -10.805117857142847, -11.26888928571428, -12.069492857142862,
   -12.293746428571424, -12.489910714285713, -12.127517857142834,
   -11.708817857142847, -11.323874999999987, -13.290378571428562,
   -14.06192142857141, -14.63557142857141, -14.525110714285702,
   -14.97413214285713, -14.741746428571417, -13.944810714285694,
   -13.106692857142846, -12.216803571428558, -11.305385714285698,
   -10.88609999999998, -10.668435714285696, -9.43966428571428,
   -8.485039285714272, -7.19250357142856, -6.108778571428559,
   -4.753685714285709, -3.776857142857125, -2.613235714285679,
   -0.943257142857135, 0.19374285714286543, 2.141914285714307,
   4.3787392857143175, 6.072589285714287, 6.952450000000027,
   7.849421428571446, 8.693417857142862, 9.01928571428573,
   9.076960714285747, 9.334814285714316, 10.212425000000025,
   11.176607142857165, 12.064585714285698, 12.823196428571464,
   12.631071428571431, 11.254503571428614, 9.96599642857143,
   8.291667857142869, 6.13679642857144, 4.527507142857161,
   3.1487857142857223, 1.8945678571428601, 1.201992857142855,
   0.016725000000022305, -1.1222499999999798, -1.9318214285714248,
   -2.7463392857142708, -3.7803642857142563, -4.83401785714284,
   -5.857271428571437
};
static const char sqzOracleLvlJ2020[] =
   "222222111111111000000000000000000000000000000000000000000000000000011122"
   "222222111100000000000000000000000000000000001212222222222222110000000000"
   "0111000000000000000000000000000000000012210000000000000000000000001111";

/**** Local functions declarations.    ****/

static ErrorNumber sqzReference( const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars,
                                 const SqzCase *c, int startIdx,
                                 int *refBeg, int *refNb );
static ErrorNumber sqzLegCompose( TA_History *history );
static ErrorNumber sqzLegBoundary( TA_History *history );
static ErrorNumber sqzLegLookback( TA_History *history );
static ErrorNumber sqzLegNesting( TA_History *history );
static ErrorNumber sqzLegFlat( void );
static ErrorNumber sqzLegAlias( TA_History *history );
static ErrorNumber sqzLegEdges( TA_History *history );
static ErrorNumber sqzLegSubRange( TA_History *history );
static ErrorNumber sqzLegRange( TA_History *history );
static ErrorNumber sqzLegOracles( TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_sqzmom( TA_History *history )
{
   ErrorNumber e;

   if( (int)history->nbBars != SQZ_NB_BAR )
   {
      printf( "Fail: SQZMOM expects %d bars, history has %d\n",
              SQZ_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   g_sqzComposeCmp = g_sqzBoundaryCmp = g_sqzBoundaryEdge = 0;
   g_sqzLookbackCmp = g_sqzNestCmp = g_sqzFlatCmp = 0;
   g_sqzAliasCmp = g_sqzEdgeCmp = g_sqzRangeCmp = g_sqzOracleCmp = 0;

   if( (e = sqzLegCompose ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegBoundary( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegLookback( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegNesting ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegFlat    (         )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegAlias   ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegEdges   ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegSubRange( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegRange   ( history )) != TA_TEST_PASS ) return e;
   if( (e = sqzLegOracles ( history )) != TA_TEST_PASS ) return e;

   /* Every leg must have compared what it says it compares. Without these a
    * leg that silently stopped early reads exactly like a leg that passed. */
   if( g_sqzComposeCmp  != SQZ_COMPOSE_CMP  ||
       g_sqzBoundaryCmp != SQZ_BOUNDARY_CMP ||
       g_sqzBoundaryEdge < SQZ_BOUNDARY_EDGE ||
       g_sqzLookbackCmp != SQZ_LOOKBACK_CMP ||
       g_sqzNestCmp     != SQZ_NEST_CMP     ||
       g_sqzFlatCmp     != SQZ_FLAT_CMP     ||
       g_sqzAliasCmp    != SQZ_ALIAS_CMP    ||
       g_sqzEdgeCmp     != SQZ_EDGE_CMP     ||
       g_sqzRangeCmp    != SQZ_RANGE_CMP    ||
       g_sqzOracleCmp   != SQZ_ORACLE_CMP )
   {
      printf( "Fail: SQZMOM comparison counts %d/%d/%d/%d/%d/%d/%d/%d/%d/%d, expected "
              "%d/%d/>=%d/%d/%d/%d/%d/%d/%d/%d\n",
              g_sqzComposeCmp, g_sqzBoundaryCmp, g_sqzBoundaryEdge,
              g_sqzLookbackCmp, g_sqzNestCmp, g_sqzFlatCmp, g_sqzAliasCmp,
              g_sqzEdgeCmp, g_sqzRangeCmp, g_sqzOracleCmp,
              SQZ_COMPOSE_CMP, SQZ_BOUNDARY_CMP, SQZ_BOUNDARY_EDGE,
              SQZ_LOOKBACK_CMP, SQZ_NEST_CMP, SQZ_FLAT_CMP, SQZ_ALIAS_CMP,
              SQZ_EDGE_CMP, SQZ_RANGE_CMP, SQZ_ORACLE_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* The card's formula, every leg a public call, nothing of its own but the two
 * lines the card writes out. Fills refMom/refSqz, 0-based at *refBeg. The
 * output starts at startIdx or at the lookback, whichever is later. */
static ErrorNumber sqzReference( const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars,
                                 const SqzCase *c, int startIdx,
                                 int *refBeg, int *refNb )
{
   TA_RetCode rc;
   TA_Integer b1, n1;
   int i, beg, nb, stage, stageLen, leg;

   beg = c->bbPeriod - 1;
   leg = 1 + (c->kcPeriod - 1);
   if( leg > beg ) beg = leg;
   leg = 2 * (c->kcPeriod - 1);
   if( leg > beg ) beg = leg;
   if( startIdx > beg ) beg = startIdx;

   nb = nbBars - beg;
   stage = beg - (c->kcPeriod - 1);
   stageLen = nbBars - stage;

   rc = TA_BBANDS( beg, nbBars-1, close, c->bbPeriod, c->nbDev, c->nbDev,
                   TA_MAType_SMA, &b1, &n1, refBBU, refBBM, refBBL );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_SMA( beg, nbBars-1, close, c->kcPeriod, &b1, &n1, refKCM );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_TRANGE( stage, nbBars-1, high, low, close, &b1, &n1, refBand );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;
   rc = TA_SMA( 0, stageLen-1, refBand, c->kcPeriod, &b1, &n1, refBand );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   rc = TA_MIDPRICE( stage, nbBars-1, high, low, c->kcPeriod, &b1, &n1, refDev );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;
   rc = TA_SMA( stage, nbBars-1, close, c->kcPeriod, &b1, &n1, refStage );
   if( rc != TA_SUCCESS || n1 != stageLen ) goto refFail;

   for( i = 0; i < stageLen; i++ )
      refDev[i] = close[stage+i] - (refDev[i] + refStage[i]) * 0.5;

   rc = TA_LINEARREG( 0, stageLen-1, refDev, c->kcPeriod, &b1, &n1, refMom );
   if( rc != TA_SUCCESS || n1 != nb ) goto refFail;

   for( i = 0; i < nb; i++ )
   {
      double up = refBBU[i], lo = refBBL[i], ce = refKCM[i], bd = refBand[i];

      if( lo > ce - c->narrow*bd && up < ce + c->narrow*bd )
         refSqz[i] = 3;
      else if( lo > ce - c->normal*bd && up < ce + c->normal*bd )
         refSqz[i] = 2;
      else if( lo > ce - c->wide*bd && up < ce + c->wide*bd )
         refSqz[i] = 1;
      else if( lo < ce - c->wide*bd && up > ce + c->wide*bd )
         refSqz[i] = -1;
      else
         refSqz[i] = 0;
   }

   *refBeg = beg;
   *refNb  = nb;
   return TA_TEST_PASS;

refFail:
   printf( "Fail: SQZMOM reference leg failed (b=%d k=%d) rc=%d nb=%d\n",
           c->bbPeriod, c->kcPeriod, (int)rc, (int)n1 );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* Leg 1. The composition, bit for bit, and the only leg that reaches a server. */
static ErrorNumber sqzLegCompose( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, i, refBeg, refNb, nbBars;
   ErrorNumber e;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];
      int cmpBefore = 0;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM b=%d k=%d rc=%d\n",
                 c->bbPeriod, c->kcPeriod, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: TA_SQZMOM b=%d k=%d begIdx %d/%d nbElement %d/%d\n",
                 c->bbPeriod, c->kcPeriod, (int)begIdx, refBeg,
                 (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* Once per case, the same call through every language server. Without
       * this the group runs under --codegen having compared nothing against
       * any server, which --codegen reports as a failure. */
      cmpBefore = server_verify_comparisons();
      if( server_verify_active() )
      {
         const double opt[6] = { (double)c->bbPeriod, c->nbDev,
                                 (double)c->kcPeriod,
                                 c->wide, c->normal, c->narrow };
         e = server_verify( "SQZMOM", 0, nbBars-1, nbBars,
                rc, begIdx, nbElement,
                (const TA_Real*[]){ history->high, history->low,
                                    history->close, NULL },
                opt, 6,
                (const TA_Real*[]){ gotMom, NULL },
                (const TA_Integer*[]){ gotSqz, NULL } );
         if( e != TA_TEST_PASS )
            return e;
         /* "No failure reported" and "nothing was compared" are the same
          * observation without this. */
         if( server_verify_comparisons() == cmpBefore )
         {
            printf( "SQZMOM [b=%d k=%d]: compared no server despite a live "
                    "session\n", c->bbPeriod, c->kcPeriod );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotMom[i], &refMom[i], sizeof(TA_Real) ) != 0 )
         {
            printf( "Fail: TA_SQZMOM b=%d k=%d bar %d outMomentum %.17g, the "
                    "composition gives %.17g\n", c->bbPeriod, c->kcPeriod,
                    refBeg+i, gotMom[i], refMom[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzComposeCmp++;

         if( gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM b=%d k=%d bar %d outSqueeze %d, the "
                    "composition gives %d\n", c->bbPeriod, c->kcPeriod,
                    refBeg+i, (int)gotSqz[i], (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzComposeCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 2. The ordinal decided at a channel edge.
 *
 * At leg 1's factors every bar sits far from an edge: scaling BAND by 1+1e-7
 * moves not one ordinal, so that leg proves the ladder is wired up and almost
 * nothing about its arithmetic. Here the normal factor is read off the data --
 * (BBU - KCM)/BAND at a chosen bar -- so that bar lies ON the upper edge. The
 * leg asserts that some bar really is within a few ulps of an edge, because a
 * tuned factor that lands nowhere near one would prove nothing either.
 */
static ErrorNumber sqzLegBoundary( TA_History *history )
{
   static const int tuneBar[3] = { 60, 120, 200 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, i, refBeg, refNb, nbBars;
   ErrorNumber e;
   SqzCase c;

   nbBars = (int)history->nbBars;

   for( t = 0; t < 3; t++ )
   {
      double tuned;

      c.bbPeriod = 20; c.nbDev = 2.0; c.kcPeriod = 20;
      c.wide = 2.0; c.normal = 1.5; c.narrow = 1.0; c.wantLookback = -1;

      /* First pass: read the factor at which tuneBar[t] flips its upper test. */
      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, &c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      i = tuneBar[t] - refBeg;
      if( i < 0 || i >= refNb || !(refBand[i] > 0.0) )
      {
         printf( "Fail: SQZMOM boundary leg, bar %d is outside the output or "
                 "has a zero band\n", tuneBar[t] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      tuned = (refBBU[i] - refKCM[i]) / refBand[i];
      if( !(tuned > 0.0) )
      {
         printf( "Fail: SQZMOM boundary leg, bar %d gives a non-positive "
                 "factor %.17g\n", tuneBar[t], tuned );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      /* Second pass at that factor: the chosen bar now sits on the edge. */
      c.wide = tuned + 0.5; c.normal = tuned; c.narrow = tuned - 0.5;
      if( !(c.narrow > 0.0) ) c.narrow = tuned * 0.5;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c.bbPeriod, c.nbDev, c.kcPeriod,
                      c.wide, c.normal, c.narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM boundary rc=%d\n", (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, &c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: SQZMOM boundary begIdx %d/%d nbElement %d/%d\n",
                 (int)begIdx, refBeg, (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < nbElement; i++ )
      {
         double edge, gap, ulp;

         if( gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM boundary bar %d outSqueeze %d, the "
                    "composition gives %d\n", refBeg+i, (int)gotSqz[i],
                    (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzBoundaryCmp++;

         /* How many bars actually landed on an edge: without at least a few,
          * this leg is leg 1 again under another name. */
         edge = refKCM[i] + c.normal * refBand[i];
         gap  = fabs( refBBU[i] - edge );
         ulp  = fabs( edge ) * 2.220446049250313e-16;
         if( gap <= 4.0 * ulp )
            g_sqzBoundaryEdge++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 3. The lookback is the max of the four chains, and the call starts there. */
static ErrorNumber sqzLegLookback( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, want, nbBars;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];
      int got = TA_SQZMOM_Lookback( c->bbPeriod, c->nbDev, c->kcPeriod,
                                    c->wide, c->normal, c->narrow );

      want = c->bbPeriod - 1;
      if( 1 + (c->kcPeriod - 1) > want ) want = 1 + (c->kcPeriod - 1);
      if( 2 * (c->kcPeriod - 1) > want ) want = 2 * (c->kcPeriod - 1);

      if( got != want || (c->wantLookback >= 0 && got != c->wantLookback) )
      {
         printf( "Fail: TA_SQZMOM_Lookback(b=%d,k=%d) = %d, expected %d"
                 " (card: %d)\n", c->bbPeriod, c->kcPeriod, got, want,
                 c->wantLookback );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzLookbackCmp++;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS || begIdx != want )
      {
         printf( "Fail: TA_SQZMOM(b=%d,k=%d) rc=%d begIdx=%d, expected %d\n",
                 c->bbPeriod, c->kcPeriod, (int)rc, (int)begIdx, want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzLookbackCmp++;
   }

   /* No leg may be routed through a function that has an unstable period. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 50 );
   k = TA_SQZMOM_Lookback( 20, 2.0, 20, 2.0, 1.5, 1.0 );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   if( k != 38 )
   {
      printf( "Fail: TA_SQZMOM_Lookback is %d under an unstable period of 50, "
              "expected 38\n", k );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_sqzLookbackCmp++;

   return TA_TEST_PASS;
}

/* Leg 4. The ladder nests. The three channels share a centre and BAND >= 0, so
 * a bar inside the narrow one is inside the normal and the wide one too. A
 * level the nesting forbids means the ladder was tested in the wrong order.
 */
static ErrorNumber sqzLegNesting( TA_History *history )
{
   int k, i, refBeg, refNb, nbBars;
   ErrorNumber e;

   nbBars = (int)history->nbBars;

   for( k = 0; k < SQZ_NB_CASE; k++ )
   {
      const SqzCase *c = &sqzCases[k];

      if( (e = sqzReference( history->high, history->low, history->close,
                             nbBars, c, 0, &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      for( i = 0; i < refNb; i++ )
      {
         double up = refBBU[i], lo = refBBL[i], ce = refKCM[i], bd = refBand[i];
         int inW, inN, inR, bad = 0;

         inR = ( lo > ce - c->narrow*bd && up < ce + c->narrow*bd );
         inN = ( lo > ce - c->normal*bd && up < ce + c->normal*bd );
         inW = ( lo > ce - c->wide*bd   && up < ce + c->wide*bd   );

         if( bd < 0.0 )
            bad = 1;                                   /* a mean of true ranges is never negative */
         else if( inR && !(inN && inW) )
            bad = 1;                                   /* narrow must imply normal and wide */
         else if( inN && !inW )
            bad = 1;                                   /* normal must imply wide */
         else if( refSqz[i] == 3 && !inR )
            bad = 1;
         else if( refSqz[i] == 2 && !inN )
            bad = 1;
         else if( refSqz[i] == 1 && !inW )
            bad = 1;
         else if( refSqz[i] == -1 && inW )
            bad = 1;

         if( bad )
         {
            printf( "Fail: SQZMOM nesting b=%d k=%d bar %d level %d, "
                    "inside narrow/normal/wide = %d/%d/%d, band %.17g\n",
                    c->bbPeriod, c->kcPeriod, refBeg+i, (int)refSqz[i],
                    inR, inN, inW, bd );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzNestCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 5. A constant series. BAND is 0 and the bands collapse onto the centre,
 * so no strict inequality holds on either side and every bar reads 0.
 */
static ErrorNumber sqzLegFlat( void )
{
   static TA_Real flat[SQZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < SQZ_NB_BAR; i++ )
      flat[i] = 42.5;

   rc = TA_SQZMOM( 0, SQZ_NB_BAR-1, flat, flat, flat,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || begIdx != 38 )
   {
      printf( "Fail: SQZMOM flat rc=%d begIdx=%d, expected 38\n",
              (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      if( gotSqz[i] != 0 )
      {
         printf( "Fail: SQZMOM flat bar %d outSqueeze %d, expected 0: with a "
                 "zero band no channel has width and no test is strict\n",
                 (int)begIdx+i, (int)gotSqz[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzFlatCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 6. Aliasing. The body must read every bar input before it writes a caller
 * buffer. It did not at first: TA_LINEARREG wrote outMomentum in the middle and
 * TA_TRANGE then read a high or low the caller had aliased onto it. The
 * abstract layer caught that; this pins it in the group itself.
 */
static ErrorNumber sqzLegAlias( TA_History *history )
{
   static TA_Real buf[SQZ_NB_BAR];
   static TA_Real baseMom[SQZ_NB_BAR];
   static TA_Integer baseSqz[SQZ_NB_BAR];
   static TA_Integer ibuf[SQZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, baseBeg, baseNb;
   int src, out, i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &baseBeg, &baseNb, baseMom, baseSqz );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: SQZMOM alias baseline rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( out = 0; out < 2; out++ )
   {
      for( src = 0; src < 3; src++ )
      {
         const TA_Real *h = history->high, *l = history->low, *c = history->close;

         memcpy( buf, src==0 ? history->high : src==1 ? history->low
                                             : history->close,
                 nbBars * sizeof(TA_Real) );
         if( src == 0 ) h = buf; else if( src == 1 ) l = buf; else c = buf;

         rc = TA_SQZMOM( 0, nbBars-1, h, l, c, 20, 2.0, 20, 2.0, 1.5, 1.0,
                         &begIdx, &nbElement,
                         out==0 ? buf : gotMom,
                         out==0 ? gotSqz : ibuf );
         if( rc != TA_SUCCESS || begIdx != baseBeg || nbElement != baseNb )
         {
            printf( "Fail: SQZMOM alias out%d over input %d rc=%d\n",
                    out, src, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbElement; i++ )
         {
            if( out == 0 )
            {
               if( memcmp( &buf[i], &baseMom[i], sizeof(TA_Real) ) != 0 ||
                   gotSqz[i] != baseSqz[i] )
                  goto aliasDiffers;
            }
            else
            {
               if( memcmp( &gotMom[i], &baseMom[i], sizeof(TA_Real) ) != 0 ||
                   ibuf[i] != baseSqz[i] )
                  goto aliasDiffers;
            }
         }
         g_sqzAliasCmp++;
         continue;

      aliasDiffers:
         printf( "Fail: SQZMOM out%d aliased over input %d differs at bar %d\n",
                 out, src, baseBeg+i );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 7. Parameter edges. A period below its range is rejected; nbdev of 0
 * collapses both bands onto the middle band, which is strictly inside every
 * channel of positive width, so every bar reads the tightest level.
 */
static ErrorNumber sqzLegEdges( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   1, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_BAD_PARAM )
   {
      printf( "Fail: SQZMOM bbPeriod=1 rc=%d, expected TA_BAD_PARAM\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 2.0, 1, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_BAD_PARAM )
   {
      printf( "Fail: SQZMOM kcPeriod=1 rc=%d, expected TA_BAD_PARAM\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                   20, 0.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || begIdx != 38 )
   {
      printf( "Fail: SQZMOM nbDev=0 rc=%d begIdx=%d\n", (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   for( i = 0; i < nbElement; i++ )
   {
      if( gotSqz[i] != 3 )
      {
         printf( "Fail: SQZMOM nbDev=0 bar %d outSqueeze %d, expected 3: both "
                 "bands sit on the middle band\n", (int)begIdx+i,
                 (int)gotSqz[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;
   }

   /* Factor order: wide >= normal >= narrow, ties allowed. */
   {
      static const struct { double wide, normal, narrow; TA_RetCode want; } order[] =
      {
         { 1.4, 1.5, 1.0, TA_BAD_PARAM },   /* normal above wide */
         { 2.0, 1.5, 1.6, TA_BAD_PARAM },   /* narrow above normal */
         { 2.0, 1.5,-0.5, TA_BAD_PARAM },   /* below the range */
         { 1.5, 1.5, 1.5, TA_SUCCESS },
      };

      for( i = 0; i < (int)(sizeof(order)/sizeof(order[0])); i++ )
      {
         int lb = TA_SQZMOM_Lookback( 20, 2.0, 20, order[i].wide,
                                      order[i].normal, order[i].narrow );

         rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                         20, 2.0, 20, order[i].wide, order[i].normal,
                         order[i].narrow, &begIdx, &nbElement, gotMom, gotSqz );
         if( rc != order[i].want )
         {
            printf( "Fail: SQZMOM factors %g/%g/%g rc=%d, expected %d\n",
                    order[i].wide, order[i].normal, order[i].narrow,
                    (int)rc, (int)order[i].want );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         g_sqzEdgeCmp++;

         if( lb != ( order[i].want == TA_SUCCESS ? 38 : -1 ) )
         {
            printf( "Fail: TA_SQZMOM_Lookback factors %g/%g/%g = %d\n",
                    order[i].wide, order[i].normal, order[i].narrow, lb );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzEdgeCmp++;
      }
   }

   /* One bar short of the lookback: success and nothing written. */
   rc = TA_SQZMOM( 0, 37, history->high, history->low, history->close,
                   20, 2.0, 20, 2.0, 1.5, 1.0,
                   &begIdx, &nbElement, gotMom, gotSqz );
   if( rc != TA_SUCCESS || nbElement != 0 )
   {
      printf( "Fail: SQZMOM over 38 bars rc=%d nbElement=%d, expected no output\n",
              (int)rc, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_sqzEdgeCmp++;

   /* Exactly one output bar, equal to the first bar of the full run. */
   {
      TA_Real oneMom = 0.0;
      TA_Integer oneSqz = 99;

      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      20, 2.0, 20, 2.0, 1.5, 1.0,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS || begIdx != 38 )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      rc = TA_SQZMOM( 0, 38, history->high, history->low, history->close,
                      20, 2.0, 20, 2.0, 1.5, 1.0,
                      &begIdx, &nbElement, &oneMom, &oneSqz );
      if( rc != TA_SUCCESS || begIdx != 38 || nbElement != 1 )
      {
         printf( "Fail: SQZMOM over 39 bars rc=%d begIdx=%d nbElement=%d\n",
                 (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      g_sqzEdgeCmp++;

      if( memcmp( &oneMom, &gotMom[0], sizeof(TA_Real) ) != 0 )
      {
         printf( "Fail: SQZMOM single bar outMomentum %.17g, the full run gives "
                 "%.17g\n", oneMom, gotMom[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;

      if( oneSqz != gotSqz[0] )
      {
         printf( "Fail: SQZMOM single bar outSqueeze %d, the full run gives %d\n",
                 (int)oneSqz, (int)gotSqz[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_sqzEdgeCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 8. Entered above the lookback and ended early. Leg 1 always starts at the
 * lookback, where the staged range begins as early as it ever can.
 */
static ErrorNumber sqzLegSubRange( TA_History *history )
{
   static const struct { int start, nbBars; } sub[2] = { { 100, 252 }, { 57, 200 } };
   const SqzCase *c = &sqzCases[0];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, i, refBeg, refNb;
   ErrorNumber e;

   for( t = 0; t < 2; t++ )
   {
      rc = TA_SQZMOM( sub[t].start, sub[t].nbBars-1,
                      history->high, history->low, history->close,
                      c->bbPeriod, c->nbDev, c->kcPeriod,
                      c->wide, c->normal, c->narrow,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS )
      {
         printf( "Fail: TA_SQZMOM from bar %d rc=%d\n", sub[t].start, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( (e = sqzReference( history->high, history->low, history->close,
                             sub[t].nbBars, c, sub[t].start,
                             &refBeg, &refNb )) != TA_TEST_PASS )
         return e;

      if( begIdx != sub[t].start || begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: TA_SQZMOM from bar %d begIdx %d/%d nbElement %d/%d\n",
                 sub[t].start, (int)begIdx, refBeg, (int)nbElement, refNb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( memcmp( &gotMom[i], &refMom[i], sizeof(TA_Real) ) != 0
             || gotSqz[i] != refSqz[i] )
         {
            printf( "Fail: TA_SQZMOM from bar %d, bar %d gives %.17g/%d, the "
                    "composition %.17g/%d\n", sub[t].start, refBeg+i,
                    gotMom[i], (int)gotSqz[i], refMom[i], (int)refSqz[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzRangeCmp += 2;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 9. A sub-range equals the same slice of a full run, within the drift of
 * the running sums; the ordinal exactly. */
static TA_RetCode sqzRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   TA_History *h = (TA_History *)opaqueData;
   static TA_Real otherMom[MAX_RANGE_SIZE];
   static TA_Integer otherSqz[MAX_RANGE_SIZE];

   *lookback = TA_SQZMOM_Lookback( 20, 2.0, 20, 2.0, 1.5, 1.0 );
   *isOutputInteger = ( outputNb == 1 );

   return TA_SQZMOM( startIdx, endIdx, h->high, h->low, h->close,
                     20, 2.0, 20, 2.0, 1.5, 1.0, outBegIdx, outNbElement,
                     outputNb == 0 ? outputBuffer : otherMom,
                     outputNb == 1 ? outputBufferInt : otherSqz );
}

static ErrorNumber sqzLegRange( TA_History *history )
{
   return doRangeTestEx( sqzRangeTestFunction, TA_STABLE_EPSILON,
                         TA_TEST_UNST_NONE, (void *)history, 2, 0 );
}

/* Leg 10. Two other libraries over the same bars. The momentum is held to
 * 1e-13 of the largest close: the distance is the rounding of the running sums
 * in TA_SMA and TA_LINEARREG, which scales with the price and not with the
 * momentum, so a relative bound fails where the momentum crosses zero. The
 * level is exact.
 */
static ErrorNumber sqzLegOracles( TA_History *history )
{
   static const struct
   {
      const char *name;
      int bbPeriod, kcPeriod, mergeOff;
      const TA_Real *mom;
      const char *lvl;
   } cap[4] =
   {
      { "pandas", 20, 20, 0, sqzOracleMomP2020, sqzOracleLvlP2020 },
      { "pandas", 14, 20, 0, sqzOracleMomP1420, sqzOracleLvlP1420 },
      { "pandas", 20, 14, 0, sqzOracleMomP2014, sqzOracleLvlP2014 },
      { "ta4j",   20, 20, 1, sqzOracleMomJ2020, sqzOracleLvlJ2020 },
   };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int t, i, nbBars, want, got;
   double tol;

   nbBars = (int)history->nbBars;

   tol = 0.0;
   for( i = 0; i < nbBars; i++ )
      if( fabs( history->close[i] ) > tol ) tol = fabs( history->close[i] );
   tol *= 1e-13;

   for( t = 0; t < 4; t++ )
   {
      rc = TA_SQZMOM( 0, nbBars-1, history->high, history->low, history->close,
                      cap[t].bbPeriod, 2.0, cap[t].kcPeriod, 2.0, 1.5, 1.0,
                      &begIdx, &nbElement, gotMom, gotSqz );
      if( rc != TA_SUCCESS || nbElement != (int)strlen( cap[t].lvl )
          || begIdx != nbBars - nbElement )
      {
         printf( "Fail: SQZMOM vs %s b=%d k=%d rc=%d begIdx=%d nbElement=%d, the "
                 "capture has %d bars\n", cap[t].name, cap[t].bbPeriod,
                 cap[t].kcPeriod, (int)rc, (int)begIdx, (int)nbElement,
                 (int)strlen( cap[t].lvl ) );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( !checkOracleValue( gotMom[i], cap[t].mom[i], 0.0, tol, NULL, NULL ) )
         {
            printf( "Fail: SQZMOM b=%d k=%d bar %d outMomentum %.17g, %s gives "
                    "%.17g\n", cap[t].bbPeriod, cap[t].kcPeriod, (int)begIdx+i,
                    gotMom[i], cap[t].name, cap[t].mom[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzOracleCmp++;

         want = ( cap[t].lvl[i] == 'm' ) ? -1 : cap[t].lvl[i] - '0';
         got = gotSqz[i];
         if( cap[t].mergeOff && got < 0 )
            got = 0;
         if( got != want )
         {
            printf( "Fail: SQZMOM b=%d k=%d bar %d outSqueeze %d, %s gives %d\n",
                    cap[t].bbPeriod, cap[t].kcPeriod, (int)begIdx+i,
                    (int)gotSqz[i], cap[t].name, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_sqzOracleCmp++;
      }
   }

   return TA_TEST_PASS;
}
