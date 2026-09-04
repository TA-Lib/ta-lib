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
 *  090426 MF,CC  First version. FOSC (Forecast Oscillator). See issue #345.
 */

/* Description:
 *
 *   Regression tests for FOSC, the Forecast Oscillator (issue #345):
 *   100*(close - TSF[t-1])/close, where the forecast is the one made one bar
 *   EARLIER for the bar being emitted.
 *
 *   Coverage:
 *     (1) Published vectors. Tulip Indicators ships two checked-in FOSC
 *         expected-output vectors, one of them transcribed from Achelis,
 *         "Technical Analysis from A to Z", page 147 -- a printed book table,
 *         so the strongest provenance available for the FORMULA. Compared at
 *         half the last printed digit.
 *     (2) External-oracle values at full precision on the standard 252-bar
 *         close series, captured live from two independent libraries. This is
 *         the leg that would go red on the unlagged variant (which several
 *         vendors publish as "CFO"): dropping the lag moves every value by
 *         O(1), ten orders above the bound here. The same call is checked
 *         in-place (outReal == inReal) and, under --codegen, bit-for-bit on
 *         every language server.
 *     (3) Bit-exact differential against the shipped TA_TSF. FOSC fuses TSF's
 *         window arithmetic rather than calling it; this proves the fusion is
 *         exact, not merely close, over period 2..40 x 35 start indices.
 *     (4) The divide-by-zero guard: a zero (and negative-zero) close reports
 *         exactly 0.0 instead of +-inf / NaN.
 *     (5) Degenerate ranges, and the generic startIdx/endIdx range sweep.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_reference.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* (1) Tulip Indicators' own checked-in FOSC vectors, both at period 5,
 * transcribed unchanged from the pinned source tree
 * (github.com/TulipCharts/tulipindicators, SHA
 * be18abb13e075ba866898dcc7cb52399603302a6, TI_VERSION 0.9.2):
 *
 *   tests/atoz.txt:118, tagged "#page 147" -- Achelis, "Technical Analysis
 *   from A to Z", 2nd ed., page 147. A printed book table.
 *
 *   tests/untest.txt:199 -- Tulip's own regression vector.
 *
 * Tolerance is half of the last digit each vector is PRINTED at (4 decimals /
 * 3 decimals): the published numbers carry no more information than that, so a
 * tighter bound would be testing the transcription, not the formula. It still
 * separates the two variants by three orders -- the unlagged "CFO" differs from
 * these by ~1 whole unit. */
static const TA_Real foscAtozIn[] =
   { 16.4444, 16.3333, 16.3333, 16.4444, 16.4444, 16.0556,
     16.1667, 15.9444, 16.3889, 16.5556, 16.6111 };
static const TA_Real foscAtozExp[] =
   { -2.3524, -0.1374, -0.8020, 3.4237, 2.5506, 0.1336 };
#define FOSC_ATOZ_TOL 5e-5

static const TA_Real foscUntestIn[] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const TA_Real foscUntestExp[] =
   { -1.287, -1.659, 1.035, 1.028, -0.100, 0.600, 0.648, 0.083, 0.157, -1.583 };
#define FOSC_UNTEST_TOL 5e-4

