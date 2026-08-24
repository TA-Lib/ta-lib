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
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  082326 MF,CC Created. Issue #253.
 */

/* Description:
 *
 *   The quote unit is a choice of the exchange, not a property of the market:
 *   the same instrument priced in dollars, in cents or in BTC is the same
 *   instrument. So multiplying every price by a constant must not change what
 *   an indicator says -- an oscillator must return the SAME number, and a
 *   price-valued output must return the same number times that constant.
 *
 *   This gate makes that exact rather than approximate. The constant is a
 *   POWER OF TWO, and FP addition, subtraction, multiplication, division,
 *   sqrt, min/max and every comparison are exactly equivariant under a
 *   power-of-two rescale: no rounding decision anywhere in a homogeneous body
 *   can change. The outputs must therefore come back BIT-IDENTICAL after
 *   dividing out the scale factor, at any exponent that avoids overflow and
 *   the denormal floor.
 *
 *   That leaves exactly one way for a function to fail: an absolute constant
 *   meeting a quantity that carries the quote unit. Which is issue #253 --
 *   `TA_IS_ZERO`'s fixed 1e-14 sitting on a price, a range, a money flow or a
 *   sum of those. 20 functions failed this sweep when it was written
 *   (ACCBANDS ADX ADXR BETA BOP CCI CMO CMOU DX KAMA MINUS_DI NATR PLUS_DI PPO
 *   PVO RSI SMI STOCH STOCHF STOCHRSI, plus PVO again on the volume leg), and
 *   #243 (STDDEV/BBANDS) and #244 (MFI) would have failed it before their own
 *   fixes. It needs no oracle and no reference values: the invariant is
 *   internal, so it cannot go stale the way a frozen expected value does.
 *
 *   Legs:
 *     1. HOMOGENEITY, corpus-wide and exact. Every function, over three series
 *        and a ladder of exponents, on the price axis and on the volume axis.
 *        The degree (0 for an oscillator, 1 for a price, 2 for a variance) is
 *        MEASURED from a x2 and a x4 probe rather than declared, so the table
 *        cannot drift away from the code.
 *     3. THE VALUE IT IS INVARIANT AT. Leg 1 proves the output does not depend
 *        on the quote unit; it cannot say whether the value it settles on is
 *        the right one, and before #253 the answer at these ticks was 0 --
 *        perfectly scale-invariant nonsense. So every fixed function carries a
 *        value from OUTSIDE this library, on the suite's own committed history,
 *        checked at a tick where the old code returned 0.
 *     2. RANGE, for the bounded oscillators, at every rung of the same ladder.
 *        Leg 1 cannot see a 0/0 that divides rounding residue into itself --
 *        residue rescales exactly like everything else -- and that is the trap
 *        #244 fell into when it relaxed MFI's guard. A window that empties
 *        while the sums stay finite is exactly what the "halt" series below
 *        produces, and a blown-up ratio leaves the documented range by orders
 *        of magnitude.
 *
 *   Exemptions, all of them things that are not homogeneous in the first
 *   place: the Math Transform group (a price is not a legal argument to sin,
 *   log or exp, and sqrt is degree one HALF), and LINEARREG_ANGLE, whose
 *   output is an arctangent of a slope. Every other function in the library is
 *   swept. The exempt names are checked to still exist, so the list cannot rot
 *   into a silent skip.
 *
 *   PPO and PVO are a THIRD thing -- not exempt, and not passing: a known open
 *   defect, listed and asserted to still fail. See quKnownOpen below for why
 *   the fix is not simply the one the other nineteen took.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_test_reference.h"
#include "ta_utility.h"

/**** Local declarations. ****/

#define QU_NB_BARS     252   /* Same length as the suite's history. */
#define QU_MAX_OUTPUT  8
#define QU_NB_SERIES   3
#define QU_FLAT_TAIL   60    /* Bars of the "halt" series that do not move. */

/* The ladder. Exponents of two, spanning both directions far enough that a
 * fixed 1e-14 band is crossed on a normally quoted series, but not so far that
 * a degree-2 output (a variance) overflows or falls into the denormals: at
 * 2^120 a variance of the base series is ~1e40, at 2^-120 it is ~1e-70. Both
 * comfortably normal. Every element that would leave the normal range anyway
 * is skipped and counted, so an accidental overflow cannot pass as a match. */
static const int quScaleExp[] = { -120, -60, -30, 30, 60, 120 };
#define QU_NB_SCALE ((int)(sizeof(quScaleExp)/sizeof(quScaleExp[0])))

/* Which inputs the leg multiplies. A function can be homogeneous in the prices
 * and in the volumes with DIFFERENT degrees (AD is degree 0 in price and 1 in
 * volume), so the two axes are swept, and measured, separately. */
typedef enum { QU_AXIS_PRICE = 0, QU_AXIS_VOLUME = 1 } QuAxis;

/* Which parameters the sweep runs at. The defaults reach the body every caller
 * reaches; the minimum of every integer range reaches the period-conditional
 * arms, which are where several of these guards actually live (+DI/-DI keep a
 * separate no-smoothing branch at period 1, NATR another). A function whose
 * minima are mutually inconsistent -- a fast period that must stay below a slow
 * one -- simply declines the second setting; the count of functions that took
 * it is reported so the pass cannot hollow out unnoticed. */
typedef enum { QU_PARAMS_DEFAULT = 0, QU_PARAMS_MIN = 1 } QuParams;
#define QU_NB_PARAMS 2

typedef struct
{
   const char *name;
   TA_Real open[QU_NB_BARS];
   TA_Real high[QU_NB_BARS];
   TA_Real low[QU_NB_BARS];
   TA_Real close[QU_NB_BARS];
   TA_Real volume[QU_NB_BARS];
} QuSeries;

