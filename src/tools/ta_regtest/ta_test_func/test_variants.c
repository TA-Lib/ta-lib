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
 *  072126 MF,CC  First version (issue #137).
 */

/* Generic variant parity gate.
 *
 * Every TA function ships twice over:
 *
 *    TA_<N>              double inputs
 *    TA_S_<N>            float  inputs
 *
 * One contract binds them, and it is exact — no tolerance, no exemptions:
 *
 *  TA_S_ == TA_ ON WIDENED INPUTS.  Per PR #33 the single-precision variants
 *  differ only in the type of their input arrays; every value is widened to
 *  double at the point of first use and all arithmetic happens in double. Feed
 *  TA_S_ a float array and TA_ the exact same values widened back to double, and
 *  the outputs must be bit-identical.
 *
 * This gate found a live defect on its first run: TA_S_WMA at
 * optInTimePeriod==1 did a memmove of *sizeof(double)* out of a `const float*`.
 *
 * Before it, the contract was not checked generically: the abstract layer
 * (TA_CallFunc / ta_frame.c) only reaches the guarded double entry point, so
 * nothing swept the float one.
 *
 * The gate needs no oracle and no server — the two variants check each other.
 * It runs on a bare ./ta_regtest so the autotools dist-verification nightly
 * (which never passes --codegen) is covered too.
 *
 * PRECONDITIONS. Every vector this file generates satisfies:
 *      startIdx >= 0;  endIdx >= 0;  endIdx >= startIdx;
 *      every input pointer non-NULL;
 *      every optional parameter resolved to a concrete in-range value;
 *      every output pointer non-NULL and pairwise distinct.
 * startIdx < lookback is deliberately allowed: the clamp and the empty-range
 * early return are part of the shared body.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_variant_frame.h"
#include "fuzz_data.h"

/**** Local definitions. ****/

#define V_MAX_BARS   300   /* history is 252 bars; assert, never clamp */
#define V_MAX_INPUT    6   /* widest is OHLCV + one spare */
#define V_MAX_OUTPUT   3   /* BBANDS/MACD/MINMAXINDEX are the widest */
#define V_MAX_OPT      8   /* SAREXT */
#define V_MAX_CAND     7   /* candidate values probed per optional parameter */

/* Written past the end of every output buffer and re-checked after each call:
 * a variant that writes more elements than it reports is a defect the value
 * comparison alone would miss. */
#define V_CANARY_D  (-1.2345678901234567e-77)
#define V_CANARY_I  0x5A5AA5A5

/* Input regimes.
 *
 * The four-variant contracts hold for all data, but the BRANCHES that carry a
 * defect are data-selected: the divide-by-zero and TA_IS_ZERO guards are dead on
 * a realistic price series. Degenerate inputs are what reach them, and they are
 * a property of the data, not of any one indicator — so they belong here, once,
 * for all 167 functions, rather than being re-hand-written per function.
 *
 * The nine FUZZ_* shapes are reused verbatim from fuzz_data.h (the same
 * generator --fuzz-064 and --xlang-hash drive), which already covers flat bars
 * (FUZZ_CONSTANT), ties, extreme magnitudes, signed zeros and exact zero-sum
 * bars. Only the zero-volume regime is added here: every fuzz shape emits
 * `v[i] = 1000.0 + t`, so NO existing generator in the tree ever produces a zero
 * or all-zero volume window — which is exactly what the sumVol guards in
 * CMF/VWMA/MFI/AD/ADOSC/OBV/NVI/PVI branch on.
 *
 * Adding zero volume as a tenth FUZZ_* shape was deliberately avoided: the fuzz
 * shape list is iterated by --fuzz-064, whose oracle is a frozen v0.6.4 binary,
 * so a new shape silently changes what that gate compares. This list is local. */
typedef enum
{
   V_REG_HISTORY = 0,      /* the real 252-bar series (primary; full param sweep) */
   V_REG_FUZZ_FIRST,
   V_REG_ZERO_VOLUME = V_REG_FUZZ_FIRST + FUZZ_NSHAPES,
   V_REG_FLAT_ZERO_VOLUME, /* flat bars AND zero volume: both guards at once */
   V_NB_REGIME
} VariantRegime;

static const char *regime_name( int regime )
{
   static const char * const fuzzNames[FUZZ_NSHAPES] = {
      "randwalk", "mono-up", "mono-down", "constant", "tie-heavy",
      "extreme", "with-zeros", "candle", "zerosum" };
   if( regime == V_REG_HISTORY )          return "history";
   if( regime == V_REG_ZERO_VOLUME )      return "zero-volume";
   if( regime == V_REG_FLAT_ZERO_VOLUME ) return "flat+zero-volume";
   return fuzzNames[regime - V_REG_FUZZ_FIRST];
}

typedef struct
{
   /* Inputs. The float arrays are heap-allocated at exactly nb elements so an
    * over-read past the end (the TA_S_WMA defect class) is a genuine heap
    * over-read that ASan reports, rather than a quiet walk into the tail of an
    * oversized static array. */
   float  *fIn[V_MAX_INPUT];
   TA_Real dIn[V_MAX_INPUT][V_MAX_BARS];

   /* One output set per variant: double and single-precision. Distinct
    * allocations on purpose. */
   TA_Real    outD [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Real    outS [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outDi [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outSi [V_MAX_OUTPUT][V_MAX_BARS + 1];

   /* The current regime's OHLCV+OI series, staged once per regime and then
    * dealt out to each function's input slots by kind. */
   TA_Real srcOpen[V_MAX_BARS], srcHigh[V_MAX_BARS], srcLow[V_MAX_BARS];
   TA_Real srcClose[V_MAX_BARS], srcVolume[V_MAX_BARS], srcOI[V_MAX_BARS];

   int nb;
   int regime;

   /* Coverage counters — printed, and asserted non-zero so the gate can never
    * pass by doing nothing. */
   long nbFunc;
   long nbVector;
   long nbCompare;    /* variant comparisons actually performed */
   long nbValueCmp;   /* of those, ones that compared >0 output elements */
   long nbOutputCmp;  /* actual memcmp calls — incremented AT the comparison */
} VariantCtx;

/**** Local functions declarations. ****/

static void build_regime( VariantCtx *ctx, int regime, const TA_History *history );
static int  build_candidates( const TA_VOptSpec *spec, double *out );
static void set_canaries( VariantCtx *ctx, const TA_VariantEntry *e );
static int  canaries_intact( VariantCtx *ctx, const TA_VariantEntry *e );
static ErrorNumber run_one_vector( VariantCtx *ctx, const TA_VariantEntry *e,
                                   const double *optIn, int startIdx, int endIdx );
static ErrorNumber run_one_function( VariantCtx *ctx, const TA_VariantEntry *e );

/**** Global functions definitions.  ****/

ErrorNumber test_func_variants( TA_History *history )
{
   static VariantCtx ctx;   /* ~30 KB of buffers: static, not stack */
   ErrorNumber rc = TA_TEST_PASS;
   unsigned int t;
   int i, k, nb, regime;
   long nbDelegating = 0;

   if( history->nbBars > V_MAX_BARS )
   {
      /* Assert, never clamp: silently shrinking the sample would make the
       * whole gate weaker without saying so. */
      printf( "\nVariant gate Fail: history has %u bars, V_MAX_BARS is %d\n",
              history->nbBars, V_MAX_BARS );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   nb = (int)history->nbBars;
   ctx.nb = nb;
   ctx.nbFunc = ctx.nbVector = ctx.nbCompare = ctx.nbValueCmp = ctx.nbOutputCmp = 0;

   /* TA_EMA_Lookback reads the unstable period and TA_EMA_Private branches on
    * the compatibility mode; neutralise both so a leftover setting from an
    * earlier test group cannot colour these results. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   for( i = 0; i < V_MAX_INPUT; i++ )
   {
      ctx.fIn[i] = (float *)malloc( (size_t)nb * sizeof(float) );
      if( ctx.fIn[i] == NULL )
      {
         for( k = 0; k < i; k++ ) free( ctx.fIn[k] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   for( regime = 0; regime < V_NB_REGIME && rc == TA_TEST_PASS; regime++ )
   {
      build_regime( &ctx, regime, history );
      ctx.regime = regime;

      for( t = 0; t < TA_VARIANT_TABLE_SIZE && rc == TA_TEST_PASS; t++ )
      {
         const TA_VariantEntry *e = &TA_VariantTable[t];

         if( e->nbInput > V_MAX_INPUT || e->nbOutput > V_MAX_OUTPUT ||
             e->nbOptInput > V_MAX_OPT )
         {
            printf( "\nVariant gate Fail [TA_%s]: %d inputs / %d outputs / %d optIn "
                    "exceeds the gate's V_MAX_* bounds\n",
                    e->name, e->nbInput, e->nbOutput, e->nbOptInput );
            rc = TA_TESTUTIL_TFRR_BAD_PARAM;
            break;
         }

         /* Narrow every input series to float, then widen those same floats
          * back into the double arrays. Both legs then see numerically
          * identical values, so any difference between them is algorithmic
          * rather than a rounding artefact of the narrowing. Getting this
          * backwards produces a gate that fails everywhere and invites a
          * tolerance until it is vacuous. */
         for( i = 0; i < e->nbInput; i++ )
         {
            const TA_Real *src;
            switch( e->inputKind[i] )
            {
            case TA_VIN_OPEN:         src = ctx.srcOpen;   break;
            case TA_VIN_HIGH:         src = ctx.srcHigh;   break;
            case TA_VIN_LOW:          src = ctx.srcLow;    break;
            case TA_VIN_CLOSE:        src = ctx.srcClose;  break;
            case TA_VIN_VOLUME:       src = ctx.srcVolume; break;
            case TA_VIN_OPENINTEREST: src = ctx.srcOI;     break;
            case TA_VIN_PERIODS:      src = NULL;          break;
            case TA_VIN_REAL:
            default:
               /* Same convention as test_codegen.c: the first real input is the
                * close series, a second one is volume. */
               src = ( i == 0 ) ? ctx.srcClose : ctx.srcVolume;
               break;
            }
            for( k = 0; k < nb; k++ )
            {
               /* MAVP's per-element period series: prices here would drive
                * every period far out of range and make the function a no-op. */
               TA_Real v = ( src == NULL ) ? (TA_Real)( 2 + ( k % 25 ) ) : src[k];
               ctx.fIn[i][k] = (float)v;
               ctx.dIn[i][k] = (TA_Real)ctx.fIn[i][k];
            }
         }

         rc = run_one_function( &ctx, e );
         if( regime == V_REG_HISTORY )
         {
            ctx.nbFunc++;
            if( e->delegatesToPrivate )
               nbDelegating++;
         }
      }
   }

   for( i = 0; i < V_MAX_INPUT; i++ )
      free( ctx.fIn[i] );

   if( rc != TA_TEST_PASS )
      return rc;

   /* Non-vacuity. A gate that silently compared nothing is worse than no gate:
    * it reads as coverage. Fail on zero rather than printing a cheerful zero. */
   if( ctx.nbFunc == 0 || ctx.nbCompare == 0 || ctx.nbValueCmp == 0 ||
       ctx.nbOutputCmp == 0 )
   {
      printf( "\nVariant gate Fail: vacuous run (%ld functions, %ld comparisons, "
              "%ld of them with output elements, %ld output memcmp(s))\n",
              ctx.nbFunc, ctx.nbCompare, ctx.nbValueCmp, ctx.nbOutputCmp );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   printf( "\n  Variant parity: %ld functions x %ld vectors, %ld TA_/TA_S_ "
           "comparisons (%ld with output, %ld output memcmp(s)), bit-identical\n",
           ctx.nbFunc, ctx.nbVector, ctx.nbCompare, ctx.nbValueCmp, ctx.nbOutputCmp );
   (void)nbDelegating;

   return TA_TEST_PASS;
}

/* Targeted precision regression — issue #138.
 *
 * The generic gate above sweeps every CDL* across the history and nine fuzz
 * shapes and reports them bit-identical across both variants. That green is
 * partly hollow: TA_CANDLERANGE/TA_CANDLEAVERAGE reach the leaf macros in
 * ta_utility.h (TA_HIGHLOWRANGE etc.), which read their input arrays WITHOUT a
 * (double) cast. Inside a TA_S_ body those arrays are `const float[]`, so the
 * range term is evaluated in float and rounded before it widens into the double
 * accumulator — violating PR #33's "TA_S_ computes in double throughout".
 *
 * No realistic bar triggers it. Subtracting two floats within a factor of two is
 * exact (Sterbenz's lemma), so any bar whose high and low are close — every bar
 * in the fuzz corpus — produces zero float error, which is why the sweep above
 * sees nothing. It bites only when one operand sits below the other's float ULP,
 * a high/low ratio past ~2^24. This vector constructs exactly that and pins the
 * resulting integer flip, so the fix is regression-locked with a concrete case
 * rather than the comment it used to be.
 *
 * Construction (CDLDOJI, HighLow range, avgPeriod 10, factor 0.1):
 *   bars 0..8 : flat O=H=L=C=1000   -> range 0, exact in both
 *   bar 9     : H=2^30, L=31        -> true range 2^30-31 = 1073741793, but the
 *               float subtraction rounds UP to 2^30 (31 < half-ULP(2^30f) = 32),
 *               a +31 error the double path never makes. Windowed sum: double
 *               1073741793 vs float 1073741824.
 *   bar 10    : O=0, C=10737418     -> body 10737418 tuned into the 0.31-wide gap
 *               between the float threshold 10737418.24 and the double 10737417.93.
 * At bar 10 the double variant sees body > threshold (not a doji -> 0); the float
 * variant sees body <= threshold (doji -> 100). Pre-fix the two TA_S_ legs return
 * 100 while the two double legs return 0 and this test fails; post-fix all four
 * agree at 0.
 */
ErrorNumber test_candle_precision( TA_History *history )
{
   enum { NB = 11 };
   float  fO[NB], fH[NB], fL[NB], fC[NB];
   double dO[NB], dH[NB], dL[NB], dC[NB];
   int    outD[NB], outS[NB];
   int    begD = 0, nbD = 0, begS = 0, nbS = 0;
   TA_RetCode rcD, rcS, rcSet, rcRestore;
   int i;

   (void)history;

   /* Pin the settings this vector depends on, independent of any earlier test
    * group that may have left custom candle globals behind. HighLow is the range
    * type that reaches TA_HIGHLOWRANGE. */
   rcSet = TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, 10, 0.1 );
   if( rcSet != TA_SUCCESS )
   {
      printf( "\nCandle precision Fail: TA_SetCandleSettings rc=%d\n", (int)rcSet );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( i = 0; i < NB; i++ )
   {
      fO[i] = 1000.0f; fH[i] = 1000.0f; fL[i] = 1000.0f; fC[i] = 1000.0f;
   }
   /* bar 9: huge high, low below half the float ULP of the high */
   fH[9] = 1073741824.0f;   /* 2^30, exactly representable */
   fL[9] = 31.0f;
   /* bar 10: the evaluated bar; body sits in the float-vs-double threshold gap */
   fO[10] = 0.0f;
   fC[10] = 10737418.0f;    /* < 2^24, exactly representable */
   fH[10] = 10737418.0f;
   fL[10] = 0.0f;

   /* Widen the SAME float values into the double arrays, so any divergence is
    * algorithmic rather than a narrowing artefact — the identical convention the
    * generic gate uses above. */
   for( i = 0; i < NB; i++ )
   {
      dO[i] = (double)fO[i]; dH[i] = (double)fH[i];
      dL[i] = (double)fL[i]; dC[i] = (double)fC[i];
   }

   rcD = TA_CDLDOJI  ( 10, 10, dO, dH, dL, dC, &begD, &nbD, outD );
   rcS = TA_S_CDLDOJI( 10, 10, fO, fH, fL, fC, &begS, &nbS, outS );

   /* Restore before any early return so a failure here cannot leak custom candle
    * globals into the tests that follow. */
   rcRestore = TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );

   if( rcD != TA_SUCCESS || rcS != TA_SUCCESS || rcRestore != TA_SUCCESS )
   {
      printf( "\nCandle precision Fail [CDLDOJI]: retCode double=%d single=%d "
              "restore=%d\n", (int)rcD, (int)rcS, (int)rcRestore );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   if( begD != 10 || nbD != 1 || begS != 10 || nbS != 1 )
   {
      printf( "\nCandle precision Fail [CDLDOJI]: expected one output at index 10, got "
              "double(%d,%d) single(%d,%d)\n", begD, nbD, begS, nbS );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* The contract: both variants agree bit-for-bit. Pre-#138-fix the float leg
    * returns 100 (doji) and the double leg 0, because the leaf range term was
    * rounded to float in the TA_S_ body. */
   if( outS[0] != outD[0] )
   {
      printf( "\nCandle precision Fail [CDLDOJI issue #138]: variants disagree at bar 10"
              " -- double=%d TA_S_=%d. The single-precision body must compute the candle"
              " range in double, not float.\n", outD[0], outS[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   printf( "\n  Candle precision (issue #138): CDLDOJI TA_/TA_S_ parity on a high/low"
           " span past the float ULP -- bit-identical (out=%d)\n", outD[0] );

   return TA_TEST_PASS;
}

/**** Local functions definitions.  ****/

/* Stage one regime's OHLCV+OI series into ctx. */
static void build_regime( VariantCtx *ctx, int regime, const TA_History *history )
{
   int nb = ctx->nb, i;

   if( regime == V_REG_HISTORY )
   {
      for( i = 0; i < nb; i++ )
      {
         ctx->srcOpen[i]   = history->open[i];
         ctx->srcHigh[i]   = history->high[i];
         ctx->srcLow[i]    = history->low[i];
         ctx->srcClose[i]  = history->close[i];
         ctx->srcVolume[i] = history->volume[i];
         /* The test history carries no open-interest series (the field is
          * NULL). No shipped function takes that input today, but synthesize a
          * plausible one rather than leaving a NULL in the table: the first
          * function that does would otherwise crash here instead of failing. */
         ctx->srcOI[i]     = ( history->openInterest != NULL )
                                ? history->openInterest[i]
                                : 100.0 + (TA_Real)i;
      }
      return;
   }

   /* Everything else is generated. Zero-volume reuses realistic prices so the
    * volume guard is the only degeneracy under test; flat+zero-volume stacks
    * both (hh == ll AND sumVol == 0) because several functions guard on one and
    * then divide by the other. */
   {
      int shape = ( regime >= V_REG_FUZZ_FIRST && regime < V_REG_ZERO_VOLUME )
                     ? regime - V_REG_FUZZ_FIRST
                     : ( regime == V_REG_FLAT_ZERO_VOLUME ? FUZZ_CONSTANT
                                                          : FUZZ_RANDWALK );
      fuzz_gen( shape, /*seed*/ 7, nb, ctx->srcOpen, ctx->srcHigh, ctx->srcLow,
                ctx->srcClose, ctx->srcVolume, ctx->srcOI );

      if( regime == V_REG_ZERO_VOLUME || regime == V_REG_FLAT_ZERO_VOLUME )
         for( i = 0; i < nb; i++ )
            ctx->srcVolume[i] = 0.0;
   }
}

/* Candidate values to probe for one optional parameter, others held at their
 * default. Returns how many were written to `out` (always >= 1: the default).
 *
 * The bounds come straight from the YAML metadata, already resolved to concrete
 * numbers by the generator. The DEFAULT SENTINEL is probed too, appended after
 * the clamp so the range cannot drop it: both variants must substitute the
 * documented default and therefore agree. That leg is not decorative — it is the
 * only place in the suite where a sentinel meets the TA_S_ path, and it fails
 * today on TA_S_EMA, which computes its k factor from the raw sentinel before
 * the prologue substitutes it.
 *
 * For integer parameters the minimum matters most: it is where optInTimePeriod
 * == 1 lives, the passthrough branch that carried the TA_S_WMA defect. Values
 * beyond the sample size are kept too (both variants must agree that there is
 * nothing to output), but a usable interior value is always included so the
 * comparison is not left vacuous.
 *
 * Real parameters are only probed at finite, meaningful values: most declare
 * +/-DBL_MAX bounds, and driving those produces inf/NaN in every variant
 * alike — equal, but uninformative. */
static int build_candidates( const TA_VOptSpec *spec, double *out )
{
   double cand[V_MAX_CAND];
   int n = 0, i, j;

   cand[n++] = spec->defValue;

   if( !spec->isReal )
   {
      cand[n++] = spec->minValue;
      cand[n++] = spec->minValue + 1.0;
      cand[n++] = 14.0;
      cand[n++] = ( spec->maxValue < 60.0 ) ? spec->maxValue : 60.0;
      cand[n++] = spec->maxValue;
   }
   else
   {
      if( spec->minValue > -1.0e6 )
         cand[n++] = spec->minValue;
      if( spec->maxValue < 1.0e6 )
         cand[n++] = spec->maxValue;
      cand[n++] = spec->defValue * 0.5;
      cand[n++] = ( spec->defValue == 0.0 ) ? 1.0 : spec->defValue * 2.0;
   }

   /* Clamp into range, drop duplicates. */
   for( i = 0, j = 0; i < n; i++ )
   {
      double v = cand[i];
      int dup = 0, k;
      if( v < spec->minValue ) v = spec->minValue;
      if( v > spec->maxValue ) v = spec->maxValue;
      for( k = 0; k < j; k++ )
         if( out[k] == v ) { dup = 1; break; }
      if( !dup )
         out[j++] = v;
   }

   /* The default sentinel, appended AFTER the clamp: it is out of range by
    * construction, and the guarded prologue is what turns it into the declared
    * default. Both variants carry that prologue, so both must land on the same
    * value and produce the same output. TA_INTEGER_DEFAULT is INT_MIN, exactly
    * representable in double, so the frame's (int) cast round-trips it. */
   out[j++] = spec->isReal ? (double)TA_REAL_DEFAULT : (double)TA_INTEGER_DEFAULT;
   return j;
}

static void set_canaries( VariantCtx *ctx, const TA_VariantEntry *e )
{
   int i;
   for( i = 0; i < e->nbOutput; i++ )
   {
      if( e->outIsInteger )
      {
         ctx->outDi [i][ctx->nb] = V_CANARY_I;
         ctx->outSi [i][ctx->nb] = V_CANARY_I;
      }
      else
      {
         ctx->outD [i][ctx->nb] = V_CANARY_D;
         ctx->outS [i][ctx->nb] = V_CANARY_D;
      }
   }
}

static int canaries_intact( VariantCtx *ctx, const TA_VariantEntry *e )
{
   int i;
   for( i = 0; i < e->nbOutput; i++ )
   {
      if( e->outIsInteger )
      {
         if( ctx->outDi [i][ctx->nb] != V_CANARY_I ) return 0;
         if( ctx->outSi [i][ctx->nb] != V_CANARY_I ) return 0;
      }
      else
      {
         if( ctx->outD [i][ctx->nb] != V_CANARY_D ) return 0;
         if( ctx->outS [i][ctx->nb] != V_CANARY_D ) return 0;
      }
   }
   return 1;
}

/* Drive both variants at one (parameter vector, range) point and assert they
 * agree exactly. */
static ErrorNumber run_one_vector( VariantCtx *ctx, const TA_VariantEntry *e,
                                   const double *optIn, int startIdx, int endIdx )
{
   const double *dPtr[V_MAX_INPUT];
   const float  *fPtr[V_MAX_INPUT];
   double    *outRealD[V_MAX_OUTPUT], *outRealS[V_MAX_OUTPUT];
   int       *outIntD [V_MAX_OUTPUT], *outIntS [V_MAX_OUTPUT];
   TA_RetCode rcD, rcS;
   int begD = 0, nbD = 0, begS = 0, nbS = 0;
   int i;

   for( i = 0; i < e->nbInput; i++ )
   {
      dPtr[i] = ctx->dIn[i];
      fPtr[i] = ctx->fIn[i];
   }
   for( i = 0; i < e->nbOutput; i++ )
   {
      outRealD[i] = ctx->outD[i];   outRealS[i] = ctx->outS[i];
      outIntD [i] = ctx->outDi[i];  outIntS [i] = ctx->outSi[i];
   }

   set_canaries( ctx, e );

   /* A guarded rejection returns without touching *outBegIdx / *outNBElement,
    * so these must be pre-set or an unequal retCode would be compared against
    * uninitialised memory. */
   rcD = e->guarded( startIdx, endIdx, dPtr, optIn, &begD, &nbD, outRealD, outIntD );
   rcS = e->single ( startIdx, endIdx, fPtr, optIn, &begS, &nbS, outRealS, outIntS );

   ctx->nbCompare++;

   if( !canaries_intact( ctx, e ) )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: a variant wrote past "
              "the end of an output buffer\n", e->name, regime_name(ctx->regime), startIdx, endIdx );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( rcS != rcD )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: retCode guarded=%d "
              "TA_S_=%d\n",
              e->name, regime_name(ctx->regime), startIdx, endIdx, (int)rcD, (int)rcS );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( rcD != TA_SUCCESS )
      return TA_TEST_PASS;   /* agreed rejection — nothing to compare */

   if( begS != begD || nbS != nbD )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: shape guarded(%d,%d) "
              "TA_S_(%d,%d)\n",
              e->name, regime_name(ctx->regime), startIdx, endIdx, begD, nbD, begS, nbS );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( nbD == 0 )
      return TA_TEST_PASS;

   if( nbD < 0 || begD < 0 || nbD > ctx->nb )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: implausible shape "
              "(begIdx=%d nbElement=%d, nb=%d)\n",
              e->name, regime_name(ctx->regime), startIdx, endIdx, begD, nbD, ctx->nb );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   ctx->nbValueCmp++;

   for( i = 0; i < e->nbOutput; i++ )
   {
      size_t width = e->outIsInteger ? sizeof(TA_Integer) : sizeof(TA_Real);
      const void *a  = e->outIsInteger ? (const void *)ctx->outDi[i] : (const void *)ctx->outD[i];
      const void *c  = e->outIsInteger ? (const void *)ctx->outSi[i] : (const void *)ctx->outS[i];
      size_t len = (size_t)nbD * width;

      /* Counted HERE, at the memcmp itself, not at the call above: a counter
       * incremented before the comparisons and independently of them lets a
       * deleted memcmp leave the summary printing byte-identical numbers while
       * the gate checks strictly less. */
      ctx->nbOutputCmp++;

      if( memcmp( a, c, len ) != 0 )
      {
         printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d out%d]: "
                 "TA_S_%s differs from TA_%s on widened inputs -- TA_S_ must "
                 "compute in double throughout (PR #33)\n",
                 e->name, regime_name(ctx->regime), startIdx, endIdx, i, e->name, e->name );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* Sweep one function: the all-defaults vector, then each optional parameter
 * varied alone across its candidate values, each over several ranges. */
static ErrorNumber run_one_function( VariantCtx *ctx, const TA_VariantEntry *e )
{
   double optIn[V_MAX_OPT];
   double cand[V_MAX_CAND];
   ErrorNumber rc;
   int j, c, r, nbCand;
   int nb = ctx->nb;

   /* startIdx deliberately spans below and above any plausible lookback; both
    * variants must handle either, the clamp lives in the shared body. */
   const int ranges[][2] = { { 0, 0 }, { 37, 0 }, { 0, 60 }, { 200, 0 } };
   const int nbRange = (int)( sizeof(ranges) / sizeof(ranges[0]) );

   for( j = 0; j < e->nbOptInput; j++ )
      optIn[j] = e->optInput[j].defValue;

   for( r = 0; r < nbRange; r++ )
   {
      int startIdx = ranges[r][0];
      int endIdx   = ( ranges[r][1] == 0 ) ? nb - 1 : ranges[r][1];
      if( startIdx > endIdx || endIdx > nb - 1 )
         continue;
      ctx->nbVector++;
      rc = run_one_vector( ctx, e, optIn, startIdx, endIdx );
      if( rc != TA_TEST_PASS )
         return rc;
   }

   for( j = 0; j < e->nbOptInput; j++ )
   {
      double saved = optIn[j];
      nbCand = build_candidates( &e->optInput[j], cand );
      for( c = 0; c < nbCand; c++ )
      {
         if( cand[c] == saved )
            continue;              /* already covered by the defaults pass */
         optIn[j] = cand[c];
         for( r = 0; r < nbRange; r++ )
         {
            int startIdx = ranges[r][0];
            int endIdx   = ( ranges[r][1] == 0 ) ? nb - 1 : ranges[r][1];
            if( startIdx > endIdx || endIdx > nb - 1 )
               continue;
            ctx->nbVector++;
            rc = run_one_vector( ctx, e, optIn, startIdx, endIdx );
            if( rc != TA_TEST_PASS )
            {
               printf( "   (while sweeping %s = %.17g)\n",
                       e->optInput[j].name, cand[c] );
               return rc;
            }
         }
      }
      optIn[j] = saved;
   }

   return TA_TEST_PASS;
}
