/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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

   TA_Integer    optInFastK_Period_0;
   TA_Integer    optInSlowK_Period_1;
   TA_Integer    optInSlowK_MAType_2;
   TA_Integer    optInSlowD_Period_3;
   TA_Integer    optInSlowD_MAType_4;

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
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

static TA_RetCode referenceStoch( TA_Integer    startIdx,
                                  TA_Integer    endIdx,
                                  const TA_Real inHigh_0[],
                                  const TA_Real inLow_0[],
                                  const TA_Real inClose_0[],
                                  TA_Integer    optInFastK_Period_0, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInSlowK_Period_1, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInSlowK_MAType_2,
                                  TA_Integer    optInSlowD_Period_3, /* From 1 to TA_INTEGER_MAX */
                                  TA_Integer    optInSlowD_MAType_4,
                                  TA_Integer   *outBegIdx,
                                  TA_Integer   *outNbElement,
                                  TA_Real       outSlowK_0[],
                                  TA_Real       outSlowD_1[] );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /**********************/
   /*      STOCH TEST    */
   /**********************/
   { 1, 0, 9, 9, 5, 3, TA_MAType_SMA, 4, TA_MAType_SMA, TA_SUCCESS,  9,  1,
                                                        0, 38.139,  
                                                        0, 36.725  }, /* First Value */


   { 0, 0, 0, 251, 5, 3, TA_MAType_SMA, 4, TA_MAType_SMA, TA_SUCCESS,  9,  252-9,
                                                          252-10, 30.194, 
                                                          252-10, 46.641,   }, /* Last Value */

   { 1, 0, 0, 251, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, TA_SUCCESS,  8,  252-8,
                                                          0, 24.0128,  
                                                          0, 36.254,   }, /* First Value */

   { 0, 0, 0, 251, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA, TA_SUCCESS,  8,  252-8,
                                                          252-9, 30.194, 
                                                          252-9, 43.69,   } /* Last Value */

    /* More test needed!!! */
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_stoch( TA_History *history )
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
static TA_RetCode rangeTestFunction( 
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


  dummyOutput = TA_Malloc( (endIdx-startIdx+1) * sizeof(TA_Real) );
                     
  if( outputNb == 0 )
  {
     retCode = TA_STOCH(
                         startIdx,
                         endIdx,
                         testParam->high,
                         testParam->low,
                         testParam->close,
                         testParam->test->optInFastK_Period_0,
                         testParam->test->optInSlowK_Period_1,
                         testParam->test->optInSlowK_MAType_2,
                         testParam->test->optInSlowD_Period_3,
                         testParam->test->optInSlowD_MAType_4,
                         outBegIdx, outNbElement,
                         outputBuffer,
                         dummyOutput );

   }
   else
   {
     retCode = TA_STOCH(
                         startIdx,
                         endIdx,
                         testParam->high,
                         testParam->low,
                         testParam->close,
                         testParam->test->optInFastK_Period_0,
                         testParam->test->optInSlowK_Period_1,
                         testParam->test->optInSlowK_MAType_2,
                         testParam->test->optInSlowD_Period_3,
                         testParam->test->optInSlowD_MAType_4,
                         outBegIdx, outNbElement,
                         dummyOutput, 
                         outputBuffer );
   }

   TA_Free(  dummyOutput );

   *lookback = TA_STOCH_Lookback( testParam->test->optInFastK_Period_0,
                         testParam->test->optInSlowK_Period_1,
                         testParam->test->optInSlowK_MAType_2,
                         testParam->test->optInSlowD_Period_3,
                         testParam->test->optInSlowD_MAType_4 );

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
   setInputBuffer( 0, history->high,  history->nbBars );
   setInputBuffer( 1, history->low,   history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );
   
   /* Set the unstable period requested for that test. */
   switch( test->optInSlowK_MAType_2 )
   {
   case TA_MAType_EMA:
      retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, test->unstablePeriod );
      if( retCode != TA_SUCCESS )
         return TA_TEST_TFRR_SETUNSTABLE_PERIOD_FAIL;
      break;
   default:
      /* No unstable period for other methods. */
      break;
   }

   /* Make a simple first call. */
   retCode = TA_STOCH( test->startIdx,
                       test->endIdx,
                       gBuffer[0].in,
                       gBuffer[1].in,
                       gBuffer[2].in,
                       test->optInFastK_Period_0,
                       test->optInSlowK_Period_1,
                       test->optInSlowK_MAType_2,
                       test->optInSlowD_Period_3,
                       test->optInSlowD_MAType_4,
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

   /* Compare to the non-optimized version */
   retCode = referenceStoch(
                       test->startIdx,
                       test->endIdx,
                       gBuffer[0].in,
                       gBuffer[1].in,
                       gBuffer[2].in,
                       test->optInFastK_Period_0,
                       test->optInSlowK_Period_1,
                       test->optInSlowK_MAType_2,
                       test->optInSlowD_Period_3,
                       test->optInSlowD_MAType_4,
                       &outBegIdx, &outNbElement,
                       gBuffer[1].out0, 
                       gBuffer[1].out1 );

   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].out0, 0 );
   CHECK_EXPECTED_VALUE( gBuffer[1].out1, 1 );

   /* The non-optimized reference shall be identical to the optimzied TA-Lib
    * implementation.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[1].out0, gBuffer[0].out0 );
   if( errNb != TA_TEST_PASS )
      return errNb;

   errNb = checkSameContent( gBuffer[1].out1, gBuffer[0].out1 );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = TA_STOCH(
                       test->startIdx,
                       test->endIdx,
                       gBuffer[0].in,
                       gBuffer[1].in,
                       gBuffer[2].in,
                       test->optInFastK_Period_0,
                       test->optInSlowK_Period_1,
                       test->optInSlowK_MAType_2,
                       test->optInSlowD_Period_3,
                       test->optInSlowD_MAType_4,
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
      errNb = doRangeTest(
                           rangeTestFunction, 
                           TA_FUNC_UNST_EMA,
                           (void *)&testParam, 2, 0 );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   /* Call a local non-optimized version of the function.
    * This way, we make sure that the currently speed optimized
    * version in TA-Lib is not broken.
    */

   return TA_TEST_PASS;
}