static QuSeries quSeries[QU_NB_SERIES];

/* The staged (scaled) copy actually handed to the function, and MAVP's periods
 * input, which is a count of bars and therefore never scaled. */
static TA_Real quOpen[QU_NB_BARS], quHigh[QU_NB_BARS], quLow[QU_NB_BARS];
static TA_Real quClose[QU_NB_BARS], quVolume[QU_NB_BARS], quPeriods[QU_NB_BARS];

static TA_Real    quOutBase[QU_MAX_OUTPUT][QU_NB_BARS];
static TA_Real    quOutScaled[QU_MAX_OUTPUT][QU_NB_BARS];
static TA_Integer quIntBase[QU_MAX_OUTPUT][QU_NB_BARS];
static TA_Integer quIntScaled[QU_MAX_OUTPUT][QU_NB_BARS];

/* Not homogeneous, and not a defect. */
static const char * const quExempt[] = { "LINEARREG_ANGLE" };
#define QU_NB_EXEMPT ((int)(sizeof(quExempt)/sizeof(quExempt[0])))
static int quExemptSeen[QU_NB_EXEMPT];

/* KNOWN OPEN, not exempt (issue #253). Both fail this sweep, deliberately.
 *
 * Each divides by a moving average of its own input and still guards that
 * divide with the fixed TA_IS_ZERO band. The band sits on a quantity of the
 * SAME order as the input, so it only fires on an input that is itself ~1e-14
 * -- unlike the rest of the family, which guarded something decades below its
 * input (STOCHF's range/100 died at a price of 1e-8).
 *
 * Replacing it with an exact test is NOT safe. SMA, WMA, TRIMA and HMA maintain
 * that average with a sliding sum, so on a window whose true mean is zero the
 * divisor is rounding residue -- 1e-16 relative to the input at every magnitude
 * -- and dividing residue by residue puts PVO at +694 and -102. The band masks
 * exactly the inputs below magnitude ~10; at 1e2 and above that is already the
 * shipped behaviour. The exact test would trade a defect needing a price below
 * 1e-14 for one needing a volume below 10.
 *
 * So the band stays, and so do these entries. The sweep ASSERTS both still
 * fail, so the pair cannot rot into a silent pass.
 */
static const char * const quKnownOpen[] = { "PPO", "PVO" };
#define QU_NB_KNOWN_OPEN ((int)(sizeof(quKnownOpen)/sizeof(quKnownOpen[0])))
static int quKnownOpenFailed[QU_NB_KNOWN_OPEN];

/* Documented bounds of the oscillators this gate can range-check. Kept to
 * outputs whose bound is a THEOREM about the body (a ratio of a part to a
 * whole, both non-negative), not a habit of the data -- so a violation is a
 * real defect rather than an unusual bar. The slack absorbs the last-ulp
 * overshoot a scale-then-divide can produce (CMOU emits 100*(Su-Sd) before
 * dividing by Su+Sd); what this leg hunts is the residue blow-up of a 0/0
 * relaxed without a structural emptiness test, which misses by orders of
 * magnitude, not by an ulp. */
#define QU_RANGE_SLACK 1.0e-9
static const struct { const char *name; unsigned int out; double lo, hi; } quRange[] = {
   { "ADX",      0,    0.0, 100.0 },
   { "ADXR",     0,    0.0, 100.0 },
   { "BOP",      0,   -1.0,   1.0 },
   { "CMO",      0, -100.0, 100.0 },
   { "CMOU",     0, -100.0, 100.0 },
   { "DX",       0,    0.0, 100.0 },
   { "MFI",      0,    0.0, 100.0 },
   { "MINUS_DI", 0,    0.0, 100.0 },
   { "PLUS_DI",  0,    0.0, 100.0 },
   { "RSI",      0,    0.0, 100.0 },
   { "SMI",      0, -100.0, 100.0 },
   { "SMI",      1, -100.0, 100.0 },
   { "STOCH",    0,    0.0, 100.0 },
   { "STOCH",    1,    0.0, 100.0 },
   { "STOCHF",   0,    0.0, 100.0 },
   { "STOCHF",   1,    0.0, 100.0 },
   { "STOCHRSI", 0,    0.0, 100.0 },
   { "STOCHRSI", 1,    0.0, 100.0 },
   { "ULTOSC",   0,    0.0, 100.0 }
};
#define QU_NB_RANGE ((int)(sizeof(quRange)/sizeof(quRange[0])))
static int quRangeSeen[QU_NB_RANGE];

typedef struct
{
   ErrorNumber errNb;
   int nbFunc;             /* Functions swept (both axes counted once). */
   int nbExempt;
   int nbCompare;          /* Bit-exact output comparisons that ran. */
   int nbSkipped;          /* Elements outside the normal range, not compared. */
   int nbRangeCheck;
   int nbMinParams;        /* Functions that accepted the all-minimum setting. */
   int knownOpenIdx;       /* >=0 while sweeping a known-open function. */
   int degreeSeen[7];      /* Degrees -2..4, so a corpus of all-zeros is visible. */
} QuCtx;

/**** Local functions ****/

