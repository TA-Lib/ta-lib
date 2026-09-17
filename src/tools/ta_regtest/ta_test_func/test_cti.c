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
 *  091626 KL     First version (proposal-drafts issue #74).
 *
 */

/* Description:
 *     Test TA_CTI, Ehlers' Correlation Trend Indicator.
 *
 *     LEG 2 IS WRITTEN FIRST AND IT IS THE POINT. y runs BACKWARD in time here
 *     (bars ago), because that is the orientation the O(1) window slide is
 *     written for, so the coefficient is negated once at the output. Drop that
 *     negation and the indicator inverts completely -- Pearson's r is odd in
 *     either variable -- while every magnitude property survives untouched.
 *
 *     MEASURED with exactly that mutation:
 *
 *       negation removed   leg 1 RED (worst diff 2.0, the entire range)
 *                          leg 2 RED (sign)
 *                          leg 3 RED (exact endpoints)
 *                          leg 5 GREEN  <-- |CTI| <= 1 cannot see it
 *                          leg 6 GREEN, leg 7 GREEN
 *
 *     Leg 5 staying green under a defect that reverses every value is the
 *     reason leg 2 exists as a separate assertion rather than as a bound.
 *
 *     Leg 4 is the #242 cancellation gate and is non-vacuous by a wide margin:
 *     on its corpus this implementation lands 5.551e-17 from a 60-digit
 *     reference while a literal transcription of the author's listing errs by
 *     2.864e-01 -- fifteen orders apart, on an indicator whose entire range is
 *     two wide.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define CTI_CAP 1100

/* Every leg below feeds its own corpus to the language servers and diffs
 * bit-for-bit (issue #427). Without it these vectors are checked against
 * in-process C and nothing else: the ramp, the endpoint set and the
 * cancellation corpus are the one body of data written specifically to break
 * this function, and a Rust/Java/C# port that got the sign or the O(1) slide
 * wrong would pass every assertion here while disagreeing with C.
 *
 * MEASURED that this compares rather than merely runs: telling the servers
 * period 21 for the cancellation leg while C computed period 20 fails with
 * "SV FAIL [CTI] (pipe 0, c): BITWISE mismatch vs in-process C,
 * begIdx 19/20 nbElem 21/20", exit 22.
 */
#define CTI_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_comparisons();                               \
         ErrorNumber svErr_ = server_verify(                                     \
            "CTI", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),                  \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         /* "No failure reported" and "nothing was compared" are the same        \
          * observation without this floor. Every leg here is a success case,    \
          * so the skip-on-reject path server_verify() takes for rejected        \
          * parameters is unreachable and the count must advance.  */            \
         if( server_verify_comparisons() == svCmp_ )                             \
         {                                                                       \
            printf( "CTI oracle [period %d]: server_verify compared no server "  \
                    "despite live pipes\n", (int)(period) );                     \
            return TA_CTI_VACUOUS;                                               \
         }                                                                       \
      }                                                                          \
   } while(0)

/* ==== generated by scratchpad/cti/mkcancel.py ==== */
/* #242 cancellation corpus: level 100, spread 1e-05, 40 bars.
 * The naive listing errs by 2.864e-01 absolute here; the reference
 * below is 60-digit decimal over the exact binary value of each double. */