/* (2) Full-precision external-oracle values for FOSC on the standard 252-bar
 * close series (TA_SREF_close_daily_ref_0_PRIV), captured 2026-09-04. `idx` is
 * the OUTPUT-array index; outBegIdx == period for every period. Two independent
 * implementations, and they were captured by RUNNING each library, not by
 * re-deriving the formula:
 *
 *   tulip   Tulip Indicators 0.9.2 (C, pinned SHA
 *           be18abb13e075ba866898dcc7cb52399603302a6, TI_BUILD 1645649572),
 *           `ti_fosc`, driven through ta-lib-oracles/tulip_serve with input and
 *           output carried as hex-of-IEEE-bits so the capture is lossless.
 *
 *   ts      trading-signals 8.3.0 (TypeScript, MIT), `CFO`, via
 *           ta-lib-oracles/trading_signals_serve/capture.mjs. It spells the
 *           indicator CFO but computes the LAGGED form; it also recomputes the
 *           regression from a fresh window on every bar, carrying no running
 *           sums at all, so it agrees with neither of the others by construction.
 *
 * Both report outBegIdx == period and the same element count, so the lookback
 * is externally confirmed and not just self-consistent. This pair is what would
 * go red on the unlagged variant several vendors also publish under the name
 * "CFO": dropping the lag moves every value by O(1), ten orders above the bound
 * below.
 *
 * THE BOUND IS ABSOLUTE, AND THAT IS NOT A ROUNDED-UP RELATIVE BOUND.
 * FOSC's numerator, close - TSF, is a catastrophic cancellation of two
 * near-equal price-scale quantities, so relative error is amplified by
 * close/(close-TSF) and is unbounded at a zero crossing: over these three
 * periods the worst relative gap to Tulip is 2.3e-9 while the worst absolute
 * gap is 8.9e-12 (trading-signals: 3.1e-12). A relative gate would be red on
 * ~37% of the period-5 elements for no reason connected to correctness.
 *
 * The residual gap is accumulation order, not formula. Against the exact
 * rational value of each output, computed from the same double inputs, the
 * worst error is 3.1e-12 for TA_FOSC and 1.2e-11 for Tulip -- Tulip never
 * rebuilds its running sums, while TA_FOSC inherits TSF's re-anchoring (#254).
 * 1e-10 leaves an order of headroom over the measured worst case and still sits
 * ten orders below the O(1) separation from the unlagged variant. */
static const struct { int period; int idx; double tulip; double ts; } foscOracle[] =
{
   {   5,    0, -0.7820343461030329   , -0.7820343461030179    },
   {   5,    1, -1.8539932994703714   , -1.853993299470433     },
   {   5,    2, -0.08948787061992905  , -0.08948787061999033   },
   {   5,   41, -1.6147200349956117   , -1.6147200349956117    },
   {   5,   82, 1.7742304349816402    , 1.7742304349814584     },
   {   5,  123, 0.4492537313445379    , 0.44925373134328644    },
   {   5,  164, 1.1997516684801903    , 1.1997516684774112     },
   {   5,  205, -3.378335618597047    , -3.3783356186056355    },
   {   5,  245, -1.1429885057352827   , -1.1429885057470697    },
   {   5,  246, -1.083711875393563    , -1.0837118754054988    },

   {  14,    0, -3.742120516845753    , -3.7421205168458       },
   {  14,    1, -0.38721715713582444  , -0.38721715713582444   },
   {  14,    2, -4.3815530552113335   , -4.381553055211222     },
   {  14,   39, -7.978680002632227    , -7.97868000263209      },
   {  14,   79, -0.9065818877139841   , -0.9065818877139483    },
   {  14,  119, -3.4280296770211796   , -3.428029677020908     },
   {  14,  158, 3.2936043548283656    , 3.293604354828829      },
   {  14,  198, 0.7227591107003658    , 0.7227591107014678     },
   {  14,  236, 0.055475558922640715  , 0.05547555892377758    },
   {  14,  237, -1.2222689704856493   , -1.2222689704843979    },

   {  23,    0, -6.611933901614386    , -6.611933901614402     },
   {  23,    1, -4.002934841778904    , -4.002934841778972     },
   {  23,    2, -5.330030418389548    , -5.3300304183893905    },
   {  23,   38, 1.054733034099452     , 1.0547330340992593     },
   {  23,   76, -1.0553568814438217   , -1.0553568814438579    },
   {  23,  114, -7.773110273616304    , -7.773110273615753     },
   {  23,  152, 1.211091387632196     , 1.2110913876331555     },
   {  23,  190, 2.7474844798682048    , 2.747484479870267      },
   {  23,  227, -1.782054427334196    , -1.7820544273318308    },
   {  23,  228, -2.1098079191380332   , -2.1098079191356485    },
};
#define NB_FOSC_ORACLE (sizeof(foscOracle)/sizeof(foscOracle[0]))
#define FOSC_ORACLE_TOL 1e-10

