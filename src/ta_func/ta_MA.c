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
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  022203 MF   Add MAMA
 *  040503 MF   Add T3
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  111603 MF   Allow period of 1. Just copy input into output.
 *  060907 MF   Use TA_SMA/TA_EMA instead of internal implementation.
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
/* Generated */ int Core::MovingAverageLookback( int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                MAType        optInMAType ) /* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int movingAverageLookback( int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                 MAType        optInMAType ) /* Generated */ 
/* Generated */ #else
/* Generated */ TA_LIB_API int TA_MA_Lookback( int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                         TA_MAType     optInMAType ) /* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */
   int retValue;

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */    /* min/max are checked for optInTimePeriod. */
/* Generated */    if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInTimePeriod = 30;
/* Generated */    else if( ((int)optInTimePeriod < 1) || ((int)optInTimePeriod > 100000) )
/* Generated */       return -1;
/* Generated */ 
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    if( (int)optInMAType == TA_INTEGER_DEFAULT )
/* Generated */       optInMAType = (TA_MAType)0;
/* Generated */    else if( ((int)optInMAType < 0) || ((int)optInMAType > 8) )
/* Generated */       return -1;
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA)*/
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/

   /* insert lookback code here. */

   if( optInTimePeriod <= 1 )
      return 0;
   
   switch( optInMAType )
   {
   case ENUM_CASE(MAType, TA_MAType_SMA, Sma ):
      retValue = LOOKBACK_CALL(SMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_EMA, Ema):
      retValue = LOOKBACK_CALL(EMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_WMA, Wma):
      retValue = LOOKBACK_CALL(WMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_DEMA, Dema):
      retValue = LOOKBACK_CALL(DEMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_TEMA, Tema ):
      retValue = LOOKBACK_CALL(TEMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_TRIMA, Trima ):
      retValue = LOOKBACK_CALL(TRIMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_KAMA, Kama ):
      retValue = LOOKBACK_CALL(KAMA)( optInTimePeriod );
      break;

   case ENUM_CASE(MAType, TA_MAType_MAMA, Mama ):
      retValue = LOOKBACK_CALL(MAMA)( 0.5, 0.05 );
      break;

   case ENUM_CASE(MAType, TA_MAType_T3, T3):
      retValue = LOOKBACK_CALL(T3)( optInTimePeriod, 0.7 );
      break;

   default:
      retValue = 0;
   }

   return retValue;
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_MA - Moving average
 * 
 * Input  = double
 * Output = double
 * 
 * Optional Parameters
 * -------------------
 * optInTimePeriod:(From 1 to 100000)
 *    Number of period
 * 
 * optInMAType:
 *    Type of Moving Average
 * 
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::MovingAverage( int    startIdx,
/* Generated */                                               int    endIdx,
/* Generated */                                               SubArray<double>^ inReal,
/* Generated */                                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                               MAType        optInMAType,
/* Generated */                                               [Out]int%    outBegIdx,
/* Generated */                                               [Out]int%    outNBElement,
/* Generated */                                               SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::MovingAverage( int    startIdx,
/* Generated */                                               int    endIdx,
/* Generated */                                               cli::array<double>^ inReal,
/* Generated */                                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                               MAType        optInMAType,
/* Generated */                                               [Out]int%    outBegIdx,
/* Generated */                                               [Out]int%    outNBElement,
/* Generated */                                               cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode movingAverage( int    startIdx,
/* Generated */                               int    endIdx,
/* Generated */                               double       inReal[],
/* Generated */                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                               MAType        optInMAType,
/* Generated */                               MInteger     outBegIdx,
/* Generated */                               MInteger     outNBElement,
/* Generated */                               double        outReal[] )
/* Generated */ #else
/* Generated */ TA_LIB_API TA_RetCode TA_MA( int    startIdx,
/* Generated */                              int    endIdx,
/* Generated */                                         const double inReal[],
/* Generated */                                         int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                         TA_MAType     optInMAType,
/* Generated */                                         int          *outBegIdx,
/* Generated */                                         int          *outNBElement,
/* Generated */                                         double        outReal[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
   ARRAY_REF(dummyBuffer);
   ENUM_DECLARATION(RetCode) retCode;

   int nbElement;
   int outIdx, todayIdx;

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
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */    #endif /* !defined(_JAVA)*/
/* Generated */    /* min/max are checked for optInTimePeriod. */
/* Generated */    if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInTimePeriod = 30;
/* Generated */    else if( ((int)optInTimePeriod < 1) || ((int)optInTimePeriod > 100000) )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    if( (int)optInMAType == TA_INTEGER_DEFAULT )
/* Generated */       optInMAType = (TA_MAType)0;
/* Generated */    else if( ((int)optInMAType < 0) || ((int)optInMAType > 8) )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA)*/
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !outReal )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #endif /* !defined(_JAVA) */
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

   if( optInTimePeriod == 1 )
   {
      nbElement = endIdx-startIdx+1;
      VALUE_HANDLE_DEREF(outNBElement) = nbElement;      
      for( todayIdx=startIdx, outIdx=0; outIdx < nbElement; outIdx++, todayIdx++ )
         outReal[outIdx] = inReal[todayIdx];
      VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
   }
   /* Simply forward the job to the corresponding TA function. */
   switch( optInMAType )
   {
   case ENUM_CASE(MAType, TA_MAType_SMA, Sma):
      retCode = FUNCTION_CALL(SMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                    outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_EMA, Ema):
      retCode = FUNCTION_CALL(EMA)( startIdx, endIdx, inReal, optInTimePeriod,                                     
                                    outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_WMA, Wma):
      retCode = FUNCTION_CALL(WMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                    outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_DEMA, Dema):
      retCode = FUNCTION_CALL(DEMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                     outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_TEMA, Tema):
      retCode = FUNCTION_CALL(TEMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                     outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_TRIMA, Trima):
      retCode = FUNCTION_CALL(TRIMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                      outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_KAMA, Kama):
      retCode = FUNCTION_CALL(KAMA)( startIdx, endIdx, inReal, optInTimePeriod,
                                     outBegIdx, outNBElement, outReal );
      break;

   case ENUM_CASE(MAType, TA_MAType_MAMA, Mama):
      /* The optInTimePeriod is ignored and the FAMA output of the MAMA
       * is ignored.
       */
      ARRAY_ALLOC(dummyBuffer, (endIdx-startIdx+1) );

      #if !defined( _JAVA )
         if( !dummyBuffer )
            return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
      #endif

      retCode = FUNCTION_CALL(MAMA)( startIdx, endIdx, inReal, 0.5, 0.05,                           
                                     outBegIdx, outNBElement,
                                     outReal, dummyBuffer );
                         
      ARRAY_FREE( dummyBuffer );
      break;

   case ENUM_CASE(MAType, TA_MAType_T3, T3 ):
      retCode = FUNCTION_CALL(T3)( startIdx, endIdx, inReal,
                                   optInTimePeriod, 0.7,
                                   outBegIdx, outNBElement, outReal );
      break;

   default: 
      retCode = ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
      break;
   }

   return retCode;
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
/* Generated */ enum class Core::RetCode Core::MovingAverage( int    startIdx,
/* Generated */                                               int    endIdx,
/* Generated */                                               SubArray<float>^ inReal,
/* Generated */                                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                               MAType        optInMAType,
/* Generated */                                               [Out]int%    outBegIdx,
/* Generated */                                               [Out]int%    outNBElement,
/* Generated */                                               SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::MovingAverage( int    startIdx,
/* Generated */                                               int    endIdx,
/* Generated */                                               cli::array<float>^ inReal,
/* Generated */                                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                                               MAType        optInMAType,
/* Generated */                                               [Out]int%    outBegIdx,
/* Generated */                                               [Out]int%    outNBElement,
/* Generated */                                               cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode movingAverage( int    startIdx,
/* Generated */                               int    endIdx,
/* Generated */                               float        inReal[],
/* Generated */                               int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                               MAType        optInMAType,
/* Generated */                               MInteger     outBegIdx,
/* Generated */                               MInteger     outNBElement,
/* Generated */                               double        outReal[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_MA( int    startIdx,
/* Generated */                     int    endIdx,
/* Generated */                     const float  inReal[],
/* Generated */                     int           optInTimePeriod, /* From 1 to 100000 */
/* Generated */                     TA_MAType     optInMAType,
/* Generated */                     int          *outBegIdx,
/* Generated */                     int          *outNBElement,
/* Generated */                     double        outReal[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */    ARRAY_REF(dummyBuffer);
/* Generated */    ENUM_DECLARATION(RetCode) retCode;
/* Generated */    int nbElement;
/* Generated */    int outIdx, todayIdx;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */     if( (int)optInTimePeriod == TA_INTEGER_DEFAULT )
/* Generated */        optInTimePeriod = 30;
/* Generated */     else if( ((int)optInTimePeriod < 1) || ((int)optInTimePeriod > 100000) )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */     if( (int)optInMAType == TA_INTEGER_DEFAULT )
/* Generated */        optInMAType = (TA_MAType)0;
/* Generated */     else if( ((int)optInMAType < 0) || ((int)optInMAType > 8) )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !outReal )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */  #endif 
/* Generated */    if( optInTimePeriod == 1 )
/* Generated */    {
/* Generated */       nbElement = endIdx-startIdx+1;
/* Generated */       VALUE_HANDLE_DEREF(outNBElement) = nbElement;      
/* Generated */       for( todayIdx=startIdx, outIdx=0; outIdx < nbElement; outIdx++, todayIdx++ )
/* Generated */          outReal[outIdx] = inReal[todayIdx];
/* Generated */       VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
/* Generated */       return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */    }
/* Generated */    switch( optInMAType )
/* Generated */    {
/* Generated */    case ENUM_CASE(MAType, TA_MAType_SMA, Sma):
/* Generated */       retCode = FUNCTION_CALL(SMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                     outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_EMA, Ema):
/* Generated */       retCode = FUNCTION_CALL(EMA)( startIdx, endIdx, inReal, optInTimePeriod,                                     
/* Generated */                                     outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_WMA, Wma):
/* Generated */       retCode = FUNCTION_CALL(WMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                     outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_DEMA, Dema):
/* Generated */       retCode = FUNCTION_CALL(DEMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                      outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_TEMA, Tema):
/* Generated */       retCode = FUNCTION_CALL(TEMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                      outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_TRIMA, Trima):
/* Generated */       retCode = FUNCTION_CALL(TRIMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                       outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_KAMA, Kama):
/* Generated */       retCode = FUNCTION_CALL(KAMA)( startIdx, endIdx, inReal, optInTimePeriod,
/* Generated */                                      outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_MAMA, Mama):
/* Generated */       ARRAY_ALLOC(dummyBuffer, (endIdx-startIdx+1) );
/* Generated */       #if !defined( _JAVA )
/* Generated */          if( !dummyBuffer )
/* Generated */             return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
/* Generated */       #endif
/* Generated */       retCode = FUNCTION_CALL(MAMA)( startIdx, endIdx, inReal, 0.5, 0.05,                           
/* Generated */                                      outBegIdx, outNBElement,
/* Generated */                                      outReal, dummyBuffer );
/* Generated */       ARRAY_FREE( dummyBuffer );
/* Generated */       break;
/* Generated */    case ENUM_CASE(MAType, TA_MAType_T3, T3 ):
/* Generated */       retCode = FUNCTION_CALL(T3)( startIdx, endIdx, inReal,
/* Generated */                                    optInTimePeriod, 0.7,
/* Generated */                                    outBegIdx, outNBElement, outReal );
/* Generated */       break;
/* Generated */    default: 
/* Generated */       retCode = ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */       break;
/* Generated */    }
/* Generated */    return retCode;
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }}} // Close namespace TicTacTec.TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