/* Build the three series. All prices are ordinary doubles, so multiplying by a
 * power of two is exact for every one of them.
 *
 *   0 "history" -- the suite's own 252 bars of real market data. Lively at
 *     every bar, which is why it only trips a fixed 1e-14 band at the far end
 *     of the ladder.
 *   1 "quiet"   -- a lively head and a tail that still moves, but by ~1e-5
 *     relative. The guarded quantity approaches the band from a shorter
 *     distance, so a collapse shows up at a rung a real instrument reaches.
 *   2 "halt"    -- a head that spans eighteen decades (a geometric ramp with
 *     noise), then a tail that is EXACTLY flat. Every structural "is this
 *     window empty" test in the library runs here, and the decades are what
 *     make the answer matter: a running sum that has accumulated and then
 *     cancelled values of wildly different magnitudes does NOT come back to
 *     zero, it comes back to residue of arbitrary sign. Verified to be the
 *     difference between catching and missing a dropped reseed: with a
 *     single-decade head the same mutation is invisible.
 */
static void quBuildSeries( const TA_History *history )
{
   int s, i;
   double p, wiggle;

   memset( quSeries, 0, sizeof(quSeries) );
   quSeries[0].name = "history";
   quSeries[1].name = "quiet";
   quSeries[2].name = "halt";

   for( i = 0; i < QU_NB_BARS; i++ )
   {
      unsigned int j = (unsigned int)i % history->nbBars;
      quSeries[0].open[i]   = history->open[j];
      quSeries[0].high[i]   = history->high[j];
      quSeries[0].low[i]    = history->low[j];
      quSeries[0].close[i]  = history->close[j];
      quSeries[0].volume[i] = history->volume ? history->volume[j] : 1000.0;
   }

   for( s = 1; s < QU_NB_SERIES; s++ )
   {
      ta_test_ref_lcg_seed( 0x2530000u + (unsigned int)s );
      p = 100.0;
      for( i = 0; i < QU_NB_BARS; i++ )
      {
         if( i >= QU_NB_BARS - QU_FLAT_TAIL )
         {
            /* The tail: barely moving on "quiet", not at all on "halt". */
            if( s == 1 )
               p *= 1.0 + 1.0e-5 * ta_test_ref_lcg_sym();
         }
         else if( s == 2 )
            p = 1.0e-6 * pow( 1.0e18, (double)i / (double)( QU_NB_BARS - QU_FLAT_TAIL ) )
                       * ( 1.0 + 0.3 * ta_test_ref_lcg_sym() );
         else
            p *= 1.0 + 0.01 * ta_test_ref_lcg_sym();

         wiggle = ( i < QU_NB_BARS - QU_FLAT_TAIL || s == 1 ) ? 1.0e-3 : 0.0;
         quSeries[s].close[i]  = p;
         quSeries[s].open[i]   = p;
         quSeries[s].high[i]   = p * ( 1.0 + wiggle );
         quSeries[s].low[i]    = p * ( 1.0 - wiggle );
         quSeries[s].volume[i] = 10000.0;
      }
   }

   for( i = 0; i < QU_NB_BARS; i++ )
      quPeriods[i] = 5.0;
}

/* Stage one call's inputs: the chosen series, with the chosen axis multiplied
 * by 2^exp. ldexp is exact. */
static void quStage( int series, QuAxis axis, int exp )
{
   const QuSeries *sr = &quSeries[series];
   double f = ldexp( 1.0, exp );
   double fp = ( axis == QU_AXIS_PRICE )  ? f : 1.0;
   double fv = ( axis == QU_AXIS_VOLUME ) ? f : 1.0;
   int i;

   for( i = 0; i < QU_NB_BARS; i++ )
   {
      quOpen[i]   = sr->open[i]   * fp;
      quHigh[i]   = sr->high[i]   * fp;
      quLow[i]    = sr->low[i]    * fp;
      quClose[i]  = sr->close[i]  * fp;
      quVolume[i] = sr->volume[i] * fv;
   }
}

/* One TA_CallFunc through the abstract interface into the caller's output
 * buffers, at the requested parameter setting. */
static TA_RetCode quCall( const TA_FuncInfo *funcInfo, QuParams params,
                          TA_Real outReal[][QU_NB_BARS],
                          TA_Integer outInt[][QU_NB_BARS],
                          int *outBegIdx, int *outNbElement )
{
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;
   const TA_OptInputParameterInfo *optInfo;
   TA_ParamHolder *paramHolder;
   TA_RetCode retCode;
   unsigned int i;

   retCode = TA_ParamHolderAlloc( funcInfo->handle, &paramHolder );
   if( retCode != TA_SUCCESS )
      return retCode;

   if( params == QU_PARAMS_MIN )
   {
      for( i = 0; i < funcInfo->nbOptInput; i++ )
      {
         TA_GetOptInputParameterInfo( funcInfo->handle, i, &optInfo );
         if( optInfo->type != TA_OptInput_IntegerRange || !optInfo->dataSet )
            continue;
         retCode = TA_SetOptInputParamInteger( paramHolder, i,
                      ((const TA_IntegerRange *)optInfo->dataSet)->min );
         if( retCode != TA_SUCCESS )
         {
            TA_ParamHolderFree( paramHolder );
            return retCode;
         }
      }
   }

   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( funcInfo->handle, i, &inputInfo );
      switch( inputInfo->type )
      {
      case TA_Input_Price:
         TA_SetInputParamPricePtr( paramHolder, i,
            inputInfo->flags & TA_IN_PRICE_OPEN   ? quOpen   : NULL,
            inputInfo->flags & TA_IN_PRICE_HIGH   ? quHigh   : NULL,
            inputInfo->flags & TA_IN_PRICE_LOW    ? quLow    : NULL,
            inputInfo->flags & TA_IN_PRICE_CLOSE  ? quClose  : NULL,
            inputInfo->flags & TA_IN_PRICE_VOLUME ? quVolume : NULL,
            NULL );
         break;
      case TA_Input_Real:
         /* MAVP's second real input is a per-bar PERIOD, a count of bars: it
          * carries no quote unit and scaling it would change the parameters,
          * not the unit. Everything else takes the close. */
         TA_SetInputParamRealPtr( paramHolder, i,
            strcmp( inputInfo->paramName, "inPeriods" ) == 0 ? quPeriods : quClose );
         break;
      case TA_Input_Integer:
         break;   /* No function takes an integer input array today. */
      }
   }

   for( i = 0; i < funcInfo->nbOutput && i < QU_MAX_OUTPUT; i++ )
   {
      TA_GetOutputParameterInfo( funcInfo->handle, i, &outputInfo );
      if( outputInfo->type == TA_Output_Real )
         TA_SetOutputParamRealPtr( paramHolder, i, &outReal[i][0] );
      else
         TA_SetOutputParamIntegerPtr( paramHolder, i, &outInt[i][0] );
   }

   retCode = TA_CallFunc( paramHolder, 0, QU_NB_BARS-1, outBegIdx, outNbElement );
   TA_ParamHolderFree( paramHolder );
   return retCode;
}

