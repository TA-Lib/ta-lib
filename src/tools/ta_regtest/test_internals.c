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
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070401 MF   First version.
 *  050104 MF   Add TA_RegressionTest calls.
 *  080605 MF   Add tests for pseudo-random generator.
 *  091705 MF   Add tests for TA_AddTimeToTimestamp (Fix#1293953).
 *  110906 MF   Remove pseudo-random to eliminate dependencies.
 */

/* Description:
 *         Regression testing of some internal utility like:
 *            - collections: List/Stack/Circular buffer.
 *            - Memory allocation mechanism.
 *            etc...
 */

/**** Headers ****/
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_memory.h"
#include "ta_defs.h"
#include "ta_common.h"
#include "ta_abstract.h"
#include "codegen_pipe.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber testCircularBuffer( void );
static ErrorNumber testBoundedAppend( void );
static ErrorNumber testUnstablePeriodBounds( void );
static ErrorNumber testCandleSettingsBounds( void );
static ErrorNumber testEnumValueContract( void );
static ErrorNumber testStreamShortHistory( void );
static ErrorNumber testBatchArgumentContract( void );

static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer );


/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
/* None */

/**** Local functions definitions.     ****/
ErrorNumber test_internals( void )
{
   ErrorNumber retValue;


   retValue = testCircularBuffer();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Circular buffer tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testBoundedAppend();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Bounded append tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testUnstablePeriodBounds();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Unstable period bound tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testCandleSettingsBounds();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Candle settings bound tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testEnumValueContract();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Public enum value contract (%d)\n", retValue );
      return retValue;
   }

   retValue = testStreamShortHistory();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Streaming short-history contract (%d)\n", retValue );
      return retValue;
   }

   retValue = testBatchArgumentContract();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Batch argument contract (%d)\n", retValue );
      return retValue;
   }

   return TA_TEST_PASS; /* Success. */
}

/* Rule S7: a stream opened on fewer than `lookback + 1` bars reports
 * TA_INSUFFICIENT_HISTORY -- the library's one RECOVERABLE condition.
 *
 * It is worth its own code, and its own probe, because it is the only failure a
 * correct caller can provoke and then fix by doing nothing but waiting: every
 * other rejection means the call itself is wrong. Until TA_INSUFFICIENT_HISTORY
 * was appended (#236) it shared TA_BAD_PARAM with all of them, and the two were
 * indistinguishable.
 *
 * One case per distinct shape the generator emits the arm in -- a plain
 * transcribed body (SMA), a composed capture guard (BBANDS), the dispatch tier's
 * own precheck (MA), the period bank (MAVP), a candlestick (CDLDOJI), the
 * identity fast path (EMA at period 1 with an unstable period to lift its
 * lookback off zero) -- plus the two OpenAndFill entry points, which are
 * separate emissions from their Open.
 *
 * The identity and period-bank-OpenAndFill cases are here because they were
 * MISSED: the first version of this probe covered six shapes and neither of
 * those was among them, so eleven C entry points kept answering the catch-all
 * with every gate green. The corpus-wide structural check
 * (scripts/check_stream_retcodes.py) is what covers the shapes nobody thought
 * to write a case for; this file covers the ones worth watching execute.
 *
 * Every case carries two controls, because the assertion alone would pass
 * against a stream that answered TA_INSUFFICIENT_HISTORY for everything:
 * ONE MORE BAR must succeed (so the rejection is about the length), and a
 * rejection that is NOT about the length must keep its OWN code -- TA_BAD_PARAM
 * for a spoiled parameter, and for CDLDOJI, which has none to spoil, S1's
 * TA_OUT_OF_RANGE_START_INDEX. Either way the point is that S7's code did not
 * simply swallow every other rejection this tier owns.
 */
static int shShort, shControl;
static int shUpper, shEmpty;

#define SH_CHECK( name, lookbackExpr, shortOpen, enoughOpen, otherOpen, otherCode, closeCall ) \
   do {                                                                        \
      int lb__ = (lookbackExpr);                                               \
      TA_RetCode rc__;                                                         \
      if( lb__ < 1 )                                                           \
      {                                                                        \
         printf( "\nFailed: %s has lookback %d -- no short history exists\n",   \
                 name, lb__ );                                                 \
         return TA_STREAM_SHORT_HISTORY_CONTROL;                               \
      }                                                                        \
      rc__ = (shortOpen);                                                      \
      if( rc__ == TA_SUCCESS )                                                 \
      {                                                                        \
         printf( "\nFailed: %s opened on %d bars, one short of its lookback\n", \
                 name, lb__ );                                                 \
         return TA_STREAM_SHORT_HISTORY_ACCEPTED;                              \
      }                                                                        \
      if( rc__ != TA_INSUFFICIENT_HISTORY )                                    \
      {                                                                        \
         printf( "\nFailed: %s short history returned %d, expected "            \
                 "TA_INSUFFICIENT_HISTORY (%d)\n",                             \
                 name, (int)rc__, (int)TA_INSUFFICIENT_HISTORY );              \
         return TA_STREAM_SHORT_HISTORY_WRONG_CODE;                            \
      }                                                                        \
      shShort++;                                                               \
      rc__ = (enoughOpen);                                                     \
      if( rc__ != TA_SUCCESS )                                                 \
      {                                                                        \
         printf( "\nFailed: %s rejected %d bars (lookback + 1) with %d\n",      \
                 name, lb__ + 1, (int)rc__ );                                  \
         return TA_STREAM_SHORT_HISTORY_CONTROL;                               \
      }                                                                        \
      (closeCall);                                                             \
      shControl++;                                                             \
      rc__ = (otherOpen);                                                      \
      if( rc__ != (otherCode) )                                                \
      {                                                                        \
         printf( "\nFailed: %s bad argument returned %d, expected "             \
                 "%d -- S7's code must not swallow the others\n",              \
                 name, (int)rc__, (int)(otherCode) );                          \
         return TA_STREAM_SHORT_HISTORY_CONTROL;                               \
      }                                                                        \
      shControl++;                                                             \
   } while(0)

