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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   First version (proposal MARKETFI).
 *  081326 CC   Leg (1) recaptured from live tulip_serve + pandas_serve
 *              oracles (issue #194) instead of a hand recompute of our own
 *              formula; now a genuine EXTERNAL-ORACLE leg. Leg (2) extended
 *              to server_verify (cross-language) and the streaming API
 *              (Open/Update/Peek/OpenAndFill), neither of which previously
 *              saw an exact-zero volume anywhere in the automated suite.
 *              Leg (4) swept via doRangeTestEx instead of one fixed split.
 */

/* Description:
 *
 *   Test TA_MARKETFI (Bill Williams' Market Facilitation Index).
 *
 *   MARKETFI is not covered by the --codegen sweep: that sweep diffs against
 *   the frozen ta_ref_serve, which predates this function. Coverage comes from
 *   this file plus server_verify / --xlang-hash.
 *
 *   Legs:
 *     1. EXTERNAL-ORACLE, relative tolerance. Six samples over the reference
 *        corpus, captured from two independent live oracles: Tulip Indicators
 *        0.9.2 (`ti_marketfi`, pinned SHA be18abb13e075ba866898dcc7cb52399603302a6)
 *        and pandas-ta-classic 0.6.52 (`ta.marketfi`), both driven via the
 *        private ../ta-lib-oracles servers (tulip_serve / pandas_serve) over
 *        this exact 252-bar corpus. Both oracles agree with the shipped
 *        implementation BITWISE across the full 252 bars, not just the six
 *        pinned below -- the pins are a cheap regression spot-check of that
 *        full-corpus agreement, which is not itself embedded in the tree.
 *        Tulip's own checked-in vector (tests/untest.txt:248) is NOT the
 *        source here: it prints three decimals and MARKETFI's values are
 *        ~1e-7, so that vector reads {0.000, 0.000, ...} and would pass any
 *        implementation returning approximately nothing -- the values below
 *        come from running the oracle live, not from that file. What
 *        constrains the formula, beyond the oracle agreement itself, is the
 *        mutation set below, which a wrong formula cannot survive.
 *     2. ZERO VOLUME. The one deliberate divergence from both references:
 *        Tulip and pandas both divide unconditionally (+/-Inf, or NaN on a
 *        zero range too); issue #112 requires a successful call to never
 *        emit either, so TA-Lib reports 0 instead. Neither oracle can verify
 *        this leg -- there is no zero-volume bar in the reference corpus.
 *        Also drives the SAME vectors through server_verify (every active
 *        language server, bitwise) and the streaming API (Open/Update/Peek/
 *        OpenAndFill) -- both otherwise never see an exact-zero volume,
 *        since fuzz_data.h's generators floor volume at 1000.0 in every
 *        shape and the 252-bar corpus has none either.
 *     3. ALIASING. outReal over each input in turn.
 *     4. RANGE INDEPENDENCE. A sub-range must equal the same slice of a full
 *        run, bitwise -- the function claims no start dependency. Swept via
 *        doRangeTestEx (TA_STABLE_EXACT), not one fixed split.
 *
 *   WHY NOT CHECK_EXPECTED_VALUE: checkExpectedValue() compares against an
 *   absolute 0.01 band (test_util.c -> TA_REAL_EQ) and takes no tolerance
 *   argument. MARKETFI's values on this corpus are ~1e-7, so that band is five
 *   orders of magnitude of slack: a constant 0.0, a sign flip, dropping the
 *   divisor, an off-by-one bar and pulling the wrong component out of the new
 *   HLV bundle ALL pass it. Each of those five was run against the relative
 *   check below and each is rejected. The NVI/PVI precedent in test_per_cv.c
 *   works only because those values are ~1000.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

/* Six samples over the 252-bar reference corpus in test_data.c, captured from
 * two independent LIVE oracle runs (not a hand recompute of our own formula):
 *   - tulip_serve: Tulip Indicators 0.9.2, `ti_marketfi`, pinned SHA
 *     be18abb13e075ba866898dcc7cb52399603302a6.
 *   - pandas_serve: pandas-ta-classic 0.6.52, `ta.marketfi` (pandas 3.0.3,
 *     numpy 2.5.1). Its `non_zero_range` epsilon nudge only fires on a flat
 *     (high==low) bar, none of which occur in this corpus.
 * Both oracles produced BITWISE-IDENTICAL output to TA_MARKETFI across all
 * 252 bars (not just these six) -- verified 2026-08-13, harness not checked
 * into this tree (see ../ta-lib-oracles). These six are a fixed regression
 * spot-check of that full-corpus agreement; all three sources (TA-Lib, Tulip,
 * pandas) agree exactly (0 ulp) at every one.
 */
typedef struct { int idx; double value; } MarketfiPin;

static const MarketfiPin marketfiPin[] = {
   {   0, 6.1312078479460458e-07 },
   {   2, 4.4499822000711998e-07 },
   {  50, 2.8885037550548817e-07 },
   { 125, 7.7560920577617332e-07 },
   { 200, 3.3306847495999938e-07 },
   { 251, 1.0033095279568003e-06 }
};
#define NB_MARKETFI_PIN ((int)(sizeof(marketfiPin)/sizeof(marketfiPin[0])))

/* Relative tolerance. The formula is one subtract and one divide with no
 * accumulation, so every backend evaluates it identically and the measured
 * agreement is 0 ulp -- 1e-12 is pure headroom against cross-platform rounding,
 * and still five decades tighter than any real formula error (a sign flip or a
 * missing divisor moves these values by >100%).
 */
#define MARKETFI_PIN_TOL 1e-12

/* Near-zero absolute floor. Unlike the PVO goldens this guards, MARKETFI's
 * whole-corpus range is [5.76e-08, 1.07e-06] -- every value is far BELOW 1, so
 * the floor must sit far below them or it, not the relative term, decides the
 * comparison. PVO's 1e-12 would be 1.7e-5 of the smallest value here. 1e-20 is
 * 1.7e-13 of it: inert on this corpus, and present only so a future golden
 * taken on a near-zero range bar cannot turn the check into a division blowup.
 */
#define MARKETFI_PIN_ABS 1e-20

static ErrorNumber test_marketfi_pinned     ( const TA_History *history );
static ErrorNumber test_marketfi_zero_volume( void );
static ErrorNumber test_marketfi_aliasing   ( const TA_History *history );
static ErrorNumber test_marketfi_subrange   ( const TA_History *history );

/**** Global functions definitions.   ****/

ErrorNumber test_func_marketfi( TA_History *history )
{
   ErrorNumber retValue;

   retValue = test_marketfi_pinned( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_marketfi_zero_volume();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_marketfi_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_marketfi_subrange( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions.    ****/

/* (1) PINNED VALUES: six samples over the reference corpus, relative tolerance. */
static ErrorNumber test_marketfi_pinned( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[2000];
   int k;

   retCode = TA_MARKETFI( 0, (int)history->nbBars - 1,
                          history->high, history->low, history->volume,
                          &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "Fail: TA_MARKETFI retCode %d\n", retCode );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* Lookback is zero, so the whole input range must come back. A function
    * that quietly shortened its output would otherwise pass every value
    * check below on the values it did emit.
    */
   if( begIdx != 0 || nbElement != (TA_Integer)history->nbBars )
   {
      printf( "Fail: TA_MARKETFI range: begIdx=%d nbElement=%d (want 0/%d)\n",
              (int)begIdx, (int)nbElement, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( k = 0; k < NB_MARKETFI_PIN; k++ )
   {
      int    idx  = marketfiPin[k].idx;
      double want = marketfiPin[k].value;
      double got  = out[idx];
      double err;
      const char *mode;

      if( !checkOracleValue( got, want,
                             MARKETFI_PIN_TOL, MARKETFI_PIN_ABS,
                             &err, &mode ) )
      {
         printf( "Fail: TA_MARKETFI pinned-value idx=%d got=%.17g want=%.17g (%s err %.3e)\n",
                 idx, got, want, mode, err );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   return TA_TEST_PASS;
}

/* (2) ZERO VOLUME: the deliberate divergence from Tulip and pandas, which both
 * divide anyway and emit +/-Inf (or NaN when the range is zero too). Issue #112
 * settled that a successful call never emits either.
 */
static ErrorNumber test_marketfi_zero_volume( void )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static const TA_Real h[4] = { 10.0, 11.0, 12.0, 13.0 };
   static const TA_Real l[4] = {  9.0, 10.0, 11.0, 13.0 };
   static const TA_Real v[4] = { 100.0, 0.0, 200.0, 0.0 };
   TA_Real out[4];
   int i;

   retCode = TA_MARKETFI( 0, 3, h, l, v, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || nbElement != 4 )
   {
      printf( "Fail: TA_MARKETFI zero-volume retCode %d nbElement %d\n",
              retCode, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( i = 0; i < 4; i++ )
   {
      if( !TA_IS_FINITE( out[i] ) )
      {
         printf( "Fail: TA_MARKETFI zero-volume out[%d] = %f, expected finite\n",
                 i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   /* Bar 1 has volume 0 with a real range; bar 3 has volume 0 and a zero range,
    * which is the 0/0 that would produce NaN rather than Inf. Both report 0.
    *
    * Compared BITWISE against +0.0 rather than with `!= 0.0`, which is also
    * false for -0.0 and would accept a guard that wrote the wrong zero. That
    * is the issue #147 class, and it is reachable here: the guard's value is a
    * literal in the generated C, so nothing but an edit to the guard can move
    * it -- which is precisely what this leg exists to catch.
    *
    * Worth recording for whoever mutates this next: -0.0 cannot be introduced
    * through ta_codegen/input/marketfi/marketfi.c. Writing it there and
    * regenerating yields `= 0.0;` in src/ta_func/ta_MARKETFI.c -- the literal
    * is normalised on the way through -- so a sabotage run at the input tier
    * silently tests nothing and reads as "this check has no discriminating
    * power". Patch the generated C to prove this leg bites.
    */
   {
      const TA_Real posZero = 0.0;
      if( memcmp( &out[1], &posZero, sizeof(TA_Real) ) != 0 ||
          memcmp( &out[3], &posZero, sizeof(TA_Real) ) != 0 )
      {
         printf( "Fail: TA_MARKETFI zero-volume out[1]=%.17g out[3]=%.17g, "
                 "expected exactly +0.0 (a -0.0 here passes a != 0.0 check)\n",
                 out[1], out[3] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   /* The traded bars must be unaffected by the guard. */
   if( out[0] != (1.0/100.0) || out[2] != (1.0/200.0) )
   {
      printf( "Fail: TA_MARKETFI zero-volume perturbed a traded bar: %.17g %.17g\n",
              out[0], out[2] );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* Cross-language: the zero-volume guard must fire identically on every
    * language server. Nothing else sends an exact-zero volume anywhere:
    * fuzz_data.h's generators keep volume >= 1000.0 in every shape, and the
    * 252-bar reference corpus has none either (min 2,658,400), so without
    * this call the guard translation in Rust/Java/C# would be unverified.
    */
   if( server_verify_active() )
   {
      ErrorNumber e = server_verify( "MARKETFI", 0, 3, 4,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ h, l, v, NULL },
                         NULL, 0,
                         (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
   }

   /* Streaming: OpenAndFill shares the batch loop (TA_MARKETFI_OpenPass) and
    * is already covered transitively, but Update/Peek run a SEPARATE code
    * path (TA_MARKETFI_StepInternal) that carries its own copy of the guard.
    * Open warms up on bar 0 alone so bars 1-3 are driven through Update/Peek,
    * not the batch loop.
    */
   {
      TA_MARKETFI_Stream *stream = NULL;
      TA_Real streamOut, peekOut;
      TA_Integer sBeg, sNb;
      TA_Real fillOut[4];
      int i;

      retCode = TA_MARKETFI_OpenAndFill( &stream, h, l, v, 4, &sBeg, &sNb, fillOut );
      if( retCode != TA_SUCCESS || sBeg != begIdx || sNb != nbElement )
      {
         printf( "Fail: TA_MARKETFI_OpenAndFill zero-volume retCode %d beg %d nb %d\n",
                 retCode, (int)sBeg, (int)sNb );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      for( i = 0; i < 4; i++ )
      {
         if( fillOut[i] != out[i] )
         {
            printf( "Fail: TA_MARKETFI_OpenAndFill zero-volume at %d: %.17g vs batch %.17g\n",
                    i, fillOut[i], out[i] );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
      }
      TA_MARKETFI_Close( stream );

      stream = NULL;
      retCode = TA_MARKETFI_Open( &stream, h, l, v, 1, &streamOut );
      if( retCode != TA_SUCCESS || streamOut != out[0] )
      {
         printf( "Fail: TA_MARKETFI_Open zero-volume: %.17g vs batch %.17g (retCode %d)\n",
                 streamOut, out[0], retCode );
         if( stream ) TA_MARKETFI_Close( stream );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      for( i = 1; i < 4; i++ )
      {
         retCode = TA_MARKETFI_Peek( stream, h[i], l[i], v[i], &peekOut );
         if( retCode != TA_SUCCESS )
         {
            TA_MARKETFI_Close( stream );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         retCode = TA_MARKETFI_Update( stream, h[i], l[i], v[i], &streamOut );
         if( retCode != TA_SUCCESS || peekOut != streamOut || streamOut != out[i] )
         {
            printf( "Fail: TA_MARKETFI stream zero-volume at %d: peek=%.17g update=%.17g "
                    "batch=%.17g (retCode %d)\n", i, peekOut, streamOut, out[i], retCode );
            TA_MARKETFI_Close( stream );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
      }
      TA_MARKETFI_Close( stream );
   }

   return TA_TEST_PASS;
}

/* (3) ALIASING: outReal written over each input in turn. The body reads all
 * three components of a bar before writing that bar's output, so aliasing is
 * safe -- but the ordering is what makes it safe, and a future rewrite that
 * hoisted the store above either read would corrupt the divisor without
 * changing any value on a non-aliased call.
 */
static ErrorNumber test_marketfi_aliasing( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   static TA_Real ref[2000];
   static TA_Real buf[2000];
   int n = (int)history->nbBars;
   int which, i;

   retCode = TA_MARKETFI( 0, n - 1, history->high, history->low, history->volume,
                          &refBeg, &refNb, ref );
   if( retCode != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   for( which = 0; which < 3; which++ )
   {
      const TA_Real *h = history->high;
      const TA_Real *l = history->low;
      const TA_Real *v = history->volume;

      /* Copy the aliased input into the output buffer and point that
       * parameter at it, so out and that input are the same storage.
       */
      switch( which )
      {
      case 0: memcpy( buf, history->high,   (size_t)n * sizeof(TA_Real) ); h = buf; break;
      case 1: memcpy( buf, history->low,    (size_t)n * sizeof(TA_Real) ); l = buf; break;
      default:memcpy( buf, history->volume, (size_t)n * sizeof(TA_Real) ); v = buf; break;
      }

      retCode = TA_MARKETFI( 0, n - 1, h, l, v, &begIdx, &nbElement, buf );
      if( retCode != TA_SUCCESS || begIdx != refBeg || nbElement != refNb )
      {
         printf( "Fail: TA_MARKETFI aliasing input %d: retCode %d\n", which, retCode );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( buf[i] != ref[i] )
         {
            printf( "Fail: TA_MARKETFI aliasing input %d at %d: %.17g vs %.17g\n",
                    which, i, buf[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) RANGE INDEPENDENCE: the yaml carries no start_dependent flag, which is a
 * claim that a sub-range returns the same numbers as the matching slice of a
 * full run. Bitwise, since there is no accumulation that could legitimately
 * reassociate. Swept via the shared doRangeTestEx primitive (the same one
 * test_cmf.c's test_cmf_range and test_cmou.c's test_cmou_range use) instead
 * of one hand-picked split, so this leg is not blind to an off-by-one that
 * only shows up at a different startIdx/endIdx combination. MARKETFI is a
 * fresh-recomputed, lookback-0 finite window with no unstable period, so it
 * takes TA_STABLE_EXACT / TA_TEST_UNST_NONE, same class as CMF's own
 * TA_STABLE_EPSILON sibling minus the accumulator slack MARKETFI has none of.
 */
typedef struct
{
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *volume;
} MarketfiRangeParam;

static TA_RetCode marketfiRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                             TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                             TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                             TA_Integer *lookback, void *opaqueData,
                                             unsigned int outputNb, unsigned int *isOutputInteger )
{
   MarketfiRangeParam *p = (MarketfiRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_MARKETFI_Lookback();
   return TA_MARKETFI( startIdx, endIdx, p->high, p->low, p->volume,
                       outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_marketfi_subrange( const TA_History *history )
{
   MarketfiRangeParam param;

   param.high   = history->high;
   param.low    = history->low;
   param.volume = history->volume;

   return doRangeTestEx( marketfiRangeTestFunction,
                         TA_STABLE_EXACT, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