/* True when `d` explains the x2^exp probe already staged into quOutScaled:
 * every real output equals 2^(exp*d) times the base, bit for bit, and every
 * integer output is unchanged. */
static int quDegreeFits( const TA_FuncInfo *funcInfo, int nbElement, int exp, int d )
{
   const TA_OutputParameterInfo *outputInfo;
   double f = ldexp( 1.0, exp * d );
   unsigned int o;
   int k;

   for( o = 0; o < funcInfo->nbOutput && o < QU_MAX_OUTPUT; o++ )
   {
      TA_GetOutputParameterInfo( funcInfo->handle, o, &outputInfo );
      if( outputInfo->type != TA_Output_Real )
      {
         for( k = 0; k < nbElement; k++ )
            if( quIntBase[o][k] != quIntScaled[o][k] )
               return 0;
         continue;
      }
      for( k = 0; k < nbElement; k++ )
      {
         double want = f * quOutBase[o][k];
         if( !isfinite( want ) || !isfinite( quOutBase[o][k] ) )
            continue;
         if( memcmp( &quOutScaled[o][k], &want, sizeof(TA_Real) ) != 0 )
            return 0;
      }
   }
   return 1;
}

/* Measure the homogeneity degree on the given axis: the first candidate that
 * explains BOTH a x2 and a x4 probe. Candidates are ordered by |d| so an
 * all-zero output settles on 0 rather than on whatever is tried first.
 * Negative degrees are real: MARKETFI divides a range by a volume.
 * Returns QU_NO_DEGREE when none fits. */
#define QU_NO_DEGREE 99
static int quMeasureDegree( const TA_FuncInfo *funcInfo, QuParams params,
                            int series, QuAxis axis,
                            int baseBegIdx, int baseNbElement )
{
   static const int candidate[] = { 0, 1, -1, 2, -2, 3, 4 };
   static const int probe[2] = { 1, 2 };
   int c, e, begIdx, nbElement;

   for( c = 0; c < (int)(sizeof(candidate)/sizeof(candidate[0])); c++ )
   {
      int ok = 1;
      for( e = 0; e < 2 && ok; e++ )
      {
         quStage( series, axis, probe[e] );
         if( quCall( funcInfo, params, quOutScaled, quIntScaled, &begIdx, &nbElement ) != TA_SUCCESS )
            return QU_NO_DEGREE;
         if( begIdx != baseBegIdx || nbElement != baseNbElement )
            return QU_NO_DEGREE;
         ok = quDegreeFits( funcInfo, nbElement, probe[e], candidate[c] );
      }
      if( ok )
         return candidate[c];
   }
   return QU_NO_DEGREE;
}

/* A failure on a known-open function is the expected outcome: record it and
 * keep sweeping. Anything else is a real failure. */
static void quFail( QuCtx *ctx, ErrorNumber errNb )
{
   if( ctx->knownOpenIdx >= 0 )
   {
      quKnownOpenFailed[ctx->knownOpenIdx] = 1;
      return;
   }
   if( ctx->errNb == TA_TEST_PASS )
      ctx->errNb = errNb;
}

/* Range leg: every element of a bounded output, at whatever rung is staged. */
static void quCheckRange( QuCtx *ctx, const TA_FuncInfo *funcInfo,
                          int series, int exp, int nbElement )
{
   int r, k;

   for( r = 0; r < QU_NB_RANGE; r++ )
   {
      if( strcmp( quRange[r].name, funcInfo->name ) != 0 )
         continue;
      if( quRange[r].out >= funcInfo->nbOutput )
         continue;
      quRangeSeen[r] = 1;
      if( ctx->knownOpenIdx >= 0 ) continue;
      for( k = 0; k < nbElement; k++ )
      {
         double v = quOutScaled[quRange[r].out][k];
         if( v >= quRange[r].lo - QU_RANGE_SLACK && v <= quRange[r].hi + QU_RANGE_SLACK )
         {
            ctx->nbRangeCheck++;
            continue;
         }
         if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s out%u[%d] = %.17g on the '%s' series at 2^%d --"
                 " outside the documented [%g,%g].\n"
                 "      A 0/0 relaxed without an exact emptiness test divides"
                 " accumulator residue into itself (issue #253/#244).\n",
                 funcInfo->name, quRange[r].out, k, v, quSeries[series].name,
                 exp, quRange[r].lo, quRange[r].hi );
         quFail( ctx, TA_QUOTE_UNIT_OUT_OF_RANGE );
         return;
      }
   }
}

/* One function, one series, one axis: measure the degree, then walk the
 * ladder demanding bit-exact equivariance. */