/* This is an un-optimized version of the STOCH function */
static TA_RetCode referenceStoch( TA_Integer    startIdx,
                     TA_Integer    endIdx,
                     const TA_Real inHigh_0[],
                     const TA_Real inLow_0[],
                     const TA_Real inClose_0[],
                     TA_Integer    optInFastK_Period_0, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInSlowK_Period_1, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInSlowK_MAType_2,
                     TA_Integer    optInSlowD_Period_3, /* From 1 to TA_INTEGER_MAX */
                     TA_Integer    optInSlowD_MAType_4,
                     TA_Integer   *outBegIdx,
                     TA_Integer   *outNbElement,
                     TA_Real       outSlowK_0[],
                     TA_Real       outSlowD_1[] )
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
   TA_RetCode retCode;
   TA_Real Lt, Ht, tmp, *tempBuffer;
   TA_Integer outIdx;
   TA_Integer lookbackTotal, lookbackK, lookbackKSlow, lookbackDSlow;
   TA_Integer trailingIdx, today, i, bufferIsAllocated;

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/

#ifndef TA_FUNC_NO_RANGE_CHECK

   /* Validate the requested output range. */
   if( startIdx < 0 )
      return TA_OUT_OF_RANGE_START_INDEX;
   if( (endIdx < 0) || (endIdx < startIdx))
      return TA_OUT_OF_RANGE_END_INDEX;

   /* Validate the parameters. */
   /* Verify required price component. */
   if(!inHigh_0||!inLow_0||!inClose_0)
      return TA_BAD_PARAM;

   /* min/max are checked for optInFastK_Period_0. */
   if( optInFastK_Period_0 == TA_INTEGER_DEFAULT )
      optInFastK_Period_0 = 5;
   else if( (optInFastK_Period_0 < 1) || (optInFastK_Period_0 > 2147483647) )
      return TA_BAD_PARAM;

   /* min/max are checked for optInSlowK_Period_1. */
   if( optInSlowK_Period_1 == TA_INTEGER_DEFAULT )
      optInSlowK_Period_1 = 3;
   else if( (optInSlowK_Period_1 < 1) || (optInSlowK_Period_1 > 2147483647) )
      return TA_BAD_PARAM;

   if( optInSlowK_MAType_2 == TA_INTEGER_DEFAULT )
      optInSlowK_MAType_2 = 0;
   else if( (optInSlowK_MAType_2 < 0) || (optInSlowK_MAType_2 > 4) )
      return TA_BAD_PARAM;

   /* min/max are checked for optInSlowD_Period_3. */
   if( optInSlowD_Period_3 == TA_INTEGER_DEFAULT )
      optInSlowD_Period_3 = 3;
   else if( (optInSlowD_Period_3 < 1) || (optInSlowD_Period_3 > 2147483647) )
      return TA_BAD_PARAM;

   if( optInSlowD_MAType_4 == TA_INTEGER_DEFAULT )
      optInSlowD_MAType_4 = 0;
   else if( (optInSlowD_MAType_4 < 0) || (optInSlowD_MAType_4 > 4) )
      return TA_BAD_PARAM;

   if( outSlowK_0 == NULL )
      return TA_BAD_PARAM;

   if( outSlowD_1 == NULL )
      return TA_BAD_PARAM;

