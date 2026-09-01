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

/* Description:
 *     The frozen v0.6.4 reference gate (issue #188).
 *
 *     Replays every case in ta_test_legacy_data.h against the current library
 *     and holds it to what released v0.6.4 answered, at full double precision.
 *     This is the tight counterpart to the hand-written groups, whose expected
 *     values carry 2-6 significant digits against an absolute 0.01 window --
 *     wide enough that a third of them cannot see a 0.1% error, and wide enough
 *     that two transcription errors survived in it for two decades (#188).
 *
 *     Everything about the DATA -- scope, exclusions, sampling, the vacuous
 *     candlestick list -- is documented in ta_test_legacy_data.h. This file
 *     owns the COMPARISON, and the only hand-maintained knob is LEGACY_TOL.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_test_legacy_data.h"

/**** Local declarations. ****/

/* Per-function absolute tolerance against the frozen v0.6.4 value.
 *
 * A function absent from this table is compared EXACTLY, and that is the
 * majority. An entry is an AUTHORIZED, MEASURED divergence, not slack: each
 * bound is the largest deviation the function actually shows across its cases
 * here, times ~3, rounded up to one significant digit.
 *
 * They are ABSOLUTE, which is the right model rather than a convenience. Both
 * divergence causes are driven by the magnitude of an INTERMEDIATE at INPUT
 * scale, not by the output: v0.6.4's E[x^2]-mean^2 loses ~eps*mean^2, and a
 * fused a*b+c site differs by ~ULP of a price-scale accumulator. Neither shrinks
 * when the output happens to be small, so the error is a constant floor per
 * function on this series -- meaning that at a function's smallest sampled value
 * the same bound is a larger relative share (~1e-10 for the tightest rows). That
 * is the genuine error floor, not headroom, and still ~8 orders tighter than the
 * 0.01 absolute window the hand-written tables use at those magnitudes.
 *
 * Two causes, both bounded and both pre-existing:
 *
 *   (a) explicit fma() adoption; v0.6.4 predates the contract, so a fused site
 *       differs from it in the last bits.
 *
 *   (b) an algorithm deliberately changed after v0.6.4 and pinned elsewhere: the
 *       cancellation-free variance form (VAR, STDDEV, BBANDS), the same
 *       treatment in CORREL and BETA, the O(1) sliding-sum LINEARREG family, and
 *       TA_WMA's re-anchored weighted totals -- which APO and PPO inherit
 *       through TA_MA(TA_MAType_WMA), the only two rows here that move for a
 *       change to a function they merely dispatch to.
 *
 * A blanket contract bound would buy unearned slack: CCI, IMI, KAMA, MACD,
 * MACDEXT, APO, PPO and STOCHF are all bit-exact against v0.6.4 on this series,
 * their divergences needing the fuzz corpus's extreme magnitudes to appear.
 *
 * ONE PRINCIPLED EXCEPTION TO "3x MEASURED": a function reaching a libm routine
 * that is not correctly rounded gets a floor of 8 ULP at its own output
 * magnitude, its last bit being a property of the host libm rather than of
 * TA-Lib. Only MAMA needs it now.
 *
 * These bounds were measured with both sides on ONE host's libm, so for the two
 * functions that still reach atan they bound the fma() drift, NOT a cross-libm
 * difference. That is survivable because the atan is smooth in their output
 * (MAMA feeds a damped adaptive alpha, LINEARREG_ANGLE's atan is terminal); it
 * was not survivable for the Hilbert family, whose atan becomes an integer loop
 * bound, which is why those are excluded outright rather than widened. If MAMA
 * or LINEARREG_ANGLE ever fails on a non-glibc host, that is the cause: widen
 * the floor with that host's measurement, or follow the Hilbert family out.
 *
 * Integer outputs are NEVER given a tolerance -- a candlestick or index flip
 * must fail regardless of what is in this table.
 *
 * AT RE-FREEZE: regenerate the data header and delete every row below. Both
 * causes are divergences from v0.6.4 specifically, so a later reference is
 * bit-exact. The libm floors are the one part that may need to survive.
 */
typedef struct
{
   const char *func;
   double      absTol;
} TA_LegacyTol;

