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

/* Generic four-variant parity gate.
 *
 * Every TA function ships four times over:
 *
 *    TA_<N>              double inputs, parameter validation
 *    TA_<N>_Unguarded    double inputs, no validation
 *    TA_S_<N>            float  inputs, parameter validation
 *    TA_S_<N>_Unguarded  float  inputs, no validation
 *
 * Two contracts bind them, and both are exact — no tolerance, no exemptions:
 *
 *  (1) UNGUARDED == GUARDED.  The generator produces the unguarded variant by
 *      omitting a validation prologue and nothing else, so on arguments that
 *      would have passed validation the two must agree bit for bit.
 *
 *  (2) TA_S_ == TA_ ON WIDENED INPUTS.  Per PR #33 the single-precision
 *      variants differ only in the type of their input arrays; every value is
 *      widened to double at the point of first use and all arithmetic happens
 *      in double. Feed TA_S_ a float array and TA_ the exact same values
 *      widened back to double, and the outputs must be bit-identical.
 *
 * Before this gate neither contract was checked generically:
 *
 *  - The abstract layer (TA_CallFunc / ta_frame.c) only reaches the guarded
 *    double entry point, so nothing here could sweep the other three.
 *  - The --codegen legs reach unguarded only for functions the frozen
 *    ta_ref_serve knows (post-cutover additions are skipped), only through a
 *    server, and only at 1e-9 / 1e-6 tolerances.
 *  - The guarded TA_S_<N> was observed NOWHERE: the generated server calls
 *    TA_S_<N> and then TA_S_<N>_Unguarded into the same output buffer, so the
 *    guarded result is always overwritten before it is reported.
 *
 * That last hole is how a live defect survived: TA_S_WMA at optInTimePeriod==1
 * did a memmove of *sizeof(double)* out of a `const float*`.
 *
 * The gate needs no oracle and no server — the four variants check each other.
 * It runs on a bare ./ta_regtest so the autotools dist-verification nightly
 * (which never passes --codegen) is covered too.
 *
 * PRECONDITIONS. Calling an unguarded variant with arguments the guarded one
 * would have rejected is out of contract and reads out of bounds, so every
 * vector this file generates satisfies, by construction, the exact negation of
 * the checks the generator strips:
 *      startIdx >= 0;  endIdx >= 0;  endIdx >= startIdx;
 *      every input pointer non-NULL;
 *      every optional parameter resolved to a concrete in-range value
 *        (never a TA_INTEGER_DEFAULT / TA_REAL_DEFAULT sentinel — unguarded
 *         does not substitute defaults, that lives in the guarded prologue);
 *      every output pointer non-NULL and pairwise distinct.
 * startIdx < lookback is deliberately allowed: the clamp and the empty-range
 * early return live in the shared body, not in the stripped prologue.
 *
 * WHAT A GREEN ROW DOES NOT PROVE. A function whose guarded and unguarded
 * entry points both delegate to a single TA_<N>_Private body is structurally
 * incapable of failing contract (1) — contract (2) still bites. Those are
 * counted separately and reported as "delegating" rather than padding the pass
 * count. The flag is generated from the definition (an explicit <name>_private()
 * in ta_codegen/input/), not hand-maintained here, so it cannot go stale.
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
#define V_MAX_CAND     6   /* candidate values probed per optional parameter */

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

   /* One output set per variant: guarded-double, unguarded-double,
    * single-guarded, single-unguarded. Distinct allocations on purpose. */
   TA_Real    outD [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Real    outDU[V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Real    outS [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Real    outSU[V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outDi [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outDUi[V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outSi [V_MAX_OUTPUT][V_MAX_BARS + 1];
   TA_Integer outSUi[V_MAX_OUTPUT][V_MAX_BARS + 1];

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
   long nbCompare;    /* four-variant comparisons actually performed */
   long nbValueCmp;   /* of those, ones that compared >0 output elements */
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
   ctx.nbFunc = ctx.nbVector = ctx.nbCompare = ctx.nbValueCmp = 0;

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
   if( ctx.nbFunc == 0 || ctx.nbCompare == 0 || ctx.nbValueCmp == 0 )
   {
      printf( "\nVariant gate Fail: vacuous run (%ld functions, %ld comparisons, "
              "%ld of them with output elements)\n",
              ctx.nbFunc, ctx.nbCompare, ctx.nbValueCmp );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   printf( "\n  Variant parity: %ld functions x %ld vectors, %ld four-way "
           "comparisons (%ld with output), bit-identical\n",
           ctx.nbFunc, ctx.nbVector, ctx.nbCompare, ctx.nbValueCmp );
   if( nbDelegating > 0 )
      printf( "    (%ld function(s) delegate guarded+unguarded to a shared "
              "_Private body: unguarded parity is structural for them)\n",
              nbDelegating );

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
 * The bounds come straight from the YAML metadata, already resolved to
 * concrete numbers by the generator — never a sentinel.
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
         ctx->outDUi[i][ctx->nb] = V_CANARY_I;
         ctx->outSi [i][ctx->nb] = V_CANARY_I;
         ctx->outSUi[i][ctx->nb] = V_CANARY_I;
      }
      else
      {
         ctx->outD [i][ctx->nb] = V_CANARY_D;
         ctx->outDU[i][ctx->nb] = V_CANARY_D;
         ctx->outS [i][ctx->nb] = V_CANARY_D;
         ctx->outSU[i][ctx->nb] = V_CANARY_D;
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
         if( ctx->outDUi[i][ctx->nb] != V_CANARY_I ) return 0;
         if( ctx->outSi [i][ctx->nb] != V_CANARY_I ) return 0;
         if( ctx->outSUi[i][ctx->nb] != V_CANARY_I ) return 0;
      }
      else
      {
         if( ctx->outD [i][ctx->nb] != V_CANARY_D ) return 0;
         if( ctx->outDU[i][ctx->nb] != V_CANARY_D ) return 0;
         if( ctx->outS [i][ctx->nb] != V_CANARY_D ) return 0;
         if( ctx->outSU[i][ctx->nb] != V_CANARY_D ) return 0;
      }
   }
   return 1;
}

/* Drive all four variants at one (parameter vector, range) point and assert
 * they agree exactly. */
static ErrorNumber run_one_vector( VariantCtx *ctx, const TA_VariantEntry *e,
                                   const double *optIn, int startIdx, int endIdx )
{
   const double *dPtr[V_MAX_INPUT];
   const float  *fPtr[V_MAX_INPUT];
   double    *outRealD[V_MAX_OUTPUT], *outRealDU[V_MAX_OUTPUT];
   double    *outRealS[V_MAX_OUTPUT], *outRealSU[V_MAX_OUTPUT];
   int       *outIntD [V_MAX_OUTPUT], *outIntDU [V_MAX_OUTPUT];
   int       *outIntS [V_MAX_OUTPUT], *outIntSU [V_MAX_OUTPUT];
   TA_RetCode rcD, rcDU, rcS, rcSU;
   int begD = 0, nbD = 0, begDU = 0, nbDU = 0, begS = 0, nbS = 0, begSU = 0, nbSU = 0;
   int i;

   for( i = 0; i < e->nbInput; i++ )
   {
      dPtr[i] = ctx->dIn[i];
      fPtr[i] = ctx->fIn[i];
   }
   for( i = 0; i < e->nbOutput; i++ )
   {
      outRealD [i] = ctx->outD [i];  outRealDU[i] = ctx->outDU[i];
      outRealS [i] = ctx->outS [i];  outRealSU[i] = ctx->outSU[i];
      outIntD  [i] = ctx->outDi [i]; outIntDU [i] = ctx->outDUi[i];
      outIntS  [i] = ctx->outSi [i]; outIntSU [i] = ctx->outSUi[i];
   }

   set_canaries( ctx, e );

   /* A guarded rejection returns without touching *outBegIdx / *outNBElement,
    * so these must be pre-set or an unequal retCode would be compared against
    * uninitialised memory. */
   rcD  = e->guarded        ( startIdx, endIdx, dPtr, optIn, &begD,  &nbD,  outRealD,  outIntD  );
   rcDU = e->unguarded      ( startIdx, endIdx, dPtr, optIn, &begDU, &nbDU, outRealDU, outIntDU );
   rcS  = e->single         ( startIdx, endIdx, fPtr, optIn, &begS,  &nbS,  outRealS,  outIntS  );
   rcSU = e->singleUnguarded( startIdx, endIdx, fPtr, optIn, &begSU, &nbSU, outRealSU, outIntSU );

   ctx->nbCompare++;

   if( !canaries_intact( ctx, e ) )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: a variant wrote past "
              "the end of an output buffer\n", e->name, regime_name(ctx->regime), startIdx, endIdx );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( rcDU != rcD || rcS != rcD || rcSU != rcD )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: retCode guarded=%d "
              "unguarded=%d TA_S_=%d TA_S_unguarded=%d\n",
              e->name, regime_name(ctx->regime), startIdx, endIdx, (int)rcD, (int)rcDU, (int)rcS, (int)rcSU );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( rcD != TA_SUCCESS )
      return TA_TEST_PASS;   /* agreed rejection — nothing to compare */

   if( begDU != begD || begS != begD || begSU != begD ||
       nbDU  != nbD  || nbS  != nbD  || nbSU  != nbD )
   {
      printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d]: shape guarded(%d,%d) "
              "unguarded(%d,%d) TA_S_(%d,%d) TA_S_unguarded(%d,%d)\n",
              e->name, regime_name(ctx->regime), startIdx, endIdx, begD, nbD, begDU, nbDU, begS, nbS, begSU, nbSU );
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
      const void *a  = e->outIsInteger ? (const void *)ctx->outDi [i] : (const void *)ctx->outD [i];
      const void *b  = e->outIsInteger ? (const void *)ctx->outDUi[i] : (const void *)ctx->outDU[i];
      const void *c  = e->outIsInteger ? (const void *)ctx->outSi [i] : (const void *)ctx->outS [i];
      const void *d  = e->outIsInteger ? (const void *)ctx->outSUi[i] : (const void *)ctx->outSU[i];
      size_t len = (size_t)nbD * width;

      if( memcmp( a, b, len ) != 0 )
      {
         printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d out%d]: "
                 "TA_%s_Unguarded differs from TA_%s -- the unguarded variant "
                 "must be bit-identical on already-valid arguments\n",
                 e->name, regime_name(ctx->regime), startIdx, endIdx, i, e->name, e->name );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( memcmp( a, c, len ) != 0 )
      {
         printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d out%d]: "
                 "TA_S_%s differs from TA_%s on widened inputs -- TA_S_ must "
                 "compute in double throughout (PR #33)\n",
                 e->name, regime_name(ctx->regime), startIdx, endIdx, i, e->name, e->name );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( memcmp( a, d, len ) != 0 )
      {
         printf( "\nVariant gate Fail [TA_%s %s start=%d end=%d out%d]: "
                 "TA_S_%s_Unguarded differs from TA_%s\n",
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

   /* startIdx deliberately spans below and above any plausible lookback; the
    * unguarded variants must handle both, the clamp lives in the shared body. */
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