static void quSweepOne( QuCtx *ctx, const TA_FuncInfo *funcInfo, QuParams params,
                        int series, QuAxis axis )
{
   const TA_OutputParameterInfo *outputInfo;
   TA_RetCode retCode;
   int baseBegIdx, baseNbElement, begIdx, nbElement;
   int degree, s, k;
   unsigned int o;

   quStage( series, axis, 0 );
   retCode = quCall( funcInfo, params, quOutBase, quIntBase, &baseBegIdx, &baseNbElement );
   if( retCode != TA_SUCCESS || baseNbElement <= 0 )
   {
      /* At the minimum of every integer range the parameters can be mutually
       * inconsistent, or consume the whole series. Nothing to compare, and
       * nothing wrong: decline this setting. The DEFAULTS must always work. */
      if( params != QU_PARAMS_DEFAULT )
         return;
      if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s on the '%s' series: retCode = %d, %d output(s) at the"
              " natural quote unit and default parameters.\n",
              funcInfo->name, quSeries[series].name, (int)retCode, baseNbElement );
      quFail( ctx, retCode != TA_SUCCESS ? TA_QUOTE_UNIT_CALL_FAILED
                                         : TA_QUOTE_UNIT_VACUOUS );
      return;
   }
   if( params == QU_PARAMS_MIN && axis == QU_AXIS_PRICE && series == 0 )
      ctx->nbMinParams++;

   degree = quMeasureDegree( funcInfo, params, series, axis, baseBegIdx, baseNbElement );
   if( degree == QU_NO_DEGREE )
   {
      if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s is not homogeneous in the %s on the '%s' series: no degree"
              " explains both a x2 and a x4 rescale.\n"
              "      Either the body compares a quote-unit-carrying quantity against an"
              " absolute constant (issue #253),\n"
              "      or the function belongs on this gate's exempt list with a reason.\n",
              funcInfo->name, axis == QU_AXIS_PRICE ? "prices" : "volumes",
              quSeries[series].name );
      quFail( ctx, TA_QUOTE_UNIT_NO_DEGREE );
      return;
   }
   ctx->degreeSeen[degree + 2]++;

   for( s = 0; s < QU_NB_SCALE; s++ )
   {
      int exp = quScaleExp[s];
      double f = ldexp( 1.0, exp * degree );

      quStage( series, axis, exp );
      retCode = quCall( funcInfo, params, quOutScaled, quIntScaled, &begIdx, &nbElement );
      if( retCode != TA_SUCCESS || begIdx != baseBegIdx || nbElement != baseNbElement )
      {
         if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s on the '%s' series at 2^%d: shape moved -- retCode %d,"
                 " (begIdx,nb) = (%d,%d), natural gives (%d,%d).\n",
                 funcInfo->name, quSeries[series].name, exp, (int)retCode,
                 begIdx, nbElement, baseBegIdx, baseNbElement );
         quFail( ctx, TA_QUOTE_UNIT_SHAPE_MOVED );
         return;
      }

      quCheckRange( ctx, funcInfo, series, exp, nbElement );
      if( ctx->errNb != TA_TEST_PASS )
         return;

      for( o = 0; o < funcInfo->nbOutput && o < QU_MAX_OUTPUT; o++ )
      {
         TA_GetOutputParameterInfo( funcInfo->handle, o, &outputInfo );
         if( outputInfo->type != TA_Output_Real )
         {
            for( k = 0; k < nbElement; k++ )
            {
               if( quIntBase[o][k] != quIntScaled[o][k] )
               {
                  if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s out%u[%d] on the '%s' series at 2^%d: %d,"
                          " natural gives %d. An integer output carries no quote unit.\n",
                          funcInfo->name, o, k, quSeries[series].name, exp,
                          quIntScaled[o][k], quIntBase[o][k] );
                  quFail( ctx, TA_QUOTE_UNIT_NOT_INVARIANT );
                  return;
               }
               ctx->nbCompare++;
            }
            continue;
         }

         for( k = 0; k < nbElement; k++ )
         {
            double base = quOutBase[o][k];
            double want = f * base;

            /* The claim is exact equivariance, which holds while every
             * intermediate stays a normal number. Outside that it is the
             * rescale that broke, not the function -- skip, and count. */
            if( !isfinite( base ) || !isfinite( want ) ||
                ( want != 0.0 && fabs( want ) < DBL_MIN ) ||
                ( base != 0.0 && fabs( base ) < DBL_MIN ) )
            {
               ctx->nbSkipped++;
               continue;
            }

            if( memcmp( &quOutScaled[o][k], &want, sizeof(TA_Real) ) != 0 )
            {
               if( ctx->knownOpenIdx < 0 )
            printf( "\nFail: %s out%u[%d] on the '%s' series: at 2^%d it is %.17g,"
                       " the natural quote unit gives %.17g (x2^%d = %.17g).\n"
                       "      %s is homogeneous of degree %d, so a power-of-two change"
                       " of quote unit is exact and must be bit-identical.\n"
                       "      A difference means an absolute constant met a quantity"
                       " that carries the unit (issue #253).\n",
                       funcInfo->name, o, k, quSeries[series].name, exp,
                       quOutScaled[o][k], base, exp * degree, want,
                       funcInfo->name, degree );
               quFail( ctx, TA_QUOTE_UNIT_NOT_INVARIANT );
               return;
            }
            ctx->nbCompare++;
         }
      }
   }
}