static const TA_LegacyTol LEGACY_TOL[] =
{
   /* --- (b) deliberate post-0.6.4 algorithm changes ---------------------- */
   { "CORREL",              1e-12 },  /* #242  measured 3.15e-13             */
   { "BETA",                2e-13 },  /* #242  measured 4.21e-14             */
   { "VAR",                 4e-11 },  /* #118  measured 1.31e-11             */
   { "STDDEV",              3e-11 },  /* #118  measured 9.19e-12             */
   { "BBANDS",              2e-11 },  /* #118  measured 3.68e-12 (via STDDEV)*/
   { "LINEARREG_ANGLE",     6e-11 },  /* #103  measured 1.74e-11             */
   { "TSF",                 2e-11 },  /* #103  measured 3.42e-12             */
   { "LINEARREG",           1e-11 },  /* #103  measured 3.07e-12             */
   { "LINEARREG_INTERCEPT", 1e-11 },  /* #103  measured 3.21e-12             */
   { "APO",                 1e-13 },  /* #255  measured 4.26e-14            */
   { "PPO",                 1e-13 },  /* #255  measured 3.90e-14            */
   { "LINEARREG_SLOPE",     2e-12 },  /* #103  measured 3.49e-13             */

   /* --- (a) explicit fma() adoption, PR #96 ------------------------------
    * Verified per row: ADOSC, T3, TEMA, DEMA, SAREXT, EMA, SAR, TRIX, MACDFIX
    * and MAMA contain fma() directly. MA, MAVP and STOCH contain none and
    * inherit it one dispatch hop away -- MA through its EMA/DEMA/TEMA/T3/MAMA
    * arms, MAVP through MA, STOCH through the MAType=EMA smoothing of its alt
    * case (its all-SMA default is bit-exact against v0.6.4). Worth stating
    * because "STOCH diverges from v0.6.4" looks unexplained until you notice
    * the parameter set doing it. */
   { "ADOSC",               3e-08 },  /* measured 7.45e-09, output ~4.3e6    */
   { "T3",                  4e-13 },  /* measured 1.28e-13                   */
   { "TEMA",                3e-13 },  /* measured 9.95e-14                   */
   { "DEMA",                2e-13 },  /* measured 4.26e-14                   */
   { "SAREXT",              9e-14 },  /* measured 2.84e-14                   */
   { "EMA",                 5e-14 },  /* measured 1.42e-14                   */
   { "MA",                  5e-14 },  /* measured 1.42e-14 (EMA/T3 arms)     */
   { "MAVP",                5e-14 },  /* measured 1.42e-14, via MA->EMA      */
   { "SAR",                 5e-14 },  /* measured 1.42e-14                   */
   { "TRIX",                4e-14 },  /* measured 1.11e-14                   */
   { "STOCH",               3e-14 },  /* measured 7.11e-15, alt/EMA arm only */
   { "MACDFIX",             4e-15 },  /* measured 1.33e-15                   */

   /* --- (a) plus the 8-ULP libm floor (atan-derived output) -------------- */
   { "MAMA",                3e-13 }   /* measured 2.84e-14, floor 8 ULP(128) */
};

#define NB_LEGACY_TOL ((int)(sizeof(LEGACY_TOL)/sizeof(LEGACY_TOL[0])))

/* Absolute tolerance for one function; 0.0 (exact) unless listed. */
static double legacy_tol( const char *func )
{
   int i;
   for( i = 0; i < NB_LEGACY_TOL; i++ )
      if( strcmp( func, LEGACY_TOL[i].func ) == 0 )
         return LEGACY_TOL[i].absTol;
   return 0.0;
}

/* Output buffers. MAX_NB_TEST_ELEMENT (280) covers the 252-bar series; the
 * global gBuffer is not reused here because a case needs up to three outputs
 * simultaneously and does not need its guard bands. */
static TA_Real    legacyOutReal[3][MAX_NB_TEST_ELEMENT];
static TA_Integer legacyOutInt[3][MAX_NB_TEST_ELEMENT];

/* Which series feeds which input. Mirrors ta_test_legacy_data.h's INPUTS note
 * exactly: the frozen values were produced with this mapping and are
 * meaningless under any other one. */
static void legacy_setup_inputs( TA_ParamHolder *paramHolder,
                                 const TA_FuncInfo *funcInfo,
                                 const TA_History *history )
{
   unsigned int i;
   int realInputCount = 0;

   for( i = 0; i < funcInfo->nbInput; i++ )
   {
      const TA_InputParameterInfo *inputInfo;
      TA_GetInputParameterInfo( funcInfo->handle, i, &inputInfo );

      if( inputInfo->type == TA_Input_Price )
      {
         TA_SetInputParamPricePtr( paramHolder, i,
            inputInfo->flags & TA_IN_PRICE_OPEN         ? history->open   : NULL,
            inputInfo->flags & TA_IN_PRICE_HIGH         ? history->high   : NULL,
            inputInfo->flags & TA_IN_PRICE_LOW          ? history->low    : NULL,
            inputInfo->flags & TA_IN_PRICE_CLOSE        ? history->close  : NULL,
            inputInfo->flags & TA_IN_PRICE_VOLUME       ? history->volume : NULL,
            inputInfo->flags & TA_IN_PRICE_OPENINTEREST ? history->openInterest : NULL );
      }
      else if( inputInfo->type == TA_Input_Real )
      {
         /* first real input -> close, second -> high */
         TA_SetInputParamRealPtr( paramHolder, i,
                                  realInputCount == 0 ? history->close : history->high );
         realInputCount++;
      }
      /* TA_Input_Integer: no function in scope has one (the generator refuses
       * to freeze a case it cannot feed), so nothing to do. */
   }
}

