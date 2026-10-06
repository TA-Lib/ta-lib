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
 *  112400 MF     First version.
 *  122506 MF     Add tests for MININDEX,MAXINDEX,MINMAX and MINMAXINDEX.
 *  070226 MF,CC  Add TA_MIDPOINT tests: expected-value pins and a
 *                referenceMidpoint (the original brute rescan) compared
 *                against the cached-index implementation, like the
 *                existing MIN/MAX reference checks.
 *  100526 MF,CC  Hold what MININDEX, MAXINDEX and MINMAXINDEX write to the
 *                input they index (#501).
 */

/* Description:
 *     Test the min/max related functions.
 *
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum {
TA_MIN_TEST,
TA_MAX_TEST,
TA_MINMAX_TEST,
TA_MININDEX_TEST,
TA_MAXINDEX_TEST,
TA_MINMAXINDEX_TEST,
TA_MIDPOINT_TEST
} TA_TestId;

typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_TestId  theFunction;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInTimePeriod;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Real *input;
   unsigned int nbElement;
} TA_RefTest;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

static TA_RetCode referenceMin( TA_Integer    startIdx,
                                TA_Integer    endIdx,
                                const TA_Real inReal[],
                                TA_Integer    optInTimePeriod,
                                TA_Integer   *outBegIdx,
                                TA_Integer   *outNbElement,
                                TA_Real       outReal[] );

static TA_RetCode referenceMax( TA_Integer    startIdx,
                                TA_Integer    endIdx,
                                const TA_Real inReal[],
                                TA_Integer    optInTimePeriod,
                                TA_Integer   *outBegIdx,
                                TA_Integer   *outNbElement,
                                TA_Real       outReal[] );

static TA_RetCode referenceMidpoint( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     const TA_Real inReal[],
                                     TA_Integer    optInTimePeriod,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Real       outReal[] );

static ErrorNumber testCompareToReference( const TA_Real *input, int nbElement );

static ErrorNumber checkIndexOutput( int site, int which, const TA_Real *in,
                                     int startIdx, int endIdx, int period,
                                     int outBegIdx, int outNbElement,
                                     const TA_Integer *out, unsigned char *tied );
static ErrorNumber verifyIndexCall( int site, const TA_Real *in,
                                    int startIdx, int endIdx, int period );
static ErrorNumber testIndexSweep( const TA_Real *in, int nbElement, int exhaustive );
static ErrorNumber checkIndexFloors( void );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /**********************/
   /*      MIN TEST      */
   /**********************/

   /* No output value. */
   { 0, TA_MIN_TEST, 1, 1,  14, TA_SUCCESS, 0, 0, 0, 0},

   /* One value tests. */
   { 0, TA_MIN_TEST, 14,  14, 14, TA_SUCCESS, 0, 91.125,  14, 1},

   /* Index too low test. */
   { 0, TA_MIN_TEST, 0,  15, 14, TA_SUCCESS, 0, 91.125,     13, 3},
   { 0, TA_MIN_TEST, 1,  15, 14, TA_SUCCESS, 0, 91.125,     13, 3},
   { 0, TA_MIN_TEST, 2,  16, 14, TA_SUCCESS, 0, 91.125,     13, 4},
   { 0, TA_MIN_TEST, 2,  16, 14, TA_SUCCESS, 1, 91.125,     13, 4},
   { 0, TA_MIN_TEST, 2,  16, 14, TA_SUCCESS, 2, 91.125,     13, 4},
   { 0, TA_MIN_TEST, 0,  14, 14, TA_SUCCESS, 0, 91.125,     13, 2},
   { 0, TA_MIN_TEST, 0,  13, 14, TA_SUCCESS, 0, 91.125,     13, 1},

   /* Middle of data test. */
   { 0, TA_MIN_TEST, 20,  21, 14, TA_SUCCESS, 0, 89.345,   20, 2 },
   { 0, TA_MIN_TEST, 20,  21, 14, TA_SUCCESS, 1, 87.94,    20, 2 },

   /* Misc tests: 2 and 14 periods */
   { 1, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      0, 91.125,  13,  252-13 }, /* First Value */
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      1, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      2, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      3, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      4, 89.75,   13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS, 252-14, 107.75,  13,  252-13 },  /* Last Value */

   { 1, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      0, 91.5,  1,  252-1 }, /* First Value */
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      1, 91.5,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      2, 93.97,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      3, 93.97,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      4, 94.5,   1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS, 252-2, 109.19,  1,  252-1 },  /* Last Value */

   /**********************/
   /*      MAX TEST      */
   /**********************/

   /* One value tests. */
   { 0, TA_MAX_TEST, 14,  14, 14, TA_SUCCESS, 0, 98.815,  14, 1},

   /* Index too low test. */
   { 0, TA_MAX_TEST, 0,  15, 14, TA_SUCCESS, 0, 98.815,     13, 3},
   { 0, TA_MAX_TEST, 1,  15, 14, TA_SUCCESS, 0, 98.815,     13, 3},
   { 0, TA_MAX_TEST, 2,  16, 14, TA_SUCCESS, 0, 98.815,     13, 4},
   { 0, TA_MAX_TEST, 2,  16, 14, TA_SUCCESS, 1, 98.815,     13, 4},
   { 0, TA_MAX_TEST, 2,  16, 14, TA_SUCCESS, 2, 98.815,     13, 4},
   { 0, TA_MAX_TEST, 0,  14, 14, TA_SUCCESS, 0, 98.815,     13, 2},
   { 0, TA_MAX_TEST, 0,  13, 14, TA_SUCCESS, 0, 98.815,     13, 1},

   /* Middle of data test. */
   { 0, TA_MAX_TEST, 20,  21, 14, TA_SUCCESS,  0, 98.815,   20, 2  },
   { 0, TA_MAX_TEST, 20,  21, 14, TA_SUCCESS,  1, 98.815,   20, 2  },
   { 0, TA_MAX_TEST, 20,  99, 14, TA_SUCCESS,  6, 93.405,   20, 80 },
   { 0, TA_MAX_TEST, 20,  99, 14, TA_SUCCESS,  6, 93.405,   20, 80 },
   { 0, TA_MAX_TEST, 20,  99, 14, TA_SUCCESS, 13, 89.78,    20, 80 },

   /* Misc tests: 1, 2 and 14 periods */
   { 1, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      0, 98.815,  13,  252-13 }, /* First Value */
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      1, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      2, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      3, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      4, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS, 252-14, 110.69,  13,  252-13 },  /* Last Value */

   { 1, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      0, 92.5,  1,  252-1 }, /* First Value */
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      1, 95.155,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      2, 95.155, 1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      3, 95.5, 1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      4, 95.5,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      5, 95.0,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS, 252-2, 109.69, 1,  252-1 },  /* Last Value */

  /*************************************/
  /*  MINMAX and INDEX Functions tests */
  /*************************************/

   { 1, TA_MINMAX_TEST, 0, 251, 14, TA_SUCCESS, 0, 91.125,  13,  252-13 },
   { 1, TA_MINMAXINDEX_TEST, 0, 251, 14, TA_SUCCESS, 0, 0,  13,  252-13 },
   { 1, TA_MININDEX_TEST, 0, 251, 14, TA_SUCCESS, 0, 0,  13,  252-13 },
   { 1, TA_MAXINDEX_TEST, 0, 251, 14, TA_SUCCESS, 0, 0,  13,  252-13 },

   { 1, TA_MINMAXINDEX_TEST, 0, 251, 2, TA_SUCCESS, 0, 0,  1,  252-1 },
   { 1, TA_MININDEX_TEST, 0, 251, 2, TA_SUCCESS, 0, 0,  1,  252-1 },
   { 1, TA_MAXINDEX_TEST, 0, 251, 2, TA_SUCCESS, 0, 0,  1,  252-1 },

   { 0, TA_MINMAXINDEX_TEST, 20, 99, 14, TA_SUCCESS, 0, 0,  20,  80 },
   { 0, TA_MININDEX_TEST, 20, 99, 14, TA_SUCCESS, 0, 0,  20,  80 },
   { 0, TA_MAXINDEX_TEST, 20, 99, 14, TA_SUCCESS, 0, 0,  20,  80 },
   { 0, TA_MINMAXINDEX_TEST, 20, 99, 2, TA_SUCCESS, 0, 0,  20,  80 },
   { 0, TA_MININDEX_TEST, 20, 99, 2, TA_SUCCESS, 0, 0,  20,  80 },
   { 0, TA_MAXINDEX_TEST, 20, 99, 2, TA_SUCCESS, 0, 0,  20,  80 },

   /**********************/
   /*   MIDPOINT TEST    */
   /**********************/
   { 1, TA_MIDPOINT_TEST, 0, 251, 14, TA_SUCCESS,      0,  94.9700,  13,  252-13 }, /* First Value */
   { 0, TA_MIDPOINT_TEST, 0, 251, 14, TA_SUCCESS,      1,  94.9700,  13,  252-13 },
   { 0, TA_MIDPOINT_TEST, 0, 251, 14, TA_SUCCESS, 252-14, 109.2200,  13,  252-13 }, /* Last Value */

   { 1, TA_MIDPOINT_TEST, 0, 251,  2, TA_SUCCESS,      0,  92.0000,   1,  252-1 },  /* First Value */
   { 0, TA_MIDPOINT_TEST, 0, 251,  2, TA_SUCCESS,      1,  93.3275,   1,  252-1 },
   { 0, TA_MIDPOINT_TEST, 0, 251,  2, TA_SUCCESS,  252-2, 109.4400,   1,  252-1 },  /* Last Value */

   { 1, TA_MIDPOINT_TEST, 0, 251, 30, TA_SUCCESS,      0,  90.0325,  29,  252-29 }, /* First Value */
   { 0, TA_MIDPOINT_TEST, 0, 251, 30, TA_SUCCESS, 252-30, 107.2500,  29,  252-29 }  /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

