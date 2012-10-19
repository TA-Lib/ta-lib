/* TA-LIB Copyright (c) 1999-2008, Mario Fortier
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
 *  AB       Anatoliy Belsky
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 AB   First version.
 *
 */

/* Description:
 *     Test AVGDEV function.
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
typedef struct
{
   TA_Integer doRangeTestFlag; /* One will do a call to doRangeTest */

   TA_Integer unstablePeriod;

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
   const TA_Test *test;
   const TA_Real *close;
   const TA_Real *open;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /*************************/
   /*      AVGDEV TEST      */
   /*************************/
   { 1, 0, 0, 251, 5, TA_SUCCESS,     0, 55.9194,   4,  252-4 }, /* First Value */
   { 0, 0, 0, 251, 5, TA_SUCCESS,     1, 64.6143,   4,  252-4 }, 
   { 0, 0, 0, 251, 5, TA_SUCCESS, 252-5, 7.730673,  4,  252-4 }, /* Last Value */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_imi( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

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

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return TA_TEST_PASS; 
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

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;
  
   testParam = (TA_RangeTestParam *)opaqueData;   
   
   retCode = TA_IMI(
                        startIdx,
                        endIdx,
						testParam->open,
                        testParam->close,
                        testParam->test->optInTimePeriod,                   
                        outBegIdx,
                        outNbElement,
                        outputBuffer );


   *lookback = TA_IMI_Lookback( testParam->test->optInTimePeriod );

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

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->open, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );
   setInputBuffer( 2, history->open, history->nbBars );

    retCode = TA_SetUnstablePeriod(
                                    TA_FUNC_UNST_IMI,
                                    test->unstablePeriod );
    if( retCode != TA_SUCCESS )
        return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;

   /* Make a simple first call. */
   retCode = TA_IMI(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
						gBuffer[1].in,
                        test->optInTimePeriod,                     
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );

   /* Verify that the inputs were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->open,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkDataSame( gBuffer[1].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_IMI(
                        test->startIdx,
                        test->endIdx,
                        gBuffer[2].in,
						gBuffer[1].in,
                        test->optInTimePeriod,                     
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[2].in );

   /* Verify that the inputs were preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->close, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].in, 0 );

   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.close = history->close;
   testParam.open = history->open;
// XXX FAIL HERE
   if( test->doRangeTestFlag )
   {
      errNb = doRangeTest(
                           rangeTestFunction, 
                           TA_FUNC_UNST_IMI,
                           (void *)&testParam, 1, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

