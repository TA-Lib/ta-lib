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
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081026 MF   First version. Block-scan oracle for the rolling extremum
 *              family (issue #147).
 */

/* Description:
 *
 * The rolling extremum family -- MIN, MAX, MINMAX, MIDPOINT, MIDPRICE and
 * WILLR -- computes its batch output with a Van Herk / Gil-Werman block scan
 * (issue #147): one backward suffix pass over a block of `period` bars, one
 * forward prefix pass over the next block clamped to what remains, and a
 * combine pass. The window that produces each output is therefore assembled
 * from two halves whose boundary MOVES with the output index, and the last
 * block is short by an amount that depends on the range length modulo the
 * period.
 *
 * That gives the family a shape the rest of the suite does not probe:
 *
 *  - The boundary between the two halves moves with the output index, and the
 *    trailing block is short by (range length mod period). Nothing else varies
 *    the range length RELATIVE to the period, so the clamp's tail was reached
 *    only incidentally.
 *
 *  - The hardcoded tables pin values at one period on one 252-bar series; they
 *    cannot sweep. They also predate the rewrite, so they pin the answer, not
 *    the block structure that now produces it.
 *
 * So this file checks the family against a naive O(n*period) window scan -- the
 * definition, not a second clever algorithm -- over a spread of periods that
 * straddle the block-structure edges, and over range lengths chosen relative to
 * each period. It also pins the in-place claim ("input and output may alias")
 * that all six carry in their source comment and that only ACCBANDS was
 * otherwise tested for.
 *
 * What this file does NOT claim to be first at: the heap arm of the scratch.
 * `build_candidates` in test_variants.c probes every integer parameter at
 * {default, min, min+1, 14, min(max,60), max}, and period 60 already drives all
 * six past the 30-element stack buffer in both the TA_ and TA_S_ bodies; the
 * fuzz vectors reach 31 and 33 the same way. The periods here straddle 30
 * because the block structure changes there too, not because nothing else got
 * that far. The one path that genuinely had no coverage is the ALLOCATION
 * FAILURE unwind, and it still has none here -- nothing in the suite makes
 * TA_Malloc fail. That is gated in the generator instead, by
 * c_circbuf_alloc_failure_frees_the_circbufs_before_it.
 *
 * On signed zeros: -0.0 and +0.0 compare equal but differ in bits, so which
 * one a window's extremum keeps is a tie-break choice. The block scan's is
 * irregular by construction -- inside the backward suffix half the LATER bar
 * holds a tie, inside the forward prefix half the EARLIER one does, and across
 * the two halves the suffix wins -- so the naive oracle cannot reproduce it
 * without copying the implementation. The comparison here is bit-exact except
 * that two zeros of either sign are accepted for each other; the sign is
 * checked by the dedicated cross-tier arm in stream_verify, not here.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

#define RE_NB_BARS 400

/* Which member of the family a case drives. */
typedef enum
{
   RE_MIN,
   RE_MAX,
   RE_MINMAX,
   RE_MIDPOINT,
   RE_MIDPRICE,
   RE_WILLR,
   RE_NB_FUNC
} RollingFunc;

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/

static double reHigh[RE_NB_BARS];
static double reLow [RE_NB_BARS];
static double reClose[RE_NB_BARS];

static double reOut0[RE_NB_BARS];
static double reOut1[RE_NB_BARS];
static double reAlias[RE_NB_BARS];

static const char *const reName[RE_NB_FUNC] =
{
   "MIN", "MAX", "MINMAX", "MIDPOINT", "MIDPRICE", "WILLR"
};

/* Periods worth sweeping. 2 and 3 are the declared minimum and minimum+1;
 * 29/30/31/32 straddle the 30-element CIRCBUF stack buffer, so 31 and up are
 * the first values in the whole suite that reach TA_Malloc; 64/65 put a second
 * whole block inside the range; 199 leaves only a couple of blocks in 400 bars.
 */
static const int rePeriods[] = { 2, 3, 4, 5, 29, 30, 31, 32, 33, 64, 65, 127, 199 };
#define RE_NB_PERIODS ((int)(sizeof(rePeriods)/sizeof(rePeriods[0])))

/**** Global functions definitions.   ****/

/* Fill the three price series with a shape. The shapes matter because the
 * block scan is branchless -- it cannot degrade on input -- while the tie
 * policy and the trailing-block clamp can still be shape-sensitive.
 */
static void reBuildSeries( int shape )
{
   int i;

   for( i = 0; i < RE_NB_BARS; i++ )
   {
      double base;

      switch( shape )
      {
      case 0:  /* pseudo-random walk, no ties */
         base = 100.0 + (double)((i * 7919) % 1021) * 0.125;
         break;
      case 1:  /* monotone up: the extremum sits on one edge for every window */
         base = 10.0 + (double)i;
         break;
      case 2:  /* monotone down */
         base = 10.0 + (double)(RE_NB_BARS - i);
         break;
      case 3:  /* constant: every window is one big tie */
         base = 42.0;
         break;
      case 4:  /* plateaus: long runs of equal values, ties at every boundary */
         base = 50.0 + (double)(i / 7);
         break;
      default: /* zeros and negatives around the origin */
         base = (double)(((i * 31) % 11) - 5);
         break;
      }

      reClose[i] = base;
      reHigh[i]  = base + 1.5;
      reLow[i]   = base - 1.5;
   }
}

/* -0.0 and +0.0 are accepted for each other; everything else must match bit
 * for bit. See the signed-zero note at the top of the file.
 */
static int reSameValue( double a, double b )
{
   if( a == 0.0 && b == 0.0 )
      return 1;
   return memcmp( &a, &b, sizeof(double) ) == 0;
}

/* The definition, computed the slow way: scan the whole window for every
 * output. `wantMax` picks the extremum; both are produced in one pass so
 * MINMAX/MIDPOINT/MIDPRICE/WILLR can be assembled from them.
 */
static void reNaiveWindow( const double *src, int at, int period,
                           double *outMin, double *outMax )
{
   double lo = src[at - period + 1];
   double hi = lo;
   int k;

   for( k = at - period + 2; k <= at; k++ )
   {
      if( src[k] < lo ) lo = src[k];
      if( src[k] > hi ) hi = src[k];
   }
   *outMin = lo;
   *outMax = hi;
}

/* Drive one member of the family over [startIdx,endIdx] at `period`. */
static TA_RetCode reCall( RollingFunc which, int startIdx, int endIdx, int period,
                          const double *high, const double *low, const double *close,
                          int *outBegIdx, int *outNBElement,
                          double *out0, double *out1 )
{
   switch( which )
   {
   case RE_MIN:
      return TA_MIN( startIdx, endIdx, close, period, outBegIdx, outNBElement, out0 );
   case RE_MAX:
      return TA_MAX( startIdx, endIdx, close, period, outBegIdx, outNBElement, out0 );
   case RE_MINMAX:
      return TA_MINMAX( startIdx, endIdx, close, period, outBegIdx, outNBElement, out0, out1 );
   case RE_MIDPOINT:
      return TA_MIDPOINT( startIdx, endIdx, close, period, outBegIdx, outNBElement, out0 );
   case RE_MIDPRICE:
      return TA_MIDPRICE( startIdx, endIdx, high, low, period, outBegIdx, outNBElement, out0 );
   case RE_WILLR:
      return TA_WILLR( startIdx, endIdx, high, low, close, period, outBegIdx, outNBElement, out0 );
   default:
      return TA_BAD_PARAM;
   }
}

/* The expected outputs for bar `at`, from the naive window. */
static void reExpected( RollingFunc which, int at, int period,
                        double *exp0, double *exp1 )
{
   double lo, hi, hiOfHigh, loOfLow;

   switch( which )
   {
   case RE_MIN:
      reNaiveWindow( reClose, at, period, exp0, &hi );
      break;
   case RE_MAX:
      reNaiveWindow( reClose, at, period, &lo, exp0 );
      break;
   case RE_MINMAX:
      reNaiveWindow( reClose, at, period, exp0, exp1 );
      break;
   case RE_MIDPOINT:
      reNaiveWindow( reClose, at, period, &lo, &hi );
      *exp0 = (hi + lo) / 2.0;
      break;
   case RE_MIDPRICE:
      reNaiveWindow( reHigh, at, period, &lo, &hiOfHigh );
      reNaiveWindow( reLow,  at, period, &loOfLow, &hi );
      *exp0 = (hiOfHigh + loOfLow) / 2.0;
      break;
   case RE_WILLR:
      reNaiveWindow( reHigh, at, period, &lo, &hiOfHigh );
      reNaiveWindow( reLow,  at, period, &loOfLow, &hi );
      /* Spelled exactly as the implementation does it -- dividing by
       * (range / -100) is not bit-identical to dividing by the range and
       * then scaling, and this comparison is bit-exact. */
      {
         double diff = (hiOfHigh - loOfLow) / (-100.0);
         *exp0 = (diff != 0.0) ? (hiOfHigh - reClose[at]) / diff : 0.0;
      }
      break;
   default:
      *exp0 = 0.0;
      break;
   }
}

/* MINMAX is the only two-output member. */
static int reNbOutputs( RollingFunc which )
{
   return (which == RE_MINMAX) ? 2 : 1;
}

ErrorNumber test_func_rolling_extremum( TA_History *history )
{
   int shape, p, f, r, i;
   int startIdx, endIdx;
   int outBegIdx, outNBElement;
   TA_RetCode retCode;
   int nbCase = 0;
   int nbHeapCase = 0;

   (void)history;   /* Deterministic synthetic series only: the sweep needs
                     * more bars than the 252-bar reference history, and the
                     * shapes are chosen for the algorithm, not for realism. */

   for( shape = 0; shape < 6; shape++ )
   {
      reBuildSeries( shape );

      for( p = 0; p < RE_NB_PERIODS; p++ )
      {
         const int period = rePeriods[p];

         /* Range lengths relative to the period: exactly one output, one short
          * of a whole block, exactly a whole block, one past it, two blocks,
          * and the whole series. These are the cases where the prefix pass's
          * clamp to endIdx changes the tail.
          */
         const int lengths[] = { period, period + 1, 2*period - 1, 2*period,
                                 2*period + 1, 3*period + 2, RE_NB_BARS };

         for( r = 0; r < (int)(sizeof(lengths)/sizeof(lengths[0])); r++ )
         {
            int len = lengths[r];
            if( len > RE_NB_BARS ) continue;

            /* Two placements: anchored at 0 (startIdx below the lookback, so
             * the function moves it up) and floated (startIdx already past the
             * lookback, the case where the output index and the bar index
             * differ).
             */
            for( i = 0; i < 2; i++ )
            {
               startIdx = (i == 0) ? 0 : (RE_NB_BARS - len);
               endIdx   = startIdx + len - 1;
               if( startIdx < 0 || endIdx >= RE_NB_BARS ) continue;

               for( f = 0; f < RE_NB_FUNC; f++ )
               {
                  int nbOut = reNbOutputs( (RollingFunc)f );
                  int k;

                  memset( reOut0, 0, sizeof(reOut0) );
                  memset( reOut1, 0, sizeof(reOut1) );

                  retCode = reCall( (RollingFunc)f, startIdx, endIdx, period,
                                    reHigh, reLow, reClose,
                                    &outBegIdx, &outNBElement, reOut0, reOut1 );
                  if( retCode != TA_SUCCESS )
                  {
                     printf( "Fail: %s(shape=%d,period=%d,[%d,%d]) retCode=%d\n",
                             reName[f], shape, period, startIdx, endIdx, retCode );
                     return TA_REGTEST_ROLLING_EXTREMUM_CALL;
                  }

                  /* The lookback contract: the first output is the first bar
                   * with a whole window behind it, at or after startIdx. */
                  {
                     int expectBeg = (startIdx < period - 1) ? period - 1 : startIdx;
                     int expectNb  = endIdx - expectBeg + 1;
                     if( expectNb < 0 ) expectNb = 0;
                     if( expectNb > 0 && outBegIdx != expectBeg )
                     {
                        printf( "Fail: %s(shape=%d,period=%d,[%d,%d]) outBegIdx=%d, expected %d\n",
                                reName[f], shape, period, startIdx, endIdx, outBegIdx, expectBeg );
                        return TA_REGTEST_ROLLING_EXTREMUM_BEGIDX;
                     }
                     if( outNBElement != expectNb )
                     {
                        printf( "Fail: %s(shape=%d,period=%d,[%d,%d]) outNBElement=%d, expected %d\n",
                                reName[f], shape, period, startIdx, endIdx, outNBElement, expectNb );
                        return TA_REGTEST_ROLLING_EXTREMUM_NBELEM;
                     }
                  }

                  for( k = 0; k < outNBElement; k++ )
                  {
                     double exp0 = 0.0, exp1 = 0.0;
                     reExpected( (RollingFunc)f, outBegIdx + k, period, &exp0, &exp1 );

                     if( !reSameValue( reOut0[k], exp0 ) )
                     {
                        printf( "Fail: %s(shape=%d,period=%d,[%d,%d]) out0[%d]=%.17g, "
                                "naive window says %.17g\n",
                                reName[f], shape, period, startIdx, endIdx, k, reOut0[k], exp0 );
                        return TA_REGTEST_ROLLING_EXTREMUM_VALUE;
                     }
                     if( nbOut == 2 && !reSameValue( reOut1[k], exp1 ) )
                     {
                        printf( "Fail: %s(shape=%d,period=%d,[%d,%d]) out1[%d]=%.17g, "
                                "naive window says %.17g\n",
                                reName[f], shape, period, startIdx, endIdx, k, reOut1[k], exp1 );
                        return TA_REGTEST_ROLLING_EXTREMUM_VALUE;
                     }
                  }

                  nbCase++;
                  if( period > 30 ) nbHeapCase++;

                  /* In-place: every one of these carries "input and output may
                   * be the same buffer" in its source comment, and the block
                   * scan only holds because both scratch arrays are COPIES.
                   * Single-input members only -- aliasing one of a price
                   * bundle's arrays is not part of the claim.
                   */
                  if( f == RE_MIN || f == RE_MAX || f == RE_MIDPOINT )
                  {
                     memcpy( reAlias, reClose, sizeof(reAlias) );
                     retCode = reCall( (RollingFunc)f, startIdx, endIdx, period,
                                       reHigh, reLow, reAlias,
                                       &outBegIdx, &outNBElement, reAlias, NULL );
                     if( retCode != TA_SUCCESS )
                     {
                        printf( "Fail: %s in-place(shape=%d,period=%d,[%d,%d]) retCode=%d\n",
                                reName[f], shape, period, startIdx, endIdx, retCode );
                        return TA_REGTEST_ROLLING_EXTREMUM_CALL;
                     }
                     for( k = 0; k < outNBElement; k++ )
                     {
                        if( memcmp( &reAlias[k], &reOut0[k], sizeof(double) ) != 0 )
                        {
                           printf( "Fail: %s in-place(shape=%d,period=%d,[%d,%d]) out[%d]=%.17g "
                                   "differs from the separate-buffer result %.17g\n",
                                   reName[f], shape, period, startIdx, endIdx, k,
                                   reAlias[k], reOut0[k] );
                           return TA_REGTEST_ROLLING_EXTREMUM_INPLACE;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   /* Guard the sweep itself: the heap arm of the CIRCBUF scratch is the part
    * no other test reaches, so a change that quietly stopped reaching it would
    * make this file look green for the wrong reason.
    */
   if( nbHeapCase == 0 )
   {
      printf( "Fail: no case ran at a period above the CIRCBUF stack buffer\n" );
      return TA_REGTEST_ROLLING_EXTREMUM_VACUOUS;
   }

   printf( "\n  Rolling extremum block scan: %d case(s) vs a naive window scan, "
           "%d above the CIRCBUF stack buffer\n", nbCase, nbHeapCase );

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
/* None */
