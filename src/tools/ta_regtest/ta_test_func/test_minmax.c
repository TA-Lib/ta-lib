/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
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
 *  112400 MF   First version.
 *
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

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum {
TA_MIN_TEST,
TA_MAX_TEST
} TA_TestId;

typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_TestId  theFunction;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInTimePeriod_0;
 
   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( TA_Libc *libHandle,
                            const TA_History *history,
                            const TA_Test *test );

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

   /* Misc tests: 1, 2 and 14 periods */
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      0, 91.125,  13,  252-13 }, /* First Value */
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      1, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      2, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      3, 91.125,  13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS,      4, 89.75,   13,  252-13 },
   { 0, TA_MIN_TEST, 0, 251, 14, TA_SUCCESS, 252-14, 107.75,  13,  252-13 },  /* Last Value */

   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      0, 91.5,  1,  252-1 }, /* First Value */
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      1, 91.5,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      2, 93.97,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      3, 93.97,  1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS,      4, 94.5,   1,  252-1 },
   { 0, TA_MIN_TEST, 0, 251, 2, TA_SUCCESS, 252-2, 109.19,  1,  252-1 },  /* Last Value */

   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,      0, 92.5,  0,  252 }, /* First Value */
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,      1, 91.5,  0,  252 },
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,      2, 95.155,  0,  252 },
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,      3, 93.97,  0,  252 },
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,      4, 95.5,   0,  252 },
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,  252-2, 109.69,  0,  252 },
   { 0, TA_MIN_TEST, 0, 251, 1, TA_SUCCESS,  252-1, 109.19,  0,  252 },  /* Last Value */

   /**********************/
   /*      MAX TEST      */
   /**********************/

   /* No output value. */
   { 0, TA_MAX_TEST, 1, 1,  14, TA_SUCCESS, 0, 0, 0, 0},

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
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      0, 98.815,  13,  252-13 }, /* First Value */
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      1, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      2, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      3, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS,      4, 98.815,  13,  252-13 },
   { 0, TA_MAX_TEST, 0, 251, 14, TA_SUCCESS, 252-14, 110.69,  13,  252-13 },  /* Last Value */

   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      0, 92.5,  1,  252-1 }, /* First Value */
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      1, 95.155,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      2, 95.155, 1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      3, 95.5, 1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      4, 95.5,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS,      5, 95.0,  1,  252-1 },
   { 0, TA_MAX_TEST, 0, 251, 2, TA_SUCCESS, 252-2, 109.69, 1,  252-1 },  /* Last Value */

   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,      0, 92.5,   0,  252 }, /* First Value */
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,      1, 91.5,   0,  252 },
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,      2, 95.155, 0,  252 },
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,      3, 93.97,  0,  252 },
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,      4, 95.5,   0,  252 },
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,  252-2, 109.69,  0,  252 },
   { 0, TA_MAX_TEST, 0, 251, 1, TA_SUCCESS,  252-1, 109.19,  0,  252 },  /* Last Value */

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_minmax( TA_Libc *libHandle, TA_History *history )
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

      retValue = do_test( libHandle, history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "%s Failed Test #%d (Code=%d)\n", __FILE__,
                 i, retValue );
         return retValue;
      }
   }

   /* All test succeed. */
   return TA_TEST_PASS; 
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( TA_Libc *libHandle, 
                              TA_Integer startIdx,
                              TA_Integer endIdx,
                              TA_Real *outputBuffer,
                              TA_Integer *outBegIdx,
                              TA_Integer *outNbElement,
                              TA_Integer *lookback,
                              void *opaqueData,
                              unsigned int outputNb )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;
  
   (void)outputNb;

   testParam = (TA_RangeTestParam *)opaqueData;   

   if( testParam->test->theFunction == TA_MIN_TEST )
   {
      retCode = TA_MIN( libHandle,
                        startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod_0,                        
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback  = TA_MIN_Lookback( testParam->test->optInTimePeriod_0 );
   }
   else if( testParam->test->theFunction == TA_MAX_TEST )
   {
      retCode = TA_MAX( libHandle,
                        startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod_0,
                        outBegIdx,
                        outNbElement,                        
                        outputBuffer );
      *lookback = TA_MAX_Lookback( testParam->test->optInTimePeriod_0 );
   }
   else
      retCode = TA_UNKNOWN_ERR;

   return retCode;
}

static ErrorNumber do_test( TA_Libc *libHandle,
                            const TA_History *history,
                            const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->open, history->nbBars );
   setInputBuffer( 1, history->open, history->nbBars );

   CLEAR_EXPECTED_VALUE(0);

   /* Make a simple first call. */
   if( test->theFunction == TA_MIN_TEST )
   {
      retCode = TA_MIN( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }
   else if( test->theFunction == TA_MAX_TEST )
   {
      retCode = TA_MAX( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }

   errNb = checkDataSame( gBuffer[0].in, history->open,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   CLEAR_EXPECTED_VALUE(0);
   if( test->theFunction == TA_MIN_TEST )
   {
      retCode = TA_MIN( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }
   else if( test->theFunction == TA_MAX_TEST )
   {
      retCode = TA_MAX( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod_0,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );

   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      errNb = doRangeTest( libHandle,
                           rangeTestFunction, 
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