/* Comparisons actually performed, incremented AT each comparison -- never
 * before the loop and never from nbSample. A counter bumped from the table
 * would report full coverage while the comparison below was skipped or
 * short-circuited, which is precisely the vacuity this gate exists to close. */
static long g_legacyRealCmp;
static long g_legacyIntCmp;
static long g_legacyShapeCmp;

/* Run one case and check it. */
static ErrorNumber do_test_legacy_case( const TA_History *history,
                                        const TA_LegacyCase *c )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   TA_ParamHolder      *paramHolder = NULL;
   TA_RetCode           retCode;
   TA_Integer           outBegIdx = 0, outNbElement = 0;
   unsigned int         i;
   int                  outputIsInteger[3];
   double               tol;

   if( TA_GetFuncHandle( c->func, &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "Fail: [%s] not found in ta_abstract\n", c->func );
      return TA_REGTEST_LEGACY_UNKNOWN_FUNC;
   }

   if( funcInfo->nbOptInput != c->nbOpt )
   {
      printf( "Fail: [%s] has %u optional param(s), the frozen case carries %u. "
              "The parameter list changed since the freeze -- the case must be "
              "regenerated, not silently reinterpreted.\n",
              c->func, funcInfo->nbOptInput, c->nbOpt );
      return TA_REGTEST_LEGACY_PARAM_MISMATCH;
   }
   if( funcInfo->nbOutput > 3 )
   {
      printf( "Fail: [%s] has %u outputs; this gate handles at most 3\n",
              c->func, funcInfo->nbOutput );
      return TA_REGTEST_LEGACY_TOO_MANY_OUTPUTS;
   }
   /* A regenerated table that outgrew either array bound would otherwise read
    * past the end of the case struct. */
   if( c->nbOpt > TA_LEGACY_MAX_OPT || c->nbSample > TA_LEGACY_MAX_SAMPLE )
   {
      printf( "Fail: [%s] case carries nbOpt=%u nbSample=%u, past the "
              "TA_LEGACY_MAX_OPT=%d / TA_LEGACY_MAX_SAMPLE=%d bounds\n",
              c->func, c->nbOpt, c->nbSample,
              TA_LEGACY_MAX_OPT, TA_LEGACY_MAX_SAMPLE );
      return TA_REGTEST_LEGACY_BAD_SAMPLE;
   }

   if( TA_ParamHolderAlloc( handle, &paramHolder ) != TA_SUCCESS )
   {
      printf( "Fail: [%s] TA_ParamHolderAlloc\n", c->func );
      return TA_REGTEST_LEGACY_ALLOC_FAILED;
   }

   legacy_setup_inputs( paramHolder, funcInfo, history );

   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *optInfo;
      TA_GetOptInputParameterInfo( handle, i, &optInfo );
      if( optInfo->type == TA_OptInput_RealRange ||
          optInfo->type == TA_OptInput_RealList )
         TA_SetOptInputParamReal( paramHolder, i, c->opt[i] );
      else
         TA_SetOptInputParamInteger( paramHolder, i, (int)c->opt[i] );
   }

   for( i = 0; i < funcInfo->nbOutput; i++ )
   {
      const TA_OutputParameterInfo *outInfo;
      TA_GetOutputParameterInfo( handle, i, &outInfo );
      outputIsInteger[i] = ( outInfo->type == TA_Output_Integer );
      if( outputIsInteger[i] )
         TA_SetOutputParamIntegerPtr( paramHolder, i, legacyOutInt[i] );
      else
         TA_SetOutputParamRealPtr( paramHolder, i, legacyOutReal[i] );
   }

   retCode = TA_CallFunc( paramHolder, 0, (int)history->nbBars - 1,
                          &outBegIdx, &outNbElement );

   TA_ParamHolderFree( paramHolder );

   if( retCode != c->expectedRetCode )
   {
      printf( "Fail: [%s] retCode %d, frozen v0.6.4 gave %d\n",
              c->func, (int)retCode, (int)c->expectedRetCode );
      return TA_REGTEST_LEGACY_BAD_RETCODE;
   }
   if( outBegIdx != c->expectedBegIdx || outNbElement != c->expectedNbElement )
   {
      printf( "Fail: [%s] shape (begIdx=%d,nbElement=%d), frozen v0.6.4 gave "
              "(%d,%d)\n", c->func, (int)outBegIdx, (int)outNbElement,
              c->expectedBegIdx, c->expectedNbElement );
      return TA_REGTEST_LEGACY_BAD_SHAPE;
   }
   g_legacyShapeCmp++;

   tol = legacy_tol( c->func );

   for( i = 0; i < c->nbSample; i++ )
   {
      const TA_LegacySample *s = &c->sample[i];
      double got;

      if( s->outputNb < 0 || (unsigned int)s->outputNb >= funcInfo->nbOutput ||
          s->index < 0 || s->index >= outNbElement )
      {
         printf( "Fail: [%s] frozen sample (output %d, index %d) is outside the "
                 "current output shape\n", c->func, s->outputNb, s->index );
         return TA_REGTEST_LEGACY_BAD_SAMPLE;
      }

      if( outputIsInteger[s->outputNb] )
      {
         /* Integer outputs are exact, always: no tolerance can absorb a
          * candlestick or index flip, and none should. */
         got = (double)legacyOutInt[s->outputNb][s->index];
         g_legacyIntCmp++;
         if( got != s->value )
         {
            printf( "Fail: [%s] output %d index %d = %d, frozen v0.6.4 gave %d\n",
                    c->func, s->outputNb, s->index,
                    (int)got, (int)s->value );
            return TA_REGTEST_LEGACY_BAD_VALUE;
         }
         continue;
      }

      got = legacyOutReal[s->outputNb][s->index];
      g_legacyRealCmp++;
      /* fabs(got - want) <= tol, with tol == 0.0 meaning bit-exact. Note this
       * also accepts -0.0 against +0.0, which is correct: they are numerically
       * equal and MIN/MAX/MINMAX/MIDPOINT legitimately differ from v0.6.4 in
       * that bit alone (--fuzz-064 calls those out as BENIGN). */
      if( !( fabs( got - s->value ) <= tol ) )
      {
         printf( "Fail: [%s] output %d index %d = %.17g, frozen v0.6.4 gave "
                 "%.17g (diff %.3g, tolerance %.3g)\n",
                 c->func, s->outputNb, s->index, got, s->value,
                 fabs( got - s->value ), tol );
         return TA_REGTEST_LEGACY_BAD_VALUE;
      }
   }

   return TA_TEST_PASS;
}