#endif /* TA_FUNC_NO_RANGE_CHECK */

/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/

   /* Insert TA function code here. */

   /* Identify the lookback needed. */
   lookbackK      = optInFastK_Period_0-1;
   lookbackKSlow  = TA_MA_Lookback( optInSlowK_Period_1, optInSlowK_MAType_2 );
   lookbackDSlow  = TA_MA_Lookback( optInSlowD_Period_3, optInSlowD_MAType_4 );
   lookbackTotal  = lookbackK + lookbackDSlow + lookbackKSlow;

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      /* Succeed... but no data in the output. */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Do the K calculation:
    *
    *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
    *
    * Kt is today stochastic
    * Ct is today closing price.
    * Lt is the lowest price of the last K Period (including today)
    * Ht is the highest price of the last K Period (including today)
    */

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;

   /* Calculate just enough K for ending up with the caller 
    * requested range. (The range of k must consider all
    * the lookback involve with the smoothing).
    */
   trailingIdx = startIdx-lookbackTotal;
   today       = trailingIdx+lookbackK;

   /* Allocate a temporary buffer large enough to
    * store the K.
    *
    * If the output is the same as the input, great
    * we just save ourself one memory allocation.
    */
   bufferIsAllocated = 0;
   if( (outSlowK_0 == inHigh_0) || 
       (outSlowK_0 == inLow_0)  || 
       (outSlowK_0 == inClose_0) )
   {
      tempBuffer = outSlowK_0;
   }
   else if( (outSlowD_1 == inHigh_0) ||
            (outSlowD_1 == inLow_0)  ||
            (outSlowD_1 == inClose_0) )
   {
      tempBuffer = outSlowD_1;
   }
   else
   {
      bufferIsAllocated = 1;
      tempBuffer = TA_Malloc( (endIdx-today+1)*sizeof(TA_Real) );
   }

   /* Do the K calculation */
   while( today <= endIdx )
   {
      /* Find Lt and Ht for the requested K period. */
      Lt = inLow_0 [trailingIdx];
      Ht = inHigh_0[trailingIdx];
      trailingIdx++;
      for( i=trailingIdx; i <= today; i++ )
      {
         tmp = inLow_0[i];
         if( tmp < Lt ) Lt = tmp;
         tmp = inHigh_0[i];
         if( tmp > Ht ) Ht = tmp;
      }

      /* Calculate stochastic. */
      tmp = Ht-Lt;
      if( tmp > 0.0 )
        tempBuffer[outIdx++] = 100.0*((inClose_0[today]-Lt)/tmp);
      else
        tempBuffer[outIdx++] = 100.0;

      today++;
   }

   /* Un-smoothed K calculation completed. This K calculation is not returned
    * to the caller. It is always smoothed and then return.
    * Some documentation will refer to the smoothed version as being 
    * "K-Slow", but often this end up to be shorten to "K".
    */
   retCode = TA_MA( 0, outIdx-1,
                    tempBuffer, optInSlowK_Period_1,
                    optInSlowK_MAType_2,
                    outBegIdx, outNbElement, tempBuffer );


   if( (retCode != TA_SUCCESS) || (*outNbElement == 0) )
   {
      if( bufferIsAllocated )
        TA_Free(  tempBuffer ); 
      /* Something wrong happen? No further data? */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return retCode; 
   }

   /* Calculate the %D which is simply a moving average of
    * the already smoothed %K.
    */
   retCode = TA_MA( 0, (*outNbElement)-1,
                    tempBuffer, optInSlowD_Period_3,
                    optInSlowD_MAType_4,
                    outBegIdx, outNbElement, outSlowD_1 );

   /* Copy tempBuffer into the caller buffer. 
    * (Calculation could not be done directly in the
    *  caller buffer because more input data then the
    *  requested range was needed for doing %D).
    */
   memmove( outSlowK_0, &tempBuffer[lookbackDSlow], (*outNbElement) * sizeof(TA_Real) );

   /* Don't need K anymore, free it if it was allocated here. */
   if( bufferIsAllocated )
      TA_Free(  tempBuffer ); 

   if( retCode != TA_SUCCESS )
   {
      /* Something wrong happen while processing %D? */
      *outBegIdx    = 0;
      *outNbElement = 0;
      return retCode;
   }

   /* Note: Keep the outBegIdx relative to the
    *       caller input before returning.
    */
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}