static TA_Real testSerie1[]  = {9,8,7,6,5};
static TA_Real testSerie2[]  = {3,7,9,10,15,33,50};
static TA_Real testSerie3[]  = {0,0,0,1,2,0,0,0};
static TA_Real testSerie4[]  = {0,0,0,2,1,0,0,0};
static TA_Real testSerie5[]  = {2,0,0,0,0,0,0,0};
static TA_Real testSerie6[]  = {0,0,0,0,0,0,0,1};
static TA_Real testSerie7[]  = {-3,2};
static TA_Real testSerie8[]  = {2,-2};
static TA_Real testSerie9[]  = {4,2,3};
static TA_Real testSerie10[] = {3,3,-3,2,-1,0,2};

static TA_RefTest tableRefTest[] =
{
  {testSerie1, sizeof(testSerie1)/sizeof(TA_Real)},
  {testSerie2, sizeof(testSerie2)/sizeof(TA_Real)},
  {testSerie3, sizeof(testSerie3)/sizeof(TA_Real)},
  {testSerie4, sizeof(testSerie4)/sizeof(TA_Real)},
  {testSerie5, sizeof(testSerie5)/sizeof(TA_Real)},
  {testSerie6, sizeof(testSerie6)/sizeof(TA_Real)},
  {testSerie7, sizeof(testSerie7)/sizeof(TA_Real)},
  {testSerie8, sizeof(testSerie8)/sizeof(TA_Real)},
  {testSerie9, sizeof(testSerie9)/sizeof(TA_Real)},
  {testSerie10, sizeof(testSerie10)/sizeof(TA_Real)}
};