static ErrorNumber testStreamShortHistory( void )
{
   /* A monotone series: every value distinct and finite, so nothing here can be
    * rejected for any reason but its length. */
   static double bars[512];
   static double periods[512];
   ErrorNumber retValue;
   int i;

   /* The candle-settings test above ends with freeLib(), so the globals a
    * candlestick lookback reads are zeroed by the time this runs. Without this
    * the CDLDOJI leg would see lookback 0 and have no short history to give. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   shShort = shControl = 0;
   shUpper = shEmpty = 0;

   for( i = 0; i < 512; i++ )
   {
      bars[i] = 100.0 + (double)i * 0.25;
      periods[i] = 5.0;
   }

   /* Plain transcribed body. */
   {
      TA_SMA_Stream *st = NULL;
      double v = 0.0;
      SH_CHECK( "TA_SMA_Open", TA_SMA_Lookback( 30 ),
                TA_SMA_Open( &st, bars, TA_SMA_Lookback( 30 ), 30, &v ),
                TA_SMA_Open( &st, bars, TA_SMA_Lookback( 30 ) + 1, 30, &v ),
                TA_SMA_Open( &st, bars, 200, 0, &v ), TA_BAD_PARAM,
                TA_SMA_Close( st ) );
   }

   /* Composed: the capture guard after the sub-streams have run. */
   {
      TA_BBANDS_Stream *st = NULL;
      double a = 0.0, b = 0.0, c = 0.0;
      int lb = TA_BBANDS_Lookback( 20, 2.0, 2.0, TA_MAType_SMA );
      SH_CHECK( "TA_BBANDS_Open", lb,
                TA_BBANDS_Open( &st, bars, lb, 20, 2.0, 2.0, TA_MAType_SMA, &a, &b, &c ),
                TA_BBANDS_Open( &st, bars, lb + 1, 20, 2.0, 2.0, TA_MAType_SMA, &a, &b, &c ),
                TA_BBANDS_Open( &st, bars, 200, 0, 2.0, 2.0, TA_MAType_SMA, &a, &b, &c ), TA_BAD_PARAM,
                TA_BBANDS_Close( st ) );
   }

   /* Dispatch tier: its own precheck, before delegating to the selected arm. */
   {
      TA_MA_Stream *st = NULL;
      double v = 0.0;
      int lb = TA_MA_Lookback( 30, TA_MAType_EMA );
      SH_CHECK( "TA_MA_Open", lb,
                TA_MA_Open( &st, bars, lb, 30, TA_MAType_EMA, &v ),
                TA_MA_Open( &st, bars, lb + 1, 30, TA_MAType_EMA, &v ),
                TA_MA_Open( &st, bars, 200, 0, TA_MAType_EMA, &v ), TA_BAD_PARAM,
                TA_MA_Close( st ) );
   }

   /* Period bank. */
   {
      TA_MAVP_Stream *st = NULL;
      double v = 0.0;
      int lb = TA_MAVP_Lookback( 2, 30, TA_MAType_SMA );
      SH_CHECK( "TA_MAVP_Open", lb,
                TA_MAVP_Open( &st, bars, periods, lb, 2, 30, TA_MAType_SMA, &v ),
                TA_MAVP_Open( &st, bars, periods, lb + 1, 2, 30, TA_MAType_SMA, &v ),
                TA_MAVP_Open( &st, bars, periods, 200, 2, 0, TA_MAType_SMA, &v ), TA_BAD_PARAM,
                TA_MAVP_Close( st ) );
   }

   /* Candlestick: an integer output, and the settings-driven averaging window. */
   {
      TA_CDLDOJI_Stream *st = NULL;
      int v = 0;
      int lb = TA_CDLDOJI_Lookback();
      SH_CHECK( "TA_CDLDOJI_Open", lb,
                TA_CDLDOJI_Open( &st, bars, bars, bars, bars, lb, &v ),
                TA_CDLDOJI_Open( &st, bars, bars, bars, bars, lb + 1, &v ),
                /* No optional parameter to spoil: an empty history is the
                 * other rejection this tier owns (rule S1). */
                TA_CDLDOJI_Open( &st, bars, bars, bars, bars, 0, &v ),
                TA_OUT_OF_RANGE_START_INDEX,
                TA_CDLDOJI_Close( st ) );
   }

   /* The IDENTITY fast path: a period that makes the function a copy of its
    * input. Its guard is emitted separately from the transcribed body, and it
    * was one of the two C sites that kept answering the catch-all when
    * TA_INSUFFICIENT_HISTORY was introduced -- neither reachable by any of the
    * probes above, because they all use a period well above 1.
    *
    * At period 1 the lookback is 0 and no short history exists, so the arm is
    * unreachable until an unstable period lifts it. That is exactly the
    * configuration the divergence lives in.
    */
   {
      TA_EMA_Stream *st = NULL;
      double v = 0.0;
      int lb;
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 5 );
      lb = TA_EMA_Lookback( 1 );
      SH_CHECK( "TA_EMA_Open (identity, unstable 5)", lb,
                TA_EMA_Open( &st, bars, lb, 1, &v ),
                TA_EMA_Open( &st, bars, lb + 1, 1, &v ),
                TA_EMA_Open( &st, bars, 200, 0, &v ), TA_BAD_PARAM,
                TA_EMA_Close( st ) );
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   }

   /* The period BANK's own OpenAndFill guard, which is emitted separately from
    * the Open above -- the other of the two missed C sites. MAVP_Open and
    * MAVP_OpenAndFill answered DIFFERENT codes for the same short history.
    */
   {
      TA_MAVP_Stream *st = NULL;
      static double out2[512];
      int beg = 0, nb = 0;
      int lb = TA_MAVP_Lookback( 2, 30, TA_MAType_SMA );
      SH_CHECK( "TA_MAVP_OpenAndFill", lb,
                TA_MAVP_OpenAndFill( &st, bars, periods, lb, 2, 30, TA_MAType_SMA, &beg, &nb, out2 ),
                TA_MAVP_OpenAndFill( &st, bars, periods, lb + 1, 2, 30, TA_MAType_SMA, &beg, &nb, out2 ),
                TA_MAVP_OpenAndFill( &st, bars, periods, 200, 2, 0, TA_MAType_SMA, &beg, &nb, out2 ), TA_BAD_PARAM,
                TA_MAVP_Close( st ) );
   }

   /* OpenAndFill is a separate entry point with its own emission. */
   {
      TA_SMA_Stream *st = NULL;
      static double out[512];
      int beg = 0, nb = 0;
      int lb = TA_SMA_Lookback( 30 );
      SH_CHECK( "TA_SMA_OpenAndFill", lb,
                TA_SMA_OpenAndFill( &st, bars, lb, 30, &beg, &nb, out ),
                TA_SMA_OpenAndFill( &st, bars, lb + 1, 30, &beg, &nb, out ),
                TA_SMA_OpenAndFill( &st, bars, 200, 0, &beg, &nb, out ), TA_BAD_PARAM,
                TA_SMA_Close( st ) );
   }

   /* Rule S1, the LOWER half of the history bound: the implied `startIdx` of 0
    * has to name a bar, so an empty history is TA_OUT_OF_RANGE_START_INDEX --
    * B1's code, because an opener is a batch call over `[0, historyLen - 1]`.
    *
    * What is worth the probe is the ORDER, not the code alone. The pair is
    * evaluated ahead of every presence check, so a call that is BOTH an absent
    * output and an empty history reports this rather than TA_BAD_PARAM; until
    * #268 it reported the absent output, and a caller who fixed that argument
    * got the same rejection back for a reason nothing had mentioned. The one
    * check that still precedes the pair is the handle, and for a reason no
    * ordering choice can remove: `*stream = NULL` is how "no handle on any
    * failure" is published, and there is nowhere to publish it without one.
    * That case is the last control below. */
   {
      TA_SMA_Stream     *sst = NULL;
      TA_MA_Stream      *mst = NULL;
      TA_MAVP_Stream    *pst = NULL;
      TA_CDLDOJI_Stream *cst = NULL;
      static double out[512];
      double v = 0.0;
      int iv = 0;
      int beg = 0, nb = 0;
      TA_RetCode rc;
      struct { const char *name; TA_RetCode rc; } cases[8];

      cases[0].name = "TA_SMA_Open(historyLen=0)";
      cases[0].rc   = TA_SMA_Open( &sst, bars, 0, 30, &v );
      cases[1].name = "TA_SMA_OpenAndFill(historyLen=0)";
      cases[1].rc   = TA_SMA_OpenAndFill( &sst, bars, 0, 30, &beg, &nb, out );
      cases[2].name = "TA_SMA_Open(historyLen=-1)";
      cases[2].rc   = TA_SMA_Open( &sst, bars, -1, 30, &v );
      /* The order claim: each of these is also an S4 rejection. */
      cases[3].name = "TA_SMA_Open(historyLen=0, outReal=NULL)";
      cases[3].rc   = TA_SMA_Open( &sst, bars, 0, 30, NULL );
      cases[4].name = "TA_SMA_OpenAndFill(historyLen=0, outBegIdx=NULL)";
      cases[4].rc   = TA_SMA_OpenAndFill( &sst, bars, 0, 30, NULL, &nb, out );
      cases[5].name = "TA_SMA_OpenAndFill(historyLen=0, inReal=NULL)";
      cases[5].rc   = TA_SMA_OpenAndFill( &sst, NULL, 0, 30, &beg, &nb, out );
      /* The dispatch tier and the period bank hand-roll their own prologue. */
      cases[6].name = "TA_MA_Open(historyLen=0)";
      cases[6].rc   = TA_MA_Open( &mst, bars, 0, 30, TA_MAType_EMA, &v );
      cases[7].name = "TA_MAVP_Open(historyLen=0)";
      cases[7].rc   = TA_MAVP_Open( &pst, bars, periods, 0, 2, 30, TA_MAType_SMA, &v );

      for( i = 0; i < 8; i++ )
      {
         rc = cases[i].rc;
         if( rc != TA_OUT_OF_RANGE_START_INDEX )
         {
            printf( "\nFailed: %s returned %d, expected "
                    "TA_OUT_OF_RANGE_START_INDEX (%d)\n",
                    cases[i].name, (int)rc, (int)TA_OUT_OF_RANGE_START_INDEX );
            return TA_STREAM_EMPTY_HISTORY_WRONG_CODE;
         }
         shEmpty++;
      }

      /* A candlestick reaches it through four price legs rather than one. */
      if( TA_CDLDOJI_Open( &cst, bars, bars, bars, bars, 0, &iv )
          != TA_OUT_OF_RANGE_START_INDEX )
      {
         printf( "\nFailed: TA_CDLDOJI_Open(historyLen=0) did not report "
                 "TA_OUT_OF_RANGE_START_INDEX\n" );
         return TA_STREAM_EMPTY_HISTORY_WRONG_CODE;
      }
      shEmpty++;

      /* Controls. A history of exactly one bar is inside the domain -- it is
       * S7's business, not S1's -- and the handle still answers first. */
      if( TA_SMA_Open( &sst, bars, 1, 30, &v ) != TA_INSUFFICIENT_HISTORY )
      {
         printf( "\nFailed: a one-bar history did not reach the warm-up check\n" );
         return TA_STREAM_EMPTY_HISTORY_WRONG_CODE;
      }
      if( TA_SMA_Open( NULL, bars, 0, 30, &v ) != TA_BAD_PARAM )
      {
         printf( "\nFailed: an absent handle must answer TA_BAD_PARAM even on "
                 "an empty history\n" );
         return TA_STREAM_EMPTY_HISTORY_WRONG_CODE;
      }
   }

   /* Rule S2, the other half of the history bound: `historyLen - 1` is the
    * implied `endIdx`, so a history longer than MAX_INDEX + 1 leaves the index
    * domain. Only C can be probed cheaply -- it takes `historyLen` as a bare
    * `int`, so the rejection answers before a bar is read; the other three
    * derive it from the array and would need a 100 000 001-element one. The
    * legal upper edge is out of reach here for the same reason. */
   {
      TA_SMA_Stream *st = NULL;
      static double out[512];
      double v = 0.0;
      int beg = 0, nb = 0;
      TA_RetCode rc;
      struct { const char *name; TA_RetCode rc; } cases[3];

      cases[0].name = "TA_SMA_Open(historyLen=MAX_INDEX+2)";
      cases[0].rc   = TA_SMA_Open( &st, bars, TA_MAX_INDEX + 2, 30, &v );
      cases[1].name = "TA_SMA_OpenAndFill(historyLen=MAX_INDEX+2)";
      cases[1].rc   = TA_SMA_OpenAndFill( &st, bars, TA_MAX_INDEX + 2, 30, &beg, &nb, out );
      cases[2].name = "TA_SMA_Open(historyLen=INT_MAX)";
      cases[2].rc   = TA_SMA_Open( &st, bars, 2147483647, 30, &v );

      for( i = 0; i < 3; i++ )
      {
         rc = cases[i].rc;
         if( rc != TA_OUT_OF_RANGE_END_INDEX )
         {
            printf( "\nFailed: %s returned %d, expected "
                    "TA_OUT_OF_RANGE_END_INDEX (%d)\n",
                    cases[i].name, (int)rc, (int)TA_OUT_OF_RANGE_END_INDEX );
            return TA_STREAM_SHORT_HISTORY_WRONG_CODE;
         }
         shUpper++;
      }
   }

   /* Literal floors, not derived from the cases above: a count computed from the
    * loop would move with a deleted case and still "pass". */
   if( shUpper < 3 )
   {
      printf( "\nFailed: the history upper-bound gate ran fewer checks than it "
              "was written with\n" );
      return TA_STREAM_SHORT_HISTORY_VACUOUS;
   }
   if( shEmpty < 9 )
   {
      printf( "\nFailed: the empty-history gate ran fewer checks than it was "
              "written with\n" );
      return TA_STREAM_EMPTY_HISTORY_VACUOUS;
   }
   if( shShort < 8 || shControl < 16 )
   {
      printf( "\nFailed: the short-history gate ran fewer checks than it was "
              "written with\n" );
      return TA_STREAM_SHORT_HISTORY_VACUOUS;
   }

   return freeLib();
}

/* Rule B4: a required argument that was not supplied is TA_BAD_PARAM.
 *
 * The two range out-parameters are required arguments, and they were the one
 * pair the batch prologue never checked -- a NULL `outBegIdx` or `outNBElement`
 * was written to, so the diagnosis was a segfault. C's own OpenAndFill prologue
 * has always checked exactly those two, which is what made this an omission
 * rather than a position. Nothing could see it: the JSON-RPC servers, ta_bench
 * and every wrapper hand the pair real pointers, so no value gate reaches the
 * call at all.
 *
 * The order between B4 and B3 (an out-of-domain parameter) is NOT observable
 * from here -- both answer TA_BAD_PARAM. It is pinned structurally, over the
 * whole corpus, by `c_batch_prologue_orders_parameters_before_presence` in
 * ta_codegen's own suite.
 *
 * One case per distinct emission shape, matching the S7 probe above: a plain
 * transcribed body (SMA, plus its float twin, which is a separate emission), a
 * composed multi-output (BBANDS), the dispatch tier (MA), the period bank
 * (MAVP), a candlestick with four price legs and an integer output (CDLDOJI),
 * a candlestick leg the body never indexes (CDL3OUTSIDE, CDLHIKKAKE -- #260),
 * and a nullable output (MAMA), which is the control for what "required" means.
 *
 * Rule S4 rides along at the end: it is this rule plus the handle, over the
 * same argument shapes, so the streaming openers are driven from here rather
 * than from a gate of their own.
 */
static int bacReject, bacAccept;
static int s4Reject, s4Accept;
static int u6aFill;

#define BAC_REJECT( name, call )                                               \
   do {                                                                        \
      TA_RetCode rc__ = (call);                                                \
      if( rc__ != TA_BAD_PARAM )                                               \
      {                                                                        \
         printf( "\nFailed: %s returned %d, expected TA_BAD_PARAM (%d)\n",     \
                 name, (int)rc__, (int)TA_BAD_PARAM );                         \
         return TA_BATCH_ARG_WRONG_CODE;                                       \
      }                                                                        \
      bacReject++;                                                             \
   } while(0)

#define BAC_ACCEPT( name, call )                                               \
   do {                                                                        \
      TA_RetCode rc__ = (call);                                                \
      if( rc__ != TA_SUCCESS )                                                 \
      {                                                                        \
         printf( "\nFailed: %s returned %d, expected TA_SUCCESS\n",            \
                 name, (int)rc__ );                                            \
         return TA_BATCH_ARG_CONTROL;                                          \
      }                                                                        \
      bacAccept++;                                                             \
   } while(0)