static void quOneFunction( const TA_FuncInfo *funcInfo, void *opaque )
{
   QuCtx *ctx = (QuCtx *)opaque;
   int series, e, params;

   if( ctx->errNb != TA_TEST_PASS )
      return;

   for( e = 0; e < QU_NB_EXEMPT; e++ )
   {
      if( strcmp( quExempt[e], funcInfo->name ) == 0 )
      {
         quExemptSeen[e] = 1;
         ctx->nbExempt++;
         return;
      }
   }
   /* sin/cos/log/exp of a price, and sqrt's half degree. */
   if( strcmp( funcInfo->group, "Math Transform" ) == 0 )
   {
      ctx->nbExempt++;
      return;
   }

   ctx->knownOpenIdx = -1;
   for( e = 0; e < QU_NB_KNOWN_OPEN; e++ )
      if( strcmp( quKnownOpen[e], funcInfo->name ) == 0 )
         ctx->knownOpenIdx = e;

   if( ctx->knownOpenIdx < 0 )
      ctx->nbFunc++;
   for( params = 0; params < QU_NB_PARAMS; params++ )
      for( series = 0; series < QU_NB_SERIES; series++ )
      {
         quSweepOne( ctx, funcInfo, (QuParams)params, series, QU_AXIS_PRICE );
         if( ctx->errNb != TA_TEST_PASS ) return;
         quSweepOne( ctx, funcInfo, (QuParams)params, series, QU_AXIS_VOLUME );
         if( ctx->errNb != TA_TEST_PASS ) return;
      }
}


/* ==========================================================================
 * LEG 3: the value each function is invariant AT (issue #253).
 *
 * Leg 1 is self-referential by construction: it compares this library to
 * itself. It would pass just as happily on the OLD code, which answered 0 at
 * every rung below its band -- 0 being about as scale-invariant as a number
 * gets. So each row here carries a value computed OUTSIDE this library, and is
 * checked at 2^-60, a quote unit where the old code returned 0 (or, for KAMA
 * and the two ACCBANDS edges, a wrong non-zero).
 *
 * The corpus is the suite's committed 252-bar history -- decimal literals in
 * test_data.c, byte-identical on every platform. Deliberately NOT a generated
 * series: this suite's generators are portable in their integer stream but the
 * prices they build from it are floating-point, which puts them on the same
 * footing as every other bit-exact test here (-ffp-contract=off, and outside
 * the i386 excess-precision lane). That is an acceptable basis for a
 * STRUCTURAL assertion -- "must be exactly 0.0" cannot be flipped by a last-ulp
 * platform difference -- and the wrong basis for a captured value.
 *
 * 2^-60 is exact, so the fixed body returns the SAME bits it returns at the
 * natural quote unit; leg 1 pins that. What this leg adds is that the value is
 * the one an independent implementation computes.
 *
 * Provenance. Captured 2026-08-24 against pandas-ta-classic 0.6.52 on
 * pandas 3.0.3, via ta-lib-oracles/pandas_serve, on the history above. Rows
 * marked "definition" were not available there and are an independent
 * transcription of the published formula with no epsilon anywhere in it --
 * a naive two-pass rather than this library's incremental form:
 *
 *   STOCHF_K   100*(close-lowest)/(highest-lowest) over the 5-bar window
 *   STOCH_K    the 3-bar mean of that
 *   SMI        Blau: EMA(EMA(close-(hh+ll)/2)) / (EMA(EMA(hh-ll))/2), x100
 *   BETA       ordinary least-squares slope of the two return series
 *   NATR       100 * Wilder ATR / close  (pandas-ta seeds its ATR differently,
 *              which is the one row where its value is 17% away -- a seeding
 *              difference, not a disagreement about this fix)
 *   CMO        pandas-ta has no Wilder CMO (its `cmo` is the unsmoothed form,
 *              which is our CMOU). Taken from its RSI through the identity
 *              CMO == 2*RSI - 100, exact for the shared Wilder accumulators.
 *   DX         from pandas-ta's own DI columns through DX's definition.
 *
 * Tolerances are the measured gap with margin, not round numbers: 1e-12 where
 * the definitions coincide (measured 0 to 8.9e-14) and 1e-6 for the ADX family,
 * where pandas-ta's Wilder seeding differs from ours (measured worst 7.3e-08).
 * Every one of them is many orders below the 100% error the old code made, so
 * no row can pass on the unfixed library.
 * ==========================================================================*/
#define QU_ORACLE_EXP  (-60)
#define QU_ORACLE_BAR  251

static TA_Real qoOut1[QU_NB_BARS], qoOut2[QU_NB_BARS], qoOut3[QU_NB_BARS];

typedef struct { const char *label; double expected; double tol; } QuOracleRow;
static const QuOracleRow quOracleRow[] = {
   { "RSI",       49.632102070867582,   1.0e-12 },
   { "CMO",       -0.73579585826483651, 1.0e-12 },
   { "CMOU",      -8.0771979985703695,  1.0e-12 },
   { "CCI",       -98.359503987074376,  1.0e-12 },
   { "ULTOSC",    40.085402976991773,   1.0e-12 },
   { "KAMA",      106.0828124676471,    1.0e-12 },
   { "NATR",      3.02290672213603,     1.0e-12 },
   { "BOP",       -0.45833333333333159, 1.0e-12 },
   { "ADX",       15.526057510526849,   1.0e-6  },
   { "ADXR",      20.492086296160466,   1.0e-6  },
   { "DX",        0.47222726427924594,  1.0e-6  },
   { "PLUS_DI",   20.99955113874627,    1.0e-6  },
   { "MINUS_DI",  21.198823368250974,   1.0e-6  },
   { "STOCH_K",   30.19478427095876,    1.0e-12 },
   { "STOCHF_K",  30.266343825665892,   1.0e-12 },
   { "ACCB_U",    119.65651267719166,   1.0e-12 },
   { "ACCB_L",    101.85401267719166,   1.0e-12 },
   { "SMI",       4.3484442819111404,   1.0e-12 },
   { "BETA",      4.5859154748966056,   1.0e-12 }
};
#define QU_NB_ORACLE ((int)(sizeof(quOracleRow)/sizeof(quOracleRow[0])))
static int quOracleSeen[QU_NB_ORACLE];

