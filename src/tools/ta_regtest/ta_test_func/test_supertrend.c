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
 *  090126 MF,CC  First version (issue #272).
 */

/* Description:
 *
 *   Test TA_SUPERTREND.
 *
 *   Almost nothing generic covers this function. The --codegen sweep cannot
 *   value-compare it against the frozen ta_ref_serve, which predates it; and
 *   the one leg that would otherwise vary startIdx is switched OFF, because
 *   `path_dependent` maps to TA_DO_NOT_COMPARE and thence to TA_STABLE_SKIP.
 *   The seed and the trend latch -- the two things most likely to be wrong --
 *   are exactly what that leg would have probed. Everything below is therefore
 *   load-bearing rather than supplementary.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE (formula correctness) on the 1000-bar gData corpus,
 *        four parameter sets including the defaults, and on the 252-bar TA_SREF
 *        corpus at two fast sets. The line to a tolerance, the trend EXACTLY.
 *        Plus, from the same oracle, the COMPLETE list of bars at which the
 *        trend changes over two 1000-bar sets -- 126 exact integers pinning
 *        every bar of the trajectory, not 22 spot values. Plus cross-language,
 *        bitwise, on every one of those calls.
 *     2. SEED, bitwise: the first output bar is the unclamped lower band,
 *        TA_MEDPRICE - multiplier * TA_ATR at that bar, and the trend there is
 *        +1. Nothing else in the suite pins the seed. A constructed series then
 *        pins the seed's UPPER band, which the corpora barely reach.
 *     3. DIFFERENTIAL, bitwise, on EVERY emitted bar: the whole recurrence
 *        rebuilt over shipped TA_ATR and TA_MEDPRICE. This is what proves the
 *        inlined Wilder recursion IS TA_ATR, and what pins the bars between
 *        leg 1's spot values.
 *     4. INVARIANTS, on every emitted bar: the trend is +-1 and the line is
 *        finite. The geometric reading -- the close on the near side of the
 *        line -- holds only where the carried bands have not crossed, so it is
 *        checked inside leg 3's rebuild, which knows the bands, and the crossed
 *        bars are counted rather than silently skipped.
 *     5. THE ATR's UNSTABLE PERIOD, which is dead code at its default and is
 *        swept by nothing else: this function declares no unstable period of its
 *        own, so it is outside UNSTABLE_MAP and outside the generic unstable leg.
 *     6. PATH DEPENDENCE, earned rather than assumed: TA_FUNC_FLG_PATH_DEP is
 *        published, a sub-range call really does differ from the full-history
 *        one, and it equals a full-history call over the truncated input --
 *        i.e. the range dependence is a re-anchor and nothing else.
 *     7. DEGENERATE inputs: a multiplier of 0, and a flat series whose ATR is 0.
 *
 *   Legs 2-7 are built from shipped primitives and from the definition, so they
 *   prove the transcription and the contract, never the formula; that is leg 1's
 *   job.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP  1024   /* > ST_GD_NB and > history->nbBars */
#define ST_GD_NB 1000   /* prefix of the 10000-bar profiling corpus used by leg 1 */

extern double gDataHigh[];
extern double gDataLow[];
extern double gDataClose[];

/* Golden values come from ta4j 0.22.6 (org.ta4j:ta4j-core, Maven Central),
 * driven live over the JSON-RPC oracle server in the private ta-lib-oracles
 * repo (ta4j_serve), on OpenJDK 21.0.12. Inputs were sent as hex-of-IEEE-bits
 * (issue #115) and the returned hex decoded back bit-identically to the decimal
 * arrays, so the capture is lossless; every row below was emitted mechanically
 * from that session, none transcribed by hand.
 *
 * ta4j is the only independent implementation of THIS formula that could be
 * run. Its SuperTrendIndicator carries both bands every bar, releases each on
 * the previous close crossing it, and tests the flip against the current bar's
 * band -- the form TradingView's own documentation also states. pandas-ta and
 * pandas-ta-classic look like a second oracle and are not one: they clamp only
 * the band the trend is riding and test the flip against the PREVIOUS bar's
 * bands, which is a different function and disagrees at flips for reasons that
 * are not bugs. The AmiBroker script often cited alongside them is the same
 * lineage, not a third opinion.
 *
 * WHY THE PINNED BARS ARE DEEP IN THE CORPUS. ta4j's ATR is Wilder-smoothed
 * over the same true ranges and starts on the same bar as TA_ATR -- its
 * getCountOfUnstableBars() is the period, exactly TA_ATR_Lookback with the
 * unstable period at its default -- but it seeds with the RAW true range at
 * that bar where TA-Lib seeds with the mean of the first `period` of them. The
 * gap decays at (1 - 1/period) per bar. That is a transient in the band, and
 * the trend latch turns it into a DISCRETE disagreement while it lasts: on
 * gData the two disagree on the trend at 1 bar (10,3.0) and 12 bars (14,4.0),
 * the last at index 48, and on none afterwards. So the rows are taken from the
 * last tenth of each corpus, where the transient is spent.
 *
 * TOLERANCE. Over every row below, measured: the sixteen gData rows agree with
 * ta4j BIT-FOR-BIT, and the worst of the six TA_SREF rows is 3.9e-15 relative
 * (the residual is the two libraries' different Wilder arithmetic order --
 * prev + (tr - prev)/M against prev *= M-1; prev += tr; prev /= M -- which is
 * algebraically equal and not numerically). 1e-12 leaves ~250x headroom over
 * that and stays ~1e10 tighter than the rival ratchet variant, which differs by
 * percent. Do NOT tighten it to a bitwise comparison: the gData rows agree
 * bit-for-bit at these particular bars by luck, not by construction.
 *
 * The TREND is compared EXACTLY. It is an integer with no rounding to absorb,
 * and it is the half of the output a tolerance could never police.
 */
#define ST_ORACLE_TOL 1e-12
#define ST_ORACLE_ABS 1e-12

typedef struct { int period; double mult; int begIdx; int nbElement; } StShape;

/* bar is the ABSOLUTE bar index; the output index is bar - begIdx. */
typedef struct { int period; double mult; int bar;
                 double line; int trend; } StGolden;

/* ---- leg 1a: gData[0..999] ---- */
static const StShape stGdShape[] =
{
   { 10, 3.0,   10,  990 },   /* the defaults */
   {  7, 2.0,    7,  993 },
   { 14, 4.0,   14,  986 },
   { 10, 1.0,   10,  990 },
};
#define NB_ST_GD_SHAPE (sizeof(stGdShape)/sizeof(stGdShape[0]))

static const StGolden stGdOracle[] =
{
   { 10, 3.0,  900, 95.32237591850891,  1 },
   { 10, 3.0,  950, 92.15683632920022, -1 },
   { 10, 3.0,  975, 88.98797143766431,  1 },
   { 10, 3.0,  999, 92.86697406525593, -1 },

   {  7, 2.0,  900, 96.89865185113638,  1 },
   {  7, 2.0,  950, 88.2434271504695,   1 },
   {  7, 2.0,  975, 90.30155598413283,  1 },
   {  7, 2.0,  999, 91.03077602834395, -1 },

   { 14, 4.0,  900, 93.71338700335437,  1 },
   { 14, 4.0,  950, 93.59108694177152, -1 },
   { 14, 4.0,  975, 93.29265806991087, -1 },
   { 14, 4.0,  999, 93.29265806991087, -1 },

   { 10, 1.0,  900, 99.18786788662209, -1 },
   { 10, 1.0,  950, 89.53674610479116,  1 },
   { 10, 1.0,  975, 92.08932381255477,  1 },
   { 10, 1.0,  999, 88.94899135508531, -1 },
};
#define NB_ST_GD_ORACLE (sizeof(stGdOracle)/sizeof(stGdOracle[0]))

/* ---- leg 1b: TA_SREF, 252 bars ---- */
static const StShape stSrefShape[] =
{
   {  7, 2.0,    7,  245 },
   {  5, 1.5,    5,  247 },
};
#define NB_ST_SREF_SHAPE (sizeof(stSrefShape)/sizeof(stSrefShape[0]))

static const StGolden stSrefOracle[] =
{
   {  7, 2.0,  200, 114.35194013755326, -1 },
   {  7, 2.0,  225, 100.16118753507284,  1 },
   {  7, 2.0,  251, 113.53995615951584, -1 },

   {  5, 1.5,  200, 112.2539514483794,  -1 },
   {  5, 1.5,  225, 101.90280269605175,  1 },
   {  5, 1.5,  251, 111.78141670443482, -1 },
};
#define NB_ST_SREF_ORACLE (sizeof(stSrefOracle)/sizeof(stSrefOracle[0]))

/* ---- leg 1c: the whole TREND TRAJECTORY, from the same oracle ----
 *
 * The spot tables above pin 22 bars. This pins every bar of two of them: the
 * complete list of bars at which ta4j's SuperTrend changes direction over
 * gData[0..999]. The trend is an integer with no rounding, so the comparison is
 * exact and a single misplaced flip fails it -- and a flip is the one event that
 * depends on the seed, the ratchet and the band all at once.
 *
 * These two parameter sets and not others: at (7, 2.0) and (10, 1.0) ta4j and
 * TA_SUPERTREND agree on the trend at EVERY emitted bar, warm-up included, so
 * the list needs no start offset and no exemption. At (10, 3.0) and (14, 4.0)
 * they still differ inside the ATR seed transient (last at bars 36 and 48), and
 * freezing across that would bake the transient into the corpus. The capture
 * script refuses to emit a set it disagrees with rather than trimming one.
 */
static const int stFlips_7_2[] =
{
   28, 56, 75, 125, 148, 151, 165, 190, 205, 283,
   343, 348, 366, 392, 443, 489, 529, 580, 590, 593,
   647, 654, 687, 725, 731, 740, 747, 765, 787, 792,
   810, 830, 861, 871, 906, 949, 964, 970, 979, 988,
   992,
};

static const int stFlips_10_1[] =
{
   26, 56, 63, 65, 68, 90, 101, 123, 145, 151,
   155, 186, 194, 232, 239, 244, 249, 268, 274, 281,
   301, 305, 336, 340, 343, 347, 362, 378, 388, 391,
   410, 417, 439, 458, 465, 485, 495, 499, 506, 513,
   529, 546, 552, 560, 562, 577, 587, 591, 612, 615,
   636, 639, 646, 653, 672, 716, 717, 722, 729, 736,
   746, 764, 776, 790, 809, 825, 854, 864, 869, 871,
   898, 901, 905, 912, 931, 947, 951, 961, 962, 969,
   976, 986, 990, 996, 998,
};

typedef struct { int period; double mult; const int *bars; int nbBarsFlipped; } StFlips;

static const StFlips stFlipTable[] =
{
   {  7, 2.0, stFlips_7_2,  (int)(sizeof(stFlips_7_2)/sizeof(stFlips_7_2[0])) },
   { 10, 1.0, stFlips_10_1, (int)(sizeof(stFlips_10_1)/sizeof(stFlips_10_1[0])) },
};
#define NB_ST_FLIP_SET (sizeof(stFlipTable)/sizeof(stFlipTable[0]))

/* The parameter shapes legs 2-4 sweep. They straddle the multiplier's whole
 * useful span and both sides of the period's default, and the two smallest
 * periods put the seed bar as close to the start of the input as the parameter
 * range allows.
 */
typedef struct { int period; double mult; } StSweep;

static const StSweep stSweep[] =
{
   { 10, 3.0 },
   {  2, 3.0 },
   {  3, 0.5 },
   {  7, 2.0 },
   { 14, 4.0 },
   { 20, 1.0 },
   {  5, 0.0 },   /* the degenerate multiplier, pinned bitwise by leg 3 */
};
#define NB_ST_SWEEP (sizeof(stSweep)/sizeof(stSweep[0]))

/* Measured floors, read off an instrumented run. Every count here is a product
 * of the literal table and loop bounds in this file, so narrowing one of those
 * halves a count that an `== 0` check would sit through. Equality is not used:
 * a row added to a table above should not have to be paid for down here.
 */
#define ST_ORACLE_FLOOR  170      /* 22 rows x (line + trend), + 126 frozen flip bars */
#define ST_SEED_FLOOR     15      /* 7 shapes x 2 corpora, + the constructed upper-band case */
#define ST_DIFF_FLOOR  18517      /* differential bars compared, incl. the unstable leg */
#define ST_INV_FLOOR    8642      /* invariant bars checked */
/* The crossed-band count is a floor on a SKIP, not on a check: leg 3's geometric
 * assertion is guarded on the bands being ordered, so a sweep that never reached
 * a crossed bar would satisfy that guard everywhere and prove nothing about it.
 * 101 says the guarded branch is genuinely exercised on both sides. Only the
 * (3, 0.5) and (20, 1.0) shapes reach it -- do not drop either from stSweep. */
#define ST_CROSSED_FLOOR 649      /* bars whose carried bands had crossed */

/**** Local functions declarations. ****/
static ErrorNumber test_st_oracle_corpus( const char *corpusName,
                                          const TA_Real *high, const TA_Real *low,
                                          const TA_Real *close, int nbBars,
                                          const StShape *shape, unsigned int nbShape,
                                          const StGolden *gold, unsigned int nbGold,
                                          int *outNbCompared );
static ErrorNumber test_st_seed( const char *corpusName,
                                 const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars, int *outNbSeeds );
static ErrorNumber test_st_seed_upper( int *outNbSeeds );
static ErrorNumber test_st_flips( int *outNbCompared );
static ErrorNumber test_st_differential( const char *corpusName,
                                         const TA_Real *high, const TA_Real *low,
                                         const TA_Real *close, int nbBars,
                                         int *outNbBars, int *outNbCrossed );
static ErrorNumber test_st_invariants( const char *corpusName,
                                       const TA_Real *high, const TA_Real *low,
                                       const TA_Real *close, int nbBars,
                                       int *outNbBars );
static ErrorNumber test_st_unstable( int *outNbBars, int *outNbCrossed );
static ErrorNumber test_st_path_dependence( const TA_History *history );
static ErrorNumber test_st_degenerate( const TA_History *history );
static ErrorNumber st_rebuild_and_compare( const char *what, const char *corpusName,
                                           int period, double mult,
                                           const TA_Real *high, const TA_Real *low,
                                           const TA_Real *close, int nbBars,
                                           TA_Integer begIdx, TA_Integer nbElement,
                                           const TA_Real *line, const TA_Integer *trend,
                                           int *outNbBars, int *outNbCrossed );

/**** Global functions definitions. ****/
ErrorNumber test_func_supertrend( TA_History *history )
{
   ErrorNumber e;
   int nbCompared = 0, nbSeeds = 0, nbDiff = 0, nbInv = 0, nbCrossed = 0;
   int nbBars = (int)history->nbBars;

   e = test_st_oracle_corpus( "gData", gDataHigh, gDataLow, gDataClose, ST_GD_NB,
                              stGdShape, NB_ST_GD_SHAPE,
                              stGdOracle, NB_ST_GD_ORACLE, &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_oracle_corpus( "SREF", history->high, history->low, history->close,
                              nbBars, stSrefShape, NB_ST_SREF_SHAPE,
                              stSrefOracle, NB_ST_SREF_ORACLE, &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_flips( &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_seed( "gData", gDataHigh, gDataLow, gDataClose, ST_GD_NB, &nbSeeds );
   if( e != TA_TEST_PASS ) return e;
   e = test_st_seed( "SREF", history->high, history->low, history->close,
                     nbBars, &nbSeeds );
   if( e != TA_TEST_PASS ) return e;
   e = test_st_seed_upper( &nbSeeds );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_differential( "gData", gDataHigh, gDataLow, gDataClose, ST_GD_NB,
                             &nbDiff, &nbCrossed );
   if( e != TA_TEST_PASS ) return e;
   e = test_st_differential( "SREF", history->high, history->low, history->close,
                             nbBars, &nbDiff, &nbCrossed );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_invariants( "gData", gDataHigh, gDataLow, gDataClose, ST_GD_NB, &nbInv );
   if( e != TA_TEST_PASS ) return e;
   e = test_st_invariants( "SREF", history->high, history->low, history->close,
                           nbBars, &nbInv );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_unstable( &nbDiff, &nbCrossed );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_path_dependence( history );
   if( e != TA_TEST_PASS ) return e;

   e = test_st_degenerate( history );
   if( e != TA_TEST_PASS ) return e;

   if( nbCompared < ST_ORACLE_FLOOR || nbSeeds < ST_SEED_FLOOR ||
       nbDiff < ST_DIFF_FLOOR || nbInv < ST_INV_FLOOR ||
       nbCrossed < ST_CROSSED_FLOOR )
   {
      printf( "SUPERTREND coverage Fail: oracle %d (>= %d), seed %d (>= %d), "
              "differential %d (>= %d), invariant %d (>= %d), crossed-band %d (>= %d)\n",
              nbCompared, ST_ORACLE_FLOOR, nbSeeds, ST_SEED_FLOOR,
              nbDiff, ST_DIFF_FLOOR, nbInv, ST_INV_FLOOR,
              nbCrossed, ST_CROSSED_FLOOR );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Leg 1: shape, then spot values against ta4j, then cross-language. */
static ErrorNumber test_st_oracle_corpus( const char *corpusName,
                                          const TA_Real *high, const TA_Real *low,
                                          const TA_Real *close, int nbBars,
                                          const StShape *shape, unsigned int nbShape,
                                          const StGolden *gold, unsigned int nbGold,
                                          int *outNbCompared )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real line[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   unsigned int s, k;

   for( s = 0; s < nbShape; s++ )
   {
      int P = shape[s].period;
      double M = shape[s].mult;

      retCode = TA_SUPERTREND( 0, nbBars - 1, high, low, close, P, M,
                               &begIdx, &nbElement, line, trend );
      if( retCode != TA_SUCCESS )
      {
         printf( "SUPERTREND oracle Fail [%s %d %g]: retCode = %d\n",
                 corpusName, P, M, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* Shape before values: a function that quietly shortened its output would
       * otherwise pass every value check on what it did emit.
       */
      if( begIdx != shape[s].begIdx || nbElement != shape[s].nbElement )
      {
         printf( "SUPERTREND oracle Fail [%s %d %g]: shape got (%d,%d) expected (%d,%d)\n",
                 corpusName, P, M, begIdx, nbElement,
                 shape[s].begIdx, shape[s].nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < nbGold; k++ )
      {
         int idx;
         double err;
         const char *mode;

         if( gold[k].period != P || gold[k].mult != M )
            continue;

         idx = gold[k].bar - begIdx;
         if( idx < 0 || idx >= nbElement )
         {
            printf( "SUPERTREND oracle Fail [%s %d %g]: bar %d outside (%d,%d)\n",
                    corpusName, P, M, gold[k].bar, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         ( *outNbCompared )++;
         if( !checkOracleValue( line[idx], gold[k].line,
                                ST_ORACLE_TOL, ST_ORACLE_ABS, &err, &mode ) )
         {
            printf( "SUPERTREND oracle Fail [ta4j 0.22.6 %s %d %g] line at bar %d: "
                    "got %.17g expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                    corpusName, P, M, gold[k].bar,
                    line[idx], gold[k].line, mode, err, ST_ORACLE_TOL, ST_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         ( *outNbCompared )++;
         if( trend[idx] != gold[k].trend )
         {
            printf( "SUPERTREND oracle Fail [ta4j 0.22.6 %s %d %g] trend at bar %d: "
                    "got %d expected %d\n",
                    corpusName, P, M, gold[k].bar, (int)trend[idx], gold[k].trend );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      /* Cross-language: SUPERTREND must be bit-identical on every language
       * server, on both outputs. This is the first shipped function to hand
       * server_verify a real output and an integer one in the same call.
       */
      if( server_verify_active() )
      {
         double optIn[2];
         ErrorNumber e;

         optIn[0] = (double)P;
         optIn[1] = M;
         e = server_verify( "SUPERTREND", 0, nbBars - 1, nbBars,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 2,
                            (const TA_Real*[]){ line, NULL },
                            (const TA_Integer*[]){ trend, NULL } );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 1c: every trend change over gData, against the same oracle.
 *
 * A wrong seed moves the FIRST entry; a wrong ratchet or band moves a later one;
 * an off-by-one in the flip test moves all of them by a bar. None of that can
 * hide inside a tolerance, because there is none.
 */
static ErrorNumber test_st_flips( int *outNbCompared )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real line[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   unsigned int s;
   int i, k;

   for( s = 0; s < NB_ST_FLIP_SET; s++ )
   {
      int P = stFlipTable[s].period;
      double M = stFlipTable[s].mult;

      retCode = TA_SUPERTREND( 0, ST_GD_NB - 1, gDataHigh, gDataLow, gDataClose,
                               P, M, &begIdx, &nbElement, line, trend );
      if( retCode != TA_SUCCESS )
      {
         printf( "SUPERTREND flips Fail [%d %g]: rc=%d\n", P, M, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      k = 0;
      for( i = 1; i < nbElement; i++ )
      {
         if( trend[i] == trend[i-1] )
            continue;
         if( k >= stFlipTable[s].nbBarsFlipped )
         {
            printf( "SUPERTREND flips Fail [ta4j 0.22.6 gData %d %g]: flip at bar %d is "
                    "number %d, but the oracle records only %d\n",
                    P, M, begIdx + i, k + 1, stFlipTable[s].nbBarsFlipped );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         ( *outNbCompared )++;
         if( begIdx + i != stFlipTable[s].bars[k] )
         {
            printf( "SUPERTREND flips Fail [ta4j 0.22.6 gData %d %g]: flip %d at bar %d, "
                    "the oracle has it at %d\n",
                    P, M, k + 1, begIdx + i, stFlipTable[s].bars[k] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         k++;
      }
      if( k != stFlipTable[s].nbBarsFlipped )
      {
         printf( "SUPERTREND flips Fail [ta4j 0.22.6 gData %d %g]: %d flip(s), the oracle "
                 "has %d\n", P, M, k, stFlipTable[s].nbBarsFlipped );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 2: the seed bar, bitwise.
 *
 * The first emitted bar is the one place the recurrence has no history to lean
 * on, and it is the only place where an implementation is free to choose. Both
 * halves are pinned: the line is the UNCLAMPED lower band, which says the seed
 * took no ratchet from anywhere; and the trend is +1, which is the convention
 * itself. Assembled from TA_MEDPRICE and TA_ATR, so it also states that the
 * band is built on the median price and on Wilder's average true range and not
 * on some near neighbour of either.
 */
static ErrorNumber test_st_seed( const char *corpusName,
                                 const TA_Real *high, const TA_Real *low,
                                 const TA_Real *close, int nbBars, int *outNbSeeds )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, mpBeg, mpNb, atrBeg, atrNb;
   static TA_Real line[OUT_CAP], medprice[OUT_CAP], atr[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   unsigned int s;

   for( s = 0; s < NB_ST_SWEEP; s++ )
   {
      int P = stSweep[s].period;
      double M = stSweep[s].mult;
      double want;

      retCode = TA_SUPERTREND( 0, nbBars - 1, high, low, close, P, M,
                               &begIdx, &nbElement, line, trend );
      if( retCode != TA_SUCCESS || begIdx != TA_SUPERTREND_Lookback( P, M ) )
      {
         printf( "SUPERTREND seed Fail [%s %d %g]: rc=%d begIdx=%d expected %d\n",
                 corpusName, P, M, (int)retCode, begIdx,
                 TA_SUPERTREND_Lookback( P, M ) );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_MEDPRICE( begIdx, begIdx, high, low, &mpBeg, &mpNb, medprice );
      if( retCode != TA_SUCCESS || mpNb != 1 )
      {
         printf( "SUPERTREND seed Fail [%s %d %g]: MEDPRICE rc=%d nb=%d\n",
                 corpusName, P, M, (int)retCode, mpNb );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_ATR( begIdx, begIdx, high, low, close, P, &atrBeg, &atrNb, atr );
      if( retCode != TA_SUCCESS || atrBeg != begIdx || atrNb != 1 )
      {
         printf( "SUPERTREND seed Fail [%s %d %g]: ATR rc=%d (%d,%d) expected (%d,1)\n",
                 corpusName, P, M, (int)retCode, atrBeg, atrNb, begIdx );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      want = medprice[0] - M * atr[0];
      ( *outNbSeeds )++;
      if( line[0] != want || trend[0] != 1 )
      {
         printf( "SUPERTREND seed Fail [%s %d %g] at bar %d: got (%.17g,%d) "
                 "expected (%.17g,1) = MEDPRICE - mult*ATR\n",
                 corpusName, P, M, begIdx, line[0], (int)trend[0], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* The shared rebuild: the whole recurrence assembled here over shipped TA_ATR
 * and TA_MEDPRICE, compared bitwise against what the function emitted.
 *
 * The body carries its own copy of Wilder's recursion rather than calling
 * TA_ATR -- a whole-range intermediate between the band and the ratchet would
 * not stream -- so bit-exactness with TA_ATR is a claim the transcription makes
 * and not one its construction guarantees. This is where that claim is checked.
 *
 * It proves the composition and never the formula: the ratchet and the flip are
 * written twice, once in each place, so a wrong FORMULA would be wrong in both.
 * Leg 1 is what answers that.
 */
static ErrorNumber st_rebuild_and_compare( const char *what, const char *corpusName,
                                           int period, double mult,
                                           const TA_Real *high, const TA_Real *low,
                                           const TA_Real *close, int nbBars,
                                           TA_Integer begIdx, TA_Integer nbElement,
                                           const TA_Real *line, const TA_Integer *trend,
                                           int *outNbBars, int *outNbCrossed )
{
   TA_RetCode retCode;
   TA_Integer mpBeg, mpNb, atrBeg, atrNb;
   static TA_Real medprice[OUT_CAP], atr[OUT_CAP];
   double finalUpper, finalLower, prevClose, band, basicUpper, basicLower;
   int isUptrend, i, ordered, prevOrdered;

   retCode = TA_MEDPRICE( begIdx, nbBars - 1, high, low, &mpBeg, &mpNb, medprice );
   if( retCode != TA_SUCCESS || mpBeg != begIdx || mpNb != nbElement )
   {
      printf( "SUPERTREND %s Fail [%s %d %g]: MEDPRICE shape rc=%d (%d,%d)\n",
              what, corpusName, period, mult, (int)retCode, mpBeg, mpNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   retCode = TA_ATR( begIdx, nbBars - 1, high, low, close, period,
                     &atrBeg, &atrNb, atr );
   if( retCode != TA_SUCCESS || atrBeg != begIdx || atrNb != nbElement )
   {
      printf( "SUPERTREND %s Fail [%s %d %g]: ATR shape rc=%d (%d,%d) expected (%d,%d)\n",
              what, corpusName, period, mult, (int)retCode, atrBeg, atrNb,
              begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   band = mult * atr[0];
   finalUpper = medprice[0] + band;
   finalLower = medprice[0] - band;
   isUptrend = 1;
   prevClose = close[begIdx];
   prevOrdered = ( finalLower <= finalUpper );

   for( i = 0; i < nbElement; i++ )
   {
      double wantLine;
      int wantTrend;

      if( i > 0 )
      {
         double closeToday;

         band = mult * atr[i];
         basicUpper = medprice[i] + band;
         basicLower = medprice[i] - band;

         if( (basicUpper < finalUpper) || (prevClose > finalUpper) )
            finalUpper = basicUpper;
         if( (basicLower > finalLower) || (prevClose < finalLower) )
            finalLower = basicLower;

         closeToday = close[begIdx+i];
         if( isUptrend )
         {
            if( closeToday < finalLower )
               isUptrend = 0;
         }
         else
         {
            if( closeToday > finalUpper )
               isUptrend = 1;
         }
         prevClose = closeToday;
      }

      wantLine = isUptrend ? finalLower : finalUpper;
      wantTrend = isUptrend ? 1 : -1;

      ( *outNbBars )++;
      if( line[i] != wantLine || trend[i] != wantTrend )
      {
         printf( "SUPERTREND %s Fail [%s %d %g] bar %d: got (%.17g,%d) expected "
                 "(%.17g,%d) rebuilt over TA_ATR/TA_MEDPRICE\n",
                 what, corpusName, period, mult, begIdx + i,
                 line[i], (int)trend[i], wantLine, wantTrend );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* The support/resistance reading, checked where it is actually true.
       *
       * The two carried bands can CROSS -- each is released by its own condition,
       * so nothing keeps the lower one under the upper one -- and on a flip
       * through crossed bands the band the trend switches onto lands on the far
       * side of the close. Measured over 892k bars of this corpus at seven
       * periods x seven multipliers: with both this bar and the one before it
       * ordered, ZERO violations; without that guard, 31166. So the guard is the
       * statement, not an escape from it -- and `outNbCrossed` is what stops it
       * quietly swallowing the whole sweep.
       */
      ordered = ( finalLower <= finalUpper );
      if( !ordered )
         ( *outNbCrossed )++;
      if( ordered && prevOrdered )
      {
         double c = close[begIdx+i];
         if( (trend[i] ==  1 && c <  line[i]) ||
             (trend[i] == -1 && c >  line[i]) )
         {
            printf( "SUPERTREND %s Fail [%s %d %g] bar %d: trend %d with close %.17g on "
                    "the far side of the line %.17g, and the bands are not crossed "
                    "(%.17g <= %.17g)\n",
                    what, corpusName, period, mult, begIdx + i, (int)trend[i],
                    c, line[i], finalLower, finalUpper );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      prevOrdered = ordered;
   }

   return TA_TEST_PASS;
}

/* Leg 2b: the seed's OTHER band, which the corpora barely reach.
 *
 * The seed emits the lower band, so the upper one is only ever seen if it is
 * still being carried when the trend flips down -- and a seed upper band that is
 * too LOW is released by the very next bar (`prevClose > finalUpper` fires), so
 * it erases itself before anything can look at it. Mutation testing found
 * exactly that hole: seeding both bands on the lower one, or the upper one 10%
 * low, left every other leg in this file green.
 *
 * The series below closes it. Six quiet bars set a small Average True Range, then
 * one bar with an enormous range and a close at its bottom: the range lifts the
 * basic upper band above the carried one (so it is held, not released) while the
 * close punches through the lower band (so the trend flips down and the carried
 * upper band is what gets emitted). The second output bar is therefore the seed's
 * upper band, exactly.
 */
static ErrorNumber test_st_seed_upper( int *outNbSeeds )
{
   static const int NB = 12;
   static const int P = 5;
   static const double M = 2.0;
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, mpBeg, mpNb, atrBeg, atrNb;
   static TA_Real high[16], low[16], close[16];
   static TA_Real line[OUT_CAP], medprice[OUT_CAP], atr[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   double wantUpper;
   int i;

   for( i = 0; i < NB; i++ )
   {
      high[i]  = 150.5;
      low[i]   = 149.5;
      close[i] = 150.0;
   }
   high[P+1]  = 200.0;
   low[P+1]   = 100.0;
   close[P+1] = 101.0;

   retCode = TA_SUPERTREND( 0, NB - 1, high, low, close, P, M,
                            &begIdx, &nbElement, line, trend );
   if( retCode != TA_SUCCESS || begIdx != P || nbElement < 2 )
   {
      printf( "SUPERTREND seedUpper Fail: rc=%d (%d,%d) expected (%d,>=2)\n",
              (int)retCode, begIdx, nbElement, P );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_MEDPRICE( begIdx, begIdx, high, low, &mpBeg, &mpNb, medprice );
   if( retCode != TA_SUCCESS || mpNb != 1 )
   {
      printf( "SUPERTREND seedUpper Fail: MEDPRICE rc=%d nb=%d\n", (int)retCode, mpNb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   retCode = TA_ATR( begIdx, begIdx, high, low, close, P, &atrBeg, &atrNb, atr );
   if( retCode != TA_SUCCESS || atrBeg != begIdx || atrNb != 1 )
   {
      printf( "SUPERTREND seedUpper Fail: ATR rc=%d (%d,%d)\n",
              (int)retCode, atrBeg, atrNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   wantUpper = medprice[0] + M * atr[0];

   /* The construction is only worth anything if the trend really did flip: a
    * series that stayed long would pass the equality below on the lower band.
    */
   if( trend[0] != 1 || trend[1] != -1 )
   {
      printf( "SUPERTREND seedUpper Fail: trend went (%d,%d), expected (1,-1) -- the "
              "constructed flip did not happen, so the seed's upper band is untested\n",
              (int)trend[0], (int)trend[1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   ( *outNbSeeds )++;
   if( line[1] != wantUpper )
   {
      printf( "SUPERTREND seedUpper Fail at bar %d: got %.17g expected %.17g "
              "= MEDPRICE + mult*ATR carried from the seed bar\n",
              begIdx + 1, line[1], wantUpper );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* Leg 3: the rebuild above, on every emitted bar of both corpora, over the whole
 * parameter sweep. This is what pins the bars between leg 1's spot values.
 */
static ErrorNumber test_st_differential( const char *corpusName,
                                         const TA_Real *high, const TA_Real *low,
                                         const TA_Real *close, int nbBars,
                                         int *outNbBars, int *outNbCrossed )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real line[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   unsigned int s;

   for( s = 0; s < NB_ST_SWEEP; s++ )
   {
      ErrorNumber e;
      int P = stSweep[s].period;
      double M = stSweep[s].mult;

      retCode = TA_SUPERTREND( 0, nbBars - 1, high, low, close, P, M,
                               &begIdx, &nbElement, line, trend );
      if( retCode != TA_SUCCESS )
      {
         printf( "SUPERTREND differential Fail [%s %d %g]: rc=%d\n",
                 corpusName, P, M, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      e = st_rebuild_and_compare( "differential", corpusName, P, M,
                                  high, low, close, nbBars,
                                  begIdx, nbElement, line, trend,
                                  outNbBars, outNbCrossed );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}

/* Leg 4: the two things that are true of EVERY bar, unconditionally.
 *
 * Only these two. The geometric reading -- that the close is never on the far
 * side of the line -- is true but not unconditional, so it lives in the rebuild
 * helper, which knows where the carried bands have crossed. Two stronger claims
 * were asserted here and were simply WRONG: that the close is never on the far
 * side (31166 counterexamples), and that within one trend the line only tightens
 * toward price (10182). Both passed on these corpora at this sweep by luck. Do
 * not put either back without a corpus that reaches the crossed-band case.
 */
static ErrorNumber test_st_invariants( const char *corpusName,
                                       const TA_Real *high, const TA_Real *low,
                                       const TA_Real *close, int nbBars,
                                       int *outNbBars )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real line[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   unsigned int s;
   int i;

   for( s = 0; s < NB_ST_SWEEP; s++ )
   {
      int P = stSweep[s].period;
      double M = stSweep[s].mult;

      retCode = TA_SUPERTREND( 0, nbBars - 1, high, low, close, P, M,
                               &begIdx, &nbElement, line, trend );
      if( retCode != TA_SUCCESS )
      {
         printf( "SUPERTREND invariant Fail [%s %d %g]: rc=%d\n",
                 corpusName, P, M, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         ( *outNbBars )++;

         if( trend[i] != 1 && trend[i] != -1 )
         {
            printf( "SUPERTREND invariant Fail [%s %d %g] bar %d: trend %d is neither +1 nor -1\n",
                    corpusName, P, M, begIdx + i, (int)trend[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( !isfinite( line[i] ) )
         {
            printf( "SUPERTREND invariant Fail [%s %d %g] bar %d: line is not finite (%.17g)\n",
                    corpusName, P, M, begIdx + i, line[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* Leg 5: the Average True Range's unstable period, which is otherwise DEAD CODE.
 *
 * The lookback is TA_ATR's, so it moves with TA_FUNC_UNST_ATR, and the body has
 * a second warm-up loop that consumes exactly the bars that setting adds. At the
 * default of 0 that loop never runs a single iteration, and nothing else in this
 * suite or in the generic sweep sets the value -- SUPERTREND declares no unstable
 * period of its own, so it is outside UNSTABLE_MAP and outside the sweep's
 * unstable leg. Without this, the loop would ship having never executed.
 *
 * The setting is global, so it is restored on every path out.
 */
static ErrorNumber test_st_unstable( int *outNbBars, int *outNbCrossed )
{
   static const int periods[] = { 5, 14 };
   ErrorNumber e = TA_TEST_PASS;
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real line[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   double M = 3.0;
   unsigned int k, savedK;
   int pi;

   /* Restore what was there, not 0: this runs inside a suite whose other groups
    * set the same global, so writing a constant back would silently re-home the
    * setting for everything that follows. */
   savedK = TA_GetUnstablePeriod( TA_FUNC_UNST_ATR );

   for( k = 1; k <= 5 && e == TA_TEST_PASS; k++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, k );

      for( pi = 0; pi < (int)(sizeof(periods)/sizeof(periods[0])); pi++ )
      {
         int P = periods[pi];
         int lookback = TA_SUPERTREND_Lookback( P, M );

         if( lookback != P + (int)k )
         {
            printf( "SUPERTREND unstable Fail [%d %g] at unstable %u: lookback %d "
                    "expected %d -- it is not tracking TA_FUNC_UNST_ATR\n",
                    P, M, k, lookback, P + (int)k );
            e = TA_TESTUTIL_TFRR_BAD_CALCULATION;
            break;
         }

         retCode = TA_SUPERTREND( 0, ST_GD_NB - 1, gDataHigh, gDataLow, gDataClose,
                                  P, M, &begIdx, &nbElement, line, trend );
         if( retCode != TA_SUCCESS || begIdx != lookback )
         {
            printf( "SUPERTREND unstable Fail [%d %g] at unstable %u: rc=%d begIdx=%d "
                    "expected %d\n", P, M, k, (int)retCode, begIdx, lookback );
            e = TA_TESTUTIL_TFRR_BAD_BEGIDX;
            break;
         }

         e = st_rebuild_and_compare( "unstable", "gData", P, M,
                                     gDataHigh, gDataLow, gDataClose, ST_GD_NB,
                                     begIdx, nbElement, line, trend,
                                     outNbBars, outNbCrossed );
         if( e != TA_TEST_PASS )
            break;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ATR, savedK );
   return e;
}

/* Leg 6: the path dependence is EARNED.
 *
 * `path_dependent` costs real coverage -- it is what makes the generic range
 * sweep compare nothing at all for this function -- so it has to be paid for.
 * Three claims: the flag is actually published; a sub-range call really does
 * differ from the full-history one, so the flag is not decoration; and the
 * sub-range call is bit-identical to a full-history call over the truncated
 * input, so the range dependence is a re-anchor at startIdx and NOT the
 * function reading state it should not have.
 */
static ErrorNumber test_st_path_dependence( const TA_History *history )
{
   TA_RetCode retCode;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   TA_Integer fullBeg, fullNb, subBeg, subNb, shiftBeg, shiftNb;
   static TA_Real full[OUT_CAP], sub[OUT_CAP], shifted[OUT_CAP];
   static TA_Integer fullT[OUT_CAP], subT[OUT_CAP], shiftedT[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int P = 10, i, off, differing = 0;
   double M = 3.0;
   int startIdx = 120;
   int lookback = TA_SUPERTREND_Lookback( P, M );

   if( TA_GetFuncHandle( "SUPERTREND", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "SUPERTREND pathdep Fail: no ta_abstract entry\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( !(funcInfo->flags & TA_FUNC_FLG_PATH_DEP) )
   {
      printf( "SUPERTREND pathdep Fail: TA_FUNC_FLG_PATH_DEP is not published, so the "
              "range sweep would value-compare a function that re-seeds at startIdx\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   retCode = TA_SUPERTREND( 0, nbBars - 1, history->high, history->low, history->close,
                            P, M, &fullBeg, &fullNb, full, fullT );
   if( retCode != TA_SUCCESS )
   {
      printf( "SUPERTREND pathdep Fail: full-range rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_SUPERTREND( startIdx, nbBars - 1, history->high, history->low,
                            history->close, P, M, &subBeg, &subNb, sub, subT );
   if( retCode != TA_SUCCESS || subBeg != startIdx )
   {
      printf( "SUPERTREND pathdep Fail: sub-range rc=%d begIdx=%d expected %d\n",
              (int)retCode, subBeg, startIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < subNb; i++ )
   {
      int f = ( subBeg + i ) - fullBeg;
      if( sub[i] != full[f] || subT[i] != fullT[f] )
         differing++;
   }
   if( differing == 0 )
   {
      printf( "SUPERTREND pathdep Fail: a call anchored at bar %d matched the "
              "full-history one on all %d bars, so TA_FUNC_FLG_PATH_DEP is turning "
              "off the range gate for nothing\n", startIdx, subNb );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Same computation, viewed through a shorter input: the sub-range call reads
    * exactly `lookback` bars behind its anchor and nothing earlier.
    */
   off = startIdx - lookback;
   retCode = TA_SUPERTREND( 0, nbBars - 1 - off,
                            history->high + off, history->low + off, history->close + off,
                            P, M, &shiftBeg, &shiftNb, shifted, shiftedT );
   if( retCode != TA_SUCCESS || shiftBeg != lookback || shiftNb != subNb )
   {
      printf( "SUPERTREND pathdep Fail: truncated-input rc=%d (%d,%d) expected (%d,%d)\n",
              (int)retCode, shiftBeg, shiftNb, lookback, subNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < subNb; i++ )
   {
      if( sub[i] != shifted[i] || subT[i] != shiftedT[i] )
      {
         printf( "SUPERTREND pathdep Fail at bar %d: anchored (%.17g,%d) != truncated "
                 "(%.17g,%d) -- the sub-range call is reading past its own window\n",
                 subBeg + i, sub[i], (int)subT[i], shifted[i], (int)shiftedT[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 7: the two degenerate inputs the parameter range admits.
 *
 * A multiplier of 0 is legal, and there the band width vanishes: both basic
 * bands become the median price exactly, which is what makes this the check
 * that the centre is the MEDIAN price and not the typical price or the close.
 * The carried bands do not collapse with them -- each is still released by its
 * own condition -- so the emitted line is one of the two held median prices and
 * the trend still moves.
 *
 * A flat series drives the true range, and therefore the band, to exactly zero.
 * That is the input where an implementation that recovered the trend by
 * comparing the previous output against the previous bands would silently lose
 * its hysteresis; here it must simply hold.
 */
static ErrorNumber test_st_degenerate( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, mpBeg, mpNb;
   static TA_Real line[OUT_CAP], medprice[OUT_CAP];
   static TA_Integer trend[OUT_CAP];
   static TA_Real flatH[OUT_CAP], flatL[OUT_CAP], flatC[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int P = 10, i, moved = 0;

   retCode = TA_SUPERTREND( 0, nbBars - 1, history->high, history->low, history->close,
                            P, 0.0, &begIdx, &nbElement, line, trend );
   if( retCode != TA_SUCCESS )
   {
      printf( "SUPERTREND zeroMult Fail: rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_MEDPRICE( begIdx, nbBars - 1, history->high, history->low,
                          &mpBeg, &mpNb, medprice );
   if( retCode != TA_SUCCESS || mpBeg != begIdx || mpNb != nbElement )
   {
      printf( "SUPERTREND zeroMult Fail: MEDPRICE shape rc=%d (%d,%d)\n",
              (int)retCode, mpBeg, mpNb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( line[0] != medprice[0] )
   {
      printf( "SUPERTREND zeroMult Fail at bar %d: seed %.17g != MEDPRICE %.17g\n",
              begIdx, line[0], medprice[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < nbElement; i++ )
   {
      /* Every emitted value is a median price this series actually had: the
       * bands hold past values but never invent one.
       */
      int found = 0, j;
      for( j = 0; j <= i; j++ )
      {
         if( line[i] == medprice[j] )
         {
            found = 1;
            break;
         }
      }
      if( !found )
      {
         printf( "SUPERTREND zeroMult Fail at bar %d: %.17g is no median price at or "
                 "before it\n", begIdx + i, line[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( i > 0 && trend[i] != trend[i-1] )
         moved++;
   }
   if( moved == 0 )
   {
      printf( "SUPERTREND zeroMult Fail: the trend never flipped over %d bars with no "
              "band to cross\n", nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( i = 0; i < nbBars; i++ )
   {
      flatH[i] = 50.0;
      flatL[i] = 50.0;
      flatC[i] = 50.0;
   }
   retCode = TA_SUPERTREND( 0, nbBars - 1, flatH, flatL, flatC, P, 3.0,
                            &begIdx, &nbElement, line, trend );
   if( retCode != TA_SUCCESS )
   {
      printf( "SUPERTREND flat Fail: rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( line[i] != 50.0 || trend[i] != 1 )
      {
         printf( "SUPERTREND flat Fail at bar %d: got (%.17g,%d) expected (50,1) -- a "
                 "zero true range must collapse the band, not the trend\n",
                 begIdx + i, line[i], (int)trend[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