#define NB_TEST_REF (sizeof(tableRefTest)/sizeof(TA_RefTest))

/* What the index checks compared, per output and per place the call was made:
 * the table rows, the range test, and the sweep over the 252-bar and over the
 * short tied series. Each has its own floors in checkIndexFloors.
 */
enum { IDX_MININDEX, IDX_MAXINDEX, IDX_MINMAX_MIN, IDX_MINMAX_MAX, IDX_NB };
enum { IDX_AT_TABLE, IDX_AT_RANGE, IDX_AT_LONG, IDX_AT_SHORT, IDX_NB_SITE };
#define IDX_SMALLEST_PERIOD 2

typedef struct
{
   unsigned int window;       /* index held to its bar's window */
   unsigned int value;        /* input there held to the extremum */
   unsigned int tied;         /* ... in a window holding it twice */
   unsigned int pastLookback; /* ... from a call starting past the lookback */
   unsigned int smallest;     /* ... at the smallest period */
   unsigned int agree;        /* MINMAXINDEX held to the single-output function */
   unsigned int agreeTied;    /* ... in a window holding the extremum twice */
} TA_IdxCount;

static const char *idxName[IDX_NB] = { "MININDEX", "MAXINDEX",
                                       "MINMAXINDEX(min)", "MINMAXINDEX(max)" };
static const char *idxSite[IDX_NB_SITE] = { "table", "range test",
                                            "252-bar sweep", "tied-series sweep" };
static TA_IdxCount gIdx[IDX_NB_SITE][IDX_NB];

