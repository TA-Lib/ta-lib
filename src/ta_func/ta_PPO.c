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
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  020605 AA   Fix #1117666 Lookback bug.
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
/* Generated */ int Core::PpoLookback( int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                      int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                      MAType        optInMAType ) /* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int ppoLookback( int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                       int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                       MAType        optInMAType ) /* Generated */ 
/* Generated */ #else
/* Generated */ TA_LIB_API int TA_PPO_Lookback( int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                          int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                          TA_MAType     optInMAType ) /* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */    /* min/max are checked for optInFastPeriod. */
/* Generated */    if( (int)optInFastPeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInFastPeriod = 12;
/* Generated */    else if( ((int)optInFastPeriod < 2) || ((int)optInFastPeriod > 100000) )
/* Generated */       return -1;
/* Generated */ 
/* Generated */    /* min/max are checked for optInSlowPeriod. */
/* Generated */    if( (int)optInSlowPeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInSlowPeriod = 26;
/* Generated */    else if( ((int)optInSlowPeriod < 2) || ((int)optInSlowPeriod > 100000) )
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

   /* Lookback is driven by the slowest MA. */
   return LOOKBACK_CALL(MA)( max(optInSlowPeriod,optInFastPeriod), optInMAType );
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_PPO - Percentage Price Oscillator
 * 
 * Input  = double
 * Output = double
 * 
 * Optional Parameters
 * -------------------
 * optInFastPeriod:(From 2 to 100000)
 *    Number of period for the fast MA
 * 
 * optInSlowPeriod:(From 2 to 100000)
 *    Number of period for the slow MA
 * 
 * optInMAType:
 *    Type of Moving Average
 * 
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::Ppo( int    startIdx,
/* Generated */                                     int    endIdx,
/* Generated */                                     SubArray<double>^ inReal,
/* Generated */                                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                     MAType        optInMAType,
/* Generated */                                     [Out]int%    outBegIdx,
/* Generated */                                     [Out]int%    outNBElement,
/* Generated */                                     SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::Ppo( int    startIdx,
/* Generated */                                     int    endIdx,
/* Generated */                                     cli::array<double>^ inReal,
/* Generated */                                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                     MAType        optInMAType,
/* Generated */                                     [Out]int%    outBegIdx,
/* Generated */                                     [Out]int%    outNBElement,
/* Generated */                                     cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode ppo( int    startIdx,
/* Generated */                     int    endIdx,
/* Generated */                     double       inReal[],
/* Generated */                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                     MAType        optInMAType,
/* Generated */                     MInteger     outBegIdx,
/* Generated */                     MInteger     outNBElement,
/* Generated */                     double        outReal[] )
/* Generated */ #else
/* Generated */ TA_LIB_API TA_RetCode TA_PPO( int    startIdx,
/* Generated */                               int    endIdx,
/* Generated */                                          const double inReal[],
/* Generated */                                          int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                          int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                          TA_MAType     optInMAType,
/* Generated */                                          int          *outBegIdx,
/* Generated */                                          int          *outNBElement,
/* Generated */                                          double        outReal[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
   ARRAY_REF(tempBuffer);
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
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */    #endif /* !defined(_JAVA)*/
/* Generated */    /* min/max are checked for optInFastPeriod. */
/* Generated */    if( (int)optInFastPeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInFastPeriod = 12;
/* Generated */    else if( ((int)optInFastPeriod < 2) || ((int)optInFastPeriod > 100000) )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    /* min/max are checked for optInSlowPeriod. */
/* Generated */    if( (int)optInSlowPeriod == TA_INTEGER_DEFAULT )
/* Generated */       optInSlowPeriod = 26;
/* Generated */    else if( ((int)optInSlowPeriod < 2) || ((int)optInSlowPeriod > 100000) )
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

   /* Insert TA function code here. */

   /* Allocate an intermediate buffer. */
   ARRAY_ALLOC( tempBuffer, endIdx-startIdx+1 );
   #if !defined( _JAVA )
      if( !tempBuffer )
         return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
   #endif

   retCode = FUNCTION_CALL(INT_PO)( startIdx, endIdx, inReal,
                                    optInFastPeriod,
                                    optInSlowPeriod,
                                    optInMAType,
                                    outBegIdx,
                                    outNBElement,
                                    outReal,
                                    tempBuffer,
                                    1 /* Do percentage processing. */ );

   ARRAY_FREE( tempBuffer );
   
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
/* Generated */ enum class Core::RetCode Core::Ppo( int    startIdx,
/* Generated */                                     int    endIdx,
/* Generated */                                     SubArray<float>^ inReal,
/* Generated */                                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                     MAType        optInMAType,
/* Generated */                                     [Out]int%    outBegIdx,
/* Generated */                                     [Out]int%    outNBElement,
/* Generated */                                     SubArray<double>^  outReal )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::Ppo( int    startIdx,
/* Generated */                                     int    endIdx,
/* Generated */                                     cli::array<float>^ inReal,
/* Generated */                                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                                     MAType        optInMAType,
/* Generated */                                     [Out]int%    outBegIdx,
/* Generated */                                     [Out]int%    outNBElement,
/* Generated */                                     cli::array<double>^  outReal )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode ppo( int    startIdx,
/* Generated */                     int    endIdx,
/* Generated */                     float        inReal[],
/* Generated */                     int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                     int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                     MAType        optInMAType,
/* Generated */                     MInteger     outBegIdx,
/* Generated */                     MInteger     outNBElement,
/* Generated */                     double        outReal[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_PPO( int    startIdx,
/* Generated */                      int    endIdx,
/* Generated */                      const float  inReal[],
/* Generated */                      int           optInFastPeriod, /* From 2 to 100000 */
/* Generated */                      int           optInSlowPeriod, /* From 2 to 100000 */
/* Generated */                      TA_MAType     optInMAType,
/* Generated */                      int          *outBegIdx,
/* Generated */                      int          *outNBElement,
/* Generated */                      double        outReal[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */    ARRAY_REF(tempBuffer);
/* Generated */    ENUM_DECLARATION(RetCode) retCode;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !inReal ) return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */     if( (int)optInFastPeriod == TA_INTEGER_DEFAULT )
/* Generated */        optInFastPeriod = 12;
/* Generated */     else if( ((int)optInFastPeriod < 2) || ((int)optInFastPeriod > 100000) )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     if( (int)optInSlowPeriod == TA_INTEGER_DEFAULT )
/* Generated */        optInSlowPeriod = 26;
/* Generated */     else if( ((int)optInSlowPeriod < 2) || ((int)optInSlowPeriod > 100000) )
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
/* Generated */    ARRAY_ALLOC( tempBuffer, endIdx-startIdx+1 );
/* Generated */    #if !defined( _JAVA )
/* Generated */       if( !tempBuffer )
/* Generated */          return ENUM_VALUE(RetCode,TA_ALLOC_ERR,AllocErr);
/* Generated */    #endif
/* Generated */    retCode = FUNCTION_CALL(INT_PO)( startIdx, endIdx, inReal,
/* Generated */                                     optInFastPeriod,
/* Generated */                                     optInSlowPeriod,
/* Generated */                                     optInMAType,
/* Generated */                                     outBegIdx,
/* Generated */                                     outNBElement,
/* Generated */                                     outReal,
/* Generated */                                     tempBuffer,
/* Generated */                                     1  );
/* Generated */    ARRAY_FREE( tempBuffer );
/* Generated */    return retCode;
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }}} // Close namespace TicTacTec.TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

