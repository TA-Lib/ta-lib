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
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #372).
 */

/* Description:
 *
 *   Hand-written tests for TA_CUMSUM (Cumulative Sum).
 *
 *   The generic range sweep value-compares NOTHING here: path_dependent maps
 *   to TA_DO_NOT_COMPARE and from there to TA_STABLE_SKIP, which is the whole
 *   reason the flag exists (a sub-range call legitimately disagrees with the
 *   full-history one). These legs are the value coverage:
 *
 *   (1) C40 GOLDEN, bit-exact. The corpus published in full on the board card
 *       (TA-Lib/ta-lib-proposal-drafts#4): 40 bars of advance/decline counts,
 *       net = TA_SUB(A, D) built from the shipped primitive, then TA_CUMSUM
 *       memcmp'd against an in-test sequential +=. A left-to-right += has one
 *       correct answer in IEEE-754 doubles (ta4j 0.22.6 RunningTotalIndicator
 *       is bit-identical on this corpus, MEASURED on the card), so equality is
 *       the assertion; a tolerance could only hide a defect. Spot pins on the
 *       published values guard the reference loop itself from drifting.
 *
 *   (2) THE startIdx CONTRACT: TA_CUMSUM(s, e, x)[j] == sum(x[s .. s+j]),
 *       and in particular TA_CUMSUM(k, k, x)[0] == x[k], NOT sum(x[0..k]) --
 *       the accumulator re-seeds at the anchor, exactly as ad.c and obv.c do.
 *       This is the one place a well-meaning implementer silently diverges.
 *
 *   (3) EARN path_dependent, in test_supertrend.c Leg 6's shape: the flag is
 *       published; a sub-range call really does differ from the full-history
 *       one (non-vacuous -- or the flag disables a gate for nothing); and the
 *       sub-range call is bit-identical to a full-history call over the
 *       truncated input, proving a re-anchor rather than stale state.
 *
 *   (4) EDGES: startIdx == endIdx == 0; startIdx == N-1; outReal == inReal
 *       aliasing (the read precedes the store -- asserted, not assumed); an
 *       alternating +x/-x series whose total returns to exactly 0.0; and a
 *       series driven until total + x == total, asserting the plain
 *       uncompensated answer.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

#define CUMSUM_C40 40

/* Corpus C40, published in full on TA-Lib/ta-lib-proposal-drafts#4. */
static const TA_Real c40_A[CUMSUM_C40] =
{
   1800, 1500,  900, 2100, 1700, 1200,  600, 2400, 1900, 1100,
    800, 2200, 1600, 1000, 1400, 2000,  700, 1300, 2300, 1500,
    950, 1750, 2050, 1150,  850, 1950, 1450, 1250, 2150, 1650,
    750, 1850, 1050, 2250, 1350, 1550,  650, 2350, 1750, 1450
};
static const TA_Real c40_D[CUMSUM_C40] =
{
   1100, 1400, 2000,  800, 1200, 1700, 2300,  500, 1000, 1800,
   2100,  700, 1300, 1900, 1500,  900, 2200, 1600,  600, 1400,
   1950, 1150,  850, 1750, 2050,  950, 1450, 1650,  750, 1250,
   2150, 1050, 1850,  650, 1550, 1350, 2250,  550, 1150, 1450
};

/* Published spot values: first 8 of the full-range call, the final bar, and
 * the whole (3,7) slice that IS the anchoring convention. */
static const TA_Real c40_first8[8] = { 700, 800, -300, 1000, 1500, 1000, -700, 1200 };
static const TA_Real c40_final    = 4400;
static const TA_Real c40_slice37[5] = { 1300, 1800, 1300, -400, 1500 };

