/* TA-LIB Copyright (c) 1999-2024, Mario Fortier
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
 *  AC       Angelo Ciceri
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120906 AC   Creation
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
/* Generated */ int Core::MinMaxLookback( int           optInTimePeriod )  /* From 2 to 100000 */
/* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int minMaxLookback( int           optInTimePeriod )  /* From 2 to 100000 */
/* Generated */ 
/* Generated */ #else
/* Generated */ TA_LIB_API int TA_MINMAX_Lookback( int           optInTimePeriod )  /* From 2 to 100000 */
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

   return (optInTimePeriod-1);
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_MINMAX - Lowest and highest values over a specified period
 * 
 * Input  = double
 * Output = double, double
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
/* Generated */ enum class Core::RetCode Core::MinMax( int    startIdx,
/* Generated */                                        int    endIdx,
/* Generated */                                        SubArray<double>^ inReal,
/* Generated */                                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                        [Out]int%    outBegIdx,
/* Generated */                                        [Out]int%    outNBElement,
/* Generated */                                        SubArray<double>^  outMin,
/* Generated */                                        SubArray<double>^  outMax )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::MinMax( int    startIdx,
/* Generated */                                        int    endIdx,
/* Generated */                                        cli::array<double>^ inReal,
/* Generated */                                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                        [Out]int%    outBegIdx,
/* Generated */                                        [Out]int%    outNBElement,
/* Generated */                                        cli::array<double>^  outMin,
/* Generated */                                        cli::array<double>^  outMax )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode minMax( int    startIdx,
/* Generated */                        int    endIdx,
/* Generated */                        double       inReal[],
/* Generated */                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                        MInteger     outBegIdx,
/* Generated */                        MInteger     outNBElement,
/* Generated */                        double        outMin[],
/* Generated */                        double        outMax[] )
/* Generated */ #else
/* Generated */ TA_LIB_API TA_RetCode TA_MINMAX( int    startIdx,
/* Generated */                                  int    endIdx,
/* Generated */                                             const double inReal[],
/* Generated */                                             int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                             int          *outBegIdx,
/* Generated */                                             int          *outNBElement,
/* Generated */                                             double        outMin[],
/* Generated */                                             double        outMax[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
   double highest, lowest, tmpHigh, tmpLow;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx, lowestIdx;

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
/* Generated */    else if( ((int)optInTimePeriod < 2) || ((int)optInTimePeriod > 100000) )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !outMin )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    if( !outMax )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */ 
/* Generated */    #endif /* !defined(_JAVA) */
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

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
      VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
      VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;
   highestIdx  = -1;
   highest     = 0.0;
   lowestIdx   = -1;
   lowest      = 0.0;

   while( today <= endIdx )
   {
      tmpLow = tmpHigh = inReal[today];

      if( highestIdx < trailingIdx )
      {
        highestIdx = trailingIdx;
        highest = inReal[highestIdx];
        i = highestIdx;
        while( ++i<=today )
        {
           tmpHigh = inReal[i];
           if( tmpHigh > highest )
           {
              highestIdx = i;
              highest = tmpHigh;
           }
        }
      }
      else if( tmpHigh >= highest )
      {
        highestIdx = today;
        highest = tmpHigh;
      }

      if( lowestIdx < trailingIdx )
      {
        lowestIdx = trailingIdx;
        lowest = inReal[lowestIdx];
        i = lowestIdx;
        while( ++i<=today )
        {
           tmpLow = inReal[i];
           if( tmpLow < lowest )
           {
              lowestIdx = i;
              lowest = tmpLow;
           }
        }
      }
      else if( tmpLow <= lowest )
      {
        lowestIdx = today;
        lowest = tmpLow;
      }

      outMax[outIdx] = highest;
      outMin[outIdx] = lowest;
      outIdx++;
      trailingIdx++;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
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
/* Generated */ enum class Core::RetCode Core::MinMax( int    startIdx,
/* Generated */                                        int    endIdx,
/* Generated */                                        SubArray<float>^ inReal,
/* Generated */                                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                        [Out]int%    outBegIdx,
/* Generated */                                        [Out]int%    outNBElement,
/* Generated */                                        SubArray<double>^  outMin,
/* Generated */                                        SubArray<double>^  outMax )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::MinMax( int    startIdx,
/* Generated */                                        int    endIdx,
/* Generated */                                        cli::array<float>^ inReal,
/* Generated */                                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                                        [Out]int%    outBegIdx,
/* Generated */                                        [Out]int%    outNBElement,
/* Generated */                                        cli::array<double>^  outMin,
/* Generated */                                        cli::array<double>^  outMax )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode minMax( int    startIdx,
/* Generated */                        int    endIdx,
/* Generated */                        float        inReal[],
/* Generated */                        int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                        MInteger     outBegIdx,
/* Generated */                        MInteger     outNBElement,
/* Generated */                        double        outMin[],
/* Generated */                        double        outMax[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_MINMAX( int    startIdx,
/* Generated */                         int    endIdx,
/* Generated */                         const float  inReal[],
/* Generated */                         int           optInTimePeriod, /* From 2 to 100000 */
/* Generated */                         int          *outBegIdx,
/* Generated */                         int          *outNBElement,
/* Generated */                         double        outMin[],
/* Generated */                         double        outMax[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */    double highest, lowest, tmpHigh, tmpLow;
/* Generated */    int outIdx, nbInitialElementNeeded;
/* Generated */    int trailingIdx, today, i, highestIdx, lowestIdx;
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
/* Generated */     else if( ((int)optInTimePeriod < 2) || ((int)optInTimePeriod > 100000) )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !outMin )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     if( !outMax )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif 
/* Generated */  #endif 
/* Generated */    nbInitialElementNeeded = (optInTimePeriod-1);
/* Generated */    if( startIdx < nbInitialElementNeeded )
/* Generated */       startIdx = nbInitialElementNeeded;
/* Generated */    if( startIdx > endIdx )
/* Generated */    {
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
/* Generated */       return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */    }
/* Generated */    outIdx = 0;
/* Generated */    today       = startIdx;
/* Generated */    trailingIdx = startIdx-nbInitialElementNeeded;
/* Generated */    highestIdx  = -1;
/* Generated */    highest     = 0.0;
/* Generated */    lowestIdx   = -1;
/* Generated */    lowest      = 0.0;
/* Generated */    while( today <= endIdx )
/* Generated */    {
/* Generated */       tmpLow = tmpHigh = inReal[today];
/* Generated */       if( highestIdx < trailingIdx )
/* Generated */       {
/* Generated */         highestIdx = trailingIdx;
/* Generated */         highest = inReal[highestIdx];
/* Generated */         i = highestIdx;
/* Generated */         while( ++i<=today )
/* Generated */         {
/* Generated */            tmpHigh = inReal[i];
/* Generated */            if( tmpHigh > highest )
/* Generated */            {
/* Generated */               highestIdx = i;
/* Generated */               highest = tmpHigh;
/* Generated */            }
/* Generated */         }
/* Generated */       }
/* Generated */       else if( tmpHigh >= highest )
/* Generated */       {
/* Generated */         highestIdx = today;
/* Generated */         highest = tmpHigh;
/* Generated */       }
/* Generated */       if( lowestIdx < trailingIdx )
/* Generated */       {
/* Generated */         lowestIdx = trailingIdx;
/* Generated */         lowest = inReal[lowestIdx];
/* Generated */         i = lowestIdx;
/* Generated */         while( ++i<=today )
/* Generated */         {
/* Generated */            tmpLow = inReal[i];
/* Generated */            if( tmpLow < lowest )
/* Generated */            {
/* Generated */               lowestIdx = i;
/* Generated */               lowest = tmpLow;
/* Generated */            }
/* Generated */         }
/* Generated */       }
/* Generated */       else if( tmpLow <= lowest )
/* Generated */       {
/* Generated */         lowestIdx = today;
/* Generated */         lowest = tmpLow;
/* Generated */       }
/* Generated */       outMax[outIdx] = highest;
/* Generated */       outMin[outIdx] = lowest;
/* Generated */       outIdx++;
/* Generated */       trailingIdx++;
/* Generated */       today++;
/* Generated */    }
/* Generated */    VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
/* Generated */    VALUE_HANDLE_DEREF(outNBElement) = outIdx;
/* Generated */    return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }}} // Close namespace TicTacTec.TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

