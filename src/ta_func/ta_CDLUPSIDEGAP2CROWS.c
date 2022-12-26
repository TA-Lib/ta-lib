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
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110104 AC   Creation
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
/* Generated */ int Core::CdlUpsideGap2CrowsLookback( void )
/* Generated */
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int cdlUpsideGap2CrowsLookback(  )
/* Generated */
/* Generated */ #else
/* Generated */ TA_LIB_API int TA_CDLUPSIDEGAP2CROWS_Lookback( void )
/* Generated */
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ /* No parameters to validate. */
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/

   /* insert lookback code here. */
    return max( TA_CANDLEAVGPERIOD(BodyShort), TA_CANDLEAVGPERIOD(BodyLong) ) + 2;
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_CDLUPSIDEGAP2CROWS - Upside Gap Two Crows
 *
 * Input  = Open, High, Low, Close
 * Output = int
 *
 */
/* Generated */
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::CdlUpsideGap2Crows( int    startIdx,
/* Generated */                                                    int    endIdx,
/* Generated */                                                    SubArray<double>^ inOpen,
/* Generated */                                                    SubArray<double>^ inHigh,
/* Generated */                                                    SubArray<double>^ inLow,
/* Generated */                                                    SubArray<double>^ inClose,
/* Generated */                                                    [Out]int%    outBegIdx,
/* Generated */                                                    [Out]int%    outNBElement,
/* Generated */                                                    SubArray<int>^  outInteger )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::CdlUpsideGap2Crows( int    startIdx,
/* Generated */                                                    int    endIdx,
/* Generated */                                                    cli::array<double>^ inOpen,
/* Generated */                                                    cli::array<double>^ inHigh,
/* Generated */                                                    cli::array<double>^ inLow,
/* Generated */                                                    cli::array<double>^ inClose,
/* Generated */                                                    [Out]int%    outBegIdx,
/* Generated */                                                    [Out]int%    outNBElement,
/* Generated */                                                    cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode cdlUpsideGap2Crows( int    startIdx,
/* Generated */                                    int    endIdx,
/* Generated */                                    double       inOpen[],
/* Generated */                                    double       inHigh[],
/* Generated */                                    double       inLow[],
/* Generated */                                    double       inClose[],
/* Generated */                                    MInteger     outBegIdx,
/* Generated */                                    MInteger     outNBElement,
/* Generated */                                    int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_LIB_API TA_RetCode TA_CDLUPSIDEGAP2CROWS( int    startIdx,
/* Generated */                                              int    endIdx,
/* Generated */                                                         const double inOpen[],
/* Generated */                                                         const double inHigh[],
/* Generated */                                                         const double inLow[],
/* Generated */                                                         const double inClose[],
/* Generated */                                                         int          *outBegIdx,
/* Generated */                                                         int          *outNBElement,
/* Generated */                                                         int           outInteger[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
    double BodyShortPeriodTotal, BodyLongPeriodTotal;
    int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

/**** START GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/
/* Generated */
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */
/* Generated */    /* Validate the requested output range. */
/* Generated */    if( startIdx < 0 )
/* Generated */       return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */    if( endIdx < 0 || endIdx < startIdx)
/* Generated */       return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */
/* Generated */    #if !defined(_JAVA)
/* Generated */    /* Verify required price component. */
/* Generated */    if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */
/* Generated */    #endif /* !defined(_JAVA)*/
/* Generated */    #if !defined(_JAVA)
/* Generated */    if( !outInteger )
/* Generated */       return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */
/* Generated */    #endif /* !defined(_JAVA) */
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = LOOKBACK_CALL(CDLUPSIDEGAP2CROWS)();

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
      VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
   }

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   BodyLongPeriodTotal = 0;
   BodyShortPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx -2 - TA_CANDLEAVGPERIOD(BodyLong);
   BodyShortTrailingIdx = startIdx -1 - TA_CANDLEAVGPERIOD(BodyShort);

   i = BodyLongTrailingIdx;
   while( i < startIdx-2 ) {
        BodyLongPeriodTotal += TA_CANDLERANGE( BodyLong, i );
        i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx-1 ) {
        BodyShortPeriodTotal += TA_CANDLERANGE( BodyShort, i );
        i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: white candle, usually long
    * - second candle: small black real body
    * - gap between the first and the second candle's real bodies
    * - third candle: black candle with a real body that engulfs the preceding candle
    *   and closes above the white candle's close
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings
    * outInteger is negative (-1 to -100): upside gap two crows is always bearish;
    * the user should consider that an upside gap two crows is significant when it appears in an uptrend,
    * while this function does not consider the trend
    */
   outIdx = 0;
#ifdef TA_LIB_PRO
      /* Section for code distributed with TA-Lib Pro only. */
#else
   do
   {
        if( TA_CANDLECOLOR(i-2) == 1 &&                                                             // 1st: white
            TA_REALBODY(i-2) > TA_CANDLEAVERAGE( BodyLong, BodyLongPeriodTotal, i-2 ) &&         //      long
            TA_CANDLECOLOR(i-1) == -1 &&                                                            // 2nd: black
            TA_REALBODY(i-1) <= TA_CANDLEAVERAGE( BodyShort, BodyShortPeriodTotal, i-1 ) &&      //      short
            TA_REALBODYGAPUP(i-1,i-2) &&                                                            //      gapping up
            TA_CANDLECOLOR(i) == -1 &&                                                              // 3rd: black
            inOpen[i] > inOpen[i-1] && inClose[i] < inClose[i-1] &&                                 // 3rd: engulfing prior rb
            inClose[i] > inClose[i-2]                                                               //      closing above 1st
          )
            outInteger[outIdx++] = -100;
        else
            outInteger[outIdx++] = 0;
        /* add the current range and subtract the first range: this is done after the pattern recognition
         * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
         */
        BodyLongPeriodTotal += TA_CANDLERANGE( BodyLong, i-2 ) - TA_CANDLERANGE( BodyLong, BodyLongTrailingIdx );
        BodyShortPeriodTotal += TA_CANDLERANGE( BodyShort, i-1 ) - TA_CANDLERANGE( BodyShort, BodyShortTrailingIdx );
        i++;
        BodyLongTrailingIdx++;
        BodyShortTrailingIdx++;
   } while( i <= endIdx );
#endif

   /* All done. Indicate the output limits and return. */
   VALUE_HANDLE_DEREF(outNBElement) = outIdx;
   VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;

   return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
}

/**** START GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/
/* Generated */
/* Generated */ #define  USE_SINGLE_PRECISION_INPUT
/* Generated */ #undef  TA_LIB_PRO
/* Generated */ #if !defined( _MANAGED ) && !defined( _JAVA )
/* Generated */    #undef   TA_PREFIX
/* Generated */    #define  TA_PREFIX(x) TA_S_##x
/* Generated */ #endif
/* Generated */ #undef   INPUT_TYPE
/* Generated */ #define  INPUT_TYPE float
/* Generated */ #if defined( _MANAGED ) && defined( USE_SUBARRAY )
/* Generated */ enum class Core::RetCode Core::CdlUpsideGap2Crows( int    startIdx,
/* Generated */                                                    int    endIdx,
/* Generated */                                                    SubArray<float>^ inOpen,
/* Generated */                                                    SubArray<float>^ inHigh,
/* Generated */                                                    SubArray<float>^ inLow,
/* Generated */                                                    SubArray<float>^ inClose,
/* Generated */                                                    [Out]int%    outBegIdx,
/* Generated */                                                    [Out]int%    outNBElement,
/* Generated */                                                    SubArray<int>^  outInteger )
/* Generated */ #elif defined( _MANAGED )
/* Generated */ enum class Core::RetCode Core::CdlUpsideGap2Crows( int    startIdx,
/* Generated */                                                    int    endIdx,
/* Generated */                                                    cli::array<float>^ inOpen,
/* Generated */                                                    cli::array<float>^ inHigh,
/* Generated */                                                    cli::array<float>^ inLow,
/* Generated */                                                    cli::array<float>^ inClose,
/* Generated */                                                    [Out]int%    outBegIdx,
/* Generated */                                                    [Out]int%    outNBElement,
/* Generated */                                                    cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public RetCode cdlUpsideGap2Crows( int    startIdx,
/* Generated */                                    int    endIdx,
/* Generated */                                    float        inOpen[],
/* Generated */                                    float        inHigh[],
/* Generated */                                    float        inLow[],
/* Generated */                                    float        inClose[],
/* Generated */                                    MInteger     outBegIdx,
/* Generated */                                    MInteger     outNBElement,
/* Generated */                                    int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_CDLUPSIDEGAP2CROWS( int    startIdx,
/* Generated */                                     int    endIdx,
/* Generated */                                     const float  inOpen[],
/* Generated */                                     const float  inHigh[],
/* Generated */                                     const float  inLow[],
/* Generated */                                     const float  inClose[],
/* Generated */                                     int          *outBegIdx,
/* Generated */                                     int          *outNBElement,
/* Generated */                                     int           outInteger[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */     double BodyShortPeriodTotal, BodyLongPeriodTotal;
/* Generated */     int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_START_INDEX,OutOfRangeStartIndex);
/* Generated */     if( endIdx < 0 || endIdx < startIdx)
/* Generated */        return ENUM_VALUE(RetCode,TA_OUT_OF_RANGE_END_INDEX,OutOfRangeEndIndex);
/* Generated */     #if !defined(_JAVA)
/* Generated */     if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif
/* Generated */     #if !defined(_JAVA)
/* Generated */     if( !outInteger )
/* Generated */        return ENUM_VALUE(RetCode,TA_BAD_PARAM,BadParam);
/* Generated */     #endif
/* Generated */  #endif
/* Generated */    lookbackTotal = LOOKBACK_CALL(CDLUPSIDEGAP2CROWS)();
/* Generated */    if( startIdx < lookbackTotal )
/* Generated */       startIdx = lookbackTotal;
/* Generated */    if( startIdx > endIdx )
/* Generated */    {
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outNBElement);
/* Generated */       return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */    }
/* Generated */    BodyLongPeriodTotal = 0;
/* Generated */    BodyShortPeriodTotal = 0;
/* Generated */    BodyLongTrailingIdx = startIdx -2 - TA_CANDLEAVGPERIOD(BodyLong);
/* Generated */    BodyShortTrailingIdx = startIdx -1 - TA_CANDLEAVGPERIOD(BodyShort);
/* Generated */    i = BodyLongTrailingIdx;
/* Generated */    while( i < startIdx-2 ) {
/* Generated */         BodyLongPeriodTotal += TA_CANDLERANGE( BodyLong, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = BodyShortTrailingIdx;
/* Generated */    while( i < startIdx-1 ) {
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( BodyShort, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = startIdx;
/* Generated */    outIdx = 0;
/* Generated */ #ifdef TA_LIB_PRO
/* Generated */ #else
/* Generated */    do
/* Generated */    {
/* Generated */         if( TA_CANDLECOLOR(i-2) == 1 &&                                                             // 1st: white
/* Generated */             TA_REALBODY(i-2) > TA_CANDLEAVERAGE( BodyLong, BodyLongPeriodTotal, i-2 ) &&         //      long
/* Generated */             TA_CANDLECOLOR(i-1) == -1 &&                                                            // 2nd: black
/* Generated */             TA_REALBODY(i-1) <= TA_CANDLEAVERAGE( BodyShort, BodyShortPeriodTotal, i-1 ) &&      //      short
/* Generated */             TA_REALBODYGAPUP(i-1,i-2) &&                                                            //      gapping up
/* Generated */             TA_CANDLECOLOR(i) == -1 &&                                                              // 3rd: black
/* Generated */             inOpen[i] > inOpen[i-1] && inClose[i] < inClose[i-1] &&                                 // 3rd: engulfing prior rb
/* Generated */             inClose[i] > inClose[i-2]                                                               //      closing above 1st
/* Generated */           )
/* Generated */             outInteger[outIdx++] = -100;
/* Generated */         else
/* Generated */             outInteger[outIdx++] = 0;
/* Generated */         BodyLongPeriodTotal += TA_CANDLERANGE( BodyLong, i-2 ) - TA_CANDLERANGE( BodyLong, BodyLongTrailingIdx );
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( BodyShort, i-1 ) - TA_CANDLERANGE( BodyShort, BodyShortTrailingIdx );
/* Generated */         i++;
/* Generated */         BodyLongTrailingIdx++;
/* Generated */         BodyShortTrailingIdx++;
/* Generated */    } while( i <= endIdx );
/* Generated */ #endif
/* Generated */    VALUE_HANDLE_DEREF(outNBElement) = outIdx;
/* Generated */    VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
/* Generated */    return ENUM_VALUE(RetCode,TA_SUCCESS,Success);
/* Generated */ }
/* Generated */
/* Generated */ #if defined( _MANAGED )
/* Generated */ }}} // Close namespace TicTacTec.TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