static const double ctiCancelCorpus[] = {
   100.00000052728242, 99.99999972063162, 100.00000331676542,
   100.00000094869883, 99.99999872896572, 100.00000199852343,
   100.00000237232099, 99.99999993158009, 100.00000488002743,
   100.00000308037387, 100.00000100535377, 100.00000413847395,
   99.99999792227185, 100.00000472610638, 100.00000376223514,
   100.00000484775724, 99.99999540147753, 100.00000179882429,
   100.00000041738383, 100.00000164245195, 99.9999962106636,
   100.00000378821413, 100.0000040362304, 99.99999972475116,
   99.99999609294525, 100.00000323786804, 100.00000188249547,
   99.99999598157812, 99.99999975536775, 99.99999944905079,
   100.00000269534941, 100.00000142433277, 99.99999929568433,
   100.0000042203537, 100.00000167494639, 100.00000480511711,
   99.99999542360842, 99.99999618084148, 99.99999731524505,
   100.00000447450944,
};
/* period 20: 21 values */
static const double ctiCancelGolden[] = {
   0.04188697767321594, -0.1609529790917248, -0.13972795912991218,
   -0.004570546281627058, -0.08066290853068546, -0.31306516612322316,
   -0.24064790998780647, -0.20091833872852413, -0.370654659299611,
   -0.31425513456899035, -0.294917672600824, -0.23526001807554123,
   -0.12828623582924842, -0.25744914491221194, -0.0422099257555875,
   0.07752942283779674, 0.3300407782193946, 0.009707565037271146,
   -0.08032882030998995, -0.16043887061613263, -0.0051788511639493506,
};

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_ctiDiffCmp;
static int g_ctiSignCmp;
static int g_ctiEndpointCmp;
static int g_ctiCancelCmp;
static int g_ctiBoundCmp;
static int g_ctiFlatCmp;
static int g_ctiAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_cti_sign( void );
static ErrorNumber test_cti_differential( const TA_History *history );
static ErrorNumber test_cti_endpoints( void );
static ErrorNumber test_cti_cancellation( void );
static ErrorNumber test_cti_bounds( const TA_History *history );
static ErrorNumber test_cti_flat( void );
static ErrorNumber test_cti_aliasing( const TA_History *history );
static ErrorNumber test_cti_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_cti( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_ctiDiffCmp = g_ctiSignCmp = g_ctiEndpointCmp = g_ctiCancelCmp = 0;
   g_ctiBoundCmp = g_ctiFlatCmp = g_ctiAliasCmp = 0;

   err = test_cti_sign();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_endpoints();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_cancellation();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_bounds( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_flat();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_range( history );
   if( err != TA_TEST_PASS )
      return err;

   /* The corpus-independent legs carry literal counts; the ones that walk the
    * caller's history are floors. */
   if( g_ctiCancelCmp != 21 || g_ctiSignCmp == 0 || g_ctiEndpointCmp == 0
       || g_ctiDiffCmp == 0 || g_ctiBoundCmp == 0 || g_ctiFlatCmp == 0
       || g_ctiAliasCmp == 0 )
   {
      printf( "CTI Fail: coverage counters (diff %d, sign %d, endpoint %d, "
              "cancellation %d, bound %d, flat %d, alias %d) are not what this "
              "file was written with (>0, >0, >0, 21, >0, >0, >0)\n",
              g_ctiDiffCmp, g_ctiSignCmp, g_ctiEndpointCmp, g_ctiCancelCmp,
              g_ctiBoundCmp, g_ctiFlatCmp, g_ctiAliasCmp );
      return TA_CTI_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (2) THE SIGN. A rising series must give a positive coefficient.
 *
 * Written first because it is the one defect this function invites: the O(1)
 * slide is written for a bars-ago ramp, which runs backward in time, so the
 * coefficient has to be negated once. Without that negation every value is
 * exactly reversed -- and legs 5, 6 and 7 all stay green through it. MEASURED.
 */
static ErrorNumber test_cti_sign( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i;

   for( i = 0; i < 400; i++ )
      in[i] = 10.0 + 0.5*(double)i;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI sign Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiSignCmp++;
         if( !(out[i] > 0.0) )
         {
            printf( "CTI sign Fail [N=%d] bar %d: %.17g on a strictly rising "
                    "series. The ramp is bars-ago and runs backward, so the "
                    "coefficient must be negated once; a magnitude assertion "
                    "cannot see this\n", n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (1) Differential against shipped TA_CORREL over an ascending ramp, which is
 * what this function is by definition.
 *
 * The tolerance is MEASURED against the live kernel rather than carried in from
 * the proposal: a tolerance table dates the kernel it was measured on. Worst
 * absolute difference here is 5.893e-12 over a 400-bar random walk at periods
 * 2..60, so 1e-9 leaves three orders and still fails anything structural --
 * removing the negation moves it to 2.0.
 */
static ErrorNumber test_cti_differential( const TA_History *history )
{
   static TA_Real ramp[CTI_CAP], cti[CTI_CAP], cor[CTI_CAP];
   TA_Integer begC, nbC, begR, nbR;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( i = 0; i < nbBars; i++ )
      ramp[i] = (double)i;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n, &begC, &nbC, cti );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI differential Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      retCode = TA_CORREL( 0, nbBars-1, history->close, ramp, n,
                           &begR, &nbR, cor );
      if( retCode != TA_SUCCESS || begR != begC || nbR != nbC )
      {
         printf( "CTI differential Fail [N=%d]: CORREL rc=%d (%d,%d) vs "
                 "(%d,%d)\n", n, (int)retCode, begR, nbR, begC, nbC );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbC; i++ )
      {
         g_ctiDiffCmp++;
         if( !(fabs( cti[i] - cor[i] ) <= 1e-9) )
         {
            printf( "CTI differential Fail [N=%d] bar %d: %.17g against "
                    "TA_CORREL's %.17g, absolute %.3e over 1e-9\n",
                    n, begC+i, cti[i], cor[i], fabs(cti[i]-cor[i]) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Exact endpoints. A strictly linear rise is +1.0 exactly and a strictly
 * linear fall -1.0 exactly -- exact rather than toleranced because of the
 * clamp, which is also what makes this leg blind on its own to anything that
 * only moves the interior.
 */
static ErrorNumber test_cti_endpoints( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i, dir;
   double want;

   for( dir = 0; dir < 2; dir++ )
   {
      want = dir == 0 ? 1.0 : -1.0;
      for( i = 0; i < 400; i++ )
         in[i] = dir == 0 ? 10.0 + 0.5*(double)i : 10.0 - 0.5*(double)i;

      for( n = 2; n <= 60; n += 7 )
      {
         retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "CTI endpoint Fail [N=%d dir=%d]: rc=%d\n",
                    n, dir, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
         for( i = 0; i < nbElement; i++ )
         {
            g_ctiEndpointCmp++;
            if( out[i] != want )
            {
               printf( "CTI endpoint Fail [N=%d] bar %d: %.17g, expected "
                       "exactly %.1f on a perfectly linear series\n",
                       n, begIdx+i, out[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) The #242 cancellation gate.
 *
 * NON-VACUOUS BY FIFTEEN ORDERS. On this corpus -- a 1e2 price level with a
 * 1e-5 spread, the regime that made TA_CORREL return 0, -1 and -1.73 from
 * perfectly correlated inputs -- a literal transcription of the author's
 * listing errs by 2.864e-01 absolute against a 60-digit reference, on an
 * indicator whose entire range is two wide. This implementation lands
 * 5.551e-17. The gate is 1e-9, between them by six orders on the tight side
 * and nine on the loose one.
 *
 * Deliberately NOT gated: a 1e4 level with a 1e-6 spread. That regime is
 * representation-limited rather than algorithm-limited -- the input doubles no
 * longer carry the answer -- and no shift-and-reseed form, including shipped
 * TA_CORREL, does better there.
 */
static ErrorNumber test_cti_cancellation( void )
{
   static TA_Real out[64];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;
   double err;

   retCode = TA_CTI( 0, 39, ctiCancelCorpus, 20, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 19 || nbElement != 21 )
   {
      printf( "CTI cancellation Fail: rc=%d (%d,%d), expected (19,21)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   CTI_SERVER_VERIFY( 0, 39, 40, retCode, begIdx, nbElement,
                      ctiCancelCorpus, 20, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_ctiCancelCmp++;
      err = fabs( out[i] - ctiCancelGolden[i] );
      if( !(err <= 1e-9) )
      {
         printf( "CTI cancellation Fail at bar %d: %.17g against %.17g, "
                 "absolute %.3e over 1e-9. The naive listing errs by 2.864e-01 "
                 "here, so this is the #242 regime, not rounding\n",
                 begIdx+i, out[i], ctiCancelGolden[i], err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) |CTI| <= 1 exactly, over the caller's corpus. Exact because of the clamp.
 * On its own this leg is blind to a sign reversal -- see the header. */
static ErrorNumber test_cti_bounds( const TA_History *history )
{
   static TA_Real out[CTI_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI bounds Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      CTI_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                         history->close, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiBoundCmp++;
         if( !(fabs( out[i] ) <= 1.0) )
         {
            printf( "CTI bounds Fail [N=%d] bar %d: %.17g is outside [-1,1]\n",
                    n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) A window with no spread returns exactly 0.0.
 *
 * Non-vacuous by construction: the divisor is the window's own sum of squares,
 * so without the guard this is 0/0 and every value would be NaN, which fails
 * the equality. The choice of 0.0 follows TA_CORREL rather than the author's
 * listing, which holds the previous value -- holding would make this function
 * path-dependent, and a successful call may not emit NaN.
 */
static ErrorNumber test_cti_flat( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i;

   for( i = 0; i < 400; i++ )
      in[i] = 42.0;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI flat Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiFlatCmp++;
         if( out[i] != 0.0 )
         {
            printf( "CTI flat Fail [N=%d] bar %d: %.17g, expected exactly 0.0 "
                    "(NaN here means the zero-spread guard is gone)\n",
                    n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) In-place aliasing. The O(1) slide reads inReal[trailingIdx] on the bar
 * after this bar's store landed on that cell, which is the hazard
 * linearreg.c:107-109 documents; the trailing value is read before the output
 * write for exactly that reason.
 */
static ErrorNumber test_cti_aliasing( const TA_History *history )
{
   static TA_Real clean[CTI_CAP], alias[CTI_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n,
                        &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      /* The non-aliased call only. The aliased one below is a C-side memory
       * property; the servers are not asked to reproduce an in-place call. */
      CTI_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                         history->close, n, clean );

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_CTI( 0, nbBars-1, alias, n, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "CTI alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "CTI alias Fail [N=%d] bar %d: separate %.17g, in-place "
                    "%.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. TA_STABLE_EPSILON: the window is finite
 * but carried in a running accumulator whose shift, and whose rebuild phase,
 * are seeded from the call's own startIdx -- so two calls covering the same bar
 * from different starts rebuild on different bars and land a few ulp apart.
 * No unstable period: the rebuild re-derives the shift from the window itself.
 */
typedef struct { int period; const TA_Real *in; } CtiRangeParam;

static TA_RetCode ctiRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   CtiRangeParam *p = (CtiRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CTI_Lookback( p->period );
   return TA_CTI( startIdx, endIdx, p->in, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_cti_range( const TA_History *history )
{
   CtiRangeParam param;

   param.period = 20;
   param.in     = history->close;

   return doRangeTestEx( ctiRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
