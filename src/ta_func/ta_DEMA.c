/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *  010102 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
/* All code within this section is automatically
 * generated by gen_code. Any modification will be lost
 * next time gen_code is run.
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */    #include "TA-Lib-Core.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (RetCode::InternalError)
/* Generated */    namespace TicTacTec { namespace TA { namespace Library {
/* Generated */ #elif defined( _JAVA )
/* Generated */    #include "ta_defs.h"
/* Generated */    #include "ta_java_defs.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (RetCode.InternalError)
/* Generated */ #elif defined( _RUST )
/* Generated */    #include "ta_defs.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (RetCode.InternalError)
/* Generated */    impl core {
/* Generated */ #else
/* Generated */    #include <string.h>
/* Generated */    #include <math.h>
/* Generated */    #include "ta_func.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #ifndef TA_UTILITY_H
/* Generated */    #include "ta_utility.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #ifndef TA_MEMORY_H
/* Generated */    #include "ta_memory.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #define TA_PREFIX(x) TA_##x
/* Generated */ #define INPUT_TYPE   double
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ int Core::DemaLookback( int           optInTimePeriod )  /* From 2 to 100000 */
/* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int demaLookback( int           optInTimePeriod )  /* From 2 to 100000 */
/* Generated */ 
/* Generated */ #elif defined( _RUST )
/* Generated */ fn dema_lookback( /* Generated */ 
/* Generated */ #else
/* Generated */ TA_LIB_API int TA_DEMA_Lookback( int           optInTimePeriod )  /* From 2 to 100000 */
/* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */    /* min/max are checked for optInTimePeriod. */
/* Generated */    if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInTimePeriod = 30;
/* Generated */    else if( ((int)optInTimePeriod < 2) || ((int)optInTimePeriod > 100000) )
/* Generated */       return -1;
/* Generated */ 
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/

   /* insert lookback code here. */

   /* Get lookback for one EMA.
    * Multiply by two (because double smoothing).
    */
   return LOOKBACK_CALL(EMA)( optInTimePeriod ) * 2;
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_DEMA - Double Exponential Moving Average
 * 
 * Input  = double
 * Output = double
 * 
 * Optional Parameters
 * -------------------
 * optInTimePeriod:(From 2 to 100000)
 *    Number of period
 * 
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::Dema( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      SubArray<double>^ inReal,
/* Generated */                                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                      [Out]int%    outBegIdx,
/* Generated */                                      [Out]int%    outNBElement,
/* Generated */                                      SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::Dema( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      cli::array<double>^ inReal,
/* Generated */                                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                      [Out]int%    outBegIdx,
/* Generated */                                      [Out]int%    outNBElement,
/* Generated */                                      cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode dema( int    startIdx,
/* Generated */                      int    endIdx,
/* Generated */                      double       inReal[],
/* Generated */                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                      MInteger     outBegIdx,
/* Generated */                      MInteger     outNBElement,
/* Generated */                      double        outReal[] )
/* Generated */ #elif defined( _RUST )
/* Generated */ fn dema( int    startIdx,
/* Generated */ 
/* Generated */                           int    endIdx,
double inReal[],
double optInTimePeriod[],
mut outBegIdx,
mut outNBElement,
double outReal[],
)
/* Generated */ #else
/* Generated */ TA_LIB_API TA_RetCode TA_DEMA( int    startIdx,
/* Generated */                                int    endIdx,
/* Generated */                                           const double inReal[],
/* Generated */                                           int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                           int          *outBegIdx,
/* Generated */                                           int          *outNBElement,
/* Generated */                                           double        outReal[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
   ARRAY_REF(firstEMA);
   ARRAY_REF(secondEMA);
   double k;
   VALUE_HANDLE_INT(firstEMABegIdx);
   VALUE_HANDLE_INT(firstEMANbElement);
   VALUE_HANDLE_INT(secondEMABegIdx);
   VALUE_HANDLE_INT(secondEMANbElement);
   int tempInt, outIdx, firstEMAIdx, lookbackTotal, lookbackEMA;
   ENUM_DECLARATION(RetCode) retCode;

/**** START GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */ 
/* Generated */    /* Validate the requested output range. */
/* Generated */    if( startIdx < 0 )
/* Generated */       return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */    if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */       return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */ 
/* Generated */ #if defined( _RUST )
/* Generated */ 
/* Generated */ #else
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */    #endif /* !defined(_JAVA)*/
/* Generated */    /* min/max are checked for optInTimePeriod. */
/* Generated */    if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInTimePeriod = 30;
/* Generated */    else if( ((int)optInTimePeriod < 2) || ((int)optInTimePeriod > 100000) )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !outReal )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #endif /* !defined(_JAVA) */
/* Generated */ #endif
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

   /* Insert TA function code here. */

   /* For an explanation of this function, please read
    *
    * Stocks & Commodities V. 12:1 (11-19):
    *   Smoothing Data With Faster Moving Averages
    * Stocks & Commodities V. 12:2 (72-80):
    *   Smoothing Data With Less Lag
    *
    * Both magazine articles written by Patrick G. Mulloy
    *
    * Essentially, a DEMA of time serie 't' is:
    *   EMA2 = EMA(EMA(t,period),period)
    *   DEMA = 2*EMA(t,period)- EMA2
    *
    * DEMA offers a moving average with less lags then the
    * traditional EMA.
    *
    * Do not confuse a DEMA with the EMA2. Both are called
    * "Double EMA" in the litterature, but EMA2 is a simple
    * EMA of an EMA, while DEMA is a compostie of a single
    * EMA with EMA2.
    *
    * TEMA is very similar (and from the same author).
    */

   /* Will change only on success. */
   VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
   VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);

   /* Adjust startIdx to account for the lookback period. */
   lookbackEMA = LOOKBACK_CALL(EMA)( optInTimePeriod );
   lookbackTotal = lookbackEMA * 2;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success);

   /* Allocate a temporary buffer for the firstEMA.
    *
    * When possible, re-use the outputBuffer for temp
    * calculation.
    */
   #if defined(USE_SINGLE_PRECISION_INPUT) || defined( USE_SUBARRAY )
      tempInt = lookbackTotal+(endIdx-startIdx)+1;
      ARRAY_ALLOC(firstEMA, tempInt );
      #if !defined( _JAVA )
         if( !firstEMA )
            return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
      #endif
   #else
      if( inReal == outReal )
         firstEMA = outReal;
      else
      {
         tempInt = lookbackTotal+(endIdx-startIdx)+1;
         ARRAY_ALLOC(firstEMA, tempInt );
         #if !defined( _JAVA )
            if( !firstEMA )
               return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
         #endif
      }
   #endif

   /* Calculate the first EMA */
   k = PER_TO_K(optInTimePeriod);
   retCode = FUNCTION_CALL(INT_EMA)( startIdx-lookbackEMA, endIdx, inReal,
                                     optInTimePeriod, k,
                                     VALUE_HANDLE_OUT(firstEMABegIdx), VALUE_HANDLE_OUT(firstEMANbElement),
								     firstEMA );

   /* Verify for failure or if not enough data after
    * calculating the first EMA.
    */
   if( (retCode != ENUM_VALUE(RetCode,TA_SUCCESS,Success)) || (VALUE_HANDLE_GET(firstEMANbElement) == 0) )
   {
      ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
      return retCode;
   }

   /* Allocate a temporary buffer for storing the EMA of the EMA. */
   ARRAY_ALLOC(secondEMA, VALUE_HANDLE_GET(firstEMANbElement));

   #if !defined( _JAVA )
      if( !secondEMA )
      {
         ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
         return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
      }
   #endif

   retCode = FUNCTION_CALL_DOUBLE(INT_EMA)( 0, VALUE_HANDLE_GET(firstEMANbElement)-1, firstEMA,
                                            optInTimePeriod, k,
                                            VALUE_HANDLE_OUT(secondEMABegIdx), VALUE_HANDLE_OUT(secondEMANbElement),
						                    secondEMA );

   /* Return empty output on failure or if not enough data after
    * calculating the second EMA.
    */
   if( (retCode != ENUM_VALUE(RetCode,TA_SUCCESS,Success)) || (VALUE_HANDLE_GET(secondEMANbElement) == 0) )
   {
      #if defined(USE_SINGLE_PRECISION_INPUT)
         ARRAY_FREE( firstEMA );
      #else
         ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
      #endif
      ARRAY_FREE( secondEMA );
      return retCode;
   }

   /* Iterate through the second EMA and write the DEMA into
    * the output.
    */
   firstEMAIdx = VALUE_HANDLE_GET(secondEMABegIdx);
   outIdx = 0;
   while( outIdx < VALUE_HANDLE_GET(secondEMANbElement) )
   {
      outReal[outIdx] = (2.0*firstEMA[firstEMAIdx++]) - secondEMA[outIdx];
      outIdx++;
   }

   #if defined(USE_SINGLE_PRECISION_INPUT)
      ARRAY_FREE( firstEMA );
   #else
      ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
   #endif
   ARRAY_FREE( secondEMA );

   /* Succeed. Indicate where the output starts relative to
    * the caller input.
    */
   VALUE_HANDLE_DEREF(outBegIdx)    = VALUE_HANDLE_GET(firstEMABegIdx) + VALUE_HANDLE_GET(secondEMABegIdx);
   VALUE_HANDLE_DEREF(outNBElement) = outIdx;

   return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
}


/**** START GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #define  USE_SINGLE_PRECISION_INPUT
/* Generated */ #if !defined( _MANAGED ) && !defined( _JAVA )
/* Generated */    #undef   TA_PREFIX
/* Generated */    #define  TA_PREFIX(x) TA_S_##x
/* Generated */ #endif
/* Generated */ #undef   INPUT_TYPE
/* Generated */ #define  INPUT_TYPE float
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::Dema( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      SubArray<float>^ inReal,
/* Generated */                                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                      [Out]int%    outBegIdx,
/* Generated */                                      [Out]int%    outNBElement,
/* Generated */                                      SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::Dema( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      cli::array<float>^ inReal,
/* Generated */                                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                      [Out]int%    outBegIdx,
/* Generated */                                      [Out]int%    outNBElement,
/* Generated */                                      cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode dema( int    startIdx,
/* Generated */                      int    endIdx,
/* Generated */                      float        inReal[],
/* Generated */                      int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                      MInteger     outBegIdx,
/* Generated */                      MInteger     outNBElement,
/* Generated */                      double        outReal[] )
/* Generated */ #elif defined( _RUST )
/* Generated */ fn dema_s( int    startIdx,
/* Generated */ 
/* Generated */                             int    endIdx,
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_DEMA( int    startIdx,
/* Generated */                       int    endIdx,
/* Generated */                       const float  inReal[],
/* Generated */                       int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                       int          *outBegIdx,
/* Generated */                       int          *outNBElement,
/* Generated */                       double        outReal[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */    ARRAY_REF(firstEMA);
/* Generated */    ARRAY_REF(secondEMA);
/* Generated */    double k;
/* Generated */    VALUE_HANDLE_INT(firstEMABegIdx);
/* Generated */    VALUE_HANDLE_INT(firstEMANbElement);
/* Generated */    VALUE_HANDLE_INT(secondEMABegIdx);
/* Generated */    VALUE_HANDLE_INT(secondEMANbElement);
/* Generated */    int tempInt, outIdx, firstEMAIdx, lookbackTotal, lookbackEMA;
/* Generated */    ENUM_DECLARATION(RetCode) retCode;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */  #if defined( _RUST )
/* Generated */  #else
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */     if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */        optInTimePeriod = 30;
/* Generated */     else if( ((int)optInTimePeriod < 2) || ((int)optInTimePeriod > 100000) )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !outReal )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */  #endif
/* Generated */  #endif 
/* Generated */    VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
/* Generated */    VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
/* Generated */    lookbackEMA = LOOKBACK_CALL(EMA)( optInTimePeriod );
/* Generated */    lookbackTotal = lookbackEMA * 2;
/* Generated */    if( startIdx < lookbackTotal )
/* Generated */       startIdx = lookbackTotal;
/* Generated */    if( startIdx > endIdx )
/* Generated */       return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */    #if defined(USE_SINGLE_PRECISION_INPUT) || defined( USE_SUBARRAY )
/* Generated */       tempInt = lookbackTotal+(endIdx-startIdx)+1;
/* Generated */       ARRAY_ALLOC(firstEMA, tempInt );
/* Generated */       #if !defined( _JAVA )
/* Generated */          if( !firstEMA )
/* Generated */             return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
/* Generated */       #endif
/* Generated */    #else
/* Generated */       if( inReal == outReal )
/* Generated */          firstEMA = outReal;
/* Generated */       else
/* Generated */       {
/* Generated */          tempInt = lookbackTotal+(endIdx-startIdx)+1;
/* Generated */          ARRAY_ALLOC(firstEMA, tempInt );
/* Generated */          #if !defined( _JAVA )
/* Generated */             if( !firstEMA )
/* Generated */                return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
/* Generated */          #endif
/* Generated */       }
/* Generated */    #endif
/* Generated */    k = PER_TO_K(optInTimePeriod);
/* Generated */    retCode = FUNCTION_CALL(INT_EMA)( startIdx-lookbackEMA, endIdx, inReal,
/* Generated */                                      optInTimePeriod, k,
/* Generated */                                      VALUE_HANDLE_OUT(firstEMABegIdx), VALUE_HANDLE_OUT(firstEMANbElement),
/* Generated */ 								     firstEMA );
/* Generated */    if( (retCode != ENUM_VALUE(RetCode,TA_SUCCESS,Success)) || (VALUE_HANDLE_GET(firstEMANbElement) == 0) )
/* Generated */    {
/* Generated */       ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
/* Generated */       return retCode;
/* Generated */    }
/* Generated */    ARRAY_ALLOC(secondEMA, VALUE_HANDLE_GET(firstEMANbElement));
/* Generated */    #if !defined( _JAVA )
/* Generated */       if( !secondEMA )
/* Generated */       {
/* Generated */          ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
/* Generated */          return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
/* Generated */       }
/* Generated */    #endif
/* Generated */    retCode = FUNCTION_CALL_DOUBLE(INT_EMA)( 0, VALUE_HANDLE_GET(firstEMANbElement)-1, firstEMA,
/* Generated */                                             optInTimePeriod, k,
/* Generated */                                             VALUE_HANDLE_OUT(secondEMABegIdx), VALUE_HANDLE_OUT(secondEMANbElement),
/* Generated */ 						                    secondEMA );
/* Generated */    if( (retCode != ENUM_VALUE(RetCode,TA_SUCCESS,Success)) || (VALUE_HANDLE_GET(secondEMANbElement) == 0) )
/* Generated */    {
/* Generated */       #if defined(USE_SINGLE_PRECISION_INPUT)
/* Generated */          ARRAY_FREE( firstEMA );
/* Generated */       #else
/* Generated */          ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
/* Generated */       #endif
/* Generated */       ARRAY_FREE( secondEMA );
/* Generated */       return retCode;
/* Generated */    }
/* Generated */    firstEMAIdx = VALUE_HANDLE_GET(secondEMABegIdx);
/* Generated */    outIdx = 0;
/* Generated */    while( outIdx < VALUE_HANDLE_GET(secondEMANbElement) )
/* Generated */    {
/* Generated */       outReal[outIdx] = (2.0*firstEMA[firstEMAIdx++]) - secondEMA[outIdx];
/* Generated */       outIdx++;
/* Generated */    }
/* Generated */    #if defined(USE_SINGLE_PRECISION_INPUT)
/* Generated */       ARRAY_FREE( firstEMA );
/* Generated */    #else
/* Generated */       ARRAY_FREE_COND( firstEMA != outReal, firstEMA );
/* Generated */    #endif
/* Generated */    ARRAY_FREE( secondEMA );
/* Generated */    VALUE_HANDLE_DEREF(outBegIdx)    = VALUE_HANDLE_GET(firstEMABegIdx) + VALUE_HANDLE_GET(secondEMABegIdx);
/* Generated */    VALUE_HANDLE_DEREF(outNBElement) = outIdx;
/* Generated */    return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }}} // Close namespace TicTacTec.TA.Lib
/* Generated */ #elif defined( _RUST )
/* Generated */ } // Close impl core
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

