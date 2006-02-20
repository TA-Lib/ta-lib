/* TA-LIB Copyright (c) 1999-2006, Mario Fortier
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
 *  011505 AC   Creation           
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
/* Generated */    #define TA_INTERNAL_ERROR(Id) (NAMESPACE(TA_RetCode)TA_INTERNAL_ERROR)
/* Generated */    namespace TA { namespace Lib {
/* Generated */ #elif defined( _JAVA )
/* Generated */    #include "ta_defs.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (NAMESPACE(TA_RetCode)TA_INTERNAL_ERROR)
/* Generated */ #else
/* Generated */    #include <string.h>
/* Generated */    #include <math.h>
/* Generated */    #include "ta_func.h"
/* Generated */    #include "ta_trace.h"
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
/* Generated */ int Core::CDLTAKURI_Lookback( void )
/* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int CDLTAKURI_Lookback(  )
/* Generated */ 
/* Generated */ #else
/* Generated */ int TA_CDLTAKURI_Lookback( void )
/* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ /* No parameters to validate. */
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/

   /* insert lookback code here. */
    return max( max( TA_CANDLEAVGPERIOD(TA_BodyDoji), TA_CANDLEAVGPERIOD(TA_ShadowVeryShort) ),
                TA_CANDLEAVGPERIOD(TA_ShadowVeryLong)
            );
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_CDLTAKURI - Takuri (Dragonfly Doji with very long lower shadow)
 * 
 * Input  = Open, High, Low, Close
 * Output = int
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum class Core::TA_RetCode Core::CDLTAKURI( int    startIdx,
/* Generated */                                              int    endIdx,
/* Generated */                                              cli::array<double>^ inOpen,
/* Generated */                                              cli::array<double>^ inHigh,
/* Generated */                                              cli::array<double>^ inLow,
/* Generated */                                              cli::array<double>^ inClose,
/* Generated */                                              [Out]int%    outBegIdx,
/* Generated */                                              [Out]int%    outNbElement,
/* Generated */                                              cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public TA_RetCode CDLTAKURI( int    startIdx,
/* Generated */                              int    endIdx,
/* Generated */                              double       inOpen[],
/* Generated */                              double       inHigh[],
/* Generated */                              double       inLow[],
/* Generated */                              double       inClose[],
/* Generated */                              MInteger     outBegIdx,
/* Generated */                              MInteger     outNbElement,
/* Generated */                              int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_CDLTAKURI( int    startIdx,
/* Generated */                          int    endIdx,
/* Generated */                          const double inOpen[],
/* Generated */                          const double inHigh[],
/* Generated */                          const double inLow[],
/* Generated */                          const double inClose[],
/* Generated */                          int          *outBegIdx,
/* Generated */                          int          *outNbElement,
/* Generated */                          int           outInteger[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
    double BodyDojiPeriodTotal, ShadowVeryShortPeriodTotal, ShadowVeryLongPeriodTotal;
    int i, outIdx, BodyDojiTrailingIdx, ShadowVeryShortTrailingIdx, ShadowVeryLongTrailingIdx, lookbackTotal;

/**** START GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */ 
/* Generated */    /* Validate the requested output range. */
/* Generated */    if( startIdx < 0 )
/* Generated */       return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_START_INDEX;
/* Generated */    if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */       return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_END_INDEX;
/* Generated */ 
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    /* Verify required price component. */
/* Generated */    if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */       return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA)*/
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    if( !outInteger )
/* Generated */       return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA) */
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = LOOKBACK_CALL(CDLTAKURI)();

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
      VALUE_HANDLE_DEREF_TO_ZERO(outNbElement);
      return NAMESPACE(TA_RetCode)TA_SUCCESS;
   }

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   BodyDojiPeriodTotal = 0;
   BodyDojiTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyDoji);
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryShort);
   ShadowVeryLongPeriodTotal = 0;
   ShadowVeryLongTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryLong);
   
   i = BodyDojiTrailingIdx;
   while( i < startIdx ) {
        BodyDojiPeriodTotal += TA_CANDLERANGE( TA_BodyDoji, i );
        i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
        ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i );
        i++;
   }
   i = ShadowVeryLongTrailingIdx;
   while( i < startIdx ) {
        ShadowVeryLongPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryLong, i );
        i++;
   }

   /* Proceed with the calculation for the requested range.
    *
    * Must have:
    * - doji body
    * - open and close at the high of the day = no or very short upper shadow
    * - very long lower shadow
    * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
    * outInteger is always positive (1 to 100) but this does not mean it is bullish: takuri must be considered
    * relatively to the trend
    */
   outIdx = 0;
   do
   {
        if( TA_REALBODY(i) <= TA_CANDLEAVERAGE( TA_BodyDoji, BodyDojiPeriodTotal, i ) &&
            TA_UPPERSHADOW(i) < TA_CANDLEAVERAGE( TA_ShadowVeryShort, ShadowVeryShortPeriodTotal, i ) &&
            TA_LOWERSHADOW(i) > TA_CANDLEAVERAGE( TA_ShadowVeryLong, ShadowVeryLongPeriodTotal, i )
          )
            outInteger[outIdx++] = 100;
        else
            outInteger[outIdx++] = 0;
        /* add the current range and subtract the first range: this is done after the pattern recognition 
         * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
         */
        BodyDojiPeriodTotal += TA_CANDLERANGE( TA_BodyDoji, i ) - TA_CANDLERANGE( TA_BodyDoji, BodyDojiTrailingIdx );
        ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i ) 
                                    - TA_CANDLERANGE( TA_ShadowVeryShort, ShadowVeryShortTrailingIdx );
        ShadowVeryLongPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryLong, i ) 
                                   - TA_CANDLERANGE( TA_ShadowVeryLong, ShadowVeryLongTrailingIdx );
        i++; 
        BodyDojiTrailingIdx++;
        ShadowVeryShortTrailingIdx++;
        ShadowVeryLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   VALUE_HANDLE_DEREF(outNbElement) = outIdx;
   VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;

   return NAMESPACE(TA_RetCode)TA_SUCCESS;
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
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum class Core::TA_RetCode Core::CDLTAKURI( int    startIdx,
/* Generated */                                              int    endIdx,
/* Generated */                                              cli::array<float>^ inOpen,
/* Generated */                                              cli::array<float>^ inHigh,
/* Generated */                                              cli::array<float>^ inLow,
/* Generated */                                              cli::array<float>^ inClose,
/* Generated */                                              [Out]int%    outBegIdx,
/* Generated */                                              [Out]int%    outNbElement,
/* Generated */                                              cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public TA_RetCode CDLTAKURI( int    startIdx,
/* Generated */                              int    endIdx,
/* Generated */                              float        inOpen[],
/* Generated */                              float        inHigh[],
/* Generated */                              float        inLow[],
/* Generated */                              float        inClose[],
/* Generated */                              MInteger     outBegIdx,
/* Generated */                              MInteger     outNbElement,
/* Generated */                              int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_CDLTAKURI( int    startIdx,
/* Generated */                            int    endIdx,
/* Generated */                            const float  inOpen[],
/* Generated */                            const float  inHigh[],
/* Generated */                            const float  inLow[],
/* Generated */                            const float  inClose[],
/* Generated */                            int          *outBegIdx,
/* Generated */                            int          *outNbElement,
/* Generated */                            int           outInteger[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */     double BodyDojiPeriodTotal, ShadowVeryShortPeriodTotal, ShadowVeryLongPeriodTotal;
/* Generated */     int i, outIdx, BodyDojiTrailingIdx, ShadowVeryShortTrailingIdx, ShadowVeryLongTrailingIdx, lookbackTotal;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_START_INDEX;
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_END_INDEX;
/* Generated */     #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */     if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */        return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */     #endif 
/* Generated */     #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */     if( !outInteger )
/* Generated */        return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */     #endif 
/* Generated */  #endif 
/* Generated */    lookbackTotal = LOOKBACK_CALL(CDLTAKURI)();
/* Generated */    if( startIdx < lookbackTotal )
/* Generated */       startIdx = lookbackTotal;
/* Generated */    if( startIdx > endIdx )
/* Generated */    {
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outNbElement);
/* Generated */       return NAMESPACE(TA_RetCode)TA_SUCCESS;
/* Generated */    }
/* Generated */    BodyDojiPeriodTotal = 0;
/* Generated */    BodyDojiTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyDoji);
/* Generated */    ShadowVeryShortPeriodTotal = 0;
/* Generated */    ShadowVeryShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryShort);
/* Generated */    ShadowVeryLongPeriodTotal = 0;
/* Generated */    ShadowVeryLongTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryLong);
/* Generated */    i = BodyDojiTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         BodyDojiPeriodTotal += TA_CANDLERANGE( TA_BodyDoji, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = ShadowVeryShortTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = ShadowVeryLongTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         ShadowVeryLongPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryLong, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    outIdx = 0;
/* Generated */    do
/* Generated */    {
/* Generated */         if( TA_REALBODY(i) <= TA_CANDLEAVERAGE( TA_BodyDoji, BodyDojiPeriodTotal, i ) &&
/* Generated */             TA_UPPERSHADOW(i) < TA_CANDLEAVERAGE( TA_ShadowVeryShort, ShadowVeryShortPeriodTotal, i ) &&
/* Generated */             TA_LOWERSHADOW(i) > TA_CANDLEAVERAGE( TA_ShadowVeryLong, ShadowVeryLongPeriodTotal, i )
/* Generated */           )
/* Generated */             outInteger[outIdx++] = 100;
/* Generated */         else
/* Generated */             outInteger[outIdx++] = 0;
/* Generated */         BodyDojiPeriodTotal += TA_CANDLERANGE( TA_BodyDoji, i ) - TA_CANDLERANGE( TA_BodyDoji, BodyDojiTrailingIdx );
/* Generated */         ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i ) 
/* Generated */                                     - TA_CANDLERANGE( TA_ShadowVeryShort, ShadowVeryShortTrailingIdx );
/* Generated */         ShadowVeryLongPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryLong, i ) 
/* Generated */                                    - TA_CANDLERANGE( TA_ShadowVeryLong, ShadowVeryLongTrailingIdx );
/* Generated */         i++; 
/* Generated */         BodyDojiTrailingIdx++;
/* Generated */         ShadowVeryShortTrailingIdx++;
/* Generated */         ShadowVeryLongTrailingIdx++;
/* Generated */    } while( i <= endIdx );
/* Generated */    VALUE_HANDLE_DEREF(outNbElement) = outIdx;
/* Generated */    VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
/* Generated */    return NAMESPACE(TA_RetCode)TA_SUCCESS;
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }} // Close namespace TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