/* One row: the value at QU_ORACLE_BAR, divided by the scale factor for a
 * price-valued output, against its external reference. */
static ErrorNumber quOracleOne( const char *label, TA_RetCode retCode,
                                int begIdx, int nbElement,
                                const TA_Real *out, double divisor, int *checks )
{
   int i, k;

   for( i = 0; i < QU_NB_ORACLE; i++ )
   {
      double got, d;
      if( strcmp( quOracleRow[i].label, label ) != 0 ) continue;
      quOracleSeen[i] = 1;
      k = QU_ORACLE_BAR - begIdx;
      if( retCode != TA_SUCCESS || k < 0 || k >= nbElement )
      {
         printf( "\nFail: quote-unit oracle %s: retCode %d, bar %d outside (%d,%d)\n",
                 label, (int)retCode, QU_ORACLE_BAR, begIdx, nbElement );
         return TA_QUOTE_UNIT_CALL_FAILED;
      }
      got = out[k] / divisor;
      d = fabs( got - quOracleRow[i].expected )
        / ( quOracleRow[i].expected != 0.0 ? fabs( quOracleRow[i].expected ) : 1.0 );
      if( d > quOracleRow[i].tol )
      {
         printf( "\nFail: quote-unit oracle %s at 2^%d: got %.17g, an independent"
                 " implementation gives %.17g (rel %.3g > %.3g).\n"
                 "      The old library answered 0 here. A scale-invariant answer is"
                 " not the same as a correct one (issue #253).\n",
                 label, QU_ORACLE_EXP, got, quOracleRow[i].expected, d,
                 quOracleRow[i].tol );
         return TA_QUOTE_UNIT_NOT_INVARIANT;
      }
      (*checks)++;
      return TA_TEST_PASS;
   }
   printf( "\nFail: quote-unit oracle row '%s' is not in the table.\n", label );
   return TA_QUOTE_UNIT_VACUOUS;
}

#define QU_ORACLE(label,call,div)                                              \
   do {                                                                        \
      TA_RetCode rc_ = (call);                                                  \
      ErrorNumber e_ = quOracleOne( label, rc_, b, n, qoOut1, div, &checks );    \
      if( e_ != TA_TEST_PASS ) return e_;                                       \
   } while(0)

