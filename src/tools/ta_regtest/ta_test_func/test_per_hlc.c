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
 *
 *     Test functions which have the following
 *     characterisic: 
 *      - have one output with the optional 
 *        parameter being a period.
 *      - the input is high,low and close.
 *     
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

#include "trionan.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum {
TA_CCI_TEST
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
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( TA_Libc *libHandle,
                            const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /****************/
   /*   CCI TEST  */
   /****************/
   { 1, TA_CCI_TEST, 0, 251,  5, TA_SUCCESS,  0, 18.857, 4,  252-4 },

   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  0,   87.927,  10,  252-10 }, /* First Value */
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  1,   180.005, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  2,  143.5190963, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  3,  -113.8669783, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  4,  -111.064497, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  5,  -26.77393309, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  6,  -70.77933765, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  7,  -83.15662884, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  8,  -41.14421073, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS,  9,  -49.63059589, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 10,  -86.45142995, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 11,  -105.6275799, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 12,  -157.698269, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 13,  -190.5251436, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 14,  -142.8364298, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 15,  -122.4448056, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 16,  -79.95100041, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 17,  22.03829204, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 18,  7.765575065, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 19,  32.38905945, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 20,  -0.005587727, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 21,  43.84607294, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 22,  40.35152301, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 23,  92.89237535, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 24,  113.4778681, 10,  252-10 },
   { 1, TA_CCI_TEST, 0, 251, 11, TA_SUCCESS, 252-11,  -169.65514, 10,  252-10 }, /* Last Value */

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_per_hlc( TA_Libc *libHandle, TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( libHandle, TA_FUNC_UNST_ALL, 0 );

   for( i=0; i < NB_TEST; i++ )
   {
      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "TA_MA Failed Bad Parameter for Test #%d (%d,%d)\n",
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( libHandle, history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "TA_MA Failed Test #%d (Code=%d)\n", i, retValue );
         return retValue;
      }
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( libHandle, TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return 0; 
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

   switch( testParam->test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( libHandle,
                        startIdx,
                        endIdx,
                        testParam->high,
                        testParam->low,
                        testParam->close,
                        testParam->test->optInTimePeriod_0,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_CCI_Lookback( testParam->test->optInTimePeriod_0 );
      break;
   default:
      retCode = TA_UNKNOWN_ERR;
   }

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
   setInputBuffer( 0, history->high,  history->nbBars );
   setInputBuffer( 1, history->low,   history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );
      
   /* Make a simple first call. */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( libHandle,
                         test->startIdx,
                         test->endIdx,
                         gBuffer[0].in,
                         gBuffer[1].in,
                         gBuffer[2].in,
                         test->optInTimePeriod_0,
                         &outBegIdx,
                         &outNbElement,
                         gBuffer[0].out0 );
      break;

   default:
      retCode = TA_UNKNOWN_ERR;
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( libHandle,
                         test->startIdx,
                         test->endIdx,
                         gBuffer[0].in,
                         gBuffer[1].in,
                         gBuffer[2].in,
                         test->optInTimePeriod_0,
                         &outBegIdx,
                         &outNbElement,
                         gBuffer[0].in );
      break;
   default:
      retCode = TA_UNKNOWN_ERR;
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   setInputBuffer( 0, history->high,  history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( libHandle,
                         test->startIdx,
                         test->endIdx,
                         gBuffer[0].in,
                         gBuffer[1].in,
                         gBuffer[2].in,
                         test->optInTimePeriod_0,
                         &outBegIdx,
                         &outNbElement,
                         gBuffer[1].in );
      break;
   default:
      retCode = TA_UNKNOWN_ERR;
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );
   setInputBuffer( 1, history->low,   history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( libHandle,
                         test->startIdx,
                         test->endIdx,
                         gBuffer[0].in,
                         gBuffer[1].in,
                         gBuffer[2].in,
                         test->optInTimePeriod_0,
                         &outBegIdx,
                         &outNbElement,
                         gBuffer[2].in );
      break;
   default:
      retCode = TA_UNKNOWN_ERR;
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].in, 0 );
   setInputBuffer( 2, history->close, history->nbBars );

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.high  = history->high;
   testParam.low   = history->low;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      errNb = doRangeTest( libHandle,
                           rangeTestFunction, 
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

