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
 *  KL       Kevin
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   First version (proposal MARKETFI).
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
 *     1. PINNED VALUES, relative tolerance. Six samples over the reference
 *        corpus, recomputed from test_data.c at full precision.
 *
 *        NOT an external oracle, and deliberately not called one. These
 *        constrain the implementation against the formula as written here;
 *        they cannot catch the formula itself being wrong, because both sides
 *        are this definition. No usable external vector exists: Tulip ships
 *        one (tests/untest.txt:248) but its expected column is
 *        {0.000, 0.000, ...} for all 15 bars -- MARKETFI's values are ~1e-7
 *        and that file prints three decimals, so the published vector passes
 *        for any implementation returning approximately nothing. What does
 *        constrain the formula here is the mutation set below, which a wrong
 *        formula cannot survive.
 *     2. ZERO VOLUME. The one deliberate divergence from both references.
 *     3. ALIASING. outReal over each input in turn.
 *     4. RANGE INDEPENDENCE. A sub-range must equal the same slice of a full
 *        run, bitwise -- the function claims no start dependency.
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

/* Pinned samples, recomputed directly from the 252-bar reference corpus in
 * test_data.c at full %.17g precision -- this definition evaluated by hand,
 * not any third party's output. Measured agreement with the shipped
 * implementation is exact (0 ulp) at all six.
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
    * Compared BITWISE against +0.0, not with `!= 0.0`, which is true for -0.0
    * as well and would accept a guard that wrote the wrong zero. The value
    * here is a literal, so only a change to the guard can move it -- which is
    * exactly what this leg exists to catch. Same class as issue #147.
    */
   {
      const TA_Real posZero = 0.0;
      if( memcmp( &out[1], &posZero, sizeof(TA_Real) ) != 0 ||
          memcmp( &out[3], &posZero, sizeof(TA_Real) ) != 0 )
      {
         printf( "Fail: TA_MARKETFI zero-volume out[1]=%.17g out[3]=%.17g, "
                 "expected exactly +0.0 (a -0.0 here would pass a != 0.0 check)\n",
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
 * reassociate.
 */
static ErrorNumber test_marketfi_subrange( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer fullBeg, fullNb, subBeg, subNb;
   static TA_Real full[2000];
   static TA_Real sub[2000];
   int n = (int)history->nbBars;
   int start = n / 3;
   int end   = ( 2 * n ) / 3;
   int i;

   retCode = TA_MARKETFI( 0, n - 1, history->high, history->low, history->volume,
                          &fullBeg, &fullNb, full );
   if( retCode != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   retCode = TA_MARKETFI( start, end, history->high, history->low, history->volume,
                          &subBeg, &subNb, sub );
   if( retCode != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   if( subBeg != start || subNb != ( end - start + 1 ) )
   {
      printf( "Fail: TA_MARKETFI subrange: begIdx=%d nbElement=%d (want %d/%d)\n",
              (int)subBeg, (int)subNb, start, end - start + 1 );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   for( i = 0; i < subNb; i++ )
   {
      if( sub[i] != full[start + i] )
      {
         printf( "Fail: TA_MARKETFI subrange at %d: %.17g vs %.17g\n",
                 i, sub[i], full[start + i] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   return TA_TEST_PASS;
}