#define S4_REJECT( name, call )                                                \
   do {                                                                        \
      TA_RetCode rc__ = (call);                                                \
      if( rc__ != TA_BAD_PARAM )                                               \
      {                                                                        \
         printf( "\nFailed: %s returned %d, expected TA_BAD_PARAM (%d)\n",     \
                 name, (int)rc__, (int)TA_BAD_PARAM );                         \
         return TA_BATCH_ARG_WRONG_CODE;                                       \
      }                                                                        \
      s4Reject++;                                                              \
   } while(0)

#define S4_ACCEPT( name, call )                                                \
   do {                                                                        \
      TA_RetCode rc__ = (call);                                                \
      if( rc__ != TA_SUCCESS )                                                 \
      {                                                                        \
         printf( "\nFailed: %s returned %d, expected TA_SUCCESS\n",            \
                 name, (int)rc__ );                                            \
         return TA_BATCH_ARG_CONTROL;                                          \
      }                                                                        \
      s4Accept++;                                                              \
   } while(0)

static ErrorNumber testBatchArgumentContract( void )
{
   static double bars[512];
   static float  sbars[512];
   static double periods[512];
   static double outA[512], outB[512], outC[512];
   static int    outI[512];
   ErrorNumber retValue;
   int beg = 0, nb = 0;
   int i;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   bacReject = bacAccept = 0;
   s4Reject = s4Accept = 0;
   u6aFill = 0;

   for( i = 0; i < 512; i++ )
   {
      bars[i] = 100.0 + (double)i * 0.25;
      sbars[i] = (float)bars[i];
      periods[i] = 5.0;
   }

   /* Plain transcribed body, and its float twin. */
   BAC_ACCEPT( "TA_SMA", TA_SMA( 0, 251, bars, 30, &beg, &nb, outA ) );
   BAC_REJECT( "TA_SMA(outBegIdx=NULL)",  TA_SMA( 0, 251, bars, 30, NULL, &nb, outA ) );
   BAC_REJECT( "TA_SMA(outNBElement=NULL)", TA_SMA( 0, 251, bars, 30, &beg, NULL, outA ) );
   BAC_REJECT( "TA_SMA(inReal=NULL)",     TA_SMA( 0, 251, NULL, 30, &beg, &nb, outA ) );
   BAC_REJECT( "TA_SMA(outReal=NULL)",    TA_SMA( 0, 251, bars, 30, &beg, &nb, NULL ) );

   BAC_ACCEPT( "TA_S_SMA", TA_S_SMA( 0, 251, sbars, 30, &beg, &nb, outA ) );
   BAC_REJECT( "TA_S_SMA(outBegIdx=NULL)",  TA_S_SMA( 0, 251, sbars, 30, NULL, &nb, outA ) );
   BAC_REJECT( "TA_S_SMA(outNBElement=NULL)", TA_S_SMA( 0, 251, sbars, 30, &beg, NULL, outA ) );

   /* Composed, three outputs. */
   BAC_ACCEPT( "TA_BBANDS",
               TA_BBANDS( 0, 251, bars, 20, 2.0, 2.0, TA_MAType_SMA, &beg, &nb, outA, outB, outC ) );
   BAC_REJECT( "TA_BBANDS(outBegIdx=NULL)",
               TA_BBANDS( 0, 251, bars, 20, 2.0, 2.0, TA_MAType_SMA, NULL, &nb, outA, outB, outC ) );
   BAC_REJECT( "TA_BBANDS(outNBElement=NULL)",
               TA_BBANDS( 0, 251, bars, 20, 2.0, 2.0, TA_MAType_SMA, &beg, NULL, outA, outB, outC ) );

   /* Dispatch tier. */
   BAC_ACCEPT( "TA_MA", TA_MA( 0, 251, bars, 30, TA_MAType_EMA, &beg, &nb, outA ) );
   BAC_REJECT( "TA_MA(outBegIdx=NULL)",
               TA_MA( 0, 251, bars, 30, TA_MAType_EMA, NULL, &nb, outA ) );
   BAC_REJECT( "TA_MA(outNBElement=NULL)",
               TA_MA( 0, 251, bars, 30, TA_MAType_EMA, &beg, NULL, outA ) );

   /* Period bank, two input series. */
   BAC_ACCEPT( "TA_MAVP",
               TA_MAVP( 0, 251, bars, periods, 2, 30, TA_MAType_SMA, &beg, &nb, outA ) );
   BAC_REJECT( "TA_MAVP(outBegIdx=NULL)",
               TA_MAVP( 0, 251, bars, periods, 2, 30, TA_MAType_SMA, NULL, &nb, outA ) );
   BAC_REJECT( "TA_MAVP(inPeriods=NULL)",
               TA_MAVP( 0, 251, bars, NULL, 2, 30, TA_MAType_SMA, &beg, &nb, outA ) );

   /* Candlestick: four price legs, an integer output. */
   BAC_ACCEPT( "TA_CDLDOJI",
               TA_CDLDOJI( 0, 251, bars, bars, bars, bars, &beg, &nb, outI ) );
   BAC_REJECT( "TA_CDLDOJI(outBegIdx=NULL)",
               TA_CDLDOJI( 0, 251, bars, bars, bars, bars, NULL, &nb, outI ) );
   BAC_REJECT( "TA_CDLDOJI(outInteger=NULL)",
               TA_CDLDOJI( 0, 251, bars, bars, bars, bars, &beg, &nb, NULL ) );

   /* A price leg the algorithm never INDEXES is a required argument all the
    * same (#260). CDL3OUTSIDE reads open and close only, CDLHIKKAKE everything
    * but open; C has always rejected a NULL there, and Rust, Java and C# used
    * to accept it. Without these the worked example in
    * docs/error-handling-spec.md 2.2 is asserted from the source and executed
    * nowhere. */
   BAC_ACCEPT( "TA_CDL3OUTSIDE",
               TA_CDL3OUTSIDE( 0, 251, bars, bars, bars, bars, &beg, &nb, outI ) );
   BAC_REJECT( "TA_CDL3OUTSIDE(inHigh=NULL)",
               TA_CDL3OUTSIDE( 0, 251, bars, NULL, bars, bars, &beg, &nb, outI ) );
   BAC_REJECT( "TA_CDL3OUTSIDE(inLow=NULL)",
               TA_CDL3OUTSIDE( 0, 251, bars, bars, NULL, bars, &beg, &nb, outI ) );
   BAC_ACCEPT( "TA_CDLHIKKAKE",
               TA_CDLHIKKAKE( 0, 251, bars, bars, bars, bars, &beg, &nb, outI ) );
   BAC_REJECT( "TA_CDLHIKKAKE(inOpen=NULL)",
               TA_CDLHIKKAKE( 0, 251, NULL, bars, bars, bars, &beg, &nb, outI ) );

   /* Rule B6a: a NULLABLE output is not a required argument. Dropping it is
    * legal, and that is what keeps the rejections above about absence rather
    * than about NULL. The required half of the same call still answers
    * TA_BAD_PARAM. */
   BAC_ACCEPT( "TA_MAMA(outFAMA=NULL)",
               TA_MAMA( 0, 251, bars, 0.5, 0.05, &beg, &nb, outA, NULL ) );
   BAC_REJECT( "TA_MAMA(outMAMA=NULL)",
               TA_MAMA( 0, 251, bars, 0.5, 0.05, &beg, &nb, NULL, outA ) );
   BAC_REJECT( "TA_MAMA(outNBElement=NULL)",
               TA_MAMA( 0, 251, bars, 0.5, 0.05, &beg, NULL, outA, outB ) );

   /* "Compute but do not write it" is the whole claim, and acceptance alone
    * does not test it: a body that stopped computing FAMA — or that took a
    * different path when it is absent — would still be accepted here. So the
    * declined call has to reproduce the supplied one, value for value, on the
    * output it DID ask for, and leave the buffer it did not alone.
    *
    * The canary above the produced count is the other half: rule N2 says only
    * the reported range is written, and a guard that leaked a store past its
    * own condition would land there. */
   {
      int begRef = 0, nbRef = 0, begNull = -1, nbNull = -1;
      int k;
      TA_RetCode rcRef, rcNull;

      for( k = 0; k < 512; k++ )
      {
         outA[k] = outB[k] = 0.0;
         outC[k] = -1.2345678901234e300;
      }
      rcRef  = TA_MAMA( 0, 251, bars, 0.5, 0.05, &begRef, &nbRef, outA, outB );
      rcNull = TA_MAMA( 0, 251, bars, 0.5, 0.05, &begNull, &nbNull, outC, NULL );
      if( rcRef != TA_SUCCESS || rcNull != TA_SUCCESS || nbRef == 0 )
      {
         printf( "\nFailed: TA_MAMA nullable comparison did not run (%d, %d, nb %d)\n",
                 (int)rcRef, (int)rcNull, nbRef );
         return TA_BATCH_ARG_VACUOUS;
      }
      if( begRef != begNull || nbRef != nbNull )
      {
         printf( "\nFailed: declining outFAMA changed the reported range "
                 "(%d,%d vs %d,%d)\n", begRef, nbRef, begNull, nbNull );
         return TA_BATCH_ARG_NULLABLE_DIVERGED;
      }
      for( k = 0; k < nbRef; k++ )
      {
         if( memcmp( &outA[k], &outC[k], sizeof(double) ) != 0 )
         {
            printf( "\nFailed: declining outFAMA changed outMAMA[%d] "
                    "(%.17g vs %.17g)\n", k, outA[k], outC[k] );
            return TA_BATCH_ARG_NULLABLE_DIVERGED;
         }
      }
      for( k = nbRef; k < 512; k++ )
      {
         if( outC[k] != -1.2345678901234e300 )
         {
            printf( "\nFailed: the declining call wrote outMAMA[%d], past its "
                    "count of %d\n", k, nbRef );
            return TA_BATCH_ARG_NULLABLE_DIVERGED;
         }
      }
      bacAccept += 2;
   }

   /* Rule S4 is B4 plus one argument: the handle. The streaming openers take
    * the same declared inputs and outputs as the batch call, so rather than a
    * suite of its own, B4's shapes are re-driven through `Open` and
    * `OpenAndFill` here. What is new is the handle, `OpenAndFill`'s own range
    * out-parameters, and the fact that a rejected open must leave no handle.
    *
    * Counted separately: sharing B4's counters would let a deleted S4 case
    * hide behind a batch one. */
   {
      TA_SMA_Stream         *sst = NULL;
      TA_MAMA_Stream        *mst = NULL;
      TA_CDL3OUTSIDE_Stream *cst = NULL;
      double v = 0.0, v2 = 0.0;

      /* The argument the batch tier does not have. */
      S4_REJECT( "TA_SMA_Open(stream=NULL)",
                 TA_SMA_Open( NULL, bars, 252, 30, &v ) );
      S4_REJECT( "TA_SMA_OpenAndFill(stream=NULL)",
                 TA_SMA_OpenAndFill( NULL, bars, 252, 30, &beg, &nb, outA ) );

      /* B4's shapes, through the openers. */
      S4_REJECT( "TA_SMA_Open(inReal=NULL)",
                 TA_SMA_Open( &sst, NULL, 252, 30, &v ) );
      S4_REJECT( "TA_SMA_Open(outReal=NULL)",
                 TA_SMA_Open( &sst, bars, 252, 30, NULL ) );
      S4_REJECT( "TA_SMA_OpenAndFill(inReal=NULL)",
                 TA_SMA_OpenAndFill( &sst, NULL, 252, 30, &beg, &nb, outA ) );
      S4_REJECT( "TA_SMA_OpenAndFill(outReal=NULL)",
                 TA_SMA_OpenAndFill( &sst, bars, 252, 30, &beg, &nb, NULL ) );
      S4_REJECT( "TA_SMA_OpenAndFill(outBegIdx=NULL)",
                 TA_SMA_OpenAndFill( &sst, bars, 252, 30, NULL, &nb, outA ) );
      S4_REJECT( "TA_SMA_OpenAndFill(outNBElement=NULL)",
                 TA_SMA_OpenAndFill( &sst, bars, 252, 30, &beg, NULL, outA ) );
      S4_REJECT( "TA_CDL3OUTSIDE_Open(inHigh=NULL)",
                 TA_CDL3OUTSIDE_Open( &cst, bars, NULL, bars, bars, 252, outI ) );
      S4_REJECT( "TA_MAMA_Open(outMAMA=NULL)",
                 TA_MAMA_Open( &mst, bars, 252, 0.5, 0.05, NULL, &v2 ) );
      S4_REJECT( "TA_MAMA_OpenAndFill(outMAMA=NULL)",
                 TA_MAMA_OpenAndFill( &mst, bars, 252, 0.5, 0.05, &beg, &nb, NULL, outB ) );

      /* A rejected open leaves no handle behind: the prologue clears *stream
       * before any other check, so a caller cannot be handed a pointer the
       * call never made -- nor keep one a previous call did. */
      sst = (TA_SMA_Stream *)(void *)bars;
      if( TA_SMA_Open( &sst, NULL, 252, 30, &v ) != TA_BAD_PARAM || sst != NULL )
      {
         printf( "\nFailed: a rejected TA_SMA_Open did not clear *stream\n" );
         return TA_BATCH_ARG_WRONG_CODE;
      }
      s4Reject++;

      /* Controls. The nullable output is B6a's analogue: what the rejections
       * above reject is absence, not NULL. */
      S4_ACCEPT( "TA_SMA_Open", TA_SMA_Open( &sst, bars, 252, 30, &v ) );
      if( sst ) { TA_SMA_Close( sst ); sst = NULL; }
      S4_ACCEPT( "TA_SMA_OpenAndFill",
                 TA_SMA_OpenAndFill( &sst, bars, 252, 30, &beg, &nb, outA ) );
      if( sst ) { TA_SMA_Close( sst ); sst = NULL; }
      S4_ACCEPT( "TA_MAMA_Open(outFAMA=NULL)",
                 TA_MAMA_Open( &mst, bars, 252, 0.5, 0.05, &v, NULL ) );
      if( mst ) { TA_MAMA_Close( mst ); mst = NULL; }
      S4_ACCEPT( "TA_MAMA_OpenAndFill(outFAMA=NULL)",
                 TA_MAMA_OpenAndFill( &mst, bars, 252, 0.5, 0.05, &beg, &nb, outA, NULL ) );
      if( mst ) { TA_MAMA_Close( mst ); mst = NULL; }
      S4_ACCEPT( "TA_CDL3OUTSIDE_Open",
                 TA_CDL3OUTSIDE_Open( &cst, bars, bars, bars, bars, 252, outI ) );
      if( cst ) { TA_CDL3OUTSIDE_Close( cst ); cst = NULL; }
   }

   /* Rule U6a: a nullable output may be declined at UpdateAndFill too, and the
    * choice is the CALL's -- neither matching the opener's nor recorded on the
    * handle. All four open/fill combinations compute the same numbers.
    *
    * The comparison a fill that stopped computing FAMA cannot satisfy is the
    * PEEK after it, which reads the handle's state rather than anything that was
    * written out. C is the reference shape here, so this block is a pin rather
    * than a fix -- the three ported backends are what #270 changed. */
   {
      static double fillBars[8];
      static double refM[8], refF[8], gotM[8], gotF[8];
      TA_MAMA_Stream *st = NULL;
      double pm = 0.0, pf = 0.0, rpm = 0.0, rpf = 0.0;
      int declinedAtOpen, k;
      int beg2 = 0, nb2 = 0, begRef = 0, nbRef = 0, nbBefore = 0;

      for( k = 0; k < 8; k++ )
         fillBars[k] = bars[251] + 1.0 + (double)k * 0.25;

      /* Canary-filled, not zero-filled: comparing two arrays the fill never
       * wrote would otherwise pass on their shared initial value, which is
       * exactly the break the supplied/supplied leg is meant to catch. */
      #define U6A_CANARY (-1.2345678901234e300)
      for( k = 0; k < 8; k++ )
      {
         refM[k] = refF[k] = gotM[k] = gotF[k] = U6A_CANARY;
      }

      /* The oracle: supplied at open, supplied at the fill. */
      if( TA_MAMA_OpenAndFill( &st, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS ||
          TA_MAMA_UpdateAndFill( st, fillBars, 8, refM, refF ) != TA_SUCCESS ||
          TA_StreamOutRange( st, &begRef, &nbRef ) != TA_SUCCESS ||
          TA_MAMA_Peek( st, bars[251], &rpm, &rpf ) != TA_SUCCESS )
      {
         printf( "\nFailed: the U6a oracle did not run\n" );
         return TA_BATCH_ARG_CONTROL;
      }
      TA_MAMA_Close( st );
      st = NULL;
      for( k = 0; k < 8; k++ )
      {
         if( refM[k] == U6A_CANARY || refF[k] == U6A_CANARY )
         {
            printf( "\nFailed: the U6a oracle fill did not write [%d]\n", k );
            return TA_BATCH_ARG_CONTROL;
         }
      }
      u6aFill++;

      for( declinedAtOpen = 0; declinedAtOpen < 2; declinedAtOpen++ )
      {
         /* Declined at the fill, whatever the opener was given. */
         for( k = 0; k < 8; k++ ) gotM[k] = gotF[k] = U6A_CANARY;
         if( TA_MAMA_OpenAndFill( &st, bars, 252, 0.5, 0.05, &beg, &nb, outA,
                                  declinedAtOpen ? NULL : outB ) != TA_SUCCESS ||
             TA_MAMA_UpdateAndFill( st, fillBars, 8, gotM, NULL ) != TA_SUCCESS ||
             TA_StreamOutRange( st, &beg2, &nb2 ) != TA_SUCCESS ||
             TA_MAMA_Peek( st, bars[251], &pm, &pf ) != TA_SUCCESS )
         {
            printf( "\nFailed: declining outFAMA at UpdateAndFill was rejected "
                    "(declinedAtOpen=%d)\n", declinedAtOpen );
            if( st ) TA_MAMA_Close( st );
            return TA_BATCH_ARG_CONTROL;
         }
         TA_MAMA_Close( st );
         st = NULL;
         u6aFill++;
         for( k = 0; k < 8; k++ )
         {
            if( gotM[k] == U6A_CANARY )
            {
               printf( "\nFailed: the declining fill did not write outMAMA[%d]\n", k );
               return TA_BATCH_ARG_WRONG_CODE;
            }
            if( memcmp( &gotM[k], &refM[k], sizeof(double) ) != 0 )
            {
               printf( "\nFailed: declining outFAMA changed outMAMA[%d] "
                       "(declinedAtOpen=%d)\n", k, declinedAtOpen );
               return TA_BATCH_ARG_WRONG_CODE;
            }
         }
         u6aFill++;
         if( beg2 != begRef || nb2 != nbRef )
         {
            printf( "\nFailed: declining outFAMA moved the reported range\n" );
            return TA_BATCH_ARG_WRONG_CODE;
         }
         u6aFill++;
         if( memcmp( &pm, &rpm, sizeof(double) ) != 0 ||
             memcmp( &pf, &rpf, sizeof(double) ) != 0 )
         {
            printf( "\nFailed: a declined outFAMA is not still computed "
                    "(declinedAtOpen=%d)\n", declinedAtOpen );
            return TA_BATCH_ARG_WRONG_CODE;
         }
         u6aFill++;

         /* ...and supplying it at the fill, whatever the opener was given. */
         for( k = 0; k < 8; k++ ) gotM[k] = gotF[k] = U6A_CANARY;
         if( TA_MAMA_OpenAndFill( &st, bars, 252, 0.5, 0.05, &beg, &nb, outA,
                                  declinedAtOpen ? NULL : outB ) != TA_SUCCESS ||
             TA_MAMA_UpdateAndFill( st, fillBars, 8, gotM, gotF ) != TA_SUCCESS )
         {
            printf( "\nFailed: supplying outFAMA at UpdateAndFill was rejected "
                    "(declinedAtOpen=%d)\n", declinedAtOpen );
            if( st ) TA_MAMA_Close( st );
            return TA_BATCH_ARG_CONTROL;
         }
         TA_MAMA_Close( st );
         st = NULL;
         for( k = 0; k < 8; k++ )
         {
            if( gotM[k] == U6A_CANARY || gotF[k] == U6A_CANARY )
            {
               printf( "\nFailed: the supplying fill did not write [%d]\n", k );
               return TA_BATCH_ARG_WRONG_CODE;
            }
         }
         if( memcmp( gotM, refM, sizeof(refM) ) != 0 ||
             memcmp( gotF, refF, sizeof(refF) ) != 0 )
         {
            printf( "\nFailed: the open's declination changed what the fill wrote "
                    "(declinedAtOpen=%d)\n", declinedAtOpen );
            return TA_BATCH_ARG_WRONG_CODE;
         }
         u6aFill++;
      }

      /* "May differ again on the NEXT call" -- the sentence the whole rule rests
       * on. One handle, three fills, alternating; each has to agree with an
       * oracle driven the same way with everything supplied. */
      {
         TA_MAMA_Stream *alt = NULL, *altRef = NULL;
         static double legBars[8], wantM[8], wantF[8];
         int leg, declineLeg;
         int altBeg = 0, altNb = 0, refBeg = 0, refNb = 0;

         if( TA_MAMA_OpenAndFill( &alt, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS ||
             TA_MAMA_OpenAndFill( &altRef, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS )
         {
            printf( "\nFailed: the alternating U6a opens did not run\n" );
            if( alt ) TA_MAMA_Close( alt );
            if( altRef ) TA_MAMA_Close( altRef );
            return TA_BATCH_ARG_CONTROL;
         }
         for( leg = 0; leg < 3; leg++ )
         {
            declineLeg = ( leg != 1 );
            for( k = 0; k < 8; k++ )
            {
               legBars[k] = fillBars[k] + (double)leg;
               wantM[k] = wantF[k] = gotM[k] = gotF[k] = U6A_CANARY;
            }
            if( TA_MAMA_UpdateAndFill( altRef, legBars, 8, wantM, wantF ) != TA_SUCCESS ||
                TA_MAMA_UpdateAndFill( alt, legBars, 8, gotM, declineLeg ? NULL : gotF ) != TA_SUCCESS )
            {
               printf( "\nFailed: an alternating leg was rejected (leg %d)\n", leg );
               TA_MAMA_Close( alt );
               TA_MAMA_Close( altRef );
               return TA_BATCH_ARG_CONTROL;
            }
            if( memcmp( gotM, wantM, sizeof(wantM) ) != 0 ||
                ( !declineLeg && memcmp( gotF, wantF, sizeof(wantF) ) != 0 ) )
            {
               printf( "\nFailed: an alternating leg diverged (leg %d)\n", leg );
               TA_MAMA_Close( alt );
               TA_MAMA_Close( altRef );
               return TA_BATCH_ARG_WRONG_CODE;
            }
            TA_StreamOutRange( alt, &altBeg, &altNb );
            TA_StreamOutRange( altRef, &refBeg, &refNb );
            if( altBeg != refBeg || altNb != refNb )
            {
               printf( "\nFailed: an alternating leg moved the range (leg %d)\n", leg );
               TA_MAMA_Close( alt );
               TA_MAMA_Close( altRef );
               return TA_BATCH_ARG_WRONG_CODE;
            }
         }
         if( TA_MAMA_Peek( alt, bars[251], &pm, &pf ) != TA_SUCCESS ||
             TA_MAMA_Peek( altRef, bars[251], &rpm, &rpf ) != TA_SUCCESS ||
             memcmp( &pf, &rpf, sizeof(double) ) != 0 ||
             memcmp( &pm, &rpm, sizeof(double) ) != 0 )
         {
            printf( "\nFailed: alternating the declined set changed the handle\n" );
            TA_MAMA_Close( alt );
            TA_MAMA_Close( altRef );
            return TA_BATCH_ARG_WRONG_CODE;
         }
         TA_MAMA_Close( alt );
         TA_MAMA_Close( altRef );
         u6aFill++;
      }

      /* C alone can decline at the SCALAR entry points: Update and Peek take an
       * out-parameter per output, where the other three return the value. Same
       * rule, same per-call reading. */
      {
         double bothM = 0.0, bothF = 0.0, soloM = 0.0, peekBothM = 0.0, peekBothF = 0.0, peekSoloM = 0.0;
         TA_MAMA_Stream *ref2 = NULL;

         if( TA_MAMA_OpenAndFill( &ref2, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS ||
             TA_MAMA_OpenAndFill( &st, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS )
         {
            printf( "\nFailed: the scalar U6a opens did not run\n" );
            if( ref2 ) TA_MAMA_Close( ref2 );
            if( st ) TA_MAMA_Close( st );
            return TA_BATCH_ARG_CONTROL;
         }
         if( TA_MAMA_Update( ref2, fillBars[0], &bothM, &bothF ) != TA_SUCCESS ||
             TA_MAMA_Peek( ref2, fillBars[1], &peekBothM, &peekBothF ) != TA_SUCCESS ||
             TA_MAMA_Update( st, fillBars[0], &soloM, NULL ) != TA_SUCCESS ||
             TA_MAMA_Peek( st, fillBars[1], &peekSoloM, NULL ) != TA_SUCCESS )
         {
            printf( "\nFailed: declining outFAMA at Update or Peek was rejected\n" );
            TA_MAMA_Close( ref2 );
            TA_MAMA_Close( st );
            return TA_BATCH_ARG_CONTROL;
         }
         /* Not merely accepted: the supplied output is the same value, and the
          * NEXT bar is too -- which is what fails if declining stopped the
          * computation FAMA feeds back. */
         if( memcmp( &soloM, &bothM, sizeof(double) ) != 0 ||
             memcmp( &peekSoloM, &peekBothM, sizeof(double) ) != 0 )
         {
            printf( "\nFailed: declining outFAMA at Update changed outMAMA\n" );
            TA_MAMA_Close( ref2 );
            TA_MAMA_Close( st );
            return TA_BATCH_ARG_WRONG_CODE;
         }
         TA_MAMA_Close( ref2 );
         TA_MAMA_Close( st );
         st = NULL;
         u6aFill++;
      }

      /* Declining the nullable output did not make the REQUIRED one optional. */
      if( TA_MAMA_OpenAndFill( &st, bars, 252, 0.5, 0.05, &beg, &nb, outA, outB ) != TA_SUCCESS )
      {
         printf( "\nFailed: the U6a control open did not run\n" );
         return TA_BATCH_ARG_CONTROL;
      }
      TA_StreamOutRange( st, &beg2, &nbBefore );
      if( TA_MAMA_UpdateAndFill( st, fillBars, 8, NULL, NULL ) != TA_BAD_PARAM )
      {
         printf( "\nFailed: an absent outMAMA is still an absent argument\n" );
         TA_MAMA_Close( st );
         return TA_BATCH_ARG_WRONG_CODE;
      }
      u6aFill++;
      TA_StreamOutRange( st, &beg2, &nb2 );
      TA_MAMA_Close( st );
      st = NULL;
      if( nb2 != nbBefore )
      {
         printf( "\nFailed: a rejected UpdateAndFill committed bars (%d)\n", nb2 );
         return TA_BATCH_ARG_WRONG_CODE;
      }
      u6aFill++;
   }

   /* Literal floors: a count derived from the cases above would move with a
    * deleted case and still pass. */
   if( bacReject < 19 || bacAccept < 10 )
   {
      printf( "\nFailed: the batch argument gate ran fewer checks than it was "
              "written with\n" );
      return TA_BATCH_ARG_VACUOUS;
   }
   if( s4Reject < 12 || s4Accept < 5 )
   {
      printf( "\nFailed: the streaming argument gate ran fewer checks than it "
              "was written with\n" );
      return TA_BATCH_ARG_VACUOUS;
   }
   if( u6aFill < 15 )
   {
      printf( "\nFailed: the declined-at-UpdateAndFill gate ran fewer checks "
              "than it was written with\n" );
      return TA_BATCH_ARG_VACUOUS;
   }

   return freeLib();
}

/* The published value of every enumerator that has ever shipped, pinned.
 *
 * TA_MAType and TA_FuncUnstId are ABI: wrappers record these numbers and pass
 * them back as plain integers, so renumbering one silently re-points a caller at
 * a different function. That is not hypothetical -- TA_FUNC_UNST_IMI was once
 * inserted mid-enum, shifting every id above it, and ta-lib-python still targets
 * the pre-0.6.0 numbering as a result. Nothing detected it, because both ends of
 * a set/get pair use the same wrong id and round-trip perfectly.
 *
 * The contract is APPEND-ONLY: a new indicator adds a row here and to
 * enums.yaml; an existing row must never change. The literals are deliberately
 * hardcoded -- comparing an enumerator to itself would prove nothing.
 *
 * This pins C only. For TA_MAType and TA_FuncUnstId that is sufficient: every
 * other surface (the Rust crate, the shipped Java enum, all four servers) is
 * generated from the same enums.yaml that generates this header, and the
 * generator already fails if the hand-maintained Rust copy drifts from it.
 *
 * TA_RetCode is NOT in enums.yaml and nothing generates its Rust, Java or C#
 * copies -- each is hand-written, and the number a backend puts on the wire is
 * carried by the member there (Rust `as_c_int`, Java `asCInt`, C#'s explicit
 * discriminants). So for TA_RetCode this file pins the C numbering only, and
 * cross-language agreement is what the ta_regtest server comparison tests.
 */
static ErrorNumber testEnumValueContract( void )
{
   typedef struct { const char *name; int shipped; int current; } EnumPin;

   /* Retired ids keep their slot forever (removing one renumbers the rest). */
   static const EnumPin unstPins[] = {
      { "TA_FUNC_UNST_ADX",           0, TA_FUNC_UNST_ADX },
      { "TA_FUNC_UNST_UNUSED_1",      1, TA_FUNC_UNST_UNUSED_1 },
      { "TA_FUNC_UNST_ATR",           2, TA_FUNC_UNST_ATR },
      { "TA_FUNC_UNST_CMO",           3, TA_FUNC_UNST_CMO },
      { "TA_FUNC_UNST_DX",            4, TA_FUNC_UNST_DX },
      { "TA_FUNC_UNST_EMA",           5, TA_FUNC_UNST_EMA },
      { "TA_FUNC_UNST_HT_DCPERIOD",   6, TA_FUNC_UNST_HT_DCPERIOD },
      { "TA_FUNC_UNST_HT_DCPHASE",    7, TA_FUNC_UNST_HT_DCPHASE },
      { "TA_FUNC_UNST_HT_PHASOR",     8, TA_FUNC_UNST_HT_PHASOR },
      { "TA_FUNC_UNST_HT_SINE",       9, TA_FUNC_UNST_HT_SINE },
      { "TA_FUNC_UNST_HT_TRENDLINE", 10, TA_FUNC_UNST_HT_TRENDLINE },
      { "TA_FUNC_UNST_HT_TRENDMODE", 11, TA_FUNC_UNST_HT_TRENDMODE },
      { "TA_FUNC_UNST_UNUSED_12",    12, TA_FUNC_UNST_UNUSED_12 },
      { "TA_FUNC_UNST_KAMA",         13, TA_FUNC_UNST_KAMA },
      { "TA_FUNC_UNST_MAMA",         14, TA_FUNC_UNST_MAMA },
      { "TA_FUNC_UNST_UNUSED_15",    15, TA_FUNC_UNST_UNUSED_15 },
      { "TA_FUNC_UNST_MINUS_DI",     16, TA_FUNC_UNST_MINUS_DI },
      { "TA_FUNC_UNST_MINUS_DM",     17, TA_FUNC_UNST_MINUS_DM },
      { "TA_FUNC_UNST_NATR",         18, TA_FUNC_UNST_NATR },
      { "TA_FUNC_UNST_PLUS_DI",      19, TA_FUNC_UNST_PLUS_DI },
      { "TA_FUNC_UNST_PLUS_DM",      20, TA_FUNC_UNST_PLUS_DM },
      { "TA_FUNC_UNST_RSI",          21, TA_FUNC_UNST_RSI },
      { "TA_FUNC_UNST_UNUSED_22",    22, TA_FUNC_UNST_UNUSED_22 },
      { "TA_FUNC_UNST_T3",           23, TA_FUNC_UNST_T3 },
      { "TA_FUNC_UNST_RMA",          24, TA_FUNC_UNST_RMA },
      /* Pinned so adding an indicator can never move it (#144). */
      { "TA_FUNC_UNST_ALL",       65535, TA_FUNC_UNST_ALL }
   };

   static const EnumPin maPins[] = {
      { "TA_MAType_SMA",       0, TA_MAType_SMA },
      { "TA_MAType_EMA",       1, TA_MAType_EMA },
      { "TA_MAType_WMA",       2, TA_MAType_WMA },
      { "TA_MAType_DEMA",      3, TA_MAType_DEMA },
      { "TA_MAType_TEMA",      4, TA_MAType_TEMA },
      { "TA_MAType_TRIMA",     5, TA_MAType_TRIMA },
      { "TA_MAType_KAMA",      6, TA_MAType_KAMA },
      { "TA_MAType_MAMA",      7, TA_MAType_MAMA },
      { "TA_MAType_T3",        8, TA_MAType_T3 },
      { "TA_MAType_HMA",       9, TA_MAType_HMA },
      { "TA_MAType_DISABLED", 10, TA_MAType_DISABLED },
      { "TA_MAType_DEFAULT",  11, TA_MAType_DEFAULT },
      { "TA_MAType_ZLEMA",    12, TA_MAType_ZLEMA }
   };

   /* Returned to every caller and mapped by name in the wrappers (ta-lib-python
    * hardcodes this whole list in _ta_check_success), so it is ABI too.
    */
   static const EnumPin retCodePins[] = {
      { "TA_SUCCESS",                       0, TA_SUCCESS },
      { "TA_LIB_NOT_INITIALIZE",            1, TA_LIB_NOT_INITIALIZE },
      { "TA_BAD_PARAM",                     2, TA_BAD_PARAM },
      { "TA_ALLOC_ERR",                     3, TA_ALLOC_ERR },
      { "TA_GROUP_NOT_FOUND",               4, TA_GROUP_NOT_FOUND },
      { "TA_FUNC_NOT_FOUND",                5, TA_FUNC_NOT_FOUND },
      { "TA_INVALID_HANDLE",                6, TA_INVALID_HANDLE },
      { "TA_INVALID_PARAM_HOLDER",          7, TA_INVALID_PARAM_HOLDER },
      { "TA_INVALID_PARAM_HOLDER_TYPE",     8, TA_INVALID_PARAM_HOLDER_TYPE },
      { "TA_INVALID_PARAM_FUNCTION",        9, TA_INVALID_PARAM_FUNCTION },
      { "TA_INPUT_NOT_ALL_INITIALIZE",     10, TA_INPUT_NOT_ALL_INITIALIZE },
      { "TA_OUTPUT_NOT_ALL_INITIALIZE",    11, TA_OUTPUT_NOT_ALL_INITIALIZE },
      { "TA_OUT_OF_RANGE_START_INDEX",     12, TA_OUT_OF_RANGE_START_INDEX },
      { "TA_OUT_OF_RANGE_END_INDEX",       13, TA_OUT_OF_RANGE_END_INDEX },
      { "TA_INVALID_LIST_TYPE",            14, TA_INVALID_LIST_TYPE },
      { "TA_BAD_OBJECT",                   15, TA_BAD_OBJECT },
      { "TA_NOT_SUPPORTED",                16, TA_NOT_SUPPORTED },
      { "TA_INSUFFICIENT_HISTORY",         17, TA_INSUFFICIENT_HISTORY },
      { "TA_INTERNAL_ERROR",             5000, TA_INTERNAL_ERROR },
      { "TA_UNKNOWN_ERR",              0xFFFF, TA_UNKNOWN_ERR }
   };

   /* TA_SetCandleSettings takes both of these from the caller, so they are ABI
    * on the same terms. TA_AllCandleSettings is the count as well as the "all"
    * selector -- it sizes TA_Globals->candleSettings[] -- so it is pinned last
    * and excluded from the member count below, like TA_FUNC_UNST_ALL. */
   static const EnumPin candlePins[] = {
      { "TA_BodyLong",           0, TA_BodyLong },
      { "TA_BodyVeryLong",       1, TA_BodyVeryLong },
      { "TA_BodyShort",          2, TA_BodyShort },
      { "TA_BodyDoji",           3, TA_BodyDoji },
      { "TA_ShadowLong",         4, TA_ShadowLong },
      { "TA_ShadowVeryLong",     5, TA_ShadowVeryLong },
      { "TA_ShadowShort",        6, TA_ShadowShort },
      { "TA_ShadowVeryShort",    7, TA_ShadowVeryShort },
      { "TA_Near",               8, TA_Near },
      { "TA_Far",                9, TA_Far },
      { "TA_Equal",             10, TA_Equal },
      { "TA_AllCandleSettings", 11, TA_AllCandleSettings }
   };

   static const EnumPin rangePins[] = {
      { "TA_RangeType_RealBody", 0, TA_RangeType_RealBody },
      { "TA_RangeType_HighLow",  1, TA_RangeType_HighLow },
      { "TA_RangeType_Shadows",  2, TA_RangeType_Shadows }
   };

   /* Every pinned unstable id except the trailing ALL wildcard. */
   const int nbUnstIds = (int)(sizeof(unstPins)/sizeof(unstPins[0])) - 1;
   /* Likewise: the real settings, excluding the TA_AllCandleSettings selector. */
   const int nbCandleTypes = (int)(sizeof(candlePins)/sizeof(candlePins[0])) - 1;
   unsigned int i;

   for( i=0; i < sizeof(retCodePins)/sizeof(retCodePins[0]); i++ )
   {
      if( retCodePins[i].current != retCodePins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        every wrapper maps them by number. Append, never renumber.\n",
                 retCodePins[i].name, retCodePins[i].current, retCodePins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
      }
   }

   /* Completeness for the return codes. There is no TA_RETCODE_COUNT to compare
    * against -- the list lives in src/ta_common/ta_retcode.csv and is generated
    * into a table this file cannot see -- but TA_SetRetCodeInfo answers
    * "TA_UNKNOWN_ERR" for anything absent from that table, so probing the value
    * space finds a code that was added to the csv and never pinned here. The
    * 5000-5999 band reports TA_INTERNAL_ERROR for all 1000 values, so only its
    * first needs a row. */
   {
      unsigned long v;
      for( v = 0; v <= 0xFFFFUL; v++ )
      {
         TA_RetCodeInfo info;
         unsigned int p;
         int pinned = 0;

         if( v > 5000 && v <= 5999 ) continue;   /* one code, whole band */

         TA_SetRetCodeInfo( (TA_RetCode)v, &info );
         if( v != 0xFFFFUL && strcmp( info.enumStr, "TA_UNKNOWN_ERR" ) == 0 )
            continue;                            /* not a defined code */

         for( p=0; p < sizeof(retCodePins)/sizeof(retCodePins[0]); p++ )
            if( (unsigned long)retCodePins[p].shipped == v ) { pinned = 1; break; }

         if( !pinned )
         {
            printf( "\nFailed: TA_RetCode %lu (%s) is defined but not pinned. Add its\n"
                    "        row to retCodePins[] (append only -- never renumber).\n",
                    v, info.enumStr );
            return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
         }
      }
   }

   /* The other direction, and the one the probe above cannot see. It observes
    * TA_SetRetCodeInfo, whose table comes from src/ta_common/ta_retcode.csv --
    * so a member added to the ta_defs.h enum and to this table, but NOT to the
    * csv, reads as "not a defined code" and is skipped silently. Every pin above
    * names an enumerator that exists (it is compiled), so requiring the table to
    * know it by that name is exactly the enum -> csv check.
    *
    * TA_UNKNOWN_ERR is the table's not-found answer and is not a csv row; it
    * round-trips through this check for the same reason, so it needs no
    * exemption.
    */
   for( i=0; i < sizeof(retCodePins)/sizeof(retCodePins[0]); i++ )
   {
      TA_RetCodeInfo info;

      /* TA_SetRetCodeInfo answers the 5000-5999 band from a hardcoded literal
       * before it consults the table at all, so this check cannot see that row
       * -- it would compare the trap's "TA_INTERNAL_ERROR" against the pin's
       * identical text whether the csv row exists, is renamed, or is deleted.
       * Skipped rather than left to pass vacuously. The band itself is pinned by
       * the probe above, which walks the whole value space.
       */
      if( retCodePins[i].shipped >= 5000 && retCodePins[i].shipped <= 5999 )
         continue;

      TA_SetRetCodeInfo( (TA_RetCode)retCodePins[i].shipped, &info );
      if( strcmp( info.enumStr, retCodePins[i].name ) != 0 )
      {
         printf( "\nFailed: TA_RetCode %d is %s in include/ta_defs.h but %s in\n"
                 "        src/ta_common/ta_retcode.csv. The two are hand-written and\n"
                 "        nothing else compares them -- add the missing row.\n",
                 retCodePins[i].shipped, retCodePins[i].name, info.enumStr );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
      }
   }

   for( i=0; i < sizeof(candlePins)/sizeof(candlePins[0]); i++ )
   {
      if( candlePins[i].current != candlePins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        a caller passes them to TA_SetCandleSettings. Append, never renumber.\n",
                 candlePins[i].name, candlePins[i].current, candlePins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
      }
   }

   for( i=0; i < sizeof(rangePins)/sizeof(rangePins[0]); i++ )
   {
      if( rangePins[i].current != rangePins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        a caller passes them to TA_SetCandleSettings. Append, never renumber.\n",
                 rangePins[i].name, rangePins[i].current, rangePins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
      }
   }

   /* Same completeness rule as the unstable ids: TA_AllCandleSettings doubles as
    * the member count, so a new setting that does not gain a row here would sit
    * unpinned. It also sizes the defaults table in ta_global.c -- see the guard
    * there, which turns the same mistake into a clean error rather than a read
    * past the end. */
   if( (int)TA_AllCandleSettings != nbCandleTypes )
   {
      printf( "\nFailed: TA_AllCandleSettings is %d but %d setting(s) are pinned. Add\n"
              "        the new setting's row to candlePins[] (append only).\n",
              (int)TA_AllCandleSettings, nbCandleTypes );
      return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
   }

   for( i=0; i < sizeof(unstPins)/sizeof(unstPins[0]); i++ )
   {
      if( unstPins[i].current != unstPins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        wrappers pass them back as integers. Append new ids, never renumber.\n",
                 unstPins[i].name, unstPins[i].current, unstPins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_0;
      }
   }

   for( i=0; i < sizeof(maPins)/sizeof(maPins[0]); i++ )
   {
      if( maPins[i].current != maPins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        they are the optInMAType a caller passes. Append, never renumber.\n",
                 maPins[i].name, maPins[i].current, maPins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_1;
      }
   }

   /* Same completeness rule for MAType, which has no _COUNT to compare against:
    * use the shipped choice list, generated from the same enums.yaml. Without
    * this, an appended member sits unpinned and can later be renumbered. */
   {
      const TA_FuncHandle *maHandle;
      const TA_OptInputParameterInfo *maOpt;
      const int nbMaPins = (int)(sizeof(maPins)/sizeof(maPins[0]));
      if( TA_GetFuncHandle( "MA", &maHandle ) != TA_SUCCESS ||
          TA_GetOptInputParameterInfo( maHandle, 1, &maOpt ) != TA_SUCCESS ||
          maOpt->type != TA_OptInput_IntegerList || !maOpt->dataSet ||
          strcmp( maOpt->paramName, "optInMAType" ) != 0 )
      {
         printf( "\nFailed: cannot reach MA's optInMAType choice list to count MAType members\n" );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_1;
      }
      if( (int)((const TA_IntegerList *)maOpt->dataSet)->nbElement != nbMaPins )
      {
         printf( "\nFailed: MAType has %d member(s) but %d are pinned. Add the new\n"
                 "        member's row to maPins[] (append only -- never renumber).\n",
                 (int)((const TA_IntegerList *)maOpt->dataSet)->nbElement, nbMaPins );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_1;
      }
   }

   /* The count must equal the number of ids pinned above -- not merely be no
    * smaller. A ">=" test would let a newly added indicator sit unpinned
    * indefinitely, which is exactly the protection this table exists to give:
    * adding an id has to fail here until its row is added, or the contract only
    * covers whatever someone remembered to write down.
    */
   if( TA_FUNC_UNST_COUNT != nbUnstIds )
   {
      printf( "\nFailed: TA_FUNC_UNST_COUNT is %d but %d ids are pinned. Add the new\n"
              "        id's row to unstPins[] (append only -- never renumber).\n",
              TA_FUNC_UNST_COUNT, nbUnstIds );
      return TA_INTERNAL_ENUM_CONTRACT_FAIL_2;
   }

   /* Ids grow upward and the wildcard is fixed, so they must never meet. */
   if( TA_FUNC_UNST_COUNT >= TA_FUNC_UNST_ALL )
   {
      printf( "\nFailed: TA_FUNC_UNST_COUNT (%d) has reached TA_FUNC_UNST_ALL (%d)\n",
              TA_FUNC_UNST_COUNT, (int)TA_FUNC_UNST_ALL );
      return TA_INTERNAL_ENUM_CONTRACT_FAIL_2;
   }

   return TA_TEST_PASS;
}

/* TA_Set/GetUnstablePeriod index TA_Globals->unstablePeriod[id] after a bound
 * check that used to test only the upper end. TA_TEST_UNST_NONE is -1 and makes
 * the enum signed, so every negative id slipped past and read/wrote off the
 * front of the array -- onto TA_Globals->compatibility, which sits immediately
 * before it. The setter still returned TA_SUCCESS while silently corrupting the
 * global (issue #144).
 *
 * Asserted here: both sentinels and an arbitrary negative are rejected, the
 * wildcard still sets every function, and a normal id still round-trips.
 * Non-vacuity: the setter half is caught by the returned TA_BAD_PARAM, and the
 * getter half only because compatibility is parked at a non-zero value first --
 * otherwise an out-of-bounds read of it returns 0 and looks correct.
 */
static ErrorNumber testUnstablePeriodBounds( void )
{
   ErrorNumber retValue;
   TA_RetCode retCode;
   int id;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* Park a non-zero value in the field that unstablePeriod[-1] aliases, so the
    * assertions below can tell a real guard from an accidental zero. Without
    * this the getter checks pass even with the guard reverted, because a fresh
    * TA_Initialize leaves compatibility == 0 and an out-of-bounds read of it
    * looks exactly like the correct answer.
    */
   TA_SetCompatibility( TA_COMPATIBILITY_METASTOCK );

   /* Out-of-range ids must be refused, not indexed. */
   if( TA_SetUnstablePeriod( TA_TEST_UNST_NONE, 99 ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( (TA_FuncUnstId)-1000000, 99 ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( (TA_FuncUnstId)TA_FUNC_UNST_COUNT, 99 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: out-of-range TA_SetUnstablePeriod id not rejected\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_0;
   }

   /* ...and must not have written anything. TA_Globals->compatibility is the
    * field the id == -1 write landed on.
    */
   if( TA_GetCompatibility() != TA_COMPATIBILITY_METASTOCK )
   {
      printf( "\nFailed: rejected TA_SetUnstablePeriod id corrupted compatibility\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_1;
   }

   /* Reads of the sentinels are defined as 0, never an out-of-bounds load. With
    * compatibility == METASTOCK (1) above, an unguarded read of [-1] yields 1
    * and fails here.
    */
   if( TA_GetUnstablePeriod( TA_TEST_UNST_NONE ) != 0 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_ALL ) != 0 ||
       TA_GetUnstablePeriod( (TA_FuncUnstId)-1000000 ) != 0 )
   {
      printf( "\nFailed: sentinel TA_GetUnstablePeriod did not read as 0\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_2;
   }

   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   /* The valid range still works: the wildcard sets every function, a single
    * id round-trips, and neither disturbs compatibility.
    */
   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 7 );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed: TA_SetUnstablePeriod wildcard RetCode = %d\n", retCode );
      return TA_INTERNAL_UNST_BOUND_FAIL_3;
   }
   for( id=0; id < TA_FUNC_UNST_COUNT; id++ )
   {
      if( TA_GetUnstablePeriod( (TA_FuncUnstId)id ) != 7 )
      {
         printf( "\nFailed: TA_SetUnstablePeriod wildcard missed id %d\n", id );
         return TA_INTERNAL_UNST_BOUND_FAIL_3;
      }
   }

   /* The VALUE dimension. The id has been bounded since #144; the period never
    * was, and it is added to a lookback that is then used as an index -- so a
    * huge one overflows the lookback NEGATIVE and the call indexes ~2^31 bars
    * forward. TA_MAX_INDEX is the ceiling the index space already uses, and a
    * warm-up beyond it could never produce output anyway.
    */
   if( TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (unsigned int)TA_MAX_INDEX + 1 ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 2147483647u ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 4294967295u ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 2147483647u ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetUnstablePeriod accepted a period that overflows the lookback\n" );
      return TA_INTERNAL_UNST_VALUE_FAIL;
   }

   /* A rejected period must not have been stored, by either the single-id or
    * the wildcard path -- 7 is what the wildcard set above. */
   if( TA_GetUnstablePeriod( TA_FUNC_UNST_EMA ) != 7 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_ADX ) != 7 )
   {
      printf( "\nFailed: a rejected TA_SetUnstablePeriod still wrote the value\n" );
      return TA_INTERNAL_UNST_VALUE_FAIL;
   }

   /* The ceiling itself is accepted: the guard is a bound, not an off-by-one. */
   if( TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, (unsigned int)TA_MAX_INDEX ) != TA_SUCCESS ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_EMA ) != (unsigned int)TA_MAX_INDEX )
   {
      printf( "\nFailed: TA_SetUnstablePeriod rejected the TA_MAX_INDEX ceiling\n" );
      return TA_INTERNAL_UNST_VALUE_FAIL;
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 7 );

   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 3 );
   if( retCode != TA_SUCCESS ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_RSI ) != 3 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_EMA ) != 7 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_ADX ) != 7 ||
       TA_GetCompatibility() != TA_COMPATIBILITY_DEFAULT )
   {
      printf( "\nFailed: single-id TA_SetUnstablePeriod round-trip\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_3;
   }

   /* Pairs with the allocLib() above (as testCircularBuffer does) -- shutting
    * down zeroes TA_Globals, so the periods set here cannot leak into any
    * later test.
    */
   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

#define CANDLE_NB_BAR 64

/* Runs CDLDOJI over the whole series and checks the lookback against what the
 * call reports. Returns the number of pattern hits, or -1 on a parity failure.
 */
static int checkDoji( const double *inOpen, const double *inHigh,
                      const double *inLow,  const double *inClose )
{
   int outInteger[CANDLE_NB_BAR];
   int outBegIdx, outNbElement, lookback, i, nbHit;
   TA_RetCode retCode;

   lookback = TA_CDLDOJI_Lookback();
   retCode  = TA_CDLDOJI( 0, CANDLE_NB_BAR-1, inOpen, inHigh, inLow, inClose,
                          &outBegIdx, &outNbElement, outInteger );

   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed: TA_CDLDOJI RetCode = %d (lookback = %d)\n",
              (int)retCode, lookback );
      return -1;
   }

   /* A negative lookback is the tier disagreeing with itself: the call just
    * succeeded, so the lookback cannot be saying "rejected".
    */
   if( lookback < 0 )
   {
      printf( "\nFailed: TA_CDLDOJI_Lookback = %d but the call returned TA_SUCCESS\n",
              lookback );
      return -1;
   }

   /* A lookback longer than the series produces nothing, reported as the
    * (0,0) empty result rather than as an error.
    */
   if( lookback > CANDLE_NB_BAR-1 )
   {
      if( outBegIdx != 0 || outNbElement != 0 )
      {
         printf( "\nFailed: TA_CDLDOJI returned %d element(s) at lookback %d over %d bars\n",
                 outNbElement, lookback, CANDLE_NB_BAR );
         return -1;
      }
      return 0;
   }

   /* startIdx == 0, so the function consumed exactly `lookback` leading bars.
    * This is what the shift breaks: it reported outBegIdx = 0 with
    * CANDLE_NB_BAR-lookback... elements while the values were |avgPeriod|
    * bars later than advertised.
    */
   if( outBegIdx != lookback || outNbElement != CANDLE_NB_BAR - lookback )
   {
      printf( "\nFailed: TA_CDLDOJI outBegIdx = %d outNbElement = %d, lookback = %d\n",
              outBegIdx, outNbElement, lookback );
      return -1;
   }

   nbHit = 0;
   for( i=0; i < outNbElement; i++ )
      if( outInteger[i] != 0 )
         nbHit++;

   return nbHit;
}