/**** Global functions definitions.   ****/
ErrorNumber test_func_minmax( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {
      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "%s Failed Bad Parameter for Test #%d (%d,%d)\n", __FILE__,
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "%s Failed Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   /* Do tests against a local reference which is the non-optimized implementation */
   for( i=0; i < NB_TEST_REF; i++ )
   {
      retValue = testCompareToReference( tableRefTest[i].input,
                                         tableRefTest[i].nbElement );
      if( retValue != 0 )
      {
         printf( "%s Failed Ref Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   retValue = testIndexSweep( history->close, (int)history->nbBars, 0 );
   if( retValue != TA_TEST_PASS )
   {
      printf( "%s Failed Index Test on close (Code=%d)\n", __FILE__, retValue );
      return retValue;
   }

   for( i=0; i < NB_TEST_REF; i++ )
   {
      retValue = testIndexSweep( tableRefTest[i].input,
                                 (int)tableRefTest[i].nbElement, 1 );
      if( retValue != TA_TEST_PASS )
      {
         printf( "%s Failed Index Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   return checkIndexFloors();
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     TA_Real      *outputBuffer,
                                     TA_Integer   *outputBufferInt,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Integer   *lookback,
                                     void         *opaqueData,
                                     unsigned int  outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;
   TA_Real *dummyBufferReal;
   TA_Real *out1Real;
   TA_Real *out2Real;

   TA_Integer *dummyBufferInt;
   TA_Integer *out1Int;
   TA_Integer *out2Int;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   dummyBufferReal = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Real));
   if( !dummyBufferReal )
     return TA_ALLOC_ERR;

   dummyBufferInt = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Integer));
   if( !dummyBufferInt )
   {
      TA_Free( dummyBufferReal );
      return TA_ALLOC_ERR;
   }

   switch( outputNb )
   {
   case 0:
      out1Real = outputBuffer;
      out2Real = dummyBufferReal;
      out1Int  = outputBufferInt;
      out2Int  = dummyBufferInt;
      break;
   case 1:
      out1Real = dummyBufferReal;
      out2Real = outputBuffer;
      out1Int  = dummyBufferInt;
      out2Int  = outputBufferInt;
      break;
   default:
      TA_Free( dummyBufferReal );
      return TA_BAD_PARAM;
   }

   switch( testParam->test->theFunction )
   {
   case TA_MIN_TEST:
      retCode = TA_MIN( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback  = TA_MIN_Lookback( testParam->test->optInTimePeriod );
      break;

   case TA_MAX_TEST:
      retCode = TA_MAX( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_MAX_Lookback( testParam->test->optInTimePeriod );
      break;

   case TA_MIDPOINT_TEST:
      retCode = TA_MIDPOINT( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_MIDPOINT_Lookback( testParam->test->optInTimePeriod );
      break;

   case TA_MINMAX_TEST:
      retCode = TA_MINMAX( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        out1Real, out2Real );
      *lookback = TA_MINMAX_Lookback( testParam->test->optInTimePeriod );
      break;

   case TA_MINMAXINDEX_TEST:
      retCode = TA_MINMAXINDEX( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        out1Int, out2Int );
      *lookback = TA_MINMAXINDEX_Lookback( testParam->test->optInTimePeriod );
      *isOutputInteger = 1;
      if( retCode == TA_SUCCESS &&
          ( checkIndexOutput( IDX_AT_RANGE, IDX_MINMAX_MIN, testParam->close,
                              startIdx, endIdx, testParam->test->optInTimePeriod,
                              *outBegIdx, *outNbElement, out1Int, NULL ) != TA_TEST_PASS ||
            checkIndexOutput( IDX_AT_RANGE, IDX_MINMAX_MAX, testParam->close,
                              startIdx, endIdx, testParam->test->optInTimePeriod,
                              *outBegIdx, *outNbElement, out2Int, NULL ) != TA_TEST_PASS ) )
         retCode = TA_INTERNAL_ERROR(130);
      break;

   case TA_MININDEX_TEST:
      retCode = TA_MININDEX( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        out1Int );
      *lookback = TA_MININDEX_Lookback( testParam->test->optInTimePeriod );
      *isOutputInteger = 1;
      if( retCode == TA_SUCCESS &&
          checkIndexOutput( IDX_AT_RANGE, IDX_MININDEX, testParam->close,
                            startIdx, endIdx, testParam->test->optInTimePeriod,
                            *outBegIdx, *outNbElement, out1Int, NULL ) != TA_TEST_PASS )
         retCode = TA_INTERNAL_ERROR(130);
      break;

   case TA_MAXINDEX_TEST:
      retCode = TA_MAXINDEX( startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod,
                        outBegIdx,
                        outNbElement,
                        out1Int );
      *lookback = TA_MAXINDEX_Lookback( testParam->test->optInTimePeriod );
      *isOutputInteger = 1;
      if( retCode == TA_SUCCESS &&
          checkIndexOutput( IDX_AT_RANGE, IDX_MAXINDEX, testParam->close,
                            startIdx, endIdx, testParam->test->optInTimePeriod,
                            *outBegIdx, *outNbElement, out1Int, NULL ) != TA_TEST_PASS )
         retCode = TA_INTERNAL_ERROR(130);
      break;

   default:
      retCode = TA_INTERNAL_ERROR(129);
      break;
   }

   TA_Free( dummyBufferReal );
   TA_Free( dummyBufferInt );

   return retCode;
}

static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;
   TA_Integer outInt0[MAX_NB_TEST_ELEMENT];
   TA_Integer outInt1[MAX_NB_TEST_ELEMENT];

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->open, history->nbBars );
   setInputBuffer( 1, history->open, history->nbBars );

   CLEAR_EXPECTED_VALUE(0);

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      errNb = doRangeTest( rangeTestFunction,
                           TA_TEST_UNST_NONE,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }


   /* Make a simple first call. */
   switch( test->theFunction )
   {
   case TA_MIN_TEST:
      retCode = TA_MIN( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
      break;
   case TA_MAX_TEST:
      retCode = TA_MAX( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
      break;
   case TA_MIDPOINT_TEST:
      retCode = TA_MIDPOINT( test->startIdx,
                             test->endIdx,
                             gBuffer[0].in,
                             test->optInTimePeriod,
                             &outBegIdx,
                             &outNbElement,
                             gBuffer[0].out0 );
      break;
   case TA_MINMAX_TEST:
      retCode = TA_MINMAX( test->startIdx,
                           test->endIdx,
                           gBuffer[0].in,
                           test->optInTimePeriod,
                           &outBegIdx,
                           &outNbElement,
                           gBuffer[0].out0,
                           gBuffer[0].out1 );
      break;
   case TA_MININDEX_TEST:
      retCode = TA_MININDEX( test->startIdx,
                             test->endIdx,
                             gBuffer[0].in,
                             test->optInTimePeriod,
                             &outBegIdx,
                             &outNbElement,
                             outInt0 );
      break;
   case TA_MAXINDEX_TEST:
      retCode = TA_MAXINDEX( test->startIdx,
                             test->endIdx,
                             gBuffer[0].in,
                             test->optInTimePeriod,
                             &outBegIdx,
                             &outNbElement,
                             outInt0 );
      break;
   case TA_MINMAXINDEX_TEST:
      retCode = TA_MINMAXINDEX( test->startIdx,
                                test->endIdx,
                                gBuffer[0].in,
                                test->optInTimePeriod,
                                &outBegIdx,
                                &outNbElement,
                                outInt0,
                                outInt1 );
      break;
   default:
      return TA_TEST_PASS;
   }

   errNb = checkDataSame( gBuffer[0].in, history->open,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   if( test->theFunction == TA_MININDEX_TEST ||
       test->theFunction == TA_MAXINDEX_TEST ||
       test->theFunction == TA_MINMAXINDEX_TEST )
   {
      if( retCode != test->expectedRetCode ||
          outBegIdx != test->expectedBegIdx ||
          outNbElement != test->expectedNbElement )
      {
         printf( "Failure: index test retCode=%d begIdx=%d nbElement=%d, expected %d %d %d\n",
                 (int)retCode, outBegIdx, outNbElement,
                 (int)test->expectedRetCode, test->expectedBegIdx,
                 test->expectedNbElement );
         return TA_REGTEST_INDEX_CALL;
      }

      if( test->theFunction == TA_MINMAXINDEX_TEST )
      {
         errNb = checkIndexOutput( IDX_AT_TABLE, IDX_MINMAX_MIN, gBuffer[0].in,
                                   test->startIdx, test->endIdx,
                                   test->optInTimePeriod,
                                   outBegIdx, outNbElement, outInt0, NULL );
         if( errNb == TA_TEST_PASS )
            errNb = checkIndexOutput( IDX_AT_TABLE, IDX_MINMAX_MAX, gBuffer[0].in,
                                      test->startIdx, test->endIdx,
                                      test->optInTimePeriod,
                                      outBegIdx, outNbElement, outInt1, NULL );
      }
      else
         errNb = checkIndexOutput( IDX_AT_TABLE,
                                   test->theFunction == TA_MININDEX_TEST ?
                                   IDX_MININDEX : IDX_MAXINDEX,
                                   gBuffer[0].in, test->startIdx, test->endIdx,
                                   test->optInTimePeriod,
                                   outBegIdx, outNbElement, outInt0, NULL );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   /* CHECK_EXPECTED_VALUE only applies to functions with real outputs. */
   if( test->theFunction == TA_MIN_TEST ||
       test->theFunction == TA_MAX_TEST ||
       test->theFunction == TA_MINMAX_TEST ||
       test->theFunction == TA_MIDPOINT_TEST )
   {
      CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );
   }

   if( server_verify_active() )
   {
      switch( test->theFunction )
      {
      case TA_MIN_TEST:
         errNb = server_verify("MIN", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_MAX_TEST:
         errNb = server_verify("MAX", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_MIDPOINT_TEST:
         errNb = server_verify("MIDPOINT", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_MINMAX_TEST:
         errNb = server_verify("MINMAX", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, gBuffer[0].out1, NULL }, NULL);
         break;
      case TA_MININDEX_TEST:
         errNb = server_verify("MININDEX", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               NULL, (const TA_Integer*[]){ outInt0, NULL });
         break;
      case TA_MAXINDEX_TEST:
         errNb = server_verify("MAXINDEX", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               NULL, (const TA_Integer*[]){ outInt0, NULL });
         break;
      case TA_MINMAXINDEX_TEST:
         errNb = server_verify("MINMAXINDEX", test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               (double[]){ (double)test->optInTimePeriod }, 1,
                               NULL, (const TA_Integer*[]){ outInt0, outInt1, NULL });
         break;
      default:
         errNb = TA_TEST_PASS;
         break;
      }
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer. (Only for MIN/MAX/MIDPOINT which have one real output.)
    */
   if( test->theFunction == TA_MIN_TEST || test->theFunction == TA_MAX_TEST ||
       test->theFunction == TA_MIDPOINT_TEST )
   {
      CLEAR_EXPECTED_VALUE(0);
      if( test->theFunction == TA_MIN_TEST )
      {
         retCode = TA_MIN( test->startIdx,
                           test->endIdx,
                           gBuffer[1].in,
                           test->optInTimePeriod,
                           &outBegIdx,
                           &outNbElement,
                           gBuffer[1].in );
      }
      else if( test->theFunction == TA_MIDPOINT_TEST )
      {
         retCode = TA_MIDPOINT( test->startIdx,
                                test->endIdx,
                                gBuffer[1].in,
                                test->optInTimePeriod,
                                &outBegIdx,
                                &outNbElement,
                                gBuffer[1].in );
      }
      else
      {
         retCode = TA_MAX( test->startIdx,
                           test->endIdx,
                           gBuffer[1].in,
                           test->optInTimePeriod,
                           &outBegIdx,
                           &outNbElement,
                           gBuffer[1].in );
      }

      /* The previous call should have the same output as this call.
       */
      errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
      if( errNb != TA_TEST_PASS )
         return errNb;

      CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}


/* These reference functions were the original non-optimized
 * version of TA_MIN and TA_MAX.
 *
 * TA-Lib might implement a faster algorithm, at the cost
 * of complexity. Consequently, it is important to verify the
 * equivalence between the optimize and non-optimized version.
 */
static TA_RetCode referenceMin( TA_Integer    startIdx,
                                TA_Integer    endIdx,
                                const TA_Real inReal[],
                                TA_Integer    optInTimePeriod,
                                TA_Integer   *outBegIdx,
                                TA_Integer   *outNbElement,
                                TA_Real       outReal[] )
{
   TA_Real lowest, tmp;
   TA_Integer outIdx, nbInitialElementNeeded;
   TA_Integer trailingIdx, today, i;

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   while( today <= endIdx )
   {
      lowest = inReal[trailingIdx++];
      for( i=trailingIdx; i <= today; i++ )
      {
         tmp = inReal[i];
         if( tmp < lowest) lowest= tmp;
      }

      outReal[outIdx++] = lowest;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNbElement = outIdx;

   return TA_SUCCESS;
}

static TA_RetCode referenceMax( TA_Integer    startIdx,
                                TA_Integer    endIdx,
                                const TA_Real inReal[],
                                TA_Integer    optInTimePeriod,
                                TA_Integer   *outBegIdx,
                                TA_Integer   *outNbElement,
                                TA_Real       outReal[] )
{
   /* Insert local variables here. */
   TA_Real highest, tmp;
   TA_Integer outIdx, nbInitialElementNeeded;
   TA_Integer trailingIdx, today, i;


#ifndef TA_FUNC_NO_RANGE_CHECK

   /* Validate the requested output range. */
   if( startIdx < 0 )
      return TA_OUT_OF_RANGE_START_INDEX;
   if( (endIdx < 0) || (endIdx < startIdx))
      return TA_OUT_OF_RANGE_END_INDEX;

   /* Validate the parameters. */
   if( !inReal ) return TA_BAD_PARAM;
   /* min/max are checked for optInTimePeriod. */
   if( optInTimePeriod == TA_INTEGER_DEFAULT )
      optInTimePeriod = 30;

   if( outReal == NULL )
      return TA_BAD_PARAM;

#endif /* TA_FUNC_NO_RANGE_CHECK */

   /* Insert TA function code here. */

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   while( today <= endIdx )
   {
      highest = inReal[trailingIdx++];
      for( i=trailingIdx; i <= today; i++ )
      {
         tmp = inReal[i];
         if( tmp > highest ) highest = tmp;
      }

      outReal[outIdx++] = highest;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNbElement = outIdx;

   return TA_SUCCESS;
}

/* The original brute-rescan TA_MIDPOINT, kept as the non-optimized
 * reference for the cached-extremum-index implementation.
 */
static TA_RetCode referenceMidpoint( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     const TA_Real inReal[],
                                     TA_Integer    optInTimePeriod,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Real       outReal[] )
{
   TA_Real lowest, highest, tmp;
   TA_Integer outIdx, nbInitialElementNeeded;
   TA_Integer trailingIdx, today, i;

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   while( today <= endIdx )
   {
      lowest  = inReal[trailingIdx++];
      highest = lowest;
      for( i=trailingIdx; i <= today; i++ )
      {
         tmp = inReal[i];
         if( tmp < lowest ) lowest = tmp;
         else if( tmp > highest ) highest = tmp;
      }

      outReal[outIdx++] = (highest+lowest)/2.0;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNbElement = outIdx;

   return TA_SUCCESS;
}

static ErrorNumber testCompareToReference( const TA_Real *input, int nbElement )
{
   TA_Integer outBegIdx, outNbElement;
   TA_RetCode retCode;

   TA_Integer outBegIdxRef, outNbElementRef;
   TA_RetCode retCodeRef;

   int period, startIdx, endIdx, testNb;

   ErrorNumber errNb;

   outBegIdxRef = outNbElementRef = -1;

   /* Do a systematic tests, even for failure cases. */
   for( testNb=0; testNb <= 2; testNb++ ) /* 0=TA_MIN, 1=TA_MAX, 2=TA_MIDPOINT */
   {
      for( period=2; period <= nbElement; period++ )
      {
         for( startIdx=0; startIdx < nbElement; startIdx++ )
         {
            for( endIdx=0; (endIdx < nbElement) && (startIdx <= endIdx); endIdx++ )
            {
               /* Set to NAN all the elements of the gBuffers.
                * Note: These buffer are used as an attempt to detect
                *       out-of-bound writing in the output.
                */
               clearAllBuffers();

               /* Build the input. */
               setInputBuffer( 0, input, nbElement );

               /* Get the reference output. */
               if( testNb == 0 )
                  retCodeRef = referenceMin( startIdx, endIdx, input, period,
                                             &outBegIdxRef, &outNbElementRef, gBuffer[0].out0 );
               else if( testNb == 1 )
                  retCodeRef = referenceMax( startIdx, endIdx, input, period,
                                             &outBegIdxRef, &outNbElementRef, gBuffer[0].out0 );
               else
                  retCodeRef = referenceMidpoint( startIdx, endIdx, input, period,
                                                  &outBegIdxRef, &outNbElementRef, gBuffer[0].out0 );

               /* Verify that the input was preserved */
               errNb = checkDataSame( gBuffer[0].in, input, nbElement );
               if( errNb != TA_TEST_PASS )
                  return errNb;

               /* Get the TA-Lib implementation output. */
               if( testNb == 0 )
                  retCode = TA_MIN( startIdx, endIdx, input, period,
                                    &outBegIdx, &outNbElement, gBuffer[1].out0 );
               else if( testNb == 1 )
                  retCode = TA_MAX( startIdx, endIdx, input, period,
                                    &outBegIdx, &outNbElement, gBuffer[1].out0 );
               else
                  retCode = TA_MIDPOINT( startIdx, endIdx, input, period,
                                         &outBegIdx, &outNbElement, gBuffer[1].out0 );

               /* Verify that the input was preserved */
               errNb = checkDataSame( gBuffer[0].in, input, nbElement );
               if( errNb != TA_TEST_PASS )
                  return errNb;

               /* The reference and TA-LIB should have the same output. */
               if( retCode != retCodeRef )
               {
                  printf( "Failure: retCode != retCodeRef\n" );
                  return TA_REGTEST_OPTIMIZATION_REF_ERROR;
               }

               if( outBegIdx != outBegIdxRef )
               {
                  printf( "Failure: outBegIdx != outBegIdxRef\n" );
                  return TA_REGTEST_OPTIMIZATION_REF_ERROR;
               }

               if( outNbElement != outNbElementRef )
               {
                  printf( "Failure: outNbElement != outNbElementRef\n" );
                  return TA_REGTEST_OPTIMIZATION_REF_ERROR;
               }

               /* Two implementations of one function: compared at a tolerance,
                * not bit-for-bit.
                */
               errNb = checkSameContentApprox( gBuffer[0].out0, gBuffer[1].out0 );
               if( errNb != TA_TEST_PASS )
                  return errNb;

               if( retCode == TA_SUCCESS )
               {
                  /* Make another test using the same input/output buffer.
                   * The output should still be the same.
                   */
                  if( testNb == 0 )
                     retCode = TA_MIN( startIdx, endIdx, gBuffer[0].in, period,
                                       &outBegIdx, &outNbElement, gBuffer[0].in );
                  else if( testNb == 1 )
                     retCode = TA_MAX( startIdx, endIdx, gBuffer[0].in, period,
                                       &outBegIdx, &outNbElement, gBuffer[0].in );
                  else
                     retCode = TA_MIDPOINT( startIdx, endIdx, gBuffer[0].in, period,
                                            &outBegIdx, &outNbElement, gBuffer[0].in );

                  /* The reference and TA-LIB should have the same output. */
                  if( retCode != retCodeRef )
                  {
                     printf( "Failure: retCode != retCodeRef (2)\n" );
                     return TA_REGTEST_OPTIMIZATION_REF_ERROR;
                  }

                  if( outBegIdx != outBegIdxRef )
                  {
                     printf( "Failure: outBegIdx != outBegIdxRef (2)\n" );
                     return TA_REGTEST_OPTIMIZATION_REF_ERROR;
                  }

                  if( outNbElement != outNbElementRef )
                  {
                     printf( "Failure: outNbElement != outNbElementRef (2)\n" );
                     return TA_REGTEST_OPTIMIZATION_REF_ERROR;
                  }

                  /* Two implementations of one function: compared at a tolerance,
                   * not bit-for-bit.
                   */
                  errNb = checkSameContentApprox( gBuffer[0].out0, gBuffer[0].in );
                  if( errNb != TA_TEST_PASS )
                     return errNb;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* Holds one index output to the input it was computed from: the index lies in
 * its bar's window, counted in `in` and not from startIdx, and the input there
 * is the most recent of the window's bars holding its extremum (rW4). `tied`,
 * when given, receives which bars had a window holding it more than once.
 */
static ErrorNumber checkIndexOutput( int site, int which, const TA_Real *in,
                                     int startIdx, int endIdx, int period,
                                     int outBegIdx, int outNbElement,
                                     const TA_Integer *out, unsigned char *tied )
{
   int j, k, bar, first, idx, nbExtremum, newest;
   int isMax = (which == IDX_MAXINDEX) || (which == IDX_MINMAX_MAX);
   TA_IdxCount *count = &gIdx[site][which];
   TA_Real extremum;

   if( outNbElement > 0 &&
       ( outBegIdx < period-1 || outBegIdx < startIdx ||
         outBegIdx + outNbElement - 1 > endIdx ) )
   {
      printf( "Failure: %s period=%d [%d,%d] reports begIdx=%d nbElement=%d\n",
              idxName[which], period, startIdx, endIdx, outBegIdx, outNbElement );
      return TA_REGTEST_INDEX_CALL;
   }

   for( j=0; j < outNbElement; j++ )
   {
      bar   = outBegIdx + j;
      first = bar - period + 1;
      idx   = out[j];

      if( idx < first || idx > bar )
      {
         printf( "Failure: %s period=%d startIdx=%d bar=%d names %d, outside [%d,%d]\n",
                 idxName[which], period, startIdx, bar, idx, first, bar );
         return TA_REGTEST_INDEX_OUTSIDE_WINDOW;
      }
      count->window++;

      extremum   = in[first];
      nbExtremum = 1;
      newest     = first;
      for( k=first+1; k <= bar; k++ )
      {
         if( in[k] == extremum )
         {
            nbExtremum++;
            newest = k;
         }
         else if( isMax ? (in[k] > extremum) : (in[k] < extremum) )
         {
            extremum   = in[k];
            nbExtremum = 1;
            newest     = k;
         }
      }

      if( !(in[idx] == extremum) )
      {
         printf( "Failure: %s period=%d startIdx=%d bar=%d names %d (%.17g), window extremum is %.17g\n",
                 idxName[which], period, startIdx, bar, idx, in[idx], extremum );
         return TA_REGTEST_INDEX_NOT_EXTREMUM;
      }
      count->value++;
      if( idx != newest )
      {
         printf( "Failure: %s period=%d startIdx=%d bar=%d names %d, the most recent bar holding %.17g is %d\n",
                 idxName[which], period, startIdx, bar, idx, extremum, newest );
         return TA_REGTEST_INDEX_NOT_NEWEST;
      }
      if( nbExtremum > 1 )                count->tied++;
      if( startIdx > period-1 )           count->pastLookback++;
      if( period == IDX_SMALLEST_PERIOD ) count->smallest++;
      if( tied )
         tied[j] = (unsigned char)(nbExtremum > 1);
   }

   return TA_TEST_PASS;
}

/* One range and period through all three functions: each output against the
 * input, then MINMAXINDEX against the other two.
 */
static ErrorNumber verifyIndexCall( int site, const TA_Real *in,
                                    int startIdx, int endIdx, int period )
{
   static TA_Integer out[IDX_NB][MAX_NB_TEST_ELEMENT];
   static unsigned char tied[IDX_NB][MAX_NB_TEST_ELEMENT];
   TA_Integer begIdx[3], nbElement[3];
   TA_Integer expectedBeg, expectedNb;
   TA_RetCode retCode[3];
   ErrorNumber errNb;
   int j, which;

   for( which=0; which < IDX_NB; which++ )
      for( j=0; j < MAX_NB_TEST_ELEMENT; j++ )
         out[which][j] = -1;
   for( j=0; j < 3; j++ )
      begIdx[j] = nbElement[j] = -1;

   expectedBeg = (startIdx > period-1) ? startIdx : period-1;
   expectedNb  = endIdx - expectedBeg + 1;
   if( expectedNb <= 0 )
      expectedBeg = expectedNb = 0;

   retCode[0] = TA_MININDEX( startIdx, endIdx, in, period,
                             &begIdx[0], &nbElement[0], out[IDX_MININDEX] );
   retCode[1] = TA_MAXINDEX( startIdx, endIdx, in, period,
                             &begIdx[1], &nbElement[1], out[IDX_MAXINDEX] );
   retCode[2] = TA_MINMAXINDEX( startIdx, endIdx, in, period,
                                &begIdx[2], &nbElement[2],
                                out[IDX_MINMAX_MIN], out[IDX_MINMAX_MAX] );

   for( j=0; j < 3; j++ )
   {
      if( retCode[j] != TA_SUCCESS ||
          begIdx[j] != expectedBeg || nbElement[j] != expectedNb )
      {
         printf( "Failure: index call %d period=%d [%d,%d]: retCode=%d begIdx=%d nbElement=%d\n",
                 j, period, startIdx, endIdx, (int)retCode[j], begIdx[j], nbElement[j] );
         return TA_REGTEST_INDEX_CALL;
      }
   }

   for( which=0; which < IDX_NB; which++ )
   {
      errNb = checkIndexOutput( site, which, in, startIdx, endIdx, period,
                                expectedBeg, expectedNb, out[which], tied[which] );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   for( which=IDX_MINMAX_MIN; which <= IDX_MINMAX_MAX; which++ )
   {
      int single = which - IDX_MINMAX_MIN;
      for( j=0; j < expectedNb; j++ )
      {
         if( out[which][j] != out[single][j] )
         {
            printf( "Failure: %s period=%d startIdx=%d bar=%d names %d, %s names %d\n",
                    idxName[which], period, startIdx, expectedBeg+j,
                    out[which][j], idxName[single], out[single][j] );
            return TA_REGTEST_INDEX_DISAGREE;
         }
         gIdx[site][which].agree++;
         if( tied[which][j] )
            gIdx[site][which].agreeTied++;
      }
   }

   return TA_TEST_PASS;
}

/* Every range of a short series at every period it admits, or a long one at
 * the periods and ranges that move the origin: startIdx before, at and past
 * the lookback, where an index counted from startIdx or from outBegIdx leaves
 * the window.
 */
static ErrorNumber testIndexSweep( const TA_Real *in, int nbElement, int exhaustive )
{
   static const int periods[] = { IDX_SMALLEST_PERIOD, 3, 14, 30 };
   int starts[7], ends[3];
   int p, s, e, k, period, startIdx, endIdx;
   ErrorNumber errNb;

   if( nbElement > MAX_NB_TEST_ELEMENT )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   if( exhaustive )
   {
      for( period=IDX_SMALLEST_PERIOD; period <= nbElement; period++ )
         for( startIdx=0; startIdx < nbElement; startIdx++ )
            for( endIdx=startIdx; endIdx < nbElement; endIdx++ )
            {
               errNb = verifyIndexCall( IDX_AT_SHORT, in, startIdx, endIdx, period );
               if( errNb != TA_TEST_PASS )
                  return errNb;
            }
      return TA_TEST_PASS;
   }

   for( p=0; p < (int)(sizeof(periods)/sizeof(periods[0])); p++ )
   {
      period = periods[p];
      if( period > nbElement )
         continue;

      starts[0] = 0;
      starts[1] = 1;
      starts[2] = period-2;
      starts[3] = period-1;
      starts[4] = period;
      starts[5] = nbElement/2;
      starts[6] = nbElement-1;

      for( s=0; s < 7; s++ )
      {
         startIdx = starts[s];
         for( k=0; k < s && starts[k] != startIdx; k++ ) {}
         if( k < s || startIdx < 0 || startIdx >= nbElement )
            continue;

         ends[0] = startIdx;
         ends[1] = startIdx+1;
         ends[2] = nbElement-1;
         for( e=0; e < 3; e++ )
         {
            endIdx = ends[e];
            for( k=0; k < e && ends[k] != endIdx; k++ ) {}
            if( k < e || endIdx >= nbElement )
               continue;
            errNb = verifyIndexCall( IDX_AT_LONG, in, startIdx, endIdx, period );
            if( errNb != TA_TEST_PASS )
               return errNb;
         }
      }
   }

   return TA_TEST_PASS;
}

/* Each place is held to what it can reach: the table calls no second function,
 * and only the short series are built to tie.
 */
static ErrorNumber checkIndexFloors( void )
{
   int site, which;
   const TA_IdxCount *count;

   for( site=0; site < IDX_NB_SITE; site++ )
   {
      for( which=0; which < IDX_NB; which++ )
      {
         int isSweep  = (site == IDX_AT_LONG) || (site == IDX_AT_SHORT);
         int hasAgree = isSweep && (which >= IDX_MINMAX_MIN);
         count = &gIdx[site][which];

         if( !count->window || !count->value || !count->pastLookback ||
             !count->smallest ||
             ( site == IDX_AT_SHORT && !count->tied ) ||
             ( hasAgree && !count->agree ) ||
             ( hasAgree && site == IDX_AT_SHORT && !count->agreeTied ) )
         {
            printf( "Failure: %s in the %s compared window=%u value=%u tied=%u pastLookback=%u smallestPeriod=%u agree=%u agreeTied=%u\n",
                    idxName[which], idxSite[site],
                    count->window, count->value, count->tied,
                    count->pastLookback, count->smallest,
                    count->agree, count->agreeTied );
            return TA_REGTEST_INDEX_VACUOUS;
         }
      }
   }

   return TA_TEST_PASS;
}
