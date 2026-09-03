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
 *  083126 MF,CC  First version (issue #273).
 */

/* Description:
 *
 *   Test TA_KC (Keltner Channels).
 *
 *   The --codegen sweep cannot VALUE-compare KC against the frozen ta_ref_serve,
 *   which predates this function, so the value comparison comes from this file
 *   plus server_verify / --xlang-hash. Two legs of that sweep do still run --
 *   the float leg and, more importantly, codegen_range_generic, which drives the
 *   in-process library and is the only automated gate that varies startIdx. Do
 *   not drop it for KC: the anchoring below is exactly what it watches.
 *
 *   Legs:
 *     1. EXTERNAL ORACLE (formula correctness) on the 1000-bar gData corpus,
 *        four parameter sets including the defaults. Plus cross-language.
 *     2. EXTERNAL ORACLE on the 252-bar TA_SREF corpus, two fast parameter
 *        sets that converge inside 252 bars (see the seeding note below).
 *     3. DIFFERENTIAL, bitwise, over four shapes: the centre line is TA_EMA of
 *        TA_TYPPRICE and the bands are middle +/- nbDev * TA_ATR over the same
 *        range. Pins every bar between the oracle's spot values.
 *     4. DIFFERENTIAL, bitwise: nbDev == 0 collapses all three outputs onto the
 *        centre line.
 *
 *   Legs 3 and 4 are built from shipped primitives, so they prove the
 *   composition, never the formula; that is legs 1 and 2' job.
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
#define OUT_CAP  1024   /* > KC_GD_NB and > history->nbBars */
#define KC_GD_NB 1000   /* prefix of the 10000-bar profiling corpus used by leg 1 */

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
 * ta4j is the arm chosen for issue #273 because it implements the same variant
 * TA_KC ships -- EMA(typical price, N) +/- nbDev * Wilder ATR(M), with N and M
 * independent -- which neither Keltner's 1960 original (SMA centre line, plain
 * daily-range band) nor the close-price StockCharts form does. TTR (R/CRAN)
 * implements the same variant and corroborates the shape, but ties M to N.
 *
 * SEEDING, and why the pinned bars are deep in the corpus. ta4j seeds both
 * recurrences with a RAW sample at the bar its warm-up ends; TA-Lib seeds each
 * with an SMA of the preceding window. For the EMA leg TA-Lib also starts one
 * bar earlier (lookback N-1 against ta4j's N); the Wilder leg starts on the same
 * bar (M both ways) and differs only in the seed VALUE. The two therefore never
 * agree during warm-up -- they converge. The gap decays geometrically, at
 * (1 - 2/(N+1)) per bar for the centre line and (1 - 1/M) for the band, and was
 * MEASURED over both corpora: at the defaults (20,10,2.0) it is 4.0e-2 relative
 * at the first emitted bar, 1.1e-5 by bar 100, and still 3.1e-12 at bar 251 --
 * which is why the 252-bar corpus cannot pin the default parameter set at all,
 * and leg 1 uses a 1000-bar corpus instead. Past convergence the residual is a
 * stable one-ulp arithmetic-path difference, not a transient: ta4j's Wilder
 * step is (v - prev)/M + prev, TA-Lib's is the fused wAlpha*tr + wBeta*prev,
 * which are algebraically equal and numerically not.
 *
 * That one-ulp floor is what the tolerance is sized against. Over every bar
 * from the first pinned bar to the end of its corpus, across all six parameter
 * sets and all three outputs, the worst measured relative disagreement is
 * 1.61e-16 (exactly 0.0 for four of the six sets). A 1e-12 relative bound
 * therefore carries ~6000x headroom, while remaining ~1e10 tighter than any of
 * the competing Keltner formulas, which differ by percent. Do NOT tighten this
 * to a bitwise comparison: the sets agree bit-for-bit at these particular bars
 * by luck, and differ by one ulp at others. See checkOracleValue().
 */
#define KC_ORACLE_TOL 1e-12
#define KC_ORACLE_ABS 1e-12

typedef struct { int emaPeriod; int atrPeriod; double nbDev;
                 int begIdx; int nbElement; } KcShape;

/* bar is the ABSOLUTE bar index; the output index is bar - begIdx. */
typedef struct { int emaPeriod; int atrPeriod; double nbDev; int bar;
                 double upper; double middle; double lower; } KcGolden;

/* ---- leg 1: gData[0..999] ---- */
static const KcShape kcGdShape[] =
{
   { 20, 10, 2.0, 19, 981 },   /* the defaults */
   { 10, 10, 1.0, 10, 990 },
   { 30, 20, 2.5, 29, 971 },
   {  4, 10, 2.0, 10, 990 },   /* ATR period > centre-line period */
};
#define NB_KC_GD_SHAPE (sizeof(kcGdShape)/sizeof(kcGdShape[0]))

static const KcGolden kcGdOracle[] =
{
   { 20, 10, 2.0, 950, 93.14428910975971, 90.46443209838381, 87.78457508700791 },
   { 20, 10, 2.0, 975, 93.47455410868363, 90.84851994483456, 88.2224857809855  },
   { 20, 10, 2.0, 999, 93.23710118192301, 89.31911847175239, 85.40113576158177 },

   { 10, 10, 1.0, 950, 91.41817012460059, 90.07824161891264, 88.73831311322469 },
   { 10, 10, 1.0, 975, 92.96195655084951, 91.64893946892498, 90.33592238700045 },
   { 10, 10, 1.0, 999, 90.60783471867227, 88.64884336358696, 86.68985200850166 },

   { 30, 20, 2.5, 995, 94.38089556174569, 90.07335892176341, 85.76582228178114 },
   { 30, 20, 2.5, 997, 94.27248925154484, 89.96824993396083, 85.66401061637683 },
   { 30, 20, 2.5, 999, 94.12469626538324, 89.63605778126367, 85.14741929714411 },

   {  4, 10, 2.0, 950, 92.99094850379039, 90.31109149241449, 87.63123448103859 },
   {  4, 10, 2.0, 975, 95.14514671453844, 92.51911255068937, 89.8930783868403  },
   {  4, 10, 2.0, 999, 91.803606798729,   87.88562408855839, 83.96764137838777 },
};
#define NB_KC_GD_ORACLE (sizeof(kcGdOracle)/sizeof(kcGdOracle[0]))

/* ---- leg 2: TA_SREF, 252 bars ---- */
static const KcShape kcSrefShape[] =
{
   { 8, 4, 1.5, 7, 245 },
   { 4, 4, 3.0, 4, 248 },
};
#define NB_KC_SREF_SHAPE (sizeof(kcSrefShape)/sizeof(kcSrefShape[0]))

static const KcGolden kcSrefOracle[] =
{
   { 8, 4, 1.5, 200, 115.64127560116764, 109.58860275540715, 103.53592990964667 },
   { 8, 4, 1.5, 225, 106.9941767789163,  100.77603770649361,  94.55789863407092 },
   { 8, 4, 1.5, 251, 112.60213497004759, 109.06516670756777, 105.52819844508795 },

   { 4, 4, 3.0, 200, 119.5676090131952,  107.46226332167424,  95.35691763015329 },
   { 4, 4, 3.0, 225, 116.34064574172066, 103.90436759687529,  91.46808945202991 },
   { 4, 4, 3.0, 251, 115.89326431782158, 108.81932779286193, 101.74539126790228 },
};
#define NB_KC_SREF_ORACLE (sizeof(kcSrefOracle)/sizeof(kcSrefOracle[0]))

/**** Local functions declarations. ****/
static ErrorNumber test_kc_oracle_corpus( const char *corpusName,
                                          const TA_Real *high, const TA_Real *low,
                                          const TA_Real *close, int nbBars,
                                          const KcShape *shape, unsigned int nbShape,
                                          const KcGolden *gold, unsigned int nbGold,
                                          int *outNbCompared );
static ErrorNumber test_kc_composition( const TA_History *history );
static ErrorNumber test_kc_zero_dev( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_kc( TA_History *history )
{
   ErrorNumber e;
   int nbCompared = 0;

   e = test_kc_oracle_corpus( "gData", gDataHigh, gDataLow, gDataClose, KC_GD_NB,
                              kcGdShape, NB_KC_GD_SHAPE,
                              kcGdOracle, NB_KC_GD_ORACLE, &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   e = test_kc_oracle_corpus( "SREF", history->high, history->low, history->close,
                              (int)history->nbBars,
                              kcSrefShape, NB_KC_SREF_SHAPE,
                              kcSrefOracle, NB_KC_SREF_ORACLE, &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   /* Non-vacuity: a typo in either parameter column would silently drop rows
    * and leave both legs green on fewer comparisons than were written. */
   if( nbCompared != (int)( 3 * ( NB_KC_GD_ORACLE + NB_KC_SREF_ORACLE ) ) )
   {
      printf( "KC oracle Fail: compared %d values, expected %d\n",
              nbCompared, (int)( 3 * ( NB_KC_GD_ORACLE + NB_KC_SREF_ORACLE ) ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   e = test_kc_composition( history );
   if( e != TA_TEST_PASS ) return e;

   e = test_kc_zero_dev( history );
   if( e != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* Legs 1 and 2: shape, then spot values against ta4j, then cross-language. */
static ErrorNumber test_kc_oracle_corpus( const char *corpusName,
                                          const TA_Real *high, const TA_Real *low,
                                          const TA_Real *close, int nbBars,
                                          const KcShape *shape, unsigned int nbShape,
                                          const KcGolden *gold, unsigned int nbGold,
                                          int *outNbCompared )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   unsigned int s, k;

   for( s = 0; s < nbShape; s++ )
   {
      int N = shape[s].emaPeriod;
      int M = shape[s].atrPeriod;
      double dev = shape[s].nbDev;

      retCode = TA_KC( 0, nbBars - 1, high, low, close, N, M, dev,
                       &begIdx, &nbElement, up, mid, lo );
      if( retCode != TA_SUCCESS )
      {
         printf( "KC oracle Fail [%s %d %d %g]: retCode = %d\n",
                 corpusName, N, M, dev, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* Shape before values: a function that quietly shortened its output would
       * otherwise pass every value check on what it did emit. */
      if( begIdx != shape[s].begIdx || nbElement != shape[s].nbElement )
      {
         printf( "KC oracle Fail [%s %d %d %g]: shape got (%d,%d) expected (%d,%d)\n",
                 corpusName, N, M, dev, begIdx, nbElement,
                 shape[s].begIdx, shape[s].nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < nbGold; k++ )
      {
         int idx;
         double err;
         const char *mode;
         double wants[3];
         const double *gots[3];
         double want, got;
         int band;

         if( gold[k].emaPeriod != N || gold[k].atrPeriod != M || gold[k].nbDev != dev )
            continue;

         idx = gold[k].bar - begIdx;
         if( idx < 0 || idx >= nbElement )
         {
            printf( "KC oracle Fail [%s %d %d %g]: bar %d outside (%d,%d)\n",
                    corpusName, N, M, dev, gold[k].bar, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         wants[0] = gold[k].upper;
         wants[1] = gold[k].middle;
         wants[2] = gold[k].lower;
         gots[0]  = up;
         gots[1]  = mid;
         gots[2]  = lo;

         for( band = 0; band < 3; band++ )
         {
            want = wants[band];
            got  = gots[band][idx];
            ( *outNbCompared )++;
            if( !checkOracleValue( got, want, KC_ORACLE_TOL, KC_ORACLE_ABS, &err, &mode ) )
            {
               printf( "KC oracle Fail [ta4j 0.22.6 %s %d %d %g] band %d at bar %d: "
                       "got %.17g expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                       corpusName, N, M, dev, band, gold[k].bar,
                       got, want, mode, err, KC_ORACLE_TOL, KC_ORACLE_ABS );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }

      /* Cross-language: KC must be bit-identical on every language server. */
      if( server_verify_active() )
      {
         double optIn[3];
         ErrorNumber e;

         optIn[0] = (double)N;
         optIn[1] = (double)M;
         optIn[2] = dev;
         e = server_verify( "KC", 0, nbBars - 1, nbBars,
                            retCode, begIdx, nbElement,
                            (const TA_Real*[]){ high, low, close, NULL },
                            optIn, 3,
                            (const TA_Real*[]){ up, mid, lo, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (3) DIFFERENTIAL, bitwise, on EVERY emitted bar and over four parameter shapes.
 *
 * Each leg is entered at its own lookback, so KC's centre line IS TA_EMA over the
 * typical price on this range and its band IS nbDev * TA_ATR on this range --
 * nothing over-warms the shorter leg, and the composition is checkable by calling
 * the two shipped functions plainly. A spot table cannot see a band that drifts
 * between the pinned bars, nor one built from the wrong ATR period.
 *
 * The shapes straddle both orderings of the two lookbacks (M > N-1 in (4,10) and
 * (4,4), M < N-1 in (20,10) and (8,4)), since which leg is the longer one is what
 * decides outBegIdx.
 */
typedef struct { int emaPeriod; int atrPeriod; double nbDev; } KcShapeSweep;

static const KcShapeSweep kcSweep[] =
{
   { 20, 10, 2.0 },
   {  8,  4, 1.5 },
   {  4, 10, 2.0 },
   {  4,  4, 3.0 },
};
#define NB_KC_SWEEP (sizeof(kcSweep)/sizeof(kcSweep[0]))

static ErrorNumber test_kc_composition( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, tpBeg, tpNb, emaBeg, emaNb, atrBeg, atrNb;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   static TA_Real tp[OUT_CAP], ema[OUT_CAP], atr[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int i;
   unsigned int s;

   retCode = TA_TYPPRICE( 0, nbBars - 1, history->high, history->low, history->close,
                          &tpBeg, &tpNb, tp );
   if( retCode != TA_SUCCESS || tpBeg != 0 )
   {
      printf( "KC composition Fail: TYPPRICE rc=%d begIdx=%d\n", (int)retCode, tpBeg );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( s = 0; s < NB_KC_SWEEP; s++ )
   {
      int N = kcSweep[s].emaPeriod;
      int M = kcSweep[s].atrPeriod;
      double dev = kcSweep[s].nbDev;
      int lookback = TA_KC_Lookback( N, M, dev );

      retCode = TA_KC( 0, nbBars - 1, history->high, history->low, history->close,
                       N, M, dev, &begIdx, &nbElement, up, mid, lo );
      if( retCode != TA_SUCCESS || begIdx != lookback )
      {
         printf( "KC composition Fail [%d %d %g]: rc=%d begIdx=%d expected %d\n",
                 N, M, dev, (int)retCode, begIdx, lookback );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      retCode = TA_EMA( begIdx, nbBars - 1, tp, N, &emaBeg, &emaNb, ema );
      if( retCode != TA_SUCCESS || emaBeg != begIdx || emaNb != nbElement )
      {
         printf( "KC composition Fail [%d %d %g]: EMA shape rc=%d (%d,%d) expected (%d,%d)\n",
                 N, M, dev, (int)retCode, emaBeg, emaNb, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      retCode = TA_ATR( begIdx, nbBars - 1,
                        history->high, history->low, history->close,
                        M, &atrBeg, &atrNb, atr );
      if( retCode != TA_SUCCESS || atrBeg != begIdx || atrNb != nbElement )
      {
         printf( "KC composition Fail [%d %d %g]: ATR shape rc=%d (%d,%d) expected (%d,%d)\n",
                 N, M, dev, (int)retCode, atrBeg, atrNb, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbElement; i++ )
      {
         /* Rebuild the bands the way the function does. Comparing up[i]-mid[i]
          * against the half-width instead would be wrong: mid + w re-rounds, so
          * (mid + w) - mid is not w. */
         double w = atr[i] * dev;

         if( mid[i] != ema[i] )
         {
            printf( "KC composition Fail [%d %d %g] bar %d: middle %.17g != EMA(TYPPRICE) %.17g\n",
                    N, M, dev, begIdx + i, mid[i], ema[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( up[i] != mid[i] + w )
         {
            printf( "KC composition Fail [%d %d %g] bar %d: upper %.17g != middle + nbDev*ATR %.17g\n",
                    N, M, dev, begIdx + i, up[i], mid[i] + w );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( lo[i] != mid[i] - w )
         {
            printf( "KC composition Fail [%d %d %g] bar %d: lower %.17g != middle - nbDev*ATR %.17g\n",
                    N, M, dev, begIdx + i, lo[i], mid[i] - w );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) nbDev == 0 collapses the channel: all three outputs are the centre line,
 * which must be TA_EMA of TA_TYPPRICE bit-for-bit. This is what pins the centre
 * line to the TYPICAL PRICE rather than the close -- the single difference
 * between the variant TA_KC ships and the widely charted StockCharts one.
 */
static ErrorNumber test_kc_zero_dev( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, tpBeg, tpNb, emaBeg, emaNb;
   static TA_Real up[OUT_CAP], mid[OUT_CAP], lo[OUT_CAP];
   static TA_Real tp[OUT_CAP], ema[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int N = 20, M = 10, i;

   retCode = TA_KC( 0, nbBars - 1, history->high, history->low, history->close,
                    N, M, 0.0, &begIdx, &nbElement, up, mid, lo );
   if( retCode != TA_SUCCESS )
   {
      printf( "KC zeroDev Fail: retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_TYPPRICE( 0, nbBars - 1, history->high, history->low, history->close,
                          &tpBeg, &tpNb, tp );
   if( retCode != TA_SUCCESS )
   {
      printf( "KC zeroDev Fail: TYPPRICE retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   retCode = TA_EMA( begIdx, nbBars - 1, tp, N, &emaBeg, &emaNb, ema );
   if( retCode != TA_SUCCESS || emaBeg != begIdx || emaNb != nbElement )
   {
      printf( "KC zeroDev Fail: EMA shape got rc=%d (%d,%d) expected (%d,%d)\n",
              (int)retCode, emaBeg, emaNb, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbElement; i++ )
   {
      if( mid[i] != ema[i] || up[i] != ema[i] || lo[i] != ema[i] )
      {
         printf( "KC zeroDev Fail at bar %d: (%.17g,%.17g,%.17g) != EMA(TYPPRICE) %.17g\n",
                 begIdx + i, up[i], mid[i], lo[i], ema[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