/* Hard floors on what a run must actually have compared. Deliberately literal
 * rather than derived from the table: a floor computed from TA_LEGACY_CASE
 * would move with it, so deleting half the rows would still "pass its floor".
 * These are the counts the freeze shipped with; a regeneration that grows the
 * table only ever makes them easier to clear, and one that shrinks it has to
 * say so here. */
#define LEGACY_FLOOR_CASES  216
#define LEGACY_FLOOR_FUNCS  142
#define LEGACY_FLOOR_VALUES 911

/* Every tolerance entry must name a function that is actually in the table, and
 * must carry a positive bound. An entry for a function outside the freeze is
 * dead weight; a zero/negative one is the default written out longhand.
 *
 * What this does NOT check, deliberately: whether a bound is wider than the
 * function needs. There is no oracle for that here -- the measurement lives in
 * the generator, which is not in the tree. The bounds are documented with the
 * deviation each was measured from, and that citation is the audit trail. */
static ErrorNumber check_tol_table_is_live( void )
{
   int i, j;
   ErrorNumber errNb = TA_TEST_PASS;

   for( i = 0; i < NB_LEGACY_TOL; i++ )
   {
      int found = 0;
      for( j = 0; j < TA_LEGACY_NB_CASE; j++ )
         if( strcmp( LEGACY_TOL[i].func, TA_LEGACY_CASE[j].func ) == 0 )
         { found = 1; break; }
      if( !found )
      {
         printf( "Fail: LEGACY_TOL names [%s], which has no frozen case. A "
                 "tolerance for a function outside the freeze is dead weight; "
                 "remove it.\n", LEGACY_TOL[i].func );
         errNb = TA_REGTEST_LEGACY_DEAD_TOL;
      }
      if( !( LEGACY_TOL[i].absTol > 0.0 ) )
      {
         printf( "Fail: LEGACY_TOL[%s] is %g. A zero/negative tolerance is the "
                 "default -- delete the row instead, so the table only ever "
                 "lists real, authorized divergences.\n",
                 LEGACY_TOL[i].func, LEGACY_TOL[i].absTol );
         errNb = TA_REGTEST_LEGACY_DEAD_TOL;
      }
   }
   return errNb;
}