static ErrorNumber quOracleLeg( const TA_History *history )
{
   double tick = ldexp( 1.0, QU_ORACLE_EXP );
   int b = 0, n = 0, i, checks = 0;
   TA_RetCode rc;

   for( i = 0; i < (int)history->nbBars && i < QU_NB_BARS; i++ )
   {
      quOpen[i]   = history->open[i]   * tick;
      quHigh[i]   = history->high[i]   * tick;
      quLow[i]    = history->low[i]    * tick;
      quClose[i]  = history->close[i]  * tick;
      quVolume[i] = history->volume[i];          /* a count, not a quote unit */
   }

   QU_ORACLE( "RSI",      TA_RSI( 0, QU_NB_BARS-1, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "CMO",      TA_CMO( 0, QU_NB_BARS-1, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "CMOU",     TA_CMOU( 0, QU_NB_BARS-1, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "CCI",      TA_CCI( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "ULTOSC",   TA_ULTOSC( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 7, 14, 28, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "KAMA",     TA_KAMA( 0, QU_NB_BARS-1, quClose, 30, &b, &n, qoOut1 ), tick );
   QU_ORACLE( "NATR",     TA_NATR( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "BOP",      TA_BOP( 0, QU_NB_BARS-1, quOpen, quHigh, quLow, quClose, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "ADX",      TA_ADX( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "ADXR",     TA_ADXR( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "DX",       TA_DX( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "PLUS_DI",  TA_PLUS_DI( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "MINUS_DI", TA_MINUS_DI( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 14, &b, &n, qoOut1 ), 1.0 );
   QU_ORACLE( "BETA",     TA_BETA( 0, QU_NB_BARS-1, quClose, quVolume, 5, &b, &n, qoOut1 ), 1.0 );

   rc = TA_STOCH( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 5, 3, TA_MAType_SMA,
                  3, TA_MAType_SMA, &b, &n, qoOut1, qoOut2 );
   { ErrorNumber e_ = quOracleOne( "STOCH_K", rc, b, n, qoOut1, 1.0, &checks );
     if( e_ != TA_TEST_PASS ) return e_; }

   rc = TA_STOCHF( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 5, 3, TA_MAType_SMA,
                   &b, &n, qoOut1, qoOut2 );
   { ErrorNumber e_ = quOracleOne( "STOCHF_K", rc, b, n, qoOut1, 1.0, &checks );
     if( e_ != TA_TEST_PASS ) return e_; }

   rc = TA_ACCBANDS( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 20, &b, &n,
                     qoOut1, qoOut2, qoOut3 );
   { ErrorNumber e_ = quOracleOne( "ACCB_U", rc, b, n, qoOut1, tick, &checks );
     if( e_ != TA_TEST_PASS ) return e_;
     e_ = quOracleOne( "ACCB_L", rc, b, n, qoOut3, tick, &checks );
     if( e_ != TA_TEST_PASS ) return e_; }

   rc = TA_SMI( 0, QU_NB_BARS-1, quHigh, quLow, quClose, 10, 3, 3, 3, &b, &n,
                qoOut1, qoOut2 );
   { ErrorNumber e_ = quOracleOne( "SMI", rc, b, n, qoOut1, 1.0, &checks );
     if( e_ != TA_TEST_PASS ) return e_; }

   for( i = 0; i < QU_NB_ORACLE; i++ )
      if( !quOracleSeen[i] )
      {
         printf( "\nFail: quote-unit oracle row '%s' was never checked.\n",
                 quOracleRow[i].label );
         return TA_QUOTE_UNIT_VACUOUS;
      }
   if( checks != QU_NB_ORACLE )
   {
      printf( "\nFail: quote-unit oracle ran %d check(s), expected %d.\n",
              checks, QU_NB_ORACLE );
      return TA_QUOTE_UNIT_VACUOUS;
   }

   printf( "  Quote-unit oracle (issue #253): %d function(s) at 2^%d -- where the old"
           " library returned 0 -- match an implementation outside this library"
           " (pandas-ta-classic 0.6.52, or the published formula).\n",
           checks, QU_ORACLE_EXP );
   return TA_TEST_PASS;
}

ErrorNumber test_func_quote_unit( TA_History *history )
{
   QuCtx ctx;
   int i;

   memset( &ctx, 0, sizeof(ctx) );
   ctx.errNb = TA_TEST_PASS;
   memset( quExemptSeen, 0, sizeof(quExemptSeen) );
   memset( quRangeSeen, 0, sizeof(quRangeSeen) );

   if( history->nbBars < (unsigned int)QU_NB_BARS )
   {
      printf( "\nFail: quote-unit gate needs %d bars, history has %u.\n",
              QU_NB_BARS, history->nbBars );
      return TA_QUOTE_UNIT_VACUOUS;
   }

   memset( quOracleSeen, 0, sizeof(quOracleSeen) );
   if( history->nbBars != QU_NB_BARS || !history->volume )
   {
      printf( "\nFail: quote-unit oracle expects the suite's %d-bar history with"
              " volume; got %u bars.\n", QU_NB_BARS, history->nbBars );
      return TA_QUOTE_UNIT_VACUOUS;
   }
   { ErrorNumber e = quOracleLeg( history );
     if( e != TA_TEST_PASS ) return e; }

   quBuildSeries( history );
   TA_ForEachFunc( quOneFunction, &ctx );
   if( ctx.errNb != TA_TEST_PASS )
      return ctx.errNb;

   /* Non-vacuity. Every one of these has been a real failure mode of a sweep
    * in this suite: an exempt name that no longer exists silently drops a
    * function, a range entry whose function was renamed silently drops a leg,
    * and a corpus that produced nothing to compare reads green. */
   for( i = 0; i < QU_NB_EXEMPT; i++ )
      if( !quExemptSeen[i] )
      {
         printf( "\nFail: quote-unit exempt list names '%s', which TA_ForEachFunc"
                 " never enumerated.\n", quExempt[i] );
         return TA_QUOTE_UNIT_VACUOUS;
      }
   for( i = 0; i < QU_NB_RANGE; i++ )
      if( !quRangeSeen[i] )
      {
         printf( "\nFail: quote-unit range table names '%s' out%u, which was never"
                 " reached.\n", quRange[i].name, quRange[i].out );
         return TA_QUOTE_UNIT_VACUOUS;
      }
   for( i = 0; i < QU_NB_KNOWN_OPEN; i++ )
      if( !quKnownOpenFailed[i] )
      {
         printf( "\nFail: %s is listed as a known open quote-unit defect (#253) but now"
                 " passes the sweep.\n"
                 "      If that is the fix landing, delete the entry from quKnownOpen so"
                 " the function is held to the invariant from here on.\n", quKnownOpen[i] );
         return TA_QUOTE_UNIT_VACUOUS;
      }
   if( ctx.nbFunc < 100 || ctx.nbCompare < 100000 || ctx.nbRangeCheck < 1000 ||
       ctx.nbMinParams < 50 )
   {
      printf( "\nFail: quote-unit sweep too thin: %d function(s), %d comparison(s),"
              " %d range check(s), %d at minimum parameters.\n",
              ctx.nbFunc, ctx.nbCompare, ctx.nbRangeCheck, ctx.nbMinParams );
      return TA_QUOTE_UNIT_VACUOUS;
   }
   if( ctx.degreeSeen[2] == 0 || ctx.degreeSeen[3] == 0 )
   {
      printf( "\nFail: quote-unit sweep measured no degree-0 or no degree-1 output"
              " (degrees -2..4: %d/%d/%d/%d/%d/%d/%d): the corpus cannot be"
              " exercising both an oscillator and a price.\n",
              ctx.degreeSeen[0], ctx.degreeSeen[1], ctx.degreeSeen[2],
              ctx.degreeSeen[3], ctx.degreeSeen[4], ctx.degreeSeen[5],
              ctx.degreeSeen[6] );
      return TA_QUOTE_UNIT_VACUOUS;
   }

   printf( "  Quote-unit invariance (issue #253): %d function(s) x %d series x %d"
           " power-of-two rescale(s) on 2 axes -- %d bit-exact comparison(s),"
           " %d skipped past the normal range, %d exempt; %d also swept at the"
           " minimum of every integer parameter.\n",
           ctx.nbFunc, QU_NB_SERIES, QU_NB_SCALE, ctx.nbCompare,
           ctx.nbSkipped, ctx.nbExempt, ctx.nbMinParams );
   printf( "  Quote-unit range: %d value(s) inside the documented bounds of %d"
           " bounded output(s) at every rung.\n", ctx.nbRangeCheck, QU_NB_RANGE );
   printf( "  Quote-unit known open (#253): %s still divide by a moving average guarded"
           " with the fixed band; both confirmed still failing, so the entry cannot rot"
           " into a silent pass.\n", "PPO and PVO" );

   return TA_TEST_PASS;
}