ErrorNumber test_func_cumsum( TA_History *history )
{
   static TA_Real net[CUMSUM_C40], out[CUMSUM_C40], ref[CUMSUM_C40], buf[CUMSUM_C40];
   TA_RetCode rc;
   TA_Integer beg, nb, beg2, nb2;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   double total;
   int i, k;

   (void)history;

   /* net = TA_SUB(A, D), through the shipped primitive. */
   rc = TA_SUB( 0, CUMSUM_C40 - 1, c40_A, c40_D, &beg, &nb, net );
   if( rc != TA_SUCCESS || beg != 0 || nb != CUMSUM_C40 )
   {
      printf( "CUMSUM Fail: TA_SUB retCode %d range (%d,%d)\n", (int)rc, (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* (1) C40 golden: full range, bit-exact against a sequential +=. */
   rc = TA_CUMSUM( 0, CUMSUM_C40 - 1, net, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 || nb != CUMSUM_C40 )
   {
      printf( "CUMSUM Fail: retCode %d range (%d,%d), expected (0,%d)\n",
              (int)rc, (int)beg, (int)nb, CUMSUM_C40 );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   total = 0.0;
   for( i = 0; i < CUMSUM_C40; i++ )
   {
      total += net[i];
      ref[i] = total;
      if( memcmp( &out[i], &ref[i], sizeof(TA_Real) ) != 0 )
      {
         printf( "CUMSUM Fail bar %d: %.17g != sequential += %.17g\n", i, out[i], ref[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   /* Spot pins on the PUBLISHED values guard the reference loop itself. */
   for( i = 0; i < 8; i++ )
   {
      if( out[i] != c40_first8[i] )
      {
         printf( "CUMSUM Fail published pin bar %d: %.17g != %.17g\n",
                 i, out[i], c40_first8[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   if( out[CUMSUM_C40-1] != c40_final )
   {
      printf( "CUMSUM Fail published final bar: %.17g != %.17g\n",
              out[CUMSUM_C40-1], c40_final );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (2) The startIdx contract: the (3,7) slice from the card, plus the
    * single-bar anchor. out[0] at bar 3 is net[3] = 1300, NOT the
    * full-history 1000 that sits in ref[3]. */
   rc = TA_CUMSUM( 3, 7, net, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 3 || nb != 5 )
   {
      printf( "CUMSUM Fail slice: retCode %d range (%d,%d), expected (3,5)\n",
              (int)rc, (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < 5; i++ )
   {
      if( out[i] != c40_slice37[i] )
      {
         printf( "CUMSUM Fail slice bar %d: %.17g != published %.17g\n",
                 3 + i, out[i], c40_slice37[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   for( k = 0; k < CUMSUM_C40; k += 13 )
   {
      rc = TA_CUMSUM( k, k, net, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != k || nb != 1 || out[0] != net[k] )
      {
         printf( "CUMSUM Fail anchor: (%d,%d) out %.17g, expected x[%d] = %.17g\n",
                 k, k, out[0], k, net[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* (3) Earn path_dependent. */
   if( TA_GetFuncHandle( "CUMSUM", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "CUMSUM Fail: cannot read its own TA_FuncInfo\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( !(funcInfo->flags & TA_FUNC_FLG_PATH_DEP) )
   {
      printf( "CUMSUM Fail: TA_FUNC_FLG_PATH_DEP is not published -- the range\n"
              "       sweep would value-compare a path-dependent function\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* Non-vacuity: the sub-range call must really differ from the full-history
    * one at the same bar... */
   rc = TA_CUMSUM( 3, 7, net, &beg, &nb, out );
   if( rc != TA_SUCCESS || out[0] == ref[3] )
   {
      printf( "CUMSUM Fail: sub-range agrees with full history (%.17g) -- the\n"
              "       path_dependent flag disables a gate for nothing\n", out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* ...and be bit-identical to a full-history call over the truncated input:
    * a re-anchor, not stale state. */
   rc = TA_CUMSUM( 0, 4, net + 3, &beg2, &nb2, buf );
   if( rc != TA_SUCCESS || nb2 != nb || memcmp( out, buf, (size_t)nb * sizeof(TA_Real) ) != 0 )
   {
      printf( "CUMSUM Fail: sub-range is not a re-anchor (nb %d vs %d)\n",
              (int)nb, (int)nb2 );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (4) Edges. */
   rc = TA_CUMSUM( 0, 0, net, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 || nb != 1 || out[0] != net[0] )
   {
      printf( "CUMSUM Fail edge (0,0)\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_CUMSUM( CUMSUM_C40 - 1, CUMSUM_C40 - 1, net, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != CUMSUM_C40 - 1 || nb != 1 || out[0] != net[CUMSUM_C40-1] )
   {
      printf( "CUMSUM Fail edge (N-1,N-1)\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* In-place aliasing: the read at bar i precedes the store into slot i. */
   for( i = 0; i < CUMSUM_C40; i++ )
      buf[i] = net[i];
   rc = TA_CUMSUM( 0, CUMSUM_C40 - 1, buf, &beg, &nb, buf );
   if( rc != TA_SUCCESS || memcmp( buf, ref, CUMSUM_C40 * sizeof(TA_Real) ) != 0 )
   {
      printf( "CUMSUM Fail: in-place call differs from separate-buffer call\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* Alternating +x/-x lands on exactly 0.0 -- the same-magnitude subtraction
    * is exact in IEEE-754. */
   for( i = 0; i < CUMSUM_C40; i++ )
      buf[i] = ( i & 1 ) ? -123.456 : 123.456;
   rc = TA_CUMSUM( 0, CUMSUM_C40 - 1, buf, &beg, &nb, out );
   if( rc != TA_SUCCESS || out[CUMSUM_C40-1] != 0.0 )
   {
      printf( "CUMSUM Fail: alternating series ends at %.17g, not exactly 0.0\n",
              out[CUMSUM_C40-1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   /* Absorption: once total is large enough, total + x == total, and the
    * PLAIN uncompensated answer is the contract (a Kahan variant would
    * diverge from both external oracles and from ad.c's convention). */
   buf[0] = 1e18;
   for( i = 1; i < CUMSUM_C40; i++ )
      buf[i] = 1.0;   /* 1e18 + 1.0 == 1e18 in double */
   rc = TA_CUMSUM( 0, CUMSUM_C40 - 1, buf, &beg, &nb, out );
   if( rc != TA_SUCCESS || out[CUMSUM_C40-1] != 1e18 )
   {
      printf( "CUMSUM Fail: absorption series ends at %.17g, expected the plain\n"
              "       uncompensated 1e18\n", out[CUMSUM_C40-1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}