/* Count the distinct function names in the table (it is grouped by function,
 * but do not rely on that). */
static int legacy_distinct_funcs( void )
{
   int i, j, n = 0;
   for( i = 0; i < TA_LEGACY_NB_CASE; i++ )
   {
      int seen = 0;
      for( j = 0; j < i; j++ )
         if( strcmp( TA_LEGACY_CASE[i].func, TA_LEGACY_CASE[j].func ) == 0 )
         { seen = 1; break; }
      if( !seen ) n++;
   }
   return n;
}

/* The run compared what the table says it should have, and the table is still
 * the size it was frozen at. Without this the group passes while comparing
 * nothing: an empty table, a truncated one, or a comparison that stopped being
 * reached all look identical to "no failures". */
static ErrorNumber check_coverage( void )
{
   long expected = 0;
   int  i, funcs;

   for( i = 0; i < TA_LEGACY_NB_CASE; i++ )
      expected += (long)TA_LEGACY_CASE[i].nbSample;

   funcs = legacy_distinct_funcs();

   if( g_legacyRealCmp + g_legacyIntCmp != expected )
   {
      printf( "Fail: compared %ld value(s) but the table carries %ld. A sample "
              "was skipped without failing.\n",
              g_legacyRealCmp + g_legacyIntCmp, expected );
      return TA_REGTEST_LEGACY_VACUOUS;
   }
   if( g_legacyShapeCmp != TA_LEGACY_NB_CASE )
   {
      printf( "Fail: %ld case(s) reached the shape check, table has %d\n",
              g_legacyShapeCmp, TA_LEGACY_NB_CASE );
      return TA_REGTEST_LEGACY_VACUOUS;
   }
   if( TA_LEGACY_NB_CASE < LEGACY_FLOOR_CASES ||
       funcs < LEGACY_FLOOR_FUNCS ||
       expected < LEGACY_FLOOR_VALUES )
   {
      printf( "Fail: the frozen table shrank -- %d case(s)/%d function(s)/%ld "
              "value(s) against the %d/%d/%d it was frozen with. If that is "
              "intended, move the floor and say why.\n",
              TA_LEGACY_NB_CASE, funcs, expected,
              LEGACY_FLOOR_CASES, LEGACY_FLOOR_FUNCS, LEGACY_FLOOR_VALUES );
      return TA_REGTEST_LEGACY_VACUOUS;
   }
   return TA_TEST_PASS;
}

/**** Global functions definitions. ****/

ErrorNumber test_func_legacy( TA_History *history )
{
   ErrorNumber  errNb;
   int          i;
   unsigned int savedUnst[TA_FUNC_UNST_COUNT];
   unsigned int u;

   if( history->nbBars != 252 )
   {
      printf( "Fail: the frozen values were taken over the 252-bar series, "
              "got %u bars\n", history->nbBars );
      return TA_REGTEST_LEGACY_BAD_HISTORY;
   }

   errNb = check_tol_table_is_live();
   if( errNb != TA_TEST_PASS )
      return errNb;

   g_legacyRealCmp = g_legacyIntCmp = g_legacyShapeCmp = 0;

   /* The freeze was taken with every unstable period at 0 and the candle
    * settings at their defaults. DO_TEST resets compatibility between groups
    * but neither of those, and earlier groups do change them -- so establish
    * the state this gate needs rather than inheriting whatever ran before it.
    * Unstable periods are restored afterwards; candle settings are left at the
    * documented defaults, which is where a group that changes them is
    * responsible for leaving them anyway. */
   for( u = 0; u < TA_FUNC_UNST_COUNT; u++ )
   {
      savedUnst[u] = TA_GetUnstablePeriod( (TA_FuncUnstId)u );
      TA_SetUnstablePeriod( (TA_FuncUnstId)u, 0 );
   }
   TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );

   errNb = TA_TEST_PASS;
   for( i = 0; i < TA_LEGACY_NB_CASE; i++ )
   {
      errNb = do_test_legacy_case( history, &TA_LEGACY_CASE[i] );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: frozen v0.6.4 case #%d [%s]\n", i, TA_LEGACY_CASE[i].func );
         break;
      }
   }

   for( u = 0; u < TA_FUNC_UNST_COUNT; u++ )
      TA_SetUnstablePeriod( (TA_FuncUnstId)u, savedUnst[u] );

   if( errNb != TA_TEST_PASS )
      return errNb;

   return check_coverage();
}