/* (3) The differential sweep is a fixed grid, so its comparison count is a
 * constant: a drop means the sweep stopped reaching bars, not that FOSC got
 * better. */
#define FOSC_DIFF_CHECKS 309331

/**** Local functions declarations. ****/
static ErrorNumber test_fosc_published_vectors( void );
static ErrorNumber test_fosc_oracle( const TA_History *history );
static ErrorNumber test_fosc_tsf_differential( const TA_History *history );
static ErrorNumber test_fosc_zero_close( void );
static ErrorNumber test_fosc_degenerate_ranges( const TA_History *history );
static ErrorNumber test_fosc_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_fosc( TA_History *history )
{
   ErrorNumber retValue;

   /* FOSC has no unstable period; a leftover global setting must not reach it. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_fosc_published_vectors();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fosc_oracle( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fosc_tsf_differential( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fosc_zero_close();
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fosc_degenerate_ranges( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   retValue = test_fosc_range( history );
   if( retValue != TA_TEST_PASS ) return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

static ErrorNumber fosc_check_vector( const char *tag,
                                      const TA_Real *in, int nbIn,
                                      const TA_Real *expected, int nbExpected,
                                      double tol )
{
   TA_Real out[64];
   TA_Integer beg, nb;
   TA_RetCode retCode;
   int i;

   retCode = TA_FOSC( 0, nbIn - 1, in, 5, &beg, &nb, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "Fail: FOSC %s returned retCode=%d\n", tag, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != 5 || nb != nbExpected )
   {
      printf( "Fail: FOSC %s beg=%d nb=%d, expected 5/%d\n",
              tag, (int)beg, (int)nb, nbExpected );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbExpected; i++ )
   {
      double diff = fabs( out[i] - expected[i] );
      if( isnan( out[i] ) || diff > tol )
      {
         printf( "Fail: FOSC %s at published value %d: got %.17g expected %.17g "
                 "(|diff|=%.3e > %.3e)\n",
                 tag, i, out[i], expected[i], diff, tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (1) The two published vectors. */
static ErrorNumber test_fosc_published_vectors( void )
{
   ErrorNumber retValue;

   retValue = fosc_check_vector( "book vector (Achelis p.147, via tulip atoz.txt)",
                                 foscAtozIn, (int)(sizeof(foscAtozIn)/sizeof(TA_Real)),
                                 foscAtozExp, (int)(sizeof(foscAtozExp)/sizeof(TA_Real)),
                                 FOSC_ATOZ_TOL );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return fosc_check_vector( "tulip vector (untest.txt fosc)",
                             foscUntestIn, (int)(sizeof(foscUntestIn)/sizeof(TA_Real)),
                             foscUntestExp, (int)(sizeof(foscUntestExp)/sizeof(TA_Real)),
                             FOSC_UNTEST_TOL );
}

/* (2) Full-precision oracle values + in-place + cross-language bitwise. */
static ErrorNumber test_fosc_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   static TA_Real out[OUT_CAP];
   static TA_Real inplace[OUT_CAP];
   int endIdx = (int)history->nbBars - 1;
   int period = 0;
   unsigned int k;
   int i;

   begIdx = nbElement = 0;
   for( k = 0; k < NB_FOSC_ORACLE; k++ )
   {
      if( foscOracle[k].period != period )
      {
         period = foscOracle[k].period;
         retCode = TA_FOSC( 0, endIdx, history->close, period,
                            &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "FOSC oracle Fail: period %d retCode = %d\n", period, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( begIdx != period || nbElement != (int)history->nbBars - period )
         {
            printf( "FOSC oracle Fail: period %d shape got (%d,%d) expected (%d,%d)\n",
                    period, begIdx, nbElement, period, (int)history->nbBars - period );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* In-place (outReal == inReal) is bit-for-bit the separate-buffer
          * result: the departing window value is cached before the write that
          * lands on the very cell the next bar drops. */
         for( i = 0; i < (int)history->nbBars; i++ )
            inplace[i] = history->close[i];
         retCode = TA_FOSC( 0, endIdx, inplace, period, &begIdx2, &nbElement2, inplace );
         if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement ||
             memcmp( out, inplace, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
         {
            printf( "FOSC in-place Fail: period %d rc=%d shape (%d,%d) vs (%d,%d)\n",
                    period, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
            for( i = 0; i < nbElement; i++ )
               if( out[i] != inplace[i] )
               {
                  printf( "   first bit mismatch at out[%d] separate=%.17g inplace=%.17g\n",
                          i, out[i], inplace[i] );
                  break;
               }
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( server_verify_active() )
         {
            ErrorNumber e = server_verify( "FOSC", 0, endIdx, history->nbBars,
                                           retCode, begIdx, nbElement,
                                           (const TA_Real*[]){ history->close, NULL },
                                           (double[]){ (double)period }, 1,
                                           (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }

      {
         int idx = foscOracle[k].idx;
         double got = out[idx];
         int o;

         for( o = 0; o < 2; o++ )
         {
            double want = o ? foscOracle[k].ts : foscOracle[k].tulip;
            double diff = fabs( got - want );

            if( isnan( got ) || diff > FOSC_ORACLE_TOL )
            {
               printf( "FOSC oracle Fail at period %d out[%d] vs %s: got %.17g "
                       "expected %.17g (|diff|=%.3e > %.3e)\n",
                       period, idx, o ? "trading-signals" : "tulip",
                       got, want, diff, FOSC_ORACLE_TOL );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) FOSC fuses TA_TSF's window arithmetic instead of calling it. The fused
 * loop must be BIT-identical to TA_TSF anchored one bar earlier -- which is
 * only true when FOSC primes its window at exactly that bar, because TSF's
 * O(1) sums depend on where the caller started. A ~1e-13 mismatch here reads
 * like a tolerance problem and is a design misalignment. */
static TA_RetCode fosc_reference( int startIdx, int endIdx, const TA_Real *inReal,
                                  int period, TA_Integer *outBegIdx,
                                  TA_Integer *outNBElement, TA_Real *outReal )
{
   static TA_Real tsf[OUT_CAP];
   TA_Integer tsfBeg, tsfNb;
   TA_RetCode retCode;
   int i;
   int begIdx = startIdx < period ? period : startIdx;

   if( begIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   retCode = TA_TSF( begIdx - 1, endIdx - 1, inReal, period, &tsfBeg, &tsfNb, tsf );
   if( retCode != TA_SUCCESS )
      return retCode;

   for( i = 0; i < tsfNb; i++ )
   {
      double closeValue = inReal[begIdx + i];
      outReal[i] = ( closeValue != 0.0 )
                 ? 100.0 * ( closeValue - tsf[i] ) / closeValue
                 : 0.0;
   }
   *outBegIdx = begIdx;
   *outNBElement = tsfNb;
   return TA_SUCCESS;
}

static ErrorNumber test_fosc_tsf_differential( const TA_History *history )
{
   static TA_Real fused[OUT_CAP], composed[OUT_CAP];
   TA_Integer beg1, nb1, beg2, nb2;
   TA_RetCode retCode;
   int endIdx = (int)history->nbBars - 1;
   int period, startIdx, i, checks = 0;

   for( period = 2; period <= 40; period++ )
   {
      for( startIdx = 0; startIdx <= 34; startIdx++ )
      {
         retCode = TA_FOSC( startIdx, endIdx, history->close, period,
                            &beg1, &nb1, fused );
         if( retCode != TA_SUCCESS )
         {
            printf( "FOSC differential Fail: rc=%d period=%d startIdx=%d\n",
                    (int)retCode, period, startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         retCode = fosc_reference( startIdx, endIdx, history->close, period,
                                   &beg2, &nb2, composed );
         if( retCode != TA_SUCCESS )
         {
            printf( "FOSC differential Fail: reference rc=%d period=%d startIdx=%d\n",
                    (int)retCode, period, startIdx );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( beg1 != beg2 || nb1 != nb2 )
         {
            printf( "FOSC differential Fail: period=%d startIdx=%d shape (%d,%d) vs (%d,%d)\n",
                    period, startIdx, beg1, nb1, beg2, nb2 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         if( memcmp( fused, composed, (size_t)nb1 * sizeof(TA_Real) ) != 0 )
         {
            for( i = 0; i < nb1; i++ )
               if( fused[i] != composed[i] )
               {
                  printf( "FOSC differential Fail: period=%d startIdx=%d out[%d] "
                          "fused=%.17g composed-over-TA_TSF=%.17g\n",
                          period, startIdx, i, fused[i], composed[i] );
                  break;
               }
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         checks += nb1;
      }
   }

   if( checks < FOSC_DIFF_CHECKS )
   {
      printf( "\nFail: FOSC differential compared %d value(s), written with %d.\n",
              checks, FOSC_DIFF_CHECKS );
      return TA_FOSC_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* (4) A zero close makes the ratio 100*(0-TSF)/0. Both signed zeros take the
 * guard: -0.0 != 0.0 is false in C. Non-vacuous -- unguarded these are +-inf,
 * and FUZZ_WITH_ZEROS drives 0.0 and -0.0 through every function. */
static ErrorNumber test_fosc_zero_close( void )
{
   static TA_Real in[40];
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int sign, i, zeroBar;

   for( sign = 0; sign < 2; sign++ )
   {
      for( zeroBar = 7; zeroBar <= 9; zeroBar++ )
      {
         for( i = 0; i < 20; i++ )
            in[i] = 100.0 + 1.5 * i;
         in[zeroBar] = sign ? -0.0 : 0.0;

         retCode = TA_FOSC( 0, 19, in, 5, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 15 )
         {
            printf( "FOSC zero-close Fail: rc=%d shape (%d,%d)\n",
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( out[zeroBar - 5] != 0.0 )
         {
            printf( "FOSC zero-close Fail: close %s at bar %d gave out[%d]=%.17g,"
                    " expected exactly 0.0 (a non-finite value means the guard"
                    " did not fire)\n",
                    sign ? "-0.0" : "0.0", zeroBar, zeroBar - 5, out[zeroBar - 5] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( i = 0; i < nbElement; i++ )
            if( !TA_IS_FINITE( out[i] ) )
            {
               printf( "FOSC zero-close Fail: out[%d]=%.17g is not finite\n", i, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
      }
   }

   return TA_TEST_PASS;
}

/* (5a) Empty and short ranges report (0,0) and TA_SUCCESS, never a stale
 * outBegIdx. */
static ErrorNumber test_fosc_degenerate_ranges( const TA_History *history )
{
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;

   /* Fewer bars than the lookback. */
   begIdx = 12345; nbElement = 12345;
   retCode = TA_FOSC( 0, 3, history->close, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "FOSC short-range Fail: rc=%d (%d,%d), expected TA_SUCCESS (0,0)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* Exactly the lookback: one value, the first FOSC bar. */
   retCode = TA_FOSC( 0, 5, history->close, 5, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 5 || nbElement != 1 )
   {
      printf( "FOSC one-value Fail: rc=%d (%d,%d), expected TA_SUCCESS (5,1)\n",
              (int)retCode, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( TA_FOSC_Lookback( 5 ) != 5 || TA_FOSC_Lookback( 14 ) != 14 )
   {
      printf( "FOSC lookback Fail: got %d/%d, expected 5/14\n",
              TA_FOSC_Lookback( 5 ), TA_FOSC_Lookback( 14 ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (5b) Generic startIdx/endIdx range sweep. FOSC inherits TSF's stability
 * class: TA_STABLE_EPSILON, no unstable period. #103 is what moved the
 * LINEARREG family and TSF out of EXACT -- an O(1) sliding recurrence carries
 * accumulator drift across ranges, which #254's re-anchoring bounds but does
 * not erase (test_codegen.c's exact[] note). */
typedef struct { int period; const TA_Real *close; } FoscRangeParam;

static TA_RetCode foscRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   FoscRangeParam *p = (FoscRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_FOSC_Lookback( p->period );
   return TA_FOSC( startIdx, endIdx, p->close, p->period,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_fosc_range( const TA_History *history )
{
   FoscRangeParam param;
   param.period = 5;
   param.close  = history->close;

   return doRangeTestEx( foscRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
