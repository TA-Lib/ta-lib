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
 *     Test the simple momentum functions (MOM, ROC, ROCR).
 *
 *     TA-Lib:
 *       MOM  = price - prevPrice           [Momentum]
 *       ROC  = ((price/prevPrice)-1)*100   [Rate of change]
 *       ROCR = (price/prevPrice)*100       [Rate of change ratio]
 *
 *     Tradestation       Metastock         TA-Lib
 *     ROC                ROC (Percent)     ROC
 *     MOM                ROC (Point)       MOM
 *     -                  MO                ROCR (R for ratio)
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
TA_MOM_TEST,
TA_ROC_TEST,
TA_ROCR_TEST } TA_TestId;

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
   /*      MOM TEST      */
   /**********************/
   { 1, TA_MOM_TEST, 0, 251, 14, TA_SUCCESS,      0, -0.50,  14,  252-14 }, /* First Value */
   { 0, TA_MOM_TEST, 0, 251, 14, TA_SUCCESS,      1, -2.00,  14,  252-14 },
   { 0, TA_MOM_TEST, 0, 251, 14, TA_SUCCESS,      2, -5.22,  14,  252-14 },
   { 0, TA_MOM_TEST, 0, 251, 14, TA_SUCCESS, 252-15, -1.13,  14,  252-14 },  /* Last Value */

   /* Test out of range. */
   { 0, TA_MOM_TEST, -1, 3, 14, TA_OUT_OF_RANGE_START_INDEX, 0, 0, 0, 0},
   { 0, TA_MOM_TEST,  3, -1, 14, TA_OUT_OF_RANGE_END_INDEX,   0, 0, 0, 0},
   { 0, TA_MOM_TEST,  4, 3, 14, TA_OUT_OF_RANGE_END_INDEX,   0, 0, 0, 0},

   /* No output value. */
   { 0, TA_MOM_TEST, 1, 1,  14, TA_SUCCESS, 0, 0, 0, 0},

   /* One value tests. */
   { 0, TA_MOM_TEST, 14,  14, 14, TA_SUCCESS, 0, -0.50,     14, 1},

   /* Index too low test. */
   { 0, TA_MOM_TEST, 0,  15, 14, TA_SUCCESS, 0, -0.50,     14, 2},
   { 0, TA_MOM_TEST, 1,  15, 14, TA_SUCCESS, 0, -0.50,     14, 2},
   { 0, TA_MOM_TEST, 2,  16, 14, TA_SUCCESS, 0, -0.50,     14, 3},
   { 0, TA_MOM_TEST, 2,  16, 14, TA_SUCCESS, 1, -2.00,     14, 3},
   { 0, TA_MOM_TEST, 2,  16, 14, TA_SUCCESS, 2, -5.22,     14, 3},
   { 0, TA_MOM_TEST, 0,  14, 14, TA_SUCCESS, 0, -0.50,     14, 1},
   { 0, TA_MOM_TEST, 0,  13, 14, TA_SUCCESS, 0, -0.50,     14, 0},

   /* Middle of data test. */
   { 0, TA_MOM_TEST, 20,  21, 14, TA_SUCCESS, 0, -4.15,    20, 2 },
   { 0, TA_MOM_TEST, 20,  21, 14, TA_SUCCESS, 1, -5.12,    20, 2 },

   /**********************/
   /*      ROC TEST      */
   /**********************/
   { 1, TA_ROC_TEST, 0, 251, 14, TA_SUCCESS,      0, -0.546,  14,  252-14 }, /* First Value */
   { 0, TA_ROC_TEST, 0, 251, 14, TA_SUCCESS,      1, -2.109,  14,  252-14 },
   { 0, TA_ROC_TEST, 0, 251, 14, TA_SUCCESS,      2, -5.53,  14,  252-14 },
   { 0, TA_ROC_TEST, 0, 251, 14, TA_SUCCESS, 252-15, -1.0367,  14,  252-14 },  /* Last Value */

   /* No output value. */
   { 0, TA_ROC_TEST, 1, 1,  14, TA_SUCCESS, 0, 0, 0, 0},

   /* One value tests. */
   { 0, TA_ROC_TEST, 14,  14, 14, TA_SUCCESS, 0, -0.546,     14, 1},

   /* Index too low test. */
   { 0, TA_ROC_TEST, 0,  15, 14, TA_SUCCESS, 0, -0.546,     14, 2},
   { 0, TA_ROC_TEST, 1,  15, 14, TA_SUCCESS, 0, -0.546,     14, 2},
   { 0, TA_ROC_TEST, 2,  16, 14, TA_SUCCESS, 0, -0.546,     14, 3},
   { 0, TA_ROC_TEST, 2,  16, 14, TA_SUCCESS, 1, -2.109,     14, 3},
   { 0, TA_ROC_TEST, 2,  16, 14, TA_SUCCESS, 2, -5.53,     14, 3},
   { 0, TA_ROC_TEST, 0,  14, 14, TA_SUCCESS, 0, -0.546,     14, 1},
   { 0, TA_ROC_TEST, 0,  13, 14, TA_SUCCESS, 0, -0.546,     14, 0},

   /* Middle of data test. */
   { 0, TA_ROC_TEST, 20,  21, 14, TA_SUCCESS, 0, -4.49,    20, 2 },
   { 0, TA_ROC_TEST, 20,  21, 14, TA_SUCCESS, 1, -5.5256,    20, 2 },


   /**********************/
   /*     ROCR TEST      */
   /**********************/
   { 1, TA_ROCR_TEST, 0, 251, 14, TA_SUCCESS,      0, 99.4536,  14,  252-14 }, /* First Value */
   { 0, TA_ROCR_TEST, 0, 251, 14, TA_SUCCESS,      1, 97.8906,  14,  252-14 },
   { 0, TA_ROCR_TEST, 0, 251, 14, TA_SUCCESS,      2, 94.4689,  14,  252-14 },
   { 0, TA_ROCR_TEST, 0, 251, 14, TA_SUCCESS, 252-15, 98.9633,  14,  252-14 },  /* Last Value */

   /* No output value. */
   { 0, TA_ROCR_TEST, 1, 1,  14, TA_SUCCESS, 0, 0, 0, 0},

   /* One value tests. */
   { 0, TA_ROCR_TEST, 14,  14, 14, TA_SUCCESS, 0, 99.4536,     14, 1},

   /* Index too low test. */
   { 0, TA_ROCR_TEST, 0,  15, 14, TA_SUCCESS, 0, 99.4536,     14, 2},
   { 0, TA_ROCR_TEST, 1,  15, 14, TA_SUCCESS, 0, 99.4536,     14, 2},
   { 0, TA_ROCR_TEST, 2,  16, 14, TA_SUCCESS, 0, 99.4536,     14, 3},
   { 0, TA_ROCR_TEST, 2,  16, 14, TA_SUCCESS, 1, 97.8906,     14, 3},
   { 0, TA_ROCR_TEST, 2,  16, 14, TA_SUCCESS, 2, 94.4689,     14, 3},
   { 0, TA_ROCR_TEST, 0,  14, 14, TA_SUCCESS, 0, 99.4536,     14, 1},
   { 0, TA_ROCR_TEST, 0,  13, 14, TA_SUCCESS, 0, 99.4536,     14, 0},

   /* Middle of data test. */
   { 0, TA_ROCR_TEST, 20,  21, 14, TA_SUCCESS, 0, 95.5096,    20, 2 },
   { 0, TA_ROCR_TEST, 20,  21, 14, TA_SUCCESS, 1, 94.4744,    20, 2 }

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
ErrorNumber test_func_mom_roc( TA_Libc *libHandle, TA_History *history )
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

   if( testParam->test->theFunction == TA_MOM_TEST )
   {
      retCode = TA_MOM( libHandle,
                        startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod_0,                        
                        outBegIdx,
                        outNbElement,
                        outputBuffer );

      *lookback = TA_MOM_Lookback(testParam->test->optInTimePeriod_0 );
   }
   else if( testParam->test->theFunction == TA_ROC_TEST )
   {
      retCode = TA_ROC( libHandle,
                        startIdx,
                        endIdx,
                        testParam->close,
                        testParam->test->optInTimePeriod_0,                        
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_ROC_Lookback(testParam->test->optInTimePeriod_0 );
   }
   else if( testParam->test->theFunction == TA_ROCR_TEST )
   {
      retCode = TA_ROCR( libHandle,
                         startIdx,
                         endIdx,
                         testParam->close,
                         testParam->test->optInTimePeriod_0,                         
                         outBegIdx,
                         outNbElement,
                         outputBuffer );
      *lookback = TA_ROCR_Lookback(testParam->test->optInTimePeriod_0 );
   }
   else
      retCode = TA_INTERNAL_ERROR(130);

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
   setInputBuffer( 0, history->close, history->nbBars );
   setInputBuffer( 1, history->close, history->nbBars );

   CLEAR_EXPECTED_VALUE(0);

   /* Make a simple first call. */
   if( test->theFunction == TA_MOM_TEST )
   {
      retCode = TA_MOM( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }
   else if( test->theFunction == TA_ROC_TEST )
   {
      retCode = TA_ROC( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
   }
   else if( test->theFunction == TA_ROCR_TEST )
   {
      retCode = TA_ROCR( libHandle,
                         test->startIdx,
                         test->endIdx,
                         gBuffer[0].in,
                         test->optInTimePeriod_0,                         
                         &outBegIdx,
                         &outNbElement,
                         gBuffer[0].out0 );
   }

   errNb = checkDataSame( gBuffer[0].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   CLEAR_EXPECTED_VALUE(0);
   if( test->theFunction == TA_MOM_TEST )
   {
      retCode = TA_MOM( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }
   else if( test->theFunction == TA_ROC_TEST )
   {
      retCode = TA_ROC( libHandle,
                        test->startIdx,
                        test->endIdx,
                        gBuffer[1].in,
                        test->optInTimePeriod_0,                        
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
   }
   else if( test->theFunction == TA_ROCR_TEST )
   {
      retCode = TA_ROCR( libHandle,
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