/* The other global setter, and the same defect (issue #185).
 * TA_SetCandleSettings validated `settingType` and nothing else, so a negative
 * `avgPeriod` reached all 61 CDL* bodies: CDLDOJI's lookback returned -1 while
 * TA_CDLDOJI returned TA_SUCCESS with every value shifted under an *outBegIdx
 * still reporting startIdx. Above TA_MAX_INDEX is the mirror image -- the
 * `max(...)+N` lookbacks overflow signed-negative into that same state
 * (-2147483647 out of CDLEVENINGDOJISTAR in practice).
 *
 * The contract asserted here is the lookback/call tier parity the boundary
 * sweep enforces for a function's OWN optional parameters, which by
 * construction it cannot reach through a global setter: a setting the setter
 * ACCEPTS must produce a non-negative lookback and a call whose *outBegIdx and
 * element count agree with it, and a setting it REJECTS must leave the previous
 * one in place.
 *
 * Non-vacuity: `checkDoji` asserts the parity positively on every accepted
 * setting (not just the absence of a bad return code), and the final leg
 * requires a tuned factor to actually change CDLDOJI's output -- a setter that
 * had started rejecting everything would fail there rather than pass quietly.
 */
static ErrorNumber testCandleSettingsBounds( void )
{
   double inOpen[CANDLE_NB_BAR], inHigh[CANDLE_NB_BAR];
   double inLow[CANDLE_NB_BAR], inClose[CANDLE_NB_BAR];
   ErrorNumber retValue;
   int nbHitDefault, nbHitTuned;
   int i;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* A deterministic series with a mix of doji and non-doji bars, so the last
    * leg below can tell "the setting took effect" from "everything matches".
    */
   for( i=0; i < CANDLE_NB_BAR; i++ )
   {
      inOpen[i]  = 100.0 + (double)(i % 7);
      inClose[i] = 100.0 + (double)((i * 3) % 5);
      inHigh[i]  = (inOpen[i] > inClose[i] ? inOpen[i] : inClose[i]) + 1.0;
      inLow[i]   = (inOpen[i] < inClose[i] ? inOpen[i] : inClose[i]) - 1.0;
   }

   nbHitDefault = checkDoji( inOpen, inHigh, inLow, inClose );
   if( nbHitDefault < 0 )
      return TA_INTERNAL_CANDLE_BOUND_FAIL_0;

   /* The reported defect. Both a plain -1 and the extreme, since a guard
    * written as a magnitude test rather than a sign test would let INT_MIN
    * through.
    */
   if( TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, -1, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, -10, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, INT_MIN, 0.1 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetCandleSettings accepted a negative avgPeriod\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }

   /* The other end: an avgPeriod above the index space overflows the
    * `max(...)+N` lookbacks. TA_MAX_INDEX is the ceiling TA_SetUnstablePeriod
    * already uses for the same reason -- a warm-up longer than the largest
    * addressable series can never produce output.
    */
   if( TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, TA_MAX_INDEX+1, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, INT_MAX, 0.1 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetCandleSettings accepted an avgPeriod that overflows the lookback\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }

   /* An out-of-domain rangeType reaches the fall-through arm of TA_CANDLERANGE,
    * which evaluates to 0 -- every range zero, every threshold zero, and a
    * silently meaningless result rather than an error. The domain is the
    * member list, so 3 is as invalid as -1.
    */
   if( TA_SetCandleSettings( TA_BodyDoji, (TA_RangeType)3, 10, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( TA_BodyDoji, (TA_RangeType)-1, 10, 0.1 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetCandleSettings accepted an out-of-domain rangeType\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }

   /* factor takes any finite value -- it scales a threshold, never an index --
    * but not NaN, which silences every comparison it feeds. Both halves are
    * asserted: a guard written as a range check would accept NaN (every
    * comparison against it is false), and one written as `!(factor > 0)` would
    * refuse the legal negative.
    */
   if( TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, 10, NAN ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetCandleSettings accepted a NaN factor\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }
   if( TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, 10, -1.0 ) != TA_SUCCESS ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, 10, 0.0 ) != TA_SUCCESS )
   {
      printf( "\nFailed: TA_SetCandleSettings refused a legal factor\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }
   if( TA_RestoreCandleDefaultSettings( TA_BodyDoji ) != TA_SUCCESS )
   {
      printf( "\nFailed: TA_RestoreCandleDefaultSettings( TA_BodyDoji )\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_1;
   }

   /* Nothing above was stored: the defaults are still in force, and the tiers
    * still agree.
    */
   if( checkDoji( inOpen, inHigh, inLow, inClose ) != nbHitDefault )
   {
      printf( "\nFailed: a rejected TA_SetCandleSettings still changed the setting\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_2;
   }

   /* settingType: the wildcard is not a single-setting target, and a negative
    * one must not index candleSettings[-1] -- which lands on the tail of
    * TA_Globals->unstablePeriod[], the same adjacency #144 fixed for the other
    * setter. Live only where the enum's underlying type is signed (MSVC gives
    * enums `int`); gcc and clang pick `unsigned int` here because the enum
    * declares no negative member, so the value already wraps out of range and
    * this leg is inert there. The periods are parked at 7 first so that ANY of
    * the four fields the setter writes is detectable -- a fresh library leaves
    * them at 0, and a clobber that happened to write 0 would look untouched.
    */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 7 );
   if( TA_SetCandleSettings( TA_AllCandleSettings, TA_RangeType_HighLow, 10, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( (TA_CandleSettingType)-1, TA_RangeType_HighLow, 10, 0.1 ) != TA_BAD_PARAM ||
       TA_SetCandleSettings( (TA_CandleSettingType)-1000000, TA_RangeType_HighLow, 10, 0.1 ) != TA_BAD_PARAM ||
       TA_RestoreCandleDefaultSettings( (TA_CandleSettingType)-1 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: TA_SetCandleSettings accepted an out-of-domain settingType\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_3;
   }
   for( i=0; i < TA_FUNC_UNST_COUNT; i++ )
   {
      if( TA_GetUnstablePeriod( (TA_FuncUnstId)i ) != 7 )
      {
         printf( "\nFailed: a rejected settingType wrote onto unstablePeriod[%d]\n", i );
         return TA_INTERNAL_CANDLE_BOUND_FAIL_3;
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* The valid domain still works, bounds included -- the guards are bounds,
    * not off-by-ones. avgPeriod 0 is the "compare with the current candle"
    * mode the defaults use for ShadowLong/ShadowVeryLong, and TA_MAX_INDEX is
    * the ceiling itself.
    */
   if( TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_RealBody, 0, 0.1 ) != TA_SUCCESS ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_Shadows, TA_MAX_INDEX, 0.1 ) != TA_SUCCESS ||
       TA_SetCandleSettings( TA_Equal, TA_RangeType_HighLow, 5, 0.05 ) != TA_SUCCESS )
   {
      printf( "\nFailed: TA_SetCandleSettings rejected a valid setting\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_4;
   }

   /* At TA_MAX_INDEX the lookback swallows the whole series, so the tiers must
    * still agree on an empty result rather than the call inventing one.
    */
   if( checkDoji( inOpen, inHigh, inLow, inClose ) != 0 )
   {
      printf( "\nFailed: TA_CDLDOJI produced output at an avgPeriod of TA_MAX_INDEX\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_4;
   }

   /* ...and the setter still does its job. A huge factor makes every bar a
    * doji; if this matched the default count the guards would be rejecting
    * everything and every assertion above would be vacuous.
    */
   if( TA_RestoreCandleDefaultSettings( TA_AllCandleSettings ) != TA_SUCCESS ||
       TA_SetCandleSettings( TA_BodyDoji, TA_RangeType_HighLow, 10, 1.0e9 ) != TA_SUCCESS )
   {
      printf( "\nFailed: TA_SetCandleSettings rejected the tuned setting\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_4;
   }
   nbHitTuned = checkDoji( inOpen, inHigh, inLow, inClose );
   if( nbHitTuned < 0 )
      return TA_INTERNAL_CANDLE_BOUND_FAIL_4;
   if( nbHitTuned <= nbHitDefault )
   {
      printf( "\nFailed: TA_SetCandleSettings had no effect (%d hits, default %d)\n",
              nbHitTuned, nbHitDefault );
      return TA_INTERNAL_CANDLE_VACUOUS;
   }

   /* Leave the globals as they were found: freeLib() zeroes them, but the
    * defaults are what every later test expects if it does not re-init.
    */
   if( TA_RestoreCandleDefaultSettings( TA_AllCandleSettings ) != TA_SUCCESS )
   {
      printf( "\nFailed: TA_RestoreCandleDefaultSettings\n" );
      return TA_INTERNAL_CANDLE_BOUND_FAIL_4;
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/* codegen_appendf/codegen_appendc replaced the
 *    pos += snprintf(buf + pos, buf_size - pos, ...)
 * idiom used to build every JSON-RPC request/response in the test tools.
 * That idiom lets `pos` run past `buf_size` on the first truncation; the next
 * call then passes a negative int that converts to a huge size_t and writes
 * past the buffer (CodeQL cpp/overflowing-snprintf, CWE-190/CWE-253).
 *
 * The contract asserted here: `pos` NEVER exceeds `buf_size - 1`, the buffer
 * stays NUL-terminated, and nothing is ever written at or past `buf_size`.
 * A guard region filled with 0xA5 detects the latter directly, so the test
 * fails on the actual overflow rather than on a symptom.
 *
 * Non-vacuity: ARENA is much larger than BUFSZ and truncation is forced
 * (TA_INTERNAL_APPEND_VACUOUS fires if the overflowing loop never saturates). */
#define APPEND_ARENA  256
#define APPEND_BUFSZ   16
#define APPEND_GUARD  0xA5

static int appendGuardIntact( const char *arena, int bufSize )
{
   int i;
   for( i = bufSize; i < APPEND_ARENA; i++ )
      if( (unsigned char)arena[i] != APPEND_GUARD )
         return 0;
   return 1;
}

static ErrorNumber testBoundedAppend( void )
{
   char arena[APPEND_ARENA];
   int pos, i, saturatedAt;

   /* 1. Plain accumulation stays exact while it fits. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = codegen_appendf( arena, APPEND_BUFSZ, 0, "ab" );
   pos = codegen_appendc( arena, APPEND_BUFSZ, pos, 'c' );
   pos = codegen_appendf( arena, APPEND_BUFSZ, pos, "%d", 42 );
   if( pos != 5 || strcmp( arena, "abc42" ) != 0 )
      return TA_INTERNAL_APPEND_FAIL_0;
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_1;

   /* 2. A single oversized append truncates, saturates and NUL-terminates. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = codegen_appendf( arena, APPEND_BUFSZ, 0, "%s",
                          "0123456789ABCDEF0123456789ABCDEF" );
   if( pos != APPEND_BUFSZ - 1 )
      return TA_INTERNAL_APPEND_FAIL_2;
   if( arena[APPEND_BUFSZ - 1] != '\0' || strlen( arena ) != APPEND_BUFSZ - 1u )
      return TA_INTERNAL_APPEND_FAIL_3;
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_4;

   /* 3. The overflow loop: keep appending well past capacity. This is the
    *    shape that used to walk off the end. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = 0;
   saturatedAt = -1;
   for( i = 0; i < 64; i++ )
   {
      pos = codegen_appendf( arena, APPEND_BUFSZ, pos, ",%d", i );
      pos = codegen_appendc( arena, APPEND_BUFSZ, pos, 'x' );
      if( pos < 0 || pos > APPEND_BUFSZ - 1 )
         return TA_INTERNAL_APPEND_FAIL_5;
      if( saturatedAt < 0 && pos == APPEND_BUFSZ - 1 )
         saturatedAt = i;
   }
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_6;
   if( saturatedAt < 0 )
      return TA_INTERNAL_APPEND_VACUOUS;   /* never truncated: test proved nothing */

   /* 4. Degenerate sizes must not write at all. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   if( codegen_appendf( arena, 0, 0, "x" ) != 0 ||
       codegen_appendc( arena, 0, 0, 'x' ) != 0 ||
       !appendGuardIntact( arena, 0 ) )
      return TA_INTERNAL_APPEND_FAIL_7;

   /* buf_size 1 holds only the terminator; both helpers must stay at 0. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   if( codegen_appendf( arena, 1, 0, "x" ) != 0 ||
       codegen_appendc( arena, 1, 0, 'x' ) != 0 ||
       !appendGuardIntact( arena, 1 ) )
      return TA_INTERNAL_APPEND_FAIL_8;

   return TA_TEST_PASS; /* Success. */
}

static ErrorNumber testCircularBuffer( void )
{
   TA_RetCode retCode;
   int i;
   int buffer[20];
   ErrorNumber retValue;

   /* Initialize the library. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* The following function is supose to fill
    * the buffer with the value 0 to 8 sequentialy,
    * if somehow it is not 0 to 8, there is a bug!
    */
   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 1, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (1+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_1;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 2, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (2+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_2;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 3, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (3+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_3;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 4, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (4+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_4;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 5, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (5+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_5;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 6, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (6+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_6;
      }
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS; /* Success. */
}

/* This function is suppose to fill the buffer
 * with values going from 0 to 'size'.
 * The filling is done using the CIRCBUF macros.
 */
static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer )
{
   CIRCBUF_PROLOG(MyBuf,int,4);
   int i, value;
   int outIdx;

   CIRCBUF_INIT(MyBuf,int,size);

   outIdx = 0;

   // 1st Loop: Fill MyBuf with initial values
   //           (must be done).
   value = 0;
   for( i=0; i < size; i++ )
   {
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 2nd Loop: Get and Add subsequent values
   //           in MyBuf (optional)
   for( i=0; i < 3; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 3rd Loop: Empty MyBuf (optional)
   for( i=0; i < size; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      CIRCBUF_NEXT(MyBuf);
   }

   CIRCBUF_DESTROY(MyBuf);

   return TA_SUCCESS;
}
