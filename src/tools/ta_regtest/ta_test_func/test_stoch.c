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
 *  122101 MF   First version.
 *
 */

/* Description:
 *     Test the Stochastic function.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_Integer doRangeTestFlag; /* One will do a call to doRangeTest */

   TA_Integer unstablePeriod;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer    optInKPeriod_0;
   TA_Integer    optInKSlowPeriod_1;
   TA_Integer    optInDPeriod_2;
   TA_Integer    optInMethod_3;
   /* TA_Integer    optInCompatibility_4;*/

   TA_RetCode expectedRetCode;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer oneOfTheExpectedOutRealIndex1;
   TA_Real    oneOfTheExpectedOutReal1;
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
   /**********************/
   /*      STOCH TEST    */
   /**********************/
   { 1, 0, 9, 9, 5, 3, 4, TA_STOCH_SIMPLE, TA_SUCCESS,  9,  1,
                                                        0, 38.139,  
                                                        0, 36.725  }, /* First Value */


   { 0, 0, 0, 251, 5, 3, 4, TA_STOCH_SIMPLE, TA_SUCCESS,  9,  252-9,
                                                          252-10, 30.194, 
                                                          252-10, 46.641,   }, /* Last Value */

   { 1, 0, 0, 251, 5, 3, 3, TA_STOCH_SIMPLE, TA_SUCCESS,  8,  252-8,
                                                          0, 24.0128,  
                                                          0, 36.254,   }, /* First Value */

   { 0, 0, 0, 251, 5, 3, 3, TA_STOCH_SIMPLE, TA_SUCCESS,  8,  252-8,
                                                          252-9, 30.194, 
                                                          252-9, 43.69,   } /* Last Value */

    /* More test needed!!! */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_stoch( TA_Libc *libHandle, TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( libHandle, TA_FUNC_UNST_ALL, 0 );

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

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( libHandle, TA_FUNC_UNST_ALL, 0 );

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
  TA_Real *dummyOutput;
  
  testParam = (TA_RangeTestParam *)opaqueData;   


  dummyOutput = TA_Malloc( libHandle, (endIdx-startIdx+1) * sizeof(TA_Real) );

  if( outputNb == 0 )
  {
     retCode = TA_STOCH( libHandle,
                         startIdx,
                         endIdx,
                         testParam->high,
                         testParam->low,
                         testParam->close,
                         testParam->test->optInKPeriod_0,
                         testParam->test->optInKSlowPeriod_1,
                         testParam->test->optInDPeriod_2,
                         testParam->test->optInMethod_3,                       
                         outBegIdx, outNbElement,
                         outputBuffer,
                         dummyOutput );

   }
   else
   {
     retCode = TA_STOCH( libHandle,
                         startIdx,
                         endIdx,
                         testParam->high,
                         testParam->low,
                         testParam->close,
                         testParam->test->optInKPeriod_0,
                         testParam->test->optInKSlowPeriod_1,
                         testParam->test->optInDPeriod_2,
                         testParam->test->optInMethod_3,                                                
                         outBegIdx, outNbElement,
                         dummyOutput, 
                         outputBuffer );
   }

   TA_Free( libHandle, dummyOutput );

   *lookback = TA_STOCH_Lookback( testParam->test->optInKPeriod_0,
                         testParam->test->optInKSlowPeriod_1,
                         testParam->test->optInDPeriod_2,
                         testParam->test->optInMethod_3 );

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
   
   /* Set the unstable period requested for that test. */
   switch( test->optInMethod_3 )
   {
   case TA_STOCH_EXPONENTIAL:
      retCode = TA_SetUnstablePeriod( libHandle, TA_FUNC_UNST_EMA, test->unstablePeriod );
      if( retCode != TA_SUCCESS )
         return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;
      break;
   default:
      /* No unstable period for other methods. */
      break;
   }

   /* Make a simple first call. */
   retCode = TA_STOCH( libHandle,
                       test->startIdx,
                       test->endIdx,
                       gBuffer[0].in,
                       gBuffer[1].in,
                       gBuffer[2].in,
                       test->optInKPeriod_0,
                       test->optInKSlowPeriod_1,
                       test->optInDPeriod_2,
                       test->optInMethod_3,
                       &outBegIdx, &outNbElement,
                       gBuffer[0].out0, 
                       gBuffer[0].out1 );
                       
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
   CHECK_EXPECTED_VALUE( gBuffer[0].out1, 1 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_STOCH( libHandle,
                       test->startIdx,
                       test->endIdx,
                       gBuffer[0].in,
                       gBuffer[1].in,
                       gBuffer[2].in,
                       test->optInKPeriod_0,
                       test->optInKSlowPeriod_1,
                       test->optInDPeriod_2,
                       test->optInMethod_3,
                       &outBegIdx, &outNbElement,
                       gBuffer[0].in, 
                       gBuffer[1].in );

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkSameContent( gBuffer[0].out1, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[1].in, 1 );

   if( errNb != TA_TEST_PASS )
      return errNb;

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
                           TA_FUNC_UNST_EMA,
                           (void *)&testParam, 2, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

